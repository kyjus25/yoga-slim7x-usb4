#!/usr/bin/env python3
"""Dump SSDT and search for USB4 methods."""
import os
import sys
import struct
sys.path.insert(0, "/home/kyjus25")
from mmio import MMIO

if os.geteuid() != 0:
    print("must run as root", file=sys.stderr)
    sys.exit(1)

m = MMIO()
SSDT = 0xd57bb018
length = m.r32(SSDT + 4)
print(f"Dumping SSDT ({length} bytes)...", file=sys.stderr)

data = bytearray()
for off in range(0, length, 4):
    try:
        v = m.r32(SSDT + off)
        data.extend(struct.pack("<I", v))
    except:
        print(f"Failed at 0x{off:x}", file=sys.stderr)
        break

# Write to file
with open("/tmp/ssdt.aml", "wb") as f:
    f.write(data[:length])

print(f"Dumped {len(data[:length])} bytes to /tmp/ssdt.aml", file=sys.stderr)

# Search for interesting strings
text = bytes(data[:length])
for keyword in [b"_PS0", b"USB4", b"QCOM", b"_DSM", b"Power"]:
    count = text.count(keyword)
    if count > 0:
        print(f"Found '{keyword.decode()}': {count} occurrences")

m.close()

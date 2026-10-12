#!/usr/bin/env python3
"""Get DSDT from FACP and dump it."""
import os
import sys
import struct
sys.path.insert(0, "/home/kyjus25")
from mmio import MMIO

if os.geteuid() != 0:
    print("must run as root", file=sys.stderr)
    sys.exit(1)

m = MMIO()

FACP = 0xd57bec18

# FACP structure:
# 0x00: Signature "FACP" (4)
# 0x04: Length (4)
# 0x1C: DSDT address (4 bytes, 32-bit)
# 0x64: X_DSDT address (8 bytes, 64-bit)

facp_len = m.r32(FACP + 4)
dsdt_32 = m.r32(FACP + 0x1C)
dsdt_64_lo = m.r32(FACP + 0x64)
dsdt_64_hi = m.r32(FACP + 0x68)
dsdt_64 = (dsdt_64_hi << 32) | dsdt_64_lo

print(f"FACP length: 0x{facp_len:x}")
print(f"DSDT (32-bit): 0x{dsdt_32:08x}")
print(f"DSDT (64-bit): 0x{dsdt_64:x}")

# Use 64-bit if non-zero, else 32-bit
dsdt = dsdt_64 if dsdt_64 != 0 else dsdt_32
print(f"Using DSDT: 0x{dsdt:x}")

# Read DSDT header
dsdt_len = m.r32(dsdt + 4)
print(f"DSDT length: 0x{dsdt_len:x} ({dsdt_len} bytes)")

# Dump DSDT to file
print(f"Dumping DSDT...")
with open("/tmp/dsdt.aml", "wb") as f:
    for off in range(0, dsdt_len, 4):
        try:
            v = m.r32(dsdt + off)
            f.write(struct.pack("<I", v))
        except:
            print(f"Failed at offset 0x{off:x}")
            break

print(f"DSDT dumped to /tmp/dsdt.aml ({os.path.getsize('/tmp/dsdt.aml')} bytes)")
m.close()

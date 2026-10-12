#!/usr/bin/env python3
"""Read ACPI RSDP and find DSDT."""
import os
import sys
import struct
sys.path.insert(0, "/home/kyjus25")
from mmio import MMIO

if os.geteuid() != 0:
    print("must run as root", file=sys.stderr)
    sys.exit(1)

m = MMIO()

RSDP = 0xd57be018

# RSDP structure (ACPI 2.0):
# 0x00: Signature "RSD PTR " (8 bytes)
# 0x08: Checksum (1 byte)
# 0x09: OEMID (6 bytes)
# 0x0F: Revision (1 byte)
# 0x10: RSDT address (4 bytes)
# 0x14: Length (4 bytes)
# 0x18: XSDT address (8 bytes)
# 0x20: Extended checksum
# 0x21: Reserved

sig = m.r32(RSDP)
sig2 = m.r32(RSDP + 4)
print(f"RSDP sig: 0x{sig:08x} 0x{sig2:08x}")
# Should be "RSD PTR " = 0x20525352, 0x20525450

rsdt = m.r32(RSDP + 0x10)
xsdt_lo = m.r32(RSDP + 0x18)
xsdt_hi = m.r32(RSDP + 0x1c)
xsdt = (xsdt_hi << 32) | xsdt_lo

print(f"RSDT: 0x{rsdt:08x}")
print(f"XSDT: 0x{xsdt:x}")

# Read XSDT header to get entry count
# XSDT: 0x00 Signature "XSDT" (4), 0x04 Length (4), ...
xsdt_len = m.r32(xsdt + 4)
print(f"XSDT length: 0x{xsdt_len:x}")
num_entries = (xsdt_len - 36) // 8
print(f"Entries: {num_entries}")

# Find DSDT (signature "DSDT")
for i in range(min(num_entries, 20)):
    entry_lo = m.r32(xsdt + 36 + i*8)
    entry_hi = m.r32(xsdt + 36 + i*8 + 4)
    entry = (entry_hi << 32) | entry_lo
    # Read signature
    sig = m.r32(entry)
    # Convert to chars
    sig_str = struct.pack("<I", sig).decode('ascii', errors='ignore')
    print(f"  Entry {i}: 0x{entry:x} sig='{sig_str}'")
    if sig_str == "DSDT":
        print(f"  *** Found DSDT at 0x{entry:x} ***")
        # Get DSDT length
        dsdt_len = m.r32(entry + 4)
        print(f"  DSDT length: 0x{dsdt_len:x} ({dsdt_len} bytes)")

m.close()

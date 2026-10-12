#!/usr/bin/env python3
"""Search for ACPI RSDP in memory."""
import os
import sys
sys.path.insert(0, "/home/kyjus25")
from mmio import MMIO

if os.geteuid() != 0:
    print("must run as root", file=sys.stderr)
    sys.exit(1)

m = MMIO()

# RSDP signature: "RSD PTR " (8 bytes)
# Search BIOS area 0xE0000-0xFFFFF
print("Searching for RSDP...")
found = False
for addr in range(0xE0000, 0x100000, 16):
    try:
        # Read 8 bytes
        v1 = m.r32(addr)
        v2 = m.r32(addr + 4)
        # Check for "RSD PTR " = 0x2052545020445352 (little endian)
        # "RSD " = 0x20525352, "PTR " = 0x20525450
        if v1 == 0x20525352 and v2 == 0x20525450:
            print(f"Found RSDP at 0x{addr:x}!")
            found = True
            break
    except:
        pass

if not found:
    print("RSDP not found in BIOS area")

m.close()

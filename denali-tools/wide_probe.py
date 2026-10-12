#!/usr/bin/env python3
"""Wide probe of GCC for USB4-related registers."""
import os
import sys
sys.path.insert(0, "/home/kyjus25")
from mmio import MMIO

if os.geteuid() != 0:
    print("must run as root", file=sys.stderr)
    sys.exit(1)

m = MMIO()
GCC = 0x100000

# Probe 0x90000-0xa0000 (64KB) for non-zero values
print("Probing GCC 0x90000-0xa0000...")
found = 0
for off in range(0x90000, 0xa0000, 0x100):
    try:
        v = m.r32(GCC + off)
        if v != 0 and v != 0x80000000:
            # Look for GDSC-like patterns (bit 0 set, or specific values)
            print(f"  0x{off:x}: 0x{v:08x}")
            found += 1
            if found > 30:
                break
    except:
        pass
m.close()
print(f"Found {found} non-zero registers")

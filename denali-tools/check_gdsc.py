#!/usr/bin/env python3
"""Check GCC USB4 GDSC status."""
import os
import sys
sys.path.insert(0, "/home/kyjus25")
from mmio import MMIO

if os.geteuid() != 0:
    print("must run as root", file=sys.stderr)
    sys.exit(1)

# GCC base from driver MMIO table (0x34980000, size 0x4f0000)
GCC_BASE = 0x34980000
GDSC_OFFSET = 0x9f004

m = MMIO()
addr = GCC_BASE + GDSC_OFFSET
v = m.r32(addr)
print(f"GCC_USB4_0_GDSCR @ 0x{addr:x}: 0x{v:08x}")
print(f"  Bit 0 (EN): {(v & 0x1)}")
print(f"  Power status: {'ON' if (v & 0x1) else 'OFF'}")
m.close()

#!/usr/bin/env python3
"""Enable USB4 AHB clock."""
import os
import sys
sys.path.insert(0, "/home/kyjus25")
from mmio import MMIO

if os.geteuid() != 0:
    print("must run as root", file=sys.stderr)
    sys.exit(1)

m = MMIO()
GCC = 0x100000
AHB_CLK = GCC + 0x9f0a8

v = m.r32(AHB_CLK)
print(f"Before: 0x{v:08x}")
# Set BIT(0) to enable, preserve other bits
m.w32(AHB_CLK, v | 0x1)
v2 = m.r32(AHB_CLK)
print(f"After:  0x{v2:08x}")
print(f"Enabled: {bool(v2 & 0x1)}")
m.close()

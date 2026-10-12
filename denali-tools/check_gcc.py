#!/usr/bin/env python3
"""Check GCC at 0x100000."""
import os
import sys
sys.path.insert(0, "/home/kyjus25")
from mmio import MMIO

if os.geteuid() != 0:
    print("must run as root", file=sys.stderr)
    sys.exit(1)

m = MMIO()
# GCC base 0x100000, try reading a few offsets
for off in [0x0, 0x1000, 0x9f004]:
    try:
        v = m.r32(0x100000 + off)
        print(f"0x{0x100000+off:x}: 0x{v:08x} - READABLE")
    except Exception as e:
        print(f"0x{0x100000+off:x}: FAILED")
m.close()

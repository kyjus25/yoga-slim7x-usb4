#!/usr/bin/env python3
"""Check 0x1fc0000 clock controller."""
import os
import sys
sys.path.insert(0, "/home/kyjus25")
from mmio import MMIO

if os.geteuid() != 0:
    print("must run as root", file=sys.stderr)
    sys.exit(1)

m = MMIO()
# Try reading from 0x1fc0000
for off in [0x0, 0x1000, 0x90000]:
    try:
        v = m.r32(0x1fc0000 + off)
        print(f"0x{0x1fc0000+off:x}: 0x{v:08x} READABLE")
    except:
        print(f"0x{0x1fc0000+off:x}: FAILED")
m.close()

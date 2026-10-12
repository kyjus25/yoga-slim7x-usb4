#!/usr/bin/env python3
"""Enable all USB4 clocks."""
import os
import sys
sys.path.insert(0, "/home/kyjus25")
from mmio import MMIO

if os.geteuid() != 0:
    print("must run as root", file=sys.stderr)
    sys.exit(1)

m = MMIO()
GCC = 0x100000

clocks = [
    (0x9f0a8, "ahb"),
    (0x9f054, "sys"),
    (0x9f018, "master"),
    (0x9f044, "sb_if"),
    (0x9f088, "tmu"),
]

for off, name in clocks:
    addr = GCC + off
    v = m.r32(addr)
    # Set BIT(0)
    m.w32(addr, v | 0x1)
    v2 = m.r32(addr)
    print(f"{name:8s} @ 0x{off:x}: 0x{v:08x} -> 0x{v2:08x} {'OK' if (v2 & 0x1) else 'FAIL'}")

m.close()

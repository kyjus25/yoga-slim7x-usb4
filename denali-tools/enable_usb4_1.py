#!/usr/bin/env python3
"""Enable USB4_1 clocks."""
import os
import sys
sys.path.insert(0, "/home/kyjus25")
from mmio import MMIO

if os.geteuid() != 0:
    print("must run as root", file=sys.stderr)
    sys.exit(1)

m = MMIO()
GCC = 0x100000

# USB4_1 clocks (0x2bxxx, same offsets as 0x9fxxx)
clocks = [
    (0x2b0a8, "ahb1"),
    (0x2b054, "sys1"),
    (0x2b018, "master1"),
    (0x2b044, "sb_if1"),
    (0x2b088, "tmu1"),
]

for off, name in clocks:
    addr = GCC + off
    try:
        v = m.r32(addr)
        m.w32(addr, v | 0x1)
        v2 = m.r32(addr)
        print(f"{name:8s}: 0x{v:08x} -> 0x{v2:08x} {'OK' if (v2 & 0x1) else 'FAIL'}")
    except:
        print(f"{name:8s}: FAILED")

# Test UC
m.w32(0x15600000, 0x12345678)
v = m.r32(0x15600000)
print(f"\nUC: 0x{v:08x} {'SUCCESS!' if v == 0x12345678 else 'FAILED'}")
m.close()

#!/usr/bin/env python3
"""Enable all USB4 clocks in 0x9f000 region."""
import os
import sys
sys.path.insert(0, "/home/kyjus25")
from mmio import MMIO

if os.geteuid() != 0:
    print("must run as root", file=sys.stderr)
    sys.exit(1)

m = MMIO()
GCC = 0x100000

# All offsets that showed 0x80000000 (disabled)
offsets = [0x9f024, 0x9f048, 0x9f060, 0x9f070, 0x9f08c, 0x9f0a4,
           0x9f0b0, 0x9f0c0, 0x9f0cc, 0x9f0d0, 0x9f0d8, 0x9f0e8]

for off in offsets:
    addr = GCC + off
    v = m.r32(addr)
    if v & 0x80000000:  # If it's a clock register (bit 31 set)
        m.w32(addr, v | 0x1)
        v2 = m.r32(addr)
        print(f"0x{off:x}: 0x{v:08x} -> 0x{v2:08x}")

# Test UC
m.w32(0x15600000, 0x12345678)
v = m.r32(0x15600000)
print(f"\nUC test: 0x{v:08x} {'SUCCESS!' if v == 0x12345678 else 'FAILED'}")
m.close()

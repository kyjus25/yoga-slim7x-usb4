#!/usr/bin/env python3
"""Check and deassert USB4 BCR (reset)."""
import os
import sys
sys.path.insert(0, "/home/kyjus25")
from mmio import MMIO

if os.geteuid() != 0:
    print("must run as root", file=sys.stderr)
    sys.exit(1)

m = MMIO()
GCC = 0x100000
BCR = GCC + 0x9f000

v = m.r32(BCR)
print(f"BCR @ 0x9f000: 0x{v:08x}")
# Deassert reset: write 0
m.w32(BCR, 0x0)
v2 = m.r32(BCR)
print(f"After write 0: 0x{v2:08x}")

# Test UC
m.w32(0x15600000, 0x12345678)
v3 = m.r32(0x15600000)
print(f"UC test: 0x{v3:08x} {'SUCCESS!' if v3 == 0x12345678 else 'FAILED'}")
m.close()

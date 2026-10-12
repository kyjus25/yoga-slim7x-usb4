#!/usr/bin/env python3
"""Probe GCC 0x9f000-0x9f100 for USB4 registers."""
import os
import sys
sys.path.insert(0, "/home/kyjus25")
from mmio import MMIO

if os.geteuid() != 0:
    print("must run as root", file=sys.stderr)
    sys.exit(1)

m = MMIO()
GCC = 0x100000

print("Probing GCC USB4 region (0x9f000-0x9f100):")
for off in range(0x9f000, 0x9f100, 0x4):
    try:
        v = m.r32(GCC + off)
        # Only print non-zero or interesting values
        if v != 0:
            print(f"  0x{off:x}: 0x{v:08x}")
    except:
        pass
m.close()

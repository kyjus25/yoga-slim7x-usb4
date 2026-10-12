#!/usr/bin/env python3
"""Check UC gate registers."""
import os
import sys
sys.path.insert(0, "/home/kyjus25")
from mmio import MMIO

if os.geteuid() != 0:
    print("must run as root", file=sys.stderr)
    sys.exit(1)

UC_BASE = 0x15600000
m = MMIO()

for off, name in [(0xd064, "gate1"), (0x8018, "gate2"), (0x22000, "GO"), (0x13000, "burst")]:
    v = m.r32(UC_BASE + off)
    print(f"UC+0x{off:05x} ({name}): 0x{v:08x}")

m.close()

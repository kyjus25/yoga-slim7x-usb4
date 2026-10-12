#!/usr/bin/env python3
"""Check UC status registers. Run as root."""
import os
import sys

sys.path.insert(0, "/home/kyjus25")
from mmio import MMIO

if os.geteuid() != 0:
    print("must run as root", file=sys.stderr)
    sys.exit(1)

UC_BASE = 0x15600000
ROUTER = 0x1563F000

m = MMIO()

print("=== UC window ===")
for off in [0x0, 0x4, 0x8, 0x18, 0x8018, 0xd064, 0x13000, 0x22000]:
    try:
        v = m.r32(UC_BASE + off)
        print("UC+0x%05x: 0x%08x" % (off, v))
    except Exception as e:
        print("UC+0x%05x: ERROR %s" % (off, e))

print("\n=== Router/NHI ===")
for off in [0x0, 0x18, 0x20a8]:
    try:
        v = m.r32(ROUTER + off)
        print("ROUTER+0x%x: 0x%08x" % (off, v))
    except Exception as e:
        print("ROUTER+0x%x: ERROR %s" % (off, e))

m.close()

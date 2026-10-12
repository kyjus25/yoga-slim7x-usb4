#!/usr/bin/env python3
"""Wide probe for GDSC-like registers."""
import os
import sys
sys.path.insert(0, "/home/kyjus25")
from mmio import MMIO

if os.geteuid() != 0:
    print("must run as root", file=sys.stderr)
    sys.exit(1)

m = MMIO()
GCC = 0x100000

# Probe 0x80000-0xb0000, look for values that could be GDSCs
# GDSC pattern: bit 0 = enable, often 0x0022f801 or similar
print("Scanning for GDSC candidates...")
for off in range(0x80000, 0xb0000, 0x1000):
    try:
        v = m.r32(GCC + off + 0x4)  # GDSCs often at +0x4 offset in a block
        # Look for the 0x0022f801 pattern or similar
        if (v & 0xFF) == 0x01 and (v >> 16) & 0xFF == 0x22:
            print(f"  Candidate GDSC @ 0x{off+0x4:x}: 0x{v:08x}")
    except:
        pass
m.close()
print("Done")

#!/usr/bin/env python3
"""Probe MMIO table regions to identify GCC."""
import os
import sys
sys.path.insert(0, "/home/kyjus25")
from mmio import MMIO

if os.geteuid() != 0:
    print("must run as root", file=sys.stderr)
    sys.exit(1)

# From driver MMIO table at 0x4531e
regions = [
    (0x0f100000, "TLMM"),
    (0x34980000, "unknown1"),
    (0x34d00000, "unknown2"),
    (0x34d40000, "unknown3"),
    (0x357c0000, "unknown4"),
    (0x35b80000, "unknown5"),
    (0x35d80000, "unknown6"),
    (0x35e80000, "unknown7"),
]

m = MMIO()
for addr, name in regions:
    if addr == 0x0f100000:
        continue  # Skip TLMM, known good
    try:
        v = m.r32(addr)
        print(f"0x{addr:08x} ({name}): 0x{v:08x} - READABLE")
    except Exception as e:
        print(f"0x{addr:08x} ({name}): FAILED - {e}")
m.close()

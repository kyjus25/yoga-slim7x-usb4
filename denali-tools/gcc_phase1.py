#!/usr/bin/env python3
"""Phase 1: Compare X1E80100 GCC clocks against Denali hardware.
Reads each clock branch offset from the X1E driver and checks Denali.
Safe: only reads known offsets, no blind probing.
"""
import os
import sys
import re
sys.path.insert(0, "/home/kyjus25")
from mmio import MMIO

if os.geteuid() != 0:
    print("must run as root", file=sys.stderr)
    sys.exit(1)

# Parse gcc-x1e80100.c for clock branch offsets
# We already have the file locally, but we need it on the Surface
# For now, use the known USB4 offsets

m = MMIO()
GCC = 0x100000

# USB4 clock branches from X1E80100 (offsets within GCC)
# Format: (offset, name)
usb4_clocks = [
    # USB4_0
    (0x9f0a8, "usb4_0_ahb"),
    (0x9f054, "usb4_0_sys"),
    (0x9f018, "usb4_0_master"),
    (0x9f044, "usb4_0_sb_if"),
    (0x9f088, "usb4_0_tmu"),
    # USB4_1
    (0x2b0a8, "usb4_1_ahb"),
    (0x2b054, "usb4_1_sys"),
    (0x2b018, "usb4_1_master"),
    (0x2b044, "usb4_1_sb_if"),
    (0x2b088, "usb4_1_tmu"),
    # USB4_2 (from GDSC list, need to find clocks)
    (0x110a8, "usb4_2_ahb?"),
    (0x11054, "usb4_2_sys?"),
]

print("USB4 Clock Branch Status on Denali:")
print("=" * 60)
for off, name in usb4_clocks:
    try:
        v = m.r32(GCC + off)
        # Bit 0 = enable, Bit 31 = halt status?
        enabled = "EN" if (v & 0x1) else "dis"
        print("0x%05x %-15s: 0x%08x [%s]" % (off, name, v, enabled))
    except:
        print("0x%05x %-15s: READ FAILED" % (off, name))

m.close()
print("\nDone")

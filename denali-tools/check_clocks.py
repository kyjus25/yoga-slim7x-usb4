#!/usr/bin/env python3
"""Check USB4 clock enables in GCC."""
import os
import sys
sys.path.insert(0, "/home/kyjus25")
from mmio import MMIO

if os.geteuid() != 0:
    print("must run as root", file=sys.stderr)
    sys.exit(1)

m = MMIO()
GCC = 0x100000

# From X1E80100 gcc driver (may differ on Denali, but try)
# gcc_usb4_0_cfg_ahb_clk: 0x9f0a8, BIT(0)
# gcc_usb4_0_sys_clk: 0x9f0b0? (need to verify)

for off, name in [(0x9f004, "GDSC"), (0x9f0a8, "AHB_CLK")]:
    try:
        v = m.r32(GCC + off)
        print(f"GCC+0x{off:x} ({name}): 0x{v:08x}")
        if off == 0x9f0a8:
            print(f"  Clock enabled: {bool(v & 0x1)}")
    except Exception as e:
        print(f"GCC+0x{off:x}: FAILED")
m.close()

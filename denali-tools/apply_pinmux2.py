#!/usr/bin/env python3
"""Apply USB4 sideband pinmux via /dev/mmio. Run as root."""
import os
import sys

sys.path.insert(0, "/home/kyjus25")
from mmio import MMIO

if os.geteuid() != 0:
    print("must run as root", file=sys.stderr)
    sys.exit(1)

# (gpio, func, pull, drv)  pull: 0=none, 3=up ; drv: 4=10mA
PINS = [
    (163, 140, 0, 0),  # usb0_sbrx
    (164, 141, 3, 4),  # usb0_sbtx
    (165, 141, 0, 4),  # usb0_sbtx
    (172, 144, 0, 0),  # usb1_sbrx
    (173, 145, 3, 4),  # usb1_sbtx
    (174, 145, 0, 4),  # usb1_sbtx
]
TLMM = 0x0F100000

m = MMIO()
ok = True
for gpio, func, pull, drv in PINS:
    phys = TLMM + gpio * 0x1000
    cur = m.r32(phys)
    new = (cur & ~0x3FF) | (func << 2) | pull | (drv << 6)
    m.w32(phys, new)
    ver = m.r32(phys)
    good = (ver & 0x3FF) == (new & 0x3FF)
    ok &= good
    status = "OK" if good else "FAIL"
    print("gpio%d: 0x%08x -> 0x%08x [%s]" % (gpio, cur, ver, status))
m.close()
sys.exit(0 if ok else 1)

#!/usr/bin/env python3
"""Apply USB4 sideband pinmux on Denali. Run as root."""
import mmap, os, struct, sys

if os.geteuid() != 0:
    print("must run as root", file=sys.stderr)
    sys.exit(1)

TLMM = 0x0f100000
# (gpio, func, pull, drv)  pull: 0=none, 3=up ; drv: 4=10mA
pins = [(163,140,0,0),(164,141,3,4),(165,141,0,4),
        (172,144,0,0),(173,145,3,4),(174,145,0,4)]

fd = os.open("/dev/mem", os.O_RDWR | os.O_SYNC)
m = mmap.mmap(fd, 0x400000, offset=TLMM)
allok = True
for gpio, func, pull, drv in pins:
    off = gpio * 0x1000
    m.seek(off)
    cur = struct.unpack("<I", m.read(4))[0]
    new = (cur & ~0x3FF) | (func << 2) | pull | (drv << 6)
    m.seek(off)
    m.write(struct.pack("<I", new))
    m.seek(off)
    ver = struct.unpack("<I", m.read(4))[0]
    ok = (ver & 0x3FF) == (new & 0x3FF)
    allok &= ok
    print(f"gpio{gpio}: 0x{cur:08x} -> 0x{ver:08x} [{'OK' if ok else 'FAIL'}]")
m.close()
os.close(fd)
sys.exit(0 if allok else 1)

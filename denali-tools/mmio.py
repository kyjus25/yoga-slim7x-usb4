#!/usr/bin/env python3
"""MMIO via /dev/mmio (kernel module) with /dev/mem fallback.

Usage:
    from mmio import MMIO
    m = MMIO()
    v = m.r32(0x0f1a3000)
    m.w32(0x0f1a3000, 0x230)
"""
import fcntl
import os
import struct

_MMIO_READ = 0x80104d01    # _IOR('M', 1, 16)
_MMIO_WRITE = 0x40104d02   # _IOW('M', 2, 16)


class MMIO:
    def __init__(self):
        self.fd = None
        try:
            self.fd = os.open("/dev/mmio", os.O_RDWR)
        except OSError:
            self.fd = None
        self._mem = None

    def _via_module(self, phys, value=None):
        if value is None:
            buf = bytearray(struct.pack("<QII", phys, 0, 0))
            fcntl.ioctl(self.fd, _MMIO_READ, buf)
            return struct.unpack("<QII", bytes(buf))[1]
        req = struct.pack("<QII", phys, value, 0)
        fcntl.ioctl(self.fd, _MMIO_WRITE, req)

    def r32(self, phys):
        if self.fd is not None:
            return self._via_module(phys)
        import mmap
        if self._mem is None:
            f = os.open("/dev/mem", os.O_RDWR | os.O_SYNC)
            self._mem = (f, mmap.mmap(f, 0x1000000, offset=phys & ~0xFFFFFF))
            self._base = phys & ~0xFFFFFF
        f, m = self._mem
        if (phys & ~0xFFFFFF) != self._base:
            raise RuntimeError("cross-region read without /dev/mmio")
        m.seek(phys - self._base)
        return struct.unpack("<I", m.read(4))[0]

    def w32(self, phys, value):
        if self.fd is not None:
            self._via_module(phys, value)
            return
        import mmap
        if self._mem is None:
            f = os.open("/dev/mem", os.O_RDWR | os.O_SYNC)
            self._mem = (f, mmap.mmap(f, 0x1000000, offset=phys & ~0xFFFFFF))
            self._base = phys & ~0xFFFFFF
        f, m = self._mem
        if (phys & ~0xFFFFFF) != self._base:
            raise RuntimeError("cross-region write without /dev/mmio")
        m.seek(phys - self._base)
        m.write(struct.pack("<I", value & 0xFFFFFFFF))

    def close(self):
        if self._mem:
            self._mem[1].close()
            os.close(self._mem[0])
        if self.fd is not None:
            os.close(self.fd)

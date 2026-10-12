#!/usr/bin/env python3
"""
Denali (Surface Pro 11, X1P-64100) USB4 UC bring-up.

Rebuilt 2026-10-11 from Justin's documented bring-up sequence after the
Surface was wiped. Must run as root (needs /dev/mem).

Sequence:
  1. Mux the USB4 sideband pins (the Oct 11 root-cause find: they were
     left UNCLAIMED, so the UC sat at "Waiting for SB RX Connect" forever).
  2. Burst the firmware payload into the UC window.
  3. Clear the gates, hit GO, poll for the ready pulse.
  4. Report; ring/DROM work happens after this script succeeds.

DANGER - do not touch these; they hard-lock the SoC:
  * 0x15681000 (usbap_config)
  * 0x15623000 (UC mailbox)
  * the router+0x20a8 interrupt-arming sequence

Usage: sudo python3 uc_bringup.py <firmware.bin>
"""

import mmap
import os
import struct
import sys
import time

# --- UC windows (Justin, Oct 2026) -------------------------------------------
UC_BASE = 0x15600000
UC_SIZE = 0x00024000          # 0x15600000-0x15624000
NHI_BASE = 0x1563F000        # ACPI0015 / NHI router window

FW_BURST_OFF = 0x13000       # firmware {target,count,words[]} burst target
GATE_A_OFF = 0xD064           # clear gate A
GATE_B_OFF = 0x8018           # clear gate B
GO_OFF = 0x22000              # GO register
READY_OFF = 0x18             # ready pulse: ROUTER+0x18 BIT24
READY_BIT = 24

# --- TLMM sideband pinmux (Oct 11 find) ---------------------------------------
TLMM_BASE = 0x0F100000
GPIO_STRIDE = 0x1000
FUNC_SEL_SHIFT = 2
PULL_SHIFT = 0
DRV_SHIFT = 6
# (gpio, func, pull, drv_ma)
# pull: 0=none, 3=up ; drv: 4=10mA
SIDEBAND_PINS = [
    (163, 140, 0, 0),   # usb0_sbrx
    (164, 141, 3, 4),   # usb0_sbtx
    (165, 141, 0, 4),   # usb0_sbtx
    (172, 144, 0, 0),   # usb1_sbrx
    (173, 145, 3, 4),   # usb1_sbtx
    (174, 145, 0, 4),   # usb1_sbtx
]


class Mem:
    def __init__(self, base, size):
        self.base = base
        self.size = size
        self.fd = os.open("/dev/mem", os.O_RDWR | os.O_SYNC)
        self.map = mmap.mmap(self.fd, size, offset=base)

    def r32(self, off):
        self.map.seek(off)
        return struct.unpack("<I", self.map.read(4))[0]

    def w32(self, off, val):
        self.map.seek(off)
        self.map.write(struct.pack("<I", val & 0xFFFFFFFF))

    def close(self):
        self.map.close()
        os.close(self.fd)


def apply_sideband_pinmux():
    """Mux the USB4 sideband pins. Read-modify-write; only touches
    FUNC_SEL/PULL/DRV fields, preserves everything else."""
    tlmm = Mem(TLMM_BASE, 0x400000)
    try:
        for gpio, func, pull, drv in SIDEBAND_PINS:
            off = gpio * GPIO_STRIDE
            cur = tlmm.r32(off)
            new = cur & ~0x3FF          # clear PULL[1:0], FUNC_SEL[5:2], DRV[8:6]
            new |= (func << FUNC_SEL_SHIFT) | (pull << PULL_SHIFT) | (drv << DRV_SHIFT)
            tlmm.w32(off, new)
            verify = tlmm.r32(off)
            tag = f"gpio{gpio}"
            print(f"  {tag}: 0x{cur:08x} -> 0x{verify:08x} (func {func})")
            assert (verify & 0x3FF) == (new & 0x3FF), f"{tag} did not stick"
    finally:
        tlmm.close()
    print("sideband pinmux applied")


def load_firmware(uc, path):
    """Burst {target, count, words[]} records into the UC window.

    File format (per Justin's extraction): repeated records of
      u32 target_offset, u32 word_count, u32 words[word_count]
    """
    data = open(path, "rb").read()
    pos = 0
    nrec = 0
    while pos + 8 <= len(data):
        target, count = struct.unpack_from("<II", data, pos)
        pos += 8
        words = struct.unpack_from(f"<{count}I", data, pos)
        pos += 4 * count
        for i, w in enumerate(words):
            uc.w32(target + 4 * i, w)
        nrec += 1
    print(f"firmware: {nrec} records, {len(data)} bytes burst into +0x{FW_BURST_OFF:x}")
    assert pos == len(data), "trailing bytes in firmware image"


def main():
    if len(sys.argv) != 2:
        print(f"usage: {sys.argv[0]} <firmware.bin>", file=sys.stderr)
        sys.exit(2)
    if os.geteuid() != 0:
        print("must run as root (/dev/mem)", file=sys.stderr)
        sys.exit(1)

    print("== 1/4 sideband pinmux ==")
    apply_sideband_pinmux()

    print("== 2/4 firmware burst ==")
    uc = Mem(UC_BASE, UC_SIZE)
    try:
        load_firmware(uc, sys.argv[1])

        print("== 3/4 gates + GO ==")
        uc.w32(GATE_A_OFF, 0)
        uc.w32(GATE_B_OFF, 0)
        print(f"  cleared gates +0x{GATE_A_OFF:x}, +0x{GATE_B_OFF:x}")
        uc.w32(GO_OFF, 1)
        print(f"  GO written to +0x{GO_OFF:x}")

        print("== 4/4 ready pulse ==")
        deadline = time.time() + 5.0
        while time.time() < deadline:
            if uc.r32(READY_OFF) & (1 << READY_BIT):
                print("  READY pulse seen at ROUTER+0x18 BIT24")
                break
            time.sleep(0.001)
        else:
            print("  TIMEOUT waiting for ready pulse", file=sys.stderr)
            sys.exit(3)
    finally:
        uc.close()

    print("UC up. Next: ring 0 / DROM, then watch for SB RX connect on plug.")


if __name__ == "__main__":
    main()

# UC Doorbell Research — Why Writing `1` to 0x207c Hung the System

**Date:** 2026-10-12
**UC Base:** 0x15600000 (Qualcomm X1P-64100/Denali)
**Status:** Root cause identified

## TL;DR

Writing `1` directly to the doorbell (UC+0x207c) hung the system because:

1. **The doorbell is an incrementing counter, not a trigger bit.** The Windows driver does read-modify-write: read current value, add 1, write back. Writing `1` when the hardware expected (e.g.) `6` desynchronizes the producer-consumer protocol.

2. **We wrote to 0x2078, which is READ-ONLY.** The 0x2078 register is the ring offset set by firmware. It's not a "ring base address" we configure — it's a status register we read.

3. **The ring lives in UC MMIO space, not system RAM.** The command ring is at `UC_base + ring_offset` (where `ring_offset` is read from 0x2078). We allocated a DMA buffer in system RAM and wrote its address to 0x2078, which was wrong on both counts.

## Correct Doorbell Sequence (from QcUsb4Filter8380.sys)

**Source:** `~/workspace/sp11-windows-capture/drivers/qcusb4filter8380/uc-init-sequence.md`
**Function:** 0x14003e760

```asm
0x14003e780: ldr x19, [x0]           ; x19 = UC base
0x14003e78c: ldr w9, [x19, #0x207c]  ; w9 = doorbell count (READ FIRST)
0x14003e794: ldr w10, [x19, #0x2078] ; w10 = ring offset (READ-ONLY)
0x14003e798: ubfiz x8, w9, #4, #0x20 ; x8 = count * 16 (entry size)
0x14003e79c: add x8, x8, w10, uxtw   ; x8 = ring_offset + count*16
; ... (bounds validation) ...
0x14003e824: add x1, x19, w8, uxtw   ; x1 = UC_base + ring_offset + (count*16)
0x14003e830: bl #0x140042de0         ; memcpy 16-byte command to ring entry
0x14003e834: ldr w8, [x19, #0x2078]  ; re-read ring base
0x14003e83c: ldr w10, [x19, #0x207c] ; re-read doorbell
; ... (update descriptors in loop) ...
0x14003e86c: add w9, w10, #1         ; w9 = doorbell + 1 (INCREMENT)
0x14003e874: str w9, [x19, #0x207c]  ; write back incremented value
```

**A second confirmation** from `powerup-analysis.md` (function ~0x14003e7b4):
```asm
14003e83c: ldr w10, [x19, #0x207c]    ; w10 = *(UC_base + 0x207c)
14003e86c: add w9, w10, #1            ; w9 = w10 + 1
14003e874: str w9, [x19, #0x207c]     ; *(UC_base + 0x207c) = w10 + 1
```

## Register Semantics

| Offset | Name | Access | Description |
|--------|------|--------|-------------|
| 0x2078 | Ring offset | **Read-only** | Offset from UC base to command ring (set by firmware). Do NOT write. |
| 0x207c | Doorbell | Read/Write | Producer index counter. Read, increment by 1, write back. |

## Ring Model

This is a **standard producer-consumer ring** with these properties:

- **Ring location:** `UC_base + ring_offset` (inside UC MMIO space, NOT system RAM)
- **Entry size:** 16 bytes
- **Producer index:** The doorbell register (0x207c) is the producer index
- **Entry address:** `UC_base + ring_offset + (doorbell_count * 16)`
- **Submission:** Write 16-byte command to entry, then increment doorbell

This is fundamentally different from the Intel Thunderbolt NHI model (which uses
separate TX/RX rings in system RAM with DMA descriptors). The Qualcomm UC has the
ring in its own MMIO space.

## Why the Test Hung

Our test module did three things wrong:

1. **Wrote to 0x2078** (read-only): We wrote the DMA address of our system RAM
   buffer to 0x2078. This register is read-only (firmware-configured). Writing
   to it may have corrupted UC state or triggered undefined behavior.

2. **Wrote `1` to doorbell instead of incrementing**: The doorbell was at `0`
   (we read it earlier). Writing `1` happened to be correct for the first
   command (0 → 1). But the real problem was...

3. **Ring in wrong location**: We put the command in system RAM, but the UC
   expects commands at `UC_base + ring_offset + (count*16)` in its MMIO space.
   The UC likely tried to read from a bogus MMIO address, causing the hang.

The hang was likely caused by #1 (writing to read-only 0x2078) or #3 (UC reading
from invalid MMIO).

## Correct Test Procedure

```c
void __iomem *uc = ioremap(0x15600000, 0x3000);

// 1. Read ring offset (DO NOT WRITE)
u32 ring_offset = readl(uc + 0x2078);

// 2. Read current doorbell count
u32 doorbell = readl(uc + 0x207c);

// 3. Calculate entry address (in UC MMIO space!)
void __iomem *entry = uc + ring_offset + (doorbell * 16);

// 4. Write 16-byte command via MMIO
//    Example: TB_CFG_PKG_READ (safe, read-only)
//    Bytes: 01 00 00 00  00 00 00 00  00 00 00 00  00 20 00 04
writel(0x00000001, entry + 0);
writel(0x00000000, entry + 4);
writel(0x00000000, entry + 8);
writel(0x04002000, entry + 12);

// 5. Ensure writes complete before doorbell
wmb();

// 6. Increment doorbell (read-modify-write)
doorbell = readl(uc + 0x207c);
writel(doorbell + 1, uc + 0x207c);

// 7. Poll for completion (mechanism TBD - may need interrupt or status poll)
```

## Open Questions

1. **Completion detection:** How do we know the UC processed the command?
   - Does the doorbell register change?
   - Is there a separate completion status register?
   - Does it generate an interrupt?
   - The Windows driver reads descriptors in a loop after submission (0x14003e848:
     `ldr w10, [x8, #8]`), suggesting polling of a descriptor.

2. **Ring size:** How many entries does the ring have? What happens on wrap-around?
   The driver has bounds validation (not fully analyzed).

3. **RX path:** Is there a separate completion queue, or are responses written
   back to the same ring entry? The 16-byte entry format from the USB4 spec
   suggests the response overwrites the request in-place (with bit 31 of word 1
   set to indicate valid response).

4. **Firmware state:** Does the UC need firmware loaded before accepting commands?
   We have `qcom/x1-diag-mcu.bin` (40KB) but the loading mechanism is unknown.
   The UC responds to MMIO reads, suggesting it's at least partially initialized.

5. **Multiple instances:** There are 3 USB4 instances. Is there one UC per instance,
   or a single UC? The 0x15600000 base was for instance 0. Instances 1 and 2 may
   have different UC bases.

## Comparison: Qualcomm UC vs Intel NHI

| Aspect | Intel NHI (Linux thunderbolt) | Qualcomm UC (Denali) |
|--------|-------------------------------|----------------------|
| Ring location | System RAM (DMA) | UC MMIO space |
| Ring setup | Driver allocates, programs base addr | Firmware-configured, driver reads offset |
| TX/RX | Separate TX and RX rings | Single ring (producer-consumer) |
| Doorbell | Write count of new descriptors | Increment counter by 1 |
| Completion | Interrupt + RX ring | Unknown (polling? interrupt?) |
| Entry size | 32 bytes (NHI descriptor) | 16 bytes (USB4 packet) |

The Qualcomm design is simpler (no DMA, no separate RX ring) but completely
different from Intel's. The Linux `thunderbolt` driver cannot be used directly —
a Qualcomm-specific driver is needed.

## Recommendations

1. **Never write to 0x2078.** It's read-only. The ring offset is firmware-configured.

2. **Always use read-increment-write for the doorbell.** Never write an absolute value.

3. **Write commands via MMIO to UC space**, not to system RAM DMA buffers.

4. **Start with a read-only command** (TB_CFG_PKG_READ) to avoid changing hardware state.

5. **Implement a timeout** on completion polling to avoid hangs. If no response in
   100ms, abort and report.

6. **Check for interrupts:** The UC may signal completion via MSI or a GPIO. Check
   `/proc/interrupts` for new entries after doorbell ring.

7. **Consider firmware:** If commands get no response, the UC may need firmware
   loaded first. Investigate the `x1-diag-mcu.bin` loading mechanism.

## References

- `~/workspace/sp11-windows-capture/drivers/qcusb4filter8380/uc-init-sequence.md` — Doorbell sequence disassembly
- `~/workspace/sp11-windows-capture/drivers/qcusb4filter8380/powerup-analysis.md` — MMIO write analysis
- `~/workspace/sp11-windows-capture/drivers/qcusb4filter8380/uc-subinit-analysis.md` — Correction (no UC MMIO in init)
- `~/workspace/usb4-denali/docs/USB4-COMMAND-RING-2026-10-12.md` — 16-byte command format
- Linux `drivers/thunderbolt/nhi.c` — Intel NHI reference (different model)
- USB4 Specification — Command packet format (TB_CFG_PKG_READ=1, etc.)

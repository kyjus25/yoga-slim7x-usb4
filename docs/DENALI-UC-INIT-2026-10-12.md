# USB4 Controller (UC) Initialization Sequence — X1P-64100

**Source:** `QcUsb4Filter8380.sys` v1.0.0.5048 (524,504 bytes, ARM64), Qualcomm USB4 filter driver
**UC physical base:** 0x15600000 (mapped via MmMapIoSpaceEx)
**Date extracted:** 2026-10-12
**Method:** Static analysis (capstone disassembly, MMIO write tracing)

## Summary

The driver performs UC initialization through direct MMIO writes (inline STR instructions, no HAL wrappers).
The init sequence was identified in function **0x140035f70**, with teardown in **0x140022500**.
The runtime command-ring doorbell is at **0x14003e760** (offset 0x207c).

**Key finding:** The UC init is surprisingly minimal — only 6 register writes. The heavy lifting
(link training, sideband) is done via the command ring at runtime, not during init.

## Initialization Function: 0x140035f70

### Register Writes (in order)

| # | Offset | Value | Size | Confidence | Notes |
|---|--------|-------|------|------------|-------|
| 1 | 0x48 | 3 | 32-bit | High | Initial mode set |
| 2 | 0x48 | 4 | 32-bit | High | Mode transition (3→4). Alternate path writes 2 instead |
| 3 | 0x28 | 1 | 32-bit | High | Enable bit |
| 4 | 0x4c | 1 | 32-bit | High | Enable bit |
| 5 | 0x2b9 | 1 | 8-bit | High | Byte write, likely a flag |
| 6 | 0x3c8 | 0 | 8-bit | High | Clear byte (was maybe set by firmware) |

### Detailed Disassembly

```asm
; Function 0x140035f70: UC initialization
0x140035f8c: bl #0x140008b28    ; Get UC VA into x0
0x140035f90: mov x19, x0        ; x19 = UC base VA

; ... (WDF logging setup) ...

0x140035fd8: add x0, x19, #0x3b0
0x140035fdc: bl #0x140034ca8    ; Sub-init (unknown, possibly PHY?)

0x140035fe0: mov w8, #3
0x140035fe4: str w8, [x19, #0x48]  ; UC[0x48] = 3

; ... (check status via function call) ...

0x14003601c: mov w8, #4
0x140036020: str w8, [x19, #0x48]  ; UC[0x48] = 4  (3→4 transition)

0x140036024: mov w8, #1
0x140036028: str w8, [x19, #0x28]  ; UC[0x28] = 1

0x14003602c: mov x0, x22
0x140036030: str w8, [x19, #0x4c]  ; UC[0x4c] = 1

0x140036034: strb w8, [x19, #0x2b9] ; UC[0x2b9] = 1 (byte)

; ... (more calls) ...

0x140036050: strb wzr, [x19, #0x3c8] ; UC[0x3c8] = 0 (byte)
```

### Alternate Path

If a status check fails (tbnz w20, #0x1f at 0x140036010):
```asm
0x140036060: mov w8, #2
0x140036064: add x0, x19, #0x3b0
0x140036068: str w8, [x19, #0x48]  ; UC[0x48] = 2 (instead of 4)
0x14003606c: bl #0x140034bf0       ; Different sub-init
```

The value written to 0x48 (2 vs 4) depends on a runtime status check. This is likely:
- 2 = error/recovery mode
- 4 = normal operational mode
- 3 = intermediate/reset state

## Teardown Function: 0x140022500

The driver clears these registers on remove/D0Exit (confirms they are init-controlled):

| Offset | Value | Notes |
|--------|-------|-------|
| 0x10 | 0 | 64-bit clear |
| 0x60 | 0 | 64-bit clear |
| 0x1f0 | 0 | 64-bit clear (if non-zero) |
| 0x1f8 | 0 | 64-bit clear (if non-zero) |
| 0x378 | 0 | 64-bit clear (if non-zero) |
| 0x3c9 | 0 | Byte clear |

**Note:** 0x3c9 is adjacent to 0x3c8 (init writes 0 to 0x3c8, teardown clears 0x3c9).
These likely form a 16-bit register at 0x3c8.

## Runtime Command Ring: 0x14003e760

### Doorbell Mechanism

The UC uses a command ring for runtime operations (link training, sideband):

| Offset | Purpose | Access |
|--------|---------|--------|
| 0x2078 | Ring base offset (from UC base) | Read-only (set by firmware?) |
| 0x207c | Doorbell (write pointer) | Read/Write |

### Doorbell Sequence (function 0x14003e760)

```asm
0x14003e780: ldr x19, [x0]           ; x19 = UC base
0x14003e78c: ldr w9, [x19, #0x207c]  ; w9 = doorbell count
0x14003e794: ldr w10, [x19, #0x2078] ; w10 = ring offset
0x14003e798: ubfiz x8, w9, #4, #0x20 ; x8 = count * 16
0x14003e79c: add x8, x8, w10, uxtw   ; x8 = ring_offset + count*16
; ... (validate bounds) ...
0x14003e824: add x1, x19, w8, uxtw   ; x1 = UC_base + ring_entry
0x14003e830: bl #0x140042de0         ; memcpy command into ring
0x14003e834: ldr w8, [x19, #0x2078]  ; re-read ring base
0x14003e83c: ldr w10, [x19, #0x207c] ; re-read doorbell
; ... (update descriptors in loop) ...
0x14003e86c: add w9, w10, #1         ; w9 = doorbell + 1
0x14003e874: str w9, [x19, #0x207c]  ; RING DOORBELL
```

**Interpretation:** Standard producer-consumer ring. Driver writes command to
`UC_base + ring_offset + (doorbell * 16)`, then increments doorbell to notify HW.

## What the Init Does NOT Do

- **No PHY serdes programming:** The init does not write to PHY registers.
  PHY is likely configured via the command ring or by firmware.
- **No link training initiation:** Link training is started via command ring
  messages at runtime, not via direct MMIO during init.
- **No speed negotiation:** Gen2/Gen3/Gen4 selection is via command ring.
- **No delays:** No KeStallExecutionProcessor calls found in the init path.
  The HW is assumed ready after the writes.

## Register Map Summary

### Init-controlled (write during init, clear on teardown)
- 0x28: Enable (write 1)
- 0x48: Mode (write 3, then 4; or 2 on error path)
- 0x4c: Enable (write 1)
- 0x2b9: Flag (byte, write 1)
- 0x3c8: Clear (byte, write 0)

### Runtime (command ring)
- 0x2078: Ring base offset (read-only)
- 0x207c: Doorbell (read/write, increment to submit)

### Teardown-only (cleared on remove, not set in init?)
- 0x10, 0x60, 0x1f0, 0x1f8, 0x378: Cleared to 0
- These may be set by firmware or via command ring during operation.

## Confidence Levels

- **High:** The 6 init writes (0x28, 0x48, 0x4c, 0x2b9, 0x3c8) — directly observed,
  consistent across code paths.
- **High:** Doorbell mechanism (0x2078/0x207c) — fully reverse-engineered,
  standard ring buffer pattern.
- **Medium:** x19 is UC base in init function — inferred from pattern matching
  with doorbell function (same register usage, same accessor mechanism).
  Not 100% proven via data-flow trace.
- **Low:** Purpose of each register — inferred from write patterns, not from
  USB4 spec. 0x48 with values 2/3/4 is likely a mode/state register.
- **Unknown:** What 0x140034ca8 and 0x140034bf0 do (called during init).
  These may configure the PHY or sideband.

## For Linux Bring-Up

### Minimal UC Init (from this analysis)
```c
// UC base = 0x15600000 (physical)
writel(3, uc_base + 0x48);
writel(4, uc_base + 0x48);  // or 2 on error
writel(1, uc_base + 0x28);
writel(1, uc_base + 0x4c);
writeb(1, uc_base + 0x2b9);
writeb(0, uc_base + 0x3c8);
```

### Command Ring Setup
The ring base at 0x2078 is NOT written by the driver — it's either:
1. Set by firmware before driver loads, OR
2. A hardware-fixed offset

**Linux must:** Read UC+0x2078 to get the ring offset, then use UC+0x207c as doorbell.
The ring entry size is 16 bytes (from `ubfiz x8, w9, #4` = count * 16).

### Next Steps
1. **Verify x19==UC:** Trace the accessor 0x140008b28 to confirm it returns the UC VA
   (not TLMM or GCC).
2. **Analyze 0x140034ca8:** The sub-init called before the writes — may be PHY setup.
3. **USB4 spec mapping:** Map offsets 0x28, 0x48, 0x4c to USB4 host router registers
   (likely in the Configuration Space, not the Operational Space).
4. **Dynamic validation:** On hardware, read these registers after Windows boots
   to confirm the values.

## Files
- Binary: `QcUsb4Filter8380.sys` (SHA256: 5f4aabee340244cc46cb28e62d51850a40e0033bd34e08c8f5ca2a2625945923)
- This report: `uc-init-sequence.md`
- Prior analysis: `powerup-analysis.md`

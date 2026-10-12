# CORRECTION: Denali UC Initialization (2026-10-12)

## The prior report was wrong

`docs/DENALI-UC-INIT-2026-10-12.md` (commit a30c87d) documented "6 MMIO writes"
to initialize the USB4 controller. **This was incorrect.**

Further analysis proved those writes target a **software WDF/DMF device context**,
not UC hardware registers. The x19 register in the init function holds a
driver-allocated context pointer (from WdfObjectGetTypedContext), not the UC
MMIO base. Evidence:
- The "sub-init" functions call KeResetEvent/KeSetEvent on x19+0x3b0 —
  these require a KEVENT structure, impossible on MMIO.
- The doorbell function dereferences its input (`ldr x19, [x0]`) to get the
  real UC VA from context offset 0x0.

## What the Windows driver actually does

**No direct UC MMIO initialization.** The UC hardware is either:
1. Initialized by firmware before the driver loads, OR
2. Controlled exclusively via the command ring at runtime.

## What remains valid

The **command ring mechanism** (doorbell at UC+0x207c, ring base at UC+0x2078)
is real hardware MMIO. The driver writes commands to the ring and increments
the doorbell. This is how Linux should interact with the UC.

## For Linux bring-up

Do NOT replicate the "6 writes" — they were Windows driver bookkeeping.
Focus on:
1. GCC clocks + GDSCs (we have the registers)
2. Command ring via 0x2078/0x207c
3. Let the thunderbolt stack handle link training

The UC is likely firmware-initialized; Linux just needs to power it and
talk via the ring.

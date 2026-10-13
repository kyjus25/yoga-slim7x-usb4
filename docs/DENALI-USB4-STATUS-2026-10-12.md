# Denali USB4 Bring-Up Status — 2026-10-12

**Device:** Microsoft Surface Pro 11 (X1P-64100 "Denali")
**Kernel:** 7.2.8-arch1-3-sp11 (dwhinham/archlinux-repo)
**Goal:** Apple Studio Display via Thunderbolt/USB4

## Achievements

### ✅ UC Powers On Under Linux (First Time)

The USB4 controller (UC) at `0x15600000` has been successfully powered on
under Linux for the first time on Denali hardware.

**Test module:** `tests/usb4-power-test.c`
- Powers on GDSC (`gcc_usb4_0_gdsc` at 0x19f004)
- Enables 14 clocks via direct CBCR MMIO
- Releases 6 BCR resets
- Verifies UC responds to MMIO reads

**Verification:**
```
UC@0x15600000: ring_base(0x2078)=0x00000000, doorbell(0x207c)=0x00000000
UC appears ALIVE (valid register values)
```

### ✅ X1E GCC Driver Verified Sufficient

The existing `qcom,x1e80100-gcc` driver already has correct USB4 register
offsets for Denali. Verified 7 CBCRs and 3 GDSCs match Windows-extracted
values exactly. No custom GCC driver needed.

### ✅ Command Ring Protocol Understood

From Windows driver analysis (QcUsb4Filter8380.sys):
- Doorbell (UC+0x207c) is an incrementing counter, not a trigger bit
- Ring offset (UC+0x2078) is read-only, firmware-configured
- Ring lives in UC MMIO space, not system RAM
- 16-byte entries, producer-consumer model

**Test result:** Protocol implemented correctly (no hangs), but ring offset
reads as 0 — firmware hasn't configured a ring. This is expected; no Windows
driver configures it either.

### ✅ Windows Driver Analysis Complete

- **QcUsb4Filter8380.sys**: Does NOT do UC init. Only doorbell writes.
  The "6 MMIO writes" were proven to be software context, not hardware.
- **QcUsb4Bus8380.sys**: Does NOT do UC init. Pure WDF bus enumerator,
  no MMIO, no references to 0x15600000.
- **qcpep8380.sys**: Clocks and power domains only. 62 registers extracted
  (42 CBCRs, 17 BCRs, 3 GDSCs).

**Conclusion:** UC initialization is done by UEFI firmware, not Windows drivers.
The UC is autonomous once powered.

## Current Blocker

### ❌ No DisplayPort Output

With all three USB4 instances powered on, the Apple Studio Display is not
detected. Both DP-1 and DP-2 show "disconnected" in DRM.

**Root cause:** The UC is powered and alive, but not performing USB4 link
training or DisplayPort tunneling. This requires:
1. PHY initialization (beyond just clocks)
2. USB4 link training protocol
3. DisplayPort tunnel setup
4. Integration with Linux Thunderbolt/USB4 stack

The UC firmware (loaded by UEFI) should handle this autonomously, but it's
not happening. Possible reasons:
- PHY needs explicit initialization
- UC needs to be told which port is connected
- Missing integration with Linux Type-C/AltMode framework

## What's Next

### Phase 1: PHY Bring-Up (Weeks)
- Research Qualcomm USB4 PHY initialization sequence
- The PHY clocks are enabled, but the PHY itself may need configuration
- Look at `drivers/phy/qualcomm/` for QMP USB4 PHY driver

### Phase 2: Link Training (Weeks)
- USB4 link training is a complex state machine
- May require implementing parts of the USB4 spec in a new driver
- Or, porting the Windows driver's link training logic

### Phase 3: Linux Integration (Weeks)
- The Linux `thunderbolt` driver expects PCI devices
- Qualcomm UC is a platform device, not PCI
- Need a new `qcom-usb4` platform driver that:
  - Powers on the UC (we have this working)
  - Handles link training
  - Exposes DisplayPort tunnels to DRM
  - Integrates with Type-C framework

### Phase 4: Display Output (Days, after 1-3)
- Once the stack works, the Studio Display should appear as a DRM connector
- Test with actual hardware

## Files Committed

- `tests/usb4-power-test.c` — Power sequencing test module
- `docs/DENALI-USB4-REGISTERS-2026-10-12.md` — 62 register mappings
- `docs/DENALI-UC-BRINGUP-2026-10-12.md` — Bring-up procedure
- `docs/USB4-COMMAND-RING-2026-10-12.md` — Ring protocol
- `docs/UC-DOORBELL-RESEARCH-2026-10-12.md` — Doorbell semantics

## Honest Assessment

We've done the hard low-level work: the UC powers on, clocks work, MMIO
works. That's the foundation.

But a working display requires a full USB4 host controller driver, which is
a substantial kernel development project (estimated weeks to months). The
Windows drivers don't help — they rely on firmware for the hard parts, and
the firmware isn't doing it under Linux.

This is a marathon, not a sprint. The foundation is solid; the upper layers
need to be built.

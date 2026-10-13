# Denali USB4 Driver Port — Status

**Date:** 2026-10-12
**Source:** Yoga Slim 7x X1E80100 USB4 driver (`experiments/el1-pci0`)

## What Was Done

Adapted the Yoga Slim 7x USB4 host driver for Denali (X1P64100):

1. **Driver source** (`drivers/qcom-usb4-denali-host.c`):
   - Changed compatible from `qcom,x1e80100-usb4` to `qcom,x1p64100-usb4`
   - Changed machine check from `lenovo,yoga-slim7x` to `microsoft,surface-pro-11`
   - Changed firmware path to `qcom/x1p64100/usb4-router-legacy.bin`
   - UC base `0x15600000` is identical — no changes needed

2. **Headers collected**:
   - `drivers/include/linux/phy/phy-qcom-x1-usb4.h` (stub — uses standard PHY APIs)
   - `drivers/include/linux/soc/qcom/x1-usb4.h` (from omarchy1)
   - `drivers/qcom-usb4-*.h` (6 headers from snapshot/module/)

## What's Missing

### `qcom-usb4-nhi.h` (BLOCKER)

The NHI (Native Host Interface) abstraction header was not exported in the
snapshot. The driver uses:
- `struct qcom_usb4_nhi`
- `qcom_usb4_nhi_init_interrupts()`
- `qcom_usb4_nhi_request_ring_irq()`
- `qcom_usb4_nhi_prepare()`
- ... and other NHI functions

**Where to find it:** The implementation is in
`snapshot/kernel/drivers/thunderbolt/qcom-x1-native.c`, but the header
defining `struct qcom_usb4_nhi` was not included in the export.

**Options:**
1. Extract the struct definition from `qcom-x1-native.c` and create the header
2. Contact the Yoga maintainer for the missing header
3. Rewrite the NHI abstraction using mainline Thunderbolt APIs

### Firmware (NEEDED)

`qcom/x1p64100/usb4-router-legacy.bin` — the Denali USB4 router firmware.
The Yoga uses `qcom/x1e80100/usb4-router-legacy.bin`. We have
`x1-diag-mcu.bin` (40816 bytes) extracted from Windows, but it's unclear
if this is the same as the router firmware.

### Device Tree (NEEDED)

Need a Device Tree node for the Denali USB4 controller:
```dts
usb4@15600000 {
    compatible = "qcom,x1p64100-usb4";
    reg = <0x0 0x15600000 0x0 0x100000>;
    /* clocks, resets, phys, etc. */
};
```

## Next Steps

1. Extract/create `qcom-usb4-nhi.h` from `qcom-x1-native.c`
2. Obtain or verify the Denali USB4 firmware
3. Create Device Tree bindings
4. Attempt to compile the driver
5. Test on hardware

## Files

- `drivers/qcom-usb4-denali-host.c` — Adapted driver (2398 lines)
- `drivers/include/` — Kernel headers
- `drivers/qcom-usb4-*.h` — Qualcomm helper headers

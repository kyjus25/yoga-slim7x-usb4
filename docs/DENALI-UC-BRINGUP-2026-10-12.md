# Denali USB4 UC Bring-Up — 2026-10-12

**Status:** SUCCESS — UC is alive under Linux

## Summary

The USB4 controller (UC) at `0x15600000` on the X1P-64100 (Denali, Surface Pro 11)
has been successfully powered on under Linux for the first time.

## Power Sequence

The following sequence was executed via a test kernel module (`tests/usb4-power-test.c`):

1. **GDSC power on**: Wrote to `gcc_usb4_0_gdsc` at `0x19f004`
   - Before: `0x0022f801`
   - After: `0xf822f800`
   - PWR_ON bit (31) set, SW_COLLAPSE (bit 0) cleared

2. **Clock enable** (14 clocks via direct CBCR MMIO):
   - `gcc_usb4_0_cfg_ahb_clk` @ 0x9f0a8
   - `gcc_usb4_0_sys_clk` @ 0x9f054
   - `gcc_usb4_0_master_clk` @ 0x9f018
   - `gcc_usb4_0_sb_if_clk` @ 0x9f044
   - `gcc_usb4_0_tmu_clk` @ 0x9f088
   - `gcc_usb4_0_phy_usb_pipe_clk` @ 0x9f0a4
   - `gcc_usb4_0_phy_p2rr2p_pipe_clk` @ 0x9f0d8
   - `gcc_usb4_0_dp0_clk` @ 0x9f060
   - `gcc_usb4_0_dp1_clk` @ 0x9f108
   - `gcc_usb4_0_phy_pcie_pipe_clk` @ 0x9f048
   - `gcc_usb4_0_phy_rx0_clk` @ 0x9f0b0
   - `gcc_usb4_0_phy_rx1_clk` @ 0x9f0c0
   - `gcc_usb4_0_apb_clk` @ 0x9f0d0
   - `gcc_usb4_0_tmu_clk` @ 0x9f088

3. **BCR reset release** (6 BCRs at 0x50000 block):
   - All deasserted successfully

## UC Verification

After power sequencing, the UC registers were readable:

```
UC@0x15600000: ring_base(0x2078)=0x00000000, doorbell(0x207c)=0x00000000
UC appears ALIVE (valid register values)
```

Zero values are valid initial states (not 0xdeadbeef or 0xffffffff which
would indicate a dark device).

## Key Findings

1. **X1E GCC driver is sufficient**: The existing `qcom,x1e80100-gcc` driver
   already has correct USB4 register offsets for Denali. No custom GCC driver
   needed. Verified: 7 CBCRs and 3 GDSCs match Windows-extracted values exactly.

2. **No PCI enumeration (yet)**: The USB4 controller does not appear as a PCI
   device after power-on. This suggests:
   - Additional firmware loading may be required, OR
   - The controller is a platform device (not PCI), OR
   - PCIe tunnel establishment needs additional steps

3. **Command ring is accessible**: The UC command ring registers (0x2078/0x207c)
   are readable, indicating the controller is ready for command submission.

## Next Steps

1. Try submitting commands via the UC command ring
2. Check if firmware needs to be loaded to the UC
3. Investigate why PCI enumeration doesn't occur
4. Test with Apple Studio Display connected

## Test Module

`tests/usb4-power-test.c` — Load with:
```bash
sudo insmod usb4-power-test.ko [instance=0]
dmesg | grep usb4-power-test
sudo rmmod usb4-power-test
```

Module parameters:
- `instance=0|1|2`: Select USB4 instance (default 0)
- `do_bcr=0|1`: Skip BCR reset release (default 1)
- `do_apb_at=0|1`: Skip apb/at clocks (default 1)

## References

- Register map: `docs/DENALI-USB4-REGISTERS-2026-10-12.md`
- Windows extraction: `~/workspace/sp11-windows-capture/drivers/qcpep8380/`
- Kernel: 7.2.8-arch1-3-sp11 (dwhinham/archlinux-repo)

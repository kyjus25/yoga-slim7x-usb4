# Denali (X1P64100) GCC Driver Notes
# Work in progress - systematic mapping from X1E80100

## Confirmed: X1E80100 register map largely compatible
- GCC base: 0x100000 (confirmed via Windows driver decompilation)
- All 124 X1E clock enable registers readable on Denali
- 69 enabled by default, 55 disabled
- All 27 X1E GDSCs readable on Denali

## USB4 Subsystem (Denali-specific findings)

### GDSCs - All ON after manual enable
- usb4_0 (0x9f004): ON (was ON)
- usb4_1 (0x2b004): ON (was ON)
- usb4_2 (0x11004): ON (was ON) - **Denali has USB4_2!**
- usb_0_phy (0x50024): ON (was OFF, enabled)
- usb_1_phy (0x2a024): ON (was OFF, enabled)
- usb_2_phy (0xa3024): ON (was OFF, enabled)

### Clocks - USB4_2 was OFF, now enabled
- USB4_0: 5 clocks enabled (ahb, sys, master, sb_if, tmu)
- USB4_1: 5 clocks enabled
- USB4_2: 5 clocks enabled (were OFF, now ON) - **Key Denali difference!**

### UC Status
- Addresses tried: 0x15600000, 0x15700000, 0x15500000
- RAM offset: 0x13000 (from qcom_usb4_mcu_init LEGACY)
- Control offset: 0x22000
- Result: All dark, even with 15 GDSCs and 22 clocks enabled

## PCIe GDSCs
- pcie_0_tunnel through pcie_5: ON (except pcie_4 which was OFF, now ON)
- pcie_6a, pcie_6b, pcie_6_phy: mixed

## Open Questions
1. Why doesn't UC respond despite power/clocks?
2. Is there a Denali-specific reset or power domain not in X1E driver?
3. Is the UC address different on Denali?
4. Does the 0x1fc0000 region (mapped but unused by driver) matter?

## Next Steps
- Systematic scan for Denali-specific registers not in X1E driver
- Check reset controls (BCRs)
- Investigate UC address mapping

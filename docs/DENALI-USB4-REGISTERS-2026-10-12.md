# USB4 GCC Register Offsets — X1P-64100 (from qcpep8380.sys)

**Source:** `qcpep8380.sys` v1.0.4458.2600 (4,841,760 bytes, ARM64), Qualcomm PEP driver
**Date extracted:** 2026-10-12
**Method:** Static analysis — pointer scanning in `.data` section for clock-name string references,
followed by structure decoding. All offsets are absolute physical addresses.

## Summary

| Type | Count | Description |
|------|-------|-------------|
| Clock CBCRs | 42 | Clock Branch Control Registers |
| BCRs | 17 | Block Control Resets |
| GDSCs | 3 | Power domain controls |
| **Total** | **62** | |

## Instance Map

Three USB4 instances, each with identical register layout at different bases (all in GCC1 @ 0x100000):

| Instance | Clock block | GDSC | BCR block |
|----------|-------------|------|-----------|
| 0 (primary) | 0x100000 + 0x9f000 = **0x19f000** | 0x19f004 | 0x100000 + 0x50000 = **0x150000** |
| 1 | 0x100000 + 0x2b000 = **0x12b000** | 0x12b004 | 0x100000 + 0x2a000 = **0x12a000** |
| 2 | 0x100000 + 0x11000 = **0x111000** | 0x111004 | 0x100000 + 0xa3000 = **0x1a3000** |

Register offsets *within* each instance block are identical across instances.

## Key Clocks — Instance 0 (Primary)

These are the essential clocks for USB4 bring-up:

| Clock name | Physical addr | GCC offset | Confidence |
|------------|---------------|------------|------------|
| gcc_usb4_0_cfg_ahb_clk | 0x19f0a8 | 0x9f0a8 | High |
| gcc_usb4_0_sys_clk | 0x19f054 | 0x9f054 | High |
| gcc_usb4_0_master_clk | 0x19f018 | 0x9f018 | High |
| gcc_usb4_0_sb_if_clk | 0x19f044 | 0x9f044 | High |
| gcc_usb4_0_tmu_clk | 0x19f088 | 0x9f088 | High |
| gcc_usb4_0_phy_usb_pipe_clk | 0x19f0a4 | 0x9f0a4 | High |
| gcc_usb4_0_phy_p2rr2p_pipe_clk | 0x19f0d8 | 0x9f0d8 | High |
| gcc_usb4_0_gdsc | 0x19f004 | 0x9f004 | High |

## All Clock CBCRs — Instance 0

| Clock name | Physical addr | Offset in block |
|------------|---------------|-----------------|
| gcc_usb4_0_apb_clk | 0x19f0d0 | +0xd0 |
| gcc_usb4_0_at_clk | 0x19f0cc | +0xcc |
| gcc_usb4_0_cfg_ahb_clk | 0x19f0a8 | +0xa8 |
| gcc_usb4_0_dp0_clk | 0x19f060 | +0x60 |
| gcc_usb4_0_dp1_clk | 0x19f108 | +0x108 |
| gcc_usb4_0_master_clk | 0x19f018 | +0x18 |
| gcc_usb4_0_phy_p2rr2p_pipe_clk | 0x19f0d8 | +0xd8 |
| gcc_usb4_0_phy_pcie_pipe_clk | 0x19f048 | +0x48 |
| gcc_usb4_0_phy_rx0_clk | 0x19f0b0 | +0xb0 |
| gcc_usb4_0_phy_rx1_clk | 0x19f0c0 | +0xc0 |
| gcc_usb4_0_phy_usb_pipe_clk | 0x19f0a4 | +0xa4 |
| gcc_usb4_0_sb_if_clk | 0x19f044 | +0x44 |
| gcc_usb4_0_sys_clk | 0x19f054 | +0x54 |
| gcc_usb4_0_tmu_clk | 0x19f088 | +0x88 |

## All Clock CBCRs — Instance 1

| Clock name | Physical addr | Offset in block |
|------------|---------------|-----------------|
| gcc_usb4_1_apb_clk | 0x12b0d0 | +0xd0 |
| gcc_usb4_1_at_clk | 0x12b0cc | +0xcc |
| gcc_usb4_1_cfg_ahb_clk | 0x12b0a8 | +0xa8 |
| gcc_usb4_1_dp0_clk | 0x12b060 | +0x60 |
| gcc_usb4_1_dp1_clk | 0x12b108 | +0x108 |
| gcc_usb4_1_master_clk | 0x12b018 | +0x18 |
| gcc_usb4_1_phy_p2rr2p_pipe_clk | 0x12b0d8 | +0xd8 |
| gcc_usb4_1_phy_pcie_pipe_clk | 0x12b048 | +0x48 |
| gcc_usb4_1_phy_rx0_clk | 0x12b0b0 | +0xb0 |
| gcc_usb4_1_phy_rx1_clk | 0x12b0c0 | +0xc0 |
| gcc_usb4_1_phy_usb_pipe_clk | 0x12b0a4 | +0xa4 |
| gcc_usb4_1_sb_if_clk | 0x12b044 | +0x44 |
| gcc_usb4_1_sys_clk | 0x12b054 | +0x54 |
| gcc_usb4_1_tmu_clk | 0x12b088 | +0x88 |

## All Clock CBCRs — Instance 2

| Clock name | Physical addr | Offset in block |
|------------|---------------|-----------------|
| gcc_usb4_2_apb_clk | 0x1110d0 | +0xd0 |
| gcc_usb4_2_at_clk | 0x1110cc | +0xcc |
| gcc_usb4_2_cfg_ahb_clk | 0x1110a8 | +0xa8 |
| gcc_usb4_2_dp0_clk | 0x111060 | +0x60 |
| gcc_usb4_2_dp1_clk | 0x111108 | +0x108 |
| gcc_usb4_2_master_clk | 0x111018 | +0x18 |
| gcc_usb4_2_phy_p2rr2p_pipe_clk | 0x1110d8 | +0xd8 |
| gcc_usb4_2_phy_pcie_pipe_clk | 0x111048 | +0x48 |
| gcc_usb4_2_phy_rx0_clk | 0x1110b0 | +0xb0 |
| gcc_usb4_2_phy_rx1_clk | 0x1110c0 | +0xc0 |
| gcc_usb4_2_phy_usb_pipe_clk | 0x1110a4 | +0xa4 |
| gcc_usb4_2_sb_if_clk | 0x111044 | +0x44 |
| gcc_usb4_2_sys_clk | 0x111054 | +0x54 |
| gcc_usb4_2_tmu_clk | 0x111088 | +0x88 |

## GDSC Power Domains

| GDSC name | Physical addr | GCC offset | Notes |
|-----------|---------------|------------|-------|
| gcc_usb4_0_gdsc | 0x19f004 | 0x9f004 | Instance 0 power domain |
| gcc_usb4_1_gdsc | 0x12b004 | 0x2b004 | Instance 1 power domain |
| gcc_usb4_2_gdsc | 0x111004 | 0x11004 | Instance 2 power domain |

Note: `gcc_usb4_gdsc` (shared, no instance number) exists as a string but has no
register mapping in the PEP data — it may be an aggregate or unused.

## Block Control Resets (BCRs)

| BCR name | Physical addr | GCC offset |
|----------|---------------|------------|
| gcc_usb4_0_dp0_phy_prim_bcr | 0x150010 | 0x50010 |
| gcc_usb4_0_dp1_phy_prim_bcr | 0x150014 | 0x50014 |
| gcc_usb4_0_dpphy_aux_bcr | 0x150018 | 0x50018 |
| gcc_usb4_0_phy_phy_prim_bcr | 0x15000c | 0x5000c |
| gcc_usb4_0_phy_prim_bcr | 0x150008 | 0x50008 |
| gcc_usb4_1_dp0_phy_sec_bcr | 0x12a010 | 0x2a010 |
| gcc_usb4_1_dp1_phy_sec_bcr | 0x12a014 | 0x2a014 |
| gcc_usb4_1_phy_phy_sec_bcr | 0x12a00c | 0x2a00c |
| gcc_usb4_1_phy_sec_bcr | 0x12a008 | 0x2a008 |
| gcc_usb4_2_dp0_phy_tert_bcr | 0x1a3010 | 0xa3010 |
| gcc_usb4_2_dp1_phy_tert_bcr | 0x1a3014 | 0xa3014 |
| gcc_usb4_2_dpphy_aux_bcr | 0x1a3018 | 0xa3018 |
| gcc_usb4_2_phy_phy_tert_bcr | 0x1a300c | 0xa300c |
| gcc_usb4_2_phy_tert_bcr | 0x1a3008 | 0xa3008 |
| gcc_usb4phy_0_phy_prim_bcr | 0x15001c | 0x5001c |
| gcc_usb4phy_1_phy_sec_bcr | 0x12a01c | 0x2a01c |
| gcc_usb4phy_2_phy_tert_bcr | 0x1a301c | 0xa301c |

## Method

1. **String location:** Found all 111 USB4 clock-name strings in `.rdata` via byte search.
2. **Pointer scan:** Scanned `.data` section (8-byte aligned) for pointers to each string VA.
3. **Structure decode:** Each clock entry is a ~48-byte struct:
   ```
   +0x00: [ptr] type/descriptor pointer
   +0x08: [u32] hash/ID
   +0x10: [ptr] clock name string  <-- anchor
   +0x18: [ptr] ops or parent pointer
   +0x20: [u64] 0 (reserved)
   +0x28: [u64] CBCR physical address  <-- the register offset
   +0x30: [u64] block base address
   ```
   GDSC entries have a shorter struct with the register at +0x10 from the name pointer.
   BCR entries have the register at +0x20 from the name pointer.
4. **Validation:** The three instances show identical intra-block offsets, confirming
   correct decoding. The GDSC offsets (0x9f004, 0x2b004) match previously known
   values from independent analysis.

## Confidence

- **High:** All 42 CBCRs, 3 GDSCs, 17 BCRs — validated by cross-instance consistency
  and match with known GDSC offsets.
- **Medium:** None — the pattern is very consistent.
- **Low / Unknown:**
  - `*_clk_src` clocks (e.g., `gcc_usb4_0_master_clk_src`): These are clock *sources/muxes*,
    not branches. They do not appear in the CBCR table; their configuration is via
    a different mechanism (likely RCG — Root Clock Generator — registers).
  - `gcc_usb4_gdsc` (shared): String exists but no register mapping found.
  - 8 of 25 BCR names had no register mapping (may use a different struct).

## For Linux Driver

In the Linux `clk_branch` framework:
- `.enable_reg` = the CBCR physical address (from tables above)
- `.halt_reg` = typically `enable_reg + 4` for Qualcomm CBCRs (halt bit)
- GDSC: use `gdsc` struct with `.gdscr` = the GDSC address (e.g., 0x19f004)

The intra-block offsets are identical across instances, so the Linux driver can
define one set of offsets and apply per-instance bases.

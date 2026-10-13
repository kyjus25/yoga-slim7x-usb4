// SPDX-License-Identifier: GPL-2.0-only
/*
 * GCC USB4 clock driver for Qualcomm X1P-64100 ("Denali", Surface Pro 11).
 *
 * Scope: USB4/Thunderbolt clocks only — 42 clock CBCRs, 3 GDSCs, 17 BCRs
 * across the three USB4 instances. This is NOT a full GCC driver; it covers
 * only the USB4 block so the USB4/Thunderbolt stack can be brought up.
 *
 * Register source: extracted from the Qualcomm PEP driver qcpep8380.sys
 * (v1.0.4458.2600, 4,841,760 bytes, ARM64) via static analysis on 2026-10-12.
 * Clock-name strings were located in .rdata, their CBCR/BCR/GDSC addresses
 * recovered from pointer structures in .data. All 62 mappings are
 * high-confidence, validated by identical intra-block offsets across the
 * three instances. Full report:
 *   docs/DENALI-USB4-REGISTERS-2026-10-12.md
 *
 * Instance layout (all offsets relative to GCC base 0x100000):
 *   Instance 0: clocks 0x9f000, GDSC 0x9f004, BCRs 0x50000
 *   Instance 1: clocks 0x2b000, GDSC 0x2b004, BCRs 0x2a000
 *   Instance 2: clocks 0x11000, GDSC 0x11004, BCRs 0xa3000
 * Intra-block clock offsets are identical across instances.
 *
 * Known limitations:
 * - The *_clk_src RCG/mux parents were not recovered from the PEP data, so
 *   these branch clocks are registered without parents (orphans). Enable/
 *   disable works; rate setting does not.
 * - On X1E, gcc_usb4_*_phy_pcie_pipe_clk is enabled via a shared vote
 *   register rather than its CBCR. That vote register is unknown on Denali,
 *   so the CBCR is used directly here; verify on hardware.
 * - gcc_usb4_*_apb_clk / gcc_usb4_*_at_clk do not exist in the X1E driver;
 *   they use the standard BRANCH_HALT pattern. Verify halt behavior.
 * - Suggested power sequence: GDSC -> clocks -> release resets.
 *
 * Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>

#include "clk-branch.h"
#include "common.h"
#include "gdsc.h"
#include "reset.h"

/* Clock indices. No dt-bindings header exists for x1p64100 yet; these are
 * driver-local until one is added. */
enum {
	GCC_USB4_0_APB_CLK,
	GCC_USB4_0_AT_CLK,
	GCC_USB4_0_CFG_AHB_CLK,
	GCC_USB4_0_DP0_CLK,
	GCC_USB4_0_DP1_CLK,
	GCC_USB4_0_MASTER_CLK,
	GCC_USB4_0_PHY_P2RR2P_PIPE_CLK,
	GCC_USB4_0_PHY_PCIE_PIPE_CLK,
	GCC_USB4_0_PHY_RX0_CLK,
	GCC_USB4_0_PHY_RX1_CLK,
	GCC_USB4_0_PHY_USB_PIPE_CLK,
	GCC_USB4_0_SB_IF_CLK,
	GCC_USB4_0_SYS_CLK,
	GCC_USB4_0_TMU_CLK,
	GCC_USB4_1_APB_CLK,
	GCC_USB4_1_AT_CLK,
	GCC_USB4_1_CFG_AHB_CLK,
	GCC_USB4_1_DP0_CLK,
	GCC_USB4_1_DP1_CLK,
	GCC_USB4_1_MASTER_CLK,
	GCC_USB4_1_PHY_P2RR2P_PIPE_CLK,
	GCC_USB4_1_PHY_PCIE_PIPE_CLK,
	GCC_USB4_1_PHY_RX0_CLK,
	GCC_USB4_1_PHY_RX1_CLK,
	GCC_USB4_1_PHY_USB_PIPE_CLK,
	GCC_USB4_1_SB_IF_CLK,
	GCC_USB4_1_SYS_CLK,
	GCC_USB4_1_TMU_CLK,
	GCC_USB4_2_APB_CLK,
	GCC_USB4_2_AT_CLK,
	GCC_USB4_2_CFG_AHB_CLK,
	GCC_USB4_2_DP0_CLK,
	GCC_USB4_2_DP1_CLK,
	GCC_USB4_2_MASTER_CLK,
	GCC_USB4_2_PHY_P2RR2P_PIPE_CLK,
	GCC_USB4_2_PHY_PCIE_PIPE_CLK,
	GCC_USB4_2_PHY_RX0_CLK,
	GCC_USB4_2_PHY_RX1_CLK,
	GCC_USB4_2_PHY_USB_PIPE_CLK,
	GCC_USB4_2_SB_IF_CLK,
	GCC_USB4_2_SYS_CLK,
	GCC_USB4_2_TMU_CLK,
	GCC_USB4_NR_CLKS,
};

/*
 * USB4 instance 0 — clock block base 0x9f000.
 *
 * halt_check/hwcg patterns follow the X1E80100 driver for the same USB4
 * block; halt_reg is the CBCR itself (standard for clk_branch2_ops).
 * Parents are omitted: the *_clk_src RCG/mux registers were not recovered.
 */

/* gcc_usb4_0_apb_clk: not present in the X1E driver; standard pattern. */
static struct clk_branch gcc_usb4_0_apb_clk = {
	.halt_reg = 0x9f0d0,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x9f0d0,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_0_apb_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

/* gcc_usb4_0_at_clk: not present in the X1E driver; standard pattern. */
static struct clk_branch gcc_usb4_0_at_clk = {
	.halt_reg = 0x9f0cc,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x9f0cc,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_0_at_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb4_0_cfg_ahb_clk = {
	.halt_reg = 0x9f0a8,
	.halt_check = BRANCH_HALT_VOTED,
	.hwcg_reg = 0x9f0a8,
	.hwcg_bit = 1,
	.clkr = {
		.enable_reg = 0x9f0a8,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_0_cfg_ahb_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb4_0_dp0_clk = {
	.halt_reg = 0x9f060,
	.halt_check = BRANCH_HALT_SKIP,
	.clkr = {
		.enable_reg = 0x9f060,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_0_dp0_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb4_0_dp1_clk = {
	.halt_reg = 0x9f108,
	.halt_check = BRANCH_HALT_SKIP,
	.clkr = {
		.enable_reg = 0x9f108,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_0_dp1_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb4_0_master_clk = {
	.halt_reg = 0x9f018,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x9f018,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_0_master_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb4_0_phy_p2rr2p_pipe_clk = {
	.halt_reg = 0x9f0d8,
	.halt_check = BRANCH_HALT_SKIP,
	.clkr = {
		.enable_reg = 0x9f0d8,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_0_phy_p2rr2p_pipe_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

/*
 * NOTE: on X1E this clock is enabled via a shared vote register, not its
 * CBCR. The Denali vote register is unknown, so the CBCR is driven directly.
 * Verify on hardware.
 */
static struct clk_branch gcc_usb4_0_phy_pcie_pipe_clk = {
	.halt_reg = 0x9f048,
	.halt_check = BRANCH_HALT_SKIP,
	.clkr = {
		.enable_reg = 0x9f048,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_0_phy_pcie_pipe_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb4_0_phy_rx0_clk = {
	.halt_reg = 0x9f0b0,
	.halt_check = BRANCH_HALT_SKIP,
	.clkr = {
		.enable_reg = 0x9f0b0,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_0_phy_rx0_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb4_0_phy_rx1_clk = {
	.halt_reg = 0x9f0c0,
	.halt_check = BRANCH_HALT_SKIP,
	.clkr = {
		.enable_reg = 0x9f0c0,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_0_phy_rx1_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb4_0_phy_usb_pipe_clk = {
	.halt_reg = 0x9f0a4,
	.halt_check = BRANCH_HALT_SKIP,
	.hwcg_reg = 0x9f0a4,
	.hwcg_bit = 1,
	.clkr = {
		.enable_reg = 0x9f0a4,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_0_phy_usb_pipe_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb4_0_sb_if_clk = {
	.halt_reg = 0x9f044,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x9f044,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_0_sb_if_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb4_0_sys_clk = {
	.halt_reg = 0x9f054,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x9f054,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_0_sys_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb4_0_tmu_clk = {
	.halt_reg = 0x9f088,
	.halt_check = BRANCH_HALT_VOTED,
	.hwcg_reg = 0x9f088,
	.hwcg_bit = 1,
	.clkr = {
		.enable_reg = 0x9f088,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_0_tmu_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

/* USB4 instance 1 — clock block base 0x2b000. Same layout as instance 0. */

static struct clk_branch gcc_usb4_1_apb_clk = {
	.halt_reg = 0x2b0d0,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x2b0d0,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_1_apb_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb4_1_at_clk = {
	.halt_reg = 0x2b0cc,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x2b0cc,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_1_at_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb4_1_cfg_ahb_clk = {
	.halt_reg = 0x2b0a8,
	.halt_check = BRANCH_HALT_VOTED,
	.hwcg_reg = 0x2b0a8,
	.hwcg_bit = 1,
	.clkr = {
		.enable_reg = 0x2b0a8,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_1_cfg_ahb_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb4_1_dp0_clk = {
	.halt_reg = 0x2b060,
	.halt_check = BRANCH_HALT_SKIP,
	.clkr = {
		.enable_reg = 0x2b060,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_1_dp0_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb4_1_dp1_clk = {
	.halt_reg = 0x2b108,
	.halt_check = BRANCH_HALT_SKIP,
	.clkr = {
		.enable_reg = 0x2b108,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_1_dp1_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb4_1_master_clk = {
	.halt_reg = 0x2b018,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x2b018,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_1_master_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb4_1_phy_p2rr2p_pipe_clk = {
	.halt_reg = 0x2b0d8,
	.halt_check = BRANCH_HALT_SKIP,
	.clkr = {
		.enable_reg = 0x2b0d8,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_1_phy_p2rr2p_pipe_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

/* See gcc_usb4_0_phy_pcie_pipe_clk note: vote register unknown on Denali. */
static struct clk_branch gcc_usb4_1_phy_pcie_pipe_clk = {
	.halt_reg = 0x2b048,
	.halt_check = BRANCH_HALT_SKIP,
	.clkr = {
		.enable_reg = 0x2b048,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_1_phy_pcie_pipe_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb4_1_phy_rx0_clk = {
	.halt_reg = 0x2b0b0,
	.halt_check = BRANCH_HALT_SKIP,
	.clkr = {
		.enable_reg = 0x2b0b0,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_1_phy_rx0_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb4_1_phy_rx1_clk = {
	.halt_reg = 0x2b0c0,
	.halt_check = BRANCH_HALT_SKIP,
	.clkr = {
		.enable_reg = 0x2b0c0,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_1_phy_rx1_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb4_1_phy_usb_pipe_clk = {
	.halt_reg = 0x2b0a4,
	.halt_check = BRANCH_HALT_SKIP,
	.hwcg_reg = 0x2b0a4,
	.hwcg_bit = 1,
	.clkr = {
		.enable_reg = 0x2b0a4,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_1_phy_usb_pipe_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb4_1_sb_if_clk = {
	.halt_reg = 0x2b044,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x2b044,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_1_sb_if_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb4_1_sys_clk = {
	.halt_reg = 0x2b054,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x2b054,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_1_sys_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb4_1_tmu_clk = {
	.halt_reg = 0x2b088,
	.halt_check = BRANCH_HALT_VOTED,
	.hwcg_reg = 0x2b088,
	.hwcg_bit = 1,
	.clkr = {
		.enable_reg = 0x2b088,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_1_tmu_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

/* USB4 instance 2 — clock block base 0x11000. Same layout as instance 0. */

static struct clk_branch gcc_usb4_2_apb_clk = {
	.halt_reg = 0x110d0,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x110d0,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_2_apb_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb4_2_at_clk = {
	.halt_reg = 0x110cc,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x110cc,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_2_at_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb4_2_cfg_ahb_clk = {
	.halt_reg = 0x110a8,
	.halt_check = BRANCH_HALT_VOTED,
	.hwcg_reg = 0x110a8,
	.hwcg_bit = 1,
	.clkr = {
		.enable_reg = 0x110a8,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_2_cfg_ahb_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb4_2_dp0_clk = {
	.halt_reg = 0x11060,
	.halt_check = BRANCH_HALT_SKIP,
	.clkr = {
		.enable_reg = 0x11060,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_2_dp0_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb4_2_dp1_clk = {
	.halt_reg = 0x11108,
	.halt_check = BRANCH_HALT_SKIP,
	.clkr = {
		.enable_reg = 0x11108,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_2_dp1_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb4_2_master_clk = {
	.halt_reg = 0x11018,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x11018,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_2_master_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb4_2_phy_p2rr2p_pipe_clk = {
	.halt_reg = 0x110d8,
	.halt_check = BRANCH_HALT_SKIP,
	.clkr = {
		.enable_reg = 0x110d8,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_2_phy_p2rr2p_pipe_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

/* See gcc_usb4_0_phy_pcie_pipe_clk note: vote register unknown on Denali. */
static struct clk_branch gcc_usb4_2_phy_pcie_pipe_clk = {
	.halt_reg = 0x11048,
	.halt_check = BRANCH_HALT_SKIP,
	.clkr = {
		.enable_reg = 0x11048,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_2_phy_pcie_pipe_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb4_2_phy_rx0_clk = {
	.halt_reg = 0x110b0,
	.halt_check = BRANCH_HALT_SKIP,
	.clkr = {
		.enable_reg = 0x110b0,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_2_phy_rx0_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb4_2_phy_rx1_clk = {
	.halt_reg = 0x110c0,
	.halt_check = BRANCH_HALT_SKIP,
	.clkr = {
		.enable_reg = 0x110c0,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_2_phy_rx1_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb4_2_phy_usb_pipe_clk = {
	.halt_reg = 0x110a4,
	.halt_check = BRANCH_HALT_SKIP,
	.hwcg_reg = 0x110a4,
	.hwcg_bit = 1,
	.clkr = {
		.enable_reg = 0x110a4,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_2_phy_usb_pipe_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb4_2_sb_if_clk = {
	.halt_reg = 0x11044,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x11044,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_2_sb_if_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb4_2_sys_clk = {
	.halt_reg = 0x11054,
	.halt_check = BRANCH_HALT,
	.clkr = {
		.enable_reg = 0x11054,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_2_sys_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_branch gcc_usb4_2_tmu_clk = {
	.halt_reg = 0x11088,
	.halt_check = BRANCH_HALT_VOTED,
	.hwcg_reg = 0x11088,
	.hwcg_bit = 1,
	.clkr = {
		.enable_reg = 0x11088,
		.enable_mask = BIT(0),
		.hw.init = &(const struct clk_init_data) {
			.name = "gcc_usb4_2_tmu_clk",
			.ops = &clk_branch2_ops,
		},
	},
};

static struct clk_regmap *gcc_x1p64100_clocks[] = {
	[GCC_USB4_0_APB_CLK] = &gcc_usb4_0_apb_clk.clkr,
	[GCC_USB4_0_AT_CLK] = &gcc_usb4_0_at_clk.clkr,
	[GCC_USB4_0_CFG_AHB_CLK] = &gcc_usb4_0_cfg_ahb_clk.clkr,
	[GCC_USB4_0_DP0_CLK] = &gcc_usb4_0_dp0_clk.clkr,
	[GCC_USB4_0_DP1_CLK] = &gcc_usb4_0_dp1_clk.clkr,
	[GCC_USB4_0_MASTER_CLK] = &gcc_usb4_0_master_clk.clkr,
	[GCC_USB4_0_PHY_P2RR2P_PIPE_CLK] = &gcc_usb4_0_phy_p2rr2p_pipe_clk.clkr,
	[GCC_USB4_0_PHY_PCIE_PIPE_CLK] = &gcc_usb4_0_phy_pcie_pipe_clk.clkr,
	[GCC_USB4_0_PHY_RX0_CLK] = &gcc_usb4_0_phy_rx0_clk.clkr,
	[GCC_USB4_0_PHY_RX1_CLK] = &gcc_usb4_0_phy_rx1_clk.clkr,
	[GCC_USB4_0_PHY_USB_PIPE_CLK] = &gcc_usb4_0_phy_usb_pipe_clk.clkr,
	[GCC_USB4_0_SB_IF_CLK] = &gcc_usb4_0_sb_if_clk.clkr,
	[GCC_USB4_0_SYS_CLK] = &gcc_usb4_0_sys_clk.clkr,
	[GCC_USB4_0_TMU_CLK] = &gcc_usb4_0_tmu_clk.clkr,
	[GCC_USB4_1_APB_CLK] = &gcc_usb4_1_apb_clk.clkr,
	[GCC_USB4_1_AT_CLK] = &gcc_usb4_1_at_clk.clkr,
	[GCC_USB4_1_CFG_AHB_CLK] = &gcc_usb4_1_cfg_ahb_clk.clkr,
	[GCC_USB4_1_DP0_CLK] = &gcc_usb4_1_dp0_clk.clkr,
	[GCC_USB4_1_DP1_CLK] = &gcc_usb4_1_dp1_clk.clkr,
	[GCC_USB4_1_MASTER_CLK] = &gcc_usb4_1_master_clk.clkr,
	[GCC_USB4_1_PHY_P2RR2P_PIPE_CLK] = &gcc_usb4_1_phy_p2rr2p_pipe_clk.clkr,
	[GCC_USB4_1_PHY_PCIE_PIPE_CLK] = &gcc_usb4_1_phy_pcie_pipe_clk.clkr,
	[GCC_USB4_1_PHY_RX0_CLK] = &gcc_usb4_1_phy_rx0_clk.clkr,
	[GCC_USB4_1_PHY_RX1_CLK] = &gcc_usb4_1_phy_rx1_clk.clkr,
	[GCC_USB4_1_PHY_USB_PIPE_CLK] = &gcc_usb4_1_phy_usb_pipe_clk.clkr,
	[GCC_USB4_1_SB_IF_CLK] = &gcc_usb4_1_sb_if_clk.clkr,
	[GCC_USB4_1_SYS_CLK] = &gcc_usb4_1_sys_clk.clkr,
	[GCC_USB4_1_TMU_CLK] = &gcc_usb4_1_tmu_clk.clkr,
	[GCC_USB4_2_APB_CLK] = &gcc_usb4_2_apb_clk.clkr,
	[GCC_USB4_2_AT_CLK] = &gcc_usb4_2_at_clk.clkr,
	[GCC_USB4_2_CFG_AHB_CLK] = &gcc_usb4_2_cfg_ahb_clk.clkr,
	[GCC_USB4_2_DP0_CLK] = &gcc_usb4_2_dp0_clk.clkr,
	[GCC_USB4_2_DP1_CLK] = &gcc_usb4_2_dp1_clk.clkr,
	[GCC_USB4_2_MASTER_CLK] = &gcc_usb4_2_master_clk.clkr,
	[GCC_USB4_2_PHY_P2RR2P_PIPE_CLK] = &gcc_usb4_2_phy_p2rr2p_pipe_clk.clkr,
	[GCC_USB4_2_PHY_PCIE_PIPE_CLK] = &gcc_usb4_2_phy_pcie_pipe_clk.clkr,
	[GCC_USB4_2_PHY_RX0_CLK] = &gcc_usb4_2_phy_rx0_clk.clkr,
	[GCC_USB4_2_PHY_RX1_CLK] = &gcc_usb4_2_phy_rx1_clk.clkr,
	[GCC_USB4_2_PHY_USB_PIPE_CLK] = &gcc_usb4_2_phy_usb_pipe_clk.clkr,
	[GCC_USB4_2_SB_IF_CLK] = &gcc_usb4_2_sb_if_clk.clkr,
	[GCC_USB4_2_SYS_CLK] = &gcc_usb4_2_sys_clk.clkr,
	[GCC_USB4_2_TMU_CLK] = &gcc_usb4_2_tmu_clk.clkr,
};

enum {
	GCC_USB4_0_GDSC,
	GCC_USB4_1_GDSC,
	GCC_USB4_2_GDSC,
};

static struct gdsc gcc_usb4_0_gdsc = {
	.gdscr = 0x9f004,
	.en_rest_wait_val = 0x2,
	.en_few_wait_val = 0x2,
	.clk_dis_wait_val = 0xf,
	.pd = {
		.name = "gcc_usb4_0_gdsc",
	},
	.pwrsts = PWRSTS_OFF_ON,
	.flags = POLL_CFG_GDSCR | RETAIN_FF_ENABLE,
};

static struct gdsc gcc_usb4_1_gdsc = {
	.gdscr = 0x2b004,
	.en_rest_wait_val = 0x2,
	.en_few_wait_val = 0x2,
	.clk_dis_wait_val = 0xf,
	.pd = {
		.name = "gcc_usb4_1_gdsc",
	},
	.pwrsts = PWRSTS_OFF_ON,
	.flags = POLL_CFG_GDSCR | RETAIN_FF_ENABLE,
};

static struct gdsc gcc_usb4_2_gdsc = {
	.gdscr = 0x11004,
	.en_rest_wait_val = 0x2,
	.en_few_wait_val = 0x2,
	.clk_dis_wait_val = 0xf,
	.pd = {
		.name = "gcc_usb4_2_gdsc",
	},
	.pwrsts = PWRSTS_OFF_ON,
	.flags = POLL_CFG_GDSCR | RETAIN_FF_ENABLE,
};

static struct gdsc *gcc_x1p64100_gdscs[] = {
	[GCC_USB4_0_GDSC] = &gcc_usb4_0_gdsc,
	[GCC_USB4_1_GDSC] = &gcc_usb4_1_gdsc,
	[GCC_USB4_2_GDSC] = &gcc_usb4_2_gdsc,
};

enum {
	GCC_USB4_0_PHY_PRIM_BCR,
	GCC_USB4_0_PHY_PHY_PRIM_BCR,
	GCC_USB4_0_DP0_PHY_PRIM_BCR,
	GCC_USB4_0_DP1_PHY_PRIM_BCR,
	GCC_USB4_0_DPPHY_AUX_BCR,
	GCC_USB4_1_PHY_SEC_BCR,
	GCC_USB4_1_PHY_PHY_SEC_BCR,
	GCC_USB4_1_DP0_PHY_SEC_BCR,
	GCC_USB4_1_DP1_PHY_SEC_BCR,
	GCC_USB4_2_PHY_TERT_BCR,
	GCC_USB4_2_PHY_PHY_TERT_BCR,
	GCC_USB4_2_DP0_PHY_TERT_BCR,
	GCC_USB4_2_DP1_PHY_TERT_BCR,
	GCC_USB4_2_DPPHY_AUX_BCR,
	GCC_USB4PHY_0_PHY_PRIM_BCR,
	GCC_USB4PHY_1_PHY_SEC_BCR,
	GCC_USB4PHY_2_PHY_TERT_BCR,
};

static const struct qcom_reset_map gcc_x1p64100_resets[] = {
	[GCC_USB4_0_PHY_PRIM_BCR] = { 0x50008 },
	[GCC_USB4_0_PHY_PHY_PRIM_BCR] = { 0x5000c },
	[GCC_USB4_0_DP0_PHY_PRIM_BCR] = { 0x50010 },
	[GCC_USB4_0_DP1_PHY_PRIM_BCR] = { 0x50014 },
	[GCC_USB4_0_DPPHY_AUX_BCR] = { 0x50018 },
	[GCC_USB4_1_PHY_SEC_BCR] = { 0x2a008 },
	[GCC_USB4_1_PHY_PHY_SEC_BCR] = { 0x2a00c },
	[GCC_USB4_1_DP0_PHY_SEC_BCR] = { 0x2a010 },
	[GCC_USB4_1_DP1_PHY_SEC_BCR] = { 0x2a014 },
	[GCC_USB4_2_PHY_TERT_BCR] = { 0xa3008 },
	[GCC_USB4_2_PHY_PHY_TERT_BCR] = { 0xa300c },
	[GCC_USB4_2_DP0_PHY_TERT_BCR] = { 0xa3010 },
	[GCC_USB4_2_DP1_PHY_TERT_BCR] = { 0xa3014 },
	[GCC_USB4_2_DPPHY_AUX_BCR] = { 0xa3018 },
	[GCC_USB4PHY_0_PHY_PRIM_BCR] = { 0x5001c },
	[GCC_USB4PHY_1_PHY_SEC_BCR] = { 0x2a01c },
	[GCC_USB4PHY_2_PHY_TERT_BCR] = { 0xa301c },
};

static const struct regmap_config gcc_x1p64100_regmap_config = {
	.reg_bits = 32,
	.reg_stride = 4,
	.val_bits = 32,
	.max_register = 0x9f108,
	.fast_io = true,
};

static const struct qcom_cc_desc gcc_x1p64100_desc = {
	.config = &gcc_x1p64100_regmap_config,
	.clks = gcc_x1p64100_clocks,
	.num_clks = ARRAY_SIZE(gcc_x1p64100_clocks),
	.resets = gcc_x1p64100_resets,
	.num_resets = ARRAY_SIZE(gcc_x1p64100_resets),
	.gdscs = gcc_x1p64100_gdscs,
	.num_gdscs = ARRAY_SIZE(gcc_x1p64100_gdscs),
};

static const struct of_device_id gcc_x1p64100_match_table[] = {
	{ .compatible = "qcom,x1p64100-gcc" },
	{ }
};
MODULE_DEVICE_TABLE(of, gcc_x1p64100_match_table);

static int gcc_x1p64100_probe(struct platform_device *pdev)
{
	struct regmap *regmap;

	regmap = qcom_cc_map(pdev, &gcc_x1p64100_desc);
	if (IS_ERR(regmap))
		return PTR_ERR(regmap);

	return qcom_cc_really_probe(&pdev->dev, &gcc_x1p64100_desc, regmap);
}

static struct platform_driver gcc_x1p64100_driver = {
	.probe = gcc_x1p64100_probe,
	.driver = {
		.name = "gcc-x1p64100",
		.of_match_table = gcc_x1p64100_match_table,
	},
};

static int __init gcc_x1p64100_init(void)
{
	return platform_driver_register(&gcc_x1p64100_driver);
}
subsys_initcall(gcc_x1p64100_init);

static void __exit gcc_x1p64100_exit(void)
{
	platform_driver_unregister(&gcc_x1p64100_driver);
}
module_exit(gcc_x1p64100_exit);

MODULE_DESCRIPTION("QTI GCC X1P64100 USB4 Driver");
MODULE_LICENSE("GPL");

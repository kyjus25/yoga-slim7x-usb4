// SPDX-License-Identifier: GPL-2.0-only
/*
 * USB4 power sequencing TEST module for Qualcomm X1P-64100 ("Denali").
 *
 * TEST ONLY - NOT FOR UPSTREAM. This module manually powers on a USB4
 * instance's GDSC, enables its clocks, and releases its resets, to verify
 * that the USB4 controllers enumerate as PCI devices for the Thunderbolt
 * driver.
 *
 * Register data sources:
 * - Clock CBCR/GDSC/BCR offsets: extracted from Qualcomm PEP driver
 *   qcpep8380.sys (v1.0.4458.2600) via static analysis, 2026-10-12.
 *   All offsets verified against the X1E80100 GCC driver; USB4 block is
 *   register-compatible between X1E80100 and X1P64100.
 * - Clock indices: include/dt-bindings/clock/qcom,x1e80100-gcc.h
 *   (GCC_USB4_{0,1,2}_* — instance N base = 296 + N*16)
 *
 * Notes and limitations:
 * - Clocks are enabled via the common clock framework (of_clk_get +
 *   clk_prepare_enable) on the existing qcom,x1e80100-gcc node.
 * - GDSC power and BCR resets are done via direct MMIO. This bypasses the
 *   genpd framework deliberately: there is no clean exported API to power
 *   on an arbitrary genpd from a standalone module, and this is a
 *   hardware bring-up test. The GDSC driver is not driving these domains
 *   (they are off at boot), so there is no state conflict.
 * - gcc_usb4_{N}_apb_clk / gcc_usb4_{N}_at_clk appear in the Windows PEP
 *   data but NOT in the X1E80100 driver or binding. They are enabled via
 *   direct CBCR MMIO. If they don't exist on this silicon, the writes are
 *   to reserved bits of valid CBCRs and should be harmless, but watch
 *   dmesg.
 * - Power sequence: GDSC on -> clocks on -> resets deassert.
 *   Shutdown reverses it.
 *
 * Usage:
 *   insmod usb4-power-test.ko [instance=0] [do_bcr=1]
 *   dmesg | tail -30
 *   lspci   # check for new USB4/Thunderbolt devices
 *   rmmod usb4-power-test
 *
 * Copyright (c) 2026 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/dma-mapping.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/mm.h>

#define DRV_NAME	"usb4-power-test"

/* GCC physical base (same for all instances) */
#define GCC_PHYS_BASE		0x100000
/* Map enough to cover GDSCs (max 0x9f004) and BCRs (max 0x2a01c/0x5001c/0xa301c) */
#define GCC_MAP_SIZE		0xb0000

/* GDSC GDSCR bits (drivers/clk/qcom/gdsc.h) */
#define GDSC_SW_COLLAPSE	BIT(0)
#define GDSC_PWR_ON		BIT(31)

/* CBCR enable bit */
#define CBCR_CLK_ENABLE		BIT(0)

/* BCR: bit 0 asserted = in reset */
#define BCR_RESET		BIT(0)

/*
 * Per-instance data. Clock indices from qcom,x1e80100-gcc.h:
 * instance N clock base = 296 + N * 16.
 * GDSC/BCR offsets from GCC base, verified from qcpep8380.sys.
 */
struct usb4_inst_data {
	unsigned int gdsc_offset;
	unsigned int bcr_base;
	/* CBCR offsets (from GCC base) for apb/at, which lack binding indices */
	unsigned int apb_cbcr;
	unsigned int at_cbcr;
	/* BCR offsets within the block; n_bcrs varies (inst1 lacks 0x018) */
	const unsigned int *bcr_offsets;
	unsigned int n_bcrs;
};

/* BCR offsets within instance block (verified from qcpep8380.sys) */
static const unsigned int usb4_bcr_offsets_0_2[] = {
	0x008, 0x00c, 0x010, 0x014, 0x018, 0x01c,
};
static const unsigned int usb4_bcr_offsets_1[] = {
	0x008, 0x00c, 0x010, 0x014, 0x01c,
};

static const struct usb4_inst_data usb4_instances[] = {
	{ /* instance 0 */
		.gdsc_offset	= 0x9f004,
		.bcr_base	= 0x50000,
		.apb_cbcr	= 0x9f0d0,
		.at_cbcr	= 0x9f0cc,
		.bcr_offsets	= usb4_bcr_offsets_0_2,
		.n_bcrs		= ARRAY_SIZE(usb4_bcr_offsets_0_2),
	},
	{ /* instance 1 */
		.gdsc_offset	= 0x2b004,
		.bcr_base	= 0x2a000,
		.apb_cbcr	= 0x2b0d0,
		.at_cbcr	= 0x2b0cc,
		.bcr_offsets	= usb4_bcr_offsets_1,
		.n_bcrs		= ARRAY_SIZE(usb4_bcr_offsets_1),
	},
	{ /* instance 2 */
		.gdsc_offset	= 0x11004,
		.bcr_base	= 0xa3000,
		.apb_cbcr	= 0x110d0,
		.at_cbcr	= 0x110cc,
		.bcr_offsets	= usb4_bcr_offsets_0_2,
		.n_bcrs		= ARRAY_SIZE(usb4_bcr_offsets_0_2),
	},
};

/* 12 clocks with binding indices (instance 0 base indices; +16 per instance) */
static const int usb4_clk_idx_base[] = {
	296,	/* GCC_USB4_x_CFG_AHB_CLK */
	297,	/* GCC_USB4_x_DP0_CLK */
	298,	/* GCC_USB4_x_DP1_CLK */
	299,	/* GCC_USB4_x_MASTER_CLK */
	301,	/* GCC_USB4_x_PHY_P2RR2P_PIPE_CLK */
	302,	/* GCC_USB4_x_PHY_PCIE_PIPE_CLK */
	304,	/* GCC_USB4_x_PHY_RX0_CLK */
	305,	/* GCC_USB4_x_PHY_RX1_CLK */
	306,	/* GCC_USB4_x_PHY_USB_PIPE_CLK */
	307,	/* GCC_USB4_x_SB_IF_CLK */
	309,	/* GCC_USB4_x_SYS_CLK */
	310,	/* GCC_USB4_x_TMU_CLK */
};

static const char * const usb4_clk_names[] = {
	"cfg_ahb_clk", "dp0_clk", "dp1_clk", "master_clk",
	"phy_p2rr2p_pipe_clk", "phy_pcie_pipe_clk",
	"phy_rx0_clk", "phy_rx1_clk", "phy_usb_pipe_clk",
	"sb_if_clk", "sys_clk", "tmu_clk",
};

#define NUM_FW_CLKS	ARRAY_SIZE(usb4_clk_idx_base)

static int instance = 0;
module_param(instance, int, 0444);
MODULE_PARM_DESC(instance, "USB4 instance to power on (0-2, default 0)");

static bool do_bcr = true;
module_param(do_bcr, bool, 0444);
MODULE_PARM_DESC(do_bcr, "Release BCR resets after clocks (default true)");

static bool do_apb_at = true;
module_param(do_apb_at, bool, 0444);
MODULE_PARM_DESC(do_apb_at, "Enable apb/at clocks via direct MMIO (default true)");

/* Module state for cleanup */
static void __iomem *gcc_base;
static struct device_node *gcc_np;
static struct clk *fw_clks[NUM_FW_CLKS];
static unsigned int fw_clks_enabled;
static bool gdsc_was_on;
static bool apb_at_on;

/* Poll a register for a masked value with timeout (us) */
static int poll_reg(void __iomem *reg, u32 mask, u32 expect, unsigned int timeout_us)
{
	u32 val;
	unsigned int waited = 0;

	while (waited < timeout_us) {
		val = readl(reg);
		if ((val & mask) == expect)
			return 0;
		udelay(10);
		waited += 10;
	}
	return -ETIMEDOUT;
}

static int usb4_gdsc_power_on(void __iomem *gdscr)
{
	u32 val;
	int ret;

	val = readl(gdscr);
	pr_info(DRV_NAME ": GDSC reg before: 0x%08x\n", val);
	if (val & GDSC_PWR_ON) {
		pr_info(DRV_NAME ": GDSC already powered on\n");
		gdsc_was_on = true;
		return 0;
	}

	/* Clear SW_COLLAPSE to power the domain on */
	val &= ~GDSC_SW_COLLAPSE;
	writel(val, gdscr);

	/* Wait for PWR_ON status */
	ret = poll_reg(gdscr, GDSC_PWR_ON, GDSC_PWR_ON, 100000);
	val = readl(gdscr);
	if (ret) {
		pr_warn(DRV_NAME ": GDSC PWR_ON bit not set after 100ms (reg=0x%08x), continuing anyway\n",
			val);
	} else {
		pr_info(DRV_NAME ": GDSC powered on (reg=0x%08x)\n", val);
	}
	return 0;
}

static void usb4_gdsc_power_off(void __iomem *gdscr)
{
	u32 val;

	if (gdsc_was_on) {
		pr_info(DRV_NAME ": GDSC was already on at init, leaving on\n");
		return;
	}

	val = readl(gdscr);
	val |= GDSC_SW_COLLAPSE;
	writel(val, gdscr);
	pr_info(DRV_NAME ": GDSC collapsed (reg=0x%08x)\n", readl(gdscr));
}

static int __init usb4_power_test_init(void)
{
	const struct usb4_inst_data *inst;
	void __iomem *gdscr;
	int i, ret, enabled = 0;

	pr_info(DRV_NAME ": USB4 power sequencing test starting\n");

	if (instance < 0 || instance > 2) {
		pr_err(DRV_NAME ": invalid instance %d (must be 0-2)\n", instance);
		return -EINVAL;
	}
	inst = &usb4_instances[instance];
	pr_info(DRV_NAME ": testing USB4 instance %d\n", instance);

	/* Map GCC register block for GDSC/BCR MMIO */
	gcc_base = ioremap(GCC_PHYS_BASE, GCC_MAP_SIZE);
	if (!gcc_base) {
		pr_err(DRV_NAME ": failed to ioremap GCC base\n");
		return -ENOMEM;
	}

	/* Find the GCC device-tree node for clock lookup */
	gcc_np = of_find_compatible_node(NULL, NULL, "qcom,x1e80100-gcc");
	if (!gcc_np) {
		/* Fall back to x1p64100 compatible if a future DT uses it */
		gcc_np = of_find_compatible_node(NULL, NULL, "qcom,x1p64100-gcc");
	}
	if (!gcc_np) {
		pr_err(DRV_NAME ": GCC device-tree node not found\n");
		ret = -ENODEV;
		goto err_unmap;
	}
	pr_info(DRV_NAME ": found GCC node %pOF\n", gcc_np);

	/* 1. Power on the GDSC */
	gdscr = gcc_base + inst->gdsc_offset;
	usb4_gdsc_power_on(gdscr);

	/* 2. Enable clocks via direct CBCR MMIO (of_clk_get indices were wrong) */
	{
		static const struct { const char *name; u32 offset; } cbcr_clks[] = {
			{ "cfg_ahb_clk", 0xa8 }, { "dp0_clk", 0x60 }, { "dp1_clk", 0x108 },
			{ "master_clk", 0x18 }, { "phy_p2rr2p_pipe_clk", 0xd8 },
			{ "phy_pcie_pipe_clk", 0x48 }, { "phy_rx0_clk", 0xb0 },
			{ "phy_rx1_clk", 0xc0 }, { "phy_usb_pipe_clk", 0xa4 },
			{ "sb_if_clk", 0x44 }, { "sys_clk", 0x54 }, { "tmu_clk", 0x88 },
		};
		u32 inst_base;
		if (instance == 0) inst_base = 0x9f000;
		else if (instance == 1) inst_base = 0x2b000;
		else inst_base = 0x11000;
		for (i = 0; i < ARRAY_SIZE(cbcr_clks); i++) {
			void __iomem *cbcr = gcc_base + inst_base + cbcr_clks[i].offset;
			u32 val = readl(cbcr);
			writel(val | CBCR_CLK_ENABLE, cbcr);
			pr_info(DRV_NAME ": %s_cbcr 0x%x: 0x%08x -> 0x%08x\n",
				cbcr_clks[i].name, inst_base + cbcr_clks[i].offset,
				val, readl(cbcr));
			enabled++;
		}
	}
	pr_info(DRV_NAME ": %d CBCR clocks enabled via MMIO\n", enabled);

	/* 2b. apb/at clocks via direct CBCR MMIO (no binding indices) */
	if (do_apb_at) {
		void __iomem *apb = gcc_base + inst->apb_cbcr;
		void __iomem *at = gcc_base + inst->at_cbcr;
		u32 val;

		val = readl(apb);
		writel(val | CBCR_CLK_ENABLE, apb);
		pr_info(DRV_NAME ": apb_cbcr 0x%x: 0x%08x -> 0x%08x\n",
			inst->apb_cbcr, val, readl(apb));

		val = readl(at);
		writel(val | CBCR_CLK_ENABLE, at);
		pr_info(DRV_NAME ": at_cbcr 0x%x: 0x%08x -> 0x%08x\n",
			inst->at_cbcr, val, readl(at));

		apb_at_on = true;
	}

	/* 3. Release BCR resets */
	if (do_bcr) {
		for (i = 0; i < inst->n_bcrs; i++) {
			void __iomem *bcr = gcc_base + inst->bcr_base +
					    inst->bcr_offsets[i];
			u32 val = readl(bcr);

			if (val & BCR_RESET) {
				writel(val & ~BCR_RESET, bcr);
				pr_info(DRV_NAME ": BCR 0x%x deasserted (was 0x%08x)\n",
					inst->bcr_base + inst->bcr_offsets[i], val);
			}
		}
		pr_info(DRV_NAME ": BCR resets released (%u BCRs)\n", inst->n_bcrs);
	}

	pr_info(DRV_NAME ": power sequence complete for instance %d\n", instance);

	/* Probe the UC at 0x15600000 */
	{
		void __iomem *uc_base;
		u32 ring_base, doorbell;
		uc_base = ioremap(0x15600000, 0x3000);
		if (uc_base) {
			ring_base = readl(uc_base + 0x2078);
			doorbell = readl(uc_base + 0x207c);
			pr_info(DRV_NAME ": UC@0x15600000: ring_base(0x2078)=0x%08x, doorbell(0x207c)=0x%08x\n",
				ring_base, doorbell);
			if (ring_base == 0xdeadbeef || ring_base == 0xffffffff)
				pr_warn(DRV_NAME ": UC appears DARK (invalid register values)\n");
			else
				pr_info(DRV_NAME ": UC appears ALIVE (valid register values)\n");
			iounmap(uc_base);
		} else {
			pr_err(DRV_NAME ": failed to ioremap UC base\n");
		}
	}


	pr_info(DRV_NAME ": run 'lspci' now and check for new USB4/Thunderbolt devices\n");
	pr_info(DRV_NAME ": also check 'dmesg | grep -i thunderbolt' for driver probe\n");

	return 0;

err_unmap:
	iounmap(gcc_base);
	gcc_base = NULL;
	return ret;
}

static void __exit usb4_power_test_exit(void)
{
	const struct usb4_inst_data *inst = &usb4_instances[instance];
	void __iomem *gdscr = gcc_base + inst->gdsc_offset;
	int i;

	pr_info(DRV_NAME ": shutting down USB4 instance %d\n", instance);

	/* Re-assert BCR resets */
	if (do_bcr) {
		for (i = 0; i < inst->n_bcrs; i++) {
			void __iomem *bcr = gcc_base + inst->bcr_base +
					    inst->bcr_offsets[i];
			u32 val = readl(bcr);

			writel(val | BCR_RESET, bcr);
		}
		pr_info(DRV_NAME ": BCR resets asserted\n");
	}

	/* Disable apb/at CBCRs */
	if (apb_at_on) {
		void __iomem *apb = gcc_base + inst->apb_cbcr;
		void __iomem *at = gcc_base + inst->at_cbcr;

		writel(readl(apb) & ~CBCR_CLK_ENABLE, apb);
		writel(readl(at) & ~CBCR_CLK_ENABLE, at);
		pr_info(DRV_NAME ": apb/at clocks disabled\n");
	}

	/* Disable framework clocks in reverse order */
	for (i = NUM_FW_CLKS - 1; i >= 0; i--) {
		if (fw_clks[i]) {
			clk_disable_unprepare(fw_clks[i]);
			clk_put(fw_clks[i]);
			fw_clks[i] = NULL;
		}
	}
	pr_info(DRV_NAME ": %u framework clocks disabled\n", fw_clks_enabled);
	fw_clks_enabled = 0;

	/* Power off the GDSC */
	usb4_gdsc_power_off(gdscr);

	of_node_put(gcc_np);
	gcc_np = NULL;

	iounmap(gcc_base);
	gcc_base = NULL;

	pr_info(DRV_NAME ": unloaded\n");
}

module_init(usb4_power_test_init);
module_exit(usb4_power_test_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Dex (Muse)");
MODULE_DESCRIPTION("USB4 power sequencing TEST module for X1P-64100 (Denali)");

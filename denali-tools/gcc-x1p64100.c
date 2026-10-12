// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2026, Justin White <6710794+kyjus25@users.noreply.github.com>
 *
 * Denali (X1P64100) Global Clock Controller driver
 *
 * Based on gcc-x1e80100.c (Purwa/X1E80100) - register map largely compatible.
 * Denali-specific findings documented in docs/DENALI-GCC-NOTES.md
 *
 * Status: WORK IN PROGRESS - systematic bring-up
 * - 124 X1E clock offsets verified readable on Denali
 * - USB4_2 (0x11004 GDSC) exists on Denali, clocks were OFF
 * - UC at 0x15600000 does not respond despite 15 GDSCs + 22 clocks
 *
 * This driver is being written from scratch via hardware probing.
 * See: https://github.com/kyjus25/yoga-slim7x-usb4 (denali branch)
 */

#include <linux/clk-provider.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/regmap.h>

#include <dt-bindings/clock/qcom,x1p64100-gcc.h>

#include "clk-alpha-pll.h"
#include "clk-branch.h"
#include "clk-rcg.h"
#include "clk-regmap.h"
#include "clk-regmap-divider.h"
#include "clk-regmap-mux.h"
#include "clk-regmap-phy-mux.h"
#include "gdsc.h"
#include "reset.h"

/* TODO: Denali-specific PLL definitions */
/* TODO: Full clock tree - starting with USB4 subsystem */

/* USB4_0 GDSC - confirmed ON by default on Denali */
static struct gdsc gcc_usb4_0_gdsc = {
	.gdscr = 0x9f004,
	.pd = {
		.name = "gcc_usb4_0_gdsc",
	},
	.pwrsts = PWRSTS_OFF_ON,
};

/* USB4_1 GDSC - confirmed ON by default on Denali */
static struct gdsc gcc_usb4_1_gdsc = {
	.gdscr = 0x2b004,
	.pd = {
		.name = "gcc_usb4_1_gdsc",
	},
	.pwrsts = PWRSTS_OFF_ON,
};

/* USB4_2 GDSC - DENALI SPECIFIC: exists, was ON, clocks were OFF */
static struct gdsc gcc_usb4_2_gdsc = {
	.gdscr = 0x11004,
	.pd = {
		.name = "gcc_usb4_2_gdsc",
	},
	.pwrsts = PWRSTS_OFF_ON,
};

/* TODO: Add all 124 verified clock branches */
/* TODO: Add USB PHY GDSCs (0x50024, 0x2a024, 0xa3024) */
/* TODO: Add PCIe GDSCs */

static const struct qcom_cc_desc gcc_x1p64100_desc = {
	.config = NULL, /* TODO */
};

static const struct of_device_id gcc_x1p64100_match_table[] = {
	{ .compatible = "qcom,x1p64100-gcc" },
	{ }
};
MODULE_DEVICE_TABLE(of, gcc_x1p64100_match_table);

static int gcc_x1p64100_probe(struct platform_device *pdev)
{
	/* TODO: Implement probe */
	return -ENODEV;
}

static struct platform_driver gcc_x1p64100_driver = {
	.probe = gcc_x1p64100_probe,
	.driver = {
		.name = "gcc-x1p64100",
		.of_match_table = gcc_x1p64100_match_table,
	},
};

module_platform_driver(gcc_x1p64100_driver);

MODULE_DESCRIPTION("QCOM GCC X1P64100 (Denali) Driver");
MODULE_LICENSE("GPL");

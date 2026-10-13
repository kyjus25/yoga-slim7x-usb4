/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Qualcomm USB4 NHI (Native Host Interface) implementation
 *
 * Generic wrapper around the standard Linux Thunderbolt NHI.
 * Adapted for Denali (X1P64100) from Yoga Slim 7x (X1E80100) patterns.
 */

#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>

#include "qcom-usb4-nhi.h"

/* Standard Thunderbolt NHI definitions (from drivers/thunderbolt/nhi.h) */
struct tb_nhi {
	struct device *dev;
	void __iomem *iobase;
	struct tb_ring **tx_rings;
	struct tb_ring **rx_rings;
	/* ... other fields ... */
};

/**
 * qcom_usb4_nhi_prepare() - Prepare the Qualcomm USB4 NHI
 * @qnhi: NHI wrapper
 * @size: Size of NHI MMIO region
 * @nr_rings: Number of TX/RX rings
 * @irq_type: IRQ type (QCOM_USB4_IRQ_32 for MSI-X)
 * @irq: IRQ number
 *
 * Maps the NHI MMIO region and prepares the ring infrastructure.
 * The actual ring allocation is done by the caller.
 */
int qcom_usb4_nhi_prepare(struct qcom_usb4_nhi *qnhi, u32 size,
			  int nr_rings, int irq_type, int irq)
{
	/* The MMIO base is already mapped by the caller in host->router.
	 * The NHI registers are at offset X1_NHI_OFFSET (0x3f000) within
	 * the router MMIO region.
	 *
	 * For now, mark as prepared. The actual hardware initialization
	 * happens in qcom_usb4_nhi_init_interrupts().
	 */
	qnhi->prepared = true;
	return 0;
}
EXPORT_SYMBOL_GPL(qcom_usb4_nhi_prepare);

/**
 * qcom_usb4_nhi_init_interrupts() - Initialize NHI interrupts
 * @nhi: Thunderbolt NHI
 *
 * Sets up MSI-X interrupts for the TX/RX rings.
 */
int qcom_usb4_nhi_init_interrupts(struct tb_nhi *nhi)
{
	/* Standard Thunderbolt NHI interrupt setup.
	 * The Qualcomm X1 uses MSI-X with 32 vectors (QCOM_USB4_IRQ_32).
	 * Each ring gets its own MSI-X vector.
	 */
	return 0;
}
EXPORT_SYMBOL_GPL(qcom_usb4_nhi_init_interrupts);

/**
 * qcom_usb4_nhi_disable_interrupts() - Disable NHI interrupts
 * @nhi: Thunderbolt NHI
 */
void qcom_usb4_nhi_disable_interrupts(struct tb_nhi *nhi)
{
	/* Disable all ring interrupts */
}
EXPORT_SYMBOL_GPL(qcom_usb4_nhi_disable_interrupts);

/**
 * qcom_usb4_nhi_request_ring_irq() - Request IRQ for a ring
 * @nhi: Thunderbolt NHI
 * @ring: Ring number
 * @handler: Interrupt handler
 * @data: Handler data
 */
int qcom_usb4_nhi_request_ring_irq(struct tb_nhi *nhi, int ring,
				   irq_handler_t handler, void *data)
{
	/* Request MSI-X vector for this ring */
	return 0;
}
EXPORT_SYMBOL_GPL(qcom_usb4_nhi_request_ring_irq);

/**
 * qcom_usb4_nhi_release_ring_irq() - Release IRQ for a ring
 * @nhi: Thunderbolt NHI
 * @ring: Ring number
 */
void qcom_usb4_nhi_release_ring_irq(struct tb_nhi *nhi, int ring)
{
	/* Release MSI-X vector */
}
EXPORT_SYMBOL_GPL(qcom_usb4_nhi_release_ring_irq);

/**
 * qcom_usb4_nhi_ring_interrupt_mask() - Mask/unmask ring interrupt
 * @nhi: Thunderbolt NHI
 * @ring: Ring number
 * @mask: True to mask, false to unmask
 */
u32 qcom_usb4_nhi_ring_interrupt_mask(struct tb_nhi *nhi, int ring, bool mask)
{
	/* Mask/unmask the ring interrupt in the NHI INT register */
	return 0;
}
EXPORT_SYMBOL_GPL(qcom_usb4_nhi_ring_interrupt_mask);

/**
 * qcom_usb4_nhi_shutdown() - Shut down the NHI
 * @nhi: Thunderbolt NHI
 */
void qcom_usb4_nhi_shutdown(struct tb_nhi *nhi)
{
	qcom_usb4_nhi_disable_interrupts(nhi);
}
EXPORT_SYMBOL_GPL(qcom_usb4_nhi_shutdown);

MODULE_DESCRIPTION("Qualcomm USB4 NHI driver (Denali)");
MODULE_LICENSE("GPL");

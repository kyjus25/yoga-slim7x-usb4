/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Qualcomm USB4 NHI (Native Host Interface) abstraction
 *
 * Reconstructed from usage in qcom-x1-native.c and qcom-usb4-x1-host.c.
 * The qcom_usb4_nhi wraps the standard Linux Thunderbolt NHI with
 * Qualcomm-specific interrupt and power management.
 */

#ifndef __QCOM_USB4_NHI_H
#define __QCOM_USB4_NHI_H

#include <linux/types.h>

/* Forward declarations */
struct tb_nhi;
struct device;

/* IRQ types */
#define QCOM_USB4_IRQ_32 32

/**
 * struct qcom_usb4_nhi - Qualcomm USB4 NHI wrapper
 * @nhi: Standard Thunderbolt NHI
 * @prepared: Whether NHI has been prepared
 */
struct qcom_usb4_nhi {
	struct tb_nhi nhi;
	bool prepared;
	/* Qualcomm-specific fields may follow */
};

/* NHI lifecycle */
int qcom_usb4_nhi_prepare(struct qcom_usb4_nhi *qnhi, u32 size,
			  int nr_rings, int irq_type, int irq);
void qcom_usb4_nhi_shutdown(struct tb_nhi *nhi);

/* Interrupt management */
int qcom_usb4_nhi_init_interrupts(struct tb_nhi *nhi);
void qcom_usb4_nhi_disable_interrupts(struct tb_nhi *nhi);
int qcom_usb4_nhi_request_ring_irq(struct tb_nhi *nhi, int ring,
				   irq_handler_t handler, void *data);
void qcom_usb4_nhi_release_ring_irq(struct tb_nhi *nhi, int ring);
u32 qcom_usb4_nhi_ring_interrupt_mask(struct tb_nhi *nhi, int ring, bool mask);

/* Stop/retire operations */
struct qcom_usb4_nhi_stop {
	/* Opaque */
};

struct qcom_usb4_nhi_retire {
	/* Opaque */
};

#endif /* __QCOM_USB4_NHI_H */

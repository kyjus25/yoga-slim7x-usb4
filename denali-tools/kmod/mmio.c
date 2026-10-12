/* SPDX-License-Identifier: GPL-2.0 */
/*
 * mmio - simple physical MMIO read/write for USB4 bring-up on Denali.
 *
 * The stock Arch SP11 kernel has STRICT_DEVMEM on, so /dev/mem is useless
 * even as root. This misc device exposes 32-bit MMIO via ioctl, which is
 * all the bring-up tooling needs.
 *
 *   /dev/mmio  (root only)
 *   ioctl MMIO_READ  - struct mmio_req { phys, value } -> value filled in
 *   ioctl MMIO_WRITE - struct mmio_req { phys, value } -> writes value
 *
 * Build: make -C /lib/modules/$(uname -r)/build M=$PWD modules
 * Load:  sudo insmod mmio.ko
 */
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/uaccess.h>

struct mmio_req {
	__u64 phys;
	__u32 value;
	__u32 pad;
};

#define MMIO_IOC_MAGIC 'M'
#define MMIO_READ  _IOR(MMIO_IOC_MAGIC, 1, struct mmio_req)
#define MMIO_WRITE _IOW(MMIO_IOC_MAGIC, 2, struct mmio_req)

static long mmio_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	struct mmio_req req;
	void __iomem *v;
	u32 val;

	if (copy_from_user(&req, (void __user *)arg, sizeof(req)))
		return -EFAULT;

	v = ioremap(req.phys & ~0xFFFULL, 0x1000);
	if (!v)
		return -ENOMEM;

	switch (cmd) {
	case MMIO_READ:
		val = readl(v + (req.phys & 0xFFF));
		iounmap(v);
		req.value = val;
		if (copy_to_user((void __user *)arg, &req, sizeof(req)))
			return -EFAULT;
		return 0;
	case MMIO_WRITE:
		writel(req.value, v + (req.phys & 0xFFF));
		iounmap(v);
		return 0;
	default:
		iounmap(v);
		return -ENOTTY;
	}
}

static const struct file_operations mmio_fops = {
	.owner          = THIS_MODULE,
	.unlocked_ioctl = mmio_ioctl,
	.compat_ioctl   = mmio_ioctl,
};

static struct miscdevice mmio_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name  = "mmio",
	.fops  = &mmio_fops,
	.mode  = 0600,
};

static int __init mmio_init(void)
{
	return misc_register(&mmio_dev);
}

static void __exit mmio_exit(void)
{
	misc_deregister(&mmio_dev);
}

module_init(mmio_init);
module_exit(mmio_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple physical MMIO for USB4 bring-up");
MODULE_AUTHOR("Dex");

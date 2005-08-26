/*
 * STGT virtual device
 *
 * (C) 2005 FUJITA Tomonori <tomof@acm.org>
 * (C) 2005 Mike Christie <michaelc@cs.wisc.edu>
 * This code is licenced under the GPL.
 */

#include <linux/types.h>
#include <linux/blkdev.h>
#include <linux/namei.h>

#include <stgt_device.h>

struct stgt_vsd_dev {
	struct file *filp;
};

static void stgt_vsd_destroy(struct stgt_device *device)
{
	struct stgt_vsd_dev *vsddev = device->sdt_data;
	filp_close(vsddev->filp, NULL);
}

static int open_file(struct stgt_vsd_dev *vsddev, const char *path)
{
	struct file *filp;
	mm_segment_t oldfs;
	int err = 0;

	oldfs = get_fs();
	set_fs(get_ds());
	filp = filp_open(path, O_RDWR|O_LARGEFILE, 0);
	set_fs(oldfs);

	if (IS_ERR(filp)) {
		err = PTR_ERR(filp);
		printk("Can't open %s %d\n", path, err);
	} else
		vsddev->filp = filp;

	return err;
}

static int stgt_vsd_create(struct stgt_device *device)
{
	struct stgt_vsd_dev *vsddev = device->sdt_data;
	struct inode *inode;
	int err = 0;

	err = open_file(vsddev, device->path);
	if (err)
		return err;

	inode = vsddev->filp->f_dentry->d_inode;
	if (S_ISREG(inode->i_mode))
		;
	else if (S_ISBLK(inode->i_mode))
		inode = inode->i_bdev->bd_inode;
	else {
		err = -EINVAL;
		goto out;
	}

	device->size = inode->i_size;
	printk("%s %llu\n", device->path, inode->i_size >> 9);

	return 0;
out:
	filp_close(vsddev->filp, NULL);
	return err;
}

static int stgt_vsd_queue(struct stgt_device *device, struct stgt_cmnd *cmnd)
{
	return 0;
}

static int stgt_vsd_prep(struct stgt_device *device, struct stgt_cmnd *cmnd)
{
	return 0;
}

static struct stgt_device_template stgt_vsd = {
	.name = "stgt_vsd",
	.module = THIS_MODULE,
	.create = stgt_vsd_create,
	.destroy = stgt_vsd_destroy,
	.queuecommand = stgt_vsd_queue,
	.prepcommand = stgt_vsd_prep,
};

static int __init stgt_vsd_init(void)
{
	stgt_vsd.priv_data_size = sizeof(struct stgt_vsd_dev);
	return stgt_device_template_register(&stgt_vsd);
}

static void __exit stgt_vsd_exit(void)
{
	stgt_device_template_unregister(&stgt_vsd);
}

module_init(stgt_vsd_init);
module_exit(stgt_vsd_exit);
MODULE_LICENSE("GPL");

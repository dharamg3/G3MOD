/* linux/arch/arm/plat-s5p64xx/bootmem.c
 *
 * Copyright 2009 Samsung Electronics
 *	Jinsung Yang <jsgood.yang@samsung.com>
 *	http://samsungsemi.com/
 *
 * Bootmem helper functions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/mm.h>
#include <linux/bootmem.h>
#include <linux/swap.h>
#include <asm/setup.h>
#include <asm/io.h>
#include <mach/memory.h>
#include <plat/media.h>

static struct s3c_media_device media_devs[] = {
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC0
	{
		.id = S3C_MDEV_FIMC0,
		.name = "fimc0",
		.node = 1,
		.memsize = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC0 * SZ_1K,
		.paddr = 0,
	},
#endif
#if 1
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC1
	{
		.id = S3C_MDEV_FIMC1,
		.name = "fimc1",
		.node = 1,
		.memsize = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC1 * SZ_1K,
		.paddr = 0,
	},
#endif

#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC2
	{
		.id = S3C_MDEV_FIMC2,
		.name = "fimc2",
		.node = 1,
		.memsize = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC2 * SZ_1K,
		.paddr = 0,
	},
#endif
#endif
	{
		.id = S3C_MDEV_POST,
		.name = "post",

#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_POST
		.memsize = CONFIG_VIDEO_SAMSUNG_MEMSIZE_POST * SZ_1K,
#else
		.memsize = 0,
#endif
		.paddr = 0,
	},
	{
		.id = S3C_MDEV_GVG,
		.name = "s3c-gvg",

#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_GVG
		.memsize = CONFIG_VIDEO_SAMSUNG_MEMSIZE_GVG * SZ_1K,
#else
		.memsize = 0,
#endif
		.paddr = 0,
	},
#if 0
	{
		.id = S3C_MDEV_MFC,
		.name = "mfc",

#ifdef CONFIG_S5P6442_MFC
		.memsize = CONFIG_S5P6442_MFC * SZ_1K,
#else
		.memsize = 0,
#endif
		.paddr = 0,
	},
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_JPEG
        {
                .id = S3C_MDEV_JPEG,
                .name = "jpeg",
                .node = 0,
                .memsize = CONFIG_VIDEO_SAMSUNG_MEMSIZE_JPEG * SZ_1K,
                .paddr = 0,
	}
#endif

};

static struct s3c_media_device *s3c_get_media_device(int dev_id, int node)
{
	struct s3c_media_device *mdev = NULL;
	int i, found;

	if (dev_id < 0 || dev_id >= S3C_MDEV_MAX)
		return NULL;

	i = 0;
	found = 0;
	while (!found && (i < S3C_MDEV_MAX)) {
		mdev = &media_devs[i];
		if (mdev->id == dev_id && mdev->node == node)
			found = 1;
		else
			i++;
	}

	if (!found)
		mdev = NULL;

	return mdev;
}

dma_addr_t s3c_get_media_memory_node(int dev_id, int node)
{
	struct s3c_media_device *mdev;

	mdev = s3c_get_media_device(dev_id, node);
	if (!mdev){
		printk(KERN_ERR "invalid media device\n");
		return 0;
	}

	if (!mdev->paddr) {
		printk(KERN_ERR "no memory for %s\n", mdev->name);
		return 0;
	}

	return mdev->paddr;
}
EXPORT_SYMBOL(s3c_get_media_memory_node);

dma_addr_t s3c_get_media_memory(int dev_id)
{
	return s3c_get_media_memory_node(dev_id, 0);
}
EXPORT_SYMBOL(s3c_get_media_memory);

size_t s3c_get_media_memsize_node(int dev_id, int node)
{
	struct s3c_media_device *mdev;

	mdev = s3c_get_media_device(dev_id, node);
	if (!mdev){
		printk(KERN_ERR "invalid media device\n");
		return 0;
	}

	return mdev->memsize;
}
EXPORT_SYMBOL(s3c_get_media_memsize_node);

size_t s3c_get_media_memsize(int dev_id)
{
	return s3c_get_media_memsize_node(dev_id, 0);
}
EXPORT_SYMBOL(s3c_get_media_memsize);

void s3c64xx_reserve_bootmem(void)
{
	struct s3c_media_device *mdev;
	int i, nr_devs;

	nr_devs = sizeof(media_devs) / sizeof(media_devs[0]);
	for (i = 0; i < nr_devs; i++) {
		mdev = &media_devs[i];
		if (mdev->memsize <= 0)
			continue;

		mdev->paddr = virt_to_phys(alloc_bootmem_pages_node( \
				NODE_DATA(mdev->node), mdev->memsize));
		printk(KERN_INFO "s5p64xx: %lu bytes system memory reserved " \
			"for %s at 0x%08x\n", (unsigned long) mdev->memsize, \
				mdev->name, mdev->paddr);
		}
	}

/* FIXME: temporary implementation to avoid compile error */
int dma_needs_bounce(struct device *dev, dma_addr_t addr, size_t size)
{
	return 0;
}


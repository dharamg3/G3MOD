/* linux/arch/arm/plat-s5p64xx/dev-post.c
 *
 * Copyright 2009 Samsung Electronics
 *	Jinsung Yang <jsgood.yang@samsung.com>
 *	http://samsungsemi.com/
 *
 * S5P64XX series device definition for post processor
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/platform_device.h>

#include <mach/map.h>

#include <plat/post.h>
#include <plat/devs.h>
#include <plat/cpu.h>

static struct resource s3c_post_resource[] = {
	[0] = {
		.start = S5P64XX_PA_POST,
		.end   = S5P64XX_PA_POST + S5P64XX_SZ_POST - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_POST0,
		.end   = IRQ_POST0,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_post = {
	.name		  = "s3c-post",
	.id		  = -1,
	.num_resources	  = ARRAY_SIZE(s3c_post_resource),
	.resource	  = s3c_post_resource,
};

static struct s3c_platform_post default_post_data __initdata = {
	.clk_name	= "post",
	.clockrate	= 133000000,
	.line_length	= 1280,
	.nr_frames	= 4,
};

void __init s3c_post_set_platdata(struct s3c_platform_post *pd)
{
	struct s3c_platform_post *npd;

	if (!pd)
		pd = &default_post_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_post), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else if (!npd->cfg_gpio)
		npd->cfg_gpio = s3c_post_cfg_gpio;

	s3c_device_post.dev.platform_data = npd;
}


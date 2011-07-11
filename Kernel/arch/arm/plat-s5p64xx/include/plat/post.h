/* linux/arch/arm/plat-s5p64xx/include/plat/post.h
 *
 * Platform header file for Samsung Post Processor driver
 *
 * Jinsung Yang, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _POST_H
#define _POST_H

struct platform_device;

struct s3c_platform_post {
	const char	clk_name[16];
	u32		clockrate;
	int		line_length;
	int		nr_frames;

	void		(*cfg_gpio)(struct platform_device *dev);
};

extern void s3c_post_set_platdata(struct s3c_platform_post *post);

/* defined by architecture to configure gpio */
extern void s3c_post_cfg_gpio(struct platform_device *dev);

#endif /* _POST_H */


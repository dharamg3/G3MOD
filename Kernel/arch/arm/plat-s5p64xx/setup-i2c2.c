/* linux/arch/arm/plat-s5p64xx/setup-i2c0.c
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * Base S5P64XX I2C bus 0 gpio configuration
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>

struct platform_device; /* don't need the contents */

#include <mach/gpio.h>
#include <plat/iic.h>
#include <plat/gpio-bank-d1.h>
#include <plat/gpio-cfg.h>

void s3c_i2c2_cfg_gpio(struct platform_device *dev)
{
	s3c_gpio_cfgpin(S5P64XX_GPD1(4), S5P64XX_GPD_1_4_I2C2_SDA);
	s3c_gpio_cfgpin(S5P64XX_GPD1(5), S5P64XX_GPD_1_5_I2C2_SCL);
	s3c_gpio_setpull(S5P64XX_GPD1(4), S3C_GPIO_PULL_UP);
	s3c_gpio_setpull(S5P64XX_GPD1(5), S3C_GPIO_PULL_UP);
}

/* linux/arch/arm/plat-s5p64XX/include/plat/gpio-bank-c.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank C register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5P64XX_GPC0CON			(S5P64XX_GPC0_BASE + 0x00)
#define S5P64XX_GPC0DAT			(S5P64XX_GPC0_BASE + 0x04)
#define S5P64XX_GPC0PUD			(S5P64XX_GPC0_BASE + 0x08)
#define S5P64XX_GPC0DRV			(S5P64XX_GPC0_BASE + 0x0c)
#define S5P64XX_GPC0CONPDN		(S5P64XX_GPC0_BASE + 0x10)
#define S5P64XX_GPC0PUDPDN		(S5P64XX_GPC0_BASE + 0x14)

#define S5P64XX_GPC0_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5P64XX_GPC0_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5P64XX_GPC0_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5P64XX_GPC0_0_I2S_0_SCLK	(0x2 << 0)
#define S5P64XX_GPC0_0_PCM_0_SCLK	(0x3 << 0)
#define S5P64XX_GPC0_0_GPIO_INT_0	(0xf << 0)

#define S5P64XX_GPC0_1_I2S_0_CDCLK	(0x2 << 4)
#define S5P64XX_GPC0_1_PCM_0_EXTCLK	(0x3 << 4)
#define S5P64XX_GPC0_1_GPIO_INT_1	(0xf << 4)

#define S5P64XX_GPC0_2_I2S_0_LRCK	(0x2 << 8)
#define S5P64XX_GPC0_2_PCM_0_FSYNC	(0x3 << 8)
#define S5P64XX_GPC0_2_GPIO_INT_2	(0xf << 8)

#define S5P64XX_GPC0_3_I2S_0_SDI	(0x2 << 12)
#define S5P64XX_GPC0_3_PCM_0_SIN	(0x3 << 12)
#define S5P64XX_GPC0_3_GPIO_INT_3	(0xf << 12)

#define S5P64XX_GPC0_4_I2S_0_SDO	(0x2 << 16)
#define S5P64XX_GPC0_4_PCM_0_SOUT	(0x3 << 16)
#define S5P64XX_GPC0_4_GPIO_INT_4	(0xf << 16)


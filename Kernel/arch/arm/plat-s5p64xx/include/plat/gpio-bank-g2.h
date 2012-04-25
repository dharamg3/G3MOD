/* linux/arch/arm/plat-s5p64XX/include/plat/gpio-bank-g2.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank G2 register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5P64XX_GPG2CON			(S5P64XX_GPG2_BASE + 0x00)
#define S5P64XX_GPG2DAT			(S5P64XX_GPG2_BASE + 0x04)
#define S5P64XX_GPG2PUD			(S5P64XX_GPG2_BASE + 0x08)
#define S5P64XX_GPG2DRV			(S5P64XX_GPG2_BASE + 0x0c)
#define S5P64XX_GPG2CONPDN		(S5P64XX_GPG2_BASE + 0x10)
#define S5P64XX_GPG2PUDPDN		(S5P64XX_GPG2_BASE + 0x14)

#define S5P64XX_GPG2_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5P64XX_GPG2_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5P64XX_GPG2_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5P64XX_GPG2_0_SD_2_CLK		(0x2 << 0)
#define S5P64XX_GPG2_0_GPIO_INT_0	(0xf << 0)

#define S5P64XX_GPG2_1_SD_2_CMD		(0x2 << 4)
#define S5P64XX_GPG2_1_GPIO_INT_1	(0xf << 4)

#define S5P64XX_GPG2_2_SD_2_CDn		(0x2 << 8)
#define S5P64XX_GPG2_2_GPIO_INT_2	(0xf << 8)

#define S5P64XX_GPG2_3_SD_2_DATA_0	(0x2 << 12)
#define S5P64XX_GPG2_3_GPIO_INT_3	(0xf << 12)

#define S5P64XX_GPG2_4_SD_2_DATA_1	(0x2 << 16)
#define S5P64XX_GPG2_4_GPIO_INT_4	(0xf << 16)

#define S5P64XX_GPG2_5_SD_2_DATA_2	(0x2 << 20)
#define S5P64XX_GPG2_5_GPIO_INT_5	(0xf << 20)

#define S5P64XX_GPG2_6_SD_2_DATA_3	(0x2 << 24)
#define S5P64XX_GPG2_6_GPIO_INT_6	(0xf << 24)


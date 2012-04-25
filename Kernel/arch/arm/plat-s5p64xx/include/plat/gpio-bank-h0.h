/* linux/arch/arm/plat-s5p64XX/include/plat/gpio-bank-h0.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank H0 register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5P64XX_GPH0CON			(S5P64XX_GPH0_BASE + 0x00)
#define S5P64XX_GPH0DAT			(S5P64XX_GPH0_BASE + 0x04)
#define S5P64XX_GPH0PUD			(S5P64XX_GPH0_BASE + 0x08)
#define S5P64XX_GPH0DRV			(S5P64XX_GPH0_BASE + 0x0c)
//#define S5P64XX_GPH0CONPDN		(S5P64XX_GPH0_BASE + 0x10)
//#define S5P64XX_GPH0PUDPDN		(S5P64XX_GPH0_BASE + 0x14)

#define S5P64XX_GPH0_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5P64XX_GPH0_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5P64XX_GPH0_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5P64XX_GPH0_0_PS_HOLD		(0x3 << 0)
#define S5P64XX_GPH0_0_EXT_INT0_0	(0xf << 0)

#define S5P64XX_GPH0_1_EXT_INT0_1	(0xf << 4)

#define S5P64XX_GPH0_2_EXT_INT0_2	(0xf << 8)

#define S5P64XX_GPH0_3_EXT_INT0_3	(0xf << 12)

#define S5P64XX_GPH0_4_EXT_INT0_4	(0xf << 16)

#define S5P64XX_GPH0_5_EXT_INT0_5	(0xf << 20)

#define S5P64XX_GPH0_6_EXT_INT0_6	(0xf << 24)

#define S5P64XX_GPH0_7_EXT_INT0_7	(0xf << 28)


/* linux/arch/arm/plat-s5p64XX/include/plat/gpio-bank-j4.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank J4 register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5P64XX_GPJ4CON			(S5P64XX_GPJ4_BASE + 0x00)
#define S5P64XX_GPJ4DAT			(S5P64XX_GPJ4_BASE + 0x04)
#define S5P64XX_GPJ4PUD			(S5P64XX_GPJ4_BASE + 0x08)
#define S5P64XX_GPJ4DRV			(S5P64XX_GPJ4_BASE + 0x0c)
#define S5P64XX_GPJ4CONPDN		(S5P64XX_GPJ4_BASE + 0x10)
#define S5P64XX_GPJ4PUDPDN		(S5P64XX_GPJ4_BASE + 0x14)

#define S5P64XX_GPJ4_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5P64XX_GPJ4_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5P64XX_GPJ4_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5P64XX_GPJ4_0_MSM_CSn		(0x2 << 0)
#define S5P64XX_GPJ4_0_GPIO_INT_0	(0xf << 0)

#define S5P64XX_GPJ4_1_MSM_WEn		(0x2 << 4)
#define S5P64XX_GPJ4_1_GPIO_INT_1	(0xf << 4)

#define S5P64XX_GPJ4_2_MSM_Rn		(0x2 << 8)
#define S5P64XX_GPJ4_2_GPIO_INT_2	(0xf << 8)

#define S5P64XX_GPJ4_3_MSM_IRQn		(0x2 << 12)
#define S5P64XX_GPJ4_3_GPIO_INT_3	(0xf << 12)

#define S5P64XX_GPJ4_4_MSM_ADVN		(0x2 << 16)
#define S5P64XX_GPJ4_4_GPIO_INT_3	(0xf << 16)


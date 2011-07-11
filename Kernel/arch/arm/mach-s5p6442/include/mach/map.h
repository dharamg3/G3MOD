/* linux/arch/arm/mach-s5p6442/include/mach/map.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * S5P64XX - Memory map definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_MAP_H
#define __ASM_ARCH_MAP_H __FILE__

#include <plat/map-base.h>

#define S5P64XX_PA_FIMC0	(0xEE400000)
#define S5P64XX_PA_FIMC1	(0xEE500000)
#define S5P64XX_PA_FIMC2	(0xEE600000)

/* HSMMC units */
#define S5P64XX_PA_HSMMC(x)	(0xED800000 + ((x) * 0x100000))
#define S5P64XX_PA_HSMMC0	S5P64XX_PA_HSMMC(0)
#define S5P64XX_PA_HSMMC1	S5P64XX_PA_HSMMC(1)
#define S5P64XX_PA_HSMMC2	S5P64XX_PA_HSMMC(2)

#define S3C_PA_UART		(0xEC000000)
#define S3C_PA_UART0		(S3C_PA_UART + 0x00)
#define S3C_PA_UART1		(S3C_PA_UART + 0x400)
#define S3C_PA_UART2		(S3C_PA_UART + 0x800)
#define S3C_UART_OFFSET		(0x400)

/* See notes on UART VA mapping in debug-macro.S */
#define S3C_VA_UARTx(x)		(S3C_VA_UART + (S3C_PA_UART & 0xfffff) + ((x) * S3C_UART_OFFSET))

#define S3C_VA_UART0		S3C_VA_UARTx(0)
#define S3C_VA_UART1		S3C_VA_UARTx(1)
#define S3C_VA_UART2		S3C_VA_UARTx(2)
#define S3C_SZ_UART		SZ_256

#define S5P64XX_PA_SYSCON	(0xE0100000)
#define S5P64XX_VA_SYSCON       S3C_VA_SYS
#define S5P64XX_SZ_SYSCON       SZ_2M
#define S5P64XX_PA_TIMER	(0xEA000000)
#define S5P64XX_PA_IIC0		(0xEC100000)
#define S5P64XX_PA_IIC1		(0xEC200000)
#define S5P64XX_PA_IIC2		(0xEC600000)

#define S5P64XX_PA_SPI0		(0xEC300000)
#define S5P64XX_PA_SPI1		(0xEC400000)
#define S5P64XX_SZ_SPI0		SZ_4K
#define S5P64XX_SZ_SPI1		SZ_4K

#define S5P64XX_PA_GPIO		(0xE0200000)
#define S5P64XX_VA_GPIO		S3C_ADDR(0x00500000)
#define S5P64XX_SZ_GPIO		SZ_4K

#define S5P64XX_PA_SDRAM	(0x20000000)
#define S5P64XX_PA_VIC0		(0xE4000000)
#define S5P64XX_PA_VIC1		(0xE4100000)
#define S5P64XX_PA_VIC2		(0xE4200000)

#define S5P64XX_VA_SROMC	S3C_VA_SROMC
#define S5P64XX_PA_SROMC	(0xE7000000)
#define S5P64XX_SZ_SROMC	SZ_1M

#define S5P64XX_VA_LCD	   	S3C_VA_LCD
#define S5P64XX_PA_LCD	   	(0xEE000000)
#define S5P64XX_SZ_LCD		SZ_1M

#define S5P64XX_PA_ADC		(0xF300B000)

#define S5P64XX_PA_MFC		(0xF1000000)
#define S5P64XX_SZ_MFC		SZ_1M

/* KEYPAD IF */
#define S5P64XX_SZ_KEYPAD	SZ_4K
#define S5P64XX_PA_KEYPAD	(0xF3100000)

/* IIS */
#define S5P64XX_PA_IIS_V50   	(0xC0B00000)
#define S5P64XX_PA_IIS_V32   	(0xF2200000)
#define S3C_SZ_IIS		SZ_4K
#define S3C_PA_IIS_V50		S5P64XX_PA_IIS_V50
#define S3C_PA_IIS_V32		S5P64XX_PA_IIS_V32

/* AUDIO SUB SYSTEM */ 
#define S5P64XX_PA_AUDSS                (0xC0900000)

#define S5P64XX_PA_RTC	   	(0xEA300000)

#define S5P64XX_PA_ONENAND      (0xB0600000)
#define S5P64XX_VA_ONENAND      S3C_VA_ONENAND
#define S5P64XX_SZ_ONENAND      SZ_16K

#define S5P64XX_PA_DRAMC	(0XE6000000)
#define S5P64XX_VA_DRAMC	S3C_VA_DRAMC
#define S5P64XX_SZ_DRAMC	SZ_4K

/* place VICs close together */
#define S3C_VA_VIC0		(S3C_VA_IRQ + 0x00)
#define S3C_VA_VIC1		(S3C_VA_IRQ + 0x10000)
#define S3C_VA_VIC2		(S3C_VA_IRQ + 0x20000)

////////////////////////////////////////////////////
//These are not changed to support for s5p6442

/* DMA controller */
#define S5P64XX_PA_MDMA		(0xE8000000)
#define S5P64XX_PA_PDMA		(0xE9000000)

#define S5P64XX_PA_SMC9115	(0xA8000000)
#define S5P64XX_SZ_SMC9115	SZ_512M

///////////////////////////////////////////////////

/* Watchdog */
#define S5P64XX_PA_WATCHDOG 	(0xEA200000)
#define S5P64XX_SZ_WATCHDOG 	SZ_4K

/* MFC */
#define S5P64XX_PA_MFC		(0xF1000000)
#define S5P64XX_SZ_MFC		SZ_1M

/* JPEG */
#define S5P64XX_PA_JPEG         (0xEFC00000)
#define S3C_SZ_JPEG             SZ_4K

/* NAND flash controller */
#define S5P64XX_PA_NAND	   	(0xE7100000)
#define S5P64XX_SZ_NAND	   	SZ_1M

/* USB OTG */
#define S5P64XX_VA_OTG		S3C_ADDR(0x03900000)
#define S5P64XX_PA_OTG		(0xED200000)
#define S5P64XX_SZ_OTG		SZ_1M

/* USB OTG SFR */
#define S5P64XX_VA_OTGSFR	S3C_ADDR(0x03a00000)
#define S5P64XX_PA_OTGSFR	(0xED300000)
#define S5P64XX_SZ_OTGSFR	SZ_1M

/*FIMG 2D*/
#define S5P64XX_PA_G2D	   	(0xEF400000)
#define S5P64XX_SZ_G2D		SZ_4K

/*FIMG 3D*/

#define S3C64XX_PA_G3D      (0xD8000000)
#define S3C64XX_SZ_G3D    SZ_16M


#define S5P64XX_PA_SYSTIMER	(0xEA100000)
#define S5P64XX_VA_SYSTIMER   	S3C_VA_SYSTIMER
#define S5P64XX_SZ_SYSTIMER	SZ_1M

/* compatibiltiy defines. */
#define S3C_PA_TIMER		S5P64XX_PA_TIMER
#define S3C_PA_HSMMC0		S5P64XX_PA_HSMMC0
#define S3C_PA_HSMMC1		S5P64XX_PA_HSMMC1
#define S3C_PA_HSMMC2		S5P64XX_PA_HSMMC2
#define S3C_SZ_HSMMC		SZ_4K

#define S3C_PA_SPI0		S5P64XX_PA_SPI0
#define S3C_PA_SPI1		S5P64XX_PA_SPI1
#define S3C_SZ_SPI0		S5P64XX_SZ_SPI0
#define S3C_SZ_SPI1		S5P64XX_SZ_SPI1

#define S3C_PA_IIC		S5P64XX_PA_IIC0
#define S3C_PA_IIC1		S5P64XX_PA_IIC1
#define S3C_PA_IIC2		S5P64XX_PA_IIC2

#define S3C_PA_RTC		S5P64XX_PA_RTC



#define S3C_SZ_KEYPAD           S5P64XX_SZ_KEYPAD
#define S3C_PA_KEYPAD           S5P64XX_PA_KEYPAD


#define S3C_PA_IIS		S5P64XX_PA_IIS
#define S3C_PA_ADC		S5P64XX_PA_ADC

#define S3C_PA_MDMA		S5P64XX_PA_MDMA
#define S3C_PA_PDMA		S5P64XX_PA_PDMA

#define S3C_VA_OTG		S5P64XX_VA_OTG
#define S3C_PA_OTG		S5P64XX_PA_OTG
#define S3C_SZ_OTG		S5P64XX_SZ_OTG

#define S3C_VA_OTGSFR		S5P64XX_VA_OTGSFR
#define S3C_PA_OTGSFR		S5P64XX_PA_OTGSFR
#define S3C_SZ_OTGSFR		S5P64XX_SZ_OTGSFR

#define S3C_VA_RTC		S3C_ADDR(0x03b00000)
#endif /* __ASM_ARCH_6442_MAP_H */

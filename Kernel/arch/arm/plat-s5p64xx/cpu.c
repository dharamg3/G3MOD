/* linux/arch/arm/plat-s5p64xx/cpu.c
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * S5P64XX CPU Support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/io.h>

#include <mach/hardware.h>
#include <mach/map.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>

#include <plat/regs-serial.h>
#include <plat/regs-clock.h>

#include <plat/cpu.h>
#include <plat/devs.h>
#include <plat/clock.h>

#include <plat/s5p6442.h>

/* table of supported CPUs */

static const char name_s5p6442[] = "S5P6442";

static struct cpu_table cpu_ids[] __initdata = {
	{
		.idcode		= 0xABABAB00,
		.idmask		= 0xffffff00,
		.map_io		= s5p6442_map_io,
		.init_clocks	= s5p6442_init_clocks,
		.init_uarts	= s5p6442_init_uarts,
		.init		= s5p6442_init,
		.name		= name_s5p6442,
	},
};

/* minimal IO mapping */

/* see notes on uart map in arch/arm/mach-s5p6442/include/mach/debug-macro.S */
#define UART_OFFS (S3C_PA_UART & 0xfffff)

static struct map_desc s3c_iodesc[] __initdata = {
	{
		.virtual	= (unsigned long)S3C_VA_SYS,
		.pfn		= __phys_to_pfn(S5P64XX_PA_SYSCON),
		.length		= (SZ_16K + SZ_16K),
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)(S3C_VA_UART + UART_OFFS),
		.pfn		= __phys_to_pfn(S3C_PA_UART),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S3C_VA_VIC0,
		.pfn		= __phys_to_pfn(S5P64XX_PA_VIC0),
		.length		= SZ_16K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S3C_VA_VIC1,
		.pfn		= __phys_to_pfn(S5P64XX_PA_VIC1),
		.length		= SZ_16K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S3C_VA_VIC2,
		.pfn		= __phys_to_pfn(S5P64XX_PA_VIC2),
		.length		= SZ_16K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S3C_VA_TIMER,
		.pfn		= __phys_to_pfn(S3C_PA_TIMER),
		.length		= SZ_16K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S3C_VA_SYSTIMER,
		.pfn		= __phys_to_pfn(S5P64XX_PA_SYSTIMER),
		.length		= SZ_16K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S5P64XX_VA_GPIO,
		.pfn		= __phys_to_pfn(S5P64XX_PA_GPIO),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	}, {
		.virtual	= (unsigned long)S3C_VA_AUDSS,
		.pfn		= __phys_to_pfn(S5P64XX_PA_AUDSS),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	},
#if defined(CONFIG_HRT_RTC)
	{
		.virtual	= (unsigned long)S3C_VA_RTC,
		.pfn		= __phys_to_pfn(S3C_PA_RTC),
		.length		= SZ_4K,
		.type		= MT_DEVICE,
	},
#endif
};

/* read cpu identification code */

void __init s5p64xx_init_io(struct map_desc *mach_desc, int size)
{
	unsigned long idcode;

	/* initialise the io descriptors we need for initialisation */
	iotable_init(s3c_iodesc, ARRAY_SIZE(s3c_iodesc));
	iotable_init(mach_desc, size);

	idcode = 0xABABAB00;
	s3c_init_cpu(idcode, cpu_ids, ARRAY_SIZE(cpu_ids));
}

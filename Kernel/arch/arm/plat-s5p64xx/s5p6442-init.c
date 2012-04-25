/* linux/arch/arm/plat-s5p64xx/s5p6442-init.c
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *      Ben Dooks <ben@simtec.co.uk>
 *      http://armlinux.simtec.co.uk/
 *
 * S5P6442 - CPU initialisation (common with other S5P64XX chips)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>

#include <plat/cpu.h>
#include <plat/devs.h>
#include <plat/s5p6442.h>

/* uart registration process */

void __init s5p6442_common_init_uarts(struct s3c2410_uartcfg *cfg, int no)
{
	s3c24xx_init_uartdevs("s5pc100-uart", s5p64xx_uart_resources, cfg, no);
}

/* linux/arch/arm/mach-s5p6442/cpu.c
 *
 * Copyright 2008 Simtec Electronics
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 * Copyright 2010 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/sysdev.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include <asm/proc-fns.h>
#include <mach/idle.h>

#include <mach/hardware.h>
#include <asm/irq.h>

#include <plat/cpu-freq.h>
#include <plat/regs-serial.h>
#include <plat/regs-clock.h>

#include <plat/cpu.h>
#include <plat/devs.h>
#include <plat/clock.h>
#include <plat/sdhci.h>
#include <plat/iic-core.h>
#include <plat/s5p6442.h>
#include <mach/map.h>

/* Initial IO mappings */

static struct map_desc s5p6442_iodesc[] __initdata = {
	IODESC_ENT(LCD),
	IODESC_ENT(SROMC),
	IODESC_ENT(OTG),
	IODESC_ENT(OTGSFR),
	IODESC_ENT(SYSCON),
//	IODESC_ENT(ONENAND),
	IODESC_ENT(DRAMC),
};

static void s5p6442_idle(void)
{
#if 1	
	unsigned long tmp;

	/* Ensure our idle mode is to go to idle */
	/* Set WFI instruction to SLEEP mode */
	

	tmp = __raw_readl(S5P_PWR_CFG);
	tmp &= S5P_CFG_WFI_CLEAN;
	tmp |= S5P_CFG_WFI_IDLE;
	__raw_writel(tmp, S5P_PWR_CFG);
#endif	
	cpu_do_idle();

}

/* s5p6442_map_io
 *
 * register the standard cpu IO areas
*/

void __init s5p6442_map_io(void)
{
	iotable_init(s5p6442_iodesc, ARRAY_SIZE(s5p6442_iodesc));

	/* initialise device information early */
	s5p6442_default_sdhci0();
	s5p6442_default_sdhci1();
	s5p6442_default_sdhci2();

	/* the i2c devices are directly compatible with s3c2440 */
	s3c_i2c0_setname("s3c2440-i2c");
	s3c_i2c1_setname("s3c2440-i2c");
	s3c_i2c2_setname("s3c2440-i2c");

	/* set our idle function */
	s5p64xx_idle = s5p6442_idle;
}

void __init s5p6442_init_clocks(int xtal)
{
	printk(KERN_DEBUG "%s: initialising clocks\n", __func__);
	s3c24xx_register_baseclocks(xtal);
	s5p64xx_register_clocks();
	s5p6442_register_clocks();
	s5p6442_setup_clocks();
#ifdef CONFIG_HAVE_PWM
	s3c_pwmclk_init();
#endif
}

void __init s5p6442_init_irq(void)
{
	/* VIC0 : 20,31 reserved */
	/* VIC1 : 1~6,8,9,13,14,16~18,22,23,31 reserved */
	/* VIC2 : 9,12,13,18,22,26,29~31 reserved */
	s5p64xx_init_irq(~0 &~(IRQ_VIC0_UNUSED), ~0 &~(IRQ_VIC1_UNUSED), ~0 &~(IRQ_VIC2_UNUSED), 0);
}

struct sysdev_class s5p6442_sysclass = {
	.name	= "s5p6442-core",
};

static struct sys_device s5p6442_sysdev = {
	.cls	= &s5p6442_sysclass,
};

static int __init s5p6442_core_init(void)
{
	return sysdev_class_register(&s5p6442_sysclass);
}

core_initcall(s5p6442_core_init);

int __init s5p6442_init(void)
{
	printk("S5P6442: Initialising architecture\n");

	return sysdev_register(&s5p6442_sysdev);
}

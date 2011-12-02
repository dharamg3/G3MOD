/* linux/arch/arm/mach-s5p6442/include/mach/system.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *      Ben Dooks <ben@simtec.co.uk>
 *      http://armlinux.simtec.co.uk/
 *
 * S3C6400 - system implementation
 */

#ifndef __ASM_ARCH_SYSTEM_H
#define __ASM_ARCH_SYSTEM_H __FILE__

#include <linux/io.h>
#include <mach/map.h>
#include <plat/regs-watchdog.h>

void (*s5p64xx_idle)(void);
void (*s5p64xx_reset_hook)(void);

void s5p64xx_default_idle(void)
{
	/* nothing here yet */
}
	
static void arch_idle(void)
{
	if (s5p64xx_idle != NULL)
		(s5p64xx_idle)();
	else
		s5p64xx_default_idle();
}

void arch_reset(char mode, const char *cmd)
{
	/*  Watch Reset is used */
	void __iomem * wdt ;
	wdt = ioremap(S5P64XX_PA_WATCHDOG, 0x400);
        
        if (wdt == NULL) 
        {
                printk("failed to ioremap() region in arch_reset()\n");
                return ;
        }
        
        __raw_writel(0x1,wdt +0x4 );
        __raw_writel(0x8021,wdt);
	
        /* Never happened */
        mdelay(5000);
}

#endif /* __ASM_ARCH_IRQ_H */

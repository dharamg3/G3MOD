/* linux/arch/arm/plat-s5p64xx/pm.c
 *
 * Copyright (c) 2004,2006 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * S5P64XX Power Manager (Suspend-To-RAM) support
 *
 * See Documentation/arm/Samsung-S3C24XX/Suspend.txt for more information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Parts based on arch/arm/mach-pxa/pm.c
 *
 * Thanks to Dimitry Andric for debugging
*/

#include <linux/init.h>
#include <linux/suspend.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/crc32.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/serial_core.h>
#include <linux/io.h>
#include <linux/platform_device.h>

#include <asm/cacheflush.h>
#include <mach/hardware.h>

#include <plat/map-base.h>
#include <plat/regs-serial.h>
#include <plat/regs-clock.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-mem.h>
#include <mach/regs-irq.h>
#include <asm/gpio.h>
#include <plat/regs-audss.h>
#include <asm/mach/time.h>

#include <plat/pm.h>
#include <plat/s5p6442-dvfs.h>
#include <plat/power_clk_gating.h>
#include <plat/regs-onenand.h>

unsigned long s3c_pm_flags;

#define DEBUG_WAKEUP_STATUS	1

#if DEBUG_WAKEUP_STATUS
#define DEBUG_WAKEUP(fmt,args...) printk(fmt, ##args)
#else
#define DEBUG_WAKEUP(fmt,args...) do {} while(0)
#endif

extern unsigned int PM_STATE_PHY;

extern int call_state;

#define PFX "s5p6442-pm: "
void s5p_config_sleep_gpio(void);

static struct sleep_save core_save[] = {
	SAVE_ITEM(S5P_APLL_CON),
	SAVE_ITEM(S5P_MPLL_CON),
	SAVE_ITEM(S5P_EPLL_CON),
	SAVE_ITEM(S5P_VPLL_CON),

	SAVE_ITEM(S5P_CLK_SRC0),
	SAVE_ITEM(S5P_CLK_SRC1),
	SAVE_ITEM(S5P_CLK_SRC2),
	SAVE_ITEM(S5P_CLK_SRC3),
	SAVE_ITEM(S5P_CLK_SRC4),
	SAVE_ITEM(S5P_CLK_SRC5),
	SAVE_ITEM(S5P_CLK_SRC6),

	SAVE_ITEM(S5P_CLK_SRC_MASK0),
	SAVE_ITEM(S5P_CLK_SRC_MASK1),

	SAVE_ITEM(S5P_CLK_DIV0),
	SAVE_ITEM(S5P_CLK_DIV1),
	SAVE_ITEM(S5P_CLK_DIV2),
	SAVE_ITEM(S5P_CLK_DIV3),
	SAVE_ITEM(S5P_CLK_DIV4),
	SAVE_ITEM(S5P_CLK_DIV5),
	SAVE_ITEM(S5P_CLK_DIV6),

//	SAVE_ITEM(S5P_CLKGATE_MAIN0),
//	SAVE_ITEM(S5P_CLKGATE_MAIN1),
//	SAVE_ITEM(S5P_CLKGATE_MAIN2),

//	SAVE_ITEM(S5P_CLKGATE_PERI0),
//	SAVE_ITEM(S5P_CLKGATE_PERI1),

//	SAVE_ITEM(S5P_CLKGATE_SCLK0),
//	SAVE_ITEM(S5P_CLKGATE_SCLK1),

	SAVE_ITEM(S5P_CLKGATE_IP0),
	SAVE_ITEM(S5P_CLKGATE_IP1),
	SAVE_ITEM(S5P_CLKGATE_IP2),
	SAVE_ITEM(S5P_CLKGATE_IP3),
	SAVE_ITEM(S5P_CLKGATE_IP4),	

	SAVE_ITEM(S5P_CLKGATE_BLOCK),
//	SAVE_ITEM(S5P_CLKGATE_BUS0),
//	SAVE_ITEM(S5P_CLKGATE_BUS1),

	/* power gating */
	SAVE_ITEM(S5P_NORMAL_CFG),
	SAVE_ITEM(S5P_CLKGATE_BLOCK),

	/* audio */
	SAVE_ITEM(S5P_CLKSRC_AUDSS),
	SAVE_ITEM(S5P_CLKDIV_AUDSS),
	SAVE_ITEM(S5P_CLKGATE_AUDSS),
	SAVE_ITEM(S5P_CLK_OUT),

	/* external Interrupts */
	SAVE_ITEM(S5P64XX_EINT0MASK),
	SAVE_ITEM(S5P64XX_EINT1MASK),
	SAVE_ITEM(S5P64XX_EINT2MASK),
	SAVE_ITEM(S5P64XX_EINT3MASK),
};

static struct sleep_save gpio_save[] = {
	SAVE_ITEM(S5P64XX_GPA0CON),
	SAVE_ITEM(S5P64XX_GPA0DAT),
	SAVE_ITEM(S5P64XX_GPA0PUD),
	SAVE_ITEM(S5P64XX_GPA0DRV),
	SAVE_ITEM(S5P64XX_GPA0CONPDN),
	SAVE_ITEM(S5P64XX_GPA0PUDPDN),	

	SAVE_ITEM(S5P64XX_GPA1CON),
	SAVE_ITEM(S5P64XX_GPA1DAT),
	SAVE_ITEM(S5P64XX_GPA1PUD),
	SAVE_ITEM(S5P64XX_GPA1DRV),
	SAVE_ITEM(S5P64XX_GPA1CONPDN),
	SAVE_ITEM(S5P64XX_GPA1PUDPDN),

	SAVE_ITEM(S5P64XX_GPBCON),
	SAVE_ITEM(S5P64XX_GPBDAT),
	SAVE_ITEM(S5P64XX_GPBPUD),
	SAVE_ITEM(S5P64XX_GPBDRV),
	SAVE_ITEM(S5P64XX_GPBCONPDN),
	SAVE_ITEM(S5P64XX_GPBPUDPDN),

	SAVE_ITEM(S5P64XX_GPC0CON),
	SAVE_ITEM(S5P64XX_GPC0DAT),
	SAVE_ITEM(S5P64XX_GPC0PUD),
	SAVE_ITEM(S5P64XX_GPC0DRV),
	SAVE_ITEM(S5P64XX_GPC0CONPDN),
	SAVE_ITEM(S5P64XX_GPC0PUDPDN),	

	SAVE_ITEM(S5P64XX_GPC1CON),
	SAVE_ITEM(S5P64XX_GPC1DAT),
	SAVE_ITEM(S5P64XX_GPC1PUD),
	SAVE_ITEM(S5P64XX_GPC1DRV),
	SAVE_ITEM(S5P64XX_GPC1CONPDN),
	SAVE_ITEM(S5P64XX_GPC1PUDPDN),

	SAVE_ITEM(S5P64XX_GPD0CON),
	SAVE_ITEM(S5P64XX_GPD0DAT),
	SAVE_ITEM(S5P64XX_GPD0PUD),
	SAVE_ITEM(S5P64XX_GPD0DRV),
	SAVE_ITEM(S5P64XX_GPD0CONPDN),
	SAVE_ITEM(S5P64XX_GPD0PUDPDN),	

	SAVE_ITEM(S5P64XX_GPD1CON),
	SAVE_ITEM(S5P64XX_GPD1DAT),
	SAVE_ITEM(S5P64XX_GPD1PUD),
	SAVE_ITEM(S5P64XX_GPD1DRV),
	SAVE_ITEM(S5P64XX_GPD1CONPDN),
	SAVE_ITEM(S5P64XX_GPD1PUDPDN),

	SAVE_ITEM(S5P64XX_GPE0CON),
	SAVE_ITEM(S5P64XX_GPE0DAT),
	SAVE_ITEM(S5P64XX_GPE0PUD),
	SAVE_ITEM(S5P64XX_GPE0DRV),
	SAVE_ITEM(S5P64XX_GPE0CONPDN),
	SAVE_ITEM(S5P64XX_GPE0PUDPDN),	

	SAVE_ITEM(S5P64XX_GPE1CON),
	SAVE_ITEM(S5P64XX_GPE1DAT),
	SAVE_ITEM(S5P64XX_GPE1PUD),
	SAVE_ITEM(S5P64XX_GPE1DRV),
	SAVE_ITEM(S5P64XX_GPE1CONPDN),
	SAVE_ITEM(S5P64XX_GPE1PUDPDN),

	SAVE_ITEM(S5P64XX_GPF0CON),
	SAVE_ITEM(S5P64XX_GPF0DAT),
	SAVE_ITEM(S5P64XX_GPF0PUD),
	SAVE_ITEM(S5P64XX_GPF0DRV),
	SAVE_ITEM(S5P64XX_GPF0CONPDN),
	SAVE_ITEM(S5P64XX_GPF0PUDPDN),	

	SAVE_ITEM(S5P64XX_GPF1CON),
	SAVE_ITEM(S5P64XX_GPF1DAT),
	SAVE_ITEM(S5P64XX_GPF1PUD),
	SAVE_ITEM(S5P64XX_GPF1DRV),
	SAVE_ITEM(S5P64XX_GPF1CONPDN),
	SAVE_ITEM(S5P64XX_GPF1PUDPDN),

	SAVE_ITEM(S5P64XX_GPF2CON),
	SAVE_ITEM(S5P64XX_GPF2DAT),
	SAVE_ITEM(S5P64XX_GPF2PUD),
	SAVE_ITEM(S5P64XX_GPF2DRV),
	SAVE_ITEM(S5P64XX_GPF2CONPDN),
	SAVE_ITEM(S5P64XX_GPF2PUDPDN),

	SAVE_ITEM(S5P64XX_GPF3CON),
	SAVE_ITEM(S5P64XX_GPF3DAT),
	SAVE_ITEM(S5P64XX_GPF3PUD),
	SAVE_ITEM(S5P64XX_GPF3DRV),
	SAVE_ITEM(S5P64XX_GPF3CONPDN),
	SAVE_ITEM(S5P64XX_GPF3PUDPDN),

	SAVE_ITEM(S5P64XX_GPG0CON),
	SAVE_ITEM(S5P64XX_GPG0DAT),
	SAVE_ITEM(S5P64XX_GPG0PUD),
	SAVE_ITEM(S5P64XX_GPG0DRV),
	SAVE_ITEM(S5P64XX_GPG0CONPDN),
	SAVE_ITEM(S5P64XX_GPG0PUDPDN),	

	SAVE_ITEM(S5P64XX_GPG1CON),
	SAVE_ITEM(S5P64XX_GPG1DAT),
	SAVE_ITEM(S5P64XX_GPG1PUD),
	SAVE_ITEM(S5P64XX_GPG1DRV),
	SAVE_ITEM(S5P64XX_GPG1CONPDN),
	SAVE_ITEM(S5P64XX_GPG1PUDPDN),

	SAVE_ITEM(S5P64XX_GPG2CON),
	SAVE_ITEM(S5P64XX_GPG2DAT),
	SAVE_ITEM(S5P64XX_GPG2PUD),
	SAVE_ITEM(S5P64XX_GPG2DRV),
	SAVE_ITEM(S5P64XX_GPG2CONPDN),
	SAVE_ITEM(S5P64XX_GPG2PUDPDN),

	SAVE_ITEM(S5P64XX_GPJ0CON),
	SAVE_ITEM(S5P64XX_GPJ0DAT),
	SAVE_ITEM(S5P64XX_GPJ0PUD),
	SAVE_ITEM(S5P64XX_GPJ0DRV),
	SAVE_ITEM(S5P64XX_GPJ0CONPDN),
	SAVE_ITEM(S5P64XX_GPJ0PUDPDN),	

	SAVE_ITEM(S5P64XX_GPJ1CON),
	SAVE_ITEM(S5P64XX_GPJ1DAT),
	SAVE_ITEM(S5P64XX_GPJ1PUD),
	SAVE_ITEM(S5P64XX_GPJ1DRV),
	SAVE_ITEM(S5P64XX_GPJ1CONPDN),
	SAVE_ITEM(S5P64XX_GPJ1PUDPDN),

	SAVE_ITEM(S5P64XX_GPJ2CON),
	SAVE_ITEM(S5P64XX_GPJ2DAT),
	SAVE_ITEM(S5P64XX_GPJ2PUD),
	SAVE_ITEM(S5P64XX_GPJ2DRV),
	SAVE_ITEM(S5P64XX_GPJ2CONPDN),
	SAVE_ITEM(S5P64XX_GPJ2PUDPDN),

	SAVE_ITEM(S5P64XX_GPJ3CON),
	SAVE_ITEM(S5P64XX_GPJ3DAT),
	SAVE_ITEM(S5P64XX_GPJ3PUD),
	SAVE_ITEM(S5P64XX_GPJ3DRV),
	SAVE_ITEM(S5P64XX_GPJ3CONPDN),
	SAVE_ITEM(S5P64XX_GPJ3PUDPDN),

	SAVE_ITEM(S5P64XX_GPJ4CON),
	SAVE_ITEM(S5P64XX_GPJ4DAT),
	SAVE_ITEM(S5P64XX_GPJ4PUD),
	SAVE_ITEM(S5P64XX_GPJ4DRV),
	SAVE_ITEM(S5P64XX_GPJ4CONPDN),
	SAVE_ITEM(S5P64XX_GPJ4PUDPDN),
	
	SAVE_ITEM(S5P64XX_MP01CON),		
	SAVE_ITEM(S5P64XX_MP01DAT),		
	SAVE_ITEM(S5P64XX_MP01PUD),		
	SAVE_ITEM(S5P64XX_MP01DRV),		
	SAVE_ITEM(S5P64XX_MP01CONPDN),
	SAVE_ITEM(S5P64XX_MP01PUDPDN),
	SAVE_ITEM(S5P64XX_MP02CON),		
	SAVE_ITEM(S5P64XX_MP02DAT),		
	SAVE_ITEM(S5P64XX_MP02PUD),		
	SAVE_ITEM(S5P64XX_MP02DRV),		
	SAVE_ITEM(S5P64XX_MP02CONPDN),
	SAVE_ITEM(S5P64XX_MP02PUDPDN),
	SAVE_ITEM(S5P64XX_MP03CON),		
	SAVE_ITEM(S5P64XX_MP03DAT),		
	SAVE_ITEM(S5P64XX_MP03PUD),		
	SAVE_ITEM(S5P64XX_MP03DRV),		
	SAVE_ITEM(S5P64XX_MP03CONPDN),
	SAVE_ITEM(S5P64XX_MP03PUDPDN),
	SAVE_ITEM(S5P64XX_MP04CON),		
	SAVE_ITEM(S5P64XX_MP04DAT),		
	SAVE_ITEM(S5P64XX_MP04PUD),		
	SAVE_ITEM(S5P64XX_MP04DRV),		
	SAVE_ITEM(S5P64XX_MP04CONPDN),
	SAVE_ITEM(S5P64XX_MP04PUDPDN),
	SAVE_ITEM(S5P64XX_MP05CON),		
	SAVE_ITEM(S5P64XX_MP05DAT),		
	SAVE_ITEM(S5P64XX_MP05PUD),		
	SAVE_ITEM(S5P64XX_MP05DRV),		
	SAVE_ITEM(S5P64XX_MP05CONPDN),
	SAVE_ITEM(S5P64XX_MP05PUDPDN),
	SAVE_ITEM(S5P64XX_MP06CON),		
	SAVE_ITEM(S5P64XX_MP06DAT),		
	SAVE_ITEM(S5P64XX_MP06PUD),		
	SAVE_ITEM(S5P64XX_MP06DRV),		
	SAVE_ITEM(S5P64XX_MP06CONPDN),
	SAVE_ITEM(S5P64XX_MP06PUDPDN),
	SAVE_ITEM(S5P64XX_MP07CON),		
	SAVE_ITEM(S5P64XX_MP07DAT),		
	SAVE_ITEM(S5P64XX_MP07PUD),		
	SAVE_ITEM(S5P64XX_MP07DRV),		
	SAVE_ITEM(S5P64XX_MP07CONPDN),
	SAVE_ITEM(S5P64XX_MP07PUDPDN),
	//SAVE_ITEM(S5P64XX_MP10CON),		
	//SAVE_ITEM(S5P64XX_MP10DAT),		
	SAVE_ITEM(S5P64XX_MP10PUD),		
	SAVE_ITEM(S5P64XX_MP10DRV),		
	//SAVE_ITEM(S5P64XX_MP10CONPDN),
	SAVE_ITEM(S5P64XX_MP10PUDPDN),
	//SAVE_ITEM(S5P64XX_MP11CON),		
	//SAVE_ITEM(S5P64XX_MP11DAT),		
	SAVE_ITEM(S5P64XX_MP11PUD),		
	SAVE_ITEM(S5P64XX_MP11DRV),		
	//SAVE_ITEM(S5P64XX_MP11CONPDN),
	SAVE_ITEM(S5P64XX_MP11PUDPDN),
	//SAVE_ITEM(S5P64XX_MP12CON),	
	//SAVE_ITEM(S5P64XX_MP12DAT),	
	//SAVE_ITEM(S5P64XX_MP12PUD),	
	SAVE_ITEM(S5P64XX_MP12DRV),	
	//SAVE_ITEM(S5P64XX_MP12CONPDN),
	//SAVE_ITEM(S5P64XX_MP12PUDPDN),
	//SAVE_ITEM(S5P64XX_MP13CON),
	//SAVE_ITEM(S5P64XX_MP13DAT),
	//SAVE_ITEM(S5P64XX_MP13PUD),
	SAVE_ITEM(S5P64XX_MP13DRV),
	//SAVE_ITEM(S5P64XX_MP13CONPDN),
	//SAVE_ITEM(S5P64XX_MP13PUDPDN),
	//SAVE_ITEM(S5P64XX_MP14CON),
	//SAVE_ITEM(S5P64XX_MP14DAT),
	//SAVE_ITEM(S5P64XX_MP14PUD),
	SAVE_ITEM(S5P64XX_MP14DRV),
	//SAVE_ITEM(S5P64XX_MP14CONPDN),
	//SAVE_ITEM(S5P64XX_MP14PUDPDN),
	//SAVE_ITEM(S5P64XX_MP15CON),
	//SAVE_ITEM(S5P64XX_MP15DAT),
	//SAVE_ITEM(S5P64XX_MP15PUD),
	SAVE_ITEM(S5P64XX_MP15DRV),
	//SAVE_ITEM(S5P64XX_MP15CONPDN),
	//SAVE_ITEM(S5P64XX_MP15PUDPDN),
	//SAVE_ITEM(S5P64XX_MP16CON),
	//SAVE_ITEM(S5P64XX_MP16DAT),
	//SAVE_ITEM(S5P64XX_MP16PUD),
	SAVE_ITEM(S5P64XX_MP16DRV),
	//SAVE_ITEM(S5P64XX_MP16CONPDN),
	//SAVE_ITEM(S5P64XX_MP16PUDPDN),
	//SAVE_ITEM(S5P64XX_MP17CON),
	//SAVE_ITEM(S5P64XX_MP17DAT),
	SAVE_ITEM(S5P64XX_MP17PUD),
	SAVE_ITEM(S5P64XX_MP17DRV),
	//SAVE_ITEM(S5P64XX_MP17CONPDN),
	SAVE_ITEM(S5P64XX_MP17PUDPDN),
	//SAVE_ITEM(S5P64XX_MP18CON),
	//SAVE_ITEM(S5P64XX_MP18DAT),
	SAVE_ITEM(S5P64XX_MP18PUD),
	SAVE_ITEM(S5P64XX_MP18DRV),
	//SAVE_ITEM(S5P64XX_MP18CONPDN),
	SAVE_ITEM(S5P64XX_MP18PUDPDN),

/* ALIVE ports */
	SAVE_ITEM(S5P64XX_GPH0CON),
	SAVE_ITEM(S5P64XX_GPH0DAT),
	SAVE_ITEM(S5P64XX_GPH0PUD),
	SAVE_ITEM(S5P64XX_GPH0DRV),
	
	SAVE_ITEM(S5P64XX_GPH1CON),
	SAVE_ITEM(S5P64XX_GPH1DAT),
	SAVE_ITEM(S5P64XX_GPH1PUD),
	SAVE_ITEM(S5P64XX_GPH1DRV),

	SAVE_ITEM(S5P64XX_GPH2CON),
	SAVE_ITEM(S5P64XX_GPH2DAT),
	SAVE_ITEM(S5P64XX_GPH2PUD),
	SAVE_ITEM(S5P64XX_GPH2DRV),

	SAVE_ITEM(S5P64XX_GPH3CON),
	SAVE_ITEM(S5P64XX_GPH3DAT),
	SAVE_ITEM(S5P64XX_GPH3PUD),
	SAVE_ITEM(S5P64XX_GPH3DRV),

/* GPIO Interrupt related stuff */
	SAVE_ITEM(S5P64XX_GROUP13_INT_CON),//touch
	SAVE_ITEM(S5P64XX_GROUP18_INT_CON),//fm radio
	
	SAVE_ITEM(S5P64XX_GROUP13_INT_MASK),	
	SAVE_ITEM(S5P64XX_GROUP18_INT_MASK),
	/* Special register */
	//SAVE_ITEM(S5P64XX_SPC_BASE),
};

/* this lot should be really saved by the IRQ code */
/* VICXADDRESSXX initilaization to be needed */
static struct sleep_save irq_save[] = {
	SAVE_ITEM(S5P64XX_VIC0INTSELECT),
	SAVE_ITEM(S5P64XX_VIC1INTSELECT),
	SAVE_ITEM(S5P64XX_VIC2INTSELECT),
	SAVE_ITEM(S5P64XX_VIC0INTENABLE),
	SAVE_ITEM(S5P64XX_VIC1INTENABLE),
	SAVE_ITEM(S5P64XX_VIC2INTENABLE),
	SAVE_ITEM(S5P64XX_VIC0SOFTINT),
	SAVE_ITEM(S5P64XX_VIC1SOFTINT),
	SAVE_ITEM(S5P64XX_VIC2SOFTINT),
};

static struct sleep_save sromc_save[] = {
	SAVE_ITEM(S5P64XX_SROM_BW),
	SAVE_ITEM(S5P64XX_SROM_BC0),
	SAVE_ITEM(S5P64XX_SROM_BC1),
	SAVE_ITEM(S5P64XX_SROM_BC2),
	SAVE_ITEM(S5P64XX_SROM_BC3),
	SAVE_ITEM(S5P64XX_SROM_BC4),
	SAVE_ITEM(S5P64XX_SROM_BC5),
};

static struct sleep_save onenand_save[] = {
	SAVE_ITEM(S5P_ONENAND_IF_CTRL),
	SAVE_ITEM(S5P_ONENAND_IF_CMD),
	SAVE_ITEM(S5P_ONENAND_IF_ASYNC_TIMING_CTRL),
	SAVE_ITEM(S5P_ONENAND_IF_STATUS),
	SAVE_ITEM(S5P_DMA_SRC_ADDR),
	SAVE_ITEM(S5P_DMA_SRC_CFG),
	SAVE_ITEM(S5P_DMA_DST_ADDR),
	SAVE_ITEM(S5P_DMA_DST_CFG),
	SAVE_ITEM(S5P_DMA_TRANS_SIZE),
	SAVE_ITEM(S5P_DMA_TRANS_CMD),
	SAVE_ITEM(S5P_DMA_TRANS_STATUS),
	SAVE_ITEM(S5P_DMA_TRANS_DIR),
	SAVE_ITEM(S5P_SQC_SAO),
	SAVE_ITEM(S5P_SQC_CMD),
	SAVE_ITEM(S5P_SQC_STATUS),
	SAVE_ITEM(S5P_SQC_CAO),
	SAVE_ITEM(S5P_SQC_REG_CTRL),
	SAVE_ITEM(S5P_SQC_REG_VAL),
	SAVE_ITEM(S5P_SQC_BRPAO0),
	SAVE_ITEM(S5P_SQC_BRPAO1),
	SAVE_ITEM(S5P_SQC_CLR),
	SAVE_ITEM(S5P_DMA_CLR),
	SAVE_ITEM(S5P_ONENAND_CLR),
	SAVE_ITEM(S5P_SQC_MASK),
	SAVE_ITEM(S5P_DMA_MASK),
	SAVE_ITEM(S5P_ONENAND_MASK),
	SAVE_ITEM(S5P_SQC_PEND),
	SAVE_ITEM(S5P_DMA_PEND),
	SAVE_ITEM(S5P_ONENAND_PEND),
	SAVE_ITEM(S5P_SQC_STATUS),
	SAVE_ITEM(S5P_DMA_STATUS),
	SAVE_ITEM(S5P_ONENAND_STATUS),	
};

#define SAVE_UART(va) \
	SAVE_ITEM((va) + S3C2410_ULCON), \
	SAVE_ITEM((va) + S3C2410_UCON), \
	SAVE_ITEM((va) + S3C2410_UFCON), \
	SAVE_ITEM((va) + S3C2410_UMCON), \
	SAVE_ITEM((va) + S3C2410_UBRDIV), \
	SAVE_ITEM((va) + S3C2410_UDIVSLOT), \
	SAVE_ITEM((va) + S3C2410_UINTMSK)

static struct sleep_save uart_save[] = {
	SAVE_UART(S3C24XX_VA_UART0),
	SAVE_UART(S3C24XX_VA_UART1),
	SAVE_UART(S3C24XX_VA_UART2),
};
#ifdef CONFIG_S3C2410_PM_DEBUG
/* debug
 *
 * we send the debug to printascii() to allow it to be seen if the
 * system never wakes up from the sleep
*/

extern void printascii(const char *);

void pm_dbg(const char *fmt, ...)
{
	va_list va;
	char buff[256];

	va_start(va, fmt);
	vsprintf(buff, fmt, va);
	va_end(va);

	printascii(buff);
}

static void s3c2410_pm_debug_init(void)
{
	unsigned long tmp = __raw_readl(S3C2410_CLKCON);

	/* re-start uart clocks */
	tmp |= S3C2410_CLKCON_UART0;
	tmp |= S3C2410_CLKCON_UART1;
	tmp |= S3C2410_CLKCON_UART2;

	__raw_writel(tmp, S3C2410_CLKCON);
	udelay(10);
}

#define DBG(fmt...) pm_dbg(fmt)
#else
#define DBG(fmt...)

#define s5p6442_pm_debug_init() do { } while(0)
#endif

#if defined(CONFIG_S3C2410_PM_CHECK) && CONFIG_S3C2410_PM_CHECK_CHUNKSIZE != 0

/* suspend checking code...
 *
 * this next area does a set of crc checks over all the installed
 * memory, so the system can verify if the resume was ok.
 *
 * CONFIG_S3C6410_PM_CHECK_CHUNKSIZE defines the block-size for the CRC,
 * increasing it will mean that the area corrupted will be less easy to spot,
 * and reducing the size will cause the CRC save area to grow
*/

#define CHECK_CHUNKSIZE (CONFIG_S3C2410_PM_CHECK_CHUNKSIZE * 1024)

static u32 crc_size;	/* size needed for the crc block */
static u32 *crcs;	/* allocated over suspend/resume */

typedef u32 *(run_fn_t)(struct resource *ptr, u32 *arg);

/* s5p6442_pm_run_res
 *
 * go thorugh the given resource list, and look for system ram
*/

static void s5p6442_pm_run_res(struct resource *ptr, run_fn_t fn, u32 *arg)
{
	while (ptr != NULL) {
		if (ptr->child != NULL)
			s5p6442_pm_run_res(ptr->child, fn, arg);

		if ((ptr->flags & IORESOURCE_MEM) &&
		    strcmp(ptr->name, "System RAM") == 0) {
			DBG("Found system RAM at %08lx..%08lx\n",
			    ptr->start, ptr->end);
			arg = (fn)(ptr, arg);
		}

		ptr = ptr->sibling;
	}
}

static void s5p6442_pm_run_sysram(run_fn_t fn, u32 *arg)
{
	s5p6442_pm_run_res(&iomem_resource, fn, arg);
}

static u32 *s5p6442_pm_countram(struct resource *res, u32 *val)
{
	u32 size = (u32)(res->end - res->start)+1;

	size += CHECK_CHUNKSIZE-1;
	size /= CHECK_CHUNKSIZE;

	DBG("Area %08lx..%08lx, %d blocks\n", res->start, res->end, size);

	*val += size * sizeof(u32);
	return val;
}

/* s5p6442_pm_prepare_check
 *
 * prepare the necessary information for creating the CRCs. This
 * must be done before the final save, as it will require memory
 * allocating, and thus touching bits of the kernel we do not
 * know about.
*/

static void s5p6442_pm_check_prepare(void)
{
	crc_size = 0;

	s5p6442_pm_run_sysram(s5p6442_pm_countram, &crc_size);

	DBG("s5p6442_pm_prepare_check: %u checks needed\n", crc_size);

	crcs = kmalloc(crc_size+4, GFP_KERNEL);
	if (crcs == NULL)
		printk(KERN_ERR "Cannot allocated CRC save area\n");
}

static u32 *s5p6442_pm_makecheck(struct resource *res, u32 *val)
{
	unsigned long addr, left;

	for (addr = res->start; addr < res->end;
	     addr += CHECK_CHUNKSIZE) {
		left = res->end - addr;

		if (left > CHECK_CHUNKSIZE)
			left = CHECK_CHUNKSIZE;

		*val = crc32_le(~0, phys_to_virt(addr), left);
		val++;
	}

	return val;
}

/* s5p6442_pm_check_store
 *
 * compute the CRC values for the memory blocks before the final
 * sleep.
*/

static void s5p6442_pm_check_store(void)
{
	if (crcs != NULL)
		s5p6442_pm_run_sysram(s5p6442_pm_makecheck, crcs);
}

/* in_region
 *
 * return TRUE if the area defined by ptr..ptr+size contatins the
 * what..what+whatsz
*/

static inline int in_region(void *ptr, int size, void *what, size_t whatsz)
{
	if ((what+whatsz) < ptr)
		return 0;

	if (what > (ptr+size))
		return 0;

	return 1;
}

static u32 *s5p6442_pm_runcheck(struct resource *res, u32 *val)
{
	void *save_at = phys_to_virt(s5p6442_sleep_save_phys);
	unsigned long addr;
	unsigned long left;
	void *ptr;
	u32 calc;

	for (addr = res->start; addr < res->end;
	     addr += CHECK_CHUNKSIZE) {
		left = res->end - addr;

		if (left > CHECK_CHUNKSIZE)
			left = CHECK_CHUNKSIZE;

		ptr = phys_to_virt(addr);

		if (in_region(ptr, left, crcs, crc_size)) {
			DBG("skipping %08lx, has crc block in\n", addr);
			goto skip_check;
		}

		if (in_region(ptr, left, save_at, 32*4 )) {
			DBG("skipping %08lx, has save block in\n", addr);
			goto skip_check;
		}

		/* calculate and check the checksum */

		calc = crc32_le(~0, ptr, left);
		if (calc != *val) {
			printk(KERN_ERR PFX "Restore CRC error at "
			       "%08lx (%08x vs %08x)\n", addr, calc, *val);

			DBG("Restore CRC error at %08lx (%08x vs %08x)\n",
			    addr, calc, *val);
		}

	skip_check:
		val++;
	}

	return val;
}

/* s5p6442_pm_check_restore
 *
 * check the CRCs after the restore event and free the memory used
 * to hold them
*/

static void s5p6442_pm_check_restore(void)
{
	if (crcs != NULL) {
		s5p6442_pm_run_sysram(s5p6442_pm_runcheck, crcs);
		kfree(crcs);
		crcs = NULL;
	}
}

#else

#define s5p6442_pm_check_prepare() do { } while(0)
#define s5p6442_pm_check_restore() do { } while(0)
#define s5p6442_pm_check_store()   do { } while(0)
#endif

/* helper functions to save and restore register state */

void s5p6442_pm_do_save(struct sleep_save *ptr, int count)
{
	for (; count > 0; count--, ptr++) {
		ptr->val = __raw_readl(ptr->reg);
		//DBG("saved %p value %08lx\n", ptr->reg, ptr->val);

		if (ptr->reg == S5P_CLKGATE_IP3 && !(ptr->val & S5P_CLKGATE_IP3_UART0)){
                	ptr->val |= S5P_CLKGATE_IP3_UART0;
		}
	}
}

/* s5p6442_pm_do_restore
 *
 * restore the system from the given list of saved registers
 *
 * Note, we do not use DBG() in here, as the system may not have
 * restore the UARTs state yet
*/

void s5p6442_pm_do_restore(struct sleep_save *ptr, int count)
{
	for (; count > 0; count--, ptr++) {
		//printk(KERN_DEBUG "restore %p (restore %08lx, was %08x)\n",
		       //ptr->reg, ptr->val, __raw_readl(ptr->reg));

		__raw_writel(ptr->val, ptr->reg);
	}
}

/* s5p6442_pm_do_restore_core
 *
 * similar to s36410_pm_do_restore_core
 *
 * WARNING: Do not put any debug in here that may effect memory or use
 * peripherals, as things may be changing!
*/

/* s5p6442_pm_do_save_phy
 *
 * save register of system
 *
 * Note, I made this function to support driver with ioremap.
 * If you want to use this function, you should to input as first parameter
 * struct sleep_save_phy type
*/

int s3c2410_pm_do_save_phy(struct sleep_save_phy *ptr, struct platform_device *pdev, int count)
{
	void __iomem *target_reg;
	struct resource *res;
	u32 reg_size;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(res == NULL){
		printk(KERN_ERR "%s resource get error\n",__FUNCTION__);
		return 0;
	}
	reg_size = res->end - res->start + 1;
	target_reg = ioremap(res->start,reg_size);

	for (; count > 0; count--, ptr++) {
		ptr->val = readl(target_reg + (ptr->reg));
	}

	return 0;
}

/* s5p6442_pm_do_restore_phy
 *
 * restore register of system
 *
 * Note, I made this function to support driver with ioremap.
 * If you want to use this function, you should to input as first parameter
 * struct sleep_save_phy type
*/

int s3c2410_pm_do_restore_phy(struct sleep_save_phy *ptr, struct platform_device *pdev, int count)
{
	void __iomem *target_reg;
	struct resource *res;
	u32 reg_size;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(res == NULL){
		printk(KERN_ERR "%s resource get error\n",__FUNCTION__);
		return 0;
	}
	reg_size = res->end - res->start + 1;
	target_reg = ioremap(res->start,reg_size);

	for (; count > 0; count--, ptr++) {
		writel(ptr->val, (target_reg + ptr->reg));
	}

	return 0;
}

static void s5p6442_pm_do_restore_core(struct sleep_save *ptr, int count)
{
	for (; count > 0; count--, ptr++) {
		__raw_writel(ptr->val, ptr->reg);
	}
}
#if 1
extern unsigned int s5p_pm_config_wakeup_source(void);
static unsigned int s5p6442_pm_configure_extint(void)
{
		return s5p_pm_config_wakeup_source();
}
#else
#define POWERKEY_GPIO_NUM        6
static void s5p6442_pm_configure_extint(void)
{
	u32 tmp;

	tmp = __raw_readl(S5P64XX_GPH2DAT);
	tmp |= (0x1 << POWERKEY_GPIO_NUM);
	__raw_writel(tmp, S5P64XX_GPH2DAT);

	tmp = __raw_readl(S5P64XX_GPH2CON);
	tmp |= (0xF << (4 * POWERKEY_GPIO_NUM));
	__raw_writel(tmp, S5P64XX_GPH2CON);

	tmp = __raw_readl(S5P64XX_GPH2PUD);
	tmp &= ~(0x3 << (2 * POWERKEY_GPIO_NUM));
	tmp |= (0x2 << (2 * POWERKEY_GPIO_NUM));
	__raw_writel(tmp, S5P64XX_GPH2PUD);

//	s3c_gpio_cfgpin(GPIO_nPOWER_KEY, GPIO_nPOWER_KEY_STATE);
//	s3c_gpio_setpull(GPIO_nPOWER_KEY, S3C_GPIO_PULL_UP);
	udelay(50);

	__raw_writel((__raw_readl(S5P64XX_EINT2CON) & ~(0x7 << (4 * POWERKEY_GPIO_NUM))) |
                     (0x4 << (4 * POWERKEY_GPIO_NUM)), S5P64XX_EINT2CON);			//setting ext interrupt

	__raw_writel((1UL << POWERKEY_GPIO_NUM), S5P64XX_EINT2PEND);			//clearing pending Interrupt
	__raw_writel(__raw_readl(S5P64XX_EINT2MASK)&~(1UL << POWERKEY_GPIO_NUM), S5P64XX_EINT2MASK); //unmasking ext. INT
}
#endif
#if 0
static void s5p64xx_pm_configure_extint(void)
{
#if 0 
	s3c_gpio_cfgpin(S5P64XX_GPN(10), S5P64XX_GPN10_EINT10);
	s3c_gpio_setpull(S5P64XX_GPN(10), S3C_GPIO_PULL_UP);
#else
	__raw_writel((__raw_readl(S5P64XX_GPNCON) & ~(0x3 << 20)) |
		     (0x2 << 20), S5P64XX_GPNCON);
	__raw_writel((__raw_readl(S5P64XX_GPNPUD) & ~(0x3 << 20)) |
		     (0x2 << 20), S5P64XX_GPNPUD);
#endif
	udelay(50);

	__raw_writel((__raw_readl(S5P64XX_EINT0CON0) & ~(0x7 << 20)) |
		     (0x2 << 20), S5P64XX_EINT0CON0);

	__raw_writel(1UL << (IRQ_EINT(10) - IRQ_EINT(0)), S5P64XX_EINT0PEND);
	__raw_writel(__raw_readl(S5P64XX_EINT0MASK)&~(1UL << (IRQ_EINT(10) - IRQ_EINT(0))), S5P64XX_EINT0MASK);
}
#endif
void (*pm_cpu_prep)(void);
void (*pm_cpu_sleep)(void);

void s5p_pwr_clk_gating_reset(void)
{
	u32 tmp;
	__raw_writel(0XFFFFFFFF, S5P_CLKGATE_IP0);
	__raw_writel(0XFFFFFFFF, S5P_CLKGATE_IP1);
	__raw_writel(0XFFFFFFFF, S5P_CLKGATE_IP2);
	__raw_writel(0XFFFFFFFF, S5P_CLKGATE_IP3);
	__raw_writel(0XFFFFFFFF, S5P_CLKGATE_IP4);

	tmp = __raw_readl(S5P_NORMAL_CFG);
	tmp |= (0X3E);
	__raw_writel(tmp, S5P_NORMAL_CFG);
}

#define any_allowed(mask, allow) (((mask) & (allow)) != (allow))

/* s5p6442_pm_enter
 *
 * central control for sleep/resume process
*/

int charger_wakeup = 0;
EXPORT_SYMBOL(charger_wakeup);

// keypad driver has responsible for clearing the flag 
int wakeup_flag_for_key = 0;
int wakeup_sdslot = 0;

EXPORT_SYMBOL(wakeup_flag_for_key);

static int s5p6442_pm_enter(suspend_state_t state)
{
	unsigned long regs_save[16];
	unsigned int tmp;
	unsigned int eint_wakeup_mask;

	/* ensure the debug is initialised (if enabled) */

	DBG("s5p6442_pm_enter(%d)\n", state);

	if (pm_cpu_prep == NULL || pm_cpu_sleep == NULL) {
		printk(KERN_ERR PFX "error: no cpu sleep functions set\n");
		return -EINVAL;
	}

	/* prepare check area if configured */
	s5p6442_pm_check_prepare();

	/* store the physical address of the register recovery block */
	s5p6442_sleep_save_phys = virt_to_phys(regs_save);

	printk("s5p6442_sleep_save_phys=0x%08lx\n", s5p6442_sleep_save_phys);

	DEBUG_WAKEUP("[PM-1] EINT0(0x%08x), EINT1(0x%08x), EINT2(0x%08x), EINT3(0x%08x)\n", 
		__raw_readl(S5P64XX_GPA0_BASE+0xF40), __raw_readl(S5P64XX_GPA0_BASE+0xF44),
		__raw_readl(S5P64XX_GPA0_BASE+0xF48), __raw_readl(S5P64XX_GPA0_BASE+0xF4C));

	/* save all necessary core registers not covered by the drivers */

	s5p6442_pm_do_save(gpio_save, ARRAY_SIZE(gpio_save));
	s5p6442_pm_do_save(irq_save, ARRAY_SIZE(irq_save));
	s5p6442_pm_do_save(core_save, ARRAY_SIZE(core_save));
	s5p6442_pm_do_save(sromc_save, ARRAY_SIZE(sromc_save));
	// s5p6442_pm_do_save(onenand_save, ARRAY_SIZE(onenand_save));
	// s5p6442_pm_do_save(uart_save, ARRAY_SIZE(uart_save));

#ifdef S5P6442_POWER_GATING_IROM
	//s5p6442_pwrgate_config(S5P6442_IROM_ID, S5P6442_ACTIVE_MODE);
	 tmp = __raw_readl(S5P_NORMAL_CFG);
         tmp |= (1 << S5P6442_IROM_ID);
        __raw_writel(tmp, S5P_NORMAL_CFG);
#endif

	/* ensure INF_REG0  has the resume address */
	__raw_writel(virt_to_phys(s5p6442_cpu_resume), S5P_INFORM0);

//	s5p_pwr_clk_gating_reset();

	/* set the irq configuration for wake */
	// s5p6442_pm_configure_extint();

	/* call cpu specific preperation */

	pm_cpu_prep();

	/* flush cache back to ram */

	flush_cache_all();

	//s5p6442_pm_check_store();

	s5p_config_sleep_gpio();  // for sleep current optimization

	/* set the irq configuration for wake */
	eint_wakeup_mask = s5p6442_pm_configure_extint();

	/* USB & OSC Clock pad Disable */
	tmp = __raw_readl(S5P_SLEEP_CFG);
	tmp &= ~((1 << S5P_SLEEP_CFG_OSC_EN) | (1 << S5P_SLEEP_CFG_USBOSC_EN));
	if(call_state)
	{
		tmp |= ((1 << S5P_SLEEP_CFG_OSC_EN) | (1 << S5P_SLEEP_CFG_USBOSC_EN));
	}
	__raw_writel(tmp , S5P_SLEEP_CFG);

	/* Power mode Config setting */
	tmp = __raw_readl(S5P_PWR_CFG);
	tmp &= S5P_CFG_WFI_CLEAN;
	tmp |= S5P_CFG_WFI_SLEEP;
	__raw_writel(tmp,S5P_PWR_CFG);
#if 0
	/* Set wakeup mask regsiter */
	tmp = 0xFFFF;
	tmp &= ~(1 << 5);	// keypad
	tmp &= ~(1 << 1);
	tmp &= ~(1 << 2);
	__raw_writel(tmp , S5P_WAKEUP_MASK);
	//Save the normal mode configuration of WAKE_UP sources and make EXT Key as a
	//wake up source from suspend mode(Praveen)
#if 1 
	eint_wakeup_mask = __raw_readl(S5P_EINT_WAKEUP_MASK);
	tmp = 0xFFFFFFFF;
	//tmp &= ~(1 << 10);
	tmp &= ~(1 << 22);    //EINT2_6
	//tmp &= ~(1 << 15);
	__raw_writel(tmp, S5P_EINT_WAKEUP_MASK);
#endif
#endif	
	//Removed by Praveen(July 12, 2010)
	//__raw_writel(s3c_irqwake_eintmask,S5P_EINT_WAKEUP_MASK);

	/* send the cpu to sleep... */
	__raw_writel(0xffffffff, S5P64XX_VIC0INTENCLEAR);
	__raw_writel(0xffffffff, S5P64XX_VIC1INTENCLEAR);
	__raw_writel(0xffffffff, S5P64XX_VIC2INTENCLEAR);
	__raw_writel(0xffffffff, S5P64XX_VIC0SOFTINTCLEAR);
	__raw_writel(0xffffffff, S5P64XX_VIC1SOFTINTCLEAR);
	__raw_writel(0xffffffff, S5P64XX_VIC2SOFTINTCLEAR);

	/*  EINTPEND CLEAR */
	__raw_writel ( 0xFF, S5P64XX_GPA0_BASE + 0xF40 );
	__raw_writel ( 0xFF, S5P64XX_GPA0_BASE + 0xF44 );
	__raw_writel ( 0xFF, S5P64XX_GPA0_BASE + 0xF48 );
	__raw_writel ( 0xFF, S5P64XX_GPA0_BASE + 0xF4C );

#if (CONFIG_BOARD_REVISION == 0x0)  // EVT1 doesn't work 'PDNEN' setting.
	__raw_writel(0x2, S5P64XX_PDNEN);
#endif

	/* SYSC INT Disable */
	tmp = __raw_readl(S5P_OTHERS);
	tmp &= ~(0x3<<8);
	tmp |= S5P_OTHER_SYSC_INTOFF;
	if(call_state)
	{
		tmp |= (0x3<<8);
	}
	__raw_writel(tmp,S5P_OTHERS);

	tmp = __raw_readl(S5P_WAKEUP_STAT);
	__raw_writel(tmp, S5P_WAKEUP_STAT);

	/* s5p6442_cpu_save will also act as our return point from when
	 * we resume as it saves its own register state, so use the return
	 * code to differentiate return from save and return from sleep */

	if (s5p6442_cpu_save(regs_save) == 0) {
		flush_cache_all();
		pm_cpu_sleep();
	}

	/* restore the cpu state */
	cpu_init();

	/* restore the system state */
	s5p6442_pm_do_restore(gpio_save, ARRAY_SIZE(gpio_save));
	s5p6442_pm_do_restore(irq_save, ARRAY_SIZE(irq_save));	
	s5p6442_pm_do_restore_core(core_save, ARRAY_SIZE(core_save));
	s5p6442_pm_do_restore(sromc_save, ARRAY_SIZE(sromc_save));
	//s5p6442_pm_do_restore(onenand_save, ARRAY_SIZE(onenand_save));
	//s5p6442_pm_do_restore(uart_save, ARRAY_SIZE(uart_save));

#if (CONFIG_BOARD_REVISION == 0x0)
	__raw_writel(0x0, S5P64XX_PDNEN);
#endif

	/*enable gpio, uart, mmc*/
	tmp = __raw_readl(S5P_OTHERS);
	tmp |= (1<<31)|(1<<28)|(1<<29);
	__raw_writel(tmp, S5P_OTHERS);

	/* UART INTERRUPT PENDING REG clear */
	tmp = __raw_readl(S3C24XX_VA_UART0 + S3C2410_UINTP);	
	__raw_writel(tmp, S3C24XX_VA_UART0 + S3C2410_UINTP);
	tmp = __raw_readl(S3C24XX_VA_UART1 + S3C2410_UINTP);
	__raw_writel(tmp, S3C24XX_VA_UART1 + S3C2410_UINTP);
	tmp = __raw_readl(S3C24XX_VA_UART2 + S3C2410_UINTP);
	__raw_writel(tmp, S3C24XX_VA_UART2 + S3C2410_UINTP);

	tmp = __raw_readl(S3C24XX_VA_UART0 + S3C2410_UINTSP);
	__raw_writel(tmp, S3C24XX_VA_UART0 + S3C2410_UINTSP);
	tmp = __raw_readl(S3C24XX_VA_UART1 + S3C2410_UINTSP);
	__raw_writel(tmp, S3C24XX_VA_UART1 + S3C2410_UINTSP);
	tmp = __raw_readl(S3C24XX_VA_UART2 + S3C2410_UINTSP);
	__raw_writel(tmp, S3C24XX_VA_UART2 + S3C2410_UINTSP);

	/*
	__raw_writel(0xF, S3C24XX_VA_UART0 + S3C2410_UINTSP);
	__raw_writel(0xF, S3C24XX_VA_UART1 + S3C2410_UINTSP);
	__raw_writel(0xF, S3C24XX_VA_UART2 + S3C2410_UINTSP);
	*/

	/* Enable CLK GATE for UART */
	tmp = __raw_readl(S5P_CLKGATE_IP3);
        tmp |= (S5P_CLKGATE_IP3_UART0 | S5P_CLKGATE_IP3_UART1 | S5P_CLKGATE_IP3_UART2);
       __raw_writel(tmp, S5P_CLKGATE_IP3);

	//[sm.kim: remove inconsistant code
	//tmp = __raw_readl(S5P64XX_EINT2PEND);
	//__raw_writel(tmp, S5P64XX_EINT2PEND);
	//]

	// __raw_writel(__raw_readl(S5P64XX_EINT0MASK)&~(1UL << 1), S5P64XX_EINT0MASK); //unmasking ext. INT

	tmp = __raw_readl(S5P_WAKEUP_STAT);
	DEBUG_WAKEUP("[PM] WAKEUP_STAT (0x%08x)\n", tmp);
	__raw_writel(tmp, S5P_WAKEUP_STAT);

	if( (__raw_readl(S5P64XX_GPA0_BASE+0xF40) & (1 << 7)) ||  // AP_PMIC_IRQ
			(__raw_readl(S5P64XX_GPA0_BASE+0xF48) & (1 << 7)) )  // JACK_nINT
		charger_wakeup = 1;
	else
		charger_wakeup = 0;

	if (__raw_readl(S5P64XX_GPA0_BASE+0xF48) & 0x01)
		wakeup_sdslot = 1;

	if(tmp == 0x00000001)
	{
		if((__raw_readl(S5P64XX_GPA0_BASE+0xF48) & 0x40) ||(__raw_readl(S5P64XX_GPA0_BASE+0xF44) & 0x01) )
			wakeup_flag_for_key = 249;
		else if(__raw_readl(S5P64XX_GPA0_BASE+0xF4C) & 0x01)
			wakeup_flag_for_key = 102;
		else
			wakeup_flag_for_key = 0;
	}

	DEBUG_WAKEUP("[PM-2] EINT0(0x%08x), EINT1(0x%08x), EINT2(0x%08x), EINT3(0x%08x)\n", 
		__raw_readl(S5P64XX_GPA0_BASE+0xF40), __raw_readl(S5P64XX_GPA0_BASE+0xF44),
		__raw_readl(S5P64XX_GPA0_BASE+0xF48), __raw_readl(S5P64XX_GPA0_BASE+0xF4C));
#if 1 //Write back the normal mode configuration for WAKE_UP source(Praveen)
	__raw_writel(eint_wakeup_mask, S5P_EINT_WAKEUP_MASK);
#endif
	// mdelay(10);

	DBG("post sleep, preparing to return\n");

	//s5p6442_pm_check_restore();

	/* ok, let's return from sleep */
	DBG("S5P6442 PM Resume (post-restore)\n");
	return 0;
}

static struct platform_suspend_ops s5p6442_pm_ops = {
	.enter		= s5p6442_pm_enter,
	.valid		= suspend_valid_only_mem,
};

/* s5p6442_pm_init
 *
 * Attach the power management functions. This should be called
 * from the board specific initialisation if the board supports
 * it.
*/

int __init s5p6442_pm_init(void)
{
	printk("S5P6442 Power Management, (c) 2008 Samsung Electronics\n");

#ifdef CONFIG_S5P64XX_POWER_GATING
	s5p6442_pwrgate_init();
#else
#ifdef CONFIG_S5P64XX_CLOCK_GATING
	s5p6442_clkgate_init();
#endif /* CONFIG_S5P64XX_CLOCK_GATING */
#endif /* CONFIG_S5P64XX_POWER_GATING */ 
	suspend_set_ops(&s5p6442_pm_ops);

	/* set default value for EINT_WAKEUP_MASK */
	s3c_irqwake_eintmask = 0x00000000;
	s3c_irqwake_eintmask |= (1 << 21); /* BT_HOST_WAKE */
	
	return 0;
}

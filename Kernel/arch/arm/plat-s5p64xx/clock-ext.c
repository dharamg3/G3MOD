/* linux/arch/arm/plat-s5p64xx/clock.c
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * S5P64XX Base clock support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/io.h>

#include <mach/hardware.h>
#include <mach/map.h>

#include <plat/regs-clock.h>
#include <plat/regs-audss.h>
#include <plat/cpu.h>
#include <plat/devs.h>
#include <plat/clock.h>
#include <plat/pll.h>
#include <plat/s5p6442-dvfs.h>
#include <plat/regs-dramc.h>
#include <plat/regs-serial.h>
/* definition for cpu freq */

//#define ARM_PLL_CON 	S5P_APLL_CON
//#define ARM_CLK_DIV	S5P_CLK_DIV0

#define INIT_XTAL			12 * MHZ

/* ARMCLK, D0CLK, P0CLK, D1CLK, P1CLK, APLL_RATIO, D0CLK_RATIO, P0CLK_RATIO, D1CLK_RATIO, P1CLK_RATIO */
u32 s5p_cpu_clk_tab_ov[][10] = {
	{ 1400*MHZ, 185*MHZ, (185*MHZ)/2, 185*MHZ,  6, 0, 0, 1, 0, 1},	
	{ 1200*MHZ, 185*MHZ, (185*MHZ)/2, 185*MHZ,  5, 0, 0, 1, 0, 1},
	{ 1000*MHZ, 185*MHZ, (185*MHZ)/2, 185*MHZ,  4, 0, 0, 1, 0, 1},
        { 800*MHZ, 185*MHZ, (185*MHZ)/2, 185*MHZ,   3, 0, 0, 1, 0, 1}, 
        { 600*MHZ, 185*MHZ, (185*MHZ)/2, 185*MHZ,   3, 0, 0, 1, 0, 1},
        { 400*MHZ, 185*MHZ, (185*MHZ)/2, 185*MHZ,   3, 0, 0, 1, 0, 1},
        { 200*MHZ, 185*MHZ, (185*MHZ)/2, 185*MHZ,   3, 0, 0, 1, 0, 1},
};

u32 s5p_cpu_pll_tab[][4] = {
// A P L L                                 M P L L
{((1 << 31) | (700 << 16) | (3 << 8) | 1), ((1 << 31) | (333 << 16) | (3 << 8) | 3)},
{((1 << 31) | (600 << 16) | (3 << 8) | 1), ((1 << 31) | (333 << 16) | (3 << 8) | 3)},
{((1 << 31) | (500 << 16) | (3 << 8) | 1), ((1 << 31) | (333 << 16) | (3 << 8) | 3)},
{((1 << 31) | (800 << 16) | (6 << 8) | 1), ((1 << 31) | (333 << 16) | (3 << 8) | 3)},
{((1 << 31) | (600 << 16) | (6 << 8) | 1), ((1 << 31) | (333 << 16) | (3 << 8) | 3)},
{((1 << 31) | (400 << 16) | (6 << 8) | 1), ((1 << 31) | (333 << 16) | (3 << 8) | 3)},
{((1 << 31) | (200 << 16) | (6 << 8) | 1), ((1 << 31) | (333 << 16) | (3 << 8) | 3)},
};

#define GET_DIV(clk, field) ((((clk) & field##_MASK) >> field##_SHIFT) + 1)

unsigned int S5P6442_FREQ_TAB = 0;

u32 (*s5p_cpu_clk_tab[1])[10] = {
	s5p_cpu_clk_tab_ov,
} ;

static u32 s5p_cpu_clk_tab_size(void)
{
	u32 size;
	size = ARRAY_SIZE(s5p_cpu_clk_tab_ov);

	return size;
}

unsigned long s5p_fclk_get_rate(void)
{
	unsigned long apll_con;
	unsigned long clk_div0;
	unsigned long ret;

	apll_con = __raw_readl(S5P_APLL_CON);
	clk_div0 = __raw_readl(S5P_CLK_DIV0);

	ret = s5p64xx_get_pll(INIT_XTAL, apll_con, S5P64XX_PLL_APLL);

	return (ret / GET_DIV(clk_div0, S5P_CLKDIV0_APLL));
}

unsigned long s5p_fclk_round_rate(struct clk *clk, unsigned long rate)
{
	u32 iter;
	u32 size;

	size = s5p_cpu_clk_tab_size();

	for(iter = 1 ; iter < size ; iter++) {
		if(rate > s5p_cpu_clk_tab[S5P6442_FREQ_TAB][iter][0])
			return s5p_cpu_clk_tab[S5P6442_FREQ_TAB][iter-1][0];
	}

	return s5p_cpu_clk_tab[S5P6442_FREQ_TAB][iter - 1][0];
}


#ifdef CONFIG_CPU_FREQ
int s5p6442_clk_set_rate(unsigned int target_freq,
                                unsigned int index )
{
	int cur_freq;
	int pll_change = 0;
	unsigned int mask;
	u32 clk_div0;
	u32 size;
	unsigned long apll, a2m, xtal;
	int timeout = 1000; //10 msec //10 usec uints
	static int flag = 0;
	static unsigned int hd0_clk;
	static unsigned int hd1_clk;
	static unsigned int arm_clk, clk_tmp;
	static  u32 (*cpu_clk_tab)[10];
	static int cur_idx;

	if (!flag){
		hd0_clk = clk_hd0.rate;
		hd1_clk = clk_hd1.rate;
		arm_clk = clk_f.rate;
		flag = 1;
		cur_idx = 3;
	}

//	printk("---> [s5p6442_clk_set_rate] TARGET_FREQ : %d - Index : %d\n", target_freq, index);
	cpu_clk_tab = s5p_cpu_clk_tab[S5P6442_FREQ_TAB];

	size = s5p_cpu_clk_tab_size();

	if(index >= size) {
		return 1;
	}

	
	if (index == cur_idx)
		return 0;



	if(cpu_clk_tab[index][6] > 0)
		__raw_writel(AVG_PRD_REFRESH_INTERVAL_83MHZ, S5P_DRAMC_TIMINGAREF);


	__raw_writel(0x00001111, S5P_CLK_SRC0);

	mask = (~S5P_CLKDIV0_APLL_MASK) & (~S5P_CLKDIV0_D0CLK_MASK) & (~S5P_CLKDIV0_P0CLK_MASK) & (~S5P_CLKDIV0_D1CLK_MASK) & (~S5P_CLKDIV0_P1CLK_MASK);
	clk_div0 = __raw_readl(S5P_CLK_DIV0) & mask;
	clk_div0 |= cpu_clk_tab[index][5] << S5P_CLKDIV0_APLL_SHIFT;
	clk_div0 |= (cpu_clk_tab[index][6]) << S5P_CLKDIV0_D0CLK_SHIFT;
	clk_div0 |= (cpu_clk_tab[index][7]) << S5P_CLKDIV0_P0CLK_SHIFT;
	clk_div0 |= (cpu_clk_tab[index][8]) << S5P_CLKDIV0_D1CLK_SHIFT;
	clk_div0 |= (cpu_clk_tab[index][9]) << S5P_CLKDIV0_P1CLK_SHIFT;

#ifdef MUXD0D1_A2M
	clk_div0 &= ~(S5P_CLKDIV0_A2M_MASK );
//		printk("--> s5p6442 - enable muxd0 : set div0_a2m : %d\n", cpu_clk_tab[index][4]);
	clk_div0 |= (cpu_clk_tab[index][4] << S5P_CLKDIV0_A2M_SHIFT);
#endif /* MUXD0D1_A2M */


//      printk("---> [s5p6442_clk_set_rate] clksrc0 before : %0x\n", __raw_readl(S5P_CLK_SRC0));

	if (index == 0) {
		__raw_writel(0xe10, S5P_APLL_LOCK);
		__raw_writel(((1 << 31) | (800 << 16) | (2 << 8) | 2), S5P_APLL_CON);
	}

	if (index > cur_idx) {

		clk_tmp = __raw_readl(S5P_APLL_CON);
//		printk("----> s5p6442 : APLL_CON = %x  - will be changed to apll_tab = %x (index=%d)\n", clk_tmp, s5p_cpu_pll_tab[index][0], index);
		if (clk_tmp != s5p_cpu_pll_tab[index][0]) {
		    pll_change = 1;
		  __raw_writel(0xe10, S5P_APLL_LOCK);
		  __raw_writel(s5p_cpu_pll_tab[index][0],S5P_APLL_CON);
//		    printk("----> s5p6442 : apll changed\n");
		}

		clk_tmp = __raw_readl(S5P_MPLL_CON);
//		printk("----> s5p6442 : MPLL_CON = %x  - will be changed to mpll_tab = %x (index=%d)\n", clk_tmp, s5p_cpu_pll_tab[index][1],index);
		if (clk_tmp != s5p_cpu_pll_tab[index][1]) {
		    pll_change = 1;
		    __raw_writel(0xe10, S5P_MPLL_LOCK);
		    __raw_writel(s5p_cpu_pll_tab[index][1],S5P_MPLL_CON);
//		    printk("----> s5p6442 : mpll changed\n");
		}

//	        printk("----> s5p6442 : clk_div0 before : %x \n", __raw_readl(S5P_CLK_DIV0));
		s5p6442_changeDivider(clk_div0, S5P_CLK_DIV0);
//	        printk("----> s5p6442 : clk_div0 after : %x \n", __raw_readl(S5P_CLK_DIV0));
	}
	else
	{

//		printk("----> s5p6442 : clk_div0 before : %x \n", __raw_readl(S5P_CLK_DIV0));
		s5p6442_changeDivider(clk_div0, S5P_CLK_DIV0);
//		printk("----> s5p6442 : clk_div0 after : %x \n", __raw_readl(S5P_CLK_DIV0));

		clk_tmp = __raw_readl(S5P_APLL_CON);
//		printk("----> s5p6442 : APLL_CON = %x  - will be changed to apll_tab = %x (index=%d)\n", clk_tmp, s5p_cpu_pll_tab[index][0], index);
		if (clk_tmp != s5p_cpu_pll_tab[index][0]) {
		    pll_change = 1;
		  __raw_writel(0xe10, S5P_APLL_LOCK);
		  __raw_writel(s5p_cpu_pll_tab[index][0],S5P_APLL_CON);
//		    printk("----> s5p6442 : apll changed\n");
		}

		clk_tmp = __raw_readl(S5P_MPLL_CON);
//		printk("----> s5p6442 : MPLL_CON = %x  - will be changed to mpll_tab = %x (index=%d)\n", clk_tmp, s5p_cpu_pll_tab[index][1],index);
		if (clk_tmp != s5p_cpu_pll_tab[index][1]) {
		    pll_change = 1;
		   __raw_writel(0xe10, S5P_MPLL_LOCK);
                   __raw_writel(s5p_cpu_pll_tab[index][1],S5P_MPLL_CON);
//		    printk("----> s5p6442 : mpll changed\n");
		}
	}

	__raw_writel(0x00101111, S5P_CLK_SRC0);

	//s5p6442_postclock();

//todel	s5p_cpu_clkdiv_tab[index][0] = __raw_readl(S5P_CLK_DIV0);

	arm_clk = s5p_fclk_get_rate();

	if(cpu_clk_tab[index][6] == 0)
		__raw_writel(AVG_PRD_REFRESH_INTERVAL_166MHZ, S5P_DRAMC_TIMINGAREF);

	while(__raw_readl(S5P_CLK_DIV_STAT0) && (timeout > 0)){
                timeout--;
                udelay(10);
	}


	if (pll_change == 1) {
		s5p_cpu_pll_tab[index][0] = __raw_readl(S5P_APLL_CON);
		s5p_cpu_pll_tab[index][1] = __raw_readl(S5P_MPLL_CON);
//		printk("----> s5p6442 : stored new values : apll %x mpll %x\n", s5p_cpu_pll_tab[index][0], s5p_cpu_pll_tab[index][1]);
	}

//	printk("---> [s5p6442_clk_set_rate] sapll after : %0x\n", __raw_readl(S5P_APLL_CON));
//	printk("---> [s5p6442_clk_set_rate] smpll after : %0x\n", __raw_readl(S5P_MPLL_CON));
//	printk("---> [s5p6442_clk_set_rate] clk_div0 : %x  current: %x - s5p_fclk_rate = %d\n", clk_div0, __raw_readl(S5P_CLK_DIV0) & mask, s5p_fclk_get_rate());


#ifdef MUXD0D1_A2M
	a2m = arm_clk / GET_DIV(__raw_readl(S5P_CLK_DIV0), S5P_CLKDIV0_A2M);
	clk_f.rate = arm_clk / (cpu_clk_tab[index][5] + 1);
	clk_hd0.rate = a2m / (cpu_clk_tab[index][6] + 1);
#else
        clk_f.rate = arm_clk / (cpu_clk_tab[index][5] + 1);
        clk_hd0.rate = (hd0_clk / (cpu_clk_tab[index][6] + 1));
#endif
        clk_hd1.rate = (hd1_clk / (cpu_clk_tab[index][8] + 1));
	clk_pd0.rate = clk_hd0.rate/ (cpu_clk_tab[index][7] + 1);
	clk_pd1.rate = clk_hd1.rate/ (cpu_clk_tab[index][9] + 1);

	/* For backward compatibility */
	clk_h.rate = clk_hd1.rate;
	clk_p.rate = clk_pd1.rate;
	printk("#####arm_clk %d hd0_clk %d p0_clk %d hd1_clk %d p1_clk %d clkdiv0  %x timeout %d\n", clk_f.rate, clk_hd0.rate, clk_pd0.rate,
		              clk_hd1.rate, clk_pd1.rate, clk_div0, timeout);

	cur_idx = index;

	return 0;
}
#endif /* CONFIG_CPU_FREQ */

int s5p_fclk_set_rate(struct clk *clk, unsigned long rate)
{
#ifdef CONFIG_CPU_FREQ
	int index, ret;

	index = s5p6442_cpufreq_index;
	ret = s5p6442_clk_set_rate(rate, index);
#endif /* CONFIG_CPU_FREQ */
	return 0;
}

struct clk clk_cpu = {
	.name           = "clk_cpu",
	.id             = -1,
	.rate           = 0,
	.parent         = &clk_f,
	.ctrlbit        = 0,
	.set_rate       = s5p_fclk_set_rate,
	.round_rate     = s5p_fclk_round_rate,
};

static int inline s5p64xx_gate(void __iomem *reg,
				struct clk *clk,
				int enable)
{
	unsigned int ctrlbit = clk->ctrlbit;
	u32 con;

	con = __raw_readl(reg);

	if (enable)
		con |= ctrlbit;
	else
		con &= ~ctrlbit;

	__raw_writel(con, reg);
	return 0;
}

int s5p64xx_sclk0_ctrl(struct clk *clk, int enable)
{
	return s5p64xx_gate(S5P_CLKGATE_SCLK0, clk, enable);
}

int s5p64xx_clk_ip0_ctrl(struct clk *clk, int enable)
{
	return s5p64xx_gate(S5P_CLKGATE_IP0, clk, enable);
}

int s5p64xx_clk_ip1_ctrl(struct clk *clk, int enable)
{
	return s5p64xx_gate(S5P_CLKGATE_IP1, clk, enable);
}

int s5p64xx_clk_ip2_ctrl(struct clk *clk, int enable)
{
	return s5p64xx_gate(S5P_CLKGATE_IP2, clk, enable);
}

int s5p64xx_clk_ip3_ctrl(struct clk *clk, int enable)
{
	return s5p64xx_gate(S5P_CLKGATE_IP3, clk, enable);
}

int s5p64xx_clk_ip4_ctrl(struct clk *clk, int enable)
{
	return s5p64xx_gate(S5P_CLKGATE_IP4, clk, enable);
}

int s5p64xx_audss_clkctrl(struct clk *clk, int enable)
{
	return s5p64xx_gate(S5P_CLKGATE_AUDSS, clk, enable);
}

static struct clk init_clocks_disable[] = {
#if 0
	{
		.name		= "nand",
		.id		= -1,
		.parent		= &clk_h,
	}, {
		.name		= "adc",
		.id		= -1,
		.parent		= &clk_p,
		.enable		= s5p64xx_pclk_ctrl,
		.ctrlbit	= S5P_CLKCON_PCLK_TSADC,
	}, {
		.name		= "i2c",
		.id		= -1,
		.parent		= &clk_p,
		.enable		= s5p64xx_pclk_ctrl,
		.ctrlbit	= S5P_CLKCON_PCLK_IIC0,
	}, {
		.name		= "iis_v40",
		.id		= 0,
		.parent		= &clk_p,
		.enable		= s5p64xx_pclk_ctrl,
		.ctrlbit	= S5P_CLKCON_PCLK_IIS2,
	}, {
		.name		= "spi",
		.id		= 0,
		.parent		= &clk_p,
		.enable		= s5p64xx_pclk_ctrl,
		.ctrlbit	= S5P_CLKCON_PCLK_SPI0,
	}, {
		.name		= "spi",
		.id		= 1,
		.parent		= &clk_p,
		.enable		= s5p64xx_pclk_ctrl,
		.ctrlbit	= S5P_CLKCON_PCLK_SPI1,
	}, {
		.name		= "sclk_spi_48",
		.id		= 0,
		.parent		= &clk_48m,
		.enable		= s5p64xx_sclk_ctrl,
		.ctrlbit	= S5P_CLKCON_SCLK0_SPI0_48,
	}, {
		.name		= "sclk_spi_48",
		.id		= 1,
		.parent		= &clk_48m,
		.enable		= s5p64xx_sclk_ctrl,
		.ctrlbit	= S5P_CLKCON_SCLK0_SPI1_48,
	}, {
		.name		= "48m",
		.id		= 0,
		.parent		= &clk_48m,
		.enable		= s5p64xx_sclk_ctrl,
		.ctrlbit	= S5P_CLKCON_SCLK0_MMC0_48,
	}, {
		.name		= "48m",
		.id		= 1,
		.parent		= &clk_48m,
		.enable		= s5p64xx_sclk_ctrl,
		.ctrlbit	= S5P_CLKCON_SCLK0_MMC1_48,
	}, {
		.name		= "48m",
		.id		= 2,
		.parent		= &clk_48m,
		.enable		= s5p64xx_sclk_ctrl,
		.ctrlbit	= S5P_CLKCON_SCLK0_MMC2_48,
	}, {
		.name    	= "otg",
		.id	   	= -1,
		.parent  	= &clk_h,
		.enable  	= s5p64xx_hclk0_ctrl,
		.ctrlbit 	= S5P_CLKCON_HCLK0_USB
	}, {
		.name    	= "post",
		.id	   	= -1,
		.parent  	= &clk_h,
		.enable  	= s5p64xx_hclk0_ctrl,
		.ctrlbit 	= S5P_CLKCON_HCLK0_POST0
	},
#endif
};

static struct clk init_clocks[] = {
	/* Multimedia */
	{
		.name           = "fimc",
		.id             = 0,
		.parent         = &clk_hd1,
		.enable         = s5p64xx_clk_ip0_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP0_FIMC0,
	}, {
		.name           = "fimc",
		.id             = 1,
		.parent         = &clk_hd1,
		.enable         = s5p64xx_clk_ip0_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP0_FIMC1,
	}, {
		.name           = "fimc",
		.id             = 2,
		.parent         = &clk_hd1,
		.enable         = s5p64xx_clk_ip0_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP0_FIMC2,
	}, {
		.name           = "mfc",
		.id             = -1,
		.parent         = &clk_hd1,   //changed from hd1 to hd0
        	.enable         = s5p64xx_clk_ip0_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP0_MFC,
	}, {
		.name           = "jpeg",
		.id             = -1,
		.parent         = &clk_hd0,  //hd0
		.enable         = s5p64xx_clk_ip0_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP0_JPEG,
	}, {
		.name           = "rotator",
		.id             = -1,
		.parent         = &clk_hd0,
		.enable         = s5p64xx_clk_ip0_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP0_ROTATOR,
	}, {
		.name           = "g3d",
		.id             = -1,
		.parent         = &clk_hd0,
		.enable         = s5p64xx_clk_ip0_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP0_G3D,
	},
	/* Connectivity and Multimedia */
	{
		.name           = "otg",
		.id             = -1,
		.parent         = &clk_hd1,   //hd1
		.enable         = s5p64xx_clk_ip1_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP1_USBOTG,
	}, {
		.name           = "tvenc",
		.id             = -1,
		.parent         = &clk_hd1,
		.enable         = s5p64xx_clk_ip1_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP1_TVENC, 
	}, { 
		.name           = "mixer",
		.id             = -1,
		.parent         = &clk_hd1,  
		.enable         = s5p64xx_clk_ip1_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP1_MIXER,
	}, {
		.name           = "vp",
		.id             = -1,
		.parent         = &clk_hd1,   // HD1
		.enable         = s5p64xx_clk_ip1_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP1_VP,
	}, { 
		.name           = "lcd",	// fimd ?
		.id             = -1,
		.parent         = &clk_hd1,  //hd1
		.enable         = s5p64xx_clk_ip1_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP1_FIMD,
	},  {
		.name           = "nandxl",
		.id             = 0,
		.parent         = &clk_hd1,
		.enable         = s5p64xx_clk_ip1_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP1_NANDXL,
	},

	/* Connectivity */
	{
		.name		= "hsmmc",
		.id		= 0,
		.parent		= &clk_hd1,
		.enable		= s5p64xx_clk_ip2_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP2_HSMMC0,
	}, {
		.name		= "hsmmc",
		.id		= 1,
		.parent		= &clk_hd1,
		.enable		= s5p64xx_clk_ip2_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP2_HSMMC1,
	}, {
		.name		= "hsmmc",
		.id		= 2,
		.parent		= &clk_hd1,
		.enable		= s5p64xx_clk_ip2_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP2_HSMMC2,
	},
	/* Peripherals */

	{
		.name		= "systimer",
		.id		= -1,
		.parent		= &clk_pd1,  //pd1
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_SYSTIMER,
	}, {
		.name		= "watchdog",
		.id		= -1,
		.parent		= &clk_pd1, //pd1
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_WDT,
	}, {
		.name		= "rtc",
		.id		= -1,
		.parent		= &clk_pd1,  //pd1
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_RTC,
	}, {
		.name		= "uart",
		.id		= 0,
		.parent		= &clk_pd1,
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_UART0,
	}, {
		.name		= "uart",
		.id		= 1,
		.parent		= &clk_pd1,
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_UART1,
	}, {
		.name		= "uart",
		.id		= 2,
		.parent		= &clk_pd1,
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_UART2,
	}, {
		.name		= "i2c",
		.id		= 0,
		.parent		= &clk_pd1, //pd1
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_I2C0,
	}, {
		.name		= "i2c",
		.id		= 1,
		.parent		= &clk_pd1, //pd1
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_I2C1,
	}, {
		.name		= "i2c",
		.id		= 2,
		.parent		= &clk_pd1,  //pd1
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_I2C2,
	}, {
		.name		= "spi",
		.id		= 0,
		.parent		= &clk_pd1, //pd1
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_SPI0,
	}, {
		.name		= "timers",
		.id		= -1,
		.parent		= &clk_pd1,   //pd1
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_PWM,
	}, {
		.name		= "lcd",
		.id		= -1,
		.parent		= &clk_hd1,   //hd1
		.enable		= s5p64xx_clk_ip1_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP1_FIMD,
	},{
		.name		= "adc",
		.id		= -1,
		.parent		= &clk_pd1, //pd1
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_TSADC,
	}, {
		.name		= "keypad",
		.id		= -1,
		.parent		= &clk_pd1,    // pd1
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_KEYIF,
	},

	/* Audio (IP3) devices */
	{
		.name		= "i2s_v50",
		.id		= 0,
		.parent		= &clk_pd1,   //pd1
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_I2S0, /* I2S0 is v5.0 */
	},{
		.name		= "i2s_v32",
		.id		= 0,
		.parent		= &clk_p,
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_I2S1,  /* I2S1 is v3.2 */ 
	}, {
		.name		= "pcm",
	        .id     = 0,
		.parent		= &clk_p,
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_PCM0,
	}, {
		.name		= "pcm",
		.id		= 1,
		.parent		= &clk_p,
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_PCM1,
	},{
		.name		= "mfc",
		.id		= -1,
		.parent		= &clk_hd1,  //hd1
		.enable		= s5p64xx_clk_ip0_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP0_MFC,
        },
};

static struct clk *clks[] __initdata = {
	&clk_ext,
	&clk_epll,
	&clk_cpu,
};

void __init s5p64xx_register_clocks(void)
{
	struct clk *clkp;
	int ret;
	int ptr;

	s3c24xx_register_clocks(clks, ARRAY_SIZE(clks));

	clkp = init_clocks;
	for (ptr = 0; ptr < ARRAY_SIZE(init_clocks); ptr++, clkp++) {
		ret = s3c24xx_register_clock(clkp);
		if (ret < 0) {
			printk(KERN_ERR "Failed to register clock %s (%d)\n", clkp->name, ret);
		}
	}

	clkp = init_clocks_disable;
	for (ptr = 0; ptr < ARRAY_SIZE(init_clocks_disable); ptr++, clkp++) {
		ret = s3c24xx_register_clock(clkp);
		if (ret < 0) {
			printk(KERN_ERR "Failed to register clock %s (%d)\n", clkp->name, ret);
		}

		(clkp->enable)(clkp, 0);
	}
}

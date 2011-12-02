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

#if 0
static const u32 s3c_cpu_clock_table[][3] = {
	{532*MHZ, (0<<ARM_DIV_RATIO_BIT), (3<<HCLK_DIV_RATIO_BIT)},
	{266*MHZ, (1<<ARM_DIV_RATIO_BIT), (1<<HCLK_DIV_RATIO_BIT)},
	{133*MHZ, (3<<ARM_DIV_RATIO_BIT), (0<<HCLK_DIV_RATIO_BIT)},
	//{APLL, DIVarm, DIVhclk}	
};
#endif

#ifndef MUXD0D1_A2M
/* ARMCLK, D0CLK, P0CLK, D1CLK, P1CLK, APLL_RATIO, D0CLK_RATIO, P0CLK_RATIO, D1CLK_RATIO, P1CLK_RATIO */
static const u32 s5p_cpu_clk_tab_533MHz[][10] = {
	{533*MHZ, 133*MHZ, (133*MHZ)/2, 133*MHZ, (133*MHZ)/2, 0, 0, 1, 0, 1},
	{(533*MHZ)/2, 133*MHZ, (133*MHZ)/2, 133*MHZ, (133*MHZ)/2, 1, 0, 1, 0, 1},
	{(533*MHZ)/3, 133*MHZ, (133*MHZ)/2, 133*MHZ, (133*MHZ)/2, 2, 0, 1, 0, 1},
	{(533*MHZ)/4, 133*MHZ, (133*MHZ)/2, 133*MHZ, (133*MHZ)/2, 3, 0, 1, 0, 1},
#ifdef SYSCLK_CHANGE
#ifdef USE_DVFS_AL1_LEVEL
	{(533*MHZ)/4, (133*MHZ), (133*MHZ)/2, 133*MHZ/2, (133*MHZ)/2, 3, 0, 1, 1, 0},
#endif /* USE_DVFS_AL1_LEVEL */
	{(533*MHZ)/8, (133*MHZ)/2, (133*MHZ)/2, 133*MHZ/2, (133*MHZ)/2, 7, 1, 0, 1, 0},
#endif /* SYSCLK_CHANGE */
};
#else
/* ARMCLK, D0CLK, P0CLK, D1CLK, A2M_RATIO, APLL_RATIO, D0CLK_RATIO, P0CLK_RATIO, D1CLK_RATIO, P1CLK_RATIO */
static const u32 s5p_cpu_clk_tab_533MHz[][10] = {
	{533*MHZ, 		133*MHZ, 	(133*MHZ)/2, 	133*MHZ, 	3, 0, 0, 1, 0, 1},
	{(533*MHZ)/2, 	133*MHZ, 	(133*MHZ)/2, 	133*MHZ, 	3, 1, 0, 1, 0, 1},
	{(533*MHZ)/3, 	133*MHZ, 	(133*MHZ)/2, 	133*MHZ, 	3, 2, 0, 1, 0, 1},
	{(533*MHZ)/4, 	133*MHZ, 	(133*MHZ)/2, 	133*MHZ, 	3, 3, 0, 1, 0, 1},
#ifdef SYSCLK_CHANGE
#ifdef USE_DVFS_AL1_LEVEL
	{(533*MHZ)/4, 	(133*MHZ)/2, (133*MHZ)/2, 	(133*MHZ)/2, 7, 3, 0, 0, 0, 0},
#endif /* USE_DVFS_AL1_LEVEL */
	{(533*MHZ)/8,	 (133*MHZ)/2, (133*MHZ)/2, 	(133*MHZ)/2, 7, 7, 0, 0, 0, 0},
#endif /* SYSCLK_CHANGE */
};
#endif /* MUXD0D1_A2M */
#if 0
/* ARMCLK, D0CLK, P0CLK, D1CLK, P1CLK, APLL_RATIO, A2M_RATIO, REFRESH_RATE, UART_DIV, RESERVED */
static const u32 s5p_cpu_clk_tab_666MHz[][10] = {
	{667*MHZ,     166*MHZ, (166*MHZ)/2, 166*MHZ, (166*MHZ)/2, 	  0, 3, AVG_PRD_REFRESH_INTERVAL_166MHZ, 0x2C, 1},
	{(667*MHZ)/2, 166*MHZ, (166*MHZ)/2, 166*MHZ, (166*MHZ)/2,         1, 3, AVG_PRD_REFRESH_INTERVAL_166MHZ, 0x2C, 1},
//	{(667*MHZ)/2, 133*MHZ, (133*MHZ)/2, 133*MHZ, (133*MHZ)/2, 	  1, 4, AVG_PRD_REFRESH_INTERVAL_133MHZ, 0x23, 1},
	{(667*MHZ)/4, 133*MHZ, (133*MHZ)/2, 133*MHZ, (133*MHZ)/2, 	  3, 4, AVG_PRD_REFRESH_INTERVAL_133MHZ, 0x23, 1},
#ifdef SYSCLK_CHANGE
	{(667*MHZ)/4, (166*MHZ)/2, (166*MHZ)/4, (166*MHZ)/2, (166*MHZ)/4, 3, 7, AVG_PRD_REFRESH_INTERVAL_83MHZ, 0x16, 1},
        {(667*MHZ)/8, (166*MHZ)/2, (166*MHZ)/4, (166*MHZ)/2, (166*MHZ)/4, 7, 7, AVG_PRD_REFRESH_INTERVAL_83MHZ, 0x16, 1},
//	{(667*MHZ)/4, (166*MHZ)/2, (166*MHZ)/2, (166*MHZ)/2, (166*MHZ)/2, 3, 7, AVG_PRD_REFRESH_INTERVAL_83MHZ, 0x2C, 0},
//        {(667*MHZ)/8, (166*MHZ)/2, (166*MHZ)/2, (166*MHZ)/2, (166*MHZ)/2, 7, 7, AVG_PRD_REFRESH_INTERVAL_83MHZ, 0x2C, 0},
//	{(667*MHZ)/4, (133*MHZ)/2, (133*MHZ)/2, (133*MHZ)/2, (133*MHZ)/2, 3, 9, AVG_PRD_REFRESH_INTERVAL_66MHZ, 0x23, 0},
//      {(667*MHZ)/8, (133*MHZ)/2, (133*MHZ)/2, (133*MHZ)/2, (133*MHZ)/2, 7, 9, AVG_PRD_REFRESH_INTERVAL_66MHZ, 0x23, 0},
//	{(667*MHZ)/4, (133*MHZ)/2, (133*MHZ)/4, (133*MHZ)/2, (133*MHZ)/4, 3, 9, AVG_PRD_REFRESH_INTERVAL_66MHZ, 0x12, 1},
//	{(667*MHZ)/8, (133*MHZ)/2, (133*MHZ)/4, (133*MHZ)/2, (133*MHZ)/4, 7, 9, AVG_PRD_REFRESH_INTERVAL_66MHZ, 0x12, 1},
#endif /* SYSCLK_CHANGE */
};
#else
/* ARMCLK, D0CLK, P0CLK, //D1CLK, P1CLK, // APLL_RATIO, D0CLK_RATIO, P0CLK_RATIO, D1CLK_RATIO, P1CLK_RATIO, REFRESH_RATE, UART_DIV, RESERVED */
static const u32 s5p_cpu_clk_tab_666MHz[][10] = {
        {667*MHZ,     166*MHZ, (166*MHZ)/2, /*133*MHZ, (133*MHZ)/2,*/ 0, 3, 1, 4, 1, AVG_PRD_REFRESH_INTERVAL_166MHZ, 0x23},
        {(667*MHZ)/2, 166*MHZ, (166*MHZ)/2, /*133*MHZ, (133*MHZ)/2,*/ 1, 3, 1, 4, 1, AVG_PRD_REFRESH_INTERVAL_166MHZ, 0x23},
//	{(667*MHZ)/3, 133*MHZ, (133*MHZ)/2, /*133*MHZ, (133*MHZ)/2,*/ 2, 3, 1, 3, 1, AVG_PRD_REFRESH_INTERVAL_166MHZ, 0x2C},
//	{(667*MHZ)/4, 133*MHZ, (133*MHZ)/2, /*133*MHZ, (133*MHZ)/2,*/ 3, 3, 1, 3, 1, AVG_PRD_REFRESH_INTERVAL_166MHZ, 0x2C},
        {(667*MHZ)/3, 133*MHZ, (133*MHZ)/2, /*133*MHZ, (133*MHZ)/2,*/ 2, 4, 1, 4, 1, AVG_PRD_REFRESH_INTERVAL_133MHZ, 0x23},
        {(667*MHZ)/4, 133*MHZ, (133*MHZ)/2, /*133*MHZ, (133*MHZ)/2,*/ 3, 4, 1, 4, 1, AVG_PRD_REFRESH_INTERVAL_133MHZ, 0x23},
#ifdef SYSCLK_CHANGE
//	{(667*MHZ)/4, (133*MHZ)/2, (133*MHZ)/2, /*133*MHZ, (133*MHZ)/2,*/ 3, 7, 0, 7, 0, AVG_PRD_REFRESH_INTERVAL_83MHZ, 0x2C},
//      {(667*MHZ)/8, (133*MHZ)/2, (133*MHZ)/2,   /*133*MHZ, (133*MHZ)/2,*/ 7, 7, 0, 7, 0, AVG_PRD_REFRESH_INTERVAL_83MHZ, 0x2C},
	{(667*MHZ)/4, (133*MHZ)/2, (133*MHZ)/2, /*133*MHZ, (133*MHZ)/2,*/ 3, 9, 0, 9, 0, AVG_PRD_REFRESH_INTERVAL_66MHZ, 0x23},
        {(667*MHZ)/8, (133*MHZ)/2, (133*MHZ)/2,   /*133*MHZ, (133*MHZ)/2,*/ 7, 9, 0, 9, 0, AVG_PRD_REFRESH_INTERVAL_66MHZ, 0x23},
#endif /* SYSCLK_CHANGE */
};

#endif
/* ARMCLK, D0CLK, P0CLK, D1CLK, P1CLK, APLL_RATIO, D0CLK_RATIO, P0CLK_RATIO, D1CLK_RATIO, P1CLK_RATIO */
static const u32 s5p_cpu_clk_tab_666_166MHz[][10] = {
        {667*MHZ,         166*MHZ, (166*MHZ)/2, 166*MHZ, (166*MHZ)/2, 0, 0, 1, 0, 1},
        {(667*MHZ)/2, 166*MHZ, (166*MHZ)/2, 166*MHZ, (166*MHZ)/2, 1, 0, 1, 0, 1},
        {(667*MHZ)/3, 166*MHZ, (166*MHZ)/2, 166*MHZ, (166*MHZ)/2, 2, 0, 1, 0, 1},
        {(667*MHZ)/4, 166*MHZ, (166*MHZ)/2, 166*MHZ, (166*MHZ)/2, 3, 0, 1, 0, 1},
#ifdef SYSCLK_CHANGE
#ifdef USE_DVFS_AL1_LEVEL
        {(667*MHZ)/4, (166*MHZ)/2, (166*MHZ)/2, 166*MHZ/2, (166*MHZ)/2, 3, 1, 0, 1, 0},
#endif /* USE_DVFS_AL1_LEVEL */
	{(667*MHZ)/8, (166*MHZ)/2, (166*MHZ)/2, 166*MHZ/2, (166*MHZ)/2, 7, 1, 0, 1, 0},   
#endif /* SYSCLK_CHANGE */
};

#define GET_DIV(clk, field) ((((clk) & field##_MASK) >> field##_SHIFT) + 1)

static const u32 s5p_avg_prd_refresh_rate[6] = { AVG_PRD_REFRESH_INTERVAL_133MHZ, AVG_PRD_REFRESH_INTERVAL_66MHZ, AVG_PRD_REFRESH_INTERVAL_133MHZ,
						AVG_PRD_REFRESH_INTERVAL_66MHZ, AVG_PRD_REFRESH_INTERVAL_166MHZ, AVG_PRD_REFRESH_INTERVAL_83MHZ, };
unsigned int S5P6442_FREQ_TAB = 0;

static const u32 (*s5p_cpu_clk_tab[3])[10] = {
	s5p_cpu_clk_tab_533MHz,
	s5p_cpu_clk_tab_666MHz,
	s5p_cpu_clk_tab_666_166MHz,
} ;

static u32 s5p_cpu_clk_tab_size(void)
{
	u32 size;
	if(S5P6442_FREQ_TAB)
		size = ARRAY_SIZE(s5p_cpu_clk_tab_666MHz);
	else
		size = ARRAY_SIZE(s5p_cpu_clk_tab_533MHz);

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
	static const u32 (*cpu_clk_tab)[6];

	cpu_clk_tab = s5p_cpu_clk_tab[S5P6442_FREQ_TAB];

	size = s5p_cpu_clk_tab_size();

	for(iter = 1 ; iter < size ; iter++) {
		if(rate > s5p_cpu_clk_tab[S5P6442_FREQ_TAB][iter][0])
			return s5p_cpu_clk_tab[S5P6442_FREQ_TAB][iter-1][0];
	}

	return s5p_cpu_clk_tab[S5P6442_FREQ_TAB][iter - 1][0];
}

int s5p6442_get_index(void)
{
	unsigned long clk_div0;
	unsigned long armClkRatio;
	unsigned int d0ClkRatio;	
	unsigned int d1ClkRatio;
	unsigned int p0ClkRatio;	
	unsigned int p1ClkRatio;
	unsigned int a2MClkRatio;
	int index = 0;
	u32 size;
	u32 (*cpu_clk_tab)[10];

	cpu_clk_tab = s5p_cpu_clk_tab[S5P6442_FREQ_TAB];

	clk_div0 = __raw_readl(S5P_CLK_DIV0);
	if(S5P6442_FREQ_TAB == 1){
		
	armClkRatio = GET_DIV(clk_div0, S5P_CLKDIV0_APLL)- 1;
        d0ClkRatio = GET_DIV(clk_div0, S5P_CLKDIV0_D0CLK) - 1;
        d1ClkRatio = GET_DIV(clk_div0, S5P_CLKDIV0_D1CLK) - 1;
	p0ClkRatio = GET_DIV(clk_div0, S5P_CLKDIV0_P0CLK) - 1;
        p1ClkRatio = GET_DIV(clk_div0, S5P_CLKDIV0_P1CLK) - 1;
	size = s5p_cpu_clk_tab_size();
	 for(index = 0 ; index < size ; index++) {
                if((cpu_clk_tab[index][3]== armClkRatio)&&
			(cpu_clk_tab[index][4] == d0ClkRatio) &&
                           (cpu_clk_tab[index][5] == p0ClkRatio) &&
                           (cpu_clk_tab[index][6] == d1ClkRatio) &&
                           (cpu_clk_tab[index][7] == p1ClkRatio)){
			
			return index;
		}
	   }	
	}
	else {
	armClkRatio = GET_DIV(clk_div0, S5P_CLKDIV0_APLL)- 1;
	d0ClkRatio = GET_DIV(clk_div0, S5P_CLKDIV0_D0CLK) - 1;
	d1ClkRatio = GET_DIV(clk_div0, S5P_CLKDIV0_D1CLK) - 1;
	p0ClkRatio = GET_DIV(clk_div0, S5P_CLKDIV0_P0CLK) - 1;
        p1ClkRatio = GET_DIV(clk_div0, S5P_CLKDIV0_P1CLK) - 1;

	size = s5p_cpu_clk_tab_size();
	for(index = 0 ; index < size ; index++) {
		if(cpu_clk_tab[index][5]== armClkRatio) {
			if((cpu_clk_tab[index][6] == d0ClkRatio) &&
			   (cpu_clk_tab[index][7] == p0ClkRatio) &&
			   (cpu_clk_tab[index][8] == d1ClkRatio) &&
			   (cpu_clk_tab[index][9] == p1ClkRatio))
				return index;
		}
	}
	}

	return -1;
}

#ifdef CONFIG_CPU_FREQ
int s5p6442_clk_set_rate(unsigned int target_freq,
                                unsigned int index )
{
	int cur_freq;
	int cur_idx;
	unsigned int mask;
	u32 clk_div0;
	u32 size;
	int timeout = 1000; //10 msec //10 usec uints
	static int flag = 0;
        static unsigned int hd0_clk;
        static unsigned int hd1_clk;
        static unsigned int arm_clk;
	static const u32 (*cpu_clk_tab)[10];

	if (!flag){
		hd0_clk = clk_hd0.rate;
		hd1_clk = clk_hd1.rate;
		arm_clk = clk_f.rate;
		flag = 1;
	}

	cpu_clk_tab = s5p_cpu_clk_tab[S5P6442_FREQ_TAB];

	size = s5p_cpu_clk_tab_size();

	if(index >= size) {
		return 1;
	}
	
	cur_idx = s5p6442_get_index();
	if(cur_idx < 0) {
		printk("%s: cpu_clk_tab: problem!!!!!\n", __func__);
		return 1;
	}

	if(cur_idx == index)
		return 0;

	if(S5P6442_FREQ_TAB == 1){
		
		if(cur_idx > index){ // incresing frequency

		__raw_writel(cpu_clk_tab[index][8], S5P_DRAMC_TIMINGAREF);
//		__raw_writel(cpu_clk_tab[index][9], S3C24XX_VA_UART1 + S3C2410_UBRDIV);

		 mask = (~S5P_CLKDIV0_APLL_MASK) & (~S5P_CLKDIV0_D0CLK_MASK) & (~S5P_CLKDIV0_P0CLK_MASK) & (~S5P_CLKDIV0_D1CLK_MASK) & (~S5P_CLKDIV0_P1CLK_MASK);
                clk_div0 = __raw_readl(S5P_CLK_DIV0) & mask;
                clk_div0 |= cpu_clk_tab[index][3] << S5P_CLKDIV0_APLL_SHIFT;
		clk_div0 |= (cpu_clk_tab[index][4]) << S5P_CLKDIV0_D0CLK_SHIFT;
		clk_div0 |= (cpu_clk_tab[index][5]) << S5P_CLKDIV0_P0CLK_SHIFT;
		clk_div0 |= (cpu_clk_tab[index][6]) << S5P_CLKDIV0_D1CLK_SHIFT;
                clk_div0 |= (cpu_clk_tab[index][7]) << S5P_CLKDIV0_P1CLK_SHIFT;
		s5p6442_changeDivider(clk_div0, S5P_CLK_DIV0);
	
		}
		else{
		
		mask = (~S5P_CLKDIV0_APLL_MASK) & (~S5P_CLKDIV0_D0CLK_MASK) & (~S5P_CLKDIV0_P0CLK_MASK) & (~S5P_CLKDIV0_D1CLK_MASK) & (~S5P_CLKDIV0_P1CLK_MASK);
                clk_div0 = __raw_readl(S5P_CLK_DIV0) & mask;
		clk_div0 |= cpu_clk_tab[index][3] << S5P_CLKDIV0_APLL_SHIFT;
                clk_div0 |= (cpu_clk_tab[index][4]) << S5P_CLKDIV0_D0CLK_SHIFT;
                clk_div0 |= (cpu_clk_tab[index][5]) << S5P_CLKDIV0_P0CLK_SHIFT;
                clk_div0 |= (cpu_clk_tab[index][6]) << S5P_CLKDIV0_D1CLK_SHIFT;
                clk_div0 |= (cpu_clk_tab[index][7]) << S5P_CLKDIV0_P1CLK_SHIFT;
                s5p6442_changeDivider(clk_div0, S5P_CLK_DIV0);
	
		 __raw_writel(cpu_clk_tab[index][8], S5P_DRAMC_TIMINGAREF);
  //               __raw_writel(cpu_clk_tab[index][9], S3C24XX_VA_UART1 + S3C2410_UBRDIV);

		}
		
		while(__raw_readl(S5P_CLK_DIV_STAT0) && (timeout > 0)){
			timeout--;
			udelay(10);
		}	

#if 1
        clk_f.rate = arm_clk / (cpu_clk_tab[index][3] + 1);
        clk_hd0.rate = (arm_clk / (cpu_clk_tab[index][4] + 1));
        clk_hd1.rate = (arm_clk / (cpu_clk_tab[index][6] + 1));
	clk_pd0.rate = clk_hd0.rate/ (cpu_clk_tab[index][5] + 1);
	clk_pd1.rate = clk_hd1.rate/ (cpu_clk_tab[index][7] + 1);; 

        /* For backward compatibility */
        clk_h.rate = clk_hd1.rate;
        clk_p.rate = clk_pd1.rate;	
//	printk("#####arm_clk %d hd0_clk %d p0_clk %d hd1_clk %d p1_clk %d clkdiv0  %x timeout %d\n", clk_f.rate, clk_hd0.rate, clk_pd0.rate,
//						clk_hd1.rate, clk_pd1.rate, clk_div0, timeout);
#endif
	}
	else{	
	if(cpu_clk_tab[index][6] > 0)
		__raw_writel(s5p_avg_prd_refresh_rate[S5P6442_FREQ_TAB * 2 + 1], S5P_DRAMC_TIMINGAREF);
	
	mask = (~S5P_CLKDIV0_APLL_MASK) & (~S5P_CLKDIV0_D0CLK_MASK) & (~S5P_CLKDIV0_P0CLK_MASK) & (~S5P_CLKDIV0_D1CLK_MASK) & (~S5P_CLKDIV0_P1CLK_MASK);
	clk_div0 = __raw_readl(S5P_CLK_DIV0) & mask;
	clk_div0 |= cpu_clk_tab[index][5] << S5P_CLKDIV0_APLL_SHIFT;
	clk_div0 |= (cpu_clk_tab[index][6]) << S5P_CLKDIV0_D0CLK_SHIFT;
	clk_div0 |= (cpu_clk_tab[index][7]) << S5P_CLKDIV0_P0CLK_SHIFT;
	clk_div0 |= (cpu_clk_tab[index][8]) << S5P_CLKDIV0_D1CLK_SHIFT;
	clk_div0 |= (cpu_clk_tab[index][9]) << S5P_CLKDIV0_P1CLK_SHIFT;
		
#ifdef MUXD0D1_A2M
	clk_div0 &= ~(S5P_CLKDIV0_A2M_MASK );
	clk_div0 |= (cpu_clk_tab[index][4] << S5P_CLKDIV0_A2M_SHIFT);		
#endif /* MUXD0D1_A2M */

	s5p6442_changeDivider(clk_div0, S5P_CLK_DIV0);
	
	if(cpu_clk_tab[index][6] == 0)
		__raw_writel(s5p_avg_prd_refresh_rate[S5P6442_FREQ_TAB * 2], S5P_DRAMC_TIMINGAREF);

	while(__raw_readl(S5P_CLK_DIV_STAT0) && (timeout > 0)){
                        timeout--;
                        udelay(10);
	}
#if 1
        clk_f.rate = arm_clk / (cpu_clk_tab[index][5] + 1);
        clk_hd0.rate = (hd0_clk / (cpu_clk_tab[index][6] + 1));
        clk_hd1.rate = (hd1_clk / (cpu_clk_tab[index][8] + 1));
        clk_pd0.rate = clk_hd0.rate/ (cpu_clk_tab[index][7] + 1);
        clk_pd1.rate = clk_hd1.rate/ (cpu_clk_tab[index][9] + 1);

	/* For backward compatibility */
        clk_h.rate = clk_hd1.rate;
        clk_p.rate = clk_pd1.rate;
 //     printk("#####arm_clk %d hd0_clk %d p0_clk %d hd1_clk %d p1_clk %d clkdiv0  %x timeout %d\n", clk_f.rate, clk_hd0.rate, clk_pd0.rate,
//                                              clk_hd1.rate, clk_pd1.rate, clk_div0, timeout);
#endif
	}
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
		.parent         = &clk_hd1,
		.enable         = s5p64xx_clk_ip0_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP0_MFC,
	}, {
		.name           = "jpeg",
		.id             = -1,
		.parent         = &clk_hd0,
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
		.parent         = &clk_hd1,
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
		.parent         = &clk_hd1,
		.enable         = s5p64xx_clk_ip1_ctrl,
		.ctrlbit        = S5P_CLKGATE_IP1_VP,
	}, {
		.name           = "lcd",	// fimd ?
		.id             = -1,
		.parent         = &clk_hd1,
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
		.parent		= &clk_pd1,
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_SYSTIMER,
	}, {
		.name		= "watchdog",
		.id		= -1,
		.parent		= &clk_pd1,
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_WDT,
	}, {
		.name		= "rtc",
		.id		= -1,
		.parent		= &clk_pd1,
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
		.parent		= &clk_pd1,
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_I2C0,
	}, {
		.name		= "i2c",
		.id		= 1,
		.parent		= &clk_pd1,
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_I2C1,
	}, {
		.name		= "i2c",
		.id		= 2,
		.parent		= &clk_pd1,
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_I2C2,
	}, {
		.name		= "spi",
		.id		= 0,
		.parent		= &clk_pd1,
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_SPI0,
	}, {
		.name		= "timers",
		.id		= -1,
		.parent		= &clk_pd1,
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_PWM,
	}, {
		.name		= "lcd",
		.id		= -1,
		.parent		= &clk_hd1,
		.enable		= s5p64xx_clk_ip1_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP1_FIMD,
	},{
		.name		= "adc",
		.id		= -1,
		.parent		= &clk_pd1,
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_TSADC,
	}, {
		.name		= "keypad",
		.id		= -1,
		.parent		= &clk_pd1,
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_KEYIF,
	},

	/* Audio (IP3) devices */
	{
		.name		= "i2s_v50",
		.id		= 0,
		.parent		= &clk_pd1,
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_I2S0, /* I2S0 is v5.0 */
	},{
		.name		= "i2s_v32",
		.id		= 0,
		.parent		= &clk_p,
		.enable		= s5p64xx_clk_ip3_ctrl,
		.ctrlbit	= S5P_CLKGATE_IP3_I2S1, /* I2S1 is v3.2 */
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
		.parent		= &clk_hd1,
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

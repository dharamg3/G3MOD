/* linux/arch/arm/plat-s5p64xx/s5p6442-clock.c
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 * Copyright 2010 Samsung Electronics Co. Ltd.
 *
 * S5P6442 based common clock support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/sysdev.h>
#include <linux/io.h>
#include <linux/delay.h>

#include <mach/hardware.h>
#include <mach/map.h>
#include <plat/s5p6442-dvfs.h>
#include <plat/cpu-freq.h>
#include <plat/s5p6442.h>
#include <plat/regs-clock.h>
#include <plat/regs-audss.h>
#include <plat/clock.h>
#include <plat/cpu.h>
#include <plat/pll.h>

/* fin_apll, fin_mpll and fin_epll are all the same clock, which we call
 * ext_xtal_mux for want of an actual name from the manual.
*/



struct clk clk_ext_xtal_mux = {
	.name		= "ext_xtal",
	.id		= -1,
	.rate		= XTAL_FREQ,
};

struct clk clk_ext_xtal_usb = {
	.name		= "XusbXTI",
	.id		= -1,
	.rate		= 12000000,
};

struct clk clk_ext_xtal_rtc = {
	.name		= "XrtcXTI",
	.id		= -1,
	.rate		= 32768,
};

#define clk_fin_apll clk_ext_xtal_mux
#define clk_fin_mpll clk_ext_xtal_mux
#define clk_fin_epll clk_ext_xtal_mux
#define clk_fin_vpll clk_ext_xtal_mux

#define clk_fout_mpll	clk_mpll

struct clk_sources {
	unsigned int	nr_sources;
	struct clk	**sources;
};

struct clksrc_clk {
	struct clk		clk;
	unsigned int		mask;
	unsigned int		shift;

	struct clk_sources	*sources;

	unsigned int		divider_shift;
	void __iomem		*reg_divider;
	void __iomem		*reg_source;
};

struct clk clk_srclk = {
	.name		= "srclk",
	.id		= -1,
};

struct clk clk_fout_apll = {
	.name		= "fout_apll",
	.id		= -1,
};

static struct clk *clk_src_apll_list[] = {
	[0] = &clk_fin_apll,
	[1] = &clk_fout_apll,
};

static struct clk_sources clk_src_apll = {
	.sources	= clk_src_apll_list,
	.nr_sources	= ARRAY_SIZE(clk_src_apll_list),
};

struct clksrc_clk clk_mout_apll = {
	.clk	= {
		.name		= "mout_apll",
		.id		= -1,		
	},
	.shift		= S5P_CLKSRC0_APLL_SHIFT,
	.mask		= S5P_CLKSRC0_APLL_MASK,
	.sources	= &clk_src_apll,
	.reg_source	= S5P_CLK_SRC0,
};

struct clk clk_dout_a2m = {
	.name = "dout_a2m",
	.id = -1,
	.parent = &clk_mout_apll.clk,
};

static unsigned long s5p64xx_clk_doutapll_get_rate(struct clk *clk)
{
  	unsigned long rate = clk_get_rate(clk->parent);

	rate /= (((__raw_readl(S5P_CLK_DIV0) & S5P_CLKDIV0_APLL_MASK) >> S5P_CLKDIV0_APLL_SHIFT) + 1);

	return rate;
}

int s5p64xx_clk_doutapll_set_rate(struct clk *clk, unsigned long rate)
{
	struct clk *temp_clk = clk;
	unsigned int div;
	u32 val;

	rate = clk_round_rate(temp_clk, rate);
	div = clk_get_rate(temp_clk->parent) / rate;

	val = __raw_readl(S5P_CLK_DIV0);
	val &=~ S5P_CLKDIV0_APLL_MASK;
	val |= (div - 1) << S5P_CLKDIV0_APLL_SHIFT;
	__raw_writel(val, S5P_CLK_DIV0);

	temp_clk->rate = rate;

	return 0;
}

struct clk clk_dout_apll = {
	.name = "dout_apll",
	.id = -1,
	.parent = &clk_mout_apll.clk,
	.get_rate = s5p64xx_clk_doutapll_get_rate,
	.set_rate = s5p64xx_clk_doutapll_set_rate,
};

static inline struct clksrc_clk *to_clksrc(struct clk *clk)
{
	return container_of(clk, struct clksrc_clk, clk);
}

int fout_enable(struct clk *clk, int enable)
{
	unsigned int ctrlbit = clk->ctrlbit;
	unsigned int epll_con0 = __raw_readl(S5P_EPLL_CON) & ~ ctrlbit;

	if(enable)
	   __raw_writel(epll_con0 | ctrlbit, S5P_EPLL_CON);
	else
	   __raw_writel(epll_con0, S5P_EPLL_CON);

	return 0;
}

unsigned long fout_get_rate(struct clk *clk)
{
	return clk->rate;
}

int fout_set_rate(struct clk *clk, unsigned long rate)
{
	unsigned int epll_con;
	unsigned int epll_con1;
	
	epll_con = __raw_readl(S5P_EPLL_CON);

	epll_con &= ~S5P_EPLLVAL(0x1, 0x1ff, 0x3f, 0x7); /* Clear V, M, P & S */

	epll_con1 = __raw_readl(S5P_EPLL_CON1);

	switch (rate) {
	case 48000000:
		epll_con |= S5P_EPLLVAL(0, 48, 3, 3);
		break;
	case 96000000:
		epll_con |= S5P_EPLLVAL(0, 48, 3, 2);
		break;
	case 144000000:
		epll_con |= S5P_EPLLVAL(1, 72, 3, 2);
		break;
	case 192000000:
		epll_con |= S5P_EPLLVAL(0, 48, 3, 1);
		break;
	case 288000000:
		epll_con |= S5P_EPLLVAL(1, 72, 3, 1);
		break;
	case 32750000:
	case 32768000:
		epll_con |= S5P_EPLLVAL(1, 65, 3, 4);
		epll_con1 = S5P_EPLL_CON1_MASK & 35127;
		break;
	case 45000000:
	case 45158000:
//		epll_con |= S5P_EPLLVAL(0, 45, 3, 3);
//		epll_con1 = S5P_EPLL_CON1_MASK & 10355;
		epll_con |= S5P_EPLLVAL(0, 30, 1, 3);
		epll_con1 = S5P_EPLL_CON1_MASK & 6903;

		break;
	case 49125000:
	case 49152000:
//		epll_con |= S5P_EPLLVAL(0, 49, 3, 3);
//		epll_con1 = S5P_EPLL_CON1_MASK & 9961;
		epll_con |= S5P_EPLLVAL(0, 32, 1, 3);
		epll_con1 = S5P_EPLL_CON1_MASK & 50332;

		break;
	case 67737600:
	case 67738000:
//		epll_con |= S5P_EPLLVAL(1, 67, 3, 3);
//		epll_con1 = S5P_EPLL_CON1_MASK & 48366;
		epll_con |= S5P_EPLLVAL(1, 45, 1, 3);
		epll_con1 = S5P_EPLL_CON1_MASK & 10398;

		break;
	case 73800000:
	case 73728000:
		epll_con |= S5P_EPLLVAL(1, 73, 3, 3);
		epll_con1 = S5P_EPLL_CON1_MASK & 47710;
		break;
	case 36000000:
		epll_con |= S5P_EPLLVAL(1, 72, 3, 4);
		break;
	case 60000000:
		epll_con |= S5P_EPLLVAL(1, 60, 3, 3);
		break;
	case 72000000:
		epll_con |= S5P_EPLLVAL(1, 72, 3, 3);
		break;
	case 80000000:
		epll_con |= S5P_EPLLVAL(1, 80, 3, 3);
		break;
	case 84000000:
		epll_con |= S5P_EPLLVAL(0, 42, 3, 2);
		break;
	case 50000000:
		epll_con |= S5P_EPLLVAL(0, 50, 3, 3);
		break;
	default:
		printk(KERN_ERR "Invalid Clock Freq!\n");
		return -EINVAL;
	}

	__raw_writel(epll_con1, S5P_EPLL_CON1);
	__raw_writel(epll_con, S5P_EPLL_CON);

	clk->rate = rate;


	return 0;
}

struct clk clk_fout_epll = {
	.name		= "fout_epll",
	.id		= -1,
	.ctrlbit	= (1<<31),
	.enable		= fout_enable,
	.get_rate	= fout_get_rate,
	.set_rate	= fout_set_rate,
};

int mout_set_parent(struct clk *clk, struct clk *parent)
{
	int src_nr = -1;
	int ptr;
	u32 clksrc;
	struct clksrc_clk *sclk = to_clksrc(clk);
	struct clk_sources *srcs = sclk->sources;

	clksrc = __raw_readl(S5P_CLK_SRC0);

	for (ptr = 0; ptr < srcs->nr_sources; ptr++)
		if (srcs->sources[ptr] == parent) {
			src_nr = ptr;
			break;
		}

	if (src_nr >= 0) {
		clksrc &= ~sclk->mask;
		clksrc |= src_nr << sclk->shift;
		__raw_writel(clksrc, S5P_CLK_SRC0);
		clk->parent = parent;
		return 0;
	}

	return -EINVAL;
}

static struct clk *clk_src_epll_list[] = {
	[0] = &clk_fin_epll,
	[1] = &clk_fout_epll,
};

static struct clk_sources clk_src_epll = {
	.sources	= clk_src_epll_list,
	.nr_sources	= ARRAY_SIZE(clk_src_epll_list),
};

struct clksrc_clk clk_mout_epll = {
	.clk	= {
		.name		= "mout_epll",
		.id		= -1,
	},
	.shift		= S5P_CLKSRC0_EPLL_SHIFT,
	.mask		= S5P_CLKSRC0_EPLL_MASK,
	.sources	= &clk_src_epll,
	.reg_source	= S5P_CLK_SRC0,
};

struct clk clk_fout_vpll = {
	.name		= "fout_vpll",
	.id		= -1,
};

static struct clk *clk_src_vpll_list[] = {
	[0] = &clk_fin_vpll,
	[1] = &clk_fout_vpll,
};

static struct clk_sources clk_src_vpll = {
	.sources	= clk_src_vpll_list,
	.nr_sources	= ARRAY_SIZE(clk_src_vpll_list),
};

struct clksrc_clk clk_mout_vpll = {
	.clk	= {
		.name		= "mout_vpll",
		.id		= -1,
	},
	.shift		= S5P_CLKSRC0_VPLL_SHIFT,
	.mask		= S5P_CLKSRC0_VPLL_MASK,
	.sources	= &clk_src_vpll,
	.reg_source	= S5P_CLK_SRC0,
};

static struct clk *clk_src_mpll_list[] = {
	[0] = &clk_fin_mpll,
	[1] = &clk_fout_mpll,
};

static struct clk_sources clk_src_mpll = {
	.sources	= clk_src_mpll_list,
	.nr_sources	= ARRAY_SIZE(clk_src_mpll_list),
};

struct clksrc_clk clk_mout_mpll = {
	.clk = {
		.name		= "mout_mpll",
		.id		= -1,
	},
	.shift		= S5P_CLKSRC0_MPLL_SHIFT,
	.mask		= S5P_CLKSRC0_MPLL_MASK,
	.sources	= &clk_src_mpll,
	.reg_source	= S5P_CLK_SRC0,
};


static struct clk *clk_src_muxd0_list[] = {
	[0] = &clk_mout_mpll.clk,
	[1] = &clk_dout_a2m,
};

static struct clk_sources clk_src_muxd0 = {
	.sources	= clk_src_muxd0_list,
	.nr_sources	= ARRAY_SIZE(clk_src_muxd0_list),
};

struct clksrc_clk clk_mout_d0 = {
	.clk = {
		.name		= "mout_d0",
		.id		= -1,
	},
	.shift		= S5P_CLKSRC0_MUXD0_SHIFT,
	.mask		= S5P_CLKSRC0_MUXD0_MASK,
	.sources	= &clk_src_muxd0,
	.reg_source	= S5P_CLK_SRC0,
};

static struct clk *clk_src_muxd1_list[] = {
	[0] = &clk_mout_mpll.clk,
	[1] = &clk_dout_a2m,
};

static struct clk_sources clk_src_muxd1 = {
	.sources	= clk_src_muxd1_list,
	.nr_sources	= ARRAY_SIZE(clk_src_muxd1_list),
};

struct clksrc_clk clk_mout_d1 = {
	.clk = {
		.name		= "mout_d1",
		.id		= -1,
	},
	.shift		= S5P_CLKSRC0_MUXD1_SHIFT,
	.mask		= S5P_CLKSRC0_MUXD1_MASK,
	.sources	= &clk_src_muxd1,
	.reg_source	= S5P_CLK_SRC0,
};

static struct clk *clk_src_muxd0sync_list[] = {
	[0] = &clk_mout_d0.clk,
	[1] = &clk_dout_apll,
};

static struct clk_sources clk_src_muxd0sync = {
	.sources	= clk_src_muxd0sync_list,
	.nr_sources	= ARRAY_SIZE(clk_src_muxd0sync_list),
};

struct clksrc_clk clk_mout_d0sync = {
	.clk = {
		.name		= "mout_d0sync",
		.id		= -1,
	},
	.shift		= S5P_CLKSRC2_MUXD0SYNC_SEL_SHIFT,
	.mask		= S5P_CLKSRC2_MUXD0SYNC_SEL_MASK,
	.sources	= &clk_src_muxd0sync,
	.reg_source	= S5P_CLK_SRC2,
};

static struct clk *clk_src_muxd1sync_list[] = {
	[0] = &clk_mout_d1.clk,
	[1] = &clk_dout_apll,
};

static struct clk_sources clk_src_muxd1sync = {
	.sources	= clk_src_muxd1sync_list,
	.nr_sources	= ARRAY_SIZE(clk_src_muxd1sync_list),
};

struct clksrc_clk clk_mout_d1sync = {
	.clk = {
		.name		= "mout_d1sync",
		.id		= -1,
	},
	.shift		= S5P_CLKSRC2_MUXD1SYNC_SEL_SHIFT,
	.mask		= S5P_CLKSRC2_MUXD1SYNC_SEL_MASK,
	.sources	= &clk_src_muxd1sync,
	.reg_source	= S5P_CLK_SRC2,
};

struct clk clk_dout_d0 = {
	.name = "dout_d0",
	.id = -1,
	.parent = &clk_mout_d0sync.clk,
};

struct clk clk_dout_d1 = {
	.name = "dout_d1",
	.id = -1,
	.parent = &clk_mout_d1sync.clk,
};

static struct clk *clkset_uart_list[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
	NULL,
	NULL,
};

static struct clk_sources clkset_uart = {
	.sources	= clkset_uart_list,
	.nr_sources	= ARRAY_SIZE(clkset_uart_list),
};

static struct clk *clkset_mmc0_list[] = {
	NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    &clk_mout_mpll.clk,
    &clk_mout_epll.clk,
	&clk_mout_vpll.clk,
	&clk_fin_epll,
};

static struct clk *clkset_mmc1_list[] = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    &clk_mout_mpll.clk,
    &clk_mout_epll.clk,
	&clk_mout_vpll.clk,
	&clk_fin_epll,
};

static struct clk *clkset_mmc2_list[] = {
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    &clk_mout_mpll.clk,
    &clk_mout_epll.clk,
	&clk_mout_vpll.clk,
	&clk_fin_epll,
};


static struct clk_sources clkset_mmc0 = {
        .sources        = clkset_mmc0_list,
        .nr_sources     = ARRAY_SIZE(clkset_mmc0_list),
};

static struct clk_sources clkset_mmc1 = {
        .sources        = clkset_mmc1_list,
        .nr_sources     = ARRAY_SIZE(clkset_mmc1_list),
};

static struct clk_sources clkset_mmc2 = {
        .sources        = clkset_mmc2_list,
        .nr_sources     = ARRAY_SIZE(clkset_mmc2_list),
};

static struct clk *clkset_lcd_list[] = {
    NULL,
    NULL,
    NULL,
    &clk_srclk, /*XusbXTI*/
    NULL,
    NULL,
    &clk_mout_mpll.clk,
    &clk_mout_epll.clk,
    &clk_mout_vpll.clk,
};

static struct clk_sources clkset_lcd = {
    .sources    = clkset_lcd_list,
    .nr_sources = ARRAY_SIZE(clkset_lcd_list),
};

static struct clk *clkset_cam0_list[] = {
	NULL,
	NULL,
	NULL,
	&clk_srclk, /*XusbXTI*/
	NULL,
	NULL,	
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
	&clk_mout_vpll.clk,
};

static struct clk *clkset_cam1_list[] = {
	NULL,
	NULL,
	NULL,
	&clk_srclk, /*XusbXTI*/
	NULL,
	NULL,	
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
	&clk_mout_vpll.clk,
};

static struct clk_sources clkset_cam0 = {
	.sources	= clkset_cam0_list,
	.nr_sources	= ARRAY_SIZE(clkset_cam0_list),
};

static struct clk_sources clkset_cam1 = {
	.sources	= clkset_cam1_list,
	.nr_sources	= ARRAY_SIZE(clkset_cam1_list),
};

static struct clk *clkset_fimc0_list[] = {
	NULL,
	NULL,
	NULL,
	&clk_srclk, /*XusbXTI*/
	NULL,
	NULL,	
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
	&clk_mout_vpll.clk,
};

static struct clk *clkset_fimc1_list[] = {
	NULL,
	NULL,
	NULL,
	&clk_srclk, /*XusbXTI*/
	NULL,
	NULL,	
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
	&clk_mout_vpll.clk,
};

static struct clk *clkset_fimc2_list[] = {
	NULL,
	NULL,
	NULL,
	&clk_srclk, /*XusbXTI*/
	NULL,
	NULL,	
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
	&clk_mout_vpll.clk,
};

static struct clk *clkset_g2d_list[] = {
        [0] = &clk_dout_a2m,	
	[1] = &clk_mout_mpll.clk,
	[2] = &clk_mout_epll.clk,
	[3] = &clk_mout_vpll.clk,
};

static struct clk_sources clkset_fimc0 = {
	.sources	= clkset_fimc0_list,
	.nr_sources	= ARRAY_SIZE(clkset_fimc0_list),
};

static struct clk_sources clkset_fimc1 = {
	.sources	= clkset_fimc1_list,
	.nr_sources	= ARRAY_SIZE(clkset_fimc1_list),
};

static struct clk_sources clkset_fimc2 = {
	.sources	= clkset_fimc2_list,
	.nr_sources	= ARRAY_SIZE(clkset_fimc2_list),
};

static struct clk_sources clkset_g2d = {
        .sources        = clkset_g2d_list,
        .nr_sources     = ARRAY_SIZE(clkset_g2d_list),
};


static struct clk *clkset_spi_list[] = {
	NULL,
	NULL,
	NULL,
	&clk_srclk, /*XusbXTI*/
	NULL,
	NULL,
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
	&clk_mout_vpll.clk,
};

static struct clk_sources clkset_spi = {
	.sources	= clkset_spi_list,
	.nr_sources	= ARRAY_SIZE(clkset_spi_list),
};

/* The peripheral clocks are all controlled via clocksource followed
 * by an optional divider and gate stage. We currently roll this into
 * one clock which hides the intermediate clock from the mux.
 *
 * Note, the JPEG clock can only be an even divider...
 *
 * The scaler and LCD clocks depend on the S5P64XX version, and also
 * have a common parent divisor so are not included here.
 */

static unsigned long s5p64xx_getrate_clksrc(struct clk *clk)
{
	struct clksrc_clk *sclk = to_clksrc(clk);
	unsigned long rate = clk_get_rate(clk->parent);
	u32 clkdiv = __raw_readl(sclk->reg_divider);

	clkdiv >>= sclk->divider_shift;
	clkdiv &= 0xf;
	clkdiv++;

	rate /= clkdiv;
	return rate;
}

static int s5p64xx_setrate_clksrc(struct clk *clk, unsigned long rate)
{
	struct clksrc_clk *sclk = to_clksrc(clk);
	void __iomem *reg = sclk->reg_divider;
	unsigned int div;
	u32 val;

	rate = clk_round_rate(clk, rate);
	if (rate) {
		div = clk_get_rate(clk->parent) / rate;

		val = __raw_readl(reg);
		val &= ~(0xf << sclk->divider_shift);
		val |= ((div - 1) << sclk->divider_shift);
		__raw_writel(val, reg);
	}

	return 0;
}

static int s5p64xx_setparent_clksrc(struct clk *clk, struct clk *parent)
{
	struct clksrc_clk *sclk = to_clksrc(clk);
	struct clk_sources *srcs = sclk->sources;
	u32 clksrc = __raw_readl(sclk->reg_source);
	int src_nr = -1;
	int ptr;

	for (ptr = 0; ptr < srcs->nr_sources; ptr++)
		if (srcs->sources[ptr] == parent) {
			src_nr = ptr;
			break;
		}

	if (src_nr >= 0) {
		clk->parent = parent;

		clksrc &= ~sclk->mask;
		clksrc |= src_nr << sclk->shift;

		__raw_writel(clksrc, sclk->reg_source);
		return 0;
	}

	return -EINVAL;
}

static unsigned long s5p64xx_roundrate_clksrc(struct clk *clk,
					      unsigned long rate)
{
	unsigned long parent_rate = clk_get_rate(clk->parent);
	int div;

	if (rate >= parent_rate) 
		rate = parent_rate;
	else {
		div = parent_rate / rate;
		if(parent_rate % rate)
			div++;

		if (div == 0)
			div = 1;
		if (div > 16)
			div = 16;

		rate = parent_rate / div;
	}

	return rate;
}

/* Out CLK for I2S AP Slave */
struct clk clk_oscclk = {
	.name		= "out_dout",
	.id		= -1,
};

/* TODO Map the CLK_OUT sources */
static struct clk *clkset_clk_out_list[] = {
	[0x11] = &clk_oscclk,
};

static struct clk_sources clkset_clk_out = {
	.sources	= clkset_clk_out_list,
	.nr_sources	= ARRAY_SIZE(clkset_clk_out_list),
};

static struct clksrc_clk clk_clk_out = {
	.clk	= {
		.name		= "sclk_audio0",
		.id		= -1,
		.ctrlbit        = S5P_CLKGATE_SCLK0_AUDIO0,
		.enable		= s5p64xx_clk_ip3_ctrl,
		.set_parent	= s5p64xx_setparent_clksrc,
		.get_rate	= s5p64xx_getrate_clksrc,
		.set_rate	= s5p64xx_setrate_clksrc,
		.round_rate	= s5p64xx_roundrate_clksrc,
	},
	.shift		= S5P_CLK_OUT_SHIFT,
	.mask		= S5P_CLK_OUT_MASK,
	.sources	= &clkset_clk_out,
//	.divider_shift	= S5P_CLKDIV6_AUDIO0_SHIFT,
//	.reg_divider	= S5P_CLK_DCLKDIV,
	.reg_source	= S5P_CLK_OUT,
};

/* Audio 0 */
struct clk clk_i2s_XXTI = {
	.name		= "i2s_XXTI",
	.id		= -1,
	.rate		= I2S_XTAL_FREQ,

};

static struct clk *clkset_audio0_list[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
	&clk_mout_vpll.clk,
};

static struct clk_sources clkset_audio0 = {
	.sources	= clkset_audio0_list,
	.nr_sources	= ARRAY_SIZE(clkset_audio0_list),
};

static struct clksrc_clk clk_audio0 = {
	.clk	= {
		.name		= "sclk_audio0",
		.id		= -1,
		.ctrlbit        = S5P_CLKGATE_SCLK0_AUDIO0,
		.enable		= s5p64xx_clk_ip3_ctrl,
		.set_parent	= s5p64xx_setparent_clksrc,
		.get_rate	= s5p64xx_getrate_clksrc,
		.set_rate	= s5p64xx_setrate_clksrc,
		.round_rate	= s5p64xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC6_AUDIO0_SHIFT,
	.mask		= S5P_CLKSRC6_AUDIO0_MASK,
	.sources	= &clkset_audio0,
	.divider_shift	= S5P_CLKDIV6_AUDIO0_SHIFT,
	.reg_divider	= S5P_CLK_DIV6,
	.reg_source	= S5P_CLK_SRC6,
};

static struct clk *clkset_audio1_list[] = {
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	&clk_mout_mpll.clk,
	&clk_mout_epll.clk,
	&clk_mout_vpll.clk,
};

static struct clk_sources clkset_audio1 = {
	.sources	= clkset_audio1_list,
	.nr_sources	= ARRAY_SIZE(clkset_audio0_list),
};

static struct clksrc_clk clk_audio1 = {
	.clk	= {
		.name		= "sclk_audio1",
		.id		= -1,
		.ctrlbit        = S5P_CLKGATE_SCLK0_AUDIO1,
		.enable		= s5p64xx_clk_ip3_ctrl,
		.set_parent	= s5p64xx_setparent_clksrc,
		.get_rate	= s5p64xx_getrate_clksrc,
		.set_rate	= s5p64xx_setrate_clksrc,
		.round_rate	= s5p64xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC6_AUDIO1_SHIFT,
	.mask		= S5P_CLKSRC6_AUDIO1_MASK,
	.sources	= &clkset_audio1,
	.divider_shift	= S5P_CLKDIV6_AUDIO1_SHIFT,
	.reg_divider	= S5P_CLK_DIV6,
	.reg_source	= S5P_CLK_SRC6,
};

#if 1   // 091014 Univ6442 Sound (beta3) :
static struct clk *clkset_i2smain_list[] = {
        [0x0] = NULL, /* XXTI */
        [0x1] = &clk_fout_epll,
};

static struct clk_sources clkset_i2smain_clk = {
        .sources        = clkset_i2smain_list,
        .nr_sources     = ARRAY_SIZE(clkset_i2smain_list),
};

static struct clksrc_clk clk_i2smain = {
        .clk    = {
                .name           = "i2smain_clk", /* C110 calls it Main CLK */
                .id             = -1,
                .set_parent     = s5p64xx_setparent_clksrc,
		//.get_rate       = s5p64xx_getrate_clksrc,
        },
        .shift          = S5P_AUDSS_CLKSRC_MAIN_SHIFT,
        .mask           = S5P_AUDSS_CLKSRC_MAIN_MASK,
        .sources        = &clkset_i2smain_clk,
        .reg_source     = S5P_CLKSRC_AUDSS,
};
#endif // #if 1


/* I2S Releated Clocks */
struct clk clk_i2s_cd0 = {
	.name		= "i2s_cdclk0",
	.id		= -1,
	.rate		= I2S_XTAL_FREQ,
};

// 091014 Univ6442 Sound (beta3)
/* org
static struct clk *clkset_i2sclk_list[] = {
	NULL ,
	[1] = &clk_i2s_cd0,
	[2] = &clk_audio0,
};
*/
static struct clk *clkset_i2sclk_list[] = {
	&clk_i2smain.clk,
	&clk_i2s_cd0,
	&clk_audio0.clk,
};

static struct clk_sources clkset_i2sclk = {
	.sources	= clkset_i2sclk_list,
	.nr_sources	= ARRAY_SIZE(clkset_i2sclk_list),
};

static struct clksrc_clk clk_i2s = {
	.clk	= {
		.name		= "i2sclk",
		.id		= -1,
//		.pd		= &pd_audio,
		.ctrlbit        = S5P_AUDSS_CLKGATE_CLKI2S,
		.enable		= s5p64xx_audss_clkctrl,
		.set_parent	= s5p64xx_setparent_clksrc,
		.get_rate	= s5p64xx_getrate_clksrc,
		.set_rate	= s5p64xx_setrate_clksrc,
		.round_rate	= s5p64xx_roundrate_clksrc,
	},
	.shift		= S5P_AUDSS_CLKSRC_I2SCLK_SHIFT,
	.mask		= S5P_AUDSS_CLKSRC_I2SCLK_MASK,
	.sources	= &clkset_i2sclk,
	.divider_shift	= S5P_AUDSS_CLKDIV_I2SCLK_SHIFT,
	.reg_divider	= S5P_CLKDIV_AUDSS,
	.reg_source	= S5P_CLKSRC_AUDSS,
};


#if 1   // 091014 Univ6442 Sound (beta3) :
static struct clk *clkset_audss_hclk_list[] = {
        &clk_i2smain.clk,
        &clk_i2s_cd0,
};

static struct clk_sources clkset_audss_hclk = {
        .sources        = clkset_audss_hclk_list,
        .nr_sources     = ARRAY_SIZE(clkset_audss_hclk_list),
};

static struct clksrc_clk clk_audss_hclk = {
        .clk    = {
                .name           = "audss_hclk", 
                .id             = -1,
                .ctrlbit        = S5P_AUDSS_CLKGATE_HCLKI2S,
                .enable         = s5p64xx_audss_clkctrl,
                .set_parent     = s5p64xx_setparent_clksrc,
                .get_rate       = s5p64xx_getrate_clksrc,
                .set_rate       = s5p64xx_setrate_clksrc,
                .round_rate     = s5p64xx_roundrate_clksrc,
        },
        .shift          = S5P_AUDSS_CLKSRC_BUSCLK_SHIFT,
        .mask           = S5P_AUDSS_CLKSRC_BUSCLK_MASK,
        .sources        = &clkset_audss_hclk,
        .divider_shift  = S5P_AUDSS_CLKDIV_BUSCLK_SHIFT,
        .reg_divider    = S5P_CLKDIV_AUDSS,
        .reg_source     = S5P_CLKSRC_AUDSS,
};
#endif // #if 0

static struct clksrc_clk clk_mmc0 = {
	.clk	= {
		.name		= "mmc_bus",
		.id		= 0,
		.ctrlbit	= S5P_CLKGATE_IP2_HSMMC0,
		.enable		= s5p64xx_clk_ip2_ctrl,
		.set_parent	= s5p64xx_setparent_clksrc,
		.get_rate	= s5p64xx_getrate_clksrc,
		.set_rate	= s5p64xx_setrate_clksrc,
		.round_rate	= s5p64xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC4_MMC0_SHIFT,
	.mask		= S5P_CLKSRC4_MMC0_MASK,
	.sources	= &clkset_mmc0,
	.divider_shift	= S5P_CLKDIV4_MMC0_SHIFT,
	.reg_divider	= S5P_CLK_DIV4,
	.reg_source 	= S5P_CLK_SRC4,
};

static struct clksrc_clk clk_mmc1 = {
	.clk 	= {
		.name		= "mmc_bus",
		.id		= 1,
		.ctrlbit	= S5P_CLKGATE_IP2_HSMMC1,
		.enable		= s5p64xx_clk_ip2_ctrl,
		.set_parent	= s5p64xx_setparent_clksrc,
		.get_rate	= s5p64xx_getrate_clksrc,
		.set_rate	= s5p64xx_setrate_clksrc,		
		.round_rate	= s5p64xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC4_MMC1_SHIFT,
	.mask		= S5P_CLKSRC4_MMC1_MASK,
	.sources	= &clkset_mmc1,
	.divider_shift	= S5P_CLKDIV4_MMC1_SHIFT,
	.reg_divider	= S5P_CLK_DIV4,
	.reg_source	= S5P_CLK_SRC4,
};

static struct clksrc_clk clk_mmc2 = {
        .clk    = {
		.name		= "mmc_bus",
		.id		= 2,
		.ctrlbit	= S5P_CLKGATE_IP2_HSMMC2,
		.enable		= s5p64xx_clk_ip2_ctrl,
		.set_parent	= s5p64xx_setparent_clksrc,
		.get_rate	= s5p64xx_getrate_clksrc,
		.set_rate	= s5p64xx_setrate_clksrc,
		.round_rate	= s5p64xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC4_MMC2_SHIFT,
	.mask		= S5P_CLKSRC4_MMC2_MASK,
	.sources	= &clkset_mmc2,
	.divider_shift	= S5P_CLKDIV4_MMC2_SHIFT,
	.reg_divider	= S5P_CLK_DIV4,
	.reg_source	= S5P_CLK_SRC4,
};

static struct clksrc_clk clk_uart0 = {
	.clk	= {
		.name		= "sclk_uart",
		.id			= 0,
		.ctrlbit    = S5P_CLKGATE_IP3_UART0,
		.enable		= s5p64xx_clk_ip3_ctrl,
		.set_parent	= s5p64xx_setparent_clksrc,
		.get_rate	= s5p64xx_getrate_clksrc,
		.set_rate	= s5p64xx_setrate_clksrc,
		.round_rate	= s5p64xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC4_UART0_SHIFT,
	.mask		= S5P_CLKSRC4_UART0_MASK,
	.sources	= &clkset_uart,
	.divider_shift	= S5P_CLKDIV4_UART0_SHIFT,
	.reg_divider	= S5P_CLK_DIV4,
	.reg_source	= S5P_CLK_SRC4,
};

static struct clksrc_clk clk_uart1 = {
	.clk	= {
		.name		= "sclk_uart",
		.id			= 1,
		.ctrlbit    = S5P_CLKGATE_IP3_UART1,
		.enable		= s5p64xx_clk_ip3_ctrl,
		.set_parent	= s5p64xx_setparent_clksrc,
		.get_rate	= s5p64xx_getrate_clksrc,
		.set_rate	= s5p64xx_setrate_clksrc,
		.round_rate	= s5p64xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC4_UART1_SHIFT,
	.mask		= S5P_CLKSRC4_UART1_MASK,
	.sources	= &clkset_uart,
	.divider_shift	= S5P_CLKDIV4_UART1_SHIFT,
	.reg_divider	= S5P_CLK_DIV4,
	.reg_source	= S5P_CLK_SRC4,
};

static struct clksrc_clk clk_uart2 = {
	.clk	= {
		.name		= "sclk_uart",
		.id			= 2,
		.ctrlbit    = S5P_CLKGATE_IP3_UART2,
		.enable		= s5p64xx_clk_ip3_ctrl,
		.set_parent	= s5p64xx_setparent_clksrc,
		.get_rate	= s5p64xx_getrate_clksrc,
		.set_rate	= s5p64xx_setrate_clksrc,
		.round_rate	= s5p64xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC4_UART2_SHIFT,
	.mask		= S5P_CLKSRC4_UART2_MASK,
	.sources	= &clkset_uart,
	.divider_shift	= S5P_CLKDIV4_UART2_SHIFT,
	.reg_divider	= S5P_CLK_DIV4,
	.reg_source	= S5P_CLK_SRC4,
};

static struct clksrc_clk clk_spi = {
	.clk	= {
		.name		= "spi-bus",
		.id		= 0,
		.ctrlbit        = S5P_CLKGATE_SCLK0_SPI0,
		.enable		= NULL,
		.set_parent	= s5p64xx_setparent_clksrc,
		.get_rate	= s5p64xx_getrate_clksrc,
		.set_rate	= s5p64xx_setrate_clksrc,
		.round_rate	= s5p64xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC5_SPI0_SHIFT,
	.mask		= S5P_CLKSRC5_SPI0_MASK,
	.sources	= &clkset_spi,
	.divider_shift	= S5P_CLKDIV5_SPI0_SHIFT,
	.reg_divider	= S5P_CLK_DIV5,
	.reg_source	= S5P_CLK_SRC5,
};

static struct clksrc_clk clk_lcd = {
	.clk	= {
		.name		= "sclk_lcd",
		.id		= 0,
		.ctrlbit        = S5P_CLKGATE_IP1_FIMD,
		.enable		= s5p64xx_clk_ip1_ctrl,
		.set_parent	= s5p64xx_setparent_clksrc,
		.get_rate	= s5p64xx_getrate_clksrc,
		.set_rate	= s5p64xx_setrate_clksrc,
		.round_rate	= s5p64xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC1_FIMD_SHIFT,
	.mask		= S5P_CLKSRC1_FIMD_MASK,
	.sources	= &clkset_lcd,
	.divider_shift	= S5P_CLKDIV1_FIMD_SHIFT,
	.reg_divider	= S5P_CLK_DIV1,
	.reg_source	= S5P_CLK_SRC1,
};

static struct clksrc_clk clk_cam0 = {
	.clk	= {
		.name		= "sclk_cam0",
		.id		= -1,
		.ctrlbit        = NULL,
		.enable		= NULL,
		.set_parent	= s5p64xx_setparent_clksrc,
		.get_rate	= s5p64xx_getrate_clksrc,
		.set_rate	= s5p64xx_setrate_clksrc,
		.round_rate	= s5p64xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC1_CAM0_SHIFT,
	.mask		= S5P_CLKSRC1_CAM0_MASK,
	.sources	= &clkset_cam0,
	.divider_shift	= S5P_CLKDIV1_CAM0_SHIFT,
	.reg_divider	= S5P_CLK_DIV1,
	.reg_source	= S5P_CLK_SRC1,
};

static struct clksrc_clk clk_cam1 = {
	.clk	= {
		.name		= "sclk_cam1",
		.id		= -1,
		.ctrlbit        = NULL,
		.enable		= NULL,
		.set_parent	= s5p64xx_setparent_clksrc,
		.get_rate	= s5p64xx_getrate_clksrc,
		.set_rate	= s5p64xx_setrate_clksrc,
		.round_rate	= s5p64xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC1_CAM1_SHIFT,
	.mask		= S5P_CLKSRC1_CAM1_MASK,
	.sources	= &clkset_cam1,
	.divider_shift	= S5P_CLKDIV1_CAM1_SHIFT,
	.reg_divider	= S5P_CLK_DIV1,
	.reg_source	= S5P_CLK_SRC1,
};

static struct clksrc_clk clk_g2d = {
        .clk    = {
                .name           = "clk_g2d",
                .id             = 0,
                .ctrlbit        = S5P_CLKGATE_IP0_G2D,
                .enable         = s5p64xx_clk_ip0_ctrl,
                .set_parent     = s5p64xx_setparent_clksrc,
                .get_rate       = s5p64xx_getrate_clksrc,
                .set_rate       = s5p64xx_setrate_clksrc,
                .round_rate     = s5p64xx_roundrate_clksrc,
        },
        .shift          = S5P_CLKSRC2_G2D_SEL_SHIFT,
        .mask           = S5P_CLKSRC2_G2D_MASK,
        .sources        = &clkset_g2d,
        .divider_shift  = S5P_CLKDIV2_G2D_SHIFT,
        .reg_divider    = S5P_CLK_DIV2,
        .reg_source     = S5P_CLK_SRC2,
};

static struct clksrc_clk clk_fimc0 = {
	.clk	= {
		.name		= "lclk_fimc",
		.id		= 0,
		.ctrlbit        = S5P_CLKGATE_IP0_FIMC0,
		.enable		= s5p64xx_clk_ip0_ctrl,
		.set_parent	= s5p64xx_setparent_clksrc,
		.get_rate	= s5p64xx_getrate_clksrc,
		.set_rate	= s5p64xx_setrate_clksrc,
		.round_rate	= s5p64xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC3_FIMC0_LCLK_SHIFT,
	.mask		= S5P_CLKSRC3_FIMC0_LCLK_MASK,
	.sources	= &clkset_fimc0,
	.divider_shift	= S5P_CLKDIV3_FIMC0_LCLK_SHIFT,
	.reg_divider	= S5P_CLK_DIV3,
	.reg_source	= S5P_CLK_SRC3,
};

static struct clksrc_clk clk_fimc1 = {
	.clk	= {
		.name		= "lclk_fimc",
		.id		= 1,
		.ctrlbit        = S5P_CLKGATE_IP0_FIMC1,
		.enable		= s5p64xx_clk_ip0_ctrl,
		.set_parent	= s5p64xx_setparent_clksrc,
		.get_rate	= s5p64xx_getrate_clksrc,
		.set_rate	= s5p64xx_setrate_clksrc,
		.round_rate	= s5p64xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC3_FIMC1_LCLK_SHIFT,
	.mask		= S5P_CLKSRC3_FIMC1_LCLK_MASK,
	.sources	= &clkset_fimc1,
	.divider_shift	= S5P_CLKDIV3_FIMC1_LCLK_SHIFT,
	.reg_divider	= S5P_CLK_DIV3,
	.reg_source	= S5P_CLK_SRC3,
};

static struct clksrc_clk clk_fimc2 = {
	.clk	= {
		.name		= "lclk_fimc",
		.id		= 2,
		.ctrlbit        = S5P_CLKGATE_IP0_FIMC2,
		.enable		= s5p64xx_clk_ip0_ctrl,
		.set_parent	= s5p64xx_setparent_clksrc,
		.get_rate	= s5p64xx_getrate_clksrc,
		.set_rate	= s5p64xx_setrate_clksrc,
		.round_rate	= s5p64xx_roundrate_clksrc,
	},
	.shift		= S5P_CLKSRC3_FIMC2_LCLK_SHIFT,
	.mask		= S5P_CLKSRC3_FIMC2_LCLK_MASK,
	.sources	= &clkset_fimc2,
	.divider_shift	= S5P_CLKDIV3_FIMC2_LCLK_SHIFT,
	.reg_divider	= S5P_CLK_DIV3,
	.reg_source	= S5P_CLK_SRC3,
};
/* Clock initialisation code */

static struct clksrc_clk *init_parents[] = {
	&clk_mout_apll,
	&clk_mout_epll,
	&clk_mout_mpll,
//	&clk_mout_vpll,
	&clk_mout_d0,
	&clk_mout_d1,
	&clk_mout_d0sync,
	&clk_mout_d1sync,
	&clk_mmc0,
	&clk_mmc1,
	&clk_mmc2,
	&clk_uart0,
	&clk_uart1,
	&clk_uart2,
	&clk_spi,
	&clk_audio0,
	&clk_audio1,
	&clk_lcd,
	&clk_cam0,
	&clk_cam1,
	&clk_fimc0,
	&clk_fimc1,
	&clk_fimc2,
	&clk_g2d,
	&clk_i2s,
	&clk_audss_hclk, // 091014 Univ6442 Sound (beta3) 
        &clk_i2smain,    // 091014 Univ6442 Sound (beta3)     
};

static void __init_or_cpufreq s5p6442_set_clksrc(struct clksrc_clk *clk)
{
	struct clk_sources *srcs = clk->sources;
	u32 clksrc = __raw_readl(clk->reg_source);

	clksrc &= clk->mask;
	clksrc >>= clk->shift;

	if (clksrc > srcs->nr_sources || !srcs->sources[clksrc]) {
		printk(KERN_ERR "%s: bad source %d\n",
		       clk->clk.name, clksrc);
		return;
	}

	clk->clk.parent = srcs->sources[clksrc];

	printk(KERN_INFO "%s: source is %s (%d), rate is %ld\n",
	       clk->clk.name, clk->clk.parent->name, clksrc,
	       clk_get_rate(&clk->clk));
}

#define GET_DIV(clk, field) ((((clk) & field##_MASK) >> field##_SHIFT) + 1)

void __init_or_cpufreq s5p6442_setup_clocks(void)
{
	struct clk *xtal_clk;
	unsigned long xtal;
	unsigned long fclk;
	unsigned long a2m;
	unsigned long hclkd0;
	unsigned long hclkd1;
	unsigned long pclkd0;
	unsigned long pclkd1;
	unsigned long epll;
	unsigned long apll;
	unsigned long mpll;
	unsigned long vpll;
	unsigned int ptr;
	u32 clkdiv0;
	u32 clkdiv3;
	u32 clkSrc0, clkSrc1, clkSrc2;
	u32 mux_stat0;
	u32 mux_stat1;

	printk(KERN_DEBUG "%s: registering clocks\n", __func__);

	clkdiv0 = __raw_readl(S5P_CLK_DIV0);

#ifdef MUXD0D1_A2M
	clkdiv0 &= (~S5P_CLKDIV0_A2M_MASK);
	clkdiv0 |= 3 << S5P_CLKDIV0_A2M_SHIFT;
	__raw_writel(clkdiv0, S5P_CLK_DIV0); 
#endif
	printk(KERN_INFO "%s: clkdiv0 = %08x\n", __func__, clkdiv0);

	clkdiv3 = __raw_readl(S5P_CLK_DIV3);
	printk(KERN_INFO "%s: clkdiv3 = %08x\n", __func__, clkdiv3);

	clkSrc0 = __raw_readl(S5P_CLK_SRC0);
#ifdef MUXD0D1_A2M
	clkSrc0 |= 1 << S5P_CLKSRC0_MUXD0_SHIFT;   //MUXD0
        clkSrc0 |= 1 << S5P_CLKSRC0_MUXD1_SHIFT;   //MUXD1
	__raw_writel(clkSrc0, S5P_CLK_SRC0); 
#endif
	clkSrc1 = __raw_readl(S5P_CLK_SRC1);
	clkSrc2 = __raw_readl(S5P_CLK_SRC2);
	printk(KERN_INFO "%s: clkSrc0 = %08x clkSrc1 = %08x clkSrc2 = %08x\n", __func__, clkSrc0, clkSrc1, clkSrc2);

	xtal_clk = clk_get(NULL, "xtal");
	BUG_ON(IS_ERR(xtal_clk));

	xtal = clk_get_rate(xtal_clk);
	clk_put(xtal_clk);

	printk(KERN_DEBUG "%s: xtal is %ld\n", __func__, xtal);

	apll = s5p64xx_get_pll(xtal, __raw_readl(S5P_APLL_CON), S5P64XX_PLL_APLL);
	mpll = s5p64xx_get_pll(xtal, __raw_readl(S5P_MPLL_CON), S5P64XX_PLL_MPLL);
	epll = s5p64xx_get_pll(xtal, __raw_readl(S5P_EPLL_CON), S5P64XX_PLL_EPLL);
	vpll = s5p64xx_get_pll(xtal, __raw_readl(S5P_VPLL_CON), S5P64XX_PLL_VPLL);

	printk(KERN_INFO "S5P64XX: PLL settings, A=%ld.%ldMHz, M=%ld.%ldMHz," \
							" E=%ld.%ldMHz\n",
	       print_mhz(apll), print_mhz(mpll), print_mhz(epll));

	fclk = apll / GET_DIV(clkdiv0, S5P_CLKDIV0_APLL);

	mux_stat1 = __raw_readl(S5P_CLK_MUX_STAT1);
	mux_stat0 = __raw_readl(S5P_CLK_MUX_STAT0);

	switch ((mux_stat1 & S5P_CLK_MUX_STAT1_MUXD0SYNC_MASK) >> S5P_CLK_MUX_STAT1_MUXD0SYNC_SHIFT) {
	case 0x1:	/* Asynchronous mode */
		switch ((mux_stat0 & S5P_CLK_MUX_STAT0_MUXD0_MASK) >> S5P_CLK_MUX_STAT0_MUXD0_SHIFT) {
		case 0x1:	/* MPLL source */
			hclkd0 = mpll / GET_DIV(clkdiv0, S5P_CLKDIV0_D0CLK);
			pclkd0 = hclkd0 / GET_DIV(clkdiv0, S5P_CLKDIV0_P0CLK);
			break;
		case 0x2:	/* A2M source */
			a2m = apll / GET_DIV(clkdiv0, S5P_CLKDIV0_A2M);
			hclkd0 = a2m / GET_DIV(clkdiv0, S5P_CLKDIV0_D0CLK);
			pclkd0 = hclkd0 / GET_DIV(clkdiv0, S5P_CLKDIV0_P0CLK);
			break;
		default:
			break;
			
		}

		break;

	case 0x2:	/* Synchronous mode */
		hclkd0 = fclk / GET_DIV(clkdiv0, S5P_CLKDIV0_D0CLK);
		pclkd0 = hclkd0 / GET_DIV(clkdiv0, S5P_CLKDIV0_P0CLK);
		break;
	default:
		printk(KERN_ERR "failed to get sync/async mode status register\n");
		break;
		/* Synchronous mode */

	}

	switch ((mux_stat1 & S5P_CLK_MUX_STAT1_MUXD1SYNC_MASK) >> S5P_CLK_MUX_STAT1_MUXD1SYNC_SHIFT) {
        case 0x1:       /* Asynchronous mode */
		switch ((mux_stat0 & S5P_CLK_MUX_STAT0_MUXD1_MASK) >> S5P_CLK_MUX_STAT0_MUXD1_SHIFT) {
		case 0x1:	/* MPLL source */
			hclkd1 = mpll / GET_DIV(clkdiv0, S5P_CLKDIV0_D1CLK);
			pclkd1 = hclkd1 / GET_DIV(clkdiv0, S5P_CLKDIV0_P1CLK);
			break;
		case 0x2:	/* A2M source */
			a2m = apll / GET_DIV(clkdiv0, S5P_CLKDIV0_A2M);
			hclkd1 = a2m / GET_DIV(clkdiv0, S5P_CLKDIV0_D1CLK);
			pclkd1 = hclkd1 / GET_DIV(clkdiv0, S5P_CLKDIV0_P1CLK);
			break;
		default:
			break;
			
		}

		break;

	case 0x2:	/* Synchronous mode */
		hclkd1 = fclk / GET_DIV(clkdiv0, S5P_CLKDIV0_D1CLK);
		pclkd1 = hclkd1 / GET_DIV(clkdiv0, S5P_CLKDIV0_P1CLK);
		break;
	default:
		printk(KERN_ERR "failed to get sync/async mode status register\n");
		break;
		/* Synchronous mode */

	} 

	printk(KERN_INFO "S5P64XX: HCLKD0=%ld.%ldMHz, HCLKD1=%ld.%ldMHz," \
				" PCLKD0=%ld.%ldMHz, PCLKD1=%ld.%ldMHz\n",
			       print_mhz(hclkd0), print_mhz(hclkd1),
			       print_mhz(pclkd0), print_mhz(pclkd1));

	clk_fout_mpll.rate = mpll;
	clk_fout_epll.rate = epll;
	clk_fout_apll.rate = apll;
	clk_mout_vpll.clk.rate = vpll;

	clk_f.rate = fclk;
	clk_hd0.rate = hclkd0;
	clk_pd0.rate = pclkd0;
	clk_hd1.rate = hclkd1;
	clk_pd1.rate = pclkd1;

	clk_srclk.rate = xtal; //24MHz
	
	/* For backward compatibility */
	clk_h.rate = hclkd1;
	clk_p.rate = pclkd1;

	clk_set_parent(&clk_mmc0.clk, &clk_mout_mpll.clk);
	clk_set_parent(&clk_mmc1.clk, &clk_mout_mpll.clk);
	clk_set_parent(&clk_mmc2.clk, &clk_mout_mpll.clk);

	clk_set_parent(&clk_spi.clk, &clk_mout_epll.clk);
	clk_set_parent(&clk_cam0.clk, &clk_mout_mpll.clk);
	clk_set_parent(&clk_cam1.clk, &clk_mout_mpll.clk);
	clk_set_parent(&clk_fimc0.clk, &clk_mout_mpll.clk);
	clk_set_parent(&clk_fimc1.clk, &clk_mout_mpll.clk);
	clk_set_parent(&clk_fimc2.clk, &clk_mout_mpll.clk);

	clk_set_parent(&clk_g2d.clk, &clk_mout_mpll.clk);

	clk_set_parent(&clk_uart0.clk, &clk_mout_mpll.clk);
	clk_set_parent(&clk_uart1.clk, &clk_mout_mpll.clk);
	clk_set_parent(&clk_uart2.clk, &clk_mout_mpll.clk);
	 clk_set_parent(&clk_lcd.clk, &clk_mout_mpll.clk);
	
// 091014 Univ6442 Sound (beta3) : clk_set_parent(&clk_i2s.clk, &clk_i2s_cd0); 
	clk_set_parent(&clk_i2smain.clk, &clk_fout_epll);	
	clk_set_parent(&clk_i2s.clk, &clk_i2smain);

	/* For I2S CLK_OUT 12Mhz */
	clk_set_parent(&clk_clk_out.clk, &clk_oscclk); 

	for (ptr = 0; ptr < ARRAY_SIZE(init_parents); ptr++)
		s5p6442_set_clksrc(init_parents[ptr]);

        clk_set_rate(&clk_mmc0.clk, 50*MHZ);
        clk_set_rate(&clk_mmc1.clk, 50*MHZ);
        clk_set_rate(&clk_mmc2.clk, 50*MHZ);
}

static struct clk *clks[] __initdata = {
	&clk_ext_xtal_mux,
	&clk_ext_xtal_usb,
	&clk_ext_xtal_rtc,
	&clk_mout_epll.clk,
	&clk_fout_epll,
	&clk_mout_mpll.clk,
	&clk_srclk,
	&clk_mmc0.clk,
	&clk_mmc1.clk,	
	&clk_mmc2.clk,
	&clk_uart0.clk,
	&clk_uart1.clk,
	&clk_uart2.clk,
	&clk_spi.clk,
	&clk_audio0.clk,
	&clk_audio1.clk,
	&clk_lcd.clk,
	&clk_cam0.clk,
	&clk_cam1.clk,
	&clk_fimc0.clk,
	&clk_fimc1.clk,
	&clk_fimc2.clk,	
	&clk_g2d.clk,	
        &clk_i2s_cd0,    // 091014 Univ6442 Sound (beta3)	
	&clk_i2s.clk,
        &clk_audss_hclk.clk,  // 091014 Univ6442 Sound (beta3)
        &clk_i2smain.clk,     // 091014 Univ6442 Sound (beta3)
	&clk_i2s_XXTI,
	&clk_clk_out.clk,
	&clk_oscclk,
};

void __init s5p6442_register_clocks(void)
{
	struct clk *clkp;
	int ret;
	int ptr;

	for (ptr = 0; ptr < ARRAY_SIZE(clks); ptr++) {
		clkp = clks[ptr];
		ret = s3c24xx_register_clock(clkp);
		if (ret < 0) {
			printk(KERN_ERR "Failed to register clock %s (%d)\n",
			       clkp->name, ret);
		}
	}

//	clk_mpll.parent = &clk_mout_mpll.clk;
	clk_epll.parent = &clk_mout_epll.clk;
}

/* arch/arm/plat-s5p64xx/include/plat/pll.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * S5P64XX PLL code
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define XTAL_FREQ	16000000

#define S5P64XX_PLL_MDIV_MASK	((1 << (25-16+1)) - 1)
#define S5P64XX_EPLL_MDIV_MASK	((1 << (24-16+1)) - 1)
#define S5P64XX_PLL_PDIV_MASK	((1 << (13-8+1)) - 1)
#define S5P64XX_PLL_SDIV_MASK	((1 << (2-0+1)) - 1)
#define S5P64XX_PLL_MDIV_SHIFT	(16)
#define S5P64XX_PLL_PDIV_SHIFT	(8)
#define S5P64XX_PLL_SDIV_SHIFT	(0)

#include <asm/div64.h>

enum s5p64xx_base_pll_t{
	S5P64XX_PLL_APLL,
	S5P64XX_PLL_MPLL,
	S5P64XX_PLL_EPLL,
	S5P64XX_PLL_VPLL,
};

static inline unsigned long s5p64xx_get_pll(unsigned long baseclk,
					    u32 pllcon, enum s5p64xx_base_pll_t base_pll_type)
{
	u32 mdiv, pdiv, sdiv;
	u64 fvco = baseclk;

	if((base_pll_type == S5P64XX_PLL_EPLL) || (base_pll_type == S5P64XX_PLL_VPLL))
		mdiv = (pllcon >> S5P64XX_PLL_MDIV_SHIFT) & S5P64XX_EPLL_MDIV_MASK;
	else
		mdiv = (pllcon >> S5P64XX_PLL_MDIV_SHIFT) & S5P64XX_PLL_MDIV_MASK;
	
	pdiv = (pllcon >> S5P64XX_PLL_PDIV_SHIFT) & S5P64XX_PLL_PDIV_MASK;
	sdiv = (pllcon >> S5P64XX_PLL_SDIV_SHIFT) & S5P64XX_PLL_SDIV_MASK;

	fvco *= mdiv;

	switch(base_pll_type){
	case S5P64XX_PLL_APLL:

#if (CONFIG_BOARD_REVISION == CONFIG_APOLLO_EMUL)
		do_div(fvco, (pdiv << (sdiv-1)));		//only for 6442 EVT0
#else
		do_div(fvco, (pdiv << (sdiv)));
#endif
		break;
	default:
		do_div(fvco, (pdiv << sdiv));
		break;
	}

	return (unsigned long)fvco;
	
}


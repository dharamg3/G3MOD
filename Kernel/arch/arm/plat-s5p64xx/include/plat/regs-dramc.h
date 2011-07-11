/* arch/arm/plat-s5p64xx/include/plat/regs-dramc.h
 *
 * Copyright (c) 2003 Simtec Electronics <linux@simtec.co.uk>
 *		      http://www.simtec.co.uk/products/SWLINUX/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/


#ifndef ___ASM_ARCH_REGS_DRAMC_H
#define ___ASM_ARCH_REGS_DRAMC_H

#include <plat/map-base.h>

/***************************************************************************/
/* DRAMC Registers for S5P6442 */
#define S5P_DRAMCREG(x)	((x) + S3C_VA_DRAMC)


#define S5P_DRAMC_CONCONTROL		S5P_DRAMCREG(0x0)	
#define S5P_DRAMC_PHYCONTROL0		S5P_DRAMCREG(0x18)	
#define S5P_DRAMC_TIMINGAREF		S5P_DRAMCREG(0x30)	


#define AVG_PRD_REFRESH_INTERVAL_166MHZ	(0x50E)
#define AVG_PRD_REFRESH_INTERVAL_83MHZ (0x288)

#define AVG_PRD_REFRESH_INTERVAL_133MHZ (0x40E)
#define AVG_PRD_REFRESH_INTERVAL_66MHZ	(0x207)
#endif /*  __ASM_ARCH_REGS_ONENAND_H */

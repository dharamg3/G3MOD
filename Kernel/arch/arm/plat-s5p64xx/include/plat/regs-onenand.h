/* arch/arm/plat-s3c/include/plat/regs-onenand.h
 *
 * Copyright (c) 2003 Simtec Electronics <linux@simtec.co.uk>
 *		      http://www.simtec.co.uk/products/SWLINUX/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/


#ifndef ___ASM_ARCH_REGS_ONENAND_H
#define ___ASM_ARCH_REGS_ONENAND_H

#include <plat/map-base.h>

/***************************************************************************/
/* ONENAND Registers for S5P6442 */
#define S5P_ONENANDREG(x)	((x) + S3C_VA_ONENAND)


#define S5P_ONENAND_IF_CTRL		S5P_ONENANDREG(0x100)	
#define S5P_ONENAND_IF_CMD		S5P_ONENANDREG(0x104)	
#define S5P_ONENAND_IF_ASYNC_TIMING_CTRL		S5P_ONENANDREG(0x108)	
#define S5P_ONENAND_IF_STATUS		S5P_ONENANDREG(0x10C)	
#define S5P_DMA_SRC_ADDR	S5P_ONENANDREG(0x400)	
#define S5P_DMA_SRC_CFG	S5P_ONENANDREG(0x404)	
#define S5P_DMA_DST_ADDR	S5P_ONENANDREG(0x408)	
#define S5P_DMA_DST_CFG	S5P_ONENANDREG(0x40C)	
#define S5P_DMA_TRANS_SIZE		S5P_ONENANDREG(0x414)	
#define S5P_DMA_TRANS_CMD		S5P_ONENANDREG(0x418)	
#define S5P_DMA_TRANS_STATUS   	S5P_ONENANDREG(0x41C)	
#define S5P_DMA_TRANS_DIR      	S5P_ONENANDREG(0x420)	
#define S5P_SQC_SAO            	S5P_ONENANDREG(0x600)	
#define S5P_SQC_CMD       	S5P_ONENANDREG(0x608)	
#define S5P_SQC_STATUS       	S5P_ONENANDREG(0x60C)	
#define S5P_SQC_CAO       		S5P_ONENANDREG(0x610)	
#define S5P_SQC_REG_CTRL          	S5P_ONENANDREG(0x614)	
#define S5P_SQC_REG_VAL        	S5P_ONENANDREG(0x618)
#define S5P_SQC_BRPAO0        	S5P_ONENANDREG(0x620)
#define S5P_SQC_BRPAO1       	S5P_ONENANDREG(0x624)
#define S5P_SQC_CLR     	S5P_ONENANDREG(0x1000)	
#define S5P_DMA_CLR  	S5P_ONENANDREG(0x1004)
#define S5P_ONENAND_CLR        	S5P_ONENANDREG(0x1008)
#define S5P_SQC_MASK   	S5P_ONENANDREG(0x1020)
#define S5P_DMA_MASK    	S5P_ONENANDREG(0x1024)	
#define S5P_ONENAND_MASK  	S5P_ONENANDREG(0x1028)	
#define S5P_SQC_PEND     	S5P_ONENANDREG(0x1040)	
#define S5P_DMA_PEND       	S5P_ONENANDREG(0x1044)	
#define S5P_ONENAND_PEND    	S5P_ONENANDREG(0x1048)	
#define S5P_SQC_STATUS    	S5P_ONENANDREG(0x1060)	
#define S5P_DMA_STATUS    	S5P_ONENANDREG(0x1064)	
#define S5P_ONENAND_STATUS 	S5P_ONENANDREG(0x1068)	

#endif /*  __ASM_ARCH_REGS_ONENAND_H */

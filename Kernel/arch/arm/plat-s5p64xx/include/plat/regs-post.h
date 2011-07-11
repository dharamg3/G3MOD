/* linux/arch/arm/plat-s5p64xx/include/plat/regs-post.h
 *
 * Register definition file for Samsung Post Processor (POST) driver
 *
 * Jinsung Yang, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _REGS_POST_H
#define _REGS_POST_H

#define S3C_POSTREG(x) 	(x)

/*************************************************************************
 * Register part
 ************************************************************************/
#define S3C_CIOYSA(__x) 			S3C_POSTREG(0x18 + (__x) * 4)
#define S3C_CIOCBSA(__x) 			S3C_POSTREG(0x28 + (__x) * 4)
#define S3C_CIOCRSA(__x)  			S3C_POSTREG(0x38 + (__x) * 4)

#define S3C_CIGCTRL				S3C_POSTREG(0x08)	/* Global control */
#define S3C_CIOYSA1				S3C_POSTREG(0x18)	/* Y 1st frame start address for output DMA */
#define S3C_CIOYSA2				S3C_POSTREG(0x1c)	/* Y 2nd frame start address for output DMA */
#define S3C_CIOYSA3				S3C_POSTREG(0x20)	/* Y 3rd frame start address for output DMA */
#define S3C_CIOYSA4				S3C_POSTREG(0x24)	/* Y 4th frame start address for output DMA */
#define S3C_CIOCBSA1				S3C_POSTREG(0x28)	/* Cb 1st frame start address for output DMA */
#define S3C_CIOCBSA2				S3C_POSTREG(0x2c)	/* Cb 2nd frame start address for output DMA */
#define S3C_CIOCBSA3				S3C_POSTREG(0x30)	/* Cb 3rd frame start address for output DMA */
#define S3C_CIOCBSA4				S3C_POSTREG(0x34)	/* Cb 4th frame start address for output DMA */
#define S3C_CIOCRSA1				S3C_POSTREG(0x38)	/* Cr 1st frame start address for output DMA */
#define S3C_CIOCRSA2				S3C_POSTREG(0x3c)	/* Cr 2nd frame start address for output DMA */
#define S3C_CIOCRSA3				S3C_POSTREG(0x40)	/* Cr 3rd frame start address for output DMA */
#define S3C_CIOCRSA4				S3C_POSTREG(0x44)	/* Cr 4th frame start address for output DMA */
#define S3C_CITRGFMT				S3C_POSTREG(0x48)	/* Target image format */
#define S3C_CIOCTRL				S3C_POSTREG(0x4c)	/* Output DMA control */
#define S3C_CISCPRERATIO			S3C_POSTREG(0x50)	/* Pre-scaler control 1 */
#define S3C_CISCPREDST				S3C_POSTREG(0x54)	/* Pre-scaler control 2 */
#define S3C_CISCCTRL				S3C_POSTREG(0x58)	/* Main scaler control */
#define S3C_CITAREA				S3C_POSTREG(0x5c)	/* Target area */
#define S3C_CISTATUS				S3C_POSTREG(0x64)	/* Status */
#define S3C_CIIMGCPT				S3C_POSTREG(0xc0)	/* Image capture enable command */
#define S3C_CICPTSEQ				S3C_POSTREG(0xc4)	/* Capture sequence */
#define S3C_CIIMGEFF				S3C_POSTREG(0xd0)	/* Image effects */
#define S3C_CIIYSA0				S3C_POSTREG(0xd4)	/* Y frame start address 0 for input DMA */
#define S3C_CIICBSA0				S3C_POSTREG(0xd8)	/* Cb frame start address 0 for input DMA */
#define S3C_CIICRSA0				S3C_POSTREG(0xdc)	/* Cr frame start address 0 for input DMA */
#define S3C_CIREAL_ISIZE			S3C_POSTREG(0xf8)	/* Real input DMA image size */
#define S3C_MSCTRL				S3C_POSTREG(0xfc)	/* Input DMA control */
#define S3C_CIIYSA1				S3C_POSTREG(0x144)	/* Y frame start address 1 for input DMA */
#define S3C_CIICBSA1				S3C_POSTREG(0x148)	/* Cb frame start address 1 for input DMA */
#define S3C_CIICRSA1				S3C_POSTREG(0x14c)	/* Cr frame start address 1 for input DMA */
#define S3C_CIOYOFF				S3C_POSTREG(0x168)	/* Output DMA Y offset */
#define S3C_CIOCBOFF				S3C_POSTREG(0x16c)	/* Output DMA CB offset */
#define S3C_CIOCROFF				S3C_POSTREG(0x170)	/* Output DMA CR offset */
#define S3C_CIIYOFF				S3C_POSTREG(0x174)	/* Input DMA Y offset */
#define S3C_CIICBOFF				S3C_POSTREG(0x178)	/* Input DMA CB offset */
#define S3C_CIICROFF				S3C_POSTREG(0x17c)	/* Input DMA CR offset */
#define S3C_ORGISIZE				S3C_POSTREG(0x180)	/* Input DMA original image size */
#define S3C_ORGOSIZE				S3C_POSTREG(0x184)	/* Output DMA original image size */
#define S3C_CIEXTEN				S3C_POSTREG(0x188)	/* Real output DMA image size */
#define S3C_CIDMAPARAM				S3C_POSTREG(0x18c)	/* DMA parameter */

/*************************************************************************
 * Macro part
 ************************************************************************/
#define S3C_CITRGFMT_TARGETHSIZE(x)		((x) << 16)
#define S3C_CITRGFMT_TARGETVSIZE(x)		((x) << 0)

#define S3C_CISCPRERATIO_SHFACTOR(x)		((x) << 28)
#define S3C_CISCPRERATIO_PREHORRATIO(x)		((x) << 16)
#define S3C_CISCPRERATIO_PREVERRATIO(x)		((x) << 0)

#define S3C_CISCPREDST_PREDSTWIDTH(x)		((x) << 16)
#define S3C_CISCPREDST_PREDSTHEIGHT(x)		((x) << 0)

#define S3C_CISCCTRL_MAINHORRATIO(x)		((x) << 16)
#define S3C_CISCCTRL_MAINVERRATIO(x)		((x) << 0)

#define S3C_CITAREA_TARGET_AREA(x)		((x) << 0)

#define S3C_CISTATUS_GET_FRAME_COUNT(x)		(((x) >> 26) & 0x3)
#define S3C_CISTATUS_GET_FRAME_END(x)		(((x) >> 17) & 0x1)

#define S3C_CIREAL_ISIZE_HEIGHT(x)		((x) << 16)
#define S3C_CIREAL_ISIZE_WIDTH(x)		((x) << 0)

#define S3C_MSCTRL_SUCCESSIVE_COUNT(x)		((x) << 24)

#define S3C_CIOYOFF_VERTICAL(x)			((x) << 16)
#define S3C_CIOYOFF_HORIZONTAL(x)		((x) << 0)

#define S3C_CIOCBOFF_VERTICAL(x)		((x) << 16)
#define S3C_CIOCBOFF_HORIZONTAL(x)		((x) << 0)

#define S3C_CIOCROFF_VERTICAL(x)		((x) << 16)
#define S3C_CIOCROFF_HORIZONTAL(x)		((x) << 0)

#define S3C_CIIYOFF_VERTICAL(x)			((x) << 16)
#define S3C_CIIYOFF_HORIZONTAL(x)		((x) << 0)

#define S3C_CIICBOFF_VERTICAL(x)		((x) << 16)
#define S3C_CIICBOFF_HORIZONTAL(x)		((x) << 0)

#define S3C_CIICROFF_VERTICAL(x)		((x) << 16)
#define S3C_CIICROFF_HORIZONTAL(x)		((x) << 0)

#define S3C_ORGISIZE_VERTICAL(x)		((x) << 16)
#define S3C_ORGISIZE_HORIZONTAL(x)		((x) << 0)

#define S3C_ORGOSIZE_VERTICAL(x)		((x) << 16)
#define S3C_ORGOSIZE_HORIZONTAL(x)		((x) << 0)


/*************************************************************************
 * Bit definition part
 ************************************************************************/
/* Global control register */
#define S3C_CIGCTRL_SWRST			(1 << 31)
#define S3C_CIGCTRL_IRQ_OVFEN			(1 << 22)
#define S3C_CIGCTRL_IRQ_EDGE			(0 << 20)
#define S3C_CIGCTRL_IRQ_LEVEL			(1 << 20)
#define S3C_CIGCTRL_IRQ_CLR			(1 << 19)
#define S3C_CIGCTRL_IRQ_DISABLE			(0 << 16)
#define S3C_CIGCTRL_IRQ_ENABLE			(1 << 16)

/* Target format register */
#define S3C_CITRGFMT_OUTFORMAT_YCBCR420		(0 << 29)
#define S3C_CITRGFMT_OUTFORMAT_YCBCR422		(1 << 29)
#define S3C_CITRGFMT_OUTFORMAT_YCBCR422_1PLANE	(2 << 29)
#define S3C_CITRGFMT_OUTFORMAT_RGB		(3 << 29)

/* Output DMA control register */
#define S3C_CIOCTRL_ORDER2P_SHIFT		(24)
#define S3C_CIOCTRL_ORDER2P_MASK		(3 << 24)
#define S3C_CIOCTRL_YCBCR_3PLANE		(0 << 3)
#define S3C_CIOCTRL_YCBCR_2PLANE		(1 << 3)
#define S3C_CIOCTRL_YCBCR_PLANE_MASK		(1 << 3)
#define S3C_CIOCTRL_ORDER422_MASK		(3 << 0)

/* Main scaler control register */
#define S3C_CISCCTRL_SCALEUP_H			(1 << 30)
#define S3C_CISCCTRL_SCALEUP_V			(1 << 29)
#define S3C_CISCCTRL_CSCR2Y_NARROW		(0 << 28)
#define S3C_CISCCTRL_CSCR2Y_WIDE		(1 << 28)
#define S3C_CISCCTRL_CSCY2R_NARROW		(0 << 27)
#define S3C_CISCCTRL_CSCY2R_WIDE		(1 << 27)
#define S3C_CISCCTRL_LCDPATHEN_FIFO		(1 << 26)
#define S3C_CISCCTRL_PROGRESSIVE		(0 << 25)
#define S3C_CISCCTRL_INTERLACE			(1 << 25)
#define S3C_CISCCTRL_SCALERSTART		(1 << 15)
#define S3C_CISCCTRL_INRGB_FMT_RGB565		(0 << 13)
#define S3C_CISCCTRL_INRGB_FMT_RGB666		(1 << 13)
#define S3C_CISCCTRL_INRGB_FMT_RGB888		(2 << 13)
#define S3C_CISCCTRL_OUTRGB_FMT_RGB565		(0 << 11)
#define S3C_CISCCTRL_OUTRGB_FMT_RGB666		(1 << 11)
#define S3C_CISCCTRL_OUTRGB_FMT_RGB888		(2 << 11)
#define S3C_CISCCTRL_EXTRGB_NORMAL		(0 << 10)
#define S3C_CISCCTRL_EXTRGB_EXTENSION		(1 << 10)
#define S3C_CISCCTRL_ONE2ONE			(1 << 9)

/* Status register */
#define S3C_CISTATUS_IMGCPTENSC			(1 << 21)
#define S3C_CISTATUS_FRAMEEND			(1 << 17)

/* Image capture enable register */
#define S3C_CIIMGCPT_IMGCPTEN			(1 << 31)
#define S3C_CIIMGCPT_IMGCPTEN_SC		(1 << 30)
#define S3C_CIIMGCPT_CPT_FREN_ENABLE		(1 << 25)
#define S3C_CIIMGCPT_CPT_FRMOD_EN		(0 << 18)
#define S3C_CIIMGCPT_CPT_FRMOD_CNT		(1 << 18)

/* Real input DMA size register */
#define S3C_CIREAL_ISIZE_AUTOLOAD_ENABLE	(1 << 31)
#define S3C_CIREAL_ISIZE_ADDR_CH_DISABLE	(1 << 30)

/* Input DMA control register */
#define S3C_MSCTRL_2PLANE_SHIFT			(16)
#define S3C_MSCTRL_C_INT_IN_3PLANE		(0 << 15)
#define S3C_MSCTRL_C_INT_IN_2PLANE		(1 << 15)
#define S3C_MSCTRL_ORDER422_CRYCBY		(0 << 4)
#define S3C_MSCTRL_ORDER422_YCRYCB		(1 << 4)
#define S3C_MSCTRL_ORDER422_CBYCRY		(2 << 4)
#define S3C_MSCTRL_ORDER422_YCBYCR		(3 << 4)
#define S3C_MSCTRL_INFORMAT_YCBCR420		(0 << 1)
#define S3C_MSCTRL_INFORMAT_YCBCR422		(1 << 1)
#define S3C_MSCTRL_INFORMAT_YCBCR422_1PLANE	(2 << 1)
#define S3C_MSCTRL_INFORMAT_RGB			(3 << 1)
#define S3C_MSCTRL_ENVID			(1 << 0)

/* DMA parameter register */
#define S3C_CIDMAPARAM_R_MODE_LINEAR		(0 << 29)
#define S3C_CIDMAPARAM_R_MODE_CONFTILE		(1 << 29)
#define S3C_CIDMAPARAM_R_MODE_16X16		(2 << 29)
#define S3C_CIDMAPARAM_R_MODE_64X32		(3 << 29)
#define S3C_CIDMAPARAM_R_TILE_HSIZE_64		(0 << 24)
#define S3C_CIDMAPARAM_R_TILE_HSIZE_128		(1 << 24)
#define S3C_CIDMAPARAM_R_TILE_HSIZE_256		(2 << 24)
#define S3C_CIDMAPARAM_R_TILE_HSIZE_512		(3 << 24)
#define S3C_CIDMAPARAM_R_TILE_HSIZE_1024	(4 << 24)
#define S3C_CIDMAPARAM_R_TILE_HSIZE_2048	(5 << 24)
#define S3C_CIDMAPARAM_R_TILE_HSIZE_4096	(6 << 24)
#define S3C_CIDMAPARAM_R_TILE_VSIZE_1		(0 << 20)
#define S3C_CIDMAPARAM_R_TILE_VSIZE_2		(1 << 20)
#define S3C_CIDMAPARAM_R_TILE_VSIZE_4		(2 << 20)
#define S3C_CIDMAPARAM_R_TILE_VSIZE_8		(3 << 20)
#define S3C_CIDMAPARAM_R_TILE_VSIZE_16		(4 << 20)
#define S3C_CIDMAPARAM_R_TILE_VSIZE_32		(5 << 20)
#define S3C_CIDMAPARAM_W_MODE_LINEAR		(0 << 13)
#define S3C_CIDMAPARAM_W_MODE_CONFTILE		(1 << 13)
#define S3C_CIDMAPARAM_W_MODE_16X16		(2 << 13)
#define S3C_CIDMAPARAM_W_MODE_64X32		(3 << 13)
#define S3C_CIDMAPARAM_W_TILE_HSIZE_64		(0 << 8)
#define S3C_CIDMAPARAM_W_TILE_HSIZE_128		(1 << 8)
#define S3C_CIDMAPARAM_W_TILE_HSIZE_256		(2 << 8)
#define S3C_CIDMAPARAM_W_TILE_HSIZE_512		(3 << 8)
#define S3C_CIDMAPARAM_W_TILE_HSIZE_1024	(4 << 8)
#define S3C_CIDMAPARAM_W_TILE_HSIZE_2048	(5 << 8)
#define S3C_CIDMAPARAM_W_TILE_HSIZE_4096	(6 << 8)
#define S3C_CIDMAPARAM_W_TILE_VSIZE_1		(0 << 4)
#define S3C_CIDMAPARAM_W_TILE_VSIZE_2		(1 << 4)
#define S3C_CIDMAPARAM_W_TILE_VSIZE_4		(2 << 4)
#define S3C_CIDMAPARAM_W_TILE_VSIZE_8		(3 << 4)
#define S3C_CIDMAPARAM_W_TILE_VSIZE_16		(4 << 4)
#define S3C_CIDMAPARAM_W_TILE_VSIZE_32		(5 << 4)

#endif /* _REGS_POST_H */

/* arch/arm/plat-s5p6442/include/plat/regs-g2d.h
 *
 * Copyright (c) 2009 Samsung Electronics 
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * g2d Controller
*/

#ifndef __ASM_ARCH_REGS_G2D_H
#define __ASM_ARCH_REGS_G2D_H __FILE__

/*
 * MFC Interface
 */
#define S5P_G2D(x)			(x)

#define S5P_G2D_SOFT_RESET_REG					S5P_G2D(0x000)	/* Software reset register*/  //W
#define S5P_G2D_INTEN_REG							S5P_G2D(0x004)	/* Interrupt Enable register */ //R/W
#define S5P_G2D_RESERVED1							S5P_G2D(0x008)
#define S5P_G2D_INTC_PEND_REG						S5P_G2D(0x00C)	/* Interrupt Control Pending register*/ //R/W
#define S5P_G2D_FIFO_STAT_REG						S5P_G2D(0x010) /* Command FIFO Status register */ //R
#define S5P_G2D_AXI_ID_MODE_REG					S5P_G2D(0x014)	/* AXI Read ID Mode register */
#define S5P_G2D_CACHECTL_REG						S5P_G2D(0x018)	/*Cache & Buffer clear register*/
#define S5P_G2D_RESERVED2							S5P_G2D(0x01C)
#define S5P_G2D_BITBLT_START_REG					S5P_G2D(0x100)  /*BitBLT Start register */
#define S5P_G2D_BITBLT_COMMAND_REG				S5P_G2D(0x104)	/* Command register for BitBLT */
#define S5P_G2D_RESERVED3							S5P_G2D(0x108)  /*   0x108 & 0x10C    */
#define S5P_G2D_ROTATE_REG						S5P_G2D(0x200)	/*Rotation register */
#define S5P_G2D_SRC_MSK_DIRECT_REG				S5P_G2D(0x204)	/* Source and Mask Direction register */
#define S5P_G2D_DST_PAT_DIRECT_REG				S5P_G2D(0x208)  /*Destination and Pattern Direction reg */
#define S5P_G2D_SRC_SELECT_REG					S5P_G2D(0x300)  /* Source Image Selection register */
#define S5P_G2D_SRC_BASE_ADDR_REG				S5P_G2D(0x304)  /* Source Image Base Address register */
#define S5P_G2D_SRC_STRIDE_REG					S5P_G2D(0x308) /* Source Stride register */
#define S5P_G2D_SRC_COLOR_MODE_REG				S5P_G2D(0x30C)	/* Source Image Color Mode register*/
#define S5P_G2D_SRC_LEFT_TOP_REG					S5P_G2D(0x310)	/* Source Left Top Coordinate register */
#define S5P_G2D_SRC_RIGHT_BOTTOM_REG				S5P_G2D(0x314)	/*  Source Right Bottom Coordinate reg*/

#define S5P_G2D_DST_SELECT_REG					S5P_G2D(0x400)  /* Destination Image Selection register*/
#define S5P_G2D_DST_BASE_ADDR_REG				S5P_G2D(0x404)  /* Destination Image Base Address reg */
#define S5P_G2D_DST_STRIDE_REG					S5P_G2D(0x408) /* Destination Stride register */

#define S5P_G2D_DST_COLOR_MODE_REG				S5P_G2D(0x40C)  /*Destination Image Color Mode register*/
#define S5P_G2D_DST_LEFT_TOP_REG					S5P_G2D(0x410)  /*Destination Left Top Coordinate register */
#define S5P_G2D_DST_RIGHT_BOTTOM_REG			S5P_G2D(0x414)  /*Destination Right Bottom Coordinate register */

#define S5P_G2D_PAT_BASE_ADDR_REG				S5P_G2D(0x500)  /*Pattern Image Base Address register */
#define S5P_G2D_PAT_SIZE_REG						S5P_G2D(0x504)  /* Pattern Image Size register */
#define S5P_G2D_PAT_COLOR_MODE_REG				S5P_G2D(0x508) /*Pattern Image Color Mode register */
#define S5P_G2D_PAT_OFFSET_REG					S5P_G2D(0x50C)  /*Pattern Left Top Coordinate register */
#define S5P_G2D_PAT_STRIDE_REG					S5P_G2D(0x510)  /* Pattern Stride register */


#define S5P_G2D_MASK_BASE_ADDR_REG				S5P_G2D(0x520) /*Mask Base Address register */
#define S5P_G2D_MASK_STRIDE_REG					S5P_G2D(0x524) /*Mask Stride register */

#define S5P_G2D_CW_LT_REG							S5P_G2D(0x600)	/*LeftTop coordinates of Clip Window */
#define S5P_G2D_CW_RB_REG							S5P_G2D(0x604)	/*RightBottom coordinates of Clip Window */
#define S5P_G2D_THIRD_OPERAND_REG				S5P_G2D(0x610) /*Third Operand Selection register */
#define S5P_G2D_ROP4_REG							S5P_G2D(0x614)	/*Raster Operation register */
#define S5P_G2D_ALPHA_REG							S5P_G2D(0x618)  /*Alpha value, Fading offset value */

#define S5P_G2D_FG_COLOR_REG						S5P_G2D(0x700)	/*Foreground Color register */
#define S5P_G2D_BG_COLOR_REG						S5P_G2D(0x704)	/*Background Color register */
#define S5P_G2D_BS_COLOR_REG						S5P_G2D(0x708)	/*Blue Screen Color register */

#define S5P_G2D_SRC_COLORKEY_CTRL_REG			S5P_G2D(0x710)	/*Source Colorkey control register */
#define S5P_G2D_SRC_COLORKEY_DR_MIN_REG			S5P_G2D(0x714)  /*Source Colorkey Decision Reference Minimum register */
#define S5P_G2D_SRC_COLORKEY_DR_MAX_REG			S5P_G2D(0x718)  /* Source Colorkey Decision Reference Maximum register */

#define S5P_G2D_DST_COLORKEY_CTRL_REG			S5P_G2D(0x71C) /* Destination Colorkey control register */
#define S5P_G2D_DST_COLORKEY_DR_MIN_REG			S5P_G2D(0x720) /* Destination Colorkey Decision Reference Minimum register */
#define S5P_G2D_DST_COLORKEY_DR_MAX_REG			S5P_G2D(0x724) /* Destination Colorkey Decision Reference Maximum register */



/*************************************************************************
 * Bit definition part
 ************************************************************************/

/* Interrupt Enable register options */
#define G2D_INTEN_RESET_VAL			(0)
#define G2D_INTEN_CF_BITPOS			(0)
#define G2D_INTEN_CF_MASK				(1 << 0)
#define G2D_INTEN_INT_TYPE_BITPOS		(1)
#define G2D_INTEN_INT_TYPE_MASK		(1 << 1)
#define G2D_INTEN_INT_TYPE_EDGE		(1<<1)
#define G2D_INTEN_INT_TYPE_LEVEL		(0<<1)


/* Interrupt Control Pending register options*/
#define G2D_INTC_PEND_RESET_VAL				(0)
#define G2D_INTC_PEND_INTP_CMD_FIN_BITPOS	(0)
#define G2D_INTC_PEND_INTP_CMD_FIN_MASK		(1 << 0)


/* Command FIFO Status register options*/
#define G2D_FIFO_STAT_RESET_VAL			(0x1)
#define G2D_FIFO_STAT_CMD_FIN_BITPOS		(0)


/*  AXI Read ID Mode register options*/
#define G2D_AXI_ID_MODE_BITPOS			(0)
#define G2D_AXI_ID_MODE_MULTI_ID			(1 << 0)
#define G2D_AXI_ID_MODE_SINGLE_ID			(0 << 0)


/* Cache control register Options*/
#define G2D_CACHECTL_MASKBUF_CLR		(1 <<0)
#define G2D_CACHECTL_SRCBUF_CLR		(1 << 1)
#define G2D_CACHECTL_PATCACHE_CLR	(1 << 2)


/* BitBLT Start register Options */
#define G2D_BITBLT_START				(1 << 0)


/* BitBLT Command Register Options */
#define G2D_BITBLT_CMD_MASK_EN				(1 << 0)
#define G2D_BITBLT_CMD_STRETCH_EN			(1 << 4)
#define G2D_BITBLT_CMD_CLIP_WIND				(1 << 8)
#define G2D_BITBLT_CMD_OPAQUE_MODE			(0 << 12)
#define G2D_BITBLT_CMD_TRANSPARENT_MODE		(1 << 12)
#define G2D_BITBLT_CMD_BLUESCR_MODE			(2 << 12)
#define G2D_BITBLT_CMD_TRANS_MODE_MASK  		(3 << 12)
#define G2D_BITBLT_CMD_COLORKEY_MODE_MASK 	(3 << 16)
#define G2D_BITBLT_CMD_DIS_COLORKEY			(0 << 16)
#define G2D_BITBLT_CMD_EN_SRC_COLORKEY		(1 << 16)
#define G2D_BITBLT_CMD_EN_DST_COLORKEY		(2 << 16)
#define G2D_BITBLT_CMD_EN_SRC_DST_COLORKEY	(3 << 16)
#define G2D_BITBLT_CMD_ALPHA_BLEND_MODE_MASK	(3 << 20)
#define G2D_BITBLT_CMD_NO_ALPHA_BLEND			(0 << 20)
#define G2D_BITBLT_CMD_EN_ALPHA_BLEND			(1 << 20)
#define G2D_BITBLT_CMD_EN_FADING					(2 << 20)
#define G2D_BITBLT_CMD_SRC_NON_PREBLEND_MODE_MASK (3 << 22)
#define G2D_BITBLT_CMD_DIS_SRC_NON_PREBLEND		(0 << 22)
#define G2D_BITBLT_CMD_CONST_ALPHA_BLEND		(1 << 22)
#define G2D_BITBLT_CMD_PERPIXEL_ALPHA_BLEND		(2 << 22)
#define G2D_BITBLT_CMD_TRUE_COLOR_EXP			(0 << 24)
#define G2D_BITBLT_CMD_ZERO_COLOR_EXP			(1 << 24)


/*Rotation register Options*/
#define G2D_NO_ROTATION		(0 << 0)
#define G2D_90_ROTATION		(1 << 0)


/*Source and Mask Direction Register Options */
#define G2D_SRC_X_DIRECT_POSITIVE	 (0 << 0)
#define G2D_SRC_X_DIRECT_NEGATIVE	 (1 << 0)

#define G2D_SRC_Y_DIRECT_POSITIVE	 (0 << 1)
#define G2D_SRC_Y_DIRECT_NEGATIVE	 (1 << 1)

#define G2D_MSK_X_DIRECT_POSITIVE	 (0 << 4)
#define G2D_MSK_X_DIRECT_NEGATIVE	(1 << 4)

#define G2D_MSK_Y_DIRECT_POSITIVE		(0 << 5)
#define G2D_MSK_Y_DIRECT_NEGATIVE	(1 << 5)


/*Destination and Pattern Direction reg Options*/
#define G2D_DST_X_DIRECT_POSITIVE	(0 << 0)
#define G2D_DST_X_DIRECT_NEGATIVE	 (1 << 0)

#define G2D_DST_Y_DIRECT_POSITIVE	 (0 << 1)
#define G2D_DST_Y_DIRECT_NEGATIVE	 (1 << 1)

#define G2D_PAT_X_DIRECT_POSITIVE	 (0 << 4)
#define G2D_PAT_X_DIRECT_NEGATIVE 	(1 << 4)

#define G2D_PAT_Y_DIRECT_POSITIVE		(0 << 5)
#define G2D_PAT_Y_DIRECT_NEGATIVE	 (1 << 5)



 /* Source Image Selection register Options*/
#define G2D_SRC_SELECT_MASK				(3 << 0)
#define G2D_SRC_SELECT_NORM_MODE		(0 << 0)
#define G2D_SRC_SELECT_FOREGROUND	(1 << 0)
#define G2D_SRC_SELECT_BACKGROUND	(2 << 0)


/* Source Image Base Address register */
/* [31: 0] */

/* Source Stride register */
/* [15:0] Source stride(2¡¯s complement value) */

/* Source Image Color Mode register OPTIONS */
#define G2D_SRC_COLOR_FMT_MASK		(7 << 0)
#define G2D_SRC_CHAN_ORDER_MASK		(3 << 4)

#define G2D_SRC_COLOR_FMT_XRGB_8888		(0 << 0)
#define G2D_SRC_COLOR_FMT_ARGB_8888		(1 << 0)
#define G2D_SRC_COLOR_FMT_RGB_565		(2 << 0)
#define G2D_SRC_COLOR_FMT_XRGB_1555		(3 << 0)
#define G2D_SRC_COLOR_FMT_ARGB_1555		(4 << 0)
#define G2D_SRC_COLOR_FMT_XRGB_4444		(5 << 0)
#define G2D_SRC_COLOR_FMT_ARGB_4444		(6 << 0)
#define G2D_SRC_COLOR_FMT_PKD_RGB_888	(7 << 0)

#define G2D_SRC_CHAN_ORDER_A_X_RGB		(0 << 4)
#define G2D_SRC_CHAN_ORDER_RGB_A_X		(1 << 4)
#define G2D_SRC_CHAN_ORDER_A_X_BGR		(2 << 4)
#define G2D_SRC_CHAN_ORDER_BGR_A_X		(3 << 4)


/* Source Left Top Coordinate register options*/
#define G2D_SRC_TOP_Y_BITPOS				(16)
#define G2D_SRC_LEFT_X_BITPOS				(0)


/*  Source Right Bottom Coordinate reg*/
#define G2D_SRC_RIGHT_X_BITPOS			(0)
#define G2D_SRC_BOTTOM_Y_BITPOS			(16)


/* Destination Image Selection register Options*/
#define G2D_DST_SELECT_MASK			(3 << 0)
#define G2D_DST_SELECT_NORM_MODE		(0 << 0)
#define G2D_DST_SELECT_FOREGROUND	(1 << 0)
#define G2D_DST_SELECT_BACKGROUND	(2 << 0)

/* Destination Image Base Address register */
/* [31: 0] */

/* Destination Stride register */
/* [15:0] Destination stride(2¡¯s complement value) */


/* Destination Image Color Mode register OPTIONS */
#define G2D_DST_COLOR_FMT_MASK		(7 << 0)
#define G2D_DST_CHAN_ORDER_MASK		(3 << 4)

#define G2D_DST_COLOR_FMT_XRGB_8888		(0 << 0)
#define G2D_DST_COLOR_FMT_ARGB_8888		(1 << 0)
#define G2D_DST_COLOR_FMT_RGB_565		(2 << 0)
#define G2D_DST_COLOR_FMT_XRGB_1555		(3 << 0)
#define G2D_DST_COLOR_FMT_ARGB_1555		(4 << 0)
#define G2D_DST_COLOR_FMT_XRGB_4444		(5 << 0)
#define G2D_DST_COLOR_FMT_ARGB_4444		(6 << 0)
#define G2D_DST_COLOR_FMT_PKD_RGB_888	(7 << 0)

#define G2D_DST_CHAN_ORDER_A_X_RGB		(0 << 4)
#define G2D_DST_CHAN_ORDER_RGB_A_X		(1 << 4)
#define G2D_DST_CHAN_ORDER_A_X_BGR		(2 << 4)
#define G2D_DST_CHAN_ORDER_BGR_A_X		(3 << 4)


/* Destination Left Top Coordinate register options*/
#define G2D_DST_TOP_Y_BITPOS				(16)
#define G2D_DST_LEFT_X_BITPOS				(0)


/*  Destination Right Bottom Coordinate reg*/
#define G2D_DST_RIGHT_X_BITPOS			(0)
#define G2D_DST_BOTTOM_Y_BITPOS			(16)


/* Pattern Image Base Address register */
/* [31: 0] */

/* Pattern Image Size Reg */
#define G2D_PAT_WIDTH_BITPOS				(0)
#define G2D_PAT_HEIGHT_BITPOS				(16)


/* Pattern Image Color Mode register OPTIONS */
#define G2D_PAT_COLOR_FMT_MASK		(7 << 0)
#define G2D_PAT_CHAN_ORDER_MASK		(3 << 4)

#define G2D_PAT_COLOR_FMT_XRGB_8888		(0 << 0)
#define G2D_PAT_COLOR_FMT_ARGB_8888		(1 << 0)
#define G2D_PAT_COLOR_FMT_RGB_565		(2 << 0)
#define G2D_PAT_COLOR_FMT_XRGB_1555		(3 << 0)
#define G2D_PAT_COLOR_FMT_ARGB_1555		(4 << 0)
#define G2D_PAT_COLOR_FMT_XRGB_4444		(5 << 0)
#define G2D_PAT_COLOR_FMT_ARGB_4444		(6 << 0)
#define G2D_PAT_COLOR_FMT_PKD_RGB_888	(7 << 0)

#define G2D_PAT_CHAN_ORDER_A_X_RGB		(0 << 4)
#define G2D_PAT_CHAN_ORDER_RGB_A_X		(1 << 4)
#define G2D_PAT_CHAN_ORDER_A_X_BGR		(2 << 4)
#define G2D_PAT_CHAN_ORDER_BGR_A_X		(3 << 4)


/*  Pattern Offset Register */
#define G2D_PAT_OFFSET_X_BITPOS			(0)
#define G2D_PAT_OFFSET_Y_BITPOS			(16)


/* Pattern Stride Register*/
/* [15:0] Pattern stride(2¡¯s complement value) */


/*LeftTop Clipping Window Register */
#define G2D_CW_TOP_Y_BITPOS				(16)
#define G2D_CW_LEFT_X_BITPOS				(0)


/*  Right Bottom Clipping Window Register */
#define G2D_CW_RIGHT_X_BITPOS			(0)
#define G2D_CW_BOTTOM_Y_BITPOS		(16)


/* Third Operand Selection Register */
#define G2D_UNMASK_SEL_BITMASK		(3 << 0)
#define G2D_UNMASK_SEL_PAT			(0 << 0)
#define G2D_UNMASK_SEL_FOREGROUND     (1 << 0) 
#define G2D_UNMASK_SEL_BACKGROUND     (2 << 0)

#define G2D_MASK_SEL_BITMASK			(3 << 4)
#define G2D_MASK_SEL_PAT				(0 << 4)
#define G2D_MASK_SEL_FOREGROUND	      (1 << 4) 
#define G2D_MASK_SEL_BACKGROUND  		(2 << 4)


/* Raster Operation Register */
#define G2D_ROP4_MASKED_ROP3_BITPOS		(8)
#define G2D_ROP4_UNMASKED_ROP3_BITPOS		(0)


/* Alpha Register */
#define G2D_ALPHA_VAL_BITPOS		(0)
#define G2D_FAD_OFFSET_BITPOS		(8)


/* COLOR KEY: Source Colorkey Control Register */

#define G2D_SRC_CLRKEY_STENCIL_TEST_B_OFF	(0 << 0)
#define G2D_SRC_CLRKEY_STENCIL_TEST_B_ON		(1 << 0)

#define G2D_SRC_CLRKEY_STENCIL_TEST_G_OFF	(0 << 4)
#define G2D_SRC_CLRKEY_STENCIL_TEST_G_ON		(1 << 4)

#define G2D_SRC_CLRKEY_STENCIL_TEST_R_OFF	(0 << 8)
#define G2D_SRC_CLRKEY_STENCIL_TEST_R_ON		(1 << 8)

#define G2D_SRC_CLRKEY_STENCIL_TEST_A_OFF	(0 << 12)
#define G2D_SRC_CLRKEY_STENCIL_TEST_A_ON		(1 << 12)

#define G2D_SRC_CLRKEY_STENCIL_TEST_NORMAL	(0 << 16)
#define G2D_SRC_CLRKEY_STENCIL_TEST_INVERTED (1 << 16)


/* Source Colorkey Decision Reference Minimum Register */
#define G2D_CLRKEY_SRCDR_MIN_B_BITPOS		(0)
#define G2D_CLRKEY_SRCDR_MIN_G_BITPOS		(8)
#define G2D_CLRKEY_SRCDR_MIN_R_BITPOS		(16)
#define G2D_CLRKEY_SRCDR_MIN_A_BITPOS		(24)


/* Source Colorkey Decision Reference Maximum Register */
#define G2D_CLRKEY_SRCDR_MAX_B_BITPOS		(0)
#define G2D_CLRKEY_SRCDR_MAX_G_BITPOS		(8)
#define G2D_CLRKEY_SRCDR_MAX_R_BITPOS		(16)
#define G2D_CLRKEY_SRCDR_MAX_A_BITPOS		(24)


/* COLOR KEY: Destination Colorkey Control Register */


#define G2D_DST_CLRKEY_STENCIL_TEST_B_OFF	(0 << 0)
#define G2D_DST_CLRKEY_STENCIL_TEST_B_ON		(1 << 0)

#define G2D_DST_CLRKEY_STENCIL_TEST_G_OFF	(0 << 4)
#define G2D_DST_CLRKEY_STENCIL_TEST_G_ON		(1 << 4)

#define G2D_DST_CLRKEY_STENCIL_TEST_R_OFF	(0 << 8)
#define G2D_DST_CLRKEY_STENCIL_TEST_R_ON		(1 << 8)

#define G2D_DST_CLRKEY_STENCIL_TEST_A_OFF	(0 << 12)
#define G2D_DST_CLRKEY_STENCIL_TEST_A_ON		(1 << 12)

#define G2D_DST_CLRKEY_STENCIL_TEST_NORMAL	(0 << 16)
#define G2D_DST_CLRKEY_STENCIL_TEST_INVERTED (1 << 16)

/* Destination Colorkey Decision Reference Minimum Register */
#define G2D_CLRKEY_DSTDR_MIN_B_BITPOS		(0)
#define G2D_CLRKEY_DSTDR_MIN_G_BITPOS		(8)
#define G2D_CLRKEY_DSTDR_MIN_R_BITPOS		(16)
#define G2D_CLRKEY_DSTDR_MIN_A_BITPOS		(24)


/* Destination Colorkey Decision Reference Maximum Register */
#define G2D_CLRKEY_DSTDR_MAX_B_BITPOS		(0)
#define G2D_CLRKEY_DSTDR_MAX_G_BITPOS		(8)
#define G2D_CLRKEY_DSTDR_MAX_R_BITPOS		(16)
#define G2D_CLRKEY_DSTDR_MAX_A_BITPOS		(24)

#endif /* __ASM_ARCH_REGS_G2D_H */

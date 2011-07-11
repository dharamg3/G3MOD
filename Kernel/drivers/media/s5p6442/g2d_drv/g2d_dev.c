/* linux/drivers/media/s5p6442/g2d_dev.c
 *
 * Core file for Samsung 2D Graphic accelerator driver
 *
 * rama, Copyright (c) 2009 Samsung Electronics
 @date 2009-12-14
 * 	http://www.samsung.com/sec/
 * 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/


#include <linux/init.h>

#include <linux/moduleparam.h>
#include <linux/timer.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <asm/uaccess.h>
#include <linux/errno.h> /* error codes */
#include <asm/div64.h>
#include <linux/tty.h>
#include <asm/uaccess.h>
#include <linux/miscdevice.h>

#include <linux/version.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/signal.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/kmod.h>
#include <linux/vmalloc.h>
#include <linux/init.h>
#include <linux/semaphore.h>

#include <asm/io.h>
#include <asm/page.h>
#include <asm/irq.h>
#include <linux/mm.h>
#include <linux/moduleparam.h>

#include <mach/hardware.h>
#include <mach/map.h>

//#include <plat/power-clock-domain.h>
#include <plat/pm.h>

#include <plat/regs-g2d.h>
#include "g2d.h"

//#define G2D_DEBUG
//#define USE_G2D_MMAP
#define G2D_CLK_CTRL
#ifdef G2D_CLK_CTRL
#include <plat/regs-clock.h>
DEFINE_SPINLOCK(g2d_clkgate_lock);
#endif
static struct clk *s5p_g2d_clock;
static int s5p_g2d_irq_num = NO_IRQ;
static struct resource *s5p_g2d_mem;
static void __iomem *s5p_g2d_base;
static wait_queue_head_t waitq_g2d;

#define G2D_SIZE 8000

static int          g_num_of_g2d_object;
static int          g_num_of_nonblock_object = 0;


static struct mutex *h_rot_mutex;

static u16 s5p_g2d_poll_flag = 0;

int shift_x,shift_y;

int s5p_g2d_check_fifo_stat_wait(void)
{
    int cnt = 50;
   // 1 = The graphics engine finishes the execution of command.
    // 0 = in the middle of rendering process.
	while((!(__raw_readl(s5p_g2d_base + S5P_G2D_FIFO_STAT_REG) & 0x1)) && (cnt > 0)){
		cnt--;
		msleep_interruptible(2);
	}
	
	if(cnt <= 0){
		__raw_writel(1, s5p_g2d_base +S5P_G2D_FIFO_STAT_REG);
		return -1;
	}

	return 0;
}

void s5p_g2d_soft_reset(void)
{
	__raw_writel(1, s5p_g2d_base + S5P_G2D_SOFT_RESET_REG);  
}

void s5p_g2d_cache_reset(void)
{
	__raw_writel(7, s5p_g2d_base + S5P_G2D_CACHECTL_REG);  
}

void s5p_g2d_IntcEnable(int int_type)   //0 = level, 1 = edge;
{
	int intcval;
	
	intcval = (int_type == 1) ? 1: 0;	
	intcval |= ( 1 << G2D_INTEN_CF_BITPOS);
	__raw_writel(intcval, s5p_g2d_base + S5P_G2D_INTEN_REG);
}

void s5p_g2d_IntcDisable(void)   
{	
	__raw_writel(0, s5p_g2d_base + S5P_G2D_INTEN_REG);
}

void s5p_g2d_IntClear(void)
{
	__raw_writel(1, s5p_g2d_base + S5P_G2D_INTC_PEND_REG);
}


void s5p_g2d_set_bitblt_cmd(u32 mode)
{
	__raw_writel(mode, s5p_g2d_base + S5P_G2D_BITBLT_COMMAND_REG);
}

void s5p_g2d_bitblt_start(void)
{
	s5p_g2d_IntClear();
	s5p_g2d_IntcEnable(0);
	__raw_writel(1, s5p_g2d_base + S5P_G2D_BITBLT_START_REG);
}

#if 0
void s5p_g2d_set_rotate(u32 rot)
{
	rot = (rot == 1)? 1:0;     // 0 = No rotation, 1 = 90 degrees rotation
	__raw_writel(rot, s5p_g2d_base + S5P_G2D_ROTATE_REG);
}


void s5p_g2d_set_SrcMaskDirection(u32 val)
{
	__raw_writel(val & 0x33, s5p_g2d_base +S5P_G2D_SRC_MSK_DIRECT_REG);
}

void s5p_g2d_set_DstPatDirection(u32 val)
{
	__raw_writel(val & 0x33, s5p_g2d_base +S5P_G2D_DST_PAT_DIRECT_REG);
}
#endif

u32 s5p_g2d_GetNumBytesPerPixel(u32 ClrFmt)
{
	u32 NumBytes;

	switch(ClrFmt){
		case G2D_RGBA_8888:
		case G2D_RGBX_8888:
		case G2D_ARGB_8888:
		case G2D_XRGB_8888:
		case G2D_BGRA_8888:
		case G2D_BGRX_8888:
		case G2D_ABGR_8888:
		case G2D_XBGR_8888:
			NumBytes = 4;
			break;
		case G2D_RGB_888:
		case G2D_BGR_888:
			NumBytes = 3;
			break;
		case G2D_RGB_565:
		case G2D_BGR_565:
		case G2D_RGBA_5551:
		case G2D_ARGB_5551:
		case G2D_RGBA_4444:
		case G2D_ARGB_4444:
			NumBytes = 2;
			break;
		default:
			NumBytes = 4;
			printk(KERN_ALERT"s5p g2d dirver error: check color format!!!!!!!!!\n");
			break;		
	}

	return NumBytes;
	
}



u32 s5p_g2d_GetImgClrMode(u32 colorfmt)
{
	u32 ColorMode;

#ifdef G2D_DEBUG 
	printk("##########%s::colorfmt %d\n", __FUNCTION__, colorfmt);
#endif
	
	switch (colorfmt)
	{
		case G2D_RGBA_8888:		
		{
			ColorMode = G2D_SRC_COLOR_FMT_ARGB_8888 | G2D_SRC_CHAN_ORDER_RGB_A_X;
			break;
		}
		case G2D_RGBX_8888:
		{
			ColorMode = G2D_SRC_COLOR_FMT_XRGB_8888 | G2D_SRC_CHAN_ORDER_RGB_A_X;
			break;
		}
		case G2D_ARGB_8888:
		{
			ColorMode = G2D_SRC_COLOR_FMT_ARGB_8888 | G2D_SRC_CHAN_ORDER_A_X_RGB;
			break;
		}
		case G2D_XRGB_8888:
		{
			ColorMode = G2D_SRC_COLOR_FMT_XRGB_8888 | G2D_SRC_CHAN_ORDER_A_X_RGB;
			break;
		}
		case G2D_BGRA_8888:
		{
			ColorMode = G2D_SRC_COLOR_FMT_ARGB_8888 | G2D_SRC_CHAN_ORDER_BGR_A_X;
			break;
		}
		case G2D_BGRX_8888:
		{
			ColorMode = G2D_SRC_COLOR_FMT_XRGB_8888 | G2D_SRC_CHAN_ORDER_BGR_A_X;
			break;
		}
		case G2D_ABGR_8888:
		{
			ColorMode = G2D_SRC_COLOR_FMT_ARGB_8888 | G2D_SRC_CHAN_ORDER_A_X_BGR;
			break;
		}
		case G2D_XBGR_8888:
		{
			ColorMode = G2D_SRC_COLOR_FMT_XRGB_8888 | G2D_SRC_CHAN_ORDER_A_X_BGR;
			break;
		}
		case G2D_RGB_888:
		{
			ColorMode = G2D_SRC_COLOR_FMT_PKD_RGB_888 | G2D_SRC_CHAN_ORDER_A_X_RGB;
			break;
		}
		case G2D_BGR_888:
		{
			ColorMode = G2D_SRC_COLOR_FMT_PKD_RGB_888 | G2D_SRC_CHAN_ORDER_A_X_BGR;
			break;
		}
		case G2D_RGB_565:  
		{
			ColorMode = G2D_SRC_COLOR_FMT_RGB_565 | G2D_SRC_CHAN_ORDER_A_X_RGB;
			break;
		}
		case G2D_BGR_565:  
		{
			ColorMode = G2D_SRC_COLOR_FMT_RGB_565 | G2D_SRC_CHAN_ORDER_A_X_BGR;
			break;
		}
		case G2D_RGBA_5551:
		{
			ColorMode = G2D_SRC_COLOR_FMT_ARGB_1555 | G2D_SRC_CHAN_ORDER_RGB_A_X;
			break;
		}
		case G2D_ARGB_5551:
		{
			ColorMode = G2D_SRC_COLOR_FMT_ARGB_1555 | G2D_SRC_CHAN_ORDER_A_X_RGB;
			break;
		}
		case G2D_RGBA_4444:
		{
			ColorMode = G2D_SRC_COLOR_FMT_ARGB_4444 | G2D_SRC_CHAN_ORDER_RGB_A_X;
			break;
		}
		case G2D_ARGB_4444:
		{
			ColorMode = G2D_SRC_COLOR_FMT_ARGB_4444 | G2D_SRC_CHAN_ORDER_A_X_RGB;
			break;
		}
		default :
			ColorMode = 0;
			printk("%s::not matched color format : %d\n", __func__, ColorMode);
			break;
		}

		return ColorMode;
}
	
int s5p_g2d_clip(int val, int max)
{
	val = (val < 0)? 0: ((val <= max) ? val : max);
	return val;
}

void s5p_g2d_set_SrcImgInfo(s5p_g2d_params *params)
{
	u32 val;
	int left, right, top, bottom;

	/* Source Image Selection */
	__raw_writel(params->src_select & G2D_SRC_SELECT_MASK, s5p_g2d_base +S5P_G2D_SRC_SELECT_REG);

	/*Source Image Base Address */
	__raw_writel(params->src_base_addr , s5p_g2d_base + S5P_G2D_SRC_BASE_ADDR_REG); 

	
	/*Source Stride Register */
	val = s5p_g2d_GetNumBytesPerPixel(params->src_colorfmt);
	__raw_writel(params->src_full_width * val, s5p_g2d_base + S5P_G2D_SRC_STRIDE_REG);

	/*Source Image Color Mode Register */
	val = s5p_g2d_GetImgClrMode(params->src_colorfmt);
	__raw_writel(val, s5p_g2d_base + S5P_G2D_SRC_COLOR_MODE_REG);

	/*Source Left Top Coordinate Register */
	left = s5p_g2d_clip(params->src_start_x, G2D_SIZE);
        top =  s5p_g2d_clip(params->src_start_y, G2D_SIZE);
        val = (top << G2D_DST_TOP_Y_BITPOS)| left;

//	val = (params->src_start_y<<G2D_SRC_TOP_Y_BITPOS)|(params->src_start_x);
	__raw_writel(val, s5p_g2d_base + S5P_G2D_SRC_LEFT_TOP_REG);

	/*Source Right Bottom Coordinate Register */
	right = s5p_g2d_clip((params->src_start_x + params->src_work_width), G2D_SIZE);
        bottom =  s5p_g2d_clip((params->src_start_y + params->src_work_height), G2D_SIZE);

        if(right <= left){
                right = left + 1;
                printk("###%s: right <= left \n", __FUNCTION__);
        }

        if(bottom <= top){
          bottom = top + 1;
           printk("###%s: bottom <= top \n", __FUNCTION__);
        }
        val = (bottom << G2D_DST_BOTTOM_Y_BITPOS) | (right);


	//data = ((params->src_start_y + params->src_work_height - 1)<<16)|((params->src_start_x + params->src_work_width - 1));
//	val = ((params->src_start_y + params->src_work_height - 1)<<G2D_SRC_BOTTOM_Y_BITPOS)|((params->src_start_x + params->src_work_width - 1));
//	val = ((params->src_start_y + params->src_work_height)<<G2D_SRC_BOTTOM_Y_BITPOS)|((params->src_start_x + params->src_work_width));
	__raw_writel(val, s5p_g2d_base + S5P_G2D_SRC_RIGHT_BOTTOM_REG);	
		
}


void s5p_g2d_set_DstImgInfo(s5p_g2d_params *params)
{
	u32 val;
	int left, right, top, bottom;

	/* Destination Image Selection */
	__raw_writel(params->dst_select & G2D_DST_SELECT_MASK, s5p_g2d_base +S5P_G2D_DST_SELECT_REG);

	/*Destination Image Base Address */
	__raw_writel(params->dst_base_addr , s5p_g2d_base + S5P_G2D_DST_BASE_ADDR_REG); 

	
	/*Destination Stride Register */
	val = s5p_g2d_GetNumBytesPerPixel(params->dst_colorfmt);
	
	__raw_writel(params->dst_full_width * val, s5p_g2d_base + S5P_G2D_DST_STRIDE_REG);

	/*Destination Image Color Mode Register */
	val = s5p_g2d_GetImgClrMode(params->dst_colorfmt);
	__raw_writel(val, s5p_g2d_base + S5P_G2D_DST_COLOR_MODE_REG);

	/*Destination Left Top Coordinate Register */
	left = s5p_g2d_clip(params->dst_start_x, G2D_SIZE);
	top =  s5p_g2d_clip(params->dst_start_y, G2D_SIZE);
	val = (top << G2D_DST_TOP_Y_BITPOS)| left;
	//val = (params->dst_start_y<<G2D_DST_TOP_Y_BITPOS)|(params->dst_start_x);
	__raw_writel(val, s5p_g2d_base + S5P_G2D_DST_LEFT_TOP_REG);

	/*Destination Right Bottom Coordinate Register */
	//data = ((params->dst_start_y + params->dst_work_height - 1)<<16)|((params->dst_start_x + params->dst_work_width - 1));
//	val = ((params->dst_start_y + params->dst_work_height - 1)<<G2D_DST_BOTTOM_Y_BITPOS)|((params->dst_start_x + params->dst_work_width - 1));
	right = s5p_g2d_clip((params->dst_start_x + params->dst_work_width), G2D_SIZE);
        bottom =  s5p_g2d_clip((params->dst_start_y + params->dst_work_height), G2D_SIZE);
	
	if(right <= left){	
		right = left + 1;
		printk("###%s: right <= left \n", __FUNCTION__);		
	}

	if(bottom <= top){
	  bottom = top + 1;
	   printk("###%s: bottom <= top \n", __FUNCTION__);
	}	
	val = (bottom << G2D_DST_BOTTOM_Y_BITPOS) | (right);
//	val = ((params->dst_start_y + params->dst_work_height)<<G2D_DST_BOTTOM_Y_BITPOS)|((params->dst_start_x + params->dst_work_width));
	__raw_writel(val, s5p_g2d_base + S5P_G2D_DST_RIGHT_BOTTOM_REG);	
		
}

void s5p_g2d_set_PatInfo(s5p_g2d_params *params)
{
	//to do
}

void s5p_g2d_set_MaskInfo(s5p_g2d_params *params)
{
	//to do
}

void s5p_g2d_set_ClipWinInfo(s5p_g2d_params *params)
{
	u32 val;
	/*set register for clipping window*/
	val = (params->cw_y1<<G2D_CW_TOP_Y_BITPOS)|(params->cw_x1);
	__raw_writel(val, s5p_g2d_base + S5P_G2D_CW_LT_REG);	
	

	val = (params->cw_y2<<G2D_CW_BOTTOM_Y_BITPOS)|(params->cw_x2);	
	__raw_writel(val, s5p_g2d_base + S5P_G2D_CW_RB_REG);
	
}

void s5p_g2d_set_ColorInfo(s5p_g2d_params *params)
{
	/*set register for color*/
	__raw_writel(params->color_val[G2D_WHITE], s5p_g2d_base + S5P_G2D_FG_COLOR_REG); // set color to both font and foreground color
	__raw_writel(params->color_val[G2D_BLACK], s5p_g2d_base + S5P_G2D_BG_COLOR_REG);
	__raw_writel(params->color_val[G2D_BLUE], s5p_g2d_base + S5P_G2D_BS_COLOR_REG); // Set blue color to blue screen color
}

u32 s5p_g2d_set_ROP_AlphaInfo(s5p_g2d_params *params)
{
	u32 alpha_cfg = G2D_BITBLT_CMD_NO_ALPHA_BLEND;
	
	/*Third Operand Selection */
	__raw_writel(0x33, s5p_g2d_base + S5P_G2D_THIRD_OPERAND_REG); //to do //get input from user

	/* Raster Operation Register */
	__raw_writel(G2D_ROP_SRC_ONLY | (G2D_ROP_SRC_ONLY << G2D_ROP4_MASKED_ROP3_BITPOS), s5p_g2d_base + S5P_G2D_ROP4_REG); //to do //get input from user

	params->transparent_mode = G2D_BITBLT_CMD_OPAQUE_MODE;             //to do  //get input from user
	

	/*set register for ROP & Alpha==================================*/
	if(params->alpha_mode != G2D_NO_ALPHA_BLEND_MODE){
		switch(params->alpha_mode){
			case G2D_EN_ALPHA_BLEND_MODE:
				alpha_cfg = G2D_BITBLT_CMD_EN_ALPHA_BLEND;
				__raw_writel(params->alpha_val & 0XFF, s5p_g2d_base + S5P_G2D_ALPHA_REG);
				break;
			case G2D_EN_ALPHA_BLEND_CONST_ALPHA:
				alpha_cfg = G2D_BITBLT_CMD_EN_ALPHA_BLEND |G2D_BITBLT_CMD_CONST_ALPHA_BLEND;
				__raw_writel(params->alpha_val & 0XFF, s5p_g2d_base + S5P_G2D_ALPHA_REG);
				break;
			case G2D_EN_ALPHA_BLEND_PERPIXEL_ALPHA:
				alpha_cfg = G2D_BITBLT_CMD_EN_ALPHA_BLEND |G2D_BITBLT_CMD_PERPIXEL_ALPHA_BLEND;
				__raw_writel(params->alpha_val & 0XFF, s5p_g2d_base + S5P_G2D_ALPHA_REG);
				break;
			case G2D_EN_FADING_MODE:
				alpha_cfg = G2D_BITBLT_CMD_EN_FADING;
				__raw_writel((params->fading_offset & 0XFF) << G2D_FAD_OFFSET_BITPOS, s5p_g2d_base + S5P_G2D_ALPHA_REG);
				break;
			default:
				alpha_cfg = G2D_BITBLT_CMD_NO_ALPHA_BLEND;
		}
			
	}

	return alpha_cfg;
		
}


u32 s5p_g2d_set_ColorKeyInfo(s5p_g2d_params *params)
{
	u32 colorKey_cfg =G2D_BITBLT_CMD_DIS_COLORKEY;

	switch(params->color_key_mode){
		case G2D_EN_SRC_COLORKEY:
			colorKey_cfg = G2D_BITBLT_CMD_EN_SRC_COLORKEY;
			__raw_writel(params->color_key_val, s5p_g2d_base + S5P_G2D_BS_COLOR_REG);
			break;
		case G2D_EN_DST_COLORKEY:
			colorKey_cfg = G2D_BITBLT_CMD_EN_DST_COLORKEY;
			__raw_writel(params->color_key_val, s5p_g2d_base + S5P_G2D_BS_COLOR_REG);
			break;
		case G2D_EN_SRC_DST_COLORKEY:
			colorKey_cfg = G2D_BITBLT_CMD_EN_SRC_DST_COLORKEY;
			__raw_writel(params->color_key_val, s5p_g2d_base + S5P_G2D_BS_COLOR_REG);
			break;
		default:
			colorKey_cfg =G2D_BITBLT_CMD_DIS_COLORKEY;
	}	
	
	return colorKey_cfg;
	

}

void s5p_g2d_set_RotationInfo(s5p_g2d_params *params, u32 rot_degree)
{
	u32 rotateVal;
	u32 srcDirectVal;
	u32 dstDirectVal;
	
	switch(rot_degree)
	{
		case ROT_90: 
			rotateVal = G2D_90_ROTATION;   //rotation = 1;
			srcDirectVal = G2D_SRC_X_DIRECT_POSITIVE | G2D_SRC_Y_DIRECT_POSITIVE;
			dstDirectVal = G2D_DST_X_DIRECT_POSITIVE | G2D_DST_Y_DIRECT_POSITIVE;
			break;
			
		case ROT_180: 
			rotateVal = G2D_NO_ROTATION;    //rotation = 0, srcXDirection != DstXDirection, srcYDirection != DstYDirection
			srcDirectVal = G2D_SRC_X_DIRECT_POSITIVE | G2D_SRC_Y_DIRECT_POSITIVE;
			dstDirectVal = G2D_DST_X_DIRECT_NEGATIVE | G2D_DST_Y_DIRECT_NEGATIVE;
			break;

		case ROT_270: 
			rotateVal = G2D_90_ROTATION;   //rotation = 1, srcXDirection != DstXDirection, srcYDirection != DstYDirection
			srcDirectVal = G2D_SRC_X_DIRECT_POSITIVE | G2D_SRC_Y_DIRECT_POSITIVE;
			dstDirectVal = G2D_DST_X_DIRECT_NEGATIVE | G2D_DST_Y_DIRECT_NEGATIVE;
			break;		

		case ROT_X_FLIP: 
			rotateVal = G2D_NO_ROTATION;    ////rotation = 0, srcYDirection != DstYDirection
			srcDirectVal = G2D_SRC_X_DIRECT_POSITIVE | G2D_SRC_Y_DIRECT_POSITIVE;
			dstDirectVal = G2D_DST_X_DIRECT_POSITIVE |G2D_DST_Y_DIRECT_NEGATIVE;
			break;

		case ROT_Y_FLIP: 
			rotateVal = G2D_NO_ROTATION;    ////rotation = 0, srcXDirection != DstXDirection
			srcDirectVal = G2D_SRC_X_DIRECT_POSITIVE | G2D_SRC_Y_DIRECT_POSITIVE;
			dstDirectVal = G2D_DST_X_DIRECT_NEGATIVE | G2D_DST_Y_DIRECT_POSITIVE;
			break;

		default :
			rotateVal = G2D_NO_ROTATION;   //rotation = 0;
			srcDirectVal = G2D_SRC_X_DIRECT_POSITIVE | G2D_SRC_Y_DIRECT_POSITIVE;
			dstDirectVal = G2D_DST_X_DIRECT_POSITIVE | G2D_DST_Y_DIRECT_POSITIVE;
			break;
	}

	__raw_writel(rotateVal, s5p_g2d_base + S5P_G2D_ROTATE_REG);
	__raw_writel(srcDirectVal, s5p_g2d_base + S5P_G2D_SRC_MSK_DIRECT_REG);
	__raw_writel(dstDirectVal, s5p_g2d_base + S5P_G2D_DST_PAT_DIRECT_REG);
	
	
}

static int s5p_g2d_init_regs(s5p_g2d_params *params, u32 rot_degree)
{
	u32 	bpp_mode_dst;
	u32 	bpp_mode_src;
	u32 alpha_reg = 0;
	u32	tmp_reg;
	u32 bitBltCmdVal = 0;
	u32 alphaCfgVal = 0;
	u32 colorKeyCfgVal = 0;
	int ret = 0;
	
	if(s5p_g2d_check_fifo_stat_wait() != 0)
		return -1;

	
	s5p_g2d_set_SrcImgInfo(params);

	s5p_g2d_set_DstImgInfo(params);

	s5p_g2d_set_PatInfo(params);  //to do

	s5p_g2d_set_MaskInfo(params); //to do

	s5p_g2d_set_ClipWinInfo(params);

	s5p_g2d_set_ColorInfo(params);

	alphaCfgVal = s5p_g2d_set_ROP_AlphaInfo(params);

	colorKeyCfgVal = s5p_g2d_set_ColorKeyInfo(params);  //to do

	s5p_g2d_set_RotationInfo(params, rot_degree);

	bitBltCmdVal = G2D_BITBLT_CMD_STRETCH_EN | G2D_BITBLT_CMD_CLIP_WIND | G2D_BITBLT_CMD_OPAQUE_MODE \
														| colorKeyCfgVal | alphaCfgVal |G2D_BITBLT_CMD_ZERO_COLOR_EXP;


	s5p_g2d_set_bitblt_cmd(bitBltCmdVal);	
	
	
	return 0;
	
}


static int s5p_g2d_start(s5p_g2d_params *params, ROT_DEG rot_degree)
{

	if(s5p_g2d_init_regs(params, rot_degree) != 0)
		return -1;

	s5p_g2d_bitblt_start();	

	return 0;	
}



irqreturn_t s5p_g2d_irq(int irq, void *dev_id)
{
	s5p_g2d_IntClear();
	wake_up_interruptible(&waitq_g2d);
	s5p_g2d_poll_flag = 1;
	return IRQ_HANDLED;
}

 int s5p_g2d_open(struct inode *inode, struct file *file)
{
	s5p_g2d_params *params;
	params = (s5p_g2d_params *)kmalloc(sizeof(s5p_g2d_params), GFP_KERNEL);
	if(params == NULL){
		printk(KERN_ERR "Instance memory allocation was failed\n");
		return -1;
	}

	memset(params, 0, sizeof(s5p_g2d_params));

	file->private_data	= (s5p_g2d_params *)params;
	
	g_num_of_g2d_object++;

	if(file->f_flags & O_NONBLOCK)
		g_num_of_nonblock_object++;


#ifdef G2D_DEBUG	
	printk("s5p_g2d_open() <<<<<<<< NEW DRIVER >>>>>>>>>>>>>>\n"); 	
#endif

	return 0;
}


int s5p_g2d_release(struct inode *inode, struct file *file)
{
	s5p_g2d_params	*params;

	params	= (s5p_g2d_params *)file->private_data;
	if (params == NULL) {
		printk(KERN_ERR "Can't release s5p_rotator!!\n");
		return -1;
	}

	kfree(params);
	
	g_num_of_g2d_object--;

	if(file->f_flags & O_NONBLOCK)
		g_num_of_nonblock_object--;
#if 0
	if(g_num_of_g2d_object == 0)
	{
		mutex_destroy(&g_g2d_clk_mutex);
	}
#endif

#ifdef G2D_DEBUG
	printk("s5p_g2d_release() \n"); 
#endif

	return 0;
}

#ifdef USE_G2D_MMAP
int s5p_g2d_mmap(struct file* filp, struct vm_area_struct *vma) 
{
	unsigned long pageFrameNo=0;
	unsigned long size;
	
	size = vma->vm_end - vma->vm_start;

	// page frame number of the address for a source G2D_SFR_SIZE to be stored at. 
	//pageFrameNo = __phys_to_pfn(S3C6400_PA_G2D);
	
	if(size > G2D_SFR_SIZE) {
		printk("The size of G2D_SFR_SIZE mapping is too big!\n");
		return -EINVAL;
	}
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot); 
	
	if((vma->vm_flags & VM_WRITE) && !(vma->vm_flags & VM_SHARED)) {
		printk("Writable G2D_SFR_SIZE mapping must be shared !\n");
		return -EINVAL;
	}
	
	if(remap_pfn_range(vma, vma->vm_start, pageFrameNo, size, vma->vm_page_prot))
		return -EINVAL;
	
	return 0;
}
#endif
#ifdef G2D_CLK_CTRL
static void s5p_g2d_clk_enable()
{
	u32 tmp;
        unsigned long flags;
	spin_lock_irqsave(&g2d_clkgate_lock, flags);
	tmp = __raw_readl(S5P_CLKGATE_IP0);
	tmp |= S5P_CLKGATE_IP0_G2D;
	__raw_writel(tmp, S5P_CLKGATE_IP0);
	spin_unlock_irqrestore(&g2d_clkgate_lock, flags);
//	printk("###########%s\n", __FUNCTION__);
	return;
}

static void s5p_g2d_clk_disable()
{
	u32 tmp;
        unsigned long flags;
        spin_lock_irqsave(&g2d_clkgate_lock, flags);
        tmp = __raw_readl(S5P_CLKGATE_IP0);
	tmp &= ~(S5P_CLKGATE_IP0_G2D);
        __raw_writel(tmp, S5P_CLKGATE_IP0);
        spin_unlock_irqrestore(&g2d_clkgate_lock, flags);
//	printk("========================%s\n", __FUNCTION__);
        return;

}

#endif
static int s5p_g2d_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	s5p_g2d_params	*params;
	ROT_DEG 		eRotDegree;
	int             ret = 0;
#ifdef G2D_DEBUG
	printk("##########%s:start \n", __FUNCTION__);
#endif
	params	= (s5p_g2d_params*)file->private_data;
	if (copy_from_user(params, (s5p_g2d_params*)arg, sizeof(s5p_g2d_params)))
	{
		return -EFAULT;
	}
	
#ifndef G2D_CLK_CTRL	
	clk_enable(s5p_g2d_clock);
#else
	s5p_g2d_clk_enable();
#endif

#if 0 //Handle G2D limitation for 1:x and x:1 input
/* G2D work around */
	if((params->src_work_width == 1) || (params->src_work_height == 1) || 
                (params->dst_work_width == 1) || (params->dst_work_height == 1))
       {
		params->dst_start_x = 0;
		params->dst_start_y = 0;

		if((cmd == S3C_G2D_ROTATOR_0) || (cmd == S3C_G2D_ROTATOR_180))
		{ 
	       	if((params->src_work_width == 1) || (params->dst_work_width == 1))
			{
			params->src_work_width++;
			params->dst_work_width++;
				if(params->src_work_width + params->src_start_x > params->src_full_width) 
			 	params->src_start_x = (params->src_start_x > 0)? (params->src_start_x - 1):params->src_start_x; 
         }
			if((params->src_work_height == 1) || (params->dst_work_height == 1))
			{
                        params->src_work_height++;
                        params->dst_work_height++; 
	                        if(params->src_work_height + params->src_start_y > params->src_full_height)
                                params->src_start_y = (params->src_start_y > 0)? (params->src_start_y - 1):params->src_start_y;
         }
	}
		else
		{
			if((params->src_work_width == 1) || (params->dst_work_height == 1))
			{
				params->src_work_width++;
				params->dst_work_height++;
				if(params->src_work_width + params->src_start_x > params->src_full_width)
					params->src_start_x = (params->src_start_x > 0)? (params->src_start_x - 1):params->src_start_x;
			}
			if((params->src_work_height == 1) || (params->dst_work_width == 1)){
				params->src_work_height++;
				 params->dst_work_width++;
				if(params->src_work_height + params->src_start_y > params->src_full_height)
	                           	 params->src_start_y = (params->src_start_y > 0)? (params->src_start_y - 1):params->src_start_y;
			}
	        }
	}
#endif
	
	mutex_lock(h_rot_mutex);
	
	switch(cmd)
	{
		case S3C_G2D_ROTATOR_0:
			eRotDegree = ROT_0;
			ret = s5p_g2d_start(params, eRotDegree);
			break;
			
		case S3C_G2D_ROTATOR_90:
			eRotDegree = ROT_90;
			ret = s5p_g2d_start(params, eRotDegree);
			break;

		case S3C_G2D_ROTATOR_180:
			eRotDegree = ROT_180;
			ret = s5p_g2d_start(params, eRotDegree);
			break;
			
		case S3C_G2D_ROTATOR_270:
			eRotDegree = ROT_270;
			ret = s5p_g2d_start(params, eRotDegree);
			break;

		case S3C_G2D_ROTATOR_X_FLIP:
			eRotDegree = ROT_X_FLIP;
			ret = s5p_g2d_start(params, eRotDegree);
			break;

		case S3C_G2D_ROTATOR_Y_FLIP:
			eRotDegree = ROT_Y_FLIP;
			ret = s5p_g2d_start(params, eRotDegree);
			break;

		
		default:
			ret = -EINVAL;
			goto err_cmd;
	}
	
	if(ret != 0){
		printk(KERN_ERR "\n%s:G2d error !!!!\n"); 
		goto err_cmd;
	}
	
	// block mode
	if(!(file->f_flags & O_NONBLOCK))
	{
		if (interruptible_sleep_on_timeout(&waitq_g2d, msecs_to_jiffies(G2D_TIMEOUT)) == 0)
		{
#ifdef G2D_DEBUG
		printk(KERN_ERR "\n%s: Waiting for interrupt is timeout\n", __FUNCTION__);
	           printk("###2##%s::src_fw %d src_fh %d src_w %d src_h %d src_x %d src_y %d dst_fw %d dst_fh %d dst_w %d dst_h %d dst_x %d dst_y %d cmd %d\n",
                        __FUNCTION__, params->src_full_width, params->src_full_height, params->src_work_width, params->src_work_height, params->src_start_x, params->src_start_y,
                        params->dst_full_width, params->dst_full_height, params->dst_work_width, params->dst_work_height, params->dst_start_x, params->dst_start_y, cmd);
#endif
            	 //ret = -EINVAL;
		ret = 0;

		}
	}
#ifdef G2D_DEBUG
        printk("##########%s:end \n", __FUNCTION__);
#endif

err_cmd:

	s5p_g2d_cache_reset();
#ifndef G2D_CLK_CTRL
	clk_disable(s5p_g2d_clock);
#else
	s5p_g2d_clk_disable();
#endif
	mutex_unlock(h_rot_mutex);
	return ret;
	
}

static unsigned int s5p_g2d_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;

	poll_wait(file, &waitq_g2d, wait);
	if(s5p_g2d_poll_flag == 1)
	{
		mask = POLLOUT|POLLWRNORM;
		s5p_g2d_poll_flag = 0;
	}

	return mask;
}
 struct file_operations s5p_g2d_fops = {
	.owner 	= THIS_MODULE,
	.open 	= s5p_g2d_open,
	.release 	= s5p_g2d_release,
#ifdef USE_G2D_MMAP
	.mmap 	= s5p_g2d_mmap,
#endif
	.ioctl		=s5p_g2d_ioctl,
	.poll		=s5p_g2d_poll,
};


static struct miscdevice s5p_g2d_dev = {
	.minor		= G2D_MINOR,
	.name		= "s3c-g2d",
	.fops		= &s5p_g2d_fops,
};

int s5p_g2d_probe(struct platform_device *pdev)
{

	struct resource *res;
	int ret;

#ifdef G2D_DEBUG
	printk("###########################s5p_g2d_probe called\n");
#endif

	/* find the IRQs */
	s5p_g2d_irq_num = platform_get_irq(pdev, 0);
	if(s5p_g2d_irq_num <= 0) {
		printk(KERN_ERR "failed to get irq resouce\n");
                return -ENOENT;
	}

	ret = request_irq(s5p_g2d_irq_num, s5p_g2d_irq, IRQF_DISABLED, pdev->name, NULL);
	if (ret) {
		printk("request_irq(g2d) failed.\n");
		return ret;
	}


	/* get the memory region */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(res == NULL) {
			printk(KERN_ERR "failed to get memory region resouce\n");
			return -ENOENT;
	}

	s5p_g2d_mem = request_mem_region(res->start, res->end-res->start+1, pdev->name);
	if(s5p_g2d_mem == NULL) {
		printk(KERN_ERR "failed to reserve memory region\n");
                return -ENOENT;
	}


	s5p_g2d_base = ioremap(s5p_g2d_mem->start, s5p_g2d_mem->end - res->start + 1);
	if(s5p_g2d_base == NULL) {
		printk(KERN_ERR "failed ioremap\n");
                return -ENOENT;
	}
#if 1
	s5p_g2d_clock = clk_get(&pdev->dev, "clk_g2d");
	if (IS_ERR(s5p_g2d_clock)) {
			printk(KERN_ERR "failed to find g2d clock source\n");
			return -ENOENT;
	}
	clk_enable(s5p_g2d_clock);
#endif
	init_waitqueue_head(&waitq_g2d);

	ret = misc_register(&s5p_g2d_dev);
	if (ret) {
		printk (KERN_ERR "cannot register miscdev on minor=%d (%d)\n",
			G2D_MINOR, ret);
		return ret;
	}

	h_rot_mutex = (struct mutex *)kmalloc(sizeof(struct mutex), GFP_KERNEL);
	if (h_rot_mutex == NULL)
		return -1;
	
	mutex_init(h_rot_mutex);
	clk_disable(s5p_g2d_clock);
//#ifdef G2D_DEBUG
	printk(KERN_ALERT"##################### s5p_g2d_probe Success\n");
//#endif

	return 0;  
}

static int s5p_g2d_remove(struct platform_device *dev)
{
	printk(KERN_INFO "s5p_g2d_remove called !\n");

	free_irq(s5p_g2d_irq_num, NULL);
	
	if (s5p_g2d_mem != NULL) {   
		printk(KERN_INFO "S3C G2D  Driver, releasing resource\n");
		iounmap(s5p_g2d_base);
		release_resource(s5p_g2d_mem);
		kfree(s5p_g2d_mem);
	}
	
	misc_deregister(&s5p_g2d_dev);
	printk(KERN_INFO "s5p_g2d_remove Success !\n");
	return 0;
}

static int s5p_g2d_suspend(struct platform_device *dev, pm_message_t state)
{
//	clk_disable(s5p_g2d_clock);
	return 0;
}
static int s5p_g2d_resume(struct platform_device *pdev)
{
//	clk_enable(s5p_g2d_clock);
	return 0;
}


static struct platform_driver s5p_g2d_driver = {
       .probe          = s5p_g2d_probe,
       .remove         = s5p_g2d_remove,
       .suspend        = s5p_g2d_suspend,
       .resume         = s5p_g2d_resume,
       .driver		= {
		.owner	= THIS_MODULE,
		.name	= "s3c-g2d",
	},
};

int __init  s5p_g2d_init(void)
{
 	if(platform_driver_register(&s5p_g2d_driver)!=0)
  	{
   		printk("platform device register Failed \n");
   		return -1;
  	}
	printk(" S5P6442 G2D  Init : Done\n");
       	return 0;
}

void  s5p_g2d_exit(void)
{
	platform_driver_unregister(&s5p_g2d_driver);
	mutex_destroy(h_rot_mutex);
 	printk("S5P6442: G2D module exit\n");
}

module_init(s5p_g2d_init);
module_exit(s5p_g2d_exit);

MODULE_AUTHOR("SW Solution Development Team");
MODULE_DESCRIPTION("G2D Device Driver");
MODULE_LICENSE("GPL");

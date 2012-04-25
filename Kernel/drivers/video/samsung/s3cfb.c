/* linux/drivers/video/samsung/s3cfb.c
 *
 * Core file for Samsung Display Controller (FIMD) driver
 *
 * Jinsung Yang, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/mm.h>
#include <linux/fb.h>
#include <linux/ctype.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/memory.h>
#include <plat/clock.h>
#include <linux/earlysuspend.h>
#include <plat/power_clk_gating.h>

#ifdef CONFIG_FB_S3C_LTE480WV
#include "logo_rgb24_wvga_landscape.h"
#endif

#ifdef CONFIG_FB_S3C_TL2796
#include "logo_rgb24_wvga_portrait.h"
#endif

#ifdef CONFIG_FB_S3C_AMS320
#include "logo_rgb24_hvga_portrait.h"
#endif

#if defined(CONFIG_FB_S3C_LMS300)||defined(CONFIG_FB_S3C_S6D04D1)

#ifdef CONFIG_LITE
#ifndef CONFIG_PERFORMANCE
#include "g3mod-lite.h"
#endif
#endif

#ifdef CONFIG_LITE
#ifdef CONFIG_PERFORMANCE
#ifndef CONFIG_ULTIMATE
#include "g3mod-perf.h"
#endif
#endif
#endif

#ifdef CONFIG_LITE
#ifdef CONFIG_PERFORMANCE
#ifdef CONFIG_ULTIMATE
#ifndef CONFIG_EXTREME
#include "g3mod-ult.h"
#endif
#endif
#endif
#endif

#ifdef CONFIG_LITE
#ifdef CONFIG_PERFORMANCE
#ifdef CONFIG_ULTIMATE
#ifdef CONFIG_EXTREME
#include "g3mod-ext.h"
#endif
#endif
#endif
#endif

#endif


#include "s3cfb.h"

extern void s3cfb_set_lcd_power(int value);
extern int  s3cfb_get_lcd_power(void);
extern void s3cfb_set_backlight_power(int value);
extern int  s3cfb_get_backlight_power(void);
extern void s3cfb_set_backlight_level(int value);
extern int  s3cfb_get_backlight_level(void);

static struct s3cfb_global * fbdev;

/*
 *  *  Mark for GetLog (yohwa)
 *   */

struct struct_frame_buf_mark {
        u32 special_mark_1;
        u32 special_mark_2;
        u32 special_mark_3;
        u32 special_mark_4;
        void *p_fb;
        u32 resX;
        u32 resY;
        u32 bpp;    //color depth : 16 or 24
        u32 frames; // frame buffer count : 2
};

static struct struct_frame_buf_mark  frame_buf_mark = {
        .special_mark_1 = (('*' << 24) | ('^' << 16) | ('^' << 8) | ('*' << 0)),
        .special_mark_2 = (('I' << 24) | ('n' << 16) | ('f' << 8) | ('o' << 0)),
        .special_mark_3 = (('H' << 24) | ('e' << 16) | ('r' << 8) | ('e' << 0)),
        .special_mark_4 = (('f' << 24) | ('b' << 16) | ('u' << 8) | ('f' << 0)),
        .p_fb   = 0,
        .resX   = 240,
        .resY   = 400,
        .bpp    = 32,
        .frames = 2
};

#ifdef CONFIG_HAS_EARLYSUSPEND
int s3cfb_early_suspend(struct early_suspend *h);
int s3cfb_late_resume(struct early_suspend *h);
#endif

struct s3c_platform_fb *to_fb_plat(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);

	return (struct s3c_platform_fb *) pdev->dev.platform_data;
}

#ifndef CONFIG_FRAMEBUFFER_CONSOLE
static int s3cfb_draw_logo(struct fb_info *fb)
{
	//printk("###############%s \n", __FUNCTION__);
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);
	struct fb_fix_screeninfo *fix = &fb->fix;
	struct fb_var_screeninfo *var = &fb->var;

#if 0	//sm.kim: copy logo from bootloader's framebuffer
	s3cfb_display_logo(fbdev->fb[pdata->default_win]);
#else
	memcpy(fbdev->fb[pdata->default_win]->screen_base,
		LOGO_RGB24, fix->line_length * var->yres);
#endif

	return 0;
}
#else
int fb_is_primary_device(struct fb_info *fb)
{
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);
	struct s3cfb_window *win = fb->par;

	dev_dbg(fbdev->dev, "[fb%d] checking for primary device\n", win->id);

	if (win->id == pdata->default_win)
		return 1;
	else
		return 0;
}
#endif

static irqreturn_t s3cfb_irq_frame(int irq, void *dev_id)
{
	s3cfb_clear_interrupt(fbdev);

	fbdev->wq_count++;
	wake_up_interruptible(&fbdev->wq);

	return IRQ_HANDLED;
}

#ifdef CONFIG_FB_S3C_TRACE_UNDERRUN
static irqreturn_t s3cfb_irq_fifo(int irq, void *dev_id)
{
	s3cfb_clear_interrupt(fbdev);

	return IRQ_HANDLED;
}
#endif

static int s3cfb_enable_window(int id)
{
	struct s3cfb_window *win = fbdev->fb[id]->par;

	if (s3cfb_window_on(fbdev, id)) {
		win->enabled = 0;
		return -EFAULT;
	} else {
		win->enabled = 1;
		return 0;
	}
}

static int s3cfb_disable_window(int id)
{
	struct s3cfb_window *win = fbdev->fb[id]->par;

	if (s3cfb_window_off(fbdev, id)) {
		win->enabled = 1;
		return -EFAULT;
	} else {
		win->enabled = 0;
		return 0;
	}
}

static int s3cfb_init_global(void)
{
	fbdev->output   = OUTPUT_RGB;
	fbdev->rgb_mode = MODE_RGB_P;

	fbdev->wq_count = 0;
	init_waitqueue_head(&fbdev->wq);
	mutex_init(&fbdev->lock);

	s3cfb_set_output      (fbdev);
	s3cfb_set_display_mode(fbdev);
	s3cfb_set_polarity    (fbdev);
	s3cfb_set_timing      (fbdev);
	s3cfb_set_lcd_size    (fbdev);

	return 0;
}

static int s3cfb_map_video_memory(struct fb_info *fb)
{
	struct fb_fix_screeninfo *fix = &fb->fix;
	struct s3cfb_window *win = fb->par;

	if (win->path != DATA_PATH_DMA)
		return 0;

	fb->screen_base = dma_alloc_writecombine(fbdev->dev,
				PAGE_ALIGN(fix->smem_len),
						 (unsigned int *)
						 &fix->smem_start, GFP_KERNEL);
	if (!fb->screen_base)
		return -ENOMEM;
	else
		dev_info(fbdev->dev, "[fb%d] dma: 0x%08x, cpu: 0x%08x, "
                 "size: 0x%08x\n", win->id,
                 (unsigned int)fix->smem_start,
                 (unsigned int)fb->screen_base,
				 fix->smem_len);

	memset(fb->screen_base, 0, fix->smem_len);

	frame_buf_mark.p_fb = (void*)fb->screen_base;

	return 0;
}

static int s3cfb_unmap_video_memory(struct fb_info *fb)
{
	struct fb_fix_screeninfo *fix = &fb->fix;
	struct s3cfb_window *win = fb->par;

	if (fix->smem_start) {
		dma_free_writecombine(fbdev->dev, fix->smem_len,
			fb->screen_base, fix->smem_start);
		fix->smem_start = 0;
		fix->smem_len = 0;
		dev_info(fbdev->dev, "[fb%d] video memory released\n", win->id);
	}

	return 0;
}

static int s3cfb_set_bitfield(struct fb_var_screeninfo *var)
{
	switch (var->bits_per_pixel)
	{
		case 16:
			if (var->transp.length == 1) {
				var->red.offset    = 10;
				var->red.length    = 5;
				var->green.offset  = 5;
				var->green.length  = 5;
				var->blue.offset   = 0;
				var->blue.length   = 5;
				var->transp.offset = 15;
			} else if (var->transp.length == 4) {
				var->red.offset    = 8;
				var->red.length    = 4;
				var->green.offset  = 4;
				var->green.length  = 4;
				var->blue.offset   = 0;
				var->blue.length   = 4;
				var->transp.offset = 12;
			} else {
				var->red.offset    = 11;
				var->red.length    = 5;
				var->green.offset  = 5;
				var->green.length  = 6;
				var->blue.offset   = 0;
				var->blue.length   = 5;
				var->transp.offset = 0;
			}
			break;

		case 24:
			var->red.offset    = 16;
			var->red.length    = 8;
			var->green.offset  = 8;
			var->green.length  = 8;
			var->blue.offset   = 0;
			var->blue.length   = 8;
			var->transp.offset = 0;
			var->transp.length = 0;
			break;

		case 32:
			var->red.offset    = 16;
			var->red.length    = 8;
			var->green.offset  = 8;
			var->green.length  = 8;
			var->blue.offset   = 0;
			var->blue.length   = 8;
			var->transp.offset = 24;
	
			break;
	}

	return 0;
}

static int s3cfb_set_alpha_info(struct fb_var_screeninfo *var, struct s3cfb_window *win)
{
	if (var->transp.length > 0)
		win->alpha.mode = PIXEL_BLENDING;
	else {
		win->alpha.mode = PLANE_BLENDING;
		win->alpha.channel = 0;
		win->alpha.value = S3CFB_AVALUE(0xf, 0xf, 0xf);
	}

	return 0;
}

static int s3cfb_check_var(struct fb_var_screeninfo *var, struct fb_info *fb)
{
	struct s3c_platform_fb *   pdata = to_fb_plat(fbdev->dev);
	struct fb_fix_screeninfo * fix   = &fb->fix;
	struct s3cfb_window *      win   = fb->par;
	struct s3cfb_lcd *         lcd   = fbdev->lcd;

	dev_dbg(fbdev->dev, "[fb%d] check_var\n", win->id);

	// (100311 / kcoolsw ) support 24bpp & optimization
	/*
	if (var->bits_per_pixel != 16 && var->bits_per_pixel != 24 &&
		var->bits_per_pixel != 32) {
		dev_err(fbdev->dev, "invalid bits per pixel\n");
		return -EINVAL;
	}
	*/
	switch(var->bits_per_pixel)
	{
		case 16:
		case 24:
		case 32:
			break;
		default:
			dev_err(fbdev->dev, "invalid bits per pixel : %d\n", var->bits_per_pixel);
			return -EINVAL;
			break;
	}
	
	if (var->xres > lcd->width)
		var->xres = lcd->width;

	if (var->yres > lcd->height)
		var->yres = lcd->height;

	if (var->xres_virtual != var->xres)
		var->xres_virtual = var->xres;

	if (var->yres_virtual > var->yres * (fb->fix.ypanstep + 1))
		var->yres_virtual = var->yres * (fb->fix.ypanstep + 1);

	if (var->xoffset != 0)
		var->xoffset = 0;

	if (var->yoffset + var->yres > var->yres_virtual)
		var->yoffset = var->yres_virtual - var->yres;

	if (win->x + var->xres > lcd->width)
		win->x = lcd->width - var->xres;

	if (win->y + var->yres > lcd->height)
		win->y = lcd->height - var->yres;

	/* modify the fix info */
	if (win->id != pdata->default_win)
	{
		// (100311 / kcoolsw ) support 24bpp & optimization
		/*
		fix->line_length = var->xres_virtual * var->bits_per_pixel / 8;
		fix->smem_len    = fix->line_length  * var->yres_virtual;
		*/
		switch(var->bits_per_pixel)
		{
			case 16 :
				//fix->line_length = var->xres_virtual * 16 / 8;
				fix->line_length = (var->xres_virtual << 4) >> 3;
				break;

			case 24 :
			case 32 :
				//fix->line_length = var->xres_virtual * 32 / 8;
				fix->line_length = (var->xres_virtual << 5) >> 3;
				break;
				
			default:
				fix->line_length = (var->xres_virtual * var->bits_per_pixel) >> 3;
				break;
		}

		fix->smem_len = fix->line_length  * var->yres_virtual;
	}

	s3cfb_set_bitfield(var);
	s3cfb_set_alpha_info(var, win);

	return 0;	
}

static int s3cfb_set_par(struct fb_info *fb)
{
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);
	struct s3cfb_window *win = fb->par;

	dev_dbg(fbdev->dev, "[fb%d] set_par\n", win->id);

	if ((win->id != pdata->default_win) && !fb->fix.smem_start)
		s3cfb_map_video_memory(fb);

	s3cfb_set_window_control (fbdev, win->id);
	s3cfb_set_window_position(fbdev, win->id);
	s3cfb_set_window_size    (fbdev, win->id);
	s3cfb_set_buffer_address (fbdev, win->id);
	s3cfb_set_buffer_size    (fbdev, win->id);

	if (win->id > 0)
		s3cfb_set_alpha_blending(fbdev, win->id);

	return 0;	
}

static int s3cfb_blank(int blank_mode, struct fb_info *fb)
{
	struct s3cfb_window *win = fb->par;

	dev_dbg(fbdev->dev, "change blank mode\n");

	switch (blank_mode)
	{
		case FB_BLANK_UNBLANK:
			if (fb->fix.smem_start)
				s3cfb_enable_window(win->id);
			else
				dev_info(fbdev->dev,
				         "[fb%d] no allocated memory for unblank\n",
				         win->id);
			break;

		case FB_BLANK_POWERDOWN:
			s3cfb_disable_window(win->id);
			break;

		default:
			dev_dbg(fbdev->dev, "unsupported blank mode\n");
			return -EINVAL;
	}

	return 0;
}

static int s3cfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *fb)
{
	struct s3cfb_window *win = fb->par;

	if (var->yoffset + var->yres > var->yres_virtual) {
		dev_err(fbdev->dev, "invalid yoffset value\n");
		return -EINVAL;
	}

	fb->var.yoffset = var->yoffset;

	#ifdef __SEC_FULL_DEBUG_MSG__	//sm.kim: prevent this message because it is printed too many times.
	dev_dbg(fbdev->dev, "[fb%d] yoffset for pan display: %d\n", win->id,
		var->yoffset);
	#endif

	s3cfb_set_buffer_address(fbdev, win->id);

	return 0;
}

static inline unsigned int __chan_to_field(unsigned int chan,
						struct fb_bitfield bf)
{
	chan &= 0xffff;
	chan >>= 16 - bf.length;

	return chan << bf.offset;
}

static int s3cfb_setcolreg(unsigned int regno, unsigned int red,
				unsigned int green, unsigned int blue,
				unsigned int transp, struct fb_info *fb)
{
	unsigned int *pal = (unsigned int *) fb->pseudo_palette;
	unsigned int val = 0;

	if (regno < 16) {
		/* fake palette of 16 colors */
		val |= __chan_to_field(red,    fb->var.red);
		val |= __chan_to_field(green,  fb->var.green);
		val |= __chan_to_field(blue,   fb->var.blue);
		val |= __chan_to_field(transp, fb->var.transp);

		pal[regno] = val;
	}

	return 0;
}

static int s3cfb_open(struct fb_info *fb, int user)
{
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);
	struct s3cfb_window *win = fb->par;
	int ret = 0;

	mutex_lock(&fbdev->lock);

	if (atomic_read(&win->in_use)) {
		if (win->id == pdata->default_win) {
			dev_dbg(fbdev->dev,
				"multiple open for default window\n");
			ret = 0;
		} else {
			dev_dbg(fbdev->dev,
				"do not allow multiple open "
				"for non-default window\n");
			ret = -EBUSY;
		}
	} else
		atomic_inc(&win->in_use);

	mutex_unlock(&fbdev->lock);

	return ret;
}

static int s3cfb_release_window(struct fb_info *fb)
{
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);
	struct s3cfb_window *win = fb->par;

	if (win->id != pdata->default_win) {
		s3cfb_disable_window(win->id);
		s3cfb_unmap_video_memory(fb);
		s3cfb_set_buffer_address(fbdev, win->id);
	}

	win->x = 0;
	win->y = 0;

	return 0;
}

static int s3cfb_release(struct fb_info *fb, int user)
{
	struct s3cfb_window *win = fb->par;

	s3cfb_release_window(fb);

	mutex_lock(&fbdev->lock);
	atomic_dec(&win->in_use);
	mutex_unlock(&fbdev->lock);

	return 0;
}

static int s3cfb_cursor(struct fb_info *info, struct fb_cursor *cursor)
{
	/* nothing to do for removing cursor */
	return 0;
}

static int s3cfb_wait_for_vsync(void)
{
	#ifdef __SEC_FULL_DEBUG_MSG__	//sm.kim: prevent this message because it is printed too many times.
	dev_dbg(fbdev->dev, "waiting for VSYNC interrupt\n");
	#endif

	interruptible_sleep_on_timeout(&fbdev->wq, HZ / 10);

	#ifdef __SEC_FULL_DEBUG_MSG__	//sm.kim: prevent this message because it is printed too many times.
	dev_dbg(fbdev->dev, "got a VSYNC interrupt\n");
	#endif

	return 0;
}

static int s3cfb_ioctl(struct fb_info *fb, unsigned int cmd, unsigned long arg)
{
	struct fb_var_screeninfo *var = &fb->var;
	struct s3cfb_window *win = fb->par;
	struct s3cfb_lcd *lcd = fbdev->lcd;
#if 1
	// added by jamie (2009.08.18)
	struct fb_fix_screeninfo *fix = &fb->fix;
    s3cfb_next_info_t next_fb_info;
#endif

	int ret = 0;

	union {
		struct s3cfb_user_window      user_window;
		struct s3cfb_user_plane_alpha user_alpha;
		struct s3cfb_user_chroma      user_chroma;
		int vsync;
	} p;

	switch (cmd) {
	case FBIO_WAITFORVSYNC:
		s3cfb_wait_for_vsync();
		break;

	case S3CFB_WIN_POSITION:
		if (copy_from_user(&p.user_window,
			(struct s3cfb_user_window __user *) arg,
			sizeof(p.user_window)))
			ret = -EFAULT;
		else {
			if (p.user_window.x < 0)
				p.user_window.x = 0;

			if (p.user_window.y < 0)
				p.user_window.y = 0;

			if (p.user_window.x + var->xres > lcd->width)
				win->x = lcd->width - var->xres;
			else
				win->x = p.user_window.x;

			if (p.user_window.y + var->yres > lcd->height)
				win->y = lcd->height - var->yres;
			else
				win->y = p.user_window.y;

			s3cfb_set_window_position(fbdev, win->id);
		}
		break;

	case S3CFB_WIN_SET_PLANE_ALPHA:
		if (copy_from_user(&p.user_alpha,
			(struct s3cfb_user_plane_alpha __user *) arg,
			sizeof(p.user_alpha)))
			ret = -EFAULT;
		else {
			win->alpha.mode    = PLANE_BLENDING;
			win->alpha.channel = p.user_alpha.channel;
			win->alpha.value   = S3CFB_AVALUE(p.user_alpha.red,
			                                  p.user_alpha.green,
			                                  p.user_alpha.blue);

			s3cfb_set_alpha_blending(fbdev, win->id);
		}
		break;

	case S3CFB_WIN_SET_CHROMA:
		if (copy_from_user(&p.user_chroma,
			(struct s3cfb_user_chroma __user *) arg,
			sizeof(p.user_chroma)))
			ret = -EFAULT;
		else {
			win->chroma.enabled = p.user_chroma.enabled;
			win->chroma.key     = S3CFB_CHROMA(p.user_chroma.red,
			                                   p.user_chroma.green,
			                                   p.user_chroma.blue);

			s3cfb_set_chroma_key(fbdev, win->id);
		}
		break;

	case S3CFB_SET_VSYNC_INT:
		if (get_user(p.vsync, (int __user *) arg))
			ret = -EFAULT;
		else {
			if (p.vsync)
				s3cfb_set_global_interrupt(fbdev, 1);

			s3cfb_set_vsync_interrupt(fbdev, p.vsync);
		}
		break;

#if 1
	// added by jamie (2009.08.18)
    case S3CFB_GET_CURR_FB_INFO:
		next_fb_info.phy_start_addr = fix->smem_start;
		next_fb_info.xres           = var->xres;
		next_fb_info.yres           = var->yres;
		next_fb_info.xres_virtual   = var->xres_virtual;
		next_fb_info.yres_virtual   = var->yres_virtual;
		next_fb_info.xoffset        = var->xoffset;
		next_fb_info.yoffset        = var->yoffset;
		//next_fb_info.lcd_offset_x   = fbi->lcd_offset_x;
		//next_fb_info.lcd_offset_y   = fbi->lcd_offset_y;
		next_fb_info.lcd_offset_x   = 0;
		next_fb_info.lcd_offset_y   = 0;

		if (copy_to_user((void *)arg, (s3cfb_next_info_t *) &next_fb_info, sizeof(s3cfb_next_info_t)))
			return -EFAULT;
		break;
#endif
	}

	return ret;
}

struct fb_ops s3cfb_ops = {
	.owner          = THIS_MODULE,
	.fb_fillrect    = cfb_fillrect,
	.fb_copyarea    = cfb_copyarea,
	.fb_imageblit   = cfb_imageblit,
	.fb_check_var   = s3cfb_check_var,
	.fb_set_par     = s3cfb_set_par,
	.fb_blank       = s3cfb_blank,
	.fb_pan_display = s3cfb_pan_display,
	.fb_setcolreg   = s3cfb_setcolreg,
	.fb_cursor      = s3cfb_cursor,
	.fb_ioctl       = s3cfb_ioctl,
	.fb_open        = s3cfb_open,
	.fb_release     = s3cfb_release,
};

/* new function to open fifo */
int s3cfb_open_fifo(int id, int ch, int (*do_priv) (void *), void *param)
{
	struct s3cfb_window *win = fbdev->fb[id]->par;

	dev_dbg(fbdev->dev, "[fb%d] open fifo\n", win->id);

	if (win->path == DATA_PATH_DMA)
		win->path = DATA_PATH_FIFO;

	win->local_channel = ch;

	s3cfb_set_vsync_interrupt(fbdev, 1);
	s3cfb_wait_for_vsync();
	s3cfb_set_vsync_interrupt(fbdev, 0);

	if (do_priv) {
		if (do_priv(param)) {
			dev_err(fbdev->dev, "failed to run for "
				"private fifo open\n");
			s3cfb_enable_window(id);
			return -EFAULT;
		}
	}

	s3cfb_set_window_control(fbdev, id);
	s3cfb_enable_window(id);

	return 0;
}

/* new function to close fifo */
int s3cfb_close_fifo(int id, int (*do_priv) (void *), void *param, int sleep)
{
	struct s3cfb_window *win = fbdev->fb[id]->par;
	int ret = 0;

	dev_dbg(fbdev->dev, "[fb%d] close fifo\n", win->id);

	if (sleep)
		win->path = DATA_PATH_FIFO;
	else
		win->path = DATA_PATH_DMA;

	s3cfb_set_vsync_interrupt(fbdev, 1);
	s3cfb_wait_for_vsync();
	s3cfb_set_vsync_interrupt(fbdev, 0);

	s3cfb_display_off(fbdev);
	s3cfb_check_line_count(fbdev);
	s3cfb_disable_window(id);

	if (do_priv) {
		if (do_priv(param)) {
			dev_err(fbdev->dev, "failed to run for"
				"private fifo close\n");
			s3cfb_enable_window(id);
			
			ret = -EFAULT;
		}
	}

	s3cfb_display_on(fbdev);

	return ret;
}

/* for backward compatibilities */
void s3cfb_enable_local(int id, int in_yuv, int ch)
{
	struct s3cfb_window *win = fbdev->fb[id]->par;

	win->path          = DATA_PATH_FIFO;
	win->local_channel = ch;

	s3cfb_set_vsync_interrupt(fbdev, 1);
	s3cfb_wait_for_vsync();
	s3cfb_set_vsync_interrupt(fbdev, 0);

	s3cfb_set_window_control(fbdev, id);
	s3cfb_enable_window(id);
}

/* for backward compatibilities */
void s3cfb_enable_dma(int id)
{
	struct s3cfb_window *win = fbdev->fb[id]->par;

	win->path = DATA_PATH_DMA;

	s3cfb_set_vsync_interrupt(fbdev, 1);
	s3cfb_wait_for_vsync();
	s3cfb_set_vsync_interrupt(fbdev, 0);

	s3cfb_disable_window(id);
	s3cfb_display_off(fbdev);
	s3cfb_set_window_control(fbdev, id);
	s3cfb_display_on(fbdev);
}

int s3cfb_direct_ioctl(int id, unsigned int cmd, unsigned long arg)
{
	struct fb_info * fb            = fbdev->fb[id];
	struct fb_var_screeninfo * var = &fb->var;	
	struct s3cfb_window * win      = fb->par;
	struct s3cfb_lcd * lcd         = fbdev->lcd;
	struct s3cfb_user_window  user_win;
	void *argp = (void *) arg;
	int ret = 0;

	switch (cmd) {
	case FBIO_ALLOC:
		win->path = (enum s3cfb_data_path_t) argp;
		break;

	case FBIOGET_FSCREENINFO:
		ret = memcpy(argp, &fb->fix, sizeof(fb->fix)) ? 0 : -EFAULT;
		break;

	case FBIOGET_VSCREENINFO:
		ret = memcpy(argp, &fb->var, sizeof(fb->var)) ? 0 : -EFAULT;
		break;

	case FBIOPUT_VSCREENINFO:
		ret = s3cfb_check_var((struct fb_var_screeninfo *) argp, fb);
		if (ret) {
			dev_err(fbdev->dev, "invalid vscreeninfo\n");
			break;
		}

		ret = memcpy(&fb->var, (struct fb_var_screeninfo *) argp,
				sizeof(fb->var)) ? 0 : -EFAULT;
		if (ret) {
			dev_err(fbdev->dev, "failed to put new vscreeninfo\n");
			break;
		}

		ret = s3cfb_set_par(fb);
		break;

	case S3CFB_WIN_POSITION:
		ret = memcpy(&user_win, (struct s3cfb_user_window __user *) arg,
				sizeof(user_win)) ? 0 : -EFAULT;
		if (ret) {
			dev_err(fbdev->dev, "failed to S3CFB_WIN_POSITION\n");
			break;
		}

		if (user_win.x < 0)
			user_win.x = 0;

		if (user_win.y < 0)
			user_win.y = 0;

		if (user_win.x + var->xres > lcd->width)
			win->x = lcd->width - var->xres;
		else
			win->x = user_win.x;

		if (user_win.y + var->yres > lcd->height)
			win->y = lcd->height - var->yres;
		else
			win->y = user_win.y;

		s3cfb_set_window_position(fbdev, win->id);
		break;

	case S3CFB_SET_SUSPEND_FIFO:
		win->suspend_fifo = argp;
		break;

	case S3CFB_SET_RESUME_FIFO:
		win->resume_fifo = argp;
		break;

	case S3CFB_GET_LCD_WIDTH:
		ret = memcpy(argp, &lcd->width, sizeof(int)) ? 0 : -EFAULT;
		if (ret) {
			dev_err(fbdev->dev, "failed to S3CFB_GET_LCD_WIDTH\n");
			break;
		}

		break;

	case S3CFB_GET_LCD_HEIGHT:
		ret = memcpy(argp, &lcd->height, sizeof(int)) ? 0 : -EFAULT;
		if (ret) {
			dev_err(fbdev->dev, "failed to S3CFB_GET_LCD_HEIGHT\n");
			break;
		}

		break;

	/*
	 * for FBIO_WAITFORVSYNC
	*/
	default:
		ret = s3cfb_ioctl(fb, cmd, arg);
		break;
	}

	return ret;
}

EXPORT_SYMBOL(s3cfb_direct_ioctl);

static int s3cfb_init_fbinfo(int id)
{
	struct s3c_platform_fb *   pdata = to_fb_plat(fbdev->dev);
	struct fb_info * fb              = fbdev->fb[id];
	struct fb_fix_screeninfo * fix   = &fb->fix;
	struct fb_var_screeninfo * var   = &fb->var;
	struct s3cfb_window * win        = fb->par;
	struct s3cfb_alpha * alpha       = &win->alpha;
	struct s3cfb_lcd * lcd           = fbdev->lcd;
	struct s3cfb_lcd_timing * timing = &lcd->timing;
	
	memset(win, 0, sizeof(struct s3cfb_window));
	platform_set_drvdata(to_platform_device(fbdev->dev), fb);
	strcpy(fix->id, S3CFB_NAME);

	/* fimd specific */
	win->id        = id;
	win->path      = DATA_PATH_DMA;
	win->dma_burst = 16;
	alpha->mode    = PLANE_BLENDING;

	/* fbinfo */
	fb->fbops          = &s3cfb_ops;
	fb->flags          = FBINFO_FLAG_DEFAULT;
	fb->pseudo_palette = &win->pseudo_pal;

	fix->xpanstep = 0;
	fix->ypanstep = pdata->nr_buffers[id] - 1;
	fix->type     = FB_TYPE_PACKED_PIXELS;
	fix->accel    = FB_ACCEL_NONE;
	fix->visual   = FB_VISUAL_TRUECOLOR;

	var->xres         = lcd->width;
	var->yres         = lcd->height;
	var->xres_virtual = var->xres;
	var->yres_virtual = var->yres + (var->yres * fix->ypanstep);

	// (100311 / kcoolsw ) support 24bpp & optimization
#if defined (CONFIG_FB_S3C_BPP_16)
	var->bits_per_pixel = 16;
#elif defined (CONFIG_FB_S3C_BPP_24)
	var->bits_per_pixel = 24;
#elif defined (CONFIG_FB_S3C_BPP_32)
	var->bits_per_pixel = 32;
#else
	// default is 24bpp
	var->bits_per_pixel = 24;
#endif

	var->xoffset       = 0;
	var->yoffset       = 0;
	var->width = lcd->p_width;	// width of lcd in mm
	var->height= lcd->p_height;	// height of lcd in mm
	var->transp.length = 0;

	// (100311 / kcoolsw ) support 24bpp & optimization
	//fix->line_length = var->xres_virtual * var->bits_per_pixel / 8;
	switch(var->bits_per_pixel)
	{
		case 16 :
			//fix->line_length = var->xres_virtual * 16 / 8;
			fix->line_length = (var->xres_virtual << 4) >> 3;
			break;

		case 24 :
		case 32 :
			//fix->line_length = var->xres_virtual * 32 / 8;
			fix->line_length = (var->xres_virtual << 5) >> 3;
			break;
			
		default:
			fix->line_length = (var->xres_virtual * var->bits_per_pixel) >> 3;
			break;
	}
	fix->smem_len    = fix->line_length * var->yres_virtual;

	var->nonstd       = 0;
	var->activate     = FB_ACTIVATE_NOW;
	var->vmode        = FB_VMODE_NONINTERLACED;
	var->hsync_len    = timing->h_sw;
	var->vsync_len    = timing->v_sw;
	var->left_margin  = timing->h_fp;
	var->right_margin = timing->h_bp;
	var->upper_margin = timing->v_fp;
	var->lower_margin = timing->v_bp;

	var->pixclock = lcd->freq
	                * (var->left_margin  + var->right_margin + var->hsync_len + var->xres)
	                * (var->upper_margin + var->lower_margin + var->vsync_len + var->yres);

	dev_dbg(fbdev->dev, "pixclock: %d\n", var->pixclock);

	s3cfb_set_bitfield(var);
	s3cfb_set_alpha_info(var, win);

	return 0;
}

static int s3cfb_alloc_framebuffer(void)
{
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);
	int ret, i;

	/* alloc for framebuffers */
	fbdev->fb = kmalloc(pdata->nr_wins *
			sizeof(struct fb_info *), GFP_KERNEL);
	if (!fbdev->fb) {
		dev_err(fbdev->dev, "not enough memory\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	/* alloc for each framebuffer */
	for (i = 0; i < pdata->nr_wins; i++) {
		fbdev->fb[i] = framebuffer_alloc(sizeof(struct s3cfb_window),
				fbdev->dev);
		if (!fbdev->fb[i]) {
			dev_err(fbdev->dev, "not enough memory\n");
			ret = -ENOMEM;
			goto err_alloc_fb;
		}

		ret = s3cfb_init_fbinfo(i);
		if (ret) {
			dev_err(fbdev->dev,
				"failed to allocate memory for fb%d\n", i);
			ret = -ENOMEM;
			goto err_alloc_fb;
		}

		if (i == pdata->default_win) {
			if (s3cfb_map_video_memory(fbdev->fb[i])) {
				dev_err(fbdev->dev,
					"failed to map video memory "
					"for default window (%d)\n", i);
				ret = -ENOMEM;
				goto err_alloc_fb;
			}
		}
	}

	return 0;

err_alloc_fb:
	for (i = 0; i < pdata->nr_wins; i++) {
		if (fbdev->fb[i])
			framebuffer_release(fbdev->fb[i]);
	}

	kfree(fbdev->fb);

err_alloc:
	return ret;
}

int s3cfb_register_framebuffer(void)
{
	struct s3c_platform_fb *pdata = to_fb_plat(fbdev->dev);
	int ret, i;

	for (i = 0; i < pdata->nr_wins; i++) {
		ret = register_framebuffer(fbdev->fb[i]);
		if (ret) {
			dev_err(fbdev->dev, "failed to register "
				"framebuffer device\n");
			return -EINVAL;
		}

#ifndef CONFIG_FRAMEBUFFER_CONSOLE
		if (i == pdata->default_win) {
			s3cfb_check_var(&fbdev->fb[i]->var, fbdev->fb[i]);
			// We have to draw logo image before calling s3cfb_set_par, 
			// because s3cfb_set_par will write new framebuffer address.
			// Otherwise we can see blank screen for a little moment.
			s3cfb_draw_logo(fbdev->fb[i]);
			s3cfb_set_par  (fbdev->fb[i]);
		}
#endif
	}

	return 0;
}

static int s3cfb_sysfs_show_win_power(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct s3c_platform_fb *pdata = to_fb_plat(dev);
	struct s3cfb_window *win;
	char temp[16];
	int i;

	for (i = 0; i < pdata->nr_wins; i++) {
		win = fbdev->fb[i]->par;
		sprintf(temp, "[fb%d] %s\n", i, win->enabled ? "on" : "off");
		strcat(buf, temp);
	}

	return strlen(buf);
}

static int s3cfb_sysfs_store_win_power(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t len)
{
	struct s3c_platform_fb *pdata = to_fb_plat(dev);
	char temp[4] = {0, };
	const char *p = buf;
	int id, to;

	while (*p != '\0') {
		if (!isspace(*p))
			strncat(temp, p, 1);
		p++;
	}

	if (strlen(temp) != 2)
		return -EINVAL;

	strict_strtoul((const char *)temp, 10, (unsigned long *)&id);
	strict_strtoul((const char *)temp, 10, (unsigned long *)&to);

	id /= 10;
	to &= 10;

	if (id < 0 || id > pdata->nr_wins)
		return -EINVAL;

	if (to != 0 && to != 1)
		return -EINVAL;

	if (to == 0)
		s3cfb_disable_window(id);
	else
		s3cfb_enable_window(id);

	return len;
}

static DEVICE_ATTR(win_power, 0644,
		   s3cfb_sysfs_show_win_power, s3cfb_sysfs_store_win_power);
#if defined(CONFIG_FB_S3C_LMS300)||defined(CONFIG_FB_S3C_S6D04D1)
/* sysfs export of baclight control */
static int s3cfb_sysfs_show_lcd_power(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", s3cfb_get_lcd_power());
}

static int s3cfb_sysfs_store_lcd_power(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
	if (len < 1)
		return -EINVAL;

	if (strnicmp(buf, "on", 2) == 0 || strnicmp(buf, "1", 1) == 0)
		s3cfb_set_lcd_power(1);
	else if (strnicmp(buf, "off", 3) == 0 || strnicmp(buf, "0", 1) == 0)
		s3cfb_set_lcd_power(0);
	else
		return -EINVAL;

	return len;
}

static int s3cfb_sysfs_show_backlight_power(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", s3cfb_get_backlight_power());
}

static int s3cfb_sysfs_store_backlight_power(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
	if (len < 1)
		return -EINVAL;

	if (strnicmp(buf, "on", 2) == 0 || strnicmp(buf, "1", 1) == 0)
		s3cfb_set_backlight_power(1);
	else if (strnicmp(buf, "off", 3) == 0 || strnicmp(buf, "0", 1) == 0)
		s3cfb_set_backlight_power(0);
	else
		return -EINVAL;

	return len;
}

static int s3cfb_sysfs_show_backlight_level(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", s3cfb_get_backlight_level());
}

static int s3cfb_sysfs_store_backlight_level(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
	unsigned long value = simple_strtoul(buf, NULL, 10);

	s3cfb_set_backlight_level(value);

	return len;
}

static DEVICE_ATTR(lcd_power, 0666,						//sm.kim: give write permission for application
                   s3cfb_sysfs_show_lcd_power,
                   s3cfb_sysfs_store_lcd_power);

static DEVICE_ATTR(backlight_power, 0644,
                   s3cfb_sysfs_show_backlight_power,
                   s3cfb_sysfs_store_backlight_power);

static DEVICE_ATTR(backlight_level, 0644,
                   s3cfb_sysfs_show_backlight_level,
                   s3cfb_sysfs_store_backlight_level);

#endif

struct early_suspend * p_earlysuspend = NULL;

static int s3cfb_probe(struct platform_device *pdev)
{
	struct s3c_platform_fb *pdata;
	struct resource *res;
	int ret = 0;

	/* initialzing global structure */
	fbdev = kzalloc(sizeof(struct s3cfb_global), GFP_KERNEL);
	if (!fbdev) {
		dev_err(fbdev->dev, "failed to allocate for "
			"global fb structure\n");
		goto err_global;
	}

	fbdev->dev = &pdev->dev;
	s3cfb_set_lcd_info(fbdev);

	/* gpio */
	pdata = to_fb_plat(&pdev->dev);
	if (pdata->cfg_gpio)
		pdata->cfg_gpio(pdev);

	/* clock */
	fbdev->clock = clk_get(&pdev->dev, pdata->clk_name);
	if (IS_ERR(fbdev->clock)) {
		dev_err(fbdev->dev, "failed to get fimd clock source\n");
		ret = -EINVAL;
		goto err_clk;
	}

	clk_enable(fbdev->clock);

	/* io memory */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(fbdev->dev, "failed to get io memory region\n");
		ret = -EINVAL;
		goto err_io;
	}

	/* request mem region */
	res = request_mem_region(res->start,
		res->end - res->start + 1, pdev->name);
	if (!res) {
		dev_err(fbdev->dev, "failed to request io memory region\n");
		ret = -EINVAL;
		goto err_io;
	}

	/* ioremap for register block */
	fbdev->regs = ioremap(res->start, res->end - res->start + 1);
	if (!fbdev->regs) {
		dev_err(fbdev->dev, "failed to remap io region\n");
		ret = -EINVAL;
		goto err_io;
	}

	/* init global */
	s3cfb_init_global();	
	
	/* irq */
	fbdev->irq = platform_get_irq(pdev, 0);
	if (request_irq(fbdev->irq, s3cfb_irq_frame, IRQF_DISABLED,
				pdev->name, fbdev)) {
		dev_err(fbdev->dev, "request_irq failed\n");
		ret = -EINVAL;
		goto err_irq;
	}
#if 1
	// added by jamie (2009.08.18)
	// enable VSYNC
	s3cfb_set_vsync_interrupt(fbdev, 1);
	s3cfb_set_global_interrupt(fbdev, 1);
#endif

#ifdef CONFIG_FB_S3C_TRACE_UNDERRUN
	if (request_irq(platform_get_irq(pdev, 1), s3cfb_irq_fifo,
			IRQF_DISABLED, pdev->name, fbdev)) {
		dev_err(fbdev->dev, "request_irq failed\n");
		ret = -EINVAL;
		goto err_irq;
	}

	s3cfb_set_fifo_interrupt(fbdev, 1);
	dev_info(fbdev->dev, "fifo underrun trace\n");
#endif

	/* init global */
	// wait queue initialization must be done before enabling interrupt
	//s3cfb_init_global();	

	/* prepare memory */
	if (s3cfb_alloc_framebuffer())
		goto err_alloc;

	if (s3cfb_register_framebuffer())
		goto err_alloc;

#ifndef CONFIG_FRAMEBUFFER_CONSOLE

#endif 

	frame_buf_mark.p_fb = fbdev->fb[0]->fix.smem_start;
	frame_buf_mark.bpp = fbdev->fb[0]->var.bits_per_pixel;
	s3cfb_set_clock(fbdev);
	s3cfb_enable_window(pdata->default_win);

	s3cfb_display_on(fbdev);

//sm.kim: draw progress bar
#ifndef CONFIG_FRAMEBUFFER_CONSOLE
	s3cfb_start_progress(fbdev, s3cfb_set_par, s3cfb_release_window);
#endif 

	/* panel control */
	if (pdata->backlight_on)
		pdata->backlight_on(pdev);

	if (pdata->reset_lcd)
		pdata->reset_lcd(pdev);

	if (fbdev->lcd->init_ldi)
		fbdev->lcd->init_ldi();

#ifdef CONFIG_HAS_EARLYSUSPEND

        pdata->early_suspend.suspend = s3cfb_early_suspend;
        pdata->early_suspend.resume  = s3cfb_late_resume;
        pdata->early_suspend.level   = EARLY_SUSPEND_LEVEL_DISABLE_FB;

        register_early_suspend(&pdata->early_suspend);

		p_earlysuspend = &pdata->early_suspend;

#endif  /* CONFIG_HAS_EARLYSUSPEND */


	ret = device_create_file(&(pdev->dev), &dev_attr_win_power);
	if (ret < 0)
		dev_err(fbdev->dev, "failed to add sysfs entries\n");
	
#if defined(CONFIG_FB_S3C_LMS300)||defined(CONFIG_FB_S3C_S6D04D1)
	 /* create device files */
        ret = device_create_file(&(pdev->dev), &dev_attr_backlight_power);
        if (ret < 0)
                printk(KERN_WARNING "s3cfb: failed to add entries\n");

        ret = device_create_file(&(pdev->dev), &dev_attr_backlight_level);
        if (ret < 0)
                printk(KERN_WARNING "s3cfb: failed to add entries\n");

        ret = device_create_file(&(pdev->dev), &dev_attr_lcd_power);
        if (ret < 0)
                printk(KERN_WARNING "s3cfb: failed to add entries\n");
#endif

	dev_info(fbdev->dev, "registered successfully\n");

	return 0;

err_alloc:
	free_irq(fbdev->irq, fbdev);

err_irq:
	iounmap(fbdev->regs);

err_io:
	clk_disable(fbdev->clock);

err_clk:
	clk_put(fbdev->clock);

err_global:
	return ret;
}

static int s3cfb_remove(struct platform_device *pdev)
{
	struct s3c_platform_fb *pdata = to_fb_plat(&pdev->dev);
	struct fb_info *fb;
	int i;

#ifdef CONFIG_HAS_EARLYSUSPEND

	unregister_early_suspend(&pdata->early_suspend);

#endif  /* CONFIG_HAS_EARLYSUSPEND */

	free_irq(fbdev->irq, fbdev);
	iounmap(fbdev->regs);
	clk_disable(fbdev->clock);
	clk_put(fbdev->clock);

	for (i = 0; i < pdata->nr_wins; i++) {
		fb = fbdev->fb[i];

		/* free if exists */
		if (fb) {
			s3cfb_unmap_video_memory(fb);
			s3cfb_set_buffer_address(fbdev, i);
			framebuffer_release(fb);
		}
	}

	kfree(fbdev->fb);
	kfree(fbdev);

	return 0;
}

#ifdef CONFIG_PM
//static int lcd_pm_state = 1;
int lcd_pm_state = 1;
EXPORT_SYMBOL(lcd_pm_state);		//referenced by LCD driver 
#ifdef CONFIG_HAS_EARLYSUSPEND

static int s3cfb_suspend_sub(struct s3c_platform_fb *pdata)
{
	struct s3cfb_window *win;
	int i;

	for (i = 0; i < pdata->nr_wins; i++) {
		win = fbdev->fb[i]->par;
		if (win->path != DATA_PATH_DMA && win->suspend_fifo) {
			if (win->suspend_fifo()) {
				dev_info(fbdev->dev, "failed to run "
					 "the suspend for fifo\n");
				return -1;
			}
		}
	}
	
	s3cfb_display_off(fbdev);
	clk_disable(fbdev->clock);

	//[sm.kim: LCD on/off is controlled by platform
	//s3cfb_set_backlight_power(OFF);
	//s3cfb_set_lcd_power(OFF);
	//]

#ifdef S5P6442_POWER_GATING_LCD
	s5p6442_pwrgate_config(S5P6442_LCD_ID, S5P6442_LP_MODE);
	s5p6442_idle_pm_gpiocfg(S5P6442_LCD_ID, S5P6442_LP_MODE);
#endif /* S5P6442_POWER_GATING_LCD */

	return 0;
}

static int s3cfb_resume_sub(struct s3c_platform_fb *pdata)
{
	struct fb_info *fb;
	struct s3cfb_window *win;
	int i;

#ifdef S5P6442_POWER_GATING_LCD
	s5p6442_pwrgate_config(S5P6442_LCD_ID, S5P6442_ACTIVE_MODE);
#endif /* S5P6442_POWER_GATING_LCD */

	dev_dbg(fbdev->dev, "wake up from suspend\n");

	if (pdata->cfg_gpio)
		pdata->cfg_gpio(NULL);

	if (pdata->backlight_on)
		pdata->backlight_on(NULL);

	if (pdata->reset_lcd)
		pdata->reset_lcd(NULL);

	if (fbdev->lcd->init_ldi)
		fbdev->lcd->init_ldi();

	clk_enable(fbdev->clock);
	s3cfb_init_global();
	s3cfb_set_clock(fbdev);
	s3cfb_display_on(fbdev);

	for (i = 0; i < pdata->nr_wins; i++) {
		fb = fbdev->fb[i];
		win = fb->par;

		if (win->path != DATA_PATH_DMA && win->resume_fifo) {
			if (win->resume_fifo()) {
				dev_info(fbdev->dev, 
					"failed to run the resume for fifo\n");
				return -1;
			}
		} else {
			if (win->enabled) {
				s3cfb_check_var(&fb->var, fb);
				s3cfb_set_par(fb);
				s3cfb_enable_window(win->id);
			}
		}
	}
	/* enable VSYNC */
	s3cfb_set_vsync_interrupt (fbdev, 1);
	s3cfb_set_global_interrupt(fbdev, 1);

	//[sm.kim: LCD on/off is controlled by platform
	//s3cfb_set_lcd_power(ON);
	//s3cfb_set_backlight_power(ON);
	//]

	return 0;
}

int s3cfb_early_suspend(struct early_suspend *h)
{
	struct s3c_platform_fb *pdata = container_of(h, struct s3c_platform_fb, early_suspend);
	int ret;

	//printk("#%s\n", __func__);
	dev_dbg(fbdev->dev, "# %s\n", __func__);
	

	ret = s3cfb_suspend_sub(pdata);
	if (ret < 0) {
		printk(KERN_ERR "%s error occur\n", __func__);
	}
	lcd_pm_state = 0;

	return 0;
}

int s3cfb_late_resume(struct early_suspend *h)
{
	struct s3c_platform_fb *pdata = container_of(h, struct s3c_platform_fb, early_suspend);
	int ret;

	//printk("#%s\n", __func__);
	dev_dbg(fbdev->dev, "# %s\n", __func__);

	if (lcd_pm_state == 0) {
		ret = s3cfb_resume_sub(pdata);
		if (ret < 0) {
			printk(KERN_ERR "%s error occur\n", __func__);
		}
	}

	lcd_pm_state = 1;
	
	return 0;
}

int s3cfb_force_resume()
{
	if(p_earlysuspend)
		{
		s3cfb_late_resume(p_earlysuspend);
		}
}
EXPORT_SYMBOL(s3cfb_force_resume);

#endif /* CONFIG_HAS_EARLYSUSPEND */

int s3cfb_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct s3c_platform_fb *pdata = to_fb_plat(&pdev->dev);
	int ret;

	//printk("#%s\n", __func__);
	dev_dbg(fbdev->dev, "# %s\n", __func__);

	if(lcd_pm_state != 0) {
		ret = s3cfb_suspend_sub(pdata);
		if (ret < 0) {
			printk(KERN_ERR "%s error occur\n", __func__);
		}
	}

	lcd_pm_state = 0;

	return 0;
}

int s3cfb_resume(struct platform_device *pdev)
{
	struct s3c_platform_fb *pdata = to_fb_plat(&pdev->dev);
	int ret;

	//printk("#%s\n", __func__);
	dev_dbg(fbdev->dev, "# %s\n", __func__);

	ret = s3cfb_resume_sub(pdata);
	if (ret < 0) {
		printk(KERN_ERR "%s error occur\n", __func__);
	}

	lcd_pm_state = 1;

	return 0;
}

int s3cfb_shutdown(struct platform_device *pdev)
{
	/* to do */
	// lcd_power_ctrl(0);
	return 0;
}
#else
#define s3cfb_suspend  NULL
#define s3cfb_resume   NULL
#define s3cfb_shutdown NULL
#endif /* CONFIG_PM */

static struct platform_driver s3cfb_driver = {
	.probe		= s3cfb_probe,
	.remove		= s3cfb_remove,
	.suspend	= s3cfb_suspend,
	.resume		= s3cfb_resume,
	.shutdown	= s3cfb_shutdown,
	.driver		= {
		.name	= S3CFB_NAME,
		.owner	= THIS_MODULE,
	},
};

static int s3cfb_register(void)
{
	platform_driver_register(&s3cfb_driver);

	return 0;
}

static void s3cfb_unregister(void)
{
	platform_driver_unregister(&s3cfb_driver);
}

module_init(s3cfb_register);
module_exit(s3cfb_unregister);
	
MODULE_AUTHOR("Jinsung, Yang <jsgood.yang@samsung.com>");
MODULE_DESCRIPTION("Samsung Display Controller (FIMD) driver");
MODULE_LICENSE("GPL");


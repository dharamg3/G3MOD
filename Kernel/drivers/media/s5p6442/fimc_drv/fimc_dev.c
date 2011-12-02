/* linux/drivers/media/video/samsung/fimc_core.c
 *
 * Core file for Samsung Camera Interface (FIMC) driver
 *
 * Dongsoo Kim, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsung.com/sec/
 * Jinsung Yang, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 * Jonghun Han, Copyright (c) 2009 Samsung Electronics
 * 	http://www.samsungsemi.com/
 *
 * Note: This driver supports common i2c client driver style
 * which uses i2c_board_info for backward compatibility and
 * new v4l2_subdev as well.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/clk.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/fs.h>
#include <linux/irq.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <media/v4l2-device.h>
#include <linux/io.h>
#include <linux/memory.h>
#include <plat/clock.h>
#include <plat/media.h>
#include <plat/fimc.h>
#include <plat/power_clk_gating.h>
#include <plat/reserved_mem.h>
#include <plat/regs-gpio.h>
#include "fimc.h"
#include <plat/s5p6442-dvfs.h>
#define CAM_ID	0

struct fimc_global *fimc_dev;
struct s3c_platform_camera cam_struct_g;
#ifdef S5P6442_POWER_GATING_CAM
static struct timer_list  g_fimc_domain_timer;
static int fimc_timer_init_flag = 0;
DEFINE_SPINLOCK(fimc_domain_lock);
#endif

//#define VIEW_FUNCTION_CALL

int fimc_dma_alloc(struct fimc_control *ctrl, struct fimc_buf_set *bs, int i, int align)
{
#ifdef VIEW_FUNCTION_CALL
	printk("[FIMC_DEV] %s(%d)\n", __func__, __LINE__);
#endif

	dma_addr_t end, *curr;

	mutex_lock(&ctrl->lock);

	end = ctrl->mem.base + ctrl->mem.size;
	curr = &ctrl->mem.curr;

	if (!bs->length[i])
	{
		mutex_unlock(&ctrl->lock);
		return -EINVAL;
	}

	if (!align) {
		if (*curr + bs->length[i] > end) {
			goto overflow;
		} else {
			bs->base[i] = *curr;
			bs->garbage[i] = 0;
			*curr += bs->length[i];
		}
	} else {
		if (ALIGN(*curr, align) + bs->length[i] > end)
			goto overflow;
		else {
			bs->base[i] = ALIGN(*curr, align);
			bs->garbage[i] = ALIGN(*curr, align) - *curr;
			*curr += (bs->length[i] + bs->garbage[i]);
		}
	}

	mutex_unlock(&ctrl->lock);

	return 0;

overflow:
	bs->base[i] = 0;
	bs->length[i] = 0;
	bs->garbage[i] = 0;

	mutex_unlock(&ctrl->lock);

	return -ENOMEM;
}

void fimc_dma_free(struct fimc_control *ctrl, struct fimc_buf_set *bs, int i)
{
	int total = bs->length[i] + bs->garbage[i];
	mutex_lock(&ctrl->lock);
#ifdef VIEW_FUNCTION_CALL
	printk("[FIMC_DEV] %s(%d)\n", __func__, __LINE__);
#endif

	if (bs->base[i]) {
		if (ctrl->mem.base <= ctrl->mem.curr - total)
		{
			ctrl->mem.curr -= total;
			//printk("### free1 ctrl->mem.curr : %d\n", ctrl->mem.curr);
		}
		// kcoolsw(09.11.13)
		else // if (ctrl->mem.curr - total < ctrl->mem.base)
		{
			ctrl->mem.curr = ctrl->mem.base;
			//printk("### free2 ctrl->mem.curr : %d\n", ctrl->mem.curr);
		}

		bs->base[i] = 0;
		bs->length[i] = 0;
		bs->garbage[i] = 0;
	}
	/*
	else 
		printk("### free3 no free damn..%d \n", total);
	*/

	mutex_unlock(&ctrl->lock);
}

static inline u32 fimc_irq_out_dma(struct fimc_control *ctrl)
{
	u32 next = 0, wakeup = 1;
	int ret = -1;
#ifdef VIEW_FUNCTION_CALL
	printk("[FIMC_DEV] %s(%d)\n", __func__, __LINE__);
#endif

	/* Attach done buffer to outgoing queue. */
	ret = fimc_attach_out_queue(ctrl, ctrl->out->idx.active);
	if (ret < 0)
		dev_err(ctrl->dev, "Failed: fimc_attach_out_queue\n");

	if (ctrl->status == FIMC_READY_OFF) {
		ctrl->out->idx.active = -1;
		return wakeup;
	}

	/* Detach buffer from incomming queue. */
	ret =  fimc_detach_in_queue(ctrl, &next);
	if (ret == 0) {	/* There is a buffer in incomming queue. */
		fimc_outdev_set_src_addr(ctrl, ctrl->out->buf[next].base);
		ret = fimc_outdev_start_camif(ctrl);
		if (ret < 0)
			dev_err(ctrl->dev, "Fail: fimc_start_camif\n");

		ctrl->out->idx.active = next;
		ctrl->status = FIMC_STREAMON;
	} else {	/* There is no buffer in incomming queue. */
		ctrl->out->idx.active = -1;
		ctrl->status = FIMC_STREAMON_IDLE;
	}

	return wakeup;
}

static inline u32 fimc_irq_out_fimd(struct fimc_control *ctrl)
{
	u32 prev, next, wakeup = 0;
	int ret = -1;
#ifdef VIEW_FUNCTION_CALL
	printk("[FIMC_DEV] %s(%d)\n", __func__, __LINE__);
#endif

	/* Attach done buffer to outgoing queue. */
	if (ctrl->out->idx.prev != -1) {
		ret = fimc_attach_out_queue(ctrl, ctrl->out->idx.prev);
		if (ret < 0) {
			dev_err(ctrl->dev,
				"Failed: fimc_attach_out_queue\n");
		} else {
			ctrl->out->idx.prev = -1;
			wakeup = 1; /* To wake up fimc_v4l2_dqbuf(). */
		}
	}

	/* Update index structure. */
	if (ctrl->out->idx.next != -1) {
		ctrl->out->idx.active	= ctrl->out->idx.next;
		ctrl->out->idx.next	= -1;
	}

	/* Detach buffer from incomming queue. */
	ret =  fimc_detach_in_queue(ctrl, &next);
	if (ret == 0) {	/* There is a buffer in incomming queue. */
		prev = ctrl->out->idx.active;
		ctrl->out->idx.prev	= prev;
		ctrl->out->idx.next	= next;

		/* Set the address */
		fimc_outdev_set_src_addr(ctrl, ctrl->out->buf[next].base);
	}

	return wakeup;
}

static inline void fimc_irq_out(struct fimc_control *ctrl)
{
	u32 wakeup = 1;
#ifdef VIEW_FUNCTION_CALL
	printk("[FIMC_DEV] %s(%d)\n", __func__, __LINE__);
#endif

	/* Interrupt pendding clear */
	fimc_hwset_clear_irq(ctrl);

	if (ctrl->out->fbuf.base)
		wakeup = fimc_irq_out_dma(ctrl);
	else if (ctrl->status != FIMC_READY_OFF)
		wakeup = fimc_irq_out_fimd(ctrl);

	if (wakeup == 1)
		wake_up_interruptible(&ctrl->wq);
}

#define S5K4CA_SKIP_FRAMES
#ifdef S5K4CA_SKIP_FRAMES
unsigned int s5k4ca_skip_green_frame;
#define FRAMES_TO_SKIP 7
#endif
static inline void fimc_irq_cap(struct fimc_control *ctrl)
{
	struct fimc_capinfo *cap = ctrl->cap;
	int pp;
#ifdef VIEW_FUNCTION_CALL
	printk("[FIMC_DEV] %s(%d)\n", __func__, __LINE__);
#endif

	fimc_hwset_clear_irq(ctrl);
	fimc_hwget_overflow_state(ctrl);
	pp = ((fimc_hwget_frame_count(ctrl) + 2) % 4);
	if (cap->fmt.field == V4L2_FIELD_INTERLACED_TB) {
		/* odd value of pp means one frame is made with top/bottom */
		if (pp & 0x1) {	
			cap->irq = 1;
			wake_up_interruptible(&ctrl->wq);
		}
	}
	else {
#ifdef S5K4CA_SKIP_FRAMES
		if (unlikely(s5k4ca_skip_green_frame < FRAMES_TO_SKIP)) {
			s5k4ca_skip_green_frame++;
			if(s5k4ca_skip_green_frame == 7)
			{
				printk("s5k4ca_skip_green_frame = %d\n", s5k4ca_skip_green_frame);			
			}
		} else {
			cap->irq = 1;
			wake_up_interruptible(&ctrl->wq);
		}
#else
		cap->irq = 1;
		wake_up_interruptible(&ctrl->wq);
#endif
	}	
	
}

static irqreturn_t fimc_irq(int irq, void *dev_id)
{
	struct fimc_control *ctrl = (struct fimc_control *) dev_id;
#ifdef VIEW_FUNCTION_CALL
	printk("[FIMC_DEV] %s(%d)\n", __func__, __LINE__);
#endif

	if (ctrl->cap)
		fimc_irq_cap(ctrl);
	else if (ctrl->out)
		fimc_irq_out(ctrl);

	return IRQ_HANDLED;
}

static
struct fimc_control *fimc_register_controller(struct platform_device *pdev)
{
	struct s3c_platform_fimc *pdata;
	struct fimc_control *ctrl;
	struct resource *res;
	int id, mdev_id, irq;
#ifdef VIEW_FUNCTION_CALL
	printk("[FIMC_DEV] %s(%d)\n", __func__, __LINE__);
#endif

	id = pdev->id;
	mdev_id = S3C_MDEV_FIMC0 + id;
	pdata = to_fimc_plat(&pdev->dev);

	ctrl = get_fimc_ctrl(id);
	ctrl->id = id;
	ctrl->dev = &pdev->dev;
	ctrl->vd = &fimc_video_device[id];
	ctrl->vd->minor = id;

	/* alloc from bank1 as default */
#if 0
	ctrl->mem.base = s3c_get_media_memory_node(mdev_id, 1);
	ctrl->mem.size = s3c_get_media_memsize_node(mdev_id, 1);
#else
	if(id == 0){
	   ctrl->mem.base = FIMC0_RESERVED_MEM_START;
	   ctrl->mem.size = RESERVED_MEM_FIMC0;
	}
	else if(id == 1){
	   ctrl->mem.base = FIMC1_RESERVED_MEM_START;
           ctrl->mem.size = RESERVED_MEM_FIMC1;
	}
	else if(id == 2){
	   ctrl->mem.base = FIMC2_RESERVED_MEM_START;
           ctrl->mem.size = RESERVED_MEM_FIMC2;
        }
#endif
	ctrl->mem.curr = ctrl->mem.base;

	ctrl->status = FIMC_STREAMOFF;
	ctrl->limit = &fimc_limits[id];

	sprintf(ctrl->name, "%s%d", FIMC_NAME, id);
	strcpy(ctrl->vd->name, ctrl->name);

	atomic_set(&ctrl->in_use, 0);
	mutex_init(&ctrl->lock);
	mutex_init(&ctrl->v4l2_lock);
	spin_lock_init(&ctrl->lock_in);
	spin_lock_init(&ctrl->lock_out);
	init_waitqueue_head(&ctrl->wq);

	/* get resource for io memory */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(ctrl->dev, "%s: failed to get io memory region\n",
			__func__);
		return NULL;
	}

	/* request mem region */
	res = request_mem_region(res->start, res->end - res->start + 1,
			pdev->name);
	if (!res) {
		dev_err(ctrl->dev, "%s: failed to request io memory region\n",
			__func__);
		return NULL;
	}

	/* ioremap for register block */
	ctrl->regs = ioremap(res->start, res->end - res->start + 1);
	if (!ctrl->regs) {
		dev_err(ctrl->dev, "%s: failed to remap io region\n",
			__func__);
		return NULL;
	}

	/* irq */
	irq = platform_get_irq(pdev, 0);
	if (request_irq(irq, fimc_irq, IRQF_DISABLED, ctrl->name, ctrl))
		dev_err(ctrl->dev, "%s: request_irq failed\n", __func__);

	fimc_hwset_reset(ctrl);

	return ctrl;
}

static int fimc_unregister_controller(struct platform_device *pdev)
{
	struct s3c_platform_fimc *pdata;
	struct fimc_control *ctrl;
	int id = pdev->id;
#ifdef VIEW_FUNCTION_CALL
	printk("[FIMC_DEV] %s(%d)\n", __func__, __LINE__);
#endif

	pdata = to_fimc_plat(&pdev->dev);
	ctrl = get_fimc_ctrl(id);
	
	if (pdata->clk_off)
		pdata->clk_off(pdev, ctrl->clk);

	iounmap(ctrl->regs);
	memset(ctrl, 0, sizeof(*ctrl));

	return 0;
}

static void fimc_mmap_open(struct vm_area_struct *vma)
{
	struct fimc_global *dev = fimc_dev;
	int pri_data	= (int)vma->vm_private_data;
	u32 id		= (pri_data / 0x10);
	u32 idx		= (pri_data % 0x10);
#ifdef VIEW_FUNCTION_CALL
	printk("[FIMC_DEV] %s(%d)\n", __func__, __LINE__);
#endif

	atomic_inc(&dev->ctrl[id].out->buf[idx].mapped_cnt);
}

static void fimc_mmap_close(struct vm_area_struct *vma)
{
	struct fimc_global *dev = fimc_dev;
	int pri_data	= (int)vma->vm_private_data;
	u32 id		= (pri_data / 0x10);
	u32 idx		= (pri_data % 0x10);
#ifdef VIEW_FUNCTION_CALL
	printk("[FIMC_DEV] %s(%d)\n", __func__, __LINE__);
#endif

	atomic_dec(&dev->ctrl[id].out->buf[idx].mapped_cnt);
}

static struct vm_operations_struct fimc_mmap_ops = {
	.open	= fimc_mmap_open,
	.close	= fimc_mmap_close,
};

static inline int fimc_mmap_out(struct file *filp, struct vm_area_struct *vma)
{
	struct fimc_control *ctrl = filp->private_data;
	u32 start_phy_addr = 0;
	u32 size = vma->vm_end - vma->vm_start;
	u32 pfn, idx = vma->vm_pgoff;
	u32 buf_length = 0;
	int pri_data = 0;
#ifdef VIEW_FUNCTION_CALL
	printk("[FIMC_DEV] %s(%d)\n", __func__, __LINE__);
#endif

	buf_length = ctrl->out->buf[idx].length[FIMC_ADDR_Y] + \
				ctrl->out->buf[idx].length[FIMC_ADDR_CB] + \
				ctrl->out->buf[idx].length[FIMC_ADDR_CR];
	if (size > buf_length) {
		dev_err(ctrl->dev, "Requested mmap size is too big\n");
		return -EINVAL;
	}

	pri_data = (ctrl->id * 0x10) + idx;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	vma->vm_flags |= VM_RESERVED;
	vma->vm_ops = &fimc_mmap_ops;
	vma->vm_private_data = (void *)pri_data;

	if ((vma->vm_flags & VM_WRITE) && !(vma->vm_flags & VM_SHARED)) {
		dev_err(ctrl->dev, "writable mapping must be shared\n");
		return -EINVAL;
	}

	start_phy_addr = ctrl->out->buf[idx].base[FIMC_ADDR_Y];
	pfn = __phys_to_pfn(start_phy_addr);

	if (remap_pfn_range(vma, vma->vm_start, pfn, size,
						vma->vm_page_prot)) {
		dev_err(ctrl->dev, "mmap fail\n");
		return -EINVAL;
	}

	vma->vm_ops->open(vma);

	ctrl->out->buf[idx].flags |= V4L2_BUF_FLAG_MAPPED;

	return 0;
}

static inline int fimc_mmap_cap(struct file *filp, struct vm_area_struct *vma)
{
	struct fimc_control *ctrl = filp->private_data;
	u32 size = vma->vm_end - vma->vm_start;
	u32 pfn, idx = vma->vm_pgoff;
#ifdef VIEW_FUNCTION_CALL
	printk("[FIMC_DEV] %s(%d)\n", __func__, __LINE__);
#endif

	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	vma->vm_flags |= VM_RESERVED;

	/*
	 * page frame number of the address for a source frame
	 * to be stored at.
	 */
	pfn = __phys_to_pfn(ctrl->cap->bufs[idx].base[0]);

	if ((vma->vm_flags & VM_WRITE) && !(vma->vm_flags & VM_SHARED)) {
		dev_err(ctrl->dev, "%s: writable mapping must be shared\n",
			__func__);
		return -EINVAL;
	}

	if (remap_pfn_range(vma, vma->vm_start, pfn, size, vma->vm_page_prot)) {
		dev_err(ctrl->dev, "%s: mmap fail\n", __func__);
		return -EINVAL;
	}

	return 0;
}

static int fimc_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct fimc_control *ctrl = filp->private_data;
	int ret;
#ifdef VIEW_FUNCTION_CALL
	printk("[FIMC_DEV] %s(%d)\n", __func__, __LINE__);
#endif

	if (ctrl->cap)
		ret = fimc_mmap_cap(filp, vma);
	else
		ret = fimc_mmap_out(filp, vma);

	return ret;
}

static u32 fimc_poll(struct file *filp, poll_table *wait)
{
	struct fimc_control *ctrl = filp->private_data;
	struct fimc_capinfo *cap = ctrl->cap;
	u32 mask = 0;
#ifdef VIEW_FUNCTION_CALL
	printk("[FIMC_DEV] %s(%d)\n", __func__, __LINE__);
#endif

	if (cap) {
		if (cap->irq) {
			mask = POLLIN | POLLRDNORM;
			cap->irq = 0;
		} else {
			poll_wait(filp, &ctrl->wq, wait);
		}
	}

	return mask;
}

static
ssize_t fimc_read(struct file *filp, char *buf, size_t count, loff_t *pos)
{
#ifdef VIEW_FUNCTION_CALL
	printk("[FIMC_DEV] %s(%d)\n", __func__, __LINE__);
#endif

	return 0;
}

static
ssize_t fimc_write(struct file *filp, const char *b, size_t c, loff_t *offset)
{
	#ifdef VIEW_FUNCTION_CALL
	printk("[FIMC_DEV] %s(%d)\n", __func__, __LINE__);
#endif

	return 0;
}

u32 fimc_mapping_rot_flip(u32 rot, u32 flip)
{
	u32 ret = 0;
#ifdef VIEW_FUNCTION_CALL
	printk("[FIMC_DEV] %s(%d)\n", __func__, __LINE__);
#endif

	switch (rot) {
	case 0:
		if (flip & V4L2_CID_HFLIP)
			ret |= FIMC_XFLIP;

		if (flip & V4L2_CID_VFLIP)
			ret |= FIMC_YFLIP;
		break;

	case 90:
		ret = FIMC_ROT;
		if (flip & V4L2_CID_HFLIP)
			ret |= FIMC_XFLIP;

		if (flip & V4L2_CID_VFLIP)
			ret |= FIMC_YFLIP;
		break;

	case 180:
		ret = (FIMC_XFLIP | FIMC_YFLIP);
		if (flip & V4L2_CID_HFLIP)
			ret &= ~FIMC_XFLIP;

		if (flip & V4L2_CID_VFLIP)
			ret &= ~FIMC_YFLIP;
		break;

	case 270:
		ret = (FIMC_XFLIP | FIMC_YFLIP | FIMC_ROT);
		if (flip & V4L2_CID_HFLIP)
			ret &= ~FIMC_XFLIP;

		if (flip & V4L2_CID_VFLIP)
			ret &= ~FIMC_YFLIP;
		break;
	}

	return ret;
}

int fimc_get_scaler_factor(u32 src, u32 tar, u32 *ratio, u32 *shift)
{
	#ifdef VIEW_FUNCTION_CALL
	printk("[FIMC_DEV] %s(%d)\n", __func__, __LINE__);
#endif

	if (src >= tar * 64) {
		return -EINVAL;
	} else if (src >= tar * 32) {
		*ratio = 32;
		*shift = 5;
	} else if (src >= tar * 16) {
		*ratio = 16;
		*shift = 4;
	} else if (src >= tar * 8) {
		*ratio = 8;
		*shift = 3;
	} else if (src >= tar * 4) {
		*ratio = 4;
		*shift = 2;
	} else if (src >= tar * 2) {
		*ratio = 2;
		*shift = 1;
	} else {
		*ratio = 1;
		*shift = 0;
	}

	return 0;
}

#if 0
static int fimc_wakeup_fifo(struct fimc_control *ctrl)
{
	int ret = -1;

	/* Set the rot, pp param register. */
	ret = fimc_check_param(ctrl);
	if (ret < 0) {
		dev_err(ctrl->dev, "fimc_check_param failed\n");
		return -EINVAL;
	}

	if (ctrl->rot.degree != 0) {
		ret = s3c_rp_rot_set_param(ctrl);
		if (ret < 0) {
			rp_err(ctrl->log_level,
					"s3c_rp_rot_set_param failed\n");
			return -1;
		}
	}

	ret = s3c_rp_pp_set_param(ctrl);
	if (ret < 0) {
		rp_err(ctrl->log_level, "s3c_rp_pp_set_param failed\n");
		return -1;
	}

	/* Start PP */
	ret = s3c_rp_pp_start(ctrl, ctrl->pp.buf_idx.run);
	if (ret < 0) {
		rp_err(ctrl->log_level, "Failed : s3c_rp_pp_start()\n");
		return -1;
	}

	ctrl->status = FIMC_STREAMON;

	return 0;
}
#endif

#if 0
int fimc_wakeup(void)
{
	struct fimc_control	*ctrl;
	int			ret = -1;

	ctrl = &s3c_rp;

	if (ctrl->status == FIMC_READY_RESUME) {
		ret = fimc_wakeup_fifo(ctrl);
		if (ret < 0) {
			dev_err(ctrl->dev,
				"s3c_rp_wakeup_fifo failed in %s\n", __func__);
			return -EINVAL;
		}
	}

	return 0;
}
#endif

#if 0
static void fimc_sleep_fifo(struct fimc_control *ctrl)
{
	if (ctrl->rot.status != ROT_IDLE)
		rp_err(ctrl->log_level,
			"[%s : %d] ROT status isn't idle\n",
			__func__, __LINE__);

	if ((ctrl->incoming_queue[0] != -1) || (ctrl->inside_queue[0] != -1))
		rp_err(ctrl->log_level,
			"[%s : %d] queue status isn't stable\n",
			__func__, __LINE__);

	if ((ctrl->pp.buf_idx.next != -1) || (ctrl->pp.buf_idx.prev != -1))
		rp_err(ctrl->log_level,
			"[%s : %d] PP status isn't stable\n",
			__func__, __LINE__);

	s3c_rp_pp_fifo_stop(ctrl, FIFO_SLEEP);
}
#endif

#if 0
int fimc_sleep(void)
{
	struct fimc_control *ctrl;

	ctrl = &s3c_rp;

	if (ctrl->status == FIMC_STREAMON)
		fimc_sleep_fifo(ctrl);

	ctrl->status		= FIMC_ON_SLEEP;

	return 0;
}
#endif
#ifdef S5P6442_POWER_GATING_CAM
int  gFIMC_CNT[3] = {0, 0, 0};


static int s5p_fimc_domain_timer(unsigned long arg)
{
	unsigned long flags;
/* Giridhar (100413): For avoiding lockup, try acquiring lock. If not available
 * schedule this operation later. If lock is available, do the required operation
 * and exit
 */
//	spin_lock_irqsave(&fimc_domain_lock, flags);
        if(spin_trylock_irqsave(&fimc_domain_lock, flags)){
	    if((gFIMC_CNT[0] <= 0) && (gFIMC_CNT[1] <= 0) && (gFIMC_CNT[2] <= 0)){
        	s5p6442_pwrgate_config(S5P6442_CAM_ID, S5P6442_LP_MODE);
           }
	    else if((gFIMC_CNT[1] <= 0) && (gFIMC_CNT[2] <= 0)){
		mod_timer(&g_fimc_domain_timer, jiffies + HZ);
	    }
	    spin_unlock_irqrestore(&fimc_domain_lock, flags);
        }
        else {
          mod_timer(&g_fimc_domain_timer, jiffies + HZ);
        }
//	spin_unlock_irqrestore(&fimc_domain_lock, flags);
}

void s5p_fimc_out_domain_set(int dev_id, int flag)
{

	unsigned long flags;
	if(flag){
		del_timer(&g_fimc_domain_timer);
		spin_lock_irqsave(&fimc_domain_lock, flags);
		s5p6442_pwrgate_config(S5P6442_CAM_ID, S5P6442_ACTIVE_MODE);
		
		gFIMC_CNT[dev_id]++;
		spin_unlock_irqrestore(&fimc_domain_lock, flags);
	}
	else {
		spin_lock_irqsave(&fimc_domain_lock, flags);
		gFIMC_CNT[dev_id]--;
		mod_timer(&g_fimc_domain_timer, jiffies + HZ);
		spin_unlock_irqrestore(&fimc_domain_lock, flags);
	}	
}


#endif

static int make_i2c_pin_low = 1; /* Make camera i2c line low only while booting */

static int fimc_open(struct file *filp)
{
	struct fimc_control *ctrl;
	struct s3c_platform_fimc *pdata;
	unsigned long flags;
	int ret;
	u32 cfg;
	#ifdef VIEW_FUNCTION_CALL
	printk("[FIMC_DEV] %s(%d)\n", __func__, __LINE__);
#endif

	/* An ugly hack to make the i2c pins output low */
	if (unlikely(make_i2c_pin_low == 1)) {
	    cfg = readl(S5P64XX_GPD1DAT);
	    cfg &= ~(0x1 << 0);
	    writel(cfg, S5P64XX_GPD1DAT);
	    cfg = readl(S5P64XX_GPD1DAT);
	    cfg &= ~(0x1 << 1);
	    writel(cfg, S5P64XX_GPD1DAT);
	    make_i2c_pin_low = 0;
	}

	ctrl = video_get_drvdata(video_devdata(filp));

#ifdef S5P6442_POWER_GATING_CAM
	del_timer(&g_fimc_domain_timer);
//	fimc0 controller for Camera
	if(ctrl->id == CAM_ID){
	   spin_lock_irqsave(&fimc_domain_lock, flags);
	   gFIMC_CNT[CAM_ID]++;
	   s5p6442_idle_pm_gpiocfg(S5P6442_CAM_ID, S5P6442_ACTIVE_MODE); 
           s5p6442_pwrgate_config(S5P6442_CAM_ID, S5P6442_ACTIVE_MODE);
	   spin_unlock_irqrestore(&fimc_domain_lock, flags);
	}
//           gFIMC_CNT++;
//        }
#endif

#ifdef CONFIG_CPU_FREQ
	if(ctrl->id == CAM_ID){
	        set_dvfs_level(0);
	}
#endif /* CONFIG_CPU_FREQ */

	pdata = to_fimc_plat(ctrl->dev);

	mutex_lock(&ctrl->lock);

	if (atomic_read(&ctrl->in_use)) {
		ret = -EBUSY;
		goto resource_busy;
	} else {
		atomic_inc(&ctrl->in_use);
	}

	if (pdata->clk_on)
		pdata->clk_on(to_platform_device(ctrl->dev), ctrl->clk);

	/* Apply things to interface register */
	fimc_hwset_reset(ctrl);
	filp->private_data = ctrl;

	ctrl->fb.open_fifo = s3cfb_open_fifo;
	ctrl->fb.close_fifo = s3cfb_close_fifo;

	ret = s3cfb_direct_ioctl(ctrl->id, S3CFB_GET_LCD_WIDTH,
					(unsigned long)&ctrl->fb.lcd_hres);
	if (ret < 0)
		dev_err(ctrl->dev, "Fail: S3CFB_GET_LCD_WIDTH\n");

	ret = s3cfb_direct_ioctl(ctrl->id, S3CFB_GET_LCD_HEIGHT,
					(unsigned long)&ctrl->fb.lcd_vres);
	if (ret < 0)
		dev_err(ctrl->dev, "Fail: S3CFB_GET_LCD_HEIGHT\n");

	ctrl->status = FIMC_STREAMOFF;

#if 0
	/* To do : have to send ctrl to the fimd driver. */
	ret = s3cfb_direct_ioctl(ctrl->id, S3CFB_SET_SUSPEND_FIFO,
			(unsigned long)fimc_sleep);
	if (ret < 0)
		dev_err(ctrl->dev,
			"s3cfb_direct_ioctl(S3CFB_SET_SUSPEND_FIFO) fail\n");

	ret = s3cfb_direct_ioctl(ctrl->id, S3CFB_SET_RESUME_FIFO,
			(unsigned long)fimc_wakeup);
	if (ret < 0)
		dev_err(ctrl->dev,
			"s3cfb_direct_ioctl(S3CFB_SET_SUSPEND_FIFO) fail\n");
#endif
	mutex_unlock(&ctrl->lock);

	return 0;

resource_busy:
	mutex_unlock(&ctrl->lock);
	return ret;
}

static int fimc_release(struct file *filp)
{
	struct fimc_control *ctrl = filp->private_data;
	struct s3c_platform_fimc *pdata;
	unsigned long flags;
	int ret = 0, i;
#ifdef VIEW_FUNCTION_CALL
	printk("[FIMC_DEV] %s(%d)\n", __func__, __LINE__);
#endif

	pdata = to_fimc_plat(ctrl->dev);

	atomic_dec(&ctrl->in_use);
	filp->private_data = NULL;

	/* FIXME: turning off actual working camera */
	if (ctrl->cam) {
		/* shutdown the MCLK */
		clk_disable(ctrl->cam->clk);

		/* shutdown */
		if (ctrl->cam->cam_power)
			ctrl->cam->cam_power(0);

		/* should be initialized at the next open */
		ctrl->cam->initialized = 0;
	}

	if (ctrl->cap) {
		for (i = 0; i < FIMC_CAPBUFS; i++) {
			fimc_dma_free(ctrl, &ctrl->cap->bufs[i], 0);
			fimc_dma_free(ctrl, &ctrl->cap->bufs[i], 1);
		}

		kfree(ctrl->cap);
		ctrl->cap = NULL;
	}

	if (ctrl->out) {
		if (ctrl->status != FIMC_STREAMOFF) {
			ret = fimc_outdev_stop_streaming(ctrl);
			if (ret < 0)
				dev_err(ctrl->dev,
					"Fail: fimc_stop_streaming\n");
			ctrl->status = FIMC_STREAMOFF;
		}

		kfree(ctrl->out);
		ctrl->out = NULL;
	}

	if (pdata->clk_off)
		pdata->clk_off(to_platform_device(ctrl->dev), ctrl->clk);
#ifdef S5P6442_POWER_GATING_CAM
//        gFIMC_CNT--;
	if(ctrl->id == CAM_ID){
	   spin_lock_irqsave(&fimc_domain_lock, flags);
	    s5p6442_idle_pm_gpiocfg(S5P6442_CAM_ID, S5P6442_LP_MODE);
		gFIMC_CNT[ctrl->id]--;
		if((gFIMC_CNT[0] <= 0) && (gFIMC_CNT[1] <= 0) && (gFIMC_CNT[2] <= 0)){
            s5p6442_pwrgate_config(S5P6442_CAM_ID, S5P6442_LP_MODE);
	}
	   spin_unlock_irqrestore(&fimc_domain_lock, flags);
	}
#endif
#ifdef CONFIG_CPU_FREQ
	if(ctrl->id == CAM_ID){
	        set_dvfs_level(1);
	}
#endif /* CONFIG_CPU_FREQ */

	dev_info(ctrl->dev, "%s: successfully released\n", __func__);

	return 0;
}

static const struct v4l2_file_operations fimc_fops = {
	.owner = THIS_MODULE,
	.open = fimc_open,
	.release = fimc_release,
	.ioctl = video_ioctl2,
	.read = fimc_read,
	.write = fimc_write,
	.mmap = fimc_mmap,
	.poll = fimc_poll,
};

static void fimc_vdev_release(struct video_device *vdev)
{
	#ifdef VIEW_FUNCTION_CALL
	printk("[FIMC_DEV] %s(%d)\n", __func__, __LINE__);
#endif

	kfree(vdev);
}

struct video_device fimc_video_device[FIMC_DEVICES] = {
	[0] = {
		.fops = &fimc_fops,
		.ioctl_ops = &fimc_v4l2_ops,
		.release  = fimc_vdev_release,
	},
	[1] = {
		.fops = &fimc_fops,
		.ioctl_ops = &fimc_v4l2_ops,
		.release  = fimc_vdev_release,
	},
	[2] = {
		.fops = &fimc_fops,
		.ioctl_ops = &fimc_v4l2_ops,
		.release  = fimc_vdev_release,
	},
};

static int fimc_init_global(struct platform_device *pdev)
{
	struct s3c_platform_fimc *pdata;
	struct s3c_platform_camera *cam;
	struct clk *srclk;
	int i;
#ifdef VIEW_FUNCTION_CALL
	printk("[FIMC_DEV] %s(%d)\n", __func__, __LINE__);
#endif

	pdata = to_fimc_plat(&pdev->dev);

	/* Registering external camera modules. re-arrange order to be sure */
	for (i = 0; i < FIMC_MAXCAMS; i++) {
		cam = pdata->camera[i];
		if (!cam)
			break;

		srclk = clk_get(&pdev->dev, cam->srclk_name);
		if (IS_ERR(srclk)) {
			dev_err(&pdev->dev, "%s: failed to get mclk source\n",
				__func__);
			return -EINVAL;
		}

		/* mclk */
		cam->clk = clk_get(&pdev->dev, cam->clk_name);
		if (IS_ERR(cam->clk)) {
			dev_err(&pdev->dev, "%s: failed to get mclk source\n",
				__func__);
			return -EINVAL;
		}

		if (cam->clk->set_parent) {
			cam->clk->parent = srclk;
			cam->clk->set_parent(cam->clk, srclk);
		}

		/* Assign camera device to fimc */
		fimc_dev->camera[cam->id] = cam;
	}

	fimc_dev->initialized = 1;

	return 0;
}

/*
 * Assign v4l2 device and subdev to fimc
 * it is called per every fimc ctrl registering
 */
static int fimc_configure_subdev(struct platform_device *pdev, int id)
{
	struct s3c_platform_fimc *pdata;
	struct s3c_platform_camera *cam;
	struct i2c_adapter *i2c_adap;
	struct i2c_board_info *i2c_info;
	struct v4l2_subdev *sd;
	struct fimc_control *ctrl;
	unsigned short addr;
	char *name;
#ifdef VIEW_FUNCTION_CALL
	printk("[FIMC_DEV] %s(%d)\n", __func__, __LINE__);
#endif

	ctrl = get_fimc_ctrl(id);
	pdata = to_fimc_plat(&pdev->dev);
	cam = pdata->camera[id];

	/* Subdev registration */
	if (cam) {
		memcpy(&cam_struct_g,cam,sizeof(cam_struct_g));
		i2c_adap = i2c_get_adapter(cam->i2c_busnum);
		if (!i2c_adap) {
			dev_info(&pdev->dev, "subdev i2c_adapter "
					"missing-skip registration\n");
		}

		i2c_info = cam->info;
		if (!i2c_info) {
			dev_err(&pdev->dev,
					"%s: subdev i2c board info missing\n",
					__func__);
			return -ENODEV;
		}

		name = i2c_info->type;
		if (!name) {
			dev_info(&pdev->dev,
				"subdev i2c dirver name missing-skip "
				"registration\n");
			return -ENODEV;
		}

		addr = i2c_info->addr;
		if (!addr) {
			dev_info(&pdev->dev, "subdev i2c address "
					"missing-skip registration\n");
			return -ENODEV;
		}

		/*
		 * NOTE: first time subdev being registered,
		 * s_config is called and try to initialize subdev device
		 * but in this point, we are not giving MCLK and power to subdev
		 * so nothing happens but pass platform data through
		 */
		sd = v4l2_i2c_new_subdev_board(&ctrl->v4l2_dev, i2c_adap,
				name, i2c_info, &addr);
		if (!sd) {
			dev_err(&pdev->dev,
				"%s: v4l2 subdev board registering failed\n",
				__func__);
		}

		/* Assign camera device to fimc */
//		fimc_dev->camera[cam->id] = cam;
		fimc_dev->camera[cam->id] = &cam_struct_g;

		/* Assign subdev to proper camera device pointer */
		fimc_dev->camera[cam->id]->sd = sd;
	}

	return 0;
}

static int __devinit fimc_probe(struct platform_device *pdev)
{
	struct s3c_platform_fimc *pdata;
	struct fimc_control *ctrl;
	int ret;
#ifdef VIEW_FUNCTION_CALL
	printk("[FIMC_DEV] %s(%d)\n", __func__, __LINE__);
#endif

	if (!fimc_dev) {
		fimc_dev = kzalloc(sizeof(*fimc_dev), GFP_KERNEL);
		if (!fimc_dev) {
			dev_err(&pdev->dev, "%s: not enough memory\n",
				__func__);
			goto err_fimc;
		}
	}

	ctrl = fimc_register_controller(pdev);
	if (!ctrl) {
		dev_err(&pdev->dev, "%s: cannot register fimc controller\n",
			__func__);
		goto err_fimc;
	}

	pdata = to_fimc_plat(&pdev->dev);
	if (pdata->cfg_gpio)
		pdata->cfg_gpio(pdev);

	/* V4L2 device-subdev registration */
	ret = v4l2_device_register(&pdev->dev, &ctrl->v4l2_dev);
	if (ret) {
		dev_err(&pdev->dev, "%s: v4l2 device register failed\n",
			__func__);
		goto err_v4l2;
	}

#ifdef S5P6442_POWER_GATING_CAM
//      fimc0 controller for Camera
	if(ctrl->id == CAM_ID){
          s5p6442_idle_pm_gpiocfg(S5P6442_CAM_ID, S5P6442_ACTIVE_MODE);
        gFIMC_CNT[ctrl->id]++;
	  
        }
        s5p6442_pwrgate_config(S5P6442_CAM_ID, S5P6442_ACTIVE_MODE);
        //gFIMC_CNT[ctrl->id]++;
#endif

	/* things to initialize once */
	if (!fimc_dev->initialized) {
		ret = fimc_init_global(pdev);
		if (ret)
			goto err_global;
	}

	/* v4l2 subdev configuration */
	ret = fimc_configure_subdev(pdev, ctrl->id);
	if (ret) {
		dev_err(&pdev->dev, "%s: subdev[%d] registering failed\n",
			__func__, ctrl->id);
	}

	/* video device register */
	ret = video_register_device(ctrl->vd, VFL_TYPE_GRABBER, ctrl->id);
	if (ret) {
		dev_err(&pdev->dev, "%s: cannot register video driver\n",
			__func__);
		goto err_global;
	}

	video_set_drvdata(ctrl->vd, ctrl);
    
    if (ctrl->id == 1)
			pdata->hw_ver = 0x50;
		else
			pdata->hw_ver = 0x45;
 
	dev_info(&pdev->dev, "controller %d registered successfully\n",
		ctrl->id);



#ifdef S5P6442_POWER_GATING_CAM
	if(!fimc_timer_init_flag){
                fimc_timer_init_flag = 1;
        	init_timer(&g_fimc_domain_timer);
        	g_fimc_domain_timer.function = s5p_fimc_domain_timer;
	}
        if(ctrl->id == CAM_ID){
                gFIMC_CNT[ctrl->id]--;
                s5p6442_idle_pm_gpiocfg(S5P6442_CAM_ID, S5P6442_LP_MODE);
        }
        if((gFIMC_CNT[0] <= 0) && (gFIMC_CNT[1] <= 0) && (gFIMC_CNT[2] <= 0)){
            s5p6442_pwrgate_config(S5P6442_CAM_ID, S5P6442_LP_MODE);
        }
#endif


	return 0;

err_global:
	clk_disable(ctrl->clk);
	clk_put(ctrl->clk);

err_v4l2:
	fimc_unregister_controller(pdev);


#ifdef S5P6442_POWER_GATING_CAM
	if(!fimc_timer_init_flag){	
		fimc_timer_init_flag = 1;
		init_timer(&g_fimc_domain_timer);
       		g_fimc_domain_timer.function = s5p_fimc_domain_timer;		
	}
        if(ctrl->id == CAM_ID){
		gFIMC_CNT[ctrl->id]--;
                s5p6442_idle_pm_gpiocfg(S5P6442_CAM_ID, S5P6442_LP_MODE);
        }
        if((gFIMC_CNT[0] <= 0) && (gFIMC_CNT[1] <= 0) && (gFIMC_CNT[2] <= 0)){
            s5p6442_pwrgate_config(S5P6442_CAM_ID, S5P6442_LP_MODE);
        }
#endif

err_fimc:
	return -EINVAL;

}

static int fimc_remove(struct platform_device *pdev)
{
	fimc_unregister_controller(pdev);
#ifdef VIEW_FUNCTION_CALL
	printk("[FIMC_DEV] %s(%d)\n", __func__, __LINE__);
#endif

	if (fimc_dev) {
		kfree(fimc_dev);
		fimc_dev = NULL;
	}

	return 0;
}

#ifdef CONFIG_PM
int fimc_suspend(struct platform_device *dev, pm_message_t state)
{
#if 0
        struct fimc_control *ctrl;
	/* TODO: Check if something needs to be done for GPIO settings */
	fimc_save_regs(ctrl);
    clk_disable(ctrl->clk);
#endif
	return 0;
}

int fimc_resume(struct platform_device *dev)
{
#if 0
        struct fimc_control *ctrl;
	int id = dev->id;

        ctrl = &fimc_dev->ctrl[id];

        clk_enable(ctrl->clk);

	/* TODO: Check for GPIO settings */
	fimc_load_regs(ctrl);
#endif
	return 0;
}
#else
#define fimc_suspend	NULL
#define fimc_resume	NULL
#endif

static struct platform_driver fimc_driver = {
	.probe		= fimc_probe,
	.remove		= fimc_remove,
	.suspend	= fimc_suspend,
	.resume		= fimc_resume,
	.driver		= {
		.name	= FIMC_NAME,
		.owner	= THIS_MODULE,
	},
};

static int fimc_register(void)
{
	#ifdef VIEW_FUNCTION_CALL
	printk("[FIMC_DEV] %s(%d)\n", __func__, __LINE__);
#endif

	platform_driver_register(&fimc_driver);

	return 0;
}

static void fimc_unregister(void)
{
	#ifdef VIEW_FUNCTION_CALL
	printk("[FIMC_DEV] %s(%d)\n", __func__, __LINE__);
#endif

	platform_driver_unregister(&fimc_driver);
}

late_initcall(fimc_register);
module_exit(fimc_unregister);

MODULE_AUTHOR("Dongsoo, Kim <dongsoo45.kim@samsung.com>");
MODULE_AUTHOR("Jinsung, Yang <jsgood.yang@samsung.com>");
MODULE_AUTHOR("Jonghun, Han <jonghun.han@samsung.com>");
MODULE_DESCRIPTION("Samsung Camera Interface (FIMC) driver");
MODULE_LICENSE("GPL");


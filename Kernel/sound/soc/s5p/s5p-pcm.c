/*
 * s5p-pcm.c  --  ALSA Soc Audio Layer
 *
 * (c) 2006 Wolfson Microelectronics PLC.
 * Graeme Gregory graeme.gregory@wolfsonmicro.com or linux@wolfsonmicro.com
 *
 * (c) 2004-2005 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include <asm/dma.h>
#include <asm/io.h>
#include <mach/hardware.h>
#include <mach/s3c-dma.h>
#include <mach/audio.h>

#include "s5p-pcm.h"
#include "s5p-i2s.h"

struct s5p_pcm_pdata s3c_pcm_pdat;

struct s5p_runtime_data {
	spinlock_t lock;
	int state;
	unsigned int dma_limit;
	unsigned int dma_period;
	unsigned int dma_size;
	dma_addr_t dma_start;
	dma_addr_t dma_pos;
	dma_addr_t dma_end;
	struct s5p_pcm_dma_params *params;
};

static struct s5p_i2s_pdata *s3ci2s_func = NULL;

extern unsigned int ring_buf_index;
extern unsigned int period_index;

/* s5p_pcm_enqueue
 *
 * place a dma buffer onto the queue for the dma system
 * to handle.
 */
static void s5p_pcm_enqueue(struct snd_pcm_substream *substream)
{
	struct s5p_runtime_data *prtd = substream->runtime->private_data;
	dma_addr_t pos = prtd->dma_pos;
	unsigned long len = prtd->dma_period;
	int ret;

	s5pdbg("Entered %s\n", __FUNCTION__);

	/* By Jung */
	if ((pos + len) > prtd->dma_end) {
		len  = prtd->dma_end - pos;
		s5pdbg(KERN_DEBUG "%s: corrected dma len %ld\n", __FUNCTION__, len);
	}

	s5pdbg("enqing at %x, %d bytes\n", pos, len);
	ret = s3c2410_dma_enqueue(prtd->params->channel, substream, pos, len);

	prtd->dma_pos = pos;
}

static void s5p_audio_buffdone(struct s3c2410_dma_chan *channel,
				void *dev_id, int size,
				enum s3c2410_dma_buffresult result)
{
	struct snd_pcm_substream *substream = dev_id;
	struct s5p_runtime_data *prtd;

	s5pdbg("Entered %s\n", __FUNCTION__);

	if (result == S3C2410_RES_ABORT || result == S3C2410_RES_ERR)
		return;
		
	if (!substream)
		return;

	prtd = substream->runtime->private_data;
	
	/* By Jung */
	prtd->dma_pos += prtd->dma_period;
	if (prtd->dma_pos >= prtd->dma_end)
		prtd->dma_pos = prtd->dma_start;

	snd_pcm_period_elapsed(substream);

	spin_lock(&prtd->lock);
	if (prtd->state & ST_RUNNING) 
		s5p_pcm_enqueue(substream);

	spin_unlock(&prtd->lock);
}

static void pcm_dmaupdate(void *id, int bytes_xfer)
{
	struct snd_pcm_substream *substream = id;
	struct s5p_runtime_data *prtd = substream->runtime->private_data;

	s5pdbg("%s:%d\n", __func__, __LINE__);

	/* For size of transferred data  */
	prtd->dma_pos += bytes_xfer;
	if (prtd->dma_pos >= prtd->dma_end)
		prtd->dma_pos = prtd->dma_start;

	if(prtd && (prtd->state & ST_RUNNING))
		snd_pcm_period_elapsed(substream); /* Only once for any num of periods while RUNNING */
}

static int s5p_pcm_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct s5p_runtime_data *prtd = runtime->private_data;
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct s5p_pcm_dma_params *dma = rtd->dai->cpu_dai->dma_data;
	unsigned long totbytes = params_buffer_bytes(params);
	unsigned int  periods  = params_periods(params);
	int ret=0;

	s5pdbg("Entered %s, params = %p \n", __FUNCTION__, prtd->params);

	/* By Jung */
	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
		totbytes *= CONFIG_ANDROID_BUF_NUM;

	/* return if this is a bufferless transfer e.g.
	 * codec <--> BT codec or GSM modem -- lg FIXME */
	if (!dma)
		return 0;

	/* this may get called several times by oss emulation
	 * with different params */

#ifdef CONFIG_S5P64XX_LPAUDIO
	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
	
		/* We configure callback at partial playback complete acc to dutycyle selected */
		s3ci2s_func->dma_setcallbk(pcm_dmaupdate, params_period_bytes(params));
	}
	else {
#endif /* CONFIG_S5P64XX_LPAUDIO */
		if (prtd->params == NULL) {
			prtd->params = dma;
			s5pdbg("params %p, client %p, channel %d\n", prtd->params,
				prtd->params->client, prtd->params->channel);

			/* prepare DMA */
			ret = s3c2410_dma_request(prtd->params->channel,
						  prtd->params->client, NULL);

			if (ret) {
				printk(KERN_ERR "failed to get dma channel\n");
				return ret;
			}
		} else if (prtd->params != dma) {

			s3c2410_dma_free(prtd->params->channel, prtd->params->client);

			prtd->params = dma;
			s5pdbg("params %p, client %p, channel %d\n", prtd->params,
				prtd->params->client, prtd->params->channel);

			/* prepare DMA */
			ret = s3c2410_dma_request(prtd->params->channel,
						  prtd->params->client, NULL);

			if (ret) {
				printk(KERN_ERR "failed to get dma channel\n");
				return ret;
			}
		}

		/* channel needs configuring for mem=>device, increment memory addr,
		 * sync to pclk, half-word transfers to the IIS-FIFO. */
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			s3c2410_dma_devconfig(prtd->params->channel,
					S3C2410_DMASRC_MEM, 0,
					prtd->params->dma_addr);
	
			s3c2410_dma_config(prtd->params->channel,
					prtd->params->dma_size, 0);
	
		} else {
			s3c2410_dma_devconfig(prtd->params->channel,
					S3C2410_DMASRC_HW, 0,
					prtd->params->dma_addr);		
	
			s3c2410_dma_config(prtd->params->channel,
					prtd->params->dma_size, 0);
		}
	
		s3c2410_dma_set_buffdone_fn(prtd->params->channel,
					    s5p_audio_buffdone);

		snd_pcm_set_runtime_buffer(substream, &substream->dma_buffer);
#ifdef CONFIG_S5P64XX_LPAUDIO
	}
#endif /* CONFIG_S5P64XX_LPAUDIO */
		runtime->dma_bytes = totbytes;

		spin_lock_irq(&prtd->lock);
		prtd->dma_limit = runtime->hw.periods_min;
		prtd->dma_period = params_period_bytes(params);
		prtd->dma_start = runtime->dma_addr;
		prtd->dma_pos = prtd->dma_start;
		prtd->dma_end = prtd->dma_start + totbytes;
		spin_unlock_irq(&prtd->lock);
	printk("DmaAddr=@%x Total=%lubytes PrdSz=%u #Prds=%u, dmaEnd 0x%x\n",
				runtime->dma_addr, totbytes, params_period_bytes(params), periods, prtd->dma_end);
	
	totbytes = 0;
	return 0;
}

static int s5p_pcm_hw_free(struct snd_pcm_substream *substream)
{
	struct s5p_runtime_data *prtd = substream->runtime->private_data;

	s5pdbg("Entered %s\n", __FUNCTION__);

	/* TODO - do we need to ensure DMA flushed */
	snd_pcm_set_runtime_buffer(substream, NULL);

	if (prtd->params) {
		s3c2410_dma_free(prtd->params->channel, prtd->params->client);
		prtd->params = NULL;
	}

	return 0;
}

static int s5p_pcm_prepare(struct snd_pcm_substream *substream)
{
	struct s5p_runtime_data *prtd = substream->runtime->private_data;

	s5pdbg("Entered %s\n", __FUNCTION__);
	
#if !defined (CONFIG_CPU_S3C6400) && !defined (CONFIG_CPU_S3C6410) 
	/* return if this is a bufferless transfer e.g.
	 * codec <--> BT codec or GSM modem -- lg FIXME */
	if(substream->stream == SNDRV_PCM_STREAM_CAPTURE) {
		if (!prtd->params)
		 	return 0;
	}
#endif

	/* By Jung */
	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		ring_buf_index   = 0;
		period_index     = 0;
	}
	
	prtd->dma_pos = prtd->dma_start;

#ifdef CONFIG_S5P64XX_LPAUDIO
	/* By Jung */
	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		s3ci2s_func->dma_ctrl(S3C_I2SDMA_FLUSH);
		s3ci2s_func->dma_enqueue((void *)substream); /* Configure to loop the whole buffer */
	}
	else {
		s3c2410_dma_ctrl(prtd->params->channel, S3C2410_DMAOP_FLUSH);
		s5p_pcm_enqueue(substream);
	}
#else
	s3c2410_dma_ctrl(prtd->params->channel, S3C2410_DMAOP_FLUSH);
		s5p_pcm_enqueue(substream);
#endif

	return 0;
}

static int s5p_pcm_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct s5p_runtime_data *prtd = substream->runtime->private_data;
	int ret = 0;

	s5pdbg("Entered %s\n", __FUNCTION__);

	spin_lock(&prtd->lock);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_RESUME:
#ifdef CONFIG_S5P64XX_LPAUDIO
			if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
				s3ci2s_func->dma_ctrl(S3C_I2SDMA_RESUME);
#endif /* CONFIG_S5P64XX_LPAUDIO */
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		prtd->state |= ST_RUNNING;
#ifdef CONFIG_S5P64XX_LPAUDIO
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			s3ci2s_func->dma_ctrl(S3C_I2SDMA_START);
		else
			s3c2410_dma_ctrl(prtd->params->channel, S3C2410_DMAOP_START);		
#else
			s3c2410_dma_ctrl(prtd->params->channel, S3C2410_DMAOP_START);		
#endif /* CONFIG_S5P64XX_LPAUDIO */
		break;
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		prtd->state &= ~ST_RUNNING;
#ifdef CONFIG_S5P64XX_LPAUDIO
		if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
			s3ci2s_func->dma_ctrl(S3C_I2SDMA_STOP);
		else
			s3c2410_dma_ctrl(prtd->params->channel, S3C2410_DMAOP_STOP);
#else
			s3c2410_dma_ctrl(prtd->params->channel, S3C2410_DMAOP_STOP);
#endif /* CONFIG_S5P64XX_LPAUDIO */
		break;

	default:
		ret = -EINVAL;
		break;
	}

	spin_unlock(&prtd->lock);

	return ret;
}

static snd_pcm_uframes_t 
	s5p_pcm_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct s5p_runtime_data *prtd = runtime->private_data;
	unsigned long res;

	spin_lock(&prtd->lock);

	/* By Jung */
	res = prtd->dma_pos - prtd->dma_start;

	spin_unlock(&prtd->lock);
	
	/* we seem to be getting the odd error from the pcm library due
	 * to out-of-bounds pointers. this is maybe due to the dma engine
	 * not having loaded the new values for the channel before being
	 * callled... (todo - fix )
	 */
	
	/* By Jung */
	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		if (res >= snd_pcm_lib_buffer_bytes(substream) * CONFIG_ANDROID_BUF_NUM) {
			if (res == snd_pcm_lib_buffer_bytes(substream) * CONFIG_ANDROID_BUF_NUM)
				res = 0;
		}
	} else {
		if (res >= snd_pcm_lib_buffer_bytes(substream)) {
			if (res == snd_pcm_lib_buffer_bytes(substream))
				res = 0;
		}
	}

	return bytes_to_frames(substream->runtime, res);
}

static int s5p_pcm_mmap(struct snd_pcm_substream *substream,
	struct vm_area_struct *vma)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int ret;

	s5pdbg("Entered %s\n", __FUNCTION__);

	ret = dma_mmap_writecombine(substream->pcm->card->dev, vma,
								runtime->dma_area,
	                     		runtime->dma_addr,
								runtime->dma_bytes);
	return ret;
}

static int s5p_pcm_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct s5p_runtime_data *prtd;

	s5pdbg("Entered %s\n", __FUNCTION__);

	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK)
	   snd_soc_set_runtime_hwparams(substream, &s3c_pcm_pdat.pcm_hw_tx);
	else
	   snd_soc_set_runtime_hwparams(substream, &s3c_pcm_pdat.pcm_hw_rx);

	prtd = kzalloc(sizeof(struct s5p_runtime_data), GFP_KERNEL);
	if (prtd == NULL)
		return -ENOMEM;

	spin_lock_init(&prtd->lock);

	runtime->private_data = prtd;

	return 0;
}

static int s5p_pcm_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct s5p_runtime_data *prtd = runtime->private_data;

	s5pdbg("Entered %s, prtd = %p\n", __FUNCTION__, prtd);

	if (prtd)
		kfree(prtd);
	else
		printk("s5p_pcm_close called with prtd == NULL\n");

	return 0;
}

static struct snd_pcm_ops s5p_pcm_ops = {
	.open		= s5p_pcm_open,
	.close		= s5p_pcm_close,
	.ioctl		= snd_pcm_lib_ioctl,
	.hw_params	= s5p_pcm_hw_params,
	.hw_free	= s5p_pcm_hw_free,
	.prepare	= s5p_pcm_prepare,
	.trigger	= s5p_pcm_trigger,
	.pointer	= s5p_pcm_pointer,
	.mmap		= s5p_pcm_mmap,
};

static int s5p_pcm_preallocate_dma_buffer(struct snd_pcm *pcm, int stream)
{
	struct snd_pcm_substream *substream = pcm->streams[stream].substream;
	struct snd_dma_buffer *buf = &substream->dma_buffer;
	size_t size;
	unsigned char *vaddr;
	dma_addr_t paddr;

	s5pdbg("Entered %s\n", __FUNCTION__);

	buf->dev.dev = pcm->card->dev;
	buf->private_data = NULL;

	if(stream == SNDRV_PCM_STREAM_PLAYBACK)
	   size = s3c_pcm_pdat.pcm_hw_tx.buffer_bytes_max;
	else
	   size = s3c_pcm_pdat.pcm_hw_rx.buffer_bytes_max;

#ifdef CONFIG_S5P64XX_LPAUDIO
	if(stream == SNDRV_PCM_STREAM_PLAYBACK) {
		/* Remapping SRAM in I2S(Internal DMA buffer) */
		paddr = s3c_pcm_pdat.lp_buffs.dma_addr[stream];
		vaddr = (unsigned char *)ioremap(paddr, size);
	}
	else  
#endif /* CONFIG_S5P64XX_LPAUDIO */
		vaddr = dma_alloc_writecombine(pcm->card->dev, size, &paddr, GFP_KERNEL);
		
	if (!vaddr)
		return -ENOMEM;

	s3c_pcm_pdat.nm_buffs.cpu_addr[stream] = vaddr;
	s3c_pcm_pdat.nm_buffs.dma_addr[stream] = paddr;

	/* Assign PCM buffer pointers */
#ifdef CONFIG_S5P64XX_LPAUDIO
	if(stream == SNDRV_PCM_STREAM_PLAYBACK)
		buf->dev.type = SNDRV_DMA_TYPE_CONTINUOUS;
	else
		buf->dev.type = SNDRV_DMA_TYPE_DEV;		
#else
	buf->dev.type = SNDRV_DMA_TYPE_DEV;
#endif /* CONFIG_S5P64XX_LPAUDIO */
	buf->area = s3c_pcm_pdat.nm_buffs.cpu_addr[stream];
	buf->addr = s3c_pcm_pdat.nm_buffs.dma_addr[stream];
		
	printk("preallocate buffer(%s):  VA-%p  PA-%X  %ubytes\n", 
			stream ? "Capture": "Playback", 
			vaddr, paddr, size);

	buf->bytes = size;
	return 0;
}

static void s5p_pcm_free_dma_buffers(struct snd_pcm *pcm)
{
	struct snd_pcm_substream *substream;
	struct snd_dma_buffer *buf;
	int stream;

	s5pdbg("Entered %s\n", __FUNCTION__);

	for (stream = SNDRV_PCM_STREAM_PLAYBACK; stream <= SNDRV_PCM_STREAM_CAPTURE; stream++) {

		substream = pcm->streams[stream].substream;
		if (!substream)
			continue;

		buf = &substream->dma_buffer;
		if (!buf->area)
			continue;

#ifdef CONFIG_S5P64XX_LPAUDIO
		if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
			iounmap(s3c_pcm_pdat.lp_buffs.cpu_addr[stream]);
			s3c_pcm_pdat.lp_buffs.cpu_addr[stream] = NULL;
		}	
		else {
			dma_free_writecombine(pcm->card->dev, buf->bytes,
			      s3c_pcm_pdat.nm_buffs.cpu_addr[stream], s3c_pcm_pdat.nm_buffs.dma_addr[stream]);
			s3c_pcm_pdat.nm_buffs.cpu_addr[stream] = NULL;
			s3c_pcm_pdat.nm_buffs.dma_addr[stream] = 0;
		}
#else
		dma_free_writecombine(pcm->card->dev, buf->bytes,
			      s3c_pcm_pdat.nm_buffs.cpu_addr[stream], s3c_pcm_pdat.nm_buffs.dma_addr[stream]);
		s3c_pcm_pdat.nm_buffs.cpu_addr[stream] = NULL;
		s3c_pcm_pdat.nm_buffs.dma_addr[stream] = 0;
#endif /* CONFIG_S5P64XX_LPAUDIO */
		buf->area = NULL;
		buf->addr = 0;
	}
}

static u64 s5p_pcm_dmamask = DMA_32BIT_MASK;

static int s5p_pcm_new(struct snd_card *card, 
	struct snd_soc_dai *dai, struct snd_pcm *pcm)
{
	int ret = 0;

	s5pdbg("Entered %s\n", __FUNCTION__);

	if (!card->dev->dma_mask)
		card->dev->dma_mask = &s5p_pcm_dmamask;
	if (!card->dev->coherent_dma_mask)
		card->dev->coherent_dma_mask = 0xffffffff;

	if (dai->playback.channels_min) {
		ret = s5p_pcm_preallocate_dma_buffer(pcm,
			SNDRV_PCM_STREAM_PLAYBACK);
		if (ret)
			goto out;
	}

	if (dai->capture.channels_min) {
		ret = s5p_pcm_preallocate_dma_buffer(pcm,
			SNDRV_PCM_STREAM_CAPTURE);
		if (ret)
			goto out;
	}
 out:
	return ret;
}

static void s5p_pcm_setmode(void *ptr)
{
	s3ci2s_func = (struct s5p_i2s_pdata *) ptr;

	/* Configure Playback Channel */
	s3c_pcm_pdat.pcm_hw_tx.buffer_bytes_max = 128 * 1024;
	s3c_pcm_pdat.pcm_hw_tx.period_bytes_min = 128;
	s3c_pcm_pdat.pcm_hw_tx.period_bytes_max = 16 * 1024;
	s3c_pcm_pdat.pcm_hw_tx.periods_min = 2;
	s3c_pcm_pdat.pcm_hw_tx.periods_max = 128;

	/* Configure Capture Channel */
	s3c_pcm_pdat.pcm_hw_rx.buffer_bytes_max = MAX_LP_BUFF/2;
	s3c_pcm_pdat.pcm_hw_rx.channels_min = 2;
	s3c_pcm_pdat.pcm_hw_rx.channels_max = 2;
	
	s5pdbg("PrdsMn=%d PrdsMx=%d PrdSzMn=%d PrdSzMx=%d\n", 
					s3c_pcm_pdat.pcm_hw_tx.periods_min, s3c_pcm_pdat.pcm_hw_tx.periods_max,
					s3c_pcm_pdat.pcm_hw_tx.period_bytes_min, s3c_pcm_pdat.pcm_hw_tx.period_bytes_max);
}

struct s5p_pcm_pdata s3c_pcm_pdat = {
	.lp_buffs = {
		.dma_addr[SNDRV_PCM_STREAM_PLAYBACK] = LP_TXBUFF_ADDR,
	},
	.set_mode = s5p_pcm_setmode,
	.pcm_pltfm = {
		.name		= "s5p-audio",
		.pcm_ops 	= &s5p_pcm_ops,
		.pcm_new	= s5p_pcm_new,
		.pcm_free	= s5p_pcm_free_dma_buffers,
	},
	.pcm_hw_tx = {
		.info			= SNDRV_PCM_INFO_INTERLEAVED |
					    SNDRV_PCM_INFO_BLOCK_TRANSFER |
					    SNDRV_PCM_INFO_PAUSE |
					    SNDRV_PCM_INFO_RESUME |
					    SNDRV_PCM_INFO_MMAP |
					    SNDRV_PCM_INFO_MMAP_VALID,
		.formats		= SNDRV_PCM_FMTBIT_S16_LE |
					    SNDRV_PCM_FMTBIT_U16_LE |
					    SNDRV_PCM_FMTBIT_U8 |
					    SNDRV_PCM_FMTBIT_S24_LE |
					    SNDRV_PCM_FMTBIT_S8,
		.channels_min		= 2,
		.channels_max		= 2,
		.period_bytes_min	= PAGE_SIZE,
		.fifo_size		= 64,
	},
	.pcm_hw_rx = {
		.info			= SNDRV_PCM_INFO_INTERLEAVED |
					    SNDRV_PCM_INFO_BLOCK_TRANSFER |
					    SNDRV_PCM_INFO_PAUSE |
					    SNDRV_PCM_INFO_RESUME |
					    SNDRV_PCM_INFO_MMAP |
					    SNDRV_PCM_INFO_MMAP_VALID,
		.formats		= SNDRV_PCM_FMTBIT_S16_LE |
					    SNDRV_PCM_FMTBIT_U16_LE |
					    SNDRV_PCM_FMTBIT_U8 |
					    SNDRV_PCM_FMTBIT_S24_LE |
					    SNDRV_PCM_FMTBIT_S8,
		.period_bytes_min	= 128,
		.period_bytes_max	= 16 *1024,
		.periods_min		= 2,
		.periods_max		= 128,
		.fifo_size		= 64,
	},
};
EXPORT_SYMBOL_GPL(s3c_pcm_pdat);

static int __init s5p_soc_platform_init(void)
{
	return snd_soc_register_platform(&s3c_pcm_pdat.pcm_pltfm);
}
module_init(s5p_soc_platform_init);

static void __exit s5p_soc_platform_exit(void)
{
	snd_soc_unregister_platform(&s3c_pcm_pdat.pcm_pltfm);
}
module_exit(s5p_soc_platform_exit);

MODULE_AUTHOR("S.LSI");
MODULE_DESCRIPTION("Samsung S5P PCM module");
MODULE_LICENSE("GPL");

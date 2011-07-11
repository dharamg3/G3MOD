/*
 * s5p-i2s.c  --  ALSA Soc Audio Layer
 *
 * (c) 2009 Samsung Electronics 
 *  Derived from Ben Dooks' driver for s3c24xx
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>

#include <asm/dma.h>
#include <mach/map.h>
#include <mach/s3c-dma.h>
#include <plat/regs-clock.h>

#include "s5p-pcm.h"
#include "s5p-i2s.h"

static struct s3c2410_dma_client s3c_dma_client_out = {
	.name = "I2S PCM Stereo out"
};

static struct s3c2410_dma_client s3c_dma_client_in = {
	.name = "I2S PCM Stereo in"
};

static struct s5p_pcm_dma_params s3c_i2s_pcm_stereo_out = {
	.client		= &s3c_dma_client_out,
	.channel	= S3C_DMACH_I2S_OUT,
	.dma_addr	= S3C_IIS_PABASE + S3C_IISTXD,
	.dma_size	= 4,
};

static struct s5p_pcm_dma_params s3c_i2s_pcm_stereo_in = {
	.client		= &s3c_dma_client_in,
	.channel	= S3C_DMACH_I2S_IN,
	.dma_addr	= S3C_IIS_PABASE + S3C_IISRXD,
	.dma_size	= 4,
};

struct s3c_i2s_info {
	void __iomem  		*regs;
	struct clk    		*iis_clk;
	struct clk    		*audio_bus;
	struct work_struct 	work;
	u32           iiscon;
	u32           iismod;
	u32           iisfic;
	u32           iispsr;
	u32           iisahb;
	u32           slave;
	u32           clk_rate;
	
#ifdef CONFIG_S5P64XX_LPAUDIO
	unsigned long idma_end;
	unsigned int  idma_period;
#endif
};

/* Structure For to control Audio Subsystem */
struct s3c_ass_info {
	void __iomem *clkcon;
	void __iomem *commbox;
};

static struct s3c_ass_info 	s3c_ass;
struct s3c_i2s_info 		s3c_i2s;
struct s5p_i2s_pdata 		s3c_i2s_pdat;

#ifdef CONFIG_S5P64XX_LPAUDIO
static int LP_AUDIO_EN = 1;
#else
static int LP_AUDIO_EN = 0;
#endif /* CONFIG_S5P64XX_LPAUDIO */

#define S3C_IISFIC_LP 				(LP_AUDIO_EN ? S3C_IISFICS : S3C_IISFIC)
#define S3C_IISCON_TXDMACTIVE_LP 	(LP_AUDIO_EN ? S3C_IISCON_TXSDMACTIVE : S3C_IISCON_TXDMACTIVE)
#define S3C_IISCON_TXDMAPAUSE_LP 	(LP_AUDIO_EN ? S3C_IISCON_TXSDMAPAUSE : S3C_IISCON_TXDMAPAUSE)
#define S3C_IISMOD_BLCMASK_LP		(LP_AUDIO_EN ? S3C_IISMOD_BLCSMASK : S3C_IISMOD_BLCPMASK)
#define S3C_IISMOD_8BIT_LP			(LP_AUDIO_EN ? S3C_IISMOD_S8BIT : S3C_IISMOD_P8BIT)
#define S3C_IISMOD_16BIT_LP			(LP_AUDIO_EN ? S3C_IISMOD_S16BIT : S3C_IISMOD_P16BIT)
#define S3C_IISMOD_24BIT_LP			(LP_AUDIO_EN ? S3C_IISMOD_S24BIT : S3C_IISMOD_P24BIT)

#define dump_i2s()	do {																\
				printk("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);					\
				printk("\tS3C_IISCON : %x\n", readl(s3c_i2s.regs + S3C_IISCON));		\
				printk("\tS3C_IISMOD : %x\n", readl(s3c_i2s.regs + S3C_IISMOD));		\
				printk("\tS3C_IISFIC : %x\n", readl(s3c_i2s.regs + S3C_IISFIC_LP));		\
				printk("\tS3C_IISPSR : %x\n", readl(s3c_i2s.regs + S3C_IISPSR));		\
				printk("\tS3C_IISAHB : %x\n", readl(s3c_i2s.regs + S3C_IISAHB));		\
				printk("\tS3C_IISSTR : %x\n", readl(s3c_i2s.regs + S3C_IISSTR));		\
				printk("\tS3C_IISSIZE : %x\n", readl(s3c_i2s.regs + S3C_IISSIZE));		\
				printk("\tS3C_IISADDR0 : %x\n", readl(s3c_i2s.regs + S3C_IISADDR0));	\
				} while(0)

#ifdef CONFIG_SND_S5P_RP
/* s5p_rp_is_running is from s5p_rp driver */
extern int s5p_rp_is_running;
#endif

static void s5p_snd_txctrl(int on)
{
	u32 iiscon;

	iiscon  = readl(s3c_i2s.regs + S3C_IISCON);

	if (on) {
		iiscon |= S3C_IISCON_I2SACTIVE;
		iiscon  &= ~S3C_IISCON_TXCHPAUSE;
		iiscon  &= ~S3C_IISCON_TXDMAPAUSE_LP;
		iiscon  |= S3C_IISCON_TXDMACTIVE_LP;
		writel(iiscon,  s3c_i2s.regs + S3C_IISCON);
	} else {
#ifdef CONFIG_SND_S5P_RP
		if (!s5p_rp_is_running) {				/* Check RP is running */
			printk("s3c_snd_txctrl(off) - RP is off (pending)\n");
			if (!(iiscon & S3C_IISCON_RXDMACTIVE)) /* Stop only if RX not active */
				iiscon &= ~S3C_IISCON_I2SACTIVE;
			iiscon  |= S3C_IISCON_TXCHPAUSE;
			iiscon  |= S3C_IISCON_TXDMAPAUSE_LP;
			iiscon  &= ~S3C_IISCON_TXDMACTIVE_LP;
			writel(iiscon,  s3c_i2s.regs + S3C_IISCON);
		}
		else {
			printk("s3c_snd_txctrl(off) - ignoring, RP is still running......\n");
		}
#else
		if (!(iiscon & S3C_IISCON_RXDMACTIVE)) /* Stop only if RX not active */
			iiscon &= ~S3C_IISCON_I2SACTIVE;
		iiscon  |= S3C_IISCON_TXCHPAUSE;
		iiscon  |= S3C_IISCON_TXDMAPAUSE_LP;
		iiscon  &= ~S3C_IISCON_TXDMACTIVE_LP;
		writel(iiscon,  s3c_i2s.regs + S3C_IISCON);
#endif
	}
}

static void s5p_snd_rxctrl(int on)
{
	u32 iiscon;

	iiscon  = readl(s3c_i2s.regs + S3C_IISCON);

	if(on){
		iiscon |= S3C_IISCON_I2SACTIVE;
		iiscon  &= ~S3C_IISCON_RXCHPAUSE;
		iiscon  &= ~S3C_IISCON_RXDMAPAUSE;
		iiscon  |= S3C_IISCON_RXDMACTIVE;
		writel(iiscon,  s3c_i2s.regs + S3C_IISCON);
	}else{
		if(!(iiscon & S3C_IISCON_TXDMACTIVE_LP)) /* Stop only if TX not active */
			iiscon &= ~S3C_IISCON_I2SACTIVE;
		iiscon  |= S3C_IISCON_RXCHPAUSE;
		iiscon  |= S3C_IISCON_RXDMAPAUSE;
		iiscon  &= ~S3C_IISCON_RXDMACTIVE;
		writel(iiscon,  s3c_i2s.regs + S3C_IISCON);
	}
}

/*
 * Wait for the LR signal to allow synchronisation to the L/R clock
 * from the codec. May only be needed for slave mode.
 */
static int s5p_snd_lrsync(void)
{
	u32 iiscon;
	int timeout = 50; /* 5ms */

	while (1) {
		iiscon = readl(s3c_i2s.regs + S3C_IISCON);
		if (iiscon & S3C_IISCON_LRI)
			break;

		if (!timeout--)
			return -ETIMEDOUT;
		
		udelay(100);
	}

	return 0;
}

/*
 * Set s5p_ I2S DAI format
 */
static int s5p_i2s_set_fmt(struct snd_soc_dai *cpu_dai,
		unsigned int fmt)
{
	u32 iismod;

	iismod = readl(s3c_i2s.regs + S3C_IISMOD);
	iismod &= ~S3C_IISMOD_SDFMASK;

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBM_CFM:
		s3c_i2s.slave = 1;
		break;
	case SND_SOC_DAIFMT_CBS_CFS:
		s3c_i2s.slave = 0;
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		iismod &= ~S3C_IISMOD_MSB;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		iismod |= S3C_IISMOD_MSB;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		iismod |= S3C_IISMOD_LSB;
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		iismod &= ~S3C_IISMOD_LRP;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		iismod |= S3C_IISMOD_LRP;
		break;
	case SND_SOC_DAIFMT_IB_IF:
	case SND_SOC_DAIFMT_IB_NF:
	default:
		printk("Inv-combo(%d) not supported!\n", fmt & SND_SOC_DAIFMT_FORMAT_MASK);
		return -EINVAL;
	}

	writel(iismod, s3c_i2s.regs + S3C_IISMOD);
	return 0;
}

static int s5p_i2s_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	u32 iismod;
	unsigned long idma_totbytes;

    if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
#ifdef CONFIG_S5P64XX_LPAUDIO
        idma_totbytes = params_buffer_bytes(params) * CONFIG_ANDROID_BUF_NUM;
        s3c_i2s.idma_end = LP_TXBUFF_ADDR + idma_totbytes;
        s3c_i2s.idma_period = params_periods(params);
#endif
		rtd->dai->cpu_dai->dma_data = &s3c_i2s_pcm_stereo_out;
	}
	else
		rtd->dai->cpu_dai->dma_data = &s3c_i2s_pcm_stereo_in;

	/* Working copies of register */
	iismod = readl(s3c_i2s.regs + S3C_IISMOD);
#ifdef CONFIG_SND_S5P_RP
	iismod &= ~(S3C_IISMOD_BLCMASK | S3C_IISMOD_BLCPMASK | S3C_IISMOD_BLCSMASK);
#else
	iismod &= ~(S3C_IISMOD_BLCMASK | S3C_IISMOD_BLCMASK_LP);
#endif

	/* TODO */
	switch(params_channels(params)) {
	case 1:
		s3c_i2s_pcm_stereo_in.dma_size = 2;
		break;
	case 2:
		s3c_i2s_pcm_stereo_in.dma_size = 4;
		break;
	case 4:
		break;
	case 6:
		break;
	default:
		break;
	}

	/* RFS & BFS are set by dai_link(machine specific) code via set_clkdiv */
#ifdef CONFIG_SND_S5P_RP
	/* Set same pcm format for primary & secondary port */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S8:
		iismod |= S3C_IISMOD_8BIT | S3C_IISMOD_S8BIT | S3C_IISMOD_P8BIT;
 		break;
 	case SNDRV_PCM_FORMAT_S16_LE:
 		iismod |= S3C_IISMOD_16BIT | S3C_IISMOD_S16BIT | S3C_IISMOD_P16BIT;
 		break;
 	case SNDRV_PCM_FORMAT_S24_LE:
 		iismod |= S3C_IISMOD_24BIT | S3C_IISMOD_S24BIT | S3C_IISMOD_P24BIT;
 		break;
	default:
		return -EINVAL;
 	}
#else
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S8:
		iismod |= S3C_IISMOD_8BIT | S3C_IISMOD_8BIT_LP;
 		break;
 	case SNDRV_PCM_FORMAT_S16_LE:
 		iismod |= S3C_IISMOD_16BIT | S3C_IISMOD_16BIT_LP;
 		break;
 	case SNDRV_PCM_FORMAT_S24_LE:
 		iismod |= S3C_IISMOD_24BIT | S3C_IISMOD_24BIT_LP;
 		break;
	default:
		return -EINVAL;
 	}
#endif
	writel(iismod, s3c_i2s.regs + S3C_IISMOD);

	return 0;
}

void s5p_i2s_clk_gating(unsigned int flag)
{
	if(flag)
	{
		u32 tmp;

		// Power Gating: NORMAL_CFG
		tmp = __raw_readl(S5P_NORMAL_CFG);
		if(!(tmp & (1 << 7)))
		{
			printk("%s: NORMAL_CFG(0x%x)\n", __func__, tmp);
		}
		
		tmp |= (1 << 7); // AUDIO[7]
		__raw_writel(tmp, S5P_NORMAL_CFG);

		// Clock Gating: Clock Gate Block
		tmp = __raw_readl(S5P_CLKGATE_BLOCK);
		if(!(tmp & S5P_CLKGATE_BLOCK_AUDIO))
		{
			printk("%s: CLKGATE_BLOCK(0x%x)\n", __func__, tmp);
		}
				
		tmp |= S5P_CLKGATE_BLOCK_AUDIO; // CLK_AUDIO[8]
		__raw_writel(tmp, S5P_CLKGATE_BLOCK);

		// i2s clk enable
		clk_enable(s3c_i2s.iis_clk);
		clk_enable(s3c_i2s.audio_bus);
	}
}

static int s5p_i2s_startup(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	u32 iiscon, iisfic;

    s5p_i2s_clk_gating(1);

	iiscon = readl(s3c_i2s.regs + S3C_IISCON);

	/* FIFOs must be flushed before enabling PSR and other MOD bits, so we do it here. */
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
		if(iiscon & S3C_IISCON_TXDMACTIVE_LP)
			return 0;

		iisfic = readl(s3c_i2s.regs + S3C_IISFIC_LP);
		iisfic |= S3C_IISFIC_TFLUSH;
		writel(iisfic, s3c_i2s.regs + S3C_IISFIC_LP);

		do{
	   	   cpu_relax();
		   //iiscon = __raw_readl(s3c_i2s.regs + S3C_IISCON);
		}while((__raw_readl(s3c_i2s.regs + S3C_IISFIC) >> 8) & 0x7f);

		iisfic = readl(s3c_i2s.regs + S3C_IISFIC_LP);
		iisfic &= ~S3C_IISFIC_TFLUSH;
		writel(iisfic, s3c_i2s.regs + S3C_IISFIC_LP);
	}else{
		if(iiscon & S3C_IISCON_RXDMACTIVE)
			return 0;

		iisfic = readl(s3c_i2s.regs + S3C_IISFIC);
		iisfic |= S3C_IISFIC_RFLUSH;
		writel(iisfic, s3c_i2s.regs + S3C_IISFIC);

		do{
	   	   cpu_relax();
		   //iiscon = readl(s3c_i2s.regs + S3C_IISCON);
		}while((__raw_readl(s3c_i2s.regs + S3C_IISFIC) >> 0) & 0x7f);

		iisfic = readl(s3c_i2s.regs + S3C_IISFIC);
		iisfic &= ~S3C_IISFIC_RFLUSH;
		writel(iisfic, s3c_i2s.regs + S3C_IISFIC);
	}

	return 0;
}

static int s5p_i2s_prepare(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	u32 iismod;

	iismod = readl(s3c_i2s.regs + S3C_IISMOD);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK){
		if((iismod & S3C_IISMOD_TXRMASK) == S3C_IISMOD_RX){
			iismod &= ~S3C_IISMOD_TXRMASK;
			iismod |= S3C_IISMOD_TXRX;
		}
	}else{
		if((iismod & S3C_IISMOD_TXRMASK) == S3C_IISMOD_TX){
			iismod &= ~S3C_IISMOD_TXRMASK;
			iismod |= S3C_IISMOD_TXRX;
		}
	}

	writel(iismod, s3c_i2s.regs + S3C_IISMOD);

	return 0;
}

static int s5p_i2s_trigger(struct snd_pcm_substream *substream, int cmd, struct snd_soc_dai *dai)
{
	int ret = 0;

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_RESUME:
	case SNDRV_PCM_TRIGGER_START:
	case SNDRV_PCM_TRIGGER_PAUSE_RELEASE:
		if (s3c_i2s.slave) {
			ret = s5p_snd_lrsync();
			if (ret)
				goto exit_err;
		}

		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
			s5p_snd_rxctrl(1);
		else
			s5p_snd_txctrl(1);
		break;
	case SNDRV_PCM_TRIGGER_SUSPEND:
	case SNDRV_PCM_TRIGGER_STOP:
	case SNDRV_PCM_TRIGGER_PAUSE_PUSH:
		if (substream->stream == SNDRV_PCM_STREAM_CAPTURE)
			s5p_snd_rxctrl(0);
		else
			s5p_snd_txctrl(0);
		break;
	default:
		ret = -EINVAL;
		break;
	}

exit_err:
	return ret;
}

/*
 * Set s5p_ Clock source
 * Since, we set frequencies using PreScaler and BFS, RFS, we select input clock source to the IIS here.
 */
static int s5p_i2s_set_sysclk(struct snd_soc_dai *cpu_dai,
	int clk_id, unsigned int freq, int dir)
{
	u32 iismod = readl(s3c_i2s.regs + S3C_IISMOD);

	switch (clk_id) {
	case S3C_CLKSRC_PCLK: /* IIS-IP is Master and derives its clocks from PCLK */
		if(s3c_i2s.slave)
			return -EINVAL;
		iismod &= ~S3C_IISMOD_IMSMASK;
		iismod |= clk_id;
		s3c_i2s.clk_rate = clk_get_rate(s3c_i2s.iis_clk);
		break;

#ifdef USE_CLKAUDIO
	case S3C_CLKSRC_CLKAUDIO: /* IIS-IP is Master and derives its clocks from I2SCLKD2 */
		if(s3c_i2s.slave)
			return -EINVAL;
		iismod &= ~S3C_IISMOD_IMSMASK;
		iismod |= clk_id;
		break;
#endif

	case S3C_CLKSRC_SLVPCLK: /* IIS-IP is Slave, but derives its clocks from PCLK */
	case S3C_CLKSRC_I2SEXT:  /* IIS-IP is Slave and WM8994 Audio Codec Chip derives the clock via I2SCLK(WM8994 is master) */
		iismod &= ~S3C_IISMOD_IMSMASK;
		iismod |= (clk_id | S3C_IISMOD_OPPCLK | (dir << 12));

		break;

	case S3C_CDCLKSRC_INT:
		iismod &= ~S3C_IISMOD_CDCLKCON;
		break;

	case S3C_CDCLKSRC_EXT:
		iismod |= S3C_IISMOD_CDCLKCON;
		//iismod &= ~S3C_IISMOD_CDCLKCON; 	
		break; 
	case S5P_CLKSRC_MUX:
		iismod |= S3C_IISMOD_MSTCLKAUDIO;
//                iismod |= (S3C_IISMOD_MSTCLKAUDIO | S3C_IISMOD_OPPCLK | (dir << 12));//robin
//                iismod |= (S3C_IISMOD_MSTCLKAUDIO | S3C_IISMOD_OPPCLK | (dir << 12));//robin
		break;

	default:
		return -EINVAL;
	}

	writel(iismod, s3c_i2s.regs + S3C_IISMOD);
	return 0;
}

/*
 * Set s5p_ Clock dividers
 * NOTE: NOT all combinations of RFS, BFS and BCL are supported! XXX
 * Machine specific(dai-link) code must consider that while setting MCLK and BCLK in this function. XXX
 */
/* XXX BLC(bits-per-channel) --> BFS(bit clock shud be >= FS*(Bit-per-channel)*2) XXX */
/* XXX BFS --> RFS_VAL(must be a multiple of BFS)                                 XXX */
/* XXX RFS_VAL & SRC_CLK --> Prescalar Value(SRC_CLK / RFS_VAL / fs - 1)          XXX */
static int s5p_i2s_set_clkdiv(struct snd_soc_dai *cpu_dai,
	int div_id, int div)
{
	u32 reg;

	switch (div_id) {
	case S3C_DIV_MCLK:
		reg = readl(s3c_i2s.regs + S3C_IISMOD) & ~S3C_IISMOD_RFSMASK;
		switch(div) {
		case 256: div = S3C_IISMOD_256FS; break;
		case 512: div = S3C_IISMOD_512FS; break;
		case 384: div = S3C_IISMOD_384FS; break;
		case 768: div = S3C_IISMOD_768FS; break;
		default: return -EINVAL;
		}
		writel(reg | div, s3c_i2s.regs + S3C_IISMOD);
		break;
	case S3C_DIV_BCLK:
		reg = readl(s3c_i2s.regs + S3C_IISMOD) & ~S3C_IISMOD_BFSMASK;
		switch(div) {
		case 16: div = S3C_IISMOD_16FS; break;
		case 24: div = S3C_IISMOD_24FS; break;
		case 32: div = S3C_IISMOD_32FS; break;
		case 48: div = S3C_IISMOD_48FS; break;
		default: return -EINVAL;
		}
		writel(reg | div, s3c_i2s.regs + S3C_IISMOD);
		break;
	case S3C_DIV_PRESCALER:
		reg = readl(s3c_i2s.regs + S3C_IISPSR) & ~S3C_IISPSR_PSRAEN;
		writel(reg, s3c_i2s.regs + S3C_IISPSR);
		reg = readl(s3c_i2s.regs + S3C_IISPSR) & ~S3C_IISPSR_PSVALA;
		div &= 0x3f;
		writel(reg | (div<<8) | S3C_IISPSR_PSRAEN, s3c_i2s.regs + S3C_IISPSR);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static irqreturn_t s5p_iis_irq(int irqno, void *dev_id)
{
	u32 iiscon, iisahb, iisfic;

	iisahb  = readl(s3c_i2s.regs + S3C_IISAHB);
	iiscon  = readl(s3c_i2s.regs + S3C_IISCON);
	if(iiscon & S3C_IISCON_FTXSURSTAT) {
		iiscon |= S3C_IISCON_FTXURSTATUS;
		writel(iiscon, s3c_i2s.regs + S3C_IISCON);
		s5pdbg("TX_S underrun interrupt IISCON = 0x%08x\n", readl(s3c_i2s.regs + S3C_IISCON));
	}
	if(iiscon & S3C_IISCON_FTXURSTATUS) {
		iiscon &= ~S3C_IISCON_FTXURINTEN;
		iiscon |= S3C_IISCON_FTXURSTATUS;
		writel(iiscon, s3c_i2s.regs + S3C_IISCON);
		s5pdbg("TX_P underrun interrupt IISCON = 0x%08x\n", readl(s3c_i2s.regs + S3C_IISCON));
	}
	if(iiscon & S3C_IISCON_FRXOFSTAT) {
		iisfic = readl(s3c_i2s.regs + S3C_IISFIC); 
		iiscon &= ~S3C_IISCON_FRXOFINTEN;
		iiscon |= S3C_IISCON_FRXOFSTAT;
		iisfic |= S3C_IISFIC_RFLUSH;
		writel(iiscon, s3c_i2s.regs + S3C_IISCON);
		writel(iisfic, s3c_i2s.regs + S3C_IISFIC); 
		s5pdbg("RX overrun interrupt IISCON = 0x%08x\n", readl(s3c_i2s.regs + S3C_IISCON));
	}

#ifdef CONFIG_S5P64XX_LPAUDIO
	u32 val = 0;
	u32 addr0 = 0;

	/* Check internal DMA level interrupt. */
	if(iisahb & S3C_IISAHB_LVL0INT) 
		val = S3C_IISAHB_CLRLVL0 | S3C_IISAHB_INTENLVL3;	
	else if(iisahb & S3C_IISAHB_LVL1INT) 
		val = S3C_IISAHB_CLRLVL1;
	else if(iisahb & S3C_IISAHB_LVL2INT) 
		val = S3C_IISAHB_CLRLVL2;
	else if(iisahb & S3C_IISAHB_LVL3INT) 
		val = S3C_IISAHB_CLRLVL3;
	else
		val = 0;
	
	if(val) {
			iisahb |= val;
			writel(iisahb, s3c_i2s.regs + S3C_IISAHB);	
		
		/* Finished dma transfer ? */
		if(iisahb & S3C_IISLVLINTMASK) {
			if(s3c_i2s_pdat.dma_cb) 
				if(CONFIG_ANDROID_BUF_NUM == 1)
					s3c_i2s_pdat.dma_cb(s3c_i2s_pdat.dma_token, s3c_i2s_pdat.dma_prd);
				else
					s3c_i2s_pdat.dma_cb(s3c_i2s_pdat.dma_token, s3c_i2s_pdat.dma_prd * s3c_i2s.idma_period);
		}
	}
#endif

	// To avoid ALSA overflow from Aries project
	if(readl(s3c_i2s.regs + S3C_IISFIC) & (1<<7))
		writel(readl(s3c_i2s.regs + S3C_IISFIC) & (~(1<<7)) , s3c_i2s.regs + S3C_IISFIC); //reset flush rx

	return IRQ_HANDLED;
}

static void init_i2s(void)
{
	u32 iiscon, iismod, iisahb;

	writel(S3C_IISCON_I2SACTIVE | S3C_IISCON_SWRESET, s3c_i2s.regs + S3C_IISCON);

	iiscon  = readl(s3c_i2s.regs + S3C_IISCON);
	iismod  = readl(s3c_i2s.regs + S3C_IISMOD);
	iisahb  = readl(s3c_i2s.regs + S3C_IISAHB);

	/* Enable interrupts */
	iiscon |= S3C_IISCON_FRXOFINTEN;	
	iiscon |= S3C_IISCON_FTXSURINTEN; 
	iismod &= ~S3C_IISMOD_OPMSK;
	iismod |= S3C_IISMOD_OPCCO;

#ifdef CONFIG_S5P64XX_LPAUDIO
	iismod |= S3C_IISMOD_TXSLP;
	iisahb |= (S3C_IISAHB_DMARLD | S3C_IISAHB_DISRLDINT);
#else
	iismod &= ~S3C_IISMOD_TXSLP;
	iisahb &= ~(S3C_IISAHB_DMARLD | S3C_IISAHB_DISRLDINT);
#endif

#ifdef CONFIG_SND_S5P_RP
	/* ULP Audio use Secondary Port via iDMA */
        iismod |= S3C_IISMOD_TXSLP;             /* OP_MUX_SEL = 1, FIFO_S gets data from iDMA */
        iisahb |= (S3C_IISAHB_DMARLD | S3C_IISAHB_DISRLDINT);
#endif


	writel(iisahb, s3c_i2s.regs + S3C_IISAHB);
	writel(iismod, s3c_i2s.regs + S3C_IISMOD);
	writel(iiscon, s3c_i2s.regs + S3C_IISCON);
}

static void s5p_i2s_setmode(void)
{
	spin_lock_init(&s3c_i2s_pdat.lock);

	s3c_i2s_pdat.i2s_dai.capture.channels_min = 1;
	s3c_i2s_pdat.i2s_dai.capture.channels_max = 2;

#ifdef CONFIG_S5P64XX_LPAUDIO
	s3c_i2s_pcm_stereo_out.dma_addr = S3C_IIS_PABASE + S3C_IISTXDS;
#else
	s3c_i2s_pcm_stereo_out.dma_addr = S3C_IIS_PABASE + S3C_IISTXD;
#endif
}


static int s5p_i2s_probe(struct platform_device *pdev,
			     struct snd_soc_dai *dai)
{
	int ret = 0;
	u32 ass_clkcon = 0;
	u32 ass_clkdiv = 0;

	struct clk *cf, *cm, *cn;

	s3c_i2s.regs = ioremap(S3C_IIS_PABASE, 0x100);
	if (s3c_i2s.regs == NULL)
		return -ENXIO;
	
	/* Remap virtual address for ASS(CLKCON, COMMBOX, SRAM Buf) - By Jung */
	s3c_ass.clkcon = ioremap(S5P_ASS_CLK_PABASE, 0x8);
	s3c_ass.commbox = ioremap(S5P_ASS_COM_PABASE, 0x208);
	if (s3c_ass.clkcon == NULL || s3c_ass.commbox == NULL)
		return -ENXIO;
	
	ass_clkcon = readl(s3c_ass.clkcon + S5P_ASS_CLKCON);
//	ass_clkcon |= S5P_ASS_CLKCON_XXTI;
	ass_clkcon |= S5P_ASS_CLKCON_EPLL;
	writel(ass_clkcon, s3c_ass.clkcon + S5P_ASS_CLKCON);
	
	ret = request_irq(S3C_IISIRQ, s5p_iis_irq, 0, "s5p-i2s", pdev);
	if (ret < 0) {
		printk("fail to claim i2s irq , ret = %d\n", ret);
		iounmap(s3c_i2s.regs);
		return -ENODEV;
	}

	s3c_i2s.iis_clk = clk_get(&pdev->dev, PCLKCLK);
	if (IS_ERR(s3c_i2s.iis_clk)) {
		printk("failed to get clk(%s)\n", PCLKCLK);
		goto lb4;
	}
	s5pdbg("Got Clock -> %s\n", PCLKCLK);
	clk_enable(s3c_i2s.iis_clk);
	s3c_i2s.clk_rate = clk_get_rate(s3c_i2s.iis_clk);

#ifdef USE_CLKAUDIO
	/* To avoid switching between sources(LP vs NM mode),
	 * we use EXTPRNT as parent clock of i2sclkd2.
	 */
	s3c_i2s.audio_bus = clk_get(NULL, "fout_epll");
	if (IS_ERR(s3c_i2s.audio_bus)) {
		printk("failed to get clk fout_epll\n");
		goto lb3;
	}
	s3c_i2s.clk_rate = clk_get_rate(s3c_i2s.audio_bus);
	printk("audio bus clk = %ld\n", s3c_i2s.clk_rate);

#ifndef CONFIG_SND_WM8994_MASTER_MODE
#else
	// [i2s : slave, wm8994 : master]
#endif    // CONFIG_SND_WM8994_MASTER_MODE
	
	clk_enable(s3c_i2s.audio_bus);
	s3c_i2s.clk_rate = clk_get_rate(s3c_i2s.audio_bus);
#endif	// USE_CLKAUDIO

	init_i2s();

	s5p_snd_txctrl(0);
	s5p_snd_rxctrl(0);

	return 0;

#ifdef USE_CLKAUDIO
lb:	
	clk_put(cf);    // CONFIG_SND_WM8994_MASTER_MODE
lb1:
	clk_put(cm);
lb2:
	clk_put(s3c_i2s.audio_bus);
#endif
lb3:
	clk_disable(s3c_i2s.iis_clk);
	clk_put(s3c_i2s.iis_clk);
lb4:
	free_irq(S3C_IISIRQ, pdev);
	iounmap(s3c_i2s.regs);

	return -ENODEV;
}

static void s5p_i2s_remove(struct platform_device *pdev,
		       struct snd_soc_dai *dai)
{	
	writel(0, s3c_i2s.regs + S3C_IISCON);

#ifdef USE_CLKAUDIO
	clk_disable(s3c_i2s.audio_bus);
	clk_put(s3c_i2s.audio_bus);
#endif
	clk_disable(s3c_i2s.iis_clk);
	clk_put(s3c_i2s.iis_clk);
	free_irq(S3C_IISIRQ, pdev);
	iounmap(s3c_i2s.regs);
}

#ifdef CONFIG_PM
static int s5p_i2s_suspend(struct snd_soc_dai *cpu_dai)
{
	s3c_i2s.iiscon = readl(s3c_i2s.regs + S3C_IISCON);
	s3c_i2s.iismod = readl(s3c_i2s.regs + S3C_IISMOD);
	s3c_i2s.iisfic = readl(s3c_i2s.regs + S3C_IISFIC_LP);
	s3c_i2s.iispsr = readl(s3c_i2s.regs + S3C_IISPSR);
	s3c_i2s.iisahb = readl(s3c_i2s.regs + S3C_IISAHB);
	clk_disable(s3c_i2s.iis_clk);

	return 0;
}

static int s5p_i2s_resume(struct snd_soc_dai *cpu_dai)
{
	clk_enable(s3c_i2s.iis_clk);

	writel(s3c_i2s.iiscon, s3c_i2s.regs + S3C_IISCON);
	writel(s3c_i2s.iismod, s3c_i2s.regs + S3C_IISMOD);
	writel(s3c_i2s.iisfic, s3c_i2s.regs + S3C_IISFIC_LP);
	writel(s3c_i2s.iispsr, s3c_i2s.regs + S3C_IISPSR);
	writel(s3c_i2s.iisahb, s3c_i2s.regs + S3C_IISAHB);

	return 0;
}
#else
#define s5p_i2s_suspend NULL
#define s5p_i2s_resume NULL
#endif

static void s5p_i2sdma_getpos(dma_addr_t *src, dma_addr_t *dst)
{
	*dst = s3c_i2s_pcm_stereo_out.dma_addr;
	*src = LP_TXBUFF_ADDR + (readl(s3c_i2s.regs + S3C_IISTRNCNT) & 0xffffff) * 4;
}

#ifdef CONFIG_S5P64XX_LPAUDIO
static int s5p_i2sdma_enqueue(void *id)
{
	u32 val;

	spin_lock(&s3c_i2s_pdat.lock);
	s3c_i2s_pdat.dma_token = id;
	spin_unlock(&s3c_i2s_pdat.lock);
	
	s5pdbg("%s: %d@%x\n", __func__, MAX_LP_BUFF, LP_TXBUFF_ADDR);
	s5pdbg("CB @%x", LP_TXBUFF_ADDR + MAX_LP_BUFF);
	if(s3c_i2s_pdat.dma_prd != MAX_LP_BUFF)
		s5pdbg(" and @%x\n", LP_TXBUFF_ADDR + s3c_i2s_pdat.dma_prd);
	else
		s5pdbg("\n");

	/* Internal DMA Level3 Interrupt Address */
	val = LP_TXBUFF_ADDR;
	writel(val, s3c_i2s.regs + S3C_IISADDR3);

        val += s3c_i2s_pdat.dma_prd * s3c_i2s.idma_period;
        writel(val, s3c_i2s.regs + S3C_IISADDR0);

        val += s3c_i2s_pdat.dma_prd * s3c_i2s.idma_period;
        writel(val, s3c_i2s.regs + S3C_IISADDR1);

        val += s3c_i2s_pdat.dma_prd * s3c_i2s.idma_period;
        writel(val, s3c_i2s.regs + S3C_IISADDR2);

	/* Start address0 of I2S internal DMA operation. */
	val = readl(s3c_i2s.regs + S3C_IISSTR);
	val = LP_TXBUFF_ADDR;
	writel(val, s3c_i2s.regs + S3C_IISSTR);

	/* 
	 * Transfer block size for I2S internal DMA.
	 * Should decide transfer size before start dma operation 
	 */
	val = readl(s3c_i2s.regs + S3C_IISSIZE);
	val &= ~(S3C_IISSIZE_TRNMSK << S3C_IISSIZE_SHIFT);
	val |= ((((s3c_i2s.idma_end & 0x1ffff) >> 2) & S3C_IISSIZE_TRNMSK) << S3C_IISSIZE_SHIFT);

	writel(val, s3c_i2s.regs + S3C_IISSIZE);

	return 0;
}

static void s5p_i2sdma_setcallbk(void (*cb)(void *id, int result), unsigned prd)
{
	if(!prd || prd > MAX_LP_BUFF)
	   prd = MAX_LP_BUFF;

	spin_lock(&s3c_i2s_pdat.lock);
	s3c_i2s_pdat.dma_cb = cb;
	s3c_i2s_pdat.dma_prd = prd;
	spin_unlock(&s3c_i2s_pdat.lock);

	s5pdbg("%s:%d dma_period=%d\n", __func__, __LINE__, s3c_i2s_pdat.dma_prd);
}

static void s5p_i2sdma_ctrl(int state)
{
	u32 val;

	spin_lock(&s3c_i2s_pdat.lock);

	val = readl(s3c_i2s.regs + S3C_IISAHB);

	switch(state){
	   case S3C_I2SDMA_START:
			val |= (S3C_IISAHB_INTENLVL0 | S3C_IISAHB_INTENLVL1 | S3C_IISAHB_INTENLVL2 | S3C_IISAHB_DMAEN);
		                  break;
	   case S3C_I2SDMA_SUSPEND:
	   case S3C_I2SDMA_RESUME:
		                  break;
	   case S3C_I2SDMA_FLUSH:
	   case S3C_I2SDMA_STOP: 
			/* Disable LVL Interrupt and DMA Operation */
			val &= ~(S3C_IISAHB_INTENLVL0 | S3C_IISAHB_INTENLVL1 | S3C_IISAHB_INTENLVL2 | S3C_IISAHB_INTENLVL3 | S3C_IISAHB_DMAEN);
            break;
	   default:
		  spin_unlock(&s3c_i2s_pdat.lock);
		  return;
	}
			
	writel(val, s3c_i2s.regs + S3C_IISAHB);
	s3c_i2s_pdat.dma_state = state;

	spin_unlock(&s3c_i2s_pdat.lock);
}
#endif

#ifdef CONFIG_SND_S5P_RP
void s5p_i2s_idma_enable(void)
{
	u32 val;

	val  = readl(s3c_i2s.regs + S3C_IISAHB);
	val |= (S3C_IISAHB_DMARLD | S3C_IISAHB_DISRLDINT | S3C_IISAHB_DMAEN);

	writel(val, s3c_i2s.regs + S3C_IISAHB);
}
EXPORT_SYMBOL(s5p_i2s_idma_enable);

void s5p_i2s_idma_pause(void)
{
	u32 val;

	val  = readl(s3c_i2s.regs + S3C_IISAHB);
	val &= ~(S3C_IISAHB_DMARLD);

	writel(val, s3c_i2s.regs + S3C_IISAHB);
}
EXPORT_SYMBOL(s5p_i2s_idma_pause);
#endif

static struct snd_soc_dai_ops s5p_i2s_dai_ops = {
	.hw_params = s5p_i2s_hw_params,
	.prepare   = s5p_i2s_prepare,
	.startup   = s5p_i2s_startup,
	.trigger   = s5p_i2s_trigger,
	.set_fmt = s5p_i2s_set_fmt,
	.set_clkdiv = s5p_i2s_set_clkdiv,
	.set_sysclk = s5p_i2s_set_sysclk,
};

struct s5p_i2s_pdata s3c_i2s_pdat = {
	.set_mode = s5p_i2s_setmode,
	.p_rate = &s3c_i2s.clk_rate,
	.dma_getpos = s5p_i2sdma_getpos,
#ifdef CONFIG_S5P64XX_LPAUDIO
	.dma_enqueue = s5p_i2sdma_enqueue,
	.dma_setcallbk = s5p_i2sdma_setcallbk,
#endif
	.dma_token = NULL,
	.dma_cb = NULL,
#ifdef CONFIG_S5P64XX_LPAUDIO
	.dma_ctrl = s5p_i2sdma_ctrl,
#endif
	.i2s_dai = {
		.name = "s5p-i2s",
		.id = 0,
		.probe = s5p_i2s_probe,
		.remove = s5p_i2s_remove,
		.suspend = s5p_i2s_suspend,
		.resume = s5p_i2s_resume,
		.playback = {
			.channels_min = 2,
			.channels_max = PLBK_CHAN,
			.rates = SNDRV_PCM_RATE_8000_96000,
			.formats = SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
			},
		.capture = {
			.channels_min = 2,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_96000,
			.formats = SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
		},
		.ops = &s5p_i2s_dai_ops,
	},
};

EXPORT_SYMBOL_GPL(s3c_i2s_pdat);

static int __init s5p_i2s_init(void)
{
	return snd_soc_register_dai(&s3c_i2s_pdat.i2s_dai);
}
module_init(s5p_i2s_init);

static void __exit s5p_i2s_exit(void)
{
	snd_soc_unregister_dai(&s3c_i2s_pdat.i2s_dai);
}
module_exit(s5p_i2s_exit);

/* Module information */
MODULE_AUTHOR("S.LSI");
MODULE_DESCRIPTION("Apollo S5P6442 WM8994");
MODULE_LICENSE("GPL");

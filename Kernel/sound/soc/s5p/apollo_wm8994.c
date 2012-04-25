/*
 * apollo_wm8994.c
 * reference : smdkc110_wm8580.c
 *
 * Copyright (C) 2009, Samsung Elect. Ltd. - Jaswinder Singh <jassisinghbrar@gmail.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */
#include <linux/delay.h> 
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include <asm/io.h>
#include <asm/gpio.h> 
#include <plat/gpio-cfg.h> 
#include <plat/map-base.h>
#include <plat/regs-clock.h>
#include <linux/clk.h>	// CONFIG_SND_WM8994_MASTER_MODE
#include <linux/unistd.h>	// ulseep()

#include "../codecs/wm8994_def.h"
#include "s5p-pcm.h"
#include "s5p-i2s.h"

#define PLAY_51       0
#define PLAY_STEREO   1
#define PLAY_OFF      2

#define REC_MIC    0
#define REC_LINE   1
#define REC_OFF    2


extern struct s5p_pcm_pdata s3c_pcm_pdat;
extern struct s5p_i2s_pdata s3c_i2s_pdat;

#define SRC_CLK	(*s3c_i2s_pdat.p_rate)

//#define CONFIG_SND_DEBUG
#ifdef CONFIG_SND_DEBUG
#define debug_msg(x...) printk(x)
#else
#define debug_msg(x...)
#endif

#define wait_stable(utime_out)                                  \
        do {                                                    \
                if (!utime_out)                                 \
                        utime_out = 1000;                       \
                utime_out = loops_per_jiffy / HZ * utime_out;   \
                while (--utime_out) {                           \
                        cpu_relax();                            \
                }                                               \
        } while (0);

int set_epll_rate(unsigned long rate)
{
        struct clk *fout_epll;
        unsigned int wait_utime = 100;

        fout_epll = clk_get(NULL, "fout_epll");
        if (IS_ERR(fout_epll)) {
        	printk(KERN_ERR "%s: failed to get fout_epll\n", __func__);
            return -ENOENT;
        }

//      if (rate == clk_get_rate(fout_epll))
//		goto out;

        clk_disable(fout_epll);
        wait_stable(wait_utime);

        clk_set_rate(fout_epll, rate);
        wait_stable(wait_utime);

        clk_enable(fout_epll);

out:
        clk_put(fout_epll);

        return 0;
}

/* XXX BLC(bits-per-channel) --> BFS(bit clock shud be >= FS*(Bit-per-channel)*2) XXX */
/* XXX BFS --> RFS(must be a multiple of BFS)                                 XXX */
/* XXX RFS & SRC_CLK --> Prescalar Value(SRC_CLK / RFS_VAL / fs - 1)          XXX */
int s5p64xx_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->dai->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->dai->codec_dai;
	unsigned int epll_out_rate, pll_out;
	int bfs, rfs, psr, ret;
	u32 ap_codec_clk, temp;	
	u32 sample_rate = params_rate(params);

#ifdef CONFIG_SND_S5P64XX_SOC_I2S_REC_DOWNSAMPLING
	if(sample_rate == 8000) {
		printk("%s: sample_rate 8000---->44100\n", __func__);
		sample_rate = 44100;
	}
#endif

	switch (sample_rate) {
	case 22050:
	case 44100:
	case 88200:
		rfs = 256;
		pll_out =  epll_out_rate = 67738000;
                break;
        case 22025:
        case 32000:
        case 48000:
        case 96000:
        case 24000:
		pll_out =  epll_out_rate = 45158000;
                rfs = 256; 
                break;
        case 64000:
		pll_out =  epll_out_rate = 49152000;
                rfs = 384; 
                break;
       	 case 11025:
		pll_out =  epll_out_rate = 67738000;
		rfs = 512;
                break;
        case 12000:
		pll_out =  epll_out_rate = 49152000;
                rfs = 512;
                break;
        case 8000:
        case 16000: 
		pll_out =  epll_out_rate = 49152000;
                rfs = 1024;	
                break;
        default:
                printk(KERN_ERR "%s:%d Sampling Rate %u not supported!\n",
                        __func__, __LINE__, params_rate(params));
                return -EINVAL;
        }

	debug_msg("%s\n", __FUNCTION__);

	/* Choose BFS and RFS values combination that is supported by
	 * both the WM8580 codec as well as the S3C AP
	 *
	 * WM8580 codec supports only S16_LE, S20_3LE, S24_LE & S32_LE.
	 * S3C AP supports only S8, S16_LE & S24_LE.
	 * We implement all for completeness but only S16_LE & S24_LE bit-lengths 
	 * are possible for this AP-Codec combination.
	 */
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S8:
		bfs = 16;
 		break;
 	case SNDRV_PCM_FORMAT_S16_LE:
		bfs = 32;
 		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
 	case SNDRV_PCM_FORMAT_S24_LE:
		bfs = 48;
 		break;
 	case SNDRV_PCM_FORMAT_S32_LE:	/* Impossible, as the AP doesn't support 64fs or more BFS */
	default:
		return -EINVAL;
 	}
 
	/* Set EPLL clock rate */ 
	printk("\n EPLL clock rate set = %d\n", epll_out_rate);
	ret = set_epll_rate(epll_out_rate);
	if (ret < 0) {
		printk("Cannot set Epll clock\n");
		return ret;
	}

#ifdef CONFIG_SND_WM8994_MASTER_MODE	/* I2S Slave */
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBM_CFM);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBM_CFM);
	if (ret < 0)
		return ret;	

	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C_CLKSRC_I2SEXT, 0, SND_SOC_CLOCK_OUT);
	if (ret < 0)
		return ret;

        switch (sample_rate) {
                
		case 8000:
			ap_codec_clk = 4096000; //Shaju ..need to figure out this rate
			break;
		case 11025:
			ap_codec_clk = 2822400; 
			break;
		case 12000:
			ap_codec_clk = 6144000; 
			break;
		case 16000:
			ap_codec_clk = 4096000;
			break;
		case 22050:
			ap_codec_clk = 6144000;
			break;
		case 24000:
			ap_codec_clk = 6144000;
			break;
		case 32000:
			ap_codec_clk = 8192000;
			break;
		case 44100:
			ap_codec_clk = 11289600;
			break;
		case 48000:
			ap_codec_clk = 12288000;
			break;
		default:
			ap_codec_clk = 11289600;
			break;
	}

	ret = snd_soc_dai_set_sysclk(codec_dai,WM8994_SYSCLK_FLL, ap_codec_clk, 0);
	if (ret < 0)
		return ret;

#else	/* I2S Master */ 
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S | SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C_CDCLKSRC_INT, 0, SND_SOC_CLOCK_OUT);
	if (ret < 0)
		return ret;
	
#ifdef USE_CLKAUDIO
	ret = snd_soc_dai_set_sysclk(cpu_dai, S5P_CLKSRC_MUX, 0, SND_SOC_CLOCK_IN);
#endif
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C_CLKSRC_PCLK, 0, SND_SOC_CLOCK_OUT);
	if (ret < 0)
		return ret;
	
	ret = snd_soc_dai_set_sysclk(codec_dai, WM8994_SYSCLK_MCLK, epll_out_rate/6, 0); 
        if (ret < 0)
		return ret;

	psr = epll_out_rate / rfs / params_rate(params);
	ret = epll_out_rate / rfs - psr * params_rate(params);
	if(ret >= params_rate(params)/2)	// round off
	   psr += 1;
	psr -= 1;
	
	printk("epll_out_rate=%d PSR=%d RFS=%d BFS=%d\n", epll_out_rate, psr, rfs, bfs);
	
	ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C_DIV_PRESCALER, psr);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C_DIV_MCLK, rfs);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C_DIV_BCLK, bfs);
	if (ret < 0)
		return ret;

#endif	// #ifdef CONFIG_SND_WM8994_MASTER_MODE

		return 0;
}

static int s5p64xx_wm8994_init(struct snd_soc_codec *codec)
{
	return 0;
}

static struct snd_soc_ops univ6442_ops = {
	.hw_params = s5p64xx_hw_params,
};

static struct snd_soc_dai_link univ6442_dai = {

	.name = "WM8994",
	.stream_name = "WM8994 HiFi Playback",
	.cpu_dai = &s3c_i2s_pdat.i2s_dai,
	.codec_dai = &wm8994_dai,
	.init = s5p64xx_wm8994_init,
	.ops = &univ6442_ops,
};

static struct snd_soc_card univ6442 = {
	.name = "univ6442",
	.platform = &s3c_pcm_pdat.pcm_pltfm,
	.dai_link = &univ6442_dai,
	.num_links = 1,//ARRAY_SIZE(univ6442_dai),
};

static struct wm8994_setup_data univ6442_wm8994_setup = {
	.i2c_address = 0x34>>1,
	.i2c_bus = 3,
};

static struct snd_soc_device univ6442_snd_devdata = {
	.card = &univ6442,
	.codec_dev = &soc_codec_dev_wm8994,
	.codec_data = &univ6442_wm8994_setup,
};

static struct platform_device *univ6442_snd_device;
static int __init s5p64xx_audio_init(void)
{
	int ret;
	unsigned int reg;
	
	/* Set XCLK_OUT enable */
//	reg = __raw_readl(S5P_CLK_OUT);
// 	reg &= ~S5P_CLK_OUT_MASK;		/* select clock_out source(epll_out) */
//	reg |= (0x11 << 12);
//	__raw_writel(reg, S5P_CLK_OUT);

//        reg = __raw_readl(S5P_OTHERS);
//        tmp |= S5P_OTHER_SYSC_INTOFF;
//        reg |= (0x3 << 8);
//        __raw_writel(reg, S5P_OTHERS);
	
	wm8994_dai.capture.channels_min = 1;
	wm8994_dai.capture.channels_max = 2;
	
	s3c_pcm_pdat.set_mode(&s3c_i2s_pdat);
	s3c_i2s_pdat.set_mode();

	univ6442_snd_device = platform_device_alloc("soc-audio", 0);
	if (!univ6442_snd_device)
		return -ENOMEM;

	platform_set_drvdata(univ6442_snd_device, &univ6442_snd_devdata);
	univ6442_snd_devdata.dev = &univ6442_snd_device->dev;
	ret = platform_device_add(univ6442_snd_device);

	if (ret)
		platform_device_put(univ6442_snd_device);
	
	return ret;
}

static void __exit s5p64xx_audio_exit(void)
{
	debug_msg("%s\n", __FUNCTION__);

	platform_device_unregister(univ6442_snd_device);
}

module_init(s5p64xx_audio_init);
module_exit(s5p64xx_audio_exit);

/* Module information */
MODULE_DESCRIPTION("ALSA SoC Apollo S5P6442 WM8994");
MODULE_LICENSE("GPL");

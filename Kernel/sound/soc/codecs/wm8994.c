/*
 * wm8994.c  --  WM8994 ALSA Soc Audio driver
 *
 * Copyright 2008 Wolfson Microelectronics PLC.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 * Notes:
 *  The WM8994 is a multichannel codec with S/PDIF support, featuring six
 *  DAC channels and two ADC channels.
 *
 *  Currently only the primary audio interface is supported - S/PDIF and
 *  the secondary audio interfaces are not.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/tlv.h>
#include <sound/initval.h>
#include <asm/div64.h>

#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
#include <mach/apollo.h>

#ifdef CONFIG_SND_VOODOO
#include "wm8994_voodoo.h"
#endif

#include "wm8994.h"
#include "wm8994_gain.h"

#include <linux/timer.h>

#ifdef CONFIG_CPU_FREQ
#include <plat/s5p6442-dvfs.h>
#endif


#define WM8998_VERSION "0.1"

#define AUDIO_COMMON_DEBUG
#define SUBJECT "WM8994"

#ifdef AUDIO_COMMON_DEBUG
#define DEBUG_LOG_L1(format,...)\
		printk ("[ "SUBJECT " (%s,%d) ] " format "\n", __func__, __LINE__, ## __VA_ARGS__);

#define DEBUG_LOG(format,...)\
	printk ("[ "SUBJECT " (%s,%d) ] " format "\n", __func__, __LINE__, ## __VA_ARGS__);
#else
#define DEBUG_LOG(format,...)
#endif


struct pll_state {
	unsigned int in;
	unsigned int out;
};

static struct {
	int ratio;
	int clk_sys_rate;
} clk_sys_rates[] = {
	{ 64,   0 },
	{ 128,  1 },
	{ 192,  2 },
	{ 256,  3 },
	{ 384,  4 },
	{ 512,  5 },
	{ 768,  6 },
	{ 1024, 7 },
	{ 1408, 8 },
	{ 1536, 9 },
};

static struct {
	int rate;
	int sample_rate;
} sample_rates[] = {
	{ 8000,  0  },
	{ 11025, 1  },
	{ 12000, 2  },
	{ 16000, 3  },
	{ 22050, 4  },
	{ 24000, 5  },
	{ 32000, 6  },
	{ 44100, 7  },
	{ 48000, 8  },
	{ 88200, 9  },
	{ 96000, 10  },
};

static struct {
	int div; /* *10 due to .5s */
	int bclk_div;
} bclk_divs[] = {
	{ 1,   0  },
	{ 2,   1  },
	{ 4,   2  },
	{ 6,   3  },
	{ 8,   4  },
	{ 12,  5  },
	{ 16,  6  },
	{ 24,  7  },
	{ 32,  8  },
	{ 48,  9  },
};

static struct timer_list mute_on_timer;
static struct timer_list fm_input_mute_on_timer;

/* for checking wm8994 rev */
int wm8994_rev = 0;

/* Current audio path */
int wm8994_curr_path = 0;

/* Previout audio path */
int wm8994_prev_path = 0;

/* for idle current */
int wm8994_idle_mode = IDLE_POWER_DOWN_MODE_OFF; 

/* wm8994 CODEC_LDO_ON control */
static int wm8994_power = 0;

static int wm8994_probe_done = 0;

/* Codec Tuning Mode */
int wm8994_codec_tuning_mode = 0;

/* For WM8994 debugging */
int wm8994_debug_on = 0;

/* For another device calls the wm8994 i2c */
struct snd_soc_codec *wm8994_codec;

static const char *audio_path[] = { "Playback Path", 
				"Voice Call Path", 
				"Voice Memo Path", 
				"FM Radio Path",
				"MIC Path", 
				"Codec Tuning",
				"Down Gain",
				NULL};

/*===================================
Function
	wm8994_data_call_hp_switch

	To remove TDMA noise on data call connecting, 
	switch EAR_SEL gpio to call mode and compensate hp gain about 2dB
	
====================================*/
void wm8994_data_call_hp_switch(void)
{
	u16 val;

	DEBUG_LOG("wm8994_curr_path  : %d", wm8994_curr_path);

	if(wm8994_curr_path == MM_AUDIO_PLAYBACK_HP)
	{
		gpio_set_value(GPIO_EAR_SEL, 1);

		//HPOUT_VOL
		val = wm8994_read(wm8994_codec, WM8994_LEFT_OUTPUT_VOLUME); 	
		val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1L_VOL_MASK);
		val |= (WM8994_HPOUT1_VU_MASK | TUNE_PLAYBACK_HP_VOL_DATACALL);
		wm8994_write(wm8994_codec, WM8994_LEFT_OUTPUT_VOLUME ,val);
		val = wm8994_read(wm8994_codec, WM8994_RIGHT_OUTPUT_VOLUME); 	
		val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1R_VOL_MASK);
		val |= (WM8994_HPOUT1_VU_MASK | TUNE_PLAYBACK_HP_VOL_DATACALL);
		wm8994_write(wm8994_codec, WM8994_RIGHT_OUTPUT_VOLUME ,val);
	}
}

EXPORT_SYMBOL(wm8994_data_call_hp_switch);

/*===================================
Function
	wm8994_FM_headset_disable
	wm8994_FM_headset_enable

	For decreasing FM radio white noise, 
	when FM volume set to 0 excute wm8994_FM_headset_disable
	and wm8994_FM_headset_enable called if the volume set to not "0" 
	
====================================*/
int fm_enable_val;

void wm8994_FM_headset_disable(void)
{
	u16 val;
	//HPOUT_VOL
	val = wm8994_read(wm8994_codec, WM8994_POWER_MANAGEMENT_1); 	
	val &= ~(WM8994_HPOUT1L_ENA_MASK | WM8994_HPOUT1R_ENA_MASK);
	wm8994_write(wm8994_codec, WM8994_POWER_MANAGEMENT_1 ,val);
}

void wm8994_FM_headset_enable(void)
{
	u16 val;
	if((wm8994_curr_path == MM_AUDIO_FMRADIO_HP) || (wm8994_curr_path == MM_AUDIO_FMRADIO_SPK_HP))
	{
		//HPOUT_VOL
		val = wm8994_read(wm8994_codec, WM8994_POWER_MANAGEMENT_1); 	
		val &= ~(WM8994_HPOUT1L_ENA_MASK | WM8994_HPOUT1R_ENA_MASK);
		val |= (WM8994_HPOUT1L_ENA | WM8994_HPOUT1R_ENA);
		wm8994_write(wm8994_codec, WM8994_POWER_MANAGEMENT_1 ,val);
	}
}

u16 new_fm_hp_vol = TUNE_FM_HP_VOL;

void wm8994_FM_headset_gain_set(u8 fm_vol)
{
	u16 val;
	new_fm_hp_vol = TUNE_FM_HP_VOL - (0x0F - (u16)fm_vol);

	DEBUG_LOG("Volume : %d", fm_vol);
	//HPOUT_VOL
	val = wm8994_read(wm8994_codec, WM8994_LEFT_OUTPUT_VOLUME); 	
	val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1L_VOL_MASK);
	val |= (WM8994_HPOUT1_VU_MASK | new_fm_hp_vol);
	wm8994_write(wm8994_codec, WM8994_LEFT_OUTPUT_VOLUME ,val);
	val = wm8994_read(wm8994_codec, WM8994_RIGHT_OUTPUT_VOLUME); 	
	val &= ~(WM8994_HPOUT1_VU_MASK | WM8994_HPOUT1R_VOL_MASK);
	val |= (WM8994_HPOUT1_VU_MASK | new_fm_hp_vol);
	wm8994_write(wm8994_codec, WM8994_RIGHT_OUTPUT_VOLUME ,val);
}

EXPORT_SYMBOL(wm8994_FM_headset_disable);
EXPORT_SYMBOL(wm8994_FM_headset_enable);
EXPORT_SYMBOL(wm8994_FM_headset_gain_set);

static unsigned int wm8994_read_hw(struct snd_soc_codec *codec, u16 reg)
{
	struct i2c_msg xfer[2];
	u16 data;
	int ret;
	struct i2c_client *i2c = codec->control_data;

	data = ((reg & 0xff00) >> 8) | ((reg & 0xff) << 8);

	//printk("wm8994 read from i2c  reg %x, data %x\n",reg,data);
	
	/* Write register */
	xfer[0].addr = i2c->addr;
	xfer[0].flags = 0;
	xfer[0].len = 2;  //1
	//xfer[0].buf = &reg;
	xfer[0].buf = (void *)&data;

	/* Read data */
	xfer[1].addr = i2c->addr;
	xfer[1].flags = I2C_M_RD;
	xfer[1].len = 2;
	xfer[1].buf = (u8 *)&data;
	ret = i2c_transfer(i2c->adapter, xfer, 2);
	if (ret != 2) {
		dev_err(codec->dev, "Failed to read 0x%x: %d\n", reg, ret);
		return 0;
	}
	//printk("wm8994 after read  from i2c  reg %x, data %x\n",reg,data);

	return (data >> 8) | ((data & 0xff) << 8);
}

/*
 * write to the WM8994 register space
 */
int wm8994_write(struct snd_soc_codec *codec, unsigned int reg,
	unsigned int value)
{
	u8 data[4];
	int ret;

#ifdef CONFIG_SND_VOODOO
        value = voodoo_hook_wm8994_write(codec, reg, value);
#endif
	//BUG_ON(reg > WM8993_MAX_REGISTER);
	if(wm8994_debug_on) // For Debugging wm8994 setting
		printk(KERN_ERR "[WM8994] wm8994_write : reg (0x%x) - val (0x%x)\n", reg, value);
	/* data is
	 *   D15..D9 WM8993 register offset
	 *   D8...D0 register data
	 */
	data[0] = (reg & 0xff00 ) >> 8;
	data[1] = reg & 0x00ff;
	data[2] = value >> 8;
	data[3] = value & 0x00ff;
	ret = codec->hw_write(codec->control_data, data, 4);

	if (ret == 4)
		return 0;
	if (ret < 0)
		return ret;
	return -EIO;

}

inline unsigned int wm8994_read(struct snd_soc_codec *codec,
				       unsigned int reg)
{
		return wm8994_read_hw(codec, reg);
}

static const DECLARE_TLV_DB_SCALE(dac_tlv, -12750, 50, 1);



static int wm899x_outpga_put_volsw_vu(struct snd_kcontrol *kcontrol,
        struct snd_ctl_elem_value *ucontrol)
{

        int ret;
        u16 val;

        struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
        struct soc_mixer_control *mc =
                (struct soc_mixer_control *)kcontrol->private_value;
        int reg = mc->reg;

        ret = snd_soc_put_volsw_2r(kcontrol, ucontrol);
        if (ret < 0)
                return ret;

        /* now hit the volume update bits (always bit 8) */
        val = wm8994_read(codec, reg);
	//printk("going to update volume with %xval %x reg\n",val,reg);
        return wm8994_write(codec, reg, val | 0x0100);
}

static int wm899x_inpga_put_volsw_vu(struct snd_kcontrol *kcontrol,
        struct snd_ctl_elem_value *ucontrol)
{
        struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
        struct soc_mixer_control *mc =
                (struct soc_mixer_control *)kcontrol->private_value;
        int reg = mc->reg;
        int ret;
        u16 val;

        ret = snd_soc_put_volsw(kcontrol, ucontrol);
        if (ret < 0)
                return ret;

	/* now hit the volume update bits (always bit 8) */
        val = wm8994_read(codec, reg);
        //printk("going to update volume with %x val %x reg\n",val,reg);
        return wm8994_write(codec, reg, val | 0x0100);

}



static int wm8994_get_mic_path(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
#if 0
	ucontrol->value.integer.value[0] = wm8994_mic_path;
	return 0;
#endif
	return 0;
}

static int wm8994_set_mic_path(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
#if 0
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	struct wm8994_priv *wm8994 = codec->private_data;
	
	wm8994->prev_rec_path = wm8994->rec_path;	
	if (ucontrol->value.integer.value[0] == 0) { // MAIN MIC
		//printk("routing to Main Mic \n");
                wm8994->rec_path = MAIN;

	} else if (ucontrol->value.integer.value[0] == 1) { // SUB MIC
		//printk("routing to Headphone Mic \n");
                wm8994->rec_path = SUB;
	} else {
		//printk("invalid Mic value");
		return -1;
	}
//	wm8994->universal_mic_path[wm8994->rec_path ](codec);
//	wm8994->universal_mic_path[wm8994->rec_path ](codec,wm8994->prev_rec_path);
#endif
	return 0;
}

static int wm8994_get_codec_tuning(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)

{
    ucontrol->value.integer.value[0] = wm8994_codec_tuning_mode;

	return 0;
}

static int wm8994_set_codec_tuning(struct snd_kcontrol *kcontrol, struct snd_ctl_elem_value *ucontrol)

{
    struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

    printk("%s(%d) - (%d)\n", __func__, __LINE__, ucontrol->value.integer.value[0]);

    if(ucontrol->value.integer.value[0] == 0)
    {
        wm8994_codec_tuning_mode = 0;
    }else if(ucontrol->value.integer.value[0] == 1){
        wm8994_codec_tuning_mode = 1;
    }
	
}

static const char *playback_path[]  = { "Off", "RCV",  "SPK", "HP", "BT", "SPK_HP", "RING_SPK_HP" };
static const char *voicecall_path[] = { "Off", "RCV",  "SPK", "HP", "BT", "SPK_HP"};
static const char *voicememo_path[] = { "Off", "MAIN", "SUB", "EAR", "BT", "SPK_HP"};
static const char *fmradio_path[] = { "Off", "RCV", "SPK", "EAR", "BT", "SPK_HP"};
static const char *mic_path[] = { "Main Mic", "Hands Free Mic","Mic Off" };
static const char *codec_tuning_control[] = { "Off", "On" };
static const char *idle_mode[] = { "Off", "ON" };

static int set_registers(struct snd_soc_codec *codec, int mode)
{
        struct wm8994_priv *wm8994 = codec->private_data;
	int val = 0;
	printk("%s() \n", __func__);	

	wm8994_prev_path = wm8994_curr_path;
			

	/* just path change */
	int ret = 0;

	if (wm8994_power == 0) {
		audio_power(1); /* Board Specific function */
		wm8994_power = 1;
	}

	wm8994->cur_path = mode;
	wm8994->power_state = CODEC_ON;

	if (wm8994->codec_state & CALL_ACTIVE) {
		wm8994->codec_state &= ~(CALL_ACTIVE);

		val = wm8994_read(codec, WM8994_CLOCKING_1);
		val &= ~(WM8994_DSP_FS2CLK_ENA_MASK | WM8994_SYSCLK_SRC_MASK);
		wm8994_write(codec, WM8994_CLOCKING_1, val);
	}


	if ( (wm8994_idle_mode == IDLE_POWER_DOWN_MODE_OFF) && (wm8994_power == 1) ) {
		ret = wm8994_change_path(codec, mode, wm8994_prev_path);
			
		if (ret > 0) {
			printk("Codec Change Path (0x%02x ==> 0x%02x)\n", wm8994_prev_path, mode);

			wm8994_idle_mode = IDLE_POWER_DOWN_MODE_OFF; // IDLE Mode reset (off)
			wm8994_curr_path = mode;
			return 0;
		}
	}
			
	wm8994_disable_path(codec, wm8994_prev_path );
			
	wm8994_enable_path(codec, mode );

	wm8994_idle_mode = IDLE_POWER_DOWN_MODE_OFF; // IDLE Mode reset (off)
	
	set_mic_bias(codec, mode);
//	if(((mode & 0xF0) == MM_AUDIO_PLAYBACK)/* || ((mode & 0xF0) == MM_AUDIO_VOICECALL)*/) // temporary condition
//		wm8994_set_default_gain(codec, mode);

	wm8994_curr_path = mode;
	//wm8994->universal_playback_path[wm8994->cur_path] (codec);
	return 0;
}

static int wm8994_get_path(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	int i = 0;
	
	while(audio_path[i] != NULL) {
		if(!strcmp(audio_path[i], kcontrol->id.name) && ((wm8994_curr_path >> 4) == i)) {
			ucontrol->value.integer.value[0] = wm8994_curr_path & 0xf;
			break;
		}
		i++;
	}
	return 0;
}

static int wm8994_set_path(struct snd_kcontrol *kcontrol,
        struct snd_ctl_elem_value *ucontrol)
{
        struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int path_num = ucontrol->value.integer.value[0];
	int val;
	int i = 0, new_path;
	/* To remove audio noise by low clk when path changed, set the dvfs level to maximum */
#ifdef CONFIG_CPU_FREQ
	set_dvfs_perf_level();
#endif
	
	while(audio_path[i] != NULL) {
		new_path = (i << 4) | ucontrol->value.integer.value[0];

		if(!strcmp(audio_path[i], kcontrol->id.name)) 
		{
			printk("%s(): new_path : 0x%02x, curr_path : 0x%02x\n", __func__, new_path, wm8994_curr_path);
			if((wm8994_curr_path != new_path) || ((new_path & 0xf0) == MM_AUDIO_VOICEMEMO) || (new_path == MM_AUDIO_PLAYBACK_RCV))
			{
				printk("%s(): 0x%02x\n", __func__, new_path);
				set_registers(codec, new_path);
				break;
			}
			else
				printk("%s(): Audio path not changed\n", __func__);
		}
		i++;
	}
	return 1;
}

static int wm8994_get_idle_mode(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = wm8994_idle_mode;
	return 0;
}

static int wm8994_set_idle_mode(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	printk(" idle_mode_value : %d\n", (int)ucontrol->value.integer.value[0]);

	if(wm8994_power == 0 && wm8994_idle_mode == IDLE_POWER_DOWN_MODE_ON)
	{
		printk("audio power up\n");
		set_registers(codec, wm8994_curr_path);
		return 1;
	}

	if ( (wm8994_curr_path & 0xf0) == MM_AUDIO_PLAYBACK || (wm8994_curr_path & 0xf0) == MM_AUDIO_VOICEMEMO)
	{
		if (ucontrol->value.integer.value[0] == 0 && wm8994_idle_mode == IDLE_POWER_DOWN_MODE_ON) { // Off
			idle_mode_enable(codec, wm8994_curr_path);
		} else if (ucontrol->value.integer.value[0] == 1 && wm8994_idle_mode == IDLE_POWER_DOWN_MODE_OFF) { // On
			idle_mode_disable(codec, wm8994_curr_path);
		} else {
			printk("invalid idle mode value\n");
			return -1;
		}

		wm8994_idle_mode = ucontrol->value.integer.value[0];
	} else 
		printk("only Playback mode or recording mode!\n");

	return 1;
}



static int wm8994_get_voice_path(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{

	int i = 0;

	while(audio_path[i] != NULL) {
		if(!strcmp(audio_path[i], kcontrol->id.name) && ((wm8994_curr_path >> 4) == i)) {
			ucontrol->value.integer.value[0] = wm8994_curr_path & 0xf;
			break;
		}
		
		i++;
	}
	return 0;
}

static int wm8994_set_voice_path(struct snd_kcontrol *kcontrol,
        struct snd_ctl_elem_value *ucontrol)
{
        struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	 struct soc_enum *mc =
                (struct soc_enum *)kcontrol->private_value;
	int i=ucontrol->value.integer.value[0];	
//	printk("ucontrol is %d\n", ucontrol->value.integer.value[0]);

//	printk("controls are %s %s %s\n",mc->texts[0],mc->texts[1],mc->texts[2]);
	
	if(strcmp( mc->texts[i],voicecall_path[i]) )
	{		
		printk("Unknown path %s\n", mc->texts[i] );		
	}
	switch(i)
	{
		case 0:
		printk("Switching off output path\n");
		break;
		case 1:
		printk("routing voice path to %s \n", mc->texts[i] );
		wm8994_voice_rcv_path (codec);
		break;
		case 2:
		printk("routing  voice path to  %s \n", mc->texts[i] );
		 wm8994_voice_spkr_path(codec);
		break;
		case 3:
		printk("routing voice path to %s \n", mc->texts[i] );
		 wm8994_voice_ear_path(codec);
		break;
		case 4:
		printk("routing voice path to %s \n", mc->texts[i] );
		 wm8994_voice_BT_path(codec);
		break;
		default:
		printk("routing voice path to %s NOT IMPLEMENTED \n", mc->texts[i] );
		break;
	}
	return 0;
}

#define  SOC_WM899X_OUTPGA_DOUBLE_R_TLV(xname, reg_left, reg_right, xshift, xmax, xinvert, tlv_array) \
{	.iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname),\
	.access = SNDRV_CTL_ELEM_ACCESS_TLV_READ |\
		 SNDRV_CTL_ELEM_ACCESS_READWRITE,\
	.tlv.p = (tlv_array), \
	.info = snd_soc_info_volsw_2r, \
	.get = snd_soc_get_volsw_2r, .put = wm899x_outpga_put_volsw_vu, \
	.private_value = (unsigned long)&(struct soc_mixer_control) \
		{.reg = reg_left, .rreg = reg_right, .shift = xshift, \
		.max = xmax, .invert = xinvert} }


#define SOC_WM899X_OUTPGA_SINGLE_R_TLV(xname, reg, shift, max, invert,\
         tlv_array) {\
        .iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = (xname), \
        .access = SNDRV_CTL_ELEM_ACCESS_TLV_READ |\
                  SNDRV_CTL_ELEM_ACCESS_READWRITE,\
        .tlv.p = (tlv_array), \
        .info = snd_soc_info_volsw, \
        .get = snd_soc_get_volsw, .put = wm899x_inpga_put_volsw_vu, \
        .private_value = SOC_SINGLE_VALUE(reg, shift, max, invert) }

//these are all factors of .01dB
static const DECLARE_TLV_DB_SCALE(digital_tlv, -7162, 37, 1);
//static const DECLARE_TLV_DB_SCALE(digital_tlv_spkr, 5700, 37, 1);

static const DECLARE_TLV_DB_LINEAR(digital_tlv_spkr,-5700,600);
static const DECLARE_TLV_DB_LINEAR(digital_tlv_rcv,-5700,600);
static const DECLARE_TLV_DB_LINEAR(digital_tlv_headphone,-5700,600);
static const DECLARE_TLV_DB_LINEAR(digital_tlv_mic,-7162,7162);

static const struct soc_enum path_control_enum[] = {
        SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(playback_path),playback_path),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(voicecall_path),voicecall_path),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(voicememo_path),voicememo_path),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(fmradio_path),fmradio_path),
	 SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(mic_path),mic_path),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(codec_tuning_control),codec_tuning_control),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(idle_mode),idle_mode),
};

static const struct snd_kcontrol_new wm8994_snd_controls[] = {
	SOC_WM899X_OUTPGA_DOUBLE_R_TLV("Playback Volume",  WM8994_LEFT_OPGA_VOLUME ,
		  		 WM8994_RIGHT_OPGA_VOLUME , 0, 0x3F, 0, digital_tlv_rcv),

	SOC_WM899X_OUTPGA_DOUBLE_R_TLV("Playback Spkr Volume", WM8994_SPEAKER_VOLUME_LEFT  ,
				 WM8994_SPEAKER_VOLUME_RIGHT  , 1,0x3F, 0, digital_tlv_spkr),
	SOC_WM899X_OUTPGA_DOUBLE_R_TLV("Playback Headset Volume",WM8994_LEFT_OUTPUT_VOLUME  ,
				 WM8994_RIGHT_OUTPUT_VOLUME   , 1,0x3F, 0, digital_tlv_headphone),
	SOC_WM899X_OUTPGA_SINGLE_R_TLV("Capture Volume",  WM8994_AIF1_ADC1_LEFT_VOLUME ,
                                         0, 0xEF, 0, digital_tlv_mic),

	 /* Path Control */
    SOC_ENUM_EXT("Playback Path", path_control_enum[0], wm8994_get_path, wm8994_set_path),
    SOC_ENUM_EXT("Voice Call Path", path_control_enum[1], wm8994_get_path, wm8994_set_path), // wm8994_get_voice_path, wm8994_set_voice_path),
    SOC_ENUM_EXT("Voice Memo Path", path_control_enum[2], wm8994_get_path, wm8994_set_path),
    SOC_ENUM_EXT("FM Radio Path", path_control_enum[3], wm8994_get_path, wm8994_set_path),
    SOC_ENUM_EXT("MIC Path", path_control_enum[4], wm8994_get_mic_path, wm8994_set_mic_path),
    SOC_ENUM_EXT("Codec Tuning", path_control_enum[5], wm8994_get_codec_tuning, wm8994_set_codec_tuning),
    SOC_ENUM_EXT("Idle Mode", path_control_enum[6], wm8994_get_idle_mode, wm8994_set_idle_mode),
} ;//snd_ctrls

/* Add non-DAPM controls */
static int wm8994_add_controls(struct snd_soc_codec *codec)
{
	int err, i;

	for (i = 0; i < ARRAY_SIZE(wm8994_snd_controls); i++) {
		err = snd_ctl_add(codec->card,
				  snd_soc_cnew(&wm8994_snd_controls[i],
					       codec, NULL));
		if (err < 0)
			return err;
	}
	return 0;
}
static const struct snd_soc_dapm_widget wm8994_dapm_widgets[] = {
};

static const struct snd_soc_dapm_route audio_map[] = {
};

static int wm8994_add_widgets(struct snd_soc_codec *codec)
{
	snd_soc_dapm_new_controls(codec, wm8994_dapm_widgets,
				  ARRAY_SIZE(wm8994_dapm_widgets));

	snd_soc_dapm_add_routes(codec, audio_map, ARRAY_SIZE(audio_map));

	snd_soc_dapm_new_widgets(codec);
	return 0;
}

static int configure_clock(struct snd_soc_codec *codec)
{
	struct wm8994_priv *wm8994 = codec->private_data;
	unsigned int reg;

/* Wolfson recommend - do not disable AIF1 clock 
	reg = wm8994_read(codec,WM8994_AIF1_CLOCKING_1);
        reg &= ~WM8994_AIF1CLK_ENA ; //disable the clock
	reg &= ~WM8994_AIF1CLK_SRC_MASK; 
        wm8994_write(codec, WM8994_AIF1_CLOCKING_1, reg);
*/
	/* This should be done on init() for bypass paths */
	switch (wm8994->sysclk_source) {
	case WM8994_SYSCLK_MCLK:
		dev_dbg(codec->dev, "Using %dHz MCLK\n", wm8994->mclk_rate);

		reg = wm8994_read(codec,WM8994_AIF1_CLOCKING_1);
		reg &= ~WM8994_AIF1CLK_ENA ; //disable the clock 
		wm8994_write(codec, WM8994_AIF1_CLOCKING_1, reg);
		reg = wm8994_read(codec,WM8994_AIF1_CLOCKING_1);
		reg &= 0x07; //clear clksrc bits ..now it is for MCLK
		if(wm8994->mclk_rate > 13500000)
		{
			reg |= WM8994_AIF1CLK_DIV ; 
			wm8994->sysclk_rate = wm8994->mclk_rate / 2;
		}
		else
		{
			reg &= ~WM8994_AIF1CLK_DIV;
			wm8994->sysclk_rate = wm8994->mclk_rate;
		}
		reg |= WM8994_AIF1CLK_ENA ; //enable the clocks
		wm8994_write(codec, WM8994_AIF1_CLOCKING_1, reg);

	//Enable clocks to the Audio core and sysclk of wm8994
		reg = wm8994_read(codec, WM8994_CLOCKING_1 );		
		reg &= ~(WM8994_SYSCLK_SRC_MASK | WM8994_DSP_FSINTCLK_ENA_MASK|WM8994_DSP_FS1CLK_ENA_MASK  );
		reg |= (WM8994_DSP_FS1CLK_ENA | WM8994_DSP_FSINTCLK_ENA  );
		wm8994_write(codec,WM8994_CLOCKING_1 ,reg);
	break;

	case WM8994_SYSCLK_FLL:
		switch(wm8994->fs )
		{
			case  8000: 
				wm8994_write(codec, WM8994_FLL1_CONTROL_1, 0x0005);
				wm8994_write(codec, WM8994_FLL1_CONTROL_2, 0x2F00);
				wm8994_write(codec, WM8994_FLL1_CONTROL_3, 0x3126);
//				wm8994_write(codec, WM8994_FLL1_CONTROL_5, 0x0C88);
   	    			wm8994_write(codec, WM8994_FLL1_CONTROL_4, 0x0100);
				break;
			case  11025: 
				wm8994_write(codec, WM8994_FLL1_CONTROL_1, 0x0005);
   	    			wm8994_write(codec, WM8994_FLL1_CONTROL_2, 0x1F00);
   	    			wm8994_write(codec, WM8994_FLL1_CONTROL_3, 0x86C2);
   	    	 		wm8994_write(codec, WM8994_FLL1_CONTROL_5, 0x0C80);
   	    			wm8994_write(codec, WM8994_FLL1_CONTROL_4, 0x00e0);
				break;
			case  12000: 
				wm8994_write(codec, WM8994_FLL1_CONTROL_1, 0x0005);
   	    			wm8994_write(codec, WM8994_FLL1_CONTROL_2, 0x1F00);
   	    			wm8994_write(codec, WM8994_FLL1_CONTROL_3, 0x3126);
   	    	 		wm8994_write(codec, WM8994_FLL1_CONTROL_5, 0x0C80);
   	    			wm8994_write(codec, WM8994_FLL1_CONTROL_4, 0x0100);
				break;
			case  16000: 
				wm8994_write(codec, WM8994_FLL1_CONTROL_1, 0x0005);
   	    			wm8994_write(codec, WM8994_FLL1_CONTROL_2, 0x1900);
   	    			wm8994_write(codec, WM8994_FLL1_CONTROL_3, 0xE23E);
   	    	 		wm8994_write(codec, WM8994_FLL1_CONTROL_5, 0x0C80);
   	    			wm8994_write(codec, WM8994_FLL1_CONTROL_4, 0x0100);
				break;
			case  22050: 
				wm8994_write(codec, WM8994_FLL1_CONTROL_1, 0x0005);
   	    			wm8994_write(codec, WM8994_FLL1_CONTROL_2, 0x0F00);
   	    			wm8994_write(codec, WM8994_FLL1_CONTROL_3, 0x86C2);
   	    	 		wm8994_write(codec, WM8994_FLL1_CONTROL_5, 0x0C80);
   	    			wm8994_write(codec, WM8994_FLL1_CONTROL_4, 0x00E0);
				break;
			case  24000: 
				wm8994_write(codec, WM8994_FLL1_CONTROL_1, 0x0005);
   	    			wm8994_write(codec, WM8994_FLL1_CONTROL_2, 0x0F00);
   	    			wm8994_write(codec, WM8994_FLL1_CONTROL_3, 0x3126);
   	    	 		wm8994_write(codec, WM8994_FLL1_CONTROL_5, 0x0C80);
   	    			wm8994_write(codec, WM8994_FLL1_CONTROL_4, 0x0100);
				break;
			case  32000: 
				wm8994_write(codec, WM8994_FLL1_CONTROL_1, 0x0005);
   	    			wm8994_write(codec, WM8994_FLL1_CONTROL_2, 0x0C00);
   	    			wm8994_write(codec, WM8994_FLL1_CONTROL_3, 0xE23E);
   	    	 		wm8994_write(codec, WM8994_FLL1_CONTROL_5, 0x0C80);
   	    			wm8994_write(codec, WM8994_FLL1_CONTROL_4, 0x0100);
				break;
			case  44100: 
				wm8994_write(codec, WM8994_FLL1_CONTROL_1, 0x0005);		/* Fractional Mode , FLL1 Enable */
				wm8994_write(codec, WM8994_FLL1_CONTROL_2, 0x0700);		/* 11.2896Mhz = FVCO / 8 		 */
				wm8994_write(codec, WM8994_FLL1_CONTROL_3, 0x86C2);
   	    			wm8994_write(codec, WM8994_FLL1_CONTROL_4, 0x00E0);
   	    	 		wm8994_write(codec, WM8994_FLL1_CONTROL_5, 0x0C80);
				break;
			case  48000: 
				wm8994_write(codec, WM8994_FLL1_CONTROL_1, 0x0005);
   	    			wm8994_write(codec, WM8994_FLL1_CONTROL_2, 0x0700);
   	    			wm8994_write(codec, WM8994_FLL1_CONTROL_3, 0x3126);
   	    	 		wm8994_write(codec, WM8994_FLL1_CONTROL_5, 0x0C88);
   	    			wm8994_write(codec, WM8994_FLL1_CONTROL_4, 0x0100);
				break;
		default:
			printk("Unsupported Frequency\n");
			break;
		}

		reg = wm8994_read(codec, WM8994_AIF1_CLOCKING_1);
		reg |= WM8994_AIF1CLK_ENA ; //enable the clocks
		reg |= WM8994_AIF1CLK_SRC_FLL1;//selecting FLL1
		wm8994_write(codec, WM8994_AIF1_CLOCKING_1, reg);
		//Enable clocks to the Audio core and sysclk of wm8994	
		reg = wm8994_read(codec, WM8994_CLOCKING_1);
    	reg &= ~(WM8994_SYSCLK_SRC_MASK | WM8994_DSP_FSINTCLK_ENA_MASK|WM8994_DSP_FS1CLK_ENA_MASK);
    	reg |= (WM8994_DSP_FS1CLK_ENA | WM8994_DSP_FSINTCLK_ENA);
    	wm8994_write(codec,WM8994_CLOCKING_1, reg);		
		break;

	default:
		dev_err(codec->dev, "System clock not configured\n");
		return -EINVAL;
	}

	dev_dbg(codec->dev, "CLK_SYS is %dHz\n", wm8994->sysclk_rate);

	return 0;
}

static int wm8994_set_bias_level(struct snd_soc_codec *codec,
				 enum snd_soc_bias_level level)
{
	if (wm8994_power == 0) {
		audio_power(1); /* Board Specific function */
		wm8994_power = 1;
	}

	switch (level) {
	case SND_SOC_BIAS_ON:
	case SND_SOC_BIAS_PREPARE:
		/* VMID=2*40k */
		snd_soc_update_bits(codec, WM8994_POWER_MANAGEMENT_1,
				    WM8994_VMID_SEL_MASK, 0x2);
		snd_soc_update_bits(codec, WM8994_POWER_MANAGEMENT_2,
				    WM8994_TSHUT_ENA, WM8994_TSHUT_ENA);
		break;

	case SND_SOC_BIAS_STANDBY:
		if (codec->bias_level == SND_SOC_BIAS_OFF) {
			/* Bring up VMID with fast soft start */
			snd_soc_update_bits(codec, WM8994_ANTIPOP_2,
					    WM8994_STARTUP_BIAS_ENA |
					    WM8994_VMID_BUF_ENA |
					    WM8994_VMID_RAMP_MASK |
					    WM8994_BIAS_SRC,
					    WM8994_STARTUP_BIAS_ENA |
					    WM8994_VMID_BUF_ENA |
					    WM8994_VMID_RAMP_MASK |
				    WM8994_BIAS_SRC);
#if 0
			/* If either line output is single ended we
			 * need the VMID buffer */
			if (!wm8993->pdata.lineout1_diff ||
			    !wm8993->pdata.lineout2_diff)
				snd_soc_update_bits(codec, WM8994_ANTIPOP1,
						 WM8994_LINEOUT_VMID_BUF_ENA,
						 WM8994_LINEOUT_VMID_BUF_ENA);
#endif //if 0 shaju
			/* VMID=2*40k */
			snd_soc_update_bits(codec, WM8994_POWER_MANAGEMENT_1,
					    WM8994_VMID_SEL_MASK |
					    WM8994_BIAS_ENA,
					    WM8994_BIAS_ENA | 0x2);
			msleep(32);

			/* Switch to normal bias */
			snd_soc_update_bits(codec, WM8994_ANTIPOP_2,
					    WM8994_BIAS_SRC |
					    WM8994_STARTUP_BIAS_ENA, 0);
		}

		/* VMID=2*240k */
		snd_soc_update_bits(codec, WM8994_POWER_MANAGEMENT_1,
				    WM8994_VMID_SEL_MASK, 0x4);

		snd_soc_update_bits(codec, WM8994_POWER_MANAGEMENT_2,
				    WM8994_TSHUT_ENA, 0);
		break;

	case SND_SOC_BIAS_OFF:
		snd_soc_update_bits(codec, WM8994_ANTIPOP_1,
				    WM8994_LINEOUT_VMID_BUF_ENA, 0);

      		snd_soc_update_bits(codec, WM8994_POWER_MANAGEMENT_1,
				    WM8994_VMID_SEL_MASK,
				    0);
		break;
	}

	codec->bias_level = level;

	return 0;
}

static int wm8994_set_sysclk(struct snd_soc_dai *codec_dai,
			     int clk_id, unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct wm8994_priv *wm8994 = codec->private_data;

	switch (clk_id) {
	case WM8994_SYSCLK_MCLK:
		wm8994->mclk_rate = freq;
		wm8994->sysclk_source = clk_id;
		break;
	case WM8994_SYSCLK_FLL:
		wm8994->sysclk_rate = freq;
		wm8994->sysclk_source = clk_id;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int wm8994_set_dai_fmt(struct snd_soc_dai *dai,
			      unsigned int fmt)
{
	struct snd_soc_codec *codec = dai->codec;
	struct wm8994_priv *wm8994 = codec->private_data;

	if (wm8994_power == 0) {
		audio_power(1); /* Board Specific function */
		wm8994_power = 1;
	}

	unsigned int aif1 = wm8994_read(codec,WM8994_AIF1_CONTROL_1);
	unsigned int aif2 = wm8994_read(codec,WM8994_AIF1_MASTER_SLAVE );

	aif1 &= ~(WM8994_AIF1_LRCLK_INV |WM8994_AIF1_BCLK_INV |
		   WM8994_AIF1_WL_MASK | WM8994_AIF1_FMT_MASK);

	aif2 &= ~( WM8994_AIF1_LRCLK_FRC_MASK| WM8994_AIF1_CLK_FRC| WM8994_AIF1_MSTR ) ; //to enable LRCLK and bclk in master mode

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		wm8994->master = 0;
		break;
	case SND_SOC_DAIFMT_CBS_CFM:
		aif2 |= (WM8994_AIF1_MSTR|WM8994_AIF1_LRCLK_FRC);
		wm8994->master = 1;
		break;
	case SND_SOC_DAIFMT_CBM_CFS:
		aif2 |= (WM8994_AIF1_MSTR|WM8994_AIF1_CLK_FRC) ;
		wm8994->master = 1;
		break;
	case SND_SOC_DAIFMT_CBM_CFM:
		aif2 |= (WM8994_AIF1_MSTR|WM8994_AIF1_CLK_FRC| WM8994_AIF1_LRCLK_FRC);
		//aif2 |= (WM8994_AIF1_MSTR);	// CONFIG_SND_WM8994_MASTER_MODE
		wm8994->master = 1;
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_DSP_B:
		aif1 |=WM8994_AIF1_LRCLK_INV;
	case SND_SOC_DAIFMT_DSP_A:
		aif1 |= 0x18;
		break;
	case SND_SOC_DAIFMT_I2S:
		aif1 |= 0x10;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		aif1 |= 0x8;
		break;
	default:
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_DSP_A:
	case SND_SOC_DAIFMT_DSP_B:
		/* frame inversion not valid for DSP modes */
		switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
		case SND_SOC_DAIFMT_NB_NF:
			break;
		case SND_SOC_DAIFMT_IB_NF:
			aif1 |=  WM8994_AIF1_BCLK_INV;
			break;
		default:
			return -EINVAL;
		}
		break;

	case SND_SOC_DAIFMT_I2S:
	case SND_SOC_DAIFMT_RIGHT_J:
	case SND_SOC_DAIFMT_LEFT_J:
		switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
		case SND_SOC_DAIFMT_NB_NF:
			break;
		case SND_SOC_DAIFMT_IB_IF:
			aif1 |= WM8994_AIF1_BCLK_INV |WM8994_AIF1_LRCLK_INV;
			break;
		case SND_SOC_DAIFMT_IB_NF:
			aif1 |=    WM8994_AIF1_BCLK_INV;
			break;
		case SND_SOC_DAIFMT_NB_IF:
			aif1 |= WM8994_AIF1_LRCLK_INV;
			break;
		default:
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	aif1 |= 0x4000;	// CONFIG_SND_WM8994_MASTER_MODE
	wm8994_write(codec,WM8994_AIF1_CONTROL_1, aif1);
	
	// wm8994_write(codec,WM8994_AIF1DAC_LRCLK, aif2);
	wm8994_write(codec,WM8994_AIF1_MASTER_SLAVE, aif2);	// CONFIG_SND_WM8994_MASTER_MODE

	wm8994_write( codec,WM8994_AIF1_CONTROL_2, 0x4000);	// CONFIG_SND_WM8994_MASTER_MODE

	return 0;
}

static int wm8994_hw_params(struct snd_pcm_substream *substream,
			    struct snd_pcm_hw_params *params,
			    struct snd_soc_dai *dai)
{
	struct snd_soc_codec *codec = dai->codec;
	struct wm8994_priv *wm8994 = codec->private_data;
	int ret, i, best, best_val, cur_val;
	unsigned int clocking1, clocking3, aif1, aif4,aif5;
	unsigned int sample_rate = params_rate(params);

	if (wm8994_power == 0) {
		audio_power(1); /* Board Specific function */
		wm8994_power = 1;
	}

	clocking1 = wm8994_read(codec,WM8994_AIF1_BCLK);
	clocking1 &= ~ WM8994_AIF1_BCLK_DIV_MASK ;


	clocking3 = wm8994_read(codec, WM8994_AIF1_RATE);
	clocking3 &= ~(WM8994_AIF1_SR_MASK | WM8994_AIF1CLK_RATE_MASK);

	aif1 = wm8994_read(codec, WM8994_AIF1_CONTROL_1);
	aif1 &= ~WM8994_AIF1_WL_MASK;

	/* What BCLK do we need? */
	if(sample_rate == 8000)
		sample_rate = 44100;

	wm8994->fs = sample_rate;
	wm8994->bclk = 2 * wm8994->fs;
	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		wm8994->bclk *= 16;
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		wm8994->bclk *= 20;
		aif1 |= (0x1<< WM8994_AIF1_WL_SHIFT);
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
		wm8994->bclk *= 24;
		aif1 |= (0x2 << WM8994_AIF1_WL_SHIFT);
		break;
	case SNDRV_PCM_FORMAT_S32_LE:
		wm8994->bclk *= 32;
		aif1 |= (0x3 << WM8994_AIF1_WL_SHIFT);
		break;
	default:
		return -EINVAL;
	}

	dev_dbg(codec->dev, "Target BCLK is %dHz\n", wm8994->bclk);

	ret = configure_clock(codec);
	if (ret != 0)
		return ret;
	/* Select nearest CLK_SYS_RATE */
	best = 0;
	best_val = abs((wm8994->sysclk_rate / clk_sys_rates[0].ratio) - wm8994->fs);
	for (i = 1; i < ARRAY_SIZE(clk_sys_rates); i++) {
		cur_val = abs((wm8994->sysclk_rate / clk_sys_rates[i].ratio) - wm8994->fs);
		if (cur_val < best_val) {
			best = i;
			best_val = cur_val;
		}
	}
	dev_dbg(codec->dev, "Selected CLK_SYS_RATIO of %d\n",	clk_sys_rates[best].ratio);

	printk("Selected CLK_SYS_RATIO of %d\n",	clk_sys_rates[best].ratio);
	clocking3 |= (clk_sys_rates[best].clk_sys_rate << WM8994_AIF1CLK_RATE_SHIFT);

	/* SAMPLE_RATE */
	best = 0;
	best_val = abs(wm8994->fs - sample_rates[0].rate);
	for (i = 1; i < ARRAY_SIZE(sample_rates); i++) {
		/* Closest match */
		cur_val = abs(wm8994->fs - sample_rates[i].rate);
		if (cur_val < best_val) {
			best = i;
			best_val = cur_val;
		}
	}
	dev_dbg(codec->dev, "Selected SAMPLE_RATE of %dHz\n", sample_rates[best].rate);

	printk("Selected SAMPLE_RATE of %dHz\n", sample_rates[best].rate);
	clocking3 |= (sample_rates[best].sample_rate << WM8994_AIF1_SR_SHIFT);

	/* BCLK_DIV */
	wm8994->bclk = wm8994->sysclk_rate;
	printk("Target BCLK is %dHz\n", wm8994->bclk);
	
	/* Don't need to divide BCLK, BCLK = 11.2896Mhz */
	clocking1 |= 0x0 << WM8994_AIF1_BCLK_DIV_SHIFT;

	/* LRCLK is a simple fraction of BCLK */
	dev_dbg(codec->dev, "LRCLK_RATE is %d\n", wm8994->bclk / wm8994->fs);
	
	printk("LRCLK_RATE is %d\n", wm8994->bclk / wm8994->fs);
	aif4 = wm8994->bclk / wm8994->fs;
	aif5 = wm8994->bclk / wm8994->fs;

//TODO...we need to set proper BCLK & LRCLK to support different frequency songs..In modifying 
//BCLK & LRCLK , its giving noisy and improper frequency sound..this has to be checked
#ifndef CONFIG_SND_WM8994_MASTER_MODE 
	//wm8994_write(codec,WM8994_AIF1_BCLK, clocking1);
	//wm8994_write(codec,WM8994_AIF1ADC_LRCLK, aif4);
	//wm8994_write(codec,WM8994_AIF1DAC_LRCLK, aif5);
#endif	

//	wm8994_write(codec,WM8994_AIF1_BCLK, clocking1);
	wm8994_write(codec,WM8994_AIF1_RATE, clocking3);
	wm8994_write(codec, WM8994_AIF1_CONTROL_1, aif1);
//	wm8994_write(codec,WM8994_AIF1ADC_LRCLK, aif4);
//	wm8994_write(codec,WM8994_AIF1DAC_LRCLK, aif5);

	return 0;
}

static int wm8994_digital_mute(struct snd_soc_dai *codec_dai, int mute)
{
#ifdef WARN_NOT_IMPLEMENTED
	printk("%s function needs to be implemented\n");
#endif //ifdef warn_not_implemented	

//shaju this needs to be implemented
	#if 0
	unsigned int reg;
	struct snd_soc_codec *codec = codec_dai->codec;
	reg = wm8993_read(codec, WM8993_DAC_CTRL);

	if (mute)
		reg |= WM8993_DAC_MUTE;
	else
		reg &= ~WM8993_DAC_MUTE;

	wm8993_write(codec, WM8993_DAC_CTRL, reg);
	#endif
	return 0;
}

#define WM8994_RATES SNDRV_PCM_RATE_8000_96000
#define WM8994_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE |\
			SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)

static struct snd_soc_dai_ops wm8994_dai_ops = {
		 .set_sysclk = wm8994_set_sysclk,
		 .set_fmt = wm8994_set_dai_fmt,
		 .hw_params = wm8994_hw_params,
		 .digital_mute = wm8994_digital_mute,
};

struct snd_soc_dai wm8994_dai = {
	
		.name = "WM8994 PAIFRX",
		.playback = {
			.stream_name = "Playback",
			.channels_min = 1,
			.channels_max = 6,
			.rates = WM8994_RATES,
			.formats = WM8994_FORMATS,
		},
		.capture = {
			.stream_name = "Capture",
			.channels_min = 1,
			.channels_max = 2,
			.rates = WM8994_RATES,
			.formats = WM8994_FORMATS,
		},
		.ops = &wm8994_dai_ops,

};
EXPORT_SYMBOL_GPL(wm8994_dai);


/*
 * initialise the WM8994 driver
 * register the mixer and dsp interfaces with the kernel
 */
static int wm8994_init(struct snd_soc_device *socdev)
{
	struct snd_soc_codec *codec = socdev->card->codec;
	int ret = 0;

	codec->dev = socdev->dev;
	codec->name = "WM8994";
	codec->owner = THIS_MODULE;
	codec->read = wm8994_read;
	codec->write = wm8994_write;
	codec->set_bias_level = wm8994_set_bias_level;
	codec->dai = &wm8994_dai;
	codec->num_dai = 1;//ARRAY_SIZE(wm8994_dai);

	wm8994_micbias_control(codec, 0, 0);

	set_registers(codec, MM_AUDIO_PLAYBACK_SPK);

	/* register pcms */
	ret = snd_soc_new_pcms(socdev, SNDRV_DEFAULT_IDX1,
			       SNDRV_DEFAULT_STR1);
	if (ret < 0) {
		printk(KERN_ERR "wm8994: failed to create pcms\n");
		goto pcm_err;
	}

	wm8994_add_controls(codec);
	wm8994_add_widgets(codec);

	ret = snd_soc_init_card(socdev);
	if (ret < 0) {
		printk(KERN_ERR "wm8994: failed to register card\n");
		goto card_err;
	}
	return ret;

card_err:
	snd_soc_free_pcms(socdev);
	snd_soc_dapm_free(socdev);
pcm_err:
	kfree(codec->reg_cache);
	return ret;
}

/* If the i2c layer weren't so broken, we could pass this kind of data
   around */
static struct snd_soc_device *wm8994_socdev;

#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)

/*
 * WM8994 2 wire address is determined by GPIO5
 * state during powerup.
 *    low  = 0x1a
 *    high = 0x1b
 */

static int wm8994_i2c_probe(struct i2c_client *i2c,
			    const struct i2c_device_id *id)
{
	struct snd_soc_device *socdev = wm8994_socdev;
	struct snd_soc_codec *codec = socdev->card->codec;
	int ret;

	#ifdef PM_DEBUG
	pm_codec = codec;
	#endif

	i2c_set_clientdata(i2c, codec);
	codec->control_data = i2c;

	ret = wm8994_init(socdev);
#ifdef CONFIG_SND_VOODOO
	voodoo_hook_wm8994_pcm_probe(codec);
#endif
	if (ret < 0)
		dev_err(&i2c->dev, "failed to initialise WM8994\n");
	return ret;
}

static int wm8994_i2c_remove(struct i2c_client *client)
{
	struct snd_soc_codec *codec = i2c_get_clientdata(client);
	kfree(codec->reg_cache);
	return 0;
}

static const struct i2c_device_id wm8994_i2c_id[] = {
	{ "wm8994", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, wm8994_i2c_id);

static struct i2c_driver wm8994_i2c_driver = {
	.driver = {
		.name = "WM8994 I2C Codec",
		.owner = THIS_MODULE,
	},
	.probe =    wm8994_i2c_probe,
	.remove =   wm8994_i2c_remove,
	.id_table = wm8994_i2c_id,
};

static int wm8994_add_i2c_device(struct platform_device *pdev,
				 const struct wm8994_setup_data *setup)
{
	struct i2c_board_info info;
	struct i2c_adapter *adapter;
	struct i2c_client *client;
	int ret;

	ret = i2c_add_driver(&wm8994_i2c_driver);
	if (ret != 0) {
		dev_err(&pdev->dev, "can't add i2c driver\n");
		return ret;
	}

	memset(&info, 0, sizeof(struct i2c_board_info));
	info.addr = setup->i2c_address;
	strlcpy(info.type, "wm8994", I2C_NAME_SIZE);

	adapter = i2c_get_adapter(setup->i2c_bus);
	if (!adapter) {
		dev_err(&pdev->dev, "can't get i2c adapter %d\n",
			setup->i2c_bus);
		goto err_driver;
	}

	client = i2c_new_device(adapter, &info);
	i2c_put_adapter(adapter);
	if (!client) {
		dev_err(&pdev->dev, "can't add i2c device at 0x%x\n",
			(unsigned int)info.addr);
		goto err_driver;
	}

	return 0;

err_driver:
	i2c_del_driver(&wm8994_i2c_driver);
	return -ENODEV;
}
#endif

static int fm_out_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", fm_enable_val);
}

static int fm_out_enable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
	unsigned long value = simple_strtoul(buf, NULL, 10);

//	printk("%s : value = %d\n", value);
	fm_enable_val = (int) value;
	
	if(value)
		wm8994_FM_headset_enable();
	else
		wm8994_FM_headset_disable();
	
	return len;
}

static DEVICE_ATTR(fm_out_enable, 0644,
                   fm_out_enable_show,
                   fm_out_enable_store);


#define	MUTE_ON_CHECK_TIME	get_jiffies_64() + (HZ/100)// 1000ms / 100 = 10ms

int sw_mute_val;
wm8994_mute_out_path sw_mute_out_path;

/* ============================================
 * wm8994_set_sw_mute(int mute_time)
 * 	Disable codec internal PGA power about (mute_time * 10) msec
 	if mute_time is larger than 300 (3 sec) do not unmute the PGA after muting time
    ============================================ */

int wm8994_set_sw_mute(int mute_time, wm8994_mute_out_path mute_out_path)
{
	u16 val;
	u16 mute_val=0x0, unmute_val=0x0;
	
	if(!wm8994_probe_done)
		return 0;
	
	DEBUG_LOG("wm8994_set_sw_mute = %d msec path %d", (mute_time*10), mute_out_path); 

	sw_mute_out_path = mute_out_path;
	switch(mute_out_path)
	{
		case mute_none:
			mute_val = WM8994_HPOUT1L_ENA_MASK | WM8994_HPOUT1R_ENA_MASK | WM8994_SPKOUTL_ENA_MASK;
			unmute_val = WM8994_HPOUT1L_ENA | WM8994_HPOUT1R_ENA | WM8994_SPKOUTL_ENA_MASK;
			break;

		case hp_out_mute:
			mute_val = WM8994_HPOUT1L_ENA_MASK | WM8994_HPOUT1R_ENA_MASK;
			unmute_val = WM8994_HPOUT1L_ENA | WM8994_HPOUT1R_ENA;
			break;

		case spk_out_mute:
			mute_val = WM8994_SPKOUTL_ENA_MASK;
			unmute_val = WM8994_SPKOUTL_ENA_MASK;
			break;

		case hp_spk_out_mute:
			mute_val = WM8994_HPOUT1L_ENA_MASK | WM8994_HPOUT1R_ENA_MASK | WM8994_SPKOUTL_ENA_MASK;
			unmute_val = WM8994_HPOUT1L_ENA | WM8994_HPOUT1R_ENA | WM8994_SPKOUTL_ENA_MASK;
			break;

		default:
			break;
	}

	if(!mute_time)
	{
		del_timer(&mute_on_timer);
		val = wm8994_read(wm8994_codec, WM8994_POWER_MANAGEMENT_1); 	
		val &= ~(mute_val);
		val |= (unmute_val);
		wm8994_write(wm8994_codec, WM8994_POWER_MANAGEMENT_1 ,val);
	}
	else if(mute_time > 300) // permanently mute
	{
		del_timer(&mute_on_timer);
		val = wm8994_read(wm8994_codec, WM8994_POWER_MANAGEMENT_1); 	
		val &= ~(mute_val);
		wm8994_write(wm8994_codec, WM8994_POWER_MANAGEMENT_1 ,val);
	}
	else	
	{
		del_timer(&mute_on_timer);
		val = wm8994_read(wm8994_codec, WM8994_POWER_MANAGEMENT_1); 	
		val &= ~(mute_val);
		wm8994_write(wm8994_codec, WM8994_POWER_MANAGEMENT_1 ,val);

		mute_on_timer.expires = MUTE_ON_CHECK_TIME * mute_time;
		add_timer(&mute_on_timer);
	}
	
	sw_mute_val = mute_time;
	return 0;
}

EXPORT_SYMBOL(wm8994_set_sw_mute);

static void mute_on_timer_handler(unsigned long arg)
{
	u16 val;
	u16 unmute_val=0x0;
	
	DEBUG_LOG("");

	switch(sw_mute_out_path)
	{
		case mute_none:
			unmute_val = WM8994_HPOUT1L_ENA | WM8994_HPOUT1R_ENA | WM8994_SPKOUTL_ENA_MASK;
			break;

		case hp_out_mute:
			unmute_val = WM8994_HPOUT1L_ENA | WM8994_HPOUT1R_ENA;
			break;

		case spk_out_mute:
			unmute_val = WM8994_SPKOUTL_ENA_MASK;
			break;

		case hp_spk_out_mute:
			unmute_val = WM8994_HPOUT1L_ENA | WM8994_HPOUT1R_ENA | WM8994_SPKOUTL_ENA_MASK;
			break;

		default:
			break;
	}


	del_timer(&mute_on_timer);
	val = wm8994_read(wm8994_codec, WM8994_POWER_MANAGEMENT_1); 
	val &= ~(WM8994_HPOUT1L_ENA_MASK | WM8994_HPOUT1R_ENA_MASK | WM8994_SPKOUTL_ENA_MASK);
	val |= unmute_val;
	wm8994_write(wm8994_codec, WM8994_POWER_MANAGEMENT_1 ,val);
	sw_mute_val = 0;
	sw_mute_out_path = mute_none;
}

static int sw_mute_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", sw_mute_val);
}

static int sw_mute_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
	unsigned long value = simple_strtoul(buf, NULL, 10);

//	printk("%s : value = %d\n", __func__, value);
	sw_mute_val = (int) value;
	
	wm8994_set_sw_mute(value, hp_spk_out_mute);
	
	return len;
}

static DEVICE_ATTR(sw_mute, 0644,
                   sw_mute_show,
                   sw_mute_store);


/* ============================================
 * wm8994_set_fm_input_mute(int mute_time)
 
 * 	Mute FM input path about (mute_time * 10) msec
 	if mute_time is larger than 300 (3 sec) do not unmute the FM input path after muting time
    ============================================ */
int fm_input_mute_val;

int wm8994_set_fm_input_mute(int mute_time)
{
	u16 val;
	
	if(!wm8994_probe_done)
		return 0;
	
	DEBUG_LOG("wm8994_set_fm_input_mute = %d msec", (mute_time*10)); 

	if(!mute_time)
	{
		del_timer(&fm_input_mute_on_timer);
		// IN2L unmute
		val = wm8994_read(wm8994_codec, WM8994_INPUT_MIXER_3 );
		val &= ~(WM8994_IN2L_TO_MIXINL_MASK);
		val |= WM8994_IN2L_TO_MIXINL;
		wm8994_write(wm8994_codec, WM8994_INPUT_MIXER_3, val);
		// IN2R unmute
		val = wm8994_read(wm8994_codec, WM8994_INPUT_MIXER_4 );
		val &= ~(WM8994_IN2R_TO_MIXINR_MASK);
		val |= WM8994_IN2R_TO_MIXINR;
		wm8994_write(wm8994_codec, WM8994_INPUT_MIXER_4, val);
	}
	else if(mute_time > 300) // permanently mute
	{
		del_timer(&fm_input_mute_on_timer);
		// IN2L unmute
		val = wm8994_read(wm8994_codec, WM8994_INPUT_MIXER_3 );
		val &= ~(WM8994_IN2L_TO_MIXINL_MASK);
		wm8994_write(wm8994_codec, WM8994_INPUT_MIXER_3, val);
		// IN2R unmute
		val = wm8994_read(wm8994_codec, WM8994_INPUT_MIXER_4 );
		val &= ~(WM8994_IN2R_TO_MIXINR_MASK);
		wm8994_write(wm8994_codec, WM8994_INPUT_MIXER_4, val);
	}
	else	
	{
		del_timer(&fm_input_mute_on_timer);
		// IN2L unmute
		val = wm8994_read(wm8994_codec, WM8994_INPUT_MIXER_3 );
		val &= ~(WM8994_IN2L_TO_MIXINL_MASK);
		wm8994_write(wm8994_codec, WM8994_INPUT_MIXER_3, val);
		// IN2R unmute
		val = wm8994_read(wm8994_codec, WM8994_INPUT_MIXER_4 );
		val &= ~(WM8994_IN2R_TO_MIXINR_MASK);
		wm8994_write(wm8994_codec, WM8994_INPUT_MIXER_4, val);

		fm_input_mute_on_timer.expires = MUTE_ON_CHECK_TIME * mute_time;
		add_timer(&fm_input_mute_on_timer);
	}

	return 0;
}

EXPORT_SYMBOL(wm8994_set_fm_input_mute);

static void fm_input_mute_on_timer_handler(unsigned long arg)
{
	u16 val;
	
	DEBUG_LOG("");

	del_timer(&fm_input_mute_on_timer);

	// IN2L unmute
	val = wm8994_read(wm8994_codec, WM8994_INPUT_MIXER_3 );
	val &= ~(WM8994_IN2L_TO_MIXINL_MASK);
	val |= WM8994_IN2L_TO_MIXINL;
	wm8994_write(wm8994_codec, WM8994_INPUT_MIXER_3, val);
	// IN2R unmute
	val = wm8994_read(wm8994_codec, WM8994_INPUT_MIXER_4 );
	val &= ~(WM8994_IN2R_TO_MIXINR_MASK);
	val |= WM8994_IN2R_TO_MIXINR;
	wm8994_write(wm8994_codec, WM8994_INPUT_MIXER_4, val);
}

static int fm_input_mute_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", fm_input_mute_val);
}

static int fm_input_mute_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
	unsigned long value = simple_strtoul(buf, NULL, 10);

//	printk("%s : value = %d\n", __func__, value);
	fm_input_mute_val = (int) value;
	
	wm8994_set_fm_input_mute(value);
	
	return len;
}

static DEVICE_ATTR(fm_input_mute, 0644,
                   fm_input_mute_show,
                   fm_input_mute_store);

static int debug_on_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", wm8994_debug_on);
}

static int debug_on_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
	unsigned long value = simple_strtoul(buf, NULL, 10);

	wm8994_debug_on = (int) value;

	return len;
}

static DEVICE_ATTR(debug_on, 0644,
                   debug_on_show,
                   debug_on_store);

static int wm8994_probe(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct wm8994_setup_data *setup;
	struct snd_soc_codec *codec;
	struct wm8994_priv *wm8994;
	int ret = 0;

	pr_info("WM8994 Audio Codec %s\n", WM8998_VERSION);

	/* Board Specific function */
	audio_init();
	audio_power(1);
	wm8994_power = 1;
	
	setup = socdev->codec_data;
	codec = kzalloc(sizeof(struct snd_soc_codec), GFP_KERNEL);
	if (codec == NULL)
		return -ENOMEM;

	wm8994_codec = codec;
	wm8994 = kzalloc(sizeof(struct wm8994_priv), GFP_KERNEL);
	if (wm8994 == NULL) {
		kfree(codec);
		return -ENOMEM;
	}

	codec->private_data = wm8994;
	socdev->card->codec = codec;
	mutex_init(&codec->mutex);
	INIT_LIST_HEAD(&codec->dapm_widgets);
	INIT_LIST_HEAD(&codec->dapm_paths);
	wm8994_socdev = socdev;

#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
	if (setup->i2c_address) {
		codec->hw_write = (hw_write_t)i2c_master_send;
		ret = wm8994_add_i2c_device(pdev, setup);
	}
#else
		/* Add other interfaces here */
#endif

	device_create_file(&(pdev->dev), &dev_attr_fm_out_enable);
	device_create_file(&(pdev->dev), &dev_attr_sw_mute);
	device_create_file(&(pdev->dev), &dev_attr_debug_on);
	device_create_file(&(pdev->dev), &dev_attr_fm_input_mute);

	wm8994_rev = wm8994_read(codec, 0x100);
	printk("[WM8994] %s : Chip Rev 0x%x\n", __func__, wm8994_rev);
	if(wm8994_rev >= 3)
		wm8994_write(codec, 0x620, 0x0000);

	/* For decreasing pop noise seq - from Wolfson */
	/* wm8994_write(codec, 0x110, 0x8100) is same as excuting below sequences
	
		wm8994_write(codec,WM8994_DC_SERVO_1,0x000F);
		msleep(20);
		wm8994_write(codec, WM8994_ANALOGUE_HP_1, 0x00EE );
	*/
	wm8994_write(codec,  0x3000, 0x0054);
	wm8994_write(codec,  0x3001, 0x000F);
	wm8994_write(codec, 0x3002, 0x0300);
	wm8994_write(codec,  0x3003, 0x0006); // delay 16ms

	wm8994_write(codec,  0x3004, 0x0060);
	wm8994_write(codec, 0x3005, 0x00EE);
	wm8994_write(codec, 0x3006, 0x0700);
	wm8994_write(codec,  0x3007, 0x0006);
	 
	wm8994_write(codec, 0x3008, 0x0420);
	wm8994_write(codec, 0x3009, 0x0000);
	wm8994_write(codec, 0x300A, 0x0006);
	wm8994_write(codec, 0x300B, 0x010D); // wm seq end

	init_timer(&mute_on_timer);
	mute_on_timer.function = mute_on_timer_handler;

	init_timer(&fm_input_mute_on_timer);
	fm_input_mute_on_timer.function = fm_input_mute_on_timer_handler;

	wm8994_probe_done = 1;
	return ret;
}

/* power down chip */
static int wm8994_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->card->codec;
	snd_soc_free_pcms(socdev);
	snd_soc_dapm_free(socdev);
#if defined(CONFIG_I2C) || defined(CONFIG_I2C_MODULE)
	i2c_unregister_device(codec->control_data);
	i2c_del_driver(&wm8994_i2c_driver);
#endif
	kfree(codec->private_data);
	kfree(codec);

	return 0;
}

#ifdef CONFIG_PM
static int wm8994_suspend(struct platform_device *pdev,pm_message_t msg )
{

	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
    struct snd_soc_codec *codec = socdev->card->codec;
#if 0
    struct wm8994_priv *wm8994 = codec->private_data;

    wm8994_disable_playback_path(codec, wm8994_curr_path);
    wm8994_disable_rec_path(codec, wm8994_curr_path);
#else
//	printk("%s() STARTED\n",__func__);
	if( (wm8994_curr_path & 0xF0) != MM_AUDIO_VOICECALL 
/*		&& (wm8994_curr_path & 0xF0) != MM_AUDIO_VOICEMEMO */
		&& (wm8994_curr_path & 0xF0) != MM_AUDIO_FMRADIO )
	{
	    wm8994_set_bias_level(codec, SND_SOC_BIAS_OFF);

		// control interface : Generic Shut-Down
#if 1
/*
		wm8994_write(codec, 0x110, 0x812A);
		msleep(500);
*/
		wm8994_write(codec,0x00 ,0x0000);
		wm8994_write(codec,0x01 ,0x0005);
#else
		//wm8994_micbias_control(codec,0,0); 

		// control interface : Generic Shut-Down
		wm8994_write(codec, 0x1F, 0x0001);
		wm8994_write(codec, 0x1E, 0x0033);
		wm8994_write(codec, 0x60, 0x0000);
		wm8994_write(codec, 0x54, 0x0000);
		wm8994_write(codec, 0x4C, 0x0000);
		wm8994_write(codec, 0x39, 0x0017);
		wm8994_write(codec, 0x01, 0x0000);
		wm8994_write(codec, 0x38, 0x0003);
		wm8994_write(codec, 0x37, 0x0000);
		wm8994_write(codec, 0x38, 0x0000);
		wm8994_write(codec, 0x39, 0x0000);		

		wm8994_write(codec, 0x02, 0x6000);
		wm8994_write(codec, 0x03, 0x0000);		
		wm8994_write(codec, 0x04, 0x0000);		
		wm8994_write(codec, 0x05, 0x0000);		
		wm8994_write(codec, 0x06, 0x0000);		

		wm8994_write(codec, 0x00, 0x0000);		
#endif
		audio_power(0);
		wm8994_power = 0;
		wm8994_idle_mode = IDLE_POWER_DOWN_MODE_ON;
	}
//	printk("%s() END\n",__func__);
#endif
	return 0;
}

static int wm8994_resume(struct platform_device *pdev)
{
	int i;
        struct snd_soc_device *socdev = platform_get_drvdata(pdev);
        struct snd_soc_codec *codec = socdev->card->codec;
	msleep(100);
	if (wm8994_power == 0) {
		audio_power(1); /* Board Specific function */
		wm8994_power = 1;
		wm8994_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
		wm8994_enable_path(codec, wm8994_curr_path);
	}
// Audio codec LDO enabled on path setting sequence because pop noise
/*	
	wm8994_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
	wm8994_set_bias_level(codec, codec->suspend_bias_level);
*/
#if 0
    printk("%s() STARTED\n",__func__);
    if( wm8994_power == 0 )
    {
		audio_power(1);
		wm8994_power = 1;
		mdelay(100);
    }
	
    wm8994_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
	wm8994_enable_path(codec, wm8994_curr_path);
	printk("%s() END\n",__func__);
#endif
	return 0;
}
#endif

struct snd_soc_codec_device soc_codec_dev_wm8994 = {
	.probe = 	wm8994_probe,
	.remove = 	wm8994_remove,
#ifdef CONFIG_PM
	.suspend= wm8994_suspend,
	.resume= wm8994_resume,
#endif
};
EXPORT_SYMBOL_GPL(soc_codec_dev_wm8994);

static int __init wm8994_modinit(void)
{
	return snd_soc_register_dai(&wm8994_dai);
}
module_init(wm8994_modinit);

static void __exit wm8994_exit(void)
{
	snd_soc_unregister_dai(&wm8994_dai);
}
module_exit(wm8994_exit);

MODULE_DESCRIPTION("ASoC WM8994 driver");
MODULE_AUTHOR("Shaju Abraham shaju.abraham@samsung.com");
MODULE_LICENSE("GPL");

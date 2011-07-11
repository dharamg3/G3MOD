/*
 * Driver for S5K4BA (UXGA camera) from Samsung Electronics
 * 
 * 1/4" 2.0Mp CMOS Image Sensor SoC with an Embedded Image Processor
 *
 * Copyright (C) 2009, Jinsung Yang <jsgood.yang@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-i2c-drv.h>
#include <media/s5k4ca_platform.h>

#ifdef CONFIG_VIDEO_SAMSUNG_V4L2
#include <linux/videodev2_samsung.h>
#endif
#include <plat/power_clk_gating.h>
#include "s5k4ca.h"

#ifdef S5K4CA_SKIP_GREEN_FRAMES
#include "s5k4ca_init.h"
#endif

//giridhar: disabling printks
#define S5K4CA_DRIVER_NAME	"S5K4CA"

/* Default resolution & pixelformat. plz ref s5k4ca_platform.h */
#define DEFAULT_RES		XGA	/* Index of resoultion */
#define DEFAUT_FPS_INDEX	S5K4CA_15FPS
#define DEFAULT_FMT		V4L2_PIX_FMT_VYUY	/* YUV422 */

//#define CONFIG_LOAD_FILE	//For techwin tunning binary
//#define VIEW_FUNCTION_CALL

#ifndef VIEW_FUNCTION_CALL
#define printk(...)
#endif

/*
 * Specification
 * Parallel : ITU-R. 656/601 YUV422, RGB565, RGB888 (Up to VGA), RAW10 
 * Serial : MIPI CSI2 (single lane) YUV422, RGB565, RGB888 (Up to VGA), RAW10
 * Resolution : 1024 (H) x 768 (V)
 * Image control : Brightness, Contrast, Saturation, Sharpness, Glamour
 * Effect : Mono, Negative, Sepia, Aqua, Sketch
 * FPS : 15fps @full resolution, 30fps @VGA, 24fps @720p
 */

/* Camera functional setting values configured by user concept */
struct s5k4ca_userset {
	signed int exposure_bias;	/* V4L2_CID_EXPOSURE */
	unsigned int ae_lock;
	unsigned int awb_lock;
	unsigned int auto_wb;	/* V4L2_CID_AUTO_WHITE_BALANCE */
	unsigned int manual_wb;	/* V4L2_CID_WHITE_BALANCE_PRESET */
	unsigned int wb_temp;	/* V4L2_CID_WHITE_BALANCE_TEMPERATURE */
	unsigned int effect;	/* Color FX (AKA Color tone) */
	unsigned int contrast;	/* V4L2_CID_CONTRAST */
	unsigned int saturation;	/* V4L2_CID_SATURATION */
	unsigned int sharpness;		/* V4L2_CID_SHARPNESS */
	unsigned int glamour;
};

struct s5k4ca_state {
	struct s5k4ca_platform_data *pdata;
	struct v4l2_subdev sd;
	struct v4l2_pix_format pix;
	struct v4l2_fract timeperframe;
	struct s5k4ca_userset userset;
	int freq;	/* MCLK in KHz */
	int is_mipi;
	int isize;
	int ver;
	int fps;
};

static int preview_in_init = 0;
static int preview_in_init_af = 0;
static int previous_scene_mode = -1;
static int previous_WB_mode = 0;
static unsigned short lux_value = 0;
//static int first_init_end = 0;
static int macroType = 0;


#ifdef CONFIG_LOAD_FILE
	static int s5k4ca_regs_table_write(struct i2c_client *client, char *name);
#endif

static inline struct s5k4ca_state *to_state(struct v4l2_subdev *sd)
{
#ifdef VIEW_FUNCTION_CALL
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif
	return container_of(sd, struct s5k4ca_state, sd);
}

#ifdef S5K4CA_SKIP_GREEN_FRAMES
static inline int s5k4ca_direct_write(struct i2c_client *client,
		unsigned long packet)
{
	unsigned char buf[4];

	struct i2c_msg msg = {
		.addr	= client->addr,
		.flags	= 0,
		.buf	= buf,
		.len	= 4,
	};

	*(unsigned long *)buf = packet;
	
	return i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;
}
#endif

/* Arun C: inlining the function
 * need to check if there are any side effects and
 * camera performance improvement
 */
static inline int s5k4ca_sensor_read(struct i2c_client *client,
		unsigned short subaddr, unsigned short *data)
{
	int ret;
	unsigned char buf[2];
	struct i2c_msg msg = { client->addr, 0, 2, buf };
#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif
	buf[0] = (subaddr >> 8);
	buf[1] = (subaddr & 0xFF);

	ret = i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;
	/* if (ret == -EIO) */
	/* Arun C: make the error unlikely */
	if (unlikely(ret == -EIO))
		goto error;

	msg.flags = I2C_M_RD;
	
	ret = i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;
	/* if (ret == -EIO) */
	/* Arun C: make the error unlikely */
	if (unlikely(ret == -EIO))
		goto error;

	*data = ((buf[0] << 8) | buf[1]);

error:
	return ret;
}



/* Arun C: inlining the function
 * need to check if there are any side effects and
 * camera performance improvement
 */
static inline int s5k4ca_sensor_write(struct i2c_client *client, 
		unsigned short subaddr, unsigned short val)
{
	//int ret;

#if 0
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned char buf[4];
	struct i2c_msg msg = { client->addr, 0, 4, buf };
#else
	unsigned char buf[4];
	struct i2c_msg msg = { client->addr, 0, 4, buf };
#endif

	buf[0] = (subaddr >> 8);
	buf[1] = (subaddr & 0xFF);
	buf[2] = (val >> 8);
	buf[3] = (val & 0xFF);

	return i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;
}

//giridhar: copied this function from c110
#if 1
static int s5k4ba_write_regs(struct v4l2_subdev *sd, s5k4ca_short_t regs[], 
				int size, char *name)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int i, err;
//zzangdol
#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif

	
#ifdef CONFIG_LOAD_FILE
	s5k4ca_regs_table_write(client, name);
#else
	for (i = 0; i < size; i++) {
		//giridhar: commented first and wrote second
		//err = s5k4ba_i2c_write(sd, &regs[i], sizeof(regs[i]));
		err = s5k4ca_sensor_write(client, regs[i].subaddr, regs[i].value);
		/* if (err < 0) */
		/* Arun C: stop writing and return err value on error 
		 * Also make error check unlikely
		 * */ 
		if (unlikely(err < 0)) {
			v4l_info(client, "%s: register set failed\n", \
			__func__);
			return err;
		}
	}
#endif
	return 0;	/* FIXME */
}

#endif

#define S5K4CA_USE_BURSTMODE
#ifdef S5K4CA_USE_BURSTMODE	
unsigned char s5k4ca_buf_for_burstmode[2500];
static inline int s5k4ca_sensor_burst_write(struct i2c_client *client, 
		s5k4ca_short_t table[] ,int size)
{
	int i = 0;
	int idx = 0;
	int err = -EINVAL;
	
	struct i2c_msg msg = { client->addr, 0, 0, s5k4ca_buf_for_burstmode };
	
	for (i = 0; i < size; i++) 
	{
		switch( table[i].subaddr )
		{
                     case 0xFCFC :
                     case 0x0028 :
                     case 0x002A :
                         // Set Address
                         idx=0;
                         err = s5k4ca_sensor_write(client, table[i].subaddr, \
     					table[i].value);
                         break;
                     case 0x0F12 :
                         // make and fill buffer for burst mode write
                         if(idx ==0) 
         		 {
         		 	s5k4ca_buf_for_burstmode[idx++] = 0x0F;
         		 	s5k4ca_buf_for_burstmode[idx++] = 0x12;
         		 }
                         s5k4ca_buf_for_burstmode[idx++] = table[i].value>> 8;
     		         s5k4ca_buf_for_burstmode[idx++] = table[i].value & 0xFF;
     		         //write in burstmode	
                         if(table[i+1].subaddr != 0x0F12)
                         {
                         	//printk("%s: write in burstmode. length :%d\n",__func__,idx);
         			msg.len = idx;
         			err = i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;
         			idx=0;
         			//msleep(1);
                         }
                         break;
                     case 0xFFFF :
                    	 break;				
		}
	}
	if (unlikely(err < 0))
	{
		v4l_info(client, "%s: register set failed\n", \
			__func__);
		return err;
	}
	return err;
}
#endif /*S5K4CA_USE_BURSTMODE*/

static void s5k4ca_sensor_get_id(struct i2c_client *client)
{
	unsigned short id = 0;
#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif
	
	s5k4ca_sensor_write(client, 0x002C, 0x7000);
	s5k4ca_sensor_write(client, 0x002E, 0x01FA);
	s5k4ca_sensor_read(client, 0x0F12, &id);

	printk("Sensor ID(0x%04x) is %s!\n", id, (id == 0x4CA4) ? "Valid" : "Invalid"); 
}



static const char *s5k4ca_querymenu_wb_preset[] = {
	"WB Auto", "WB sunny", "WB cloudy", "WB Tungsten", "WB Fluorescent", NULL
};

static const char *s5k4ca_querymenu_effect_mode[] = {
	"Effect Sepia", "Effect Aqua", "Effect Monochrome",
	"Effect Negative", "Effect Sketch", NULL
};

static const char *s5k4ca_querymenu_ev_bias_mode[] = {
	"-3EV",	"-2,1/2EV", "-2EV", "-1,1/2EV",
	"-1EV", "-1/2EV", "0", "1/2EV",
	"1EV", "1,1/2EV", "2EV", "2,1/2EV",
	"3EV", NULL
};

static struct v4l2_queryctrl s5k4ca_controls[] = {
	{
		/*
		 * For now, we just support in preset type
		 * to be close to generic WB system,
		 * we define color temp range for each preset
		 */
		.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "White balance in kelvin",
		.minimum = 0,
		.maximum = 10000,
		.step = 1,
		.default_value = 0,	/* FIXME */
	},
	{
		.id = V4L2_CID_WHITE_BALANCE_PRESET,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "White balance preset",
		.minimum = 0,
		.maximum = ARRAY_SIZE(s5k4ca_querymenu_wb_preset) - 2,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_AUTO_WHITE_BALANCE,
		.type = V4L2_CTRL_TYPE_BOOLEAN,
		.name = "Auto white balance",
		.minimum = 0,
		.maximum = 1,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_EXPOSURE,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Exposure bias",
		.minimum = 0,
		.maximum = ARRAY_SIZE(s5k4ca_querymenu_ev_bias_mode) - 2,
		.step = 1,
		.default_value = (ARRAY_SIZE(s5k4ca_querymenu_ev_bias_mode) - 2) / 2,	/* 0 EV */
	},
	{
		.id = V4L2_CID_COLORFX,
		.type = V4L2_CTRL_TYPE_MENU,
		.name = "Image Effect",
		.minimum = 0,
		.maximum = ARRAY_SIZE(s5k4ca_querymenu_effect_mode) - 2,
		.step = 1,
		.default_value = 0,
	},
	{
		.id = V4L2_CID_CONTRAST,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Contrast",
		.minimum = 0,
		.maximum = 4,
		.step = 1,
		.default_value = 2,
	},
	{
		.id = V4L2_CID_SATURATION,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Saturation",
		.minimum = 0,
		.maximum = 4,
		.step = 1,
		.default_value = 2,
	},
	{
		.id = V4L2_CID_SHARPNESS,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Sharpness",
		.minimum = 0,
		.maximum = 4,
		.step = 1,
		.default_value = 2,
	},
};

const char **s5k4ca_ctrl_get_menu(u32 id)
{
#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif

	switch (id) {
	case V4L2_CID_WHITE_BALANCE_PRESET:
		return s5k4ca_querymenu_wb_preset;

	case V4L2_CID_COLORFX:
		return s5k4ca_querymenu_effect_mode;

	case V4L2_CID_EXPOSURE:
		return s5k4ca_querymenu_ev_bias_mode;

	default:
		return v4l2_ctrl_get_menu(id);
	}
}

static inline struct v4l2_queryctrl const *s5k4ca_find_qctrl(int id)
{
	int i;
#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif

	for (i = 0; i < ARRAY_SIZE(s5k4ca_controls); i++)
		if (s5k4ca_controls[i].id == id)
			return &s5k4ca_controls[i];

	return NULL;
}

static int s5k4ca_queryctrl(struct v4l2_subdev *sd, struct v4l2_queryctrl *qc)
{
	int i;
#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif

	for (i = 0; i < ARRAY_SIZE(s5k4ca_controls); i++) {
		if (s5k4ca_controls[i].id == qc->id) {
			memcpy(qc, &s5k4ca_controls[i], \
				sizeof(struct v4l2_queryctrl));
			return 0;
		}
	}

	return -EINVAL;
}

static int s5k4ca_querymenu(struct v4l2_subdev *sd, struct v4l2_querymenu *qm)
{
	struct v4l2_queryctrl qctrl;
#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif

	qctrl.id = qm->id;
	s5k4ca_queryctrl(sd, &qctrl);

	return v4l2_ctrl_query_menu(qm, &qctrl, s5k4ca_ctrl_get_menu(qm->id));
}

/*
 * Clock configuration
 * Configure expected MCLK from host and return EINVAL if not supported clock
 * frequency is expected
 * 	freq : in Hz
 * 	flag : not supported for now
 */
static int s5k4ca_s_crystal_freq(struct v4l2_subdev *sd, u32 freq, u32 flags)
{
	int err = -EINVAL;
#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif

	return err;
}

static int s5k4ca_g_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	int err = 0;
#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif

	return err;
}

static int s5k4ca_s_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	int err = 0;
#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif

	return err;
}
static int s5k4ca_enum_framesizes(struct v4l2_subdev *sd, \
					struct v4l2_frmsizeenum *fsize)
{
	int err = 0;
#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif

	return err;
}

static int s5k4ca_enum_frameintervals(struct v4l2_subdev *sd, 
					struct v4l2_frmivalenum *fival)
{
	int err = 0;
#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif

	return err;
}

static int s5k4ca_enum_fmt(struct v4l2_subdev *sd, struct v4l2_fmtdesc *fmtdesc)
{
	int err = 0;
#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif

	return err;
}

static int s5k4ca_try_fmt(struct v4l2_subdev *sd, struct v4l2_format *fmt)
{
	int err = 0;
#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif

	return err;
}

static int s5k4ca_g_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = 0;
#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif

	dev_dbg(&client->dev, "%s\n", __func__);

	return err;
}

static int s5k4ca_s_parm(struct v4l2_subdev *sd, struct v4l2_streamparm *param)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = 0;
#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif

	dev_dbg(&client->dev, "%s: numerator %d, denominator: %d\n", \
		__func__, param->parm.capture.timeperframe.numerator, \
		param->parm.capture.timeperframe.denominator);

	return err;
}

static int s5k4ca_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct s5k4ca_state *state = to_state(sd);
	struct s5k4ca_userset userset = state->userset;
	int err = -EINVAL;
#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif

	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE:
		ctrl->value = userset.exposure_bias;
		err = 0;
		break;

	case V4L2_CID_AUTO_WHITE_BALANCE:
		ctrl->value = userset.auto_wb;
		err = 0;
		break;

	case V4L2_CID_WHITE_BALANCE_PRESET:
		ctrl->value = userset.manual_wb;
		err = 0;
		break;

	case V4L2_CID_COLORFX:
		ctrl->value = userset.effect;
		err = 0;
		break;

	case V4L2_CID_CONTRAST:
		ctrl->value = userset.contrast;
		err = 0;
		break;

	case V4L2_CID_SATURATION:
		ctrl->value = userset.saturation;
		err = 0;
		break;

	case V4L2_CID_SHARPNESS:
		ctrl->value = userset.saturation;
		err = 0;
		break;

	default:
		dev_err(&client->dev, "%s: no such ctrl\n", __func__);
		break;
	}
	
	return err;
}

static int s5k4ca_sensor_change_wb(struct v4l2_subdev *sd, int type)
{
	int ret = -1;
#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif
	/* Arun c: Make wb registers same as techwin */
#if 0
	/* 
	 * Arun c: if the previous white balance is auto change it to
	 * manual mode. If it is already 'manual WB' then there is no need
	 * to change to manual WB simply set the desired white balance
	 */
	if (previous_WB_mode == 0) {
		ret = s5k4ba_write_regs(sd, \
			s5k4ca_ae_mwb_unlock, \
			sizeof(s5k4ca_ae_mwb_unlock)/sizeof(s5k4ca_ae_mwb_unlock[0]), "s5k4ca_ae_mwb_unlock");
	}
#endif
        switch (type)
        {
                case 0:
                default :
			/* Arun C: store the white balance value. It is required while returning 
			 * to preview mode after capture */
			previous_WB_mode = 0;
                        printk("-> WB auto mode\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_wb_auto, \
				sizeof(s5k4ca_wb_auto)/sizeof(s5k4ca_wb_auto[0]), "s5k4ca_wb_auto");
                        break;
                case 1:
			previous_WB_mode = 1;
                        printk("-> WB Sunny mode\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_wb_sunny, \
				sizeof(s5k4ca_wb_sunny)/sizeof(s5k4ca_wb_sunny[0]), "s5k4ca_wb_sunny");
                        break;
                case 2:
			previous_WB_mode = 2;
                        printk("-> WB Cloudy mode\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_wb_cloudy, \
				sizeof(s5k4ca_wb_cloudy)/sizeof(s5k4ca_wb_cloudy[0]), "s5k4ca_wb_cloudy");
                        break;
                case 3:
			previous_WB_mode = 3;
                        printk("-> WB Tungsten mode\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_wb_tungsten, \
				sizeof(s5k4ca_wb_tungsten)/sizeof(s5k4ca_wb_tungsten[0]), "s5k4ca_wb_tungsten");
                        break;
                case 4:
			previous_WB_mode = 4;
                        printk("-> WB Flourescent mode\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_wb_fluorescent, \
				sizeof(s5k4ca_wb_fluorescent)/sizeof(s5k4ca_wb_fluorescent[0]), "s5k4ca_wb_fluorescent");
                        break;
        }
//giridhar: return ret value
	return ret;
//        return 0;
}

static int s5k4ca_sensor_change_effect(struct v4l2_subdev *sd, int type)
{
        int ret = -1;
#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif

        printk("[CAM-SENSOR] =Effects Mode %d",type);

        switch (type)
        {
                case 0:
                default:
                        printk("-> Mode None\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_effect_off, \
				sizeof(s5k4ca_effect_off)/sizeof(s5k4ca_effect_off[0]), "s5k4ca_effect_off");
                        break;
                case 1:
                        printk("-> Mode Gray\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_effect_gray, \
				sizeof(s5k4ca_effect_gray)/sizeof(s5k4ca_effect_gray[0]), "s5k4ca_effect_gray");
                        break;
                case 2:
                        printk("-> Mode Negative\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_effect_negative, \
				sizeof(s5k4ca_effect_negative)/sizeof(s5k4ca_effect_negative[0]), \
				"s5k4ca_effect_negative");
                        break;
                case 3:
                        printk("-> Mode Sepia\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_effect_sepia, \
				sizeof(s5k4ca_effect_sepia)/sizeof(s5k4ca_effect_sepia[0]), \
                                "s5k4ca_effect_sepia");
                        break;
                case 4:
                        printk("-> Mode Aqua\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_effect_aqua, \
				sizeof(s5k4ca_effect_aqua)/sizeof(s5k4ca_effect_aqua[0]), "s5k4ca_effect_aqua");
                        break;
                case 5:
                        printk("-> Mode Sketch\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_effect_sketch, \
				sizeof(s5k4ca_effect_sketch)/sizeof(s5k4ca_effect_sketch[0]), "s5k4ca_effect_sketch");
                        break;
        }

        return ret;
}

static int s5k4ca_sensor_change_scene_mode(struct v4l2_subdev *sd, int type)
{
	int ret;
#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif

	printk("\n[S5k4ca] scene mode type is %d\n", type);
	//[CDH] for check dusk & dawn

//	if(previous_scene_mode != 0 && type != 0)
//	{
		//s5k4ca_sensor_write_list(client,s5k4ca_scene_auto,"s5k4ca_scene_auto");

	
	/*
	 * Arun C: for setting scene mode first you have to set auto then
	 * the requested scene mode. If it is auto mode return immediately 
	 **/
	s5k4ba_write_regs(sd, \
			s5k4ca_scene_auto, \
			sizeof(s5k4ca_scene_auto)/sizeof(s5k4ca_scene_auto[0]), "s5k4ca_scene_auto");

	if (type == 0) {
		printk("\n\n Scene mode auto\n\n");
		previous_scene_mode = 0;
		return 0;
	}

//		s5k4ca_sensor_write_list(client,s5k4ca_measure_brightness_center,"s5k4ca_measure_brightness_center");
//		s5k4ca_sensor_write_list(client,s5k4ca_iso_auto,"s5k4ca_iso_auto");
//		s5k4ca_sensor_write_list(client,s5k4ca_wb_auto,"s5k4ca_wb_auto");	
		//AF Normal setting	
/*		
		s5k4ca_sensor_write(client, 0xFCFC, 0xD000);    
		s5k4ca_sensor_write(client, 0x0028, 0x7000);    
		s5k4ca_sensor_write(client, 0x002A, 0x161C);    
		s5k4ca_sensor_write(client, 0x0F12, 0x82A8);    //Normal Mode Select

		s5k4ca_sensor_write(client, 0x002A, 0x030E);    
		s5k4ca_sensor_write(client, 0x0F12, 0x00FE);    
		s5k4ca_sensor_write(client, 0x002A, 0x030C);    
		s5k4ca_sensor_write(client, 0x0F12, 0x0000);    

		msleep(150);

		s5k4ca_sensor_write(client, 0x002A, 0x030E);    
		s5k4ca_sensor_write(client, 0x0F12, 0x00FF);   

		msleep(200);		
*/
		
	switch (type)
	{
	/* Arun C: Don't check for auto scene mode here its already set */
#if 0 
		case 0:
			printk("\n\n Scene mode auto\n\n");
			previous_scene_mode = type;
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_scene_auto, \
				sizeof(s5k4ca_scene_auto)/sizeof(s5k4ca_scene_auto[0]), "s5k4ca_scene_auto");
			break;
#endif
		case 1:
			printk("\n\n Scene mode portrait\n\n");
			previous_scene_mode = type;
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_scene_portrait, \
				sizeof(s5k4ca_scene_portrait)/sizeof(s5k4ca_scene_portrait[0]), "s5k4ca_scene_portrait");
			break;
		case 2:
			printk("\n\n Scene mode landscape\n\n");
			previous_scene_mode = type;
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_scene_landscape, \
				sizeof(s5k4ca_scene_landscape)/sizeof(s5k4ca_scene_landscape[0]), \
				"s5k4ca_scene_landscape");
			break;
		case 3:
			printk("\n\n Scene mode sport\n\n");
			previous_scene_mode = type;
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_scene_sport, \
				sizeof(s5k4ca_scene_sport)/sizeof(s5k4ca_scene_sport[0]), "s5k4ca_scene_sport");
			break;
		case 4:
			printk("\n\n Scene mode sunset\n\n");
			previous_scene_mode = type;
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_scene_sunset, \
				sizeof(s5k4ca_scene_sunset)/sizeof(s5k4ca_scene_sunset[0]), "s5k4ca_scene_sunset");
			break;
		case 5:
			printk("\n\n Scene mode candlelight\n\n");
			previous_scene_mode = type;
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_scene_candlelight, \
				sizeof(s5k4ca_scene_candlelight)/sizeof(s5k4ca_scene_candlelight[0]), \
				"s5k4ca_scene_candlelight");
			break;
		case 6:
			printk("\n\n Scene mode fireworks\n\n");
			previous_scene_mode = type;
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_scene_fireworks, \
				sizeof(s5k4ca_scene_fireworks)/sizeof(s5k4ca_scene_fireworks[0]), \
				"s5k4ca_scene_fireworks");
			break;
		case 7:
			printk("\n\n Scene mode text\n\n");
			previous_scene_mode = type;
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_scene_text, \
				sizeof(s5k4ca_scene_text)/sizeof(s5k4ca_scene_text[0]), "s5k4ca_scene_text");
			break;
		case 8:
			printk("\n\n Scene mode night\n\n");
			previous_scene_mode = type;
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_scene_night, \
				sizeof(s5k4ca_scene_night)/sizeof(s5k4ca_scene_night[0]), "s5k4ca_scene_night");
			break;
		case 9:
			printk("\n\n Scene mode beach and snow\n\n");
			previous_scene_mode = type;
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_scene_beach, \
				sizeof(s5k4ca_scene_beach)/sizeof(s5k4ca_scene_beach[0]), \
				"s5k4ca_scene_beach");
			break;
		case 10:
			printk("\n\n Scene mode party\n\n");
			previous_scene_mode = type;
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_scene_party, \
				sizeof(s5k4ca_scene_party)/sizeof(s5k4ca_scene_party[0]), \
				"s5k4ca_scene_party");
			break;
		case 11:
			printk("\n\n Scene mode backlight\n\n");
			previous_scene_mode = type;
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_scene_backlight, \
				sizeof(s5k4ca_scene_backlight)/sizeof(s5k4ca_scene_backlight[0]), \
				"s5k4ca_scene_backlight");
			break;
		case 12://[CDH] this number can changed by Application team. it's temporary number for duskdawn
			printk("\n\n Scene mode dusk and dawn\n\n");
			previous_scene_mode = type;
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_scene_duskdawn, \
				sizeof(s5k4ca_scene_duskdawn)/sizeof(s5k4ca_scene_duskdawn[0]), \
				"s5k4ca_scene_duskdawn");
			break;
		case 13:
			printk("\n\n Scene mode fall-color\n\n");
			previous_scene_mode = type;
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_scene_fallcolor, \
				sizeof(s5k4ca_scene_fallcolor)/sizeof(s5k4ca_scene_fallcolor[0]), \
				"s5k4ca_scene_fallcolor");
			break;
		default :
			printk("\n\n Scene mode default and error\n\n");
			ret = 0;
			break;
	}
	
	return 0;
}

#ifdef CONFIG_LOAD_FILE

#include <linux/vmalloc.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>

#include <asm/uaccess.h>

static char *s5k4ca_regs_table = NULL;

static int s5k4ca_regs_table_size;

void s5k4ca_regs_table_init(void)
{
#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif

	printk("[CDH] + s5k4ca_regs_table_init\n");
	struct file *filp;
	char *dp;
	long l;
	loff_t pos;
	int i;
	int ret;
	mm_segment_t fs = get_fs();

	printk("%s %d\n", __func__, __LINE__);

	set_fs(get_ds());
#if 0
	filp = filp_open("/data/camera/s5k4ca.h", O_RDONLY, 0);
#else
	filp = filp_open("/sdcard/s5k4ca.h", O_RDONLY, 0);
#endif

	if (IS_ERR(filp)) {
		printk("file open error\n");
		return;
	}
	
	l = filp->f_path.dentry->d_inode->i_size;	
	printk("l = %ld\n", l);
	dp = kmalloc(l, GFP_KERNEL);
	
	if (dp == NULL) {
		printk("Out of Memory\n");
		filp_close(filp, current->files);
	}
	
	pos = 0;
	memset(dp, 0, l);
	ret = vfs_read(filp, (char __user *)dp, l, &pos);
	
	if (ret != l) {
		printk("Failed to read file ret = %d\n", ret);
		kfree(dp);
		filp_close(filp, current->files);
		return;
	}

	filp_close(filp, current->files);
		
	set_fs(fs);
	
	s5k4ca_regs_table = dp;
		
	s5k4ca_regs_table_size = l;
	
	*((s5k4ca_regs_table + s5k4ca_regs_table_size) - 1) = '\0';
	
	printk("s5k4ca_regs_table 0x%08x, %ld\n", dp, l);
	printk("[CDH] - s5k4ca_reg_table_init\n");
}

void s5k4ca_regs_table_exit(void)
{
#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif

	printk("[CDH] + s5k4ca_regs_table_exit\n");
	printk("%s %d\n", __func__, __LINE__);
	if (s5k4ca_regs_table) {
		kfree(s5k4ca_regs_table);
		s5k4ca_regs_table = NULL;
	}
	printk("[CDH] - s5k4ca_regs_table_exit\n");
}

static int s5k4ca_regs_table_write(struct i2c_client *client, char *name)
{
	printk("[CDH] + s5k4ca_regs_table_write\n");
	char *start, *end, *reg, *data;	
	unsigned short addr, value;
	char reg_buf[7], data_buf[7];
	#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif
	
	*(reg_buf + 6) = '\0';
	*(data_buf + 6) = '\0';
	
	start = strstr(s5k4ca_regs_table, name);
	end = strstr(start, "};");
	
	while (1) {	
		/* Find Address */	
		reg = strstr(start,"{0x");		
		if (reg)
			start = (reg + 16);
		if ((reg == NULL) || (reg > end))
			break;
		/* Write Value to Address */	
		if (reg != NULL) {
			memcpy(reg_buf, (reg + 1), 6);	
			memcpy(data_buf, (reg + 9), 6);	
			addr = (unsigned short)simple_strtoul(reg_buf, NULL, 16); 
			value = (unsigned short)simple_strtoul(data_buf, NULL, 16); 
			//printk("addr 0x%04x, value 0x%04x\n", addr, value);
			
			if (addr == 0xdddd)
			{
/*				if (value == 0x0010)
				mdelay(10);
				else if (value == 0x0020)
				mdelay(20);
				else if (value == 0x0030)
				mdelay(30);
				else if (value == 0x0040)
				mdelay(40);
				else if (value == 0x0050)
				mdelay(50);
				else if (value == 0x0100)
				mdelay(100);*/
				mdelay(value);
				printk("delay 0x%04x, value 0x%04x\n", addr, value);
			}	
			else
				s5k4ca_sensor_write(client, addr, value);
		}
	}
	printk("[CDH] - s5k4ca_regs_table_write\n");
	return 0;
}

#endif

static int s5k4ca_sensor_change_br(struct v4l2_subdev *sd, int type)
{
	int ret = 0;
#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif

	printk("[CAM-SENSOR] =Brightness Mode %d",type);

	switch (type)
	{
		case 0:	
		default :
			printk("-> Brightness Minus 4\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_br_minus4, \
				sizeof(s5k4ca_br_minus4)/sizeof(s5k4ca_br_minus4[0]), \
				"s5k4ca_br_minus4");
			break;
		case 1:
			printk("-> Brightness Minus 3\n");	
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_br_minus3, \
				sizeof(s5k4ca_br_minus3)/sizeof(s5k4ca_br_minus3[0]), \
				"s5k4ca_br_minus3");
			break;
		case 2:
			printk("-> Brightness Minus 2\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_br_minus2, \
				sizeof(s5k4ca_br_minus2)/sizeof(s5k4ca_br_minus2[0]), \
				"s5k4ca_br_minus2");
			break;
		case 3:
			printk("-> Brightness Minus 1\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_br_minus1, \
				sizeof(s5k4ca_br_minus1)/sizeof(s5k4ca_br_minus1[0]), \
				"s5k4ca_br_minus1");
			break;
		case 4:
			printk("-> Brightness Zero\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_br_zero, \
				sizeof(s5k4ca_br_zero)/sizeof(s5k4ca_br_zero[0]), \
				"s5k4ca_br_zero");
			break;
		case 5:
			printk("-> Brightness Plus 1\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_br_plus1, \
				sizeof(s5k4ca_br_plus1)/sizeof(s5k4ca_br_plus1[0]), \
				"s5k4ca_br_plus1");
			break;
		case 6:
			printk("-> Brightness Plus 2\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_br_plus2, \
				sizeof(s5k4ca_br_plus2)/sizeof(s5k4ca_br_plus2[0]), \
				"s5k4ca_br_plus2");
			break;
		case 7:
			printk("-> Brightness Plus 3\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_br_plus3, \
				sizeof(s5k4ca_br_plus3)/sizeof(s5k4ca_br_plus3[0]), \
				"s5k4ca_br_plus3");
			break;
		case 8:
			printk("-> Brightness Plus 4\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_br_plus4, \
				sizeof(s5k4ca_br_plus4)/sizeof(s5k4ca_br_plus4[0]), \
				"s5k4ca_br_plus4");
			break;
	}

	return ret;
}

static int s5k4ca_sensor_change_contrast(struct v4l2_subdev *sd, int type)
{
	int ret = 0;
	#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif
	
	printk("[CAM-SENSOR] =Contras Mode %d",type);

	switch (type)
	{
		case 0:
			printk("-> Contrast -2\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_contrast_m2, \
				sizeof(s5k4ca_contrast_m2)/sizeof(s5k4ca_contrast_m2[0]), \
				"s5k4ca_contrast_m2");
			break;
		case 1:
			printk("-> Contrast -1\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_contrast_m1, \
				sizeof(s5k4ca_contrast_m1)/sizeof(s5k4ca_contrast_m1[0]), \
				"s5k4ca_contrast_m1");
			break;
		default :
		case 2:
			printk("-> Contrast 0\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_contrast_0, \
				sizeof(s5k4ca_contrast_0)/sizeof(s5k4ca_contrast_0[0]), \
				"s5k4ca_contrast_0");
			break;
		case 3:
			printk("-> Contrast +1\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_contrast_p1, \
				sizeof(s5k4ca_contrast_p1)/sizeof(s5k4ca_contrast_p1[0]), \
				"s5k4ca_contrast_p1");
			break;
		case 4:
			printk("-> Contrast +2\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_contrast_p2, \
				sizeof(s5k4ca_contrast_p2)/sizeof(s5k4ca_contrast_p2[0]), \
				"s5k4ca_contrast_p2");
			break;
	}

	return ret;
}

static int s5k4ca_sensor_change_saturation(struct v4l2_subdev *sd, int type)
{
	int ret;
	#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif
	
	printk("[CAM-SENSOR] =Saturation Mode %d",type);

	switch (type)
	{
		case 0:
			printk("-> Saturation -2\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_Saturation_m2, \
				sizeof(s5k4ca_Saturation_m2)/sizeof(s5k4ca_Saturation_m2[0]), \
				"s5k4ca_Saturation_m2");
			break;
		case 1:
			printk("-> Saturation -1\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_Saturation_m1, \
				sizeof(s5k4ca_Saturation_m1)/sizeof(s5k4ca_Saturation_m1[0]), \
				"s5k4ca_Saturation_m1");
			break;
		case 2:
		default :
			printk("-> Saturation 0\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_Saturation_0, \
				sizeof(s5k4ca_Saturation_0)/sizeof(s5k4ca_Saturation_0[0]), \
				"s5k4ca_Saturation_0");
			break;
		case 3:
			printk("-> Saturation +1\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_Saturation_p1, \
				sizeof(s5k4ca_Saturation_p1)/sizeof(s5k4ca_Saturation_p1[0]), \
				"s5k4ca_Saturation_p1");
			break;
		case 4:
			printk("-> Saturation +2\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_Saturation_p2, \
				sizeof(s5k4ca_Saturation_p2)/sizeof(s5k4ca_Saturation_p2[0]), \
				"s5k4ca_Saturation_p2");
			break;
	}

	return ret;
}

static int s5k4ca_sensor_change_sharpness(struct v4l2_subdev *sd, int type)
{
	int ret;
	#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif
	
	printk("[CAM-SENSOR] =Sharpness Mode %d",type);

	switch (type)
	{
		case 0:
			printk("-> Sharpness -2\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_Sharpness_m2, \
				sizeof(s5k4ca_Sharpness_m2)/sizeof(s5k4ca_Sharpness_m2[0]), \
				"s5k4ca_Sharpness_m2");
			break;
		case 1:
			printk("-> Sharpness -1\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_Sharpness_m1, \
				sizeof(s5k4ca_Sharpness_m1)/sizeof(s5k4ca_Sharpness_m1[0]), \
				"s5k4ca_Sharpness_m1");
			break;
		case 2:
		default :
			printk("-> Sharpness 0\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_Sharpness_0, \
				sizeof(s5k4ca_Sharpness_0)/sizeof(s5k4ca_Sharpness_0[0]), \
				"s5k4ca_Sharpness_0");
			break;
		case 3:
			printk("-> Sharpness +1\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_Sharpness_p1, \
				sizeof(s5k4ca_Sharpness_p1)/sizeof(s5k4ca_Sharpness_p1[0]), \
				"s5k4ca_Sharpness_p1");
			break;
		case 4:
			printk("-> Sharpness +2\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_Sharpness_p2, \
				sizeof(s5k4ca_Sharpness_p2)/sizeof(s5k4ca_Sharpness_p2[0]), \
				"s5k4ca_Sharpness_p2");
			break;
	}

	return ret;
}

static int s5k4ca_sensor_change_iso(struct v4l2_subdev *sd, int type)
{
	int ret;
	#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif
	
	printk("[CAM-SENSOR] =Iso Mode %d",type);

	switch (type)
	{
		case 0:
		default :
			printk("-> ISO AUTO\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_iso_auto, \
				sizeof(s5k4ca_iso_auto)/sizeof(s5k4ca_iso_auto[0]), \
				"s5k4ca_iso_auto");
			break;
		case 1:
			printk("-> ISO 50\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_iso50, \
				sizeof(s5k4ca_iso50)/sizeof(s5k4ca_iso50[0]), \
				"s5k4ca_iso50");
			break;
		case 2:
			printk("-> ISO 100\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_iso100, \
				sizeof(s5k4ca_iso100)/sizeof(s5k4ca_iso100[0]), \
				"s5k4ca_iso100");
			break;
		case 3:
			printk("-> ISO 200\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_iso200, \
				sizeof(s5k4ca_iso200)/sizeof(s5k4ca_iso200[0]), \
				"s5k4ca_iso200");
			break;
		case 4:
			printk("-> ISO 400\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_iso400, \
				sizeof(s5k4ca_iso400)/sizeof(s5k4ca_iso400[0]), \
				"s5k4ca_iso400");
			break;
	}

	return ret;
}

static int s5k4ca_sensor_change_photometry(struct v4l2_subdev *sd, int type)
{
	int ret;
	#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif
	
	printk("[CAM-SENSOR] =Photometry Mode %d",type);

	switch (type)
	{
		case 0:
			printk("-> Photometry SPOT\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_measure_brightness_spot, \
				sizeof(s5k4ca_measure_brightness_spot)/sizeof(s5k4ca_measure_brightness_spot[0]), \
				"s5k4ca_measure_brightness_spot");
			break;
		case 1:
		default :
			printk("-> Photometry Default\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_measure_brightness_default, \
				sizeof(s5k4ca_measure_brightness_default)/sizeof(s5k4ca_measure_brightness_default[0]),\
				"s5k4ca_measure_brightness_default");
			break;
		case 2:
			printk("-> Photometry CENTER\n");
			ret = s5k4ba_write_regs(sd, \
				s5k4ca_measure_brightness_center, \
				sizeof(s5k4ca_measure_brightness_center)/sizeof(s5k4ca_measure_brightness_center[0]),\
				"s5k4ca_measure_brightness_center");
			break;
	}

	return ret;
}

static int s5k4ca_sensor_mode_set(struct v4l2_subdev *sd, int type)
{
	int ret = 0;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int delay;
#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif

	printk("[CAM-SENSOR] =Sensor Mode ");

	if (type & SENSOR_PREVIEW)
	{	
		/*
		 * Arun c: UI is loaded by the HAL
		 */
		printk("-> Preview ");
		
		if (!preview_in_init) {
			ret = s5k4ba_write_regs(sd, \
					s5k4ca_preview, \
					sizeof(s5k4ca_preview)/sizeof(s5k4ca_preview[0]), "s5k4ca_preview");
			delay = 0;
		} else {
			preview_in_init = 0;
//			delay = 200;
			delay = 0;
		}

	}
	else if (type & SENSOR_CAPTURE)
	{	
		printk("-> Capture ");

//		s5k4ca_sensor_write_list(client,s5k4ca_capture_0,"s5k4ca_capture_0"); // 2048x1536(3M)

		//AE/AWB UNLOCK 
		printk("AF_AWB_UNLOCK ON PREVIEW\n");
		if(previous_WB_mode==0)
		{
			s5k4ba_write_regs(sd, \
					s5k4ca_ae_awb_unlock, \
					sizeof(s5k4ca_ae_awb_unlock)/sizeof(s5k4ca_ae_awb_unlock[0]), \
					"s5k4ca_ae_awb_unlock");			
		}
		else
		{
			s5k4ba_write_regs(sd, \
					s5k4ca_ae_mwb_unlock, \
					sizeof(s5k4ca_ae_mwb_unlock)/sizeof(s5k4ca_ae_mwb_unlock[0]), \
					"s5k4ca_ae_mwb_unlock");				
		}
		
		s5k4ca_sensor_write(client, 0xFCFC, 0xD000);	
		s5k4ca_sensor_write(client, 0x002C, 0x7000);
		s5k4ca_sensor_write(client, 0x002E, 0x12FE);

		s5k4ca_sensor_read(client, 0x0F12, &lux_value);

		if (lux_value <= 0x40) /* Low light */
		{	
#if 1//remove nightmode		
			if (previous_scene_mode==8)//scene night
			{	
				printk("Night Low Light light=0x%04x\n",lux_value);
				delay = 1600;
				//s5k4ca_sensor_write_list(client,s5k4ca_snapshot_nightmode,"s5k4ca_snapshot_nightmode");
				ret = s5k4ba_write_regs(sd, \
					s5k4ca_snapshot_nightmode, \
					sizeof(s5k4ca_snapshot_nightmode)/sizeof(s5k4ca_snapshot_nightmode[0]), \
					"s5k4ca_snapshot_nightmode");
			}
			else
#endif				
			{	
				printk("Normal Low Light light=0x%04x\n",lux_value);
				delay = 800;
				//s5k4ca_sensor_write_list(client,s5k4ca_snapshot_low,"s5k4ca_snapshot_low");
				ret = s5k4ba_write_regs(sd, \
					s5k4ca_snapshot_low, \
					sizeof(s5k4ca_snapshot_low)/sizeof(s5k4ca_snapshot_low[0]), \
					"s5k4ca_snapshot_low");
			}
		}
		else
		{
			printk("Normal Normal Light light=0x%04x\n",lux_value);
			delay = 200;
			//s5k4ca_sensor_write_list(client,s5k4ca_snapshot_normal,"s5k4ca_snapshot_normal");
			ret = s5k4ba_write_regs(sd, \
					s5k4ca_snapshot_normal, \
					sizeof(s5k4ca_snapshot_normal)/sizeof(s5k4ca_snapshot_normal[0]), \
					"s5k4ca_snapshot_normal");
		}
	}
	else if (type & SENSOR_CAMCORDER )
	{
		printk("-> Record\n");
		delay = 300;
		//s5k4ca_sensor_write_list(client,s5k4ca_fps_15fix,"s5k4ca_fps_15fix");
		ret = s5k4ba_write_regs(sd, \
					s5k4ca_fps_15fix, \
					sizeof(s5k4ca_fps_15fix)/sizeof(s5k4ca_fps_15fix[0]), \
					"s5k4ca_fps_15fix");
//giridhar: !!!!!!!!!!!!check this first init
		//first_init_end = 1;
	}

	msleep(delay);
	
	printk("[CAM-SENSOR] =delay time(%d msec)\n", delay);
	
	return ret;
}

#if 1//bestiq
static int s5k4ca_sensor_af_control(struct v4l2_subdev *sd, int type)
{
    int count = 50;
    int tmpVal = 0;
    int ret = 0;
    int size = 0;
    int i = 0;
    unsigned short light = 0;
    struct i2c_client *client = v4l2_get_subdevdata(sd);
#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
	printk("[S5k4CA] %s function type is %d\n", __func__, type);
#endif
	
    switch (type)
    {
     case 0: // CASE 0 for AF Release

		//AE/AWB UNLOCK 
		printk("AF_AWB_UNLOCK on AF RELEASE~!!!\n");
		#if 1// FOR SUNSET ERROR
		if(previous_WB_mode==0 && previous_scene_mode !=4)
		#else
		if(previous_WB_mode==0)
		#endif
		{
			s5k4ba_write_regs(sd, \
					s5k4ca_ae_awb_unlock, \
					sizeof(s5k4ca_ae_awb_unlock)/sizeof(s5k4ca_ae_awb_unlock[0]), \
					"s5k4ca_ae_awb_unlock");			
		}
		else
		{
			s5k4ba_write_regs(sd, \
					s5k4ca_ae_mwb_unlock, \
					sizeof(s5k4ca_ae_mwb_unlock)/sizeof(s5k4ca_ae_mwb_unlock[0]), \
					"s5k4ca_ae_mwb_unlock");				
		}

		if(macroType == 0)//normal AF
		{
			s5k4ca_sensor_write(client, 0xFCFC, 0xD000);    
			s5k4ca_sensor_write(client, 0x0028, 0x7000);    

			s5k4ca_sensor_write(client, 0x002A, 0x030E);    
			s5k4ca_sensor_write(client, 0x0F12, 0x00F0);    
			s5k4ca_sensor_write(client, 0x002A, 0x030C);    
			s5k4ca_sensor_write(client, 0x0F12, 0x0000);    //set manual AF
			msleep(133); // 1frame delay, 7.5fps = 133ms
		
			s5k4ca_sensor_write(client, 0x002A, 0x030E);    
			s5k4ca_sensor_write(client, 0x0F12, 0x00FF);    //00FF: infinity
		}
		else if(macroType == 1)//macro AF
		{
			s5k4ca_sensor_write(client, 0xFCFC, 0xD000);    
			s5k4ca_sensor_write(client, 0x0028, 0x7000);    

			s5k4ca_sensor_write(client, 0x002A, 0x030E);    
			s5k4ca_sensor_write(client, 0x0F12, 0x005F);    
			s5k4ca_sensor_write(client, 0x002A, 0x030C);    
			s5k4ca_sensor_write(client, 0x0F12, 0x0000);    //set manual AF
			msleep(133); // 1frame delay, 7.5fps = 133ms
		
			s5k4ca_sensor_write(client, 0x002A, 0x030E);    
			s5k4ca_sensor_write(client, 0x0F12, 0x0050);    //0050: macro
		}		
		msleep(100);
            break;
	
        case 1:
            printk("Focus Mode -> Single\n");

	
	s5k4ca_sensor_write(client, 0xFCFC, 0xD000);	
	s5k4ca_sensor_write(client, 0x002C, 0x7000);
	s5k4ca_sensor_write(client, 0x002E, 0x12FE);
						
	s5k4ca_sensor_read(client, 0x0F12, &light);
	if (light < 0x80){ /* Low light AF*/
						
		size = (sizeof(s5k4ca_af_low_lux_val)/sizeof(s5k4ca_af_low_lux_val[0]));
		for (i = 0; i < size; i++)	{
			s5k4ca_sensor_write(client, s5k4ca_af_low_lux_val[i].subaddr, \
					s5k4ca_af_low_lux_val[i].value);	
		}
		printk("[CAM-SENSOR] =Low Light AF Single light=0x%04x\n",light);
	}
	else{
		size = (sizeof(s5k4ca_af_normal_lux_val)/sizeof(s5k4ca_af_normal_lux_val[0]));
		for (i = 0; i < size; i++)	{
			s5k4ca_sensor_write(client, s5k4ca_af_normal_lux_val[i].subaddr, \
					s5k4ca_af_normal_lux_val[i].value);	
		}
		printk("[CAM-SENSOR] =Normal Light AF Single light=0x%04x\n",light);
	}		
			s5k4ba_write_regs(sd, \
					s5k4ca_ae_awb_lock, \
					sizeof(s5k4ca_ae_awb_lock)/sizeof(s5k4ca_ae_awb_lock[0]), \
					"s5k4ca_ae_awb_lock");			
						           
		if(macroType == 0)//normal AF	
		{
            s5k4ca_sensor_write(client, 0xFCFC, 0xD000);
            s5k4ca_sensor_write(client, 0x0028, 0x7000);
            
            s5k4ca_sensor_write(client, 0x002A, 0x030E);  
            s5k4ca_sensor_write(client, 0x0F12, 0x00DF);  //030E = 00FF   
            
            s5k4ca_sensor_write(client, 0x002A, 0x030C);
            s5k4ca_sensor_write(client, 0x0F12, 0x0000); // AF Manual
            
            msleep(130);
            
            s5k4ca_sensor_write(client, 0x002A, 0x030E); 
            s5k4ca_sensor_write(client, 0x0F12, 0x00E0); 

            msleep(50);		
		}
		else if(macroType == 1)
		{
		    s5k4ca_sensor_write(client, 0xFCFC, 0xD000);    
		    s5k4ca_sensor_write(client, 0x0028, 0x7000);    

		    s5k4ca_sensor_write(client, 0x002A, 0x030E);    
		    s5k4ca_sensor_write(client, 0x0F12, 0x005F);    //?? ??? ??

		    s5k4ca_sensor_write(client, 0x002A, 0x030C);    
		    s5k4ca_sensor_write(client, 0x0F12, 0x0000);    //set manual AF

		    msleep(133); // 1frame delay, 7.5fps = 133ms    //????? ?? ????? ?? ???, ???????? ???? ????.

		    s5k4ca_sensor_write(client, 0x002A, 0x030E);    
		    s5k4ca_sensor_write(client, 0x0F12, 0x0050);    //0050: macro

	            msleep(50);			
		}
            s5k4ca_sensor_write(client, 0x002A, 0x030C);
            s5k4ca_sensor_write(client, 0x0F12, 0x0002); //AF Single 
			msleep(50);
            do
            {

                if( count == 0)
                    break;

       		s5k4ca_sensor_write(client, 0xFCFC, 0xD000);
                s5k4ca_sensor_write(client, 0x002C, 0x7000);    
                s5k4ca_sensor_write(client, 0x002E, 0x130E);
                if (light < 0x80)
	                msleep(250);
		else
			msleep(100);
                s5k4ca_sensor_read(client, 0x0F12, &tmpVal); 

                count--;

                printk("CAM 3M AF Status Value = %x \n", tmpVal); 

            }
            while( (tmpVal & 0x3) != 0x3 && (tmpVal & 0x3) != 0x2 );

            if(count == 0  )
            {
	      		if(macroType == 0)//normal AF	
			{
            		s5k4ca_sensor_write(client, 0xFCFC, 0xD000); 
            		s5k4ca_sensor_write(client, 0x0028, 0x7000); 

		        s5k4ca_sensor_write(client, 0x002A, 0x030E);  
            		s5k4ca_sensor_write(client, 0x0F12, 0x00DF);  //030E = 00FF   

		        s5k4ca_sensor_write(client, 0x002A, 0x030C); 
            		s5k4ca_sensor_write(client, 0x0F12, 0x0000); // AF Manual 

		            msleep(130);
		            
		            s5k4ca_sensor_write(client, 0x002A, 0x030E); 
		            s5k4ca_sensor_write(client, 0x0F12, 0x00E0); 

		            msleep(50);		
			}
			else if(macroType == 1)
			{
			    s5k4ca_sensor_write(client, 0xFCFC, 0xD000);    
			    s5k4ca_sensor_write(client, 0x0028, 0x7000);    

    		        s5k4ca_sensor_write(client, 0x002A, 0x030E);  
			    s5k4ca_sensor_write(client, 0x0F12, 0x005F);    //?? ??? ??

			    s5k4ca_sensor_write(client, 0x002A, 0x030C);    
			    s5k4ca_sensor_write(client, 0x0F12, 0x0000);    //set manual AF

			    msleep(133); // 1frame delay, 7.5fps = 133ms    //????? ?? ????? ?? ???, ???????? ???? ????.

			    s5k4ca_sensor_write(client, 0x002A, 0x030E);    
			    s5k4ca_sensor_write(client, 0x0F12, 0x0050);    //0050: macro
             
		            msleep(50);			
			}					
            		            	
            		ret = 0;
            		printk("CAM 3M AF_Single Mode Fail.==> TIMEOUT \n");
                
            }

            if((tmpVal & 0x3) == 0x02)
            {
	      		if(macroType == 0)//normal AF	
			{
            		s5k4ca_sensor_write(client, 0xFCFC, 0xD000); 
            		s5k4ca_sensor_write(client, 0x0028, 0x7000); 

		        s5k4ca_sensor_write(client, 0x002A, 0x030E);  
            		s5k4ca_sensor_write(client, 0x0F12, 0x00DF);  //030E = 00FF   

		        s5k4ca_sensor_write(client, 0x002A, 0x030C); 
            		s5k4ca_sensor_write(client, 0x0F12, 0x0000); // AF Manual 

		            msleep(130);
		            
    		        s5k4ca_sensor_write(client, 0x002A, 0x030E);  
		            s5k4ca_sensor_write(client, 0x0F12, 0x00E0); 

		            msleep(50);		
			}
			else if(macroType == 1)
			{
			    s5k4ca_sensor_write(client, 0xFCFC, 0xD000);    
			    s5k4ca_sensor_write(client, 0x0028, 0x7000);    

			    s5k4ca_sensor_write(client, 0x002A, 0x030E);    
			    s5k4ca_sensor_write(client, 0x0F12, 0x005F);    //?? ??? ??
             
			    s5k4ca_sensor_write(client, 0x002A, 0x030C);    
			    s5k4ca_sensor_write(client, 0x0F12, 0x0000);    //set manual AF

			    msleep(133); // 1frame delay, 7.5fps = 133ms    //????? ?? ????? ?? ???, ???????? ???? ????.

			    s5k4ca_sensor_write(client, 0x002A, 0x030E);    
			    s5k4ca_sensor_write(client, 0x0F12, 0x0050);    //0050: macro

		            msleep(50);			
			}				
              	
              	ret = 0;

	            	printk("CAM 3M AF_Single Mode Fail.==> FAIL \n");
            }

            if(tmpVal & 0x3 == 0x3)
            {
                printk("CAM 3M AF_Single Mode SUCCESS. \r\n");
                ret = 1;
            }
	   
            printk("CAM:3M AF_SINGLE SET \r\n");

            break;

	case 2: //Normal AF
	if (preview_in_init_af || (macroType == 1)){
		    macroType = 0;
		    printk("[S5k4CA] Macro Mode off\n");
			size = (sizeof(s5k4ca_af_macro_off)/sizeof(s5k4ca_af_macro_off[0]));
			for (i = 0; i < size; i++)	{
				s5k4ca_sensor_write(client, s5k4ca_af_macro_off[i].subaddr, \
						s5k4ca_af_macro_off[i].value);	
			}
			preview_in_init_af = 0;
		}
	    break;
	case 4: //Macro AF
	if (preview_in_init_af || (macroType == 0)){	
		    macroType = 1;
		    printk("[S5k4CA] Macro Mode on\n");
			size = (sizeof(s5k4ca_af_macro_on)/sizeof(s5k4ca_af_macro_on[0]));
			for (i = 0; i < size; i++)	{
				s5k4ca_sensor_write(client, s5k4ca_af_macro_on[i].subaddr, \
						s5k4ca_af_macro_on[i].value);	
			}
			preview_in_init_af = 0;			
		}
	    break;
        default:
            break;

    }
    return ret;
}

#endif

/*
 * Arun c
 * Read the exposure time; and use the lux_value from the last
 * picture captured. Fill these values to the pointer supplied 
 * by the platform
 */
static int s5k4ca_sensor_exif_read(struct v4l2_subdev *sd, int type)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	unsigned short exif_data[2];
	
	/* Read the exposure value */
	s5k4ca_sensor_write(client, 0xFCFC, 0xD000);
	s5k4ca_sensor_write(client, 0x002C, 0x7000);
	s5k4ca_sensor_write(client, 0x002E, 0x1C3C);
	s5k4ca_sensor_read(client, 0x0F12, &exif_data[0]);

	exif_data[0] = exif_data[0] / 100;
	exif_data[1] = lux_value;

	copy_to_user((void __user*)type, exif_data, sizeof(exif_data));

	printk("[CAM-SENSOR] =%s extime=%d, lux=%d,\n", __func__, exif_data[0], lux_value);

	return 0;
}

static int s5k4ca_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif

#ifdef S5K4CA_COMPLETE
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = -EINVAL;

	printk("[S5k4CA] %s function ctrl->id : %d \n", __func__, ctrl->id);
	//[CDH] check for Metering

	switch (ctrl->id) {
#if 0//giridhar: nobody calling this and no values
	case V4L2_CID_EXPOSURE:
		dev_dbg(&client->dev, "%s: V4L2_CID_EXPOSURE\n", __func__);
		err = s5k4ba_write_regs(sd, \
			(unsigned char *) s5k4ba_regs_ev_bias[ctrl->value], \
			sizeof(s5k4ba_regs_ev_bias[ctrl->value]), \
			"s5k4ba_regs_ev_bias");
		break;
#endif

	case V4L2_CID_SENSOR:
#ifdef VIEW_FUNCTION_CALL
		printk("[S5k4CA] %s function V4L2_CID_SENSOR\n", __func__);
#endif
	
		err = s5k4ca_sensor_mode_set(sd, ctrl->value);
		break;

	case V4L2_CID_FOCUS_AUTO:
#ifdef VIEW_FUNCTION_CALL
		printk("[S5k4CA] %s function V4L2_CID_FOCUS_AUTO\n", __func__);
#endif
	
		err = s5k4ca_sensor_af_control(sd, ctrl->value);
		/* 
		 * Arun c: for HAL
		 * 0 -> AF fail
		 * 1 -> AF success
		 * -ve -> not found
		 */
		return err;
		break;


	case V4L2_CID_AUTO_WHITE_BALANCE:
#ifdef VIEW_FUNCTION_CALL
		printk("[S5k4CA] %s function V4L2_CID_AUTO_WHITE_BALANCE\n",__func__);
#endif
		dev_dbg(&client->dev, "%s: V4L2_CID_AUTO_WHITE_BALANCE\n", \
			__func__);
		/*err = s5k4ba_write_regs(sd, \
			(unsigned char *) s5k4ba_regs_awb_enable[ctrl->value], \
			sizeof(s5k4ba_regs_awb_enable[ctrl->value]), \
			"s5k4ba_regs_awb_enable");
		*/
		err = s5k4ca_sensor_change_wb(sd, ctrl->value);
		break;
#if 0
	case V4L2_CID_WHITE_BALANCE_PRESET:
		dev_dbg(&client->dev, "%s: V4L2_CID_WHITE_BALANCE_PRESET\n", \
			__func__);
		err = s5k4ba_write_regs(sd, \
			(unsigned char *) s5k4ba_regs_wb_preset[ctrl->value], \
			sizeof(s5k4ba_regs_wb_preset[ctrl->value]), \
			"s5k4ba_regs_wb_preset");
		break;
#endif
	case V4L2_CID_COLORFX:
#ifdef VIEW_FUNCTION_CALL
		printk("[S5k4CA] %s function V4L2_CID_COLORFX\n", __func__);
#endif
	
		dev_dbg(&client->dev, "%s: V4L2_CID_COLORFX\n", __func__);
		/*err = s5k4ba_write_regs(sd, \
			(unsigned char *) s5k4ba_regs_color_effect[ctrl->value], \
			sizeof(s5k4ba_regs_color_effect[ctrl->value]), \
			"s5k4ba_regs_color_effect");
		*/
		err = s5k4ca_sensor_change_effect(sd, ctrl->value);
		break;
	case V4L2_CID_SCENEMODE:
#ifdef VIEW_FUNCTION_CALL
		printk("[S5k4CA] %s function V4L2_CID_SCENEMODE\n", __func__);
#endif

		err = s5k4ca_sensor_change_scene_mode(sd, ctrl->value);
		break;
	
//giridhar: brightness was added newly	
	case V4L2_CID_BRIGHTNESS:
#ifdef VIEW_FUNCTION_CALL
		printk("[S5k4CA] %s function V4L2_CID_BRIGHTNESS\n", __func__);
#endif
	
		err = s5k4ca_sensor_change_br(sd, ctrl->value);
		break;

	case V4L2_CID_CONTRAST:
#ifdef VIEW_FUNCTION_CALL
		printk("[S5k4CA] %s function V4L2_CID_CONTRAST\n", __func__);
#endif
	
		dev_dbg(&client->dev, "%s: V4L2_CID_CONTRAST\n", __func__);
		/*err = s5k4ba_write_regs(sd, \
			(unsigned char *) s5k4ba_regs_contrast_bias[ctrl->value], \
			sizeof(s5k4ba_regs_contrast_bias[ctrl->value]), "s5k4ba_regs_contrast_bias");
		*/
		err = s5k4ca_sensor_change_contrast(sd, ctrl->value);
		break;

	case V4L2_CID_SATURATION:
#ifdef VIEW_FUNCTION_CALL
		printk("[S5k4CA] %s function V4L2_CID_SATURATION\n", __func__);
#endif
	
		dev_dbg(&client->dev, "%s: V4L2_CID_SATURATION\n", __func__);
		/*err = s5k4ba_write_regs(sd, \
			(unsigned char *) s5k4ba_regs_saturation_bias[ctrl->value], \
			sizeof(s5k4ba_regs_saturation_bias[ctrl->value]), "s5k4ba_regs_saturation_bias");
		*/
		err = s5k4ca_sensor_change_saturation(sd, ctrl->value);
		break;

	case V4L2_CID_SHARPNESS:
#ifdef VIEW_FUNCTION_CALL
		printk("[S5k4CA] %s function V4L2_CID_SHARPNESS\n", __func__);
#endif
	
		dev_dbg(&client->dev, "%s: V4L2_CID_SHARPNESS\n", __func__);
		/*err = s5k4ba_write_regs(sd, \
			(unsigned char *) s5k4ba_regs_sharpness_bias[ctrl->value], \
			sizeof(s5k4ba_regs_sharpness_bias[ctrl->value]), "s5k4ba_regs_sharpness_bias");
		*/
		err = s5k4ca_sensor_change_sharpness(sd, ctrl->value);
		break;

//giridhar: the below cases are added newly
	case V4L2_CID_CAM_ISO:
#ifdef VIEW_FUNCTION_CALL
		printk("[S5k4CA] %s function V4L2_CID_ISO\n", __func__);
#endif

		err = s5k4ca_sensor_change_iso(sd, ctrl->value);
		break;
		
	case V4L2_CID_PHOTOMETRY:
#ifdef VIEW_FUNCTION_CALL
		printk("[S5k4CA] %s function V4L2_CID_PHOTOMETRY\n", __func__);
#endif
	
		err = s5k4ca_sensor_change_photometry(sd, ctrl->value);
		break;
		
	case V4L2_CID_EXIF_DATA:
#ifdef VIEW_FUNCTION_CALL
		printk("[S5k4CA] %s function V4L2_CID_EXIF_DATA\n", __func__);
#endif
	
		err = s5k4ca_sensor_exif_read(sd, ctrl->value);
		break;
			
	case V4L2_CID_FLASH_CAMERA:
#ifdef VIEW_FUNCTION_CALL
		printk("[S5k4CA] %s function V4L2_CID_FLASH_CAMERA\n", __func__);
#endif
	
		err = 0;
		/* Need GPIO information to set appropriate bits */
		break;
		
	
	default:
#ifdef VIEW_FUNCTION_CALL
		printk("[S5k4CA] %s function Default\n", __func__);
#endif

		dev_err(&client->dev, "%s: no such control\n", __func__);
		err = 0;
		break;
	}

	if (err < 0)
		goto out;
	else
		return 0;

out:
	dev_dbg(&client->dev, "%s: vidioc_s_ctrl failed\n", __func__);
	return err;
#else
	return 0;
#endif
}

extern unsigned int s5k4ca_skip_green_frame;

static int s5k4ca_init(struct v4l2_subdev *sd, u32 val)
{

	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = -EINVAL, i, count;


	s5k4ca_skip_green_frame = 0;

#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif

	v4l_info(client, "%s: camera initialization start\n", __func__);
	
#ifdef CONFIG_LOAD_FILE
	printk("[CDH] + s5k4ca_init\n");
	s5k4ca_regs_table_init();
	
	s5k4ba_write_regs(sd, s5k4ca_init0, S5K4CA_INIT0_REGS, "s5k4ca_init0");
	preview_in_init = 1;
       preview_in_init_af = 1;	
//	first_init_end = 1;
	msleep(100);

	s5k4ba_write_regs(sd, s5k4ca_init1, S5K4CA_INIT1_REGS, "s5k4ca_init1");
	
	s5k4ba_write_regs(sd, s5k4ca_preview, S5K4CA_PREVIEW_REGS, "s5k4ca_preview");
	printk("[CDH] - s5k4ca_init\n");
#else
	for (i = 0; i < S5K4CA_INIT0_REGS; i++) {
#ifdef S5K4CA_SKIP_GREEN_FRAMES
		err = s5k4ca_direct_write(client, s5k4ca_init0[i]);
#else
		err = s5k4ca_sensor_write(client, s5k4ca_init0[i].subaddr, \
						s5k4ca_init0[i].value);
#endif
		if (unlikely(err < 0))
		{
			v4l_info(client, "%s: register set failed, err = %d\n", \
				__func__, err);
			return err;
		}
	}
//	first_init_end = 1;
	msleep(100);		

	/* Arun c: We are not using the id */
//	s5k4ca_sensor_get_id(client);

#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line for operation is start!\n", __func__, __LINE__);
#endif

#ifdef S5K4CA_SKIP_GREEN_FRAMES
#ifdef  S5K4CA_USE_BURSTMODE
	s5k4ca_sensor_burst_write(client,s5k4ca_init1,S5K4CA_INIT1_REGS);
#else
	for (i = 0; i < S5K4CA_INIT1_REGS; i++) {	
		err = s5k4ca_direct_write(client, s5k4ca_init1[i]);

		if (unlikely(err < 0))
		{
			v4l_info(client, "%s: register set failed\n", \
				__func__);
			return err;
		}
	    }
#endif
#else
#ifdef  S5K4CA_USE_BURSTMODE
	printk("[S5k4CA] %s function %d line launched!burst start\n", __func__, __LINE__);
	s5k4ca_sensor_burst_write(client,s5k4ca_init1,S5K4CA_INIT1_REGS);
	printk("[S5k4CA] %s function %d line launched!\n burst end", __func__, __LINE__);
#else
	printk("[S5k4CA] %s function %d line launched!notmal start \n", __func__, __LINE__);
	for (i = 0, count = 0x400; i < S5K4CA_INIT1_REGS; i++, count--) {		
		err = s5k4ca_sensor_write(client, s5k4ca_init1[i].subaddr, \
					s5k4ca_init1[i].value);	
		/* Arun c: introducing a delay to avoid Audio stop */
		if (!count) {
			msleep(1);
			count = 0x400;
		}

		if (unlikely(err < 0))
		{
			v4l_info(client, "%s: register set failed\n", \
				__func__);
			return err;
		}
	    }
	printk("[S5k4CA] %s function %d line launched!normal end\n", __func__, __LINE__);
#endif		
#endif

	preview_in_init = 1;
       preview_in_init_af = 1;

	for (i = 0; i < S5K4CA_PREVIEW_REGS; i++) {
		err = s5k4ca_sensor_write(client, s5k4ca_preview[i].subaddr, \
						s5k4ca_preview[i].value);
		if (unlikely(err < 0))
		{
			v4l_info(client, "%s: register set failed\n", \
				__func__);	
			return err;
		}
	}

#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line for operation is end!\n", __func__, __LINE__);
#endif

#endif
#if 0 // CDH /* Arun C: Remove preview mode setting from here */
#ifndef CONFIG_LOAD_FILE
	
#if 1
	for (i = 0; i < S5K4CA_PREVIEW_REGS; i++) {
		err = s5k4ca_sensor_write(client, s5k4ca_preview[i].subaddr, \
						s5k4ca_preview[i].value);
		if (err < 0)
		v4l_info(client, "%s: register set failed\n", \
		__func__);	
		}
#else	
		for (i = 0; i < S5K4CA_PREVIEW_REGS; i++) {
			s5k4ca_sensor_write(client, s5k4ca_capture[i].subaddr, \
						s5k4ca_capture[i].value);
		}
#endif

#endif
#endif

	if (err < 0) {
		v4l_err(client, "%s: camera initialization failed\n", \
			__func__);
		return -EIO;	/* FIXME */
	}

	return 0;
}

/*
 * s_config subdev ops
 * With camera device, we need to re-initialize every single opening time therefor,
 * it is not necessary to be initialized on probe time. except for version checking
 * NOTE: version checking is optional
 */
static int s5k4ca_s_config(struct v4l2_subdev *sd, int irq, void *platform_data)
{

	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct s5k4ca_state *state = to_state(sd);
	struct s5k4ca_platform_data *pdata;
#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif

	dev_info(&client->dev, "fetching platform data\n");

	pdata = client->dev.platform_data;

	if (!pdata) {
		dev_err(&client->dev, "%s: no platform data\n", __func__);
		return -ENODEV;
	}

	/*
	 * Assign default format and resolution
	 * Use configured default information in platform data
	 * or without them, use default information in driver
	 */
	if (!(pdata->default_width && pdata->default_height)) {
		/* TODO: assign driver default resolution */
	} else {
		state->pix.width = pdata->default_width;
		state->pix.height = pdata->default_height;
	}

	if (!pdata->pixelformat)
		state->pix.pixelformat = DEFAULT_FMT;
	else
		state->pix.pixelformat = pdata->pixelformat;

	if (!pdata->freq)
		state->freq = 24000000;	/* 24MHz default */
	else
		state->freq = pdata->freq;

	if (!pdata->is_mipi) {
		state->is_mipi = 0;
		dev_info(&client->dev, "parallel mode\n");
	} else
		state->is_mipi = pdata->is_mipi;

	return 0;
}

static const struct v4l2_subdev_core_ops s5k4ca_core_ops = {
	.init = s5k4ca_init,	/* initializing API */
	.s_config = s5k4ca_s_config,	/* Fetch platform data */
	.queryctrl = s5k4ca_queryctrl,
	.querymenu = s5k4ca_querymenu,
	.g_ctrl = s5k4ca_g_ctrl,
	.s_ctrl = s5k4ca_s_ctrl,
};

static const struct v4l2_subdev_video_ops s5k4ca_video_ops = {
	.s_crystal_freq = s5k4ca_s_crystal_freq,
	.g_fmt = s5k4ca_g_fmt,
	.s_fmt = s5k4ca_s_fmt,
	.enum_framesizes = s5k4ca_enum_framesizes,
	.enum_frameintervals = s5k4ca_enum_frameintervals,
	.enum_fmt = s5k4ca_enum_fmt,
	.try_fmt = s5k4ca_try_fmt,
	.g_parm = s5k4ca_g_parm,
	.s_parm = s5k4ca_s_parm,

};

static const struct v4l2_subdev_ops s5k4ca_ops = {
	.core = &s5k4ca_core_ops,
	.video = &s5k4ca_video_ops,
};

/*
 * s5k4ca_probe
 * Fetching platform data is being done with s_config subdev call.
 * In probe routine, we just register subdev device
 */
static int s5k4ca_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct s5k4ca_state *state;
	struct v4l2_subdev *sd;
#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif

#ifdef S5P6442_POWER_GATING_CAM
	s5p6442_idle_pm_gpiocfg(S5P6442_CAM_ID, S5P6442_ACTIVE_MODE);
        s5p6442_pwrgate_config(S5P6442_CAM_ID, S5P6442_ACTIVE_MODE);
#endif

	state = kzalloc(sizeof(struct s5k4ca_state), GFP_KERNEL);
	if (state == NULL)
		return -ENOMEM;

	sd = &state->sd;
	strcpy(sd->name, S5K4CA_DRIVER_NAME);

	/* Registering subdev */
	v4l2_i2c_subdev_init(sd, client, &s5k4ca_ops);

#ifdef S5P6442_POWER_GATING_CAM
        s5p6442_pwrgate_config(S5P6442_CAM_ID, S5P6442_LP_MODE);
	s5p6442_idle_pm_gpiocfg(S5P6442_CAM_ID, S5P6442_LP_MODE);
#endif

	dev_info(&client->dev, "s5k4ca has been probed\n");
	return 0;
}


static int s5k4ca_remove(struct i2c_client *client)
{
#ifdef VIEW_FUNCTION_CALL	
	printk("[S5k4CA] %s function %d line launched!\n", __func__, __LINE__);
#endif

	struct v4l2_subdev *sd = i2c_get_clientdata(client);

	v4l2_device_unregister_subdev(sd);
	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id s5k4ca_id[] = {
	{ S5K4CA_DRIVER_NAME, 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, s5k4ca_id);

static struct v4l2_i2c_driver_data v4l2_i2c_data = {
	.name = S5K4CA_DRIVER_NAME,
	.probe = s5k4ca_probe,
	.remove = s5k4ca_remove,
	.id_table = s5k4ca_id,
};

MODULE_DESCRIPTION("Samsung Electronics S5K4CA UXGA camera driver");
MODULE_AUTHOR("Jinsung Yang <jsgood.yang@samsung.com>");
MODULE_LICENSE("GPL");



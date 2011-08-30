/*
 * linux/drivers/power/s5p6442_battery.c
 *
 * Battery measurement code for S5P6442 platform.
 *
 * based on palmtx_battery.c
 *
 * Copyright (C) 2009 Samsung Electronics.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/power_supply.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/irq.h>
#include <linux/wakelock.h>
#include <linux/i2c/max8998.h>
#include <asm/mach-types.h>
#include <mach/hardware.h>
#include <plat/gpio-cfg.h>

#include "s5p6442_battery.h"


static struct wake_lock vbus_wake_lock;
static struct wake_lock wake_lock_for_dev;
static struct wake_lock wake_lock_for_off;

/* Prototypes */
extern int s3c_adc_get_adc_data(int channel);
extern int get_usb_power_state(void);
extern int get_jig_cable_state(void);
extern int s3cfb_get_lcd_power(void);
extern int apollo_get_remapped_hw_rev_pin();

static ssize_t s3c_bat_show_property(struct device *dev,
                                      struct device_attribute *attr,
                                      char *buf);
static ssize_t s3c_bat_store(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t count);
static void s3c_set_chg_en(int enable);

#ifdef __CHECK_TSP_TEMP_NOTIFICATION__

static int detect_tsp_low_temp = 0;
static int start_noti_to_tsp = 0;
void set_start_check_temp_adc_for_tsp() {start_noti_to_tsp=1;}
#endif

#define ADC_DATA_ARR_SIZE	6
#define ADC_TOTAL_COUNT		100 //
#define POLLING_INTERVAL	4000 // 4000

#ifdef __BATTERY_COMPENSATION__
/* Offset Bit Value */
#define OFFSET_VIBRATOR_ON		(0x1 << 0)
#define OFFSET_CAMERA_ON		(0x1 << 1)
#define OFFSET_MP3_PLAY			(0x1 << 2)
#define OFFSET_VIDEO_PLAY		(0x1 << 3)
#define OFFSET_VOICE_CALL_2G		(0x1 << 4)
#define OFFSET_VOICE_CALL_3G		(0x1 << 5)
#define OFFSET_DATA_CALL		(0x1 << 6)
#define OFFSET_LCD_ON			(0x1 << 7)
#define OFFSET_TA_ATTACHED		(0x1 << 8)
#define OFFSET_CAM_FLASH		(0x1 << 9)
#define OFFSET_BOOTING			(0x1 << 10)
#endif /* __BATTERY_COMPENSATION__ */

#define INVALID_VOL_ADC		200

static struct work_struct bat_work;
static struct work_struct cable_work;
static struct device *dev;
static struct timer_list polling_timer;
static struct timer_list cable_timer;
static int cable_intr_cnt = 0;
static int has_temp_adc_channel = 0;
static int resume_from_sleep = 0;
extern int cable_status;
extern int get_vdcin_status(void);

static int s3c_battery_initial;
static int force_update;
static int full_charge_flag;
static int battery_cal_updated = 0;
static int low_batt_power_off = 0;

static int batt_max;
static int batt_off;

#ifdef __BATTERY_COMPENSATION__
static int batt_compensation;
#endif /* __BATTERY_COMPENSATION__ */

int call_state = 0;
int data_call = 0;
EXPORT_SYMBOL(data_call);
//extern void wm8994_data_call_hp_switch(void);

static unsigned int start_time_msec;
static unsigned int total_time_msec;

static char *status_text[] = {
	[POWER_SUPPLY_STATUS_UNKNOWN] =		"Unknown",
	[POWER_SUPPLY_STATUS_CHARGING] =	"Charging",
	[POWER_SUPPLY_STATUS_DISCHARGING] =	"Discharging",
	[POWER_SUPPLY_STATUS_NOT_CHARGING] =	"Not Charging",
	[POWER_SUPPLY_STATUS_FULL] =		"Full",
};

typedef enum {
	CHARGER_BATTERY = 0,
	CHARGER_USB,
	CHARGER_AC,
	CHARGER_DISCHARGE
} charger_type_t;

struct battery_info {
	u32 batt_id;		/* Battery ID from ADC */
	s32 batt_vol;		/* Battery voltage from ADC */
	s32 batt_vol_adc;	/* Battery ADC value */
	s32 batt_vol_adc_cal;	/* Battery ADC value (calibrated)*/
	s32 batt_temp;		/* Battery Temperature (C) from ADC */
	s32 batt_temp_adc;	/* Battery Temperature ADC value */
	s32 batt_temp_adc_cal;	/* Battery Temperature ADC value (calibrated) */
	s32 batt_current;	/* Battery current from ADC */
	u32 level;		/* formula */
	u32 charging_source;	/* 0: no cable, 1:usb, 2:AC */
	u32 charging_enabled;	/* 0: Disable, 1: Enable */
	u32 batt_health;	/* Battery Health (Authority) */
	u32 batt_is_full;       /* 0 : Not full 1: Full */
	u32 batt_is_recharging; /* 0 : Not recharging 1: Recharging */
	s32 batt_vol_adc_aver;	/* batt vol adc average */
};

/* lock to protect the battery info */
static DEFINE_MUTEX(work_lock);

struct s3c_battery_info {
	int present;
	int polling;
	unsigned int polling_interval;
	unsigned int device_state;

	struct battery_info bat_info;
};
static struct s3c_battery_info s3c_bat_info;

struct adc_sample_info {
	unsigned int cnt;
	int total_adc;
	int average_adc;
	int adc_arr[ADC_TOTAL_COUNT];
	int index;
};
static struct adc_sample_info adc_sample[ENDOFADC];


static int s3c_bat_get_adc_data(adc_channel_type adc_ch)
{
	int adc_arr[ADC_DATA_ARR_SIZE];
	int adc_max = 0;
	int adc_min = 0;
	int adc_total = 0;
	int i;

	for (i = 0; i < ADC_DATA_ARR_SIZE; i++) {
		adc_arr[i] = s3c_adc_get_adc_data(adc_ch);
		if(adc_arr[i] < 0)
		{
			printk("%s: ADC driver is unavailable (CH : %d)\n", __func__, adc_ch);
			return -1;
		}

		dev_dbg(dev, "%s: adc_arr = %d\n", __func__, adc_arr[i]);
		if (i != 0) {
			if (adc_arr[i] > adc_max)
				adc_max = adc_arr[i];
			else if (adc_arr[i] < adc_min)
				adc_min = adc_arr[i];
		} else {
			adc_max = adc_arr[0];
			adc_min = adc_arr[0];
		}
		adc_total += adc_arr[i];
	}

	dev_dbg(dev, "%s: adc_max = %d, adc_min = %d\n",
			__func__, adc_max, adc_min);
	return (adc_total - adc_max - adc_min) / (ADC_DATA_ARR_SIZE - 2);
}


static unsigned long s3c_read_temp(struct power_supply *bat_ps)
{
	int adc = 0;

	dev_dbg(dev, "%s\n", __func__);

#if 1  // For using thermal ADC
	if(has_temp_adc_channel)  // Above Rev0.6 H/W
		adc = s3c_bat_get_adc_data(S3C_ADC_TEMPERATURE);
	else
		adc = 1660;  // temp value
#else
	adc = 1660;  // temp value
#endif

	dev_dbg(dev, "%s: adc = %d\n", __func__, adc);

	s3c_bat_info.bat_info.batt_temp_adc = adc;

	return adc;
}


static int is_over_abs_time(void)
{
	unsigned int total_time;

	if (!start_time_msec)
		return 0;

	if (s3c_bat_info.bat_info.batt_is_recharging)
		total_time = TOTAL_RECHARGING_TIME;
	else
		total_time = TOTAL_CHARGING_TIME;

	total_time_msec = jiffies_to_msecs(jiffies) - start_time_msec;
	if (total_time_msec > total_time)
		return 1;
	else
		return 0;
}


#ifdef __BATTERY_COMPENSATION__
static void s3c_bat_set_compesation(int mode,
				    int offset,
				    int compensate_value)
{
	if (mode) {
		if (!(s3c_bat_info.device_state & offset)) {
			s3c_bat_info.device_state |= offset;
			batt_compensation += compensate_value;
		}
	} else {
		if (s3c_bat_info.device_state & offset) {
			s3c_bat_info.device_state &= ~offset;
			batt_compensation -= compensate_value;
		}
	}
//	printk("%s: device_state=0x%x, compensation=%d\n", __func__,
//			s3c_bat_info.device_state, batt_compensation);
}
#endif /* __BATTERY_COMPENSATION__ */

static unsigned long calculate_average_adc(adc_channel_type channel, int adc)
{
	unsigned int cnt = 0;
	int total_adc = 0;
	int average_adc = 0;
	int index = 0;
	int i = 0;

	cnt = adc_sample[channel].cnt;
	total_adc = adc_sample[channel].total_adc;

	if (adc < 0 || adc == 0) {
		dev_err(dev, "%s: invalid adc : %d\n", __func__, adc);
		adc = adc_sample[channel].average_adc;
	}

	if( cnt < ADC_TOTAL_COUNT ) {
		adc_sample[channel].adc_arr[cnt] = adc;
		adc_sample[channel].index = cnt;
		adc_sample[channel].cnt = ++cnt;

		total_adc += adc;
		average_adc = total_adc / cnt;
	} else {

		if(resume_from_sleep)  // Wakeup from sleep
		{
			index = adc_sample[channel].index;
			i = 0;

			while(i < 10)  // push new adc to adc_sample 10 times (adjust new adc)
			{
				if (++index >= ADC_TOTAL_COUNT)
					index = 0;

				total_adc = (total_adc - adc_sample[channel].adc_arr[index]) + adc;
				average_adc = total_adc / ADC_TOTAL_COUNT;

				adc_sample[channel].adc_arr[index] = adc;
				adc_sample[channel].index = index;

				i++;
			}
			resume_from_sleep = 0;  // clear flag
		}
		else  // Normal case
		{
			index = adc_sample[channel].index;
			if (++index >= ADC_TOTAL_COUNT)
				index = 0;

			total_adc = (total_adc - adc_sample[channel].adc_arr[index]) + adc;
			average_adc = total_adc / ADC_TOTAL_COUNT;

			adc_sample[channel].adc_arr[index] = adc;
			adc_sample[channel].index = index;
		}
	}

	adc_sample[channel].total_adc = total_adc;
	adc_sample[channel].average_adc = average_adc;

	dev_dbg(dev, "%s: ch:%d adc=%d, avg_adc=%d\n",
			__func__, channel, adc, average_adc);
	return average_adc;
}

static int p_count = 0;
static unsigned long s3c_read_bat(struct power_supply *bat_ps)
{
	int adc = 0;
	static int cnt = 0;
	int chg_adc = 0;

	dev_dbg(dev, "%s\n", __func__);

	adc = s3c_bat_get_adc_data(S3C_ADC_VOLTAGE);
	chg_adc = s3c_bat_get_adc_data(S3C_ADC_CHG_CURRENT);
	if(adc < 0 || chg_adc < 0)
	{
		printk("%s: ADC value is invalid!!\n", __func__);
		return -1;
	}

	dev_dbg(dev, "%s: adc = %d\n", __func__, adc);

#ifndef __BATTERY_COMPENSATION__
	if(s3cfb_get_lcd_power())  // temp : compensation for lcd
		adc += 15;
#endif

	if(cable_status && s3c_bat_info.bat_info.charging_enabled)  // TA or USB cable (not jig)
		{
		int diff = ((chg_adc * 1666) / 10);
		adc -= ((diff / 1000) + (((diff % 1000) > 500) ? 1 : 0));
		}

#ifdef __BATTERY_COMPENSATION__
	adc += batt_compensation;
#endif /* __BATTERY_COMPENSATION__ */
	if (adc < s3c_bat_info.bat_info.batt_vol_adc_aver - INVALID_VOL_ADC
			&& cnt < 10 && resume_from_sleep != 1) {
		dev_err(dev, "%s: invaild adc = %d\n", __func__, adc);
		adc = s3c_bat_info.bat_info.batt_vol_adc_aver;
		cnt++;
	} else {
		cnt = 0;
	}
	s3c_bat_info.bat_info.batt_vol_adc =  adc;

	return calculate_average_adc(S3C_ADC_VOLTAGE, adc);
}

void set_low_batt_flag(void)
{
	// Set low battery flag
	low_batt_power_off = 1;

	// Wait for 2 seconds
	wake_lock_timeout(&wake_lock_for_off, 2 * HZ);
}
EXPORT_SYMBOL(set_low_batt_flag);

static int s3c_get_bat_level(struct power_supply *bat_ps)
{
	int bat_level = 0;
	int bat_vol = s3c_read_bat(bat_ps);

	if(bat_vol < 0)
	{
		printk("%s: Read battery ADC failed!!\n", __func__);
		return -1;
	}

	s3c_bat_info.bat_info.batt_vol_adc_aver = bat_vol;

	if(is_over_abs_time()) {
		bat_level = 100;
		dev_info(dev, "%s: charging time is over\n", __func__);
		s3c_set_chg_en(0);
		s3c_bat_info.bat_info.batt_is_full = 1;
		goto __end__;
	}

	if(!get_jig_cable_state() && low_batt_power_off)  // Low batt interrupt occured
	{
		bat_level = 0;  // Now, phone will be shutdown
		dev_info(dev, "%s: power off by low battery\n", __func__);
		goto __end__;
	}

	
	if (bat_vol >= batt_max)
	{
	bat_level = 100;
	}
	else
	if (bat_vol < batt_max && bat_vol > batt_off)
	{
		int temp = (batt_max - batt_off) / 100;
		if (bat_vol > (batt_off + (temp * 99)))
			bat_level = 99;
		else if (bat_vol > (batt_off + (temp * 98)))
			bat_level = 98;
		else if (bat_vol > (batt_off + (temp * 97)))
			bat_level = 97;
		else if (bat_vol > (batt_off + (temp * 96)))
			bat_level = 96;
		else if (bat_vol > (batt_off + (temp * 95)))
			bat_level = 95;
		else if (bat_vol > (batt_off + (temp * 94)))
			bat_level = 94;
		else if (bat_vol > (batt_off + (temp * 93)))
			bat_level = 93;
		else if (bat_vol > (batt_off + (temp * 92)))
			bat_level = 92;
		else if (bat_vol > (batt_off + (temp * 91)))
			bat_level = 91;
		else if (bat_vol > (batt_off + (temp * 90)))
			bat_level = 90;
		else if (bat_vol > (batt_off + (temp * 89)))
			bat_level = 89;
		else if (bat_vol > (batt_off + (temp * 88)))
			bat_level = 88;
		else if (bat_vol > (batt_off + (temp * 87)))
			bat_level = 87;
		else if (bat_vol > (batt_off + (temp * 86)))
			bat_level = 86;
		else if (bat_vol > (batt_off + (temp * 85)))
			bat_level = 85;
		else if (bat_vol > (batt_off + (temp * 84)))
			bat_level = 84;
		else if (bat_vol > (batt_off + (temp * 83)))
			bat_level = 83;
		else if (bat_vol > (batt_off + (temp * 82)))
			bat_level = 82;
		else if (bat_vol > (batt_off + (temp * 81)))
			bat_level = 81;
		else if (bat_vol > (batt_off + (temp * 80)))
			bat_level = 80;
		else if (bat_vol > (batt_off + (temp * 79)))
			bat_level = 79;
		else if (bat_vol > (batt_off + (temp * 78)))
			bat_level = 78;
		else if (bat_vol > (batt_off + (temp * 77)))
			bat_level = 77;
		else if (bat_vol > (batt_off + (temp * 76)))
			bat_level = 76;
		else if (bat_vol > (batt_off + (temp * 75)))
			bat_level = 75;
		else if (bat_vol > (batt_off + (temp * 74)))
			bat_level = 74;
		else if (bat_vol > (batt_off + (temp * 73)))
			bat_level = 73;
		else if (bat_vol > (batt_off + (temp * 72)))
			bat_level = 72;
		else if (bat_vol > (batt_off + (temp * 71)))
			bat_level = 71;
		else if (bat_vol > (batt_off + (temp * 70)))
			bat_level = 70;
		else if (bat_vol > (batt_off + (temp * 69)))
			bat_level = 69;
		else if (bat_vol > (batt_off + (temp * 68)))
			bat_level = 68;
		else if (bat_vol > (batt_off + (temp * 67)))
			bat_level = 67;
		else if (bat_vol > (batt_off + (temp * 66)))
			bat_level = 66;
		else if (bat_vol > (batt_off + (temp * 65)))
			bat_level = 65;
		else if (bat_vol > (batt_off + (temp * 64)))
			bat_level = 64;
		else if (bat_vol > (batt_off + (temp * 63)))
			bat_level = 63;
		else if (bat_vol > (batt_off + (temp * 62)))
			bat_level = 62;
		else if (bat_vol > (batt_off + (temp * 61)))
			bat_level = 61;
		else if (bat_vol > (batt_off + (temp * 60)))
			bat_level = 60;
		else if (bat_vol > (batt_off + (temp * 59)))
			bat_level = 59;
		else if (bat_vol > (batt_off + (temp * 58)))
			bat_level = 58;
		else if (bat_vol > (batt_off + (temp * 57)))
			bat_level = 57;
		else if (bat_vol > (batt_off + (temp * 56)))
			bat_level = 56;
		else if (bat_vol > (batt_off + (temp * 55)))
			bat_level = 55;
		else if (bat_vol > (batt_off + (temp * 54)))
			bat_level = 54;
		else if (bat_vol > (batt_off + (temp * 53)))
			bat_level = 53;
		else if (bat_vol > (batt_off + (temp * 52)))
			bat_level = 52;
		else if (bat_vol > (batt_off + (temp * 51)))
			bat_level = 51;
		else if (bat_vol > (batt_off + (temp * 50)))
			bat_level = 50;
		else if (bat_vol > (batt_off + (temp * 49)))
			bat_level = 49;
		else if (bat_vol > (batt_off + (temp * 48)))
			bat_level = 48;
		else if (bat_vol > (batt_off + (temp * 47)))
			bat_level = 47;
		else if (bat_vol > (batt_off + (temp * 46)))
			bat_level = 46;
		else if (bat_vol > (batt_off + (temp * 45)))
			bat_level = 45;
		else if (bat_vol > (batt_off + (temp * 44)))
			bat_level = 44;
		else if (bat_vol > (batt_off + (temp * 43)))
			bat_level = 43;
		else if (bat_vol > (batt_off + (temp * 42)))
			bat_level = 42;
		else if (bat_vol > (batt_off + (temp * 41)))
			bat_level = 41;
		else if (bat_vol > (batt_off + (temp * 40)))
			bat_level = 40;
		else if (bat_vol > (batt_off + (temp * 39)))
			bat_level = 39;
		else if (bat_vol > (batt_off + (temp * 38)))
			bat_level = 38;
		else if (bat_vol > (batt_off + (temp * 37)))
			bat_level = 37;
		else if (bat_vol > (batt_off + (temp * 36)))
			bat_level = 36;
		else if (bat_vol > (batt_off + (temp * 35)))
			bat_level = 35;
		else if (bat_vol > (batt_off + (temp * 34)))
			bat_level = 34;
		else if (bat_vol > (batt_off + (temp * 33)))
			bat_level = 33;
		else if (bat_vol > (batt_off + (temp * 32)))
			bat_level = 32;
		else if (bat_vol > (batt_off + (temp * 31)))
			bat_level = 31;
		else if (bat_vol > (batt_off + (temp * 30)))
			bat_level = 30;
		else if (bat_vol > (batt_off + (temp * 29)))
			bat_level = 29;
		else if (bat_vol > (batt_off + (temp * 28)))
			bat_level = 28;
		else if (bat_vol > (batt_off + (temp * 27)))
			bat_level = 27;
		else if (bat_vol > (batt_off + (temp * 26)))
			bat_level = 26;
		else if (bat_vol > (batt_off + (temp * 25)))
			bat_level = 25;
		else if (bat_vol > (batt_off + (temp * 24)))
			bat_level = 24;
		else if (bat_vol > (batt_off + (temp * 23)))
			bat_level = 23;
		else if (bat_vol > (batt_off + (temp * 22)))
			bat_level = 22;
		else if (bat_vol > (batt_off + (temp * 21)))
			bat_level = 21;
		else if (bat_vol > (batt_off + (temp * 20)))
			bat_level = 20;
		else if (bat_vol > (batt_off + (temp * 19)))
			bat_level = 19;
		else if (bat_vol > (batt_off + (temp * 18)))
			bat_level = 18;
		else if (bat_vol > (batt_off + (temp * 17)))
			bat_level = 17;
		else if (bat_vol > (batt_off + (temp * 16)))
			bat_level = 16;
		else if (bat_vol > (batt_off + (temp * 15)))
			bat_level = 15;
		else if (bat_vol > (batt_off + (temp * 14)))
			bat_level = 14;
		else if (bat_vol > (batt_off + (temp * 13)))
			bat_level = 13;
		else if (bat_vol > (batt_off + (temp * 12)))
			bat_level = 12;
		else if (bat_vol > (batt_off + (temp * 11)))
			bat_level = 11;
		else if (bat_vol > (batt_off + (temp * 10)))
			bat_level = 10;
		else if (bat_vol > (batt_off + (temp * 9)))
			bat_level = 9;
		else if (bat_vol > (batt_off + (temp * 8)))
			bat_level = 8;
		else if (bat_vol > (batt_off + (temp * 7)))
			bat_level = 7;
		else if (bat_vol > (batt_off + (temp * 6)))
			bat_level = 6;
		else if (bat_vol > (batt_off + (temp * 5)))
			bat_level = 5;
		else if (bat_vol > (batt_off + (temp * 4)))
			bat_level = 4;
		else if (bat_vol > (batt_off + (temp * 3)))
			bat_level = 3;
		else if (bat_vol > (batt_off + (temp * 2)))
			bat_level = 2;
		else if (bat_vol > (batt_off + (temp * 1)))
			bat_level = 1;
		else if (bat_vol > (batt_off + (temp * 0)))
			bat_level = 0;
	}
	else
	if (batt_off >= bat_vol)
	{
	bat_level = 0;
	}
	printk("----> Battery : bat_vol = %d batt_level : %d batt_max : %d batt_off : %d\n", bat_vol, bat_level, batt_max, batt_off );
	/*
	if (bat_vol > batt_full)
	{
		int temp = (batt_max - batt_full);
		if (bat_vol > (batt_full + temp) ||
				s3c_bat_info.bat_info.batt_is_full)
			bat_level = 100;
		else
		{
		        bat_level = 90 + (((bat_vol - batt_full) *10) / temp);
		}

		dev_dbg(dev, "%s: (full)level = %d\n", __func__, bat_level );
	}
	else if (batt_full >= bat_vol && bat_vol > batt_almost)
	{
		int temp = (batt_full - batt_almost) / 2;
		bat_level = 70 + (((bat_vol - batt_almost) *10) / temp);

		dev_dbg(dev, "%s: (almost)level = %d\n", __func__, bat_level);
	}
	else if (batt_almost >= bat_vol && bat_vol > batt_high)
	{
		int temp = (batt_almost - batt_high) / 2;
		bat_level = 50 + (((bat_vol - batt_high) *10) / temp);
		dev_dbg(dev, "%s: (high)level = %d\n", __func__, bat_level );
	}
	else if (batt_high >= bat_vol && bat_vol > batt_medium)
	{
		int temp = (batt_high - batt_medium) / 2;
		bat_level = 30 + (((bat_vol - batt_medium) *10) / temp);
		dev_dbg(dev, "%s: (med)level = %d\n", __func__, bat_level);
	}
	else if (batt_medium >= bat_vol && bat_vol > batt_low)
	{
		int temp = (batt_medium - batt_low) / 2;
		bat_level = 15 + ((( bat_vol - batt_low) *5) / temp);
		dev_dbg(dev, "%s: (low)level = %d\n", __func__, bat_level);
	}
	else if (batt_low >= bat_vol && bat_vol > batt_critical)
	{
		bat_level = 5;
		dev_dbg(dev, "%s: (cri)level = %d, vol = %d\n", __func__,
				bat_level, bat_vol);
	}
	else if (batt_critical >= bat_vol && bat_vol > batt_min)
	{
		bat_level = 3;
		dev_info(dev, "%s: (min)level = %d, vol = %d\n", __func__,
				bat_level, bat_vol);
	}
	else if (batt_min >= bat_vol && bat_vol > batt_off)
	{
		bat_level = 1;
		dev_info(dev, "%s: (off)level = %d, vol = %d\n", __func__,
				bat_level, bat_vol);
	}
	else if (batt_off >= bat_vol)
	{
		bat_level = 0;
		dev_info(dev, "%s: (off)level = %d, vol = %d", __func__,
				bat_level, bat_vol);
	}
	*/
	// If current status is full or recharging, then it should be 100% regardless of current real battery level.
	if (s3c_bat_info.bat_info.batt_is_full || s3c_bat_info.bat_info.batt_is_recharging)
		bat_level = 100;

	if( (++p_count % 150) == 0) {  // Print debug message every 5 minutes.
		p_count = 1;
		printk("[BATT] level(%d), is_full(%d), is_recharging(%d), charging_enabled(%d), batt_vol(%d)\n",
			bat_level, s3c_bat_info.bat_info.batt_is_full, s3c_bat_info.bat_info.batt_is_recharging,
			s3c_bat_info.bat_info.charging_enabled, bat_vol);
	}

	// If current status is full because of absolute timer, then it should be recharging.
	if (s3c_bat_info.bat_info.batt_is_full &&
		!s3c_bat_info.bat_info.charging_enabled &&
		bat_vol < (batt_max + 45)) {  // under 4.15V
		dev_info(dev, "%s: recharging(under full)\n", __func__);
		s3c_bat_info.bat_info.batt_is_recharging = 1;
		s3c_set_chg_en(1);
		bat_level = 100;
	}

	dev_dbg(dev, "%s: level = %d\n", __func__, bat_level);

    //bat_level = tmpcal;

__end__:
	dev_dbg(dev, "%s: bat_vol = %d, level = %d, is_full = %d\n",
			__func__, bat_vol, bat_level,
			s3c_bat_info.bat_info.batt_is_full);

	return bat_level;
}

static int s3c_get_bat_vol(struct power_supply *bat_ps)
{
	int batt_adc = s3c_bat_info.bat_info.batt_vol_adc_aver;
	int batt_vol = 0;
	int batt_mon_vol = 0;
	int compensation = 0;

	// check difference BATT_CAL and real value (ADC)
#if 0
	if(s3c_bat_info.bat_info.batt_vol_adc_cal != 0)
		compensation = BATT_CAL - s3c_bat_info.bat_info.batt_vol_adc_cal;
	else
		compensation = -(VOLTAGE_CAL);
#endif

	batt_mon_vol = (3300 * (batt_adc + compensation)) / 4095;
	if( ((3300 * batt_adc) % 4095) >= 2048 )
		batt_mon_vol++;

	batt_vol = (((batt_mon_vol + 896) * 1000) / 1083) + 1249;
	if(batt_vol >= 4200)
		batt_vol = 4200;


	dev_dbg(dev, "%s: adc = %d, bat_vol = %d\n", __func__, batt_adc, batt_vol);

	return batt_vol;
}


static u32 s3c_get_bat_health(void)
{
	return s3c_bat_info.bat_info.batt_health;
}


static void s3c_set_bat_health(u32 batt_health)
{
	s3c_bat_info.bat_info.batt_health = batt_health;
}


static inline int gpio_get_value_ex(unsigned int pin)
{
	return gpio_get_value(pin);
}


static inline void gpio_set_value_ex(unsigned int pin, unsigned int level)
{
	gpio_set_value(pin, level);
}


static void s3c_set_time_for_charging(int mode) {
	if (mode) {
		/* record start time for abs timer */
		start_time_msec = jiffies_to_msecs(jiffies);
		dev_info(dev, "%s: start_time(%d)\n", __func__,
				start_time_msec);
	} else {
		/* initialize start time for abs timer */
		start_time_msec = 0;
		total_time_msec = 0;
		dev_info(dev, "%s: reset abs timer\n", __func__);
	}
}


static void s3c_set_chg_en(int enable)
{
#ifdef __USING_MAX8998_CHARGER__
	// Enable CHGENB bit of MAX8998
	if (enable)
	{
		// enable
		Set_MAX8998_PM_REG(CHGENB, 0);
		s3c_set_time_for_charging(1);
	}
	else
	{
		// disable
		Set_MAX8998_PM_REG(CHGENB, 1);
		s3c_set_time_for_charging(0);
		s3c_bat_info.bat_info.batt_is_recharging = 0;
	}
#else /* __USING_MAX8998_CHARGER__ */
	int chg_en_val = gpio_get_value_ex(gpio_chg_en);

	if (enable) {
		if (chg_en_val == GPIO_LEVEL_HIGH) {
			gpio_set_value_ex(gpio_chg_en, GPIO_LEVEL_LOW);
			dev_info(dev, "%s: gpio_chg_en(0)\n", __func__);
			s3c_set_time_for_charging(1);
#if 0 //def __BATTERY_COMPENSATION__
			s3c_bat_set_compesation(1, OFFSET_TA_ATTACHED,
					COMPENSATE_TA);
#endif /* __BATTERY_COMPENSATION__ */
		}
	} else {
		if (chg_en_val == GPIO_LEVEL_LOW) {
			gpio_set_value_ex(gpio_chg_en, GPIO_LEVEL_HIGH);
			dev_info(dev, "%s: gpio_chg_en(1)\n", __func__);
			s3c_set_time_for_charging(0);
			s3c_bat_info.bat_info.batt_is_recharging = 0;
#if 0 //def __BATTERY_COMPENSATION__
			s3c_bat_set_compesation(0, OFFSET_TA_ATTACHED,
					COMPENSATE_TA);
#endif /* __BATTERY_COMPENSATION__ */
		}
	}
#endif /* __USING_MAX8998_CHARGER__ */
	s3c_bat_info.bat_info.charging_enabled = enable;
}


static void s3c_temp_control(int mode) {

	switch (mode) {
	case POWER_SUPPLY_HEALTH_GOOD:
		dev_info(dev, "%s: GOOD\n", __func__);
		s3c_set_bat_health(mode);
		break;
	case POWER_SUPPLY_HEALTH_OVERHEAT:
		dev_info(dev, "%s: OVERHEAT\n", __func__);
		s3c_set_bat_health(mode);
		break;
	case POWER_SUPPLY_HEALTH_COLD:
		dev_info(dev, "%s: COLD\n", __func__);
		s3c_set_bat_health(mode);
		break;
	default:
		break;
	}
	schedule_work(&cable_work);
}


static int s3c_get_bat_temp(struct power_supply *bat_ps)
{
	int temp = 0;
	int array_size = 0;
	int i = 0;

	int temp_adc = s3c_read_temp(bat_ps);
	if(temp_adc < 0)
	{
		printk("%s: Read temp ADC failed!!\n", __func__);
		return -EINVAL;
	}

	int health = s3c_get_bat_health();

	// No exceptional state needed (iks.kim)
#if 0 //def __BATTERY_COMPENSATION__
	unsigned int ex_case = 0;
#endif /* __BATTERY_COMPENSATION__ */

#if 0 //def __BATTERY_COMPENSATION__
	ex_case = OFFSET_MP3_PLAY | OFFSET_VOICE_CALL_2G | OFFSET_VOICE_CALL_3G
		| OFFSET_DATA_CALL | OFFSET_VIDEO_PLAY;
	if (s3c_bat_info.device_state & ex_case)
		goto __map_temperature__;
#endif /* __BATTERY_COMPENSATION__ */

	// Apollo has different thermister form used before
	if (temp_adc >= TEMP_HIGH_BLOCK) {
		if (health != POWER_SUPPLY_HEALTH_OVERHEAT &&
				health != POWER_SUPPLY_HEALTH_UNSPEC_FAILURE)
			s3c_temp_control(POWER_SUPPLY_HEALTH_OVERHEAT);
	} else if (temp_adc <= TEMP_HIGH_RECOVER &&
			temp_adc >= TEMP_LOW_RECOVER) {
		if (health == POWER_SUPPLY_HEALTH_OVERHEAT ||
				health == POWER_SUPPLY_HEALTH_COLD)
			s3c_temp_control(POWER_SUPPLY_HEALTH_GOOD);
	} else if (temp_adc <= TEMP_LOW_BLOCK) {
		if (health != POWER_SUPPLY_HEALTH_COLD &&
				health != POWER_SUPPLY_HEALTH_UNSPEC_FAILURE)
			s3c_temp_control(POWER_SUPPLY_HEALTH_COLD);
	}

#ifdef __CHECK_TSP_TEMP_NOTIFICATION__
	// Apollo TSP needs notification from temperature change.
	if(start_noti_to_tsp)
		{
		if(temp_adc >= TSP_TEMP_HIGH_ENTRY)
			{
			if(-1 < detect_tsp_low_temp)
				{
				printk(KERN_INFO "detect high temp for TSP\n");
				//qt602240_detect_low_temp(-1);
				detect_tsp_low_temp = -1;
				}
			}
		else if(temp_adc >= TSP_TEMP_LOW_CLEAR)
			{
			if(0 < detect_tsp_low_temp)
				{
				printk(KERN_INFO "release low temp for TSP\n");
				//qt602240_detect_low_temp(0);
				detect_tsp_low_temp = 0;
				}
			}

		if(temp_adc <= TSP_TEMP_LOW_ENTRY)
			{
			if(1 > detect_tsp_low_temp)
				{
				printk(KERN_INFO "detect low temp for TSP\n");
				//qt602240_detect_low_temp(1);
				detect_tsp_low_temp = 1;
				}
			}
		else if(temp_adc <= TSP_TEMP_HIGH_CLEAR)
			{
			if(0 > detect_tsp_low_temp)
				{
				printk(KERN_INFO "release high temp for TSP\n");
				//qt602240_detect_low_temp(0);
				detect_tsp_low_temp = 0;
				}
			}
		}

#endif

#if 0 //def __BATTERY_COMPENSATION__
__map_temperature__:
#endif /* __BATTERY_COMPENSATION__ */
	array_size = ARRAY_SIZE(temper_table);
	for (i = 0; i < (array_size - 1); i++) {
		if (i == 0) {
			if (temp_adc <= temper_table[0][0]) {
				temp = temper_table[0][1];
				break;
			} else if (temp_adc >= temper_table[array_size-1][0]) {
				temp = temper_table[array_size-1][1];
				break;
			}
		}

		if (temper_table[i+1][0] >= temp_adc &&
			temper_table[i][0] < temp_adc) {
			temp = temper_table[i][1];
		}
	}

	dev_dbg(dev, "%s: temp = %d, adc = %d\n", __func__, temp, temp_adc);

	return temp;
}


static int s3c_bat_get_charging_status(void)
{
        charger_type_t charger = CHARGER_BATTERY;
        int ret = 0;

        charger = s3c_bat_info.bat_info.charging_source;

        switch (charger) {
        case CHARGER_BATTERY:
                ret = POWER_SUPPLY_STATUS_NOT_CHARGING;
                break;
        case CHARGER_USB:
        case CHARGER_AC:
                if (s3c_bat_info.bat_info.batt_is_full)
                        ret = POWER_SUPPLY_STATUS_FULL;
		else
                        ret = POWER_SUPPLY_STATUS_CHARGING;
                break;
	case CHARGER_DISCHARGE:
		ret = POWER_SUPPLY_STATUS_DISCHARGING;
		break;
        default:
                ret = POWER_SUPPLY_STATUS_UNKNOWN;
        }

	dev_dbg(dev, "%s: %s\n", __func__, status_text[ret]);
        return ret;
}


static int s3c_bat_get_property(struct power_supply *bat_ps,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	dev_dbg(bat_ps->dev, "%s: psp = %d\n", __func__, psp);

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = s3c_bat_get_charging_status();
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = s3c_get_bat_health();
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = s3c_bat_info.present;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = s3c_bat_info.bat_info.level;
		dev_dbg(dev, "%s: level = %d\n", __func__, val->intval);
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = s3c_bat_info.bat_info.batt_temp;
		dev_dbg(bat_ps->dev, "%s: temp = %d\n", __func__, val->intval);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}


static int s3c_power_get_property(struct power_supply *bat_ps,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	charger_type_t charger;

	dev_dbg(bat_ps->dev, "%s: psp = %d\n", __func__, psp);

	charger = s3c_bat_info.bat_info.charging_source;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		if (bat_ps->type == POWER_SUPPLY_TYPE_MAINS)
			val->intval = (charger == CHARGER_AC ? 1 : 0);
		else if (bat_ps->type == POWER_SUPPLY_TYPE_USB)
			val->intval = (charger == CHARGER_USB ? 1 : 0);
		else
			val->intval = 0;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}


#define SEC_BATTERY_ATTR(_name)								\
{											\
        .attr = { .name = #_name, .mode = S_IRUGO | S_IWUGO, .owner = THIS_MODULE },	\
        .show = s3c_bat_show_property,							\
        .store = s3c_bat_store,								\
}

static struct device_attribute s3c_battery_attrs[] = {
        SEC_BATTERY_ATTR(batt_vol),
        SEC_BATTERY_ATTR(batt_vol_adc),
        SEC_BATTERY_ATTR(batt_vol_adc_cal),
        SEC_BATTERY_ATTR(batt_temp),
        SEC_BATTERY_ATTR(batt_temp_adc),
        SEC_BATTERY_ATTR(batt_temp_adc_cal),
	SEC_BATTERY_ATTR(batt_vol_adc_aver),
	SEC_BATTERY_ATTR(charging_source),
#ifdef __BATTERY_COMPENSATION__
	SEC_BATTERY_ATTR(vibrator),
	SEC_BATTERY_ATTR(camera),
	SEC_BATTERY_ATTR(mp3),
	SEC_BATTERY_ATTR(video),
	SEC_BATTERY_ATTR(talk_gsm),
	SEC_BATTERY_ATTR(talk_wcdma),
	SEC_BATTERY_ATTR(data_call),
	SEC_BATTERY_ATTR(device_state),
	SEC_BATTERY_ATTR(batt_compensation),
	SEC_BATTERY_ATTR(is_booting),
#endif /* __BATTERY_COMPENSATION__ */
};

enum {
        BATT_VOL = 0,
        BATT_VOL_ADC,
        BATT_VOL_ADC_CAL,
        BATT_TEMP,
        BATT_TEMP_ADC,
        BATT_TEMP_ADC_CAL,
	BATT_VOL_ADC_AVER,
	BATT_CHARGING_SOURCE,
#ifdef __BATTERY_COMPENSATION__
	BATT_VIBRATOR,
	BATT_CAMERA,
	BATT_MP3,
	BATT_VIDEO,
	BATT_VOICE_CALL_2G,
	BATT_VOICE_CALL_3G,
	BATT_DATA_CALL,
	BATT_DEV_STATE,
	BATT_COMPENSATION,
	BATT_BOOTING,
#endif /* __BATTERY_COMPENSATION__ */
};


static int s3c_bat_create_attrs(struct device * dev)
{
        int i, rc;

        for (i = 0; i < ARRAY_SIZE(s3c_battery_attrs); i++) {
                rc = device_create_file(dev, &s3c_battery_attrs[i]);
                if (rc)
                        goto s3c_attrs_failed;
        }
        goto succeed;

s3c_attrs_failed:
        while (i--)
                device_remove_file(dev, &s3c_battery_attrs[i]);
succeed:
        return rc;
}


static ssize_t s3c_bat_show_property(struct device *dev,
                                      struct device_attribute *attr,
                                      char *buf)
{
        int i = 0;
        const ptrdiff_t off = attr - s3c_battery_attrs;

        switch (off) {
        case BATT_VOL:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                               s3c_bat_info.bat_info.batt_vol);
                break;
        case BATT_VOL_ADC:
		s3c_bat_info.bat_info.batt_vol_adc =
			s3c_bat_get_adc_data(S3C_ADC_VOLTAGE);

                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                               s3c_bat_info.bat_info.batt_vol_adc);
                break;
        case BATT_VOL_ADC_CAL:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                               s3c_bat_info.bat_info.batt_vol_adc_cal);
                break;
        case BATT_TEMP:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                               s3c_bat_info.bat_info.batt_temp);
                break;
        case BATT_TEMP_ADC:
/*
		s3c_bat_info.bat_info.batt_temp_adc =
			s3c_bat_get_adc_data(S3C_ADC_TEMPERATURE);
*/
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                               s3c_bat_info.bat_info.batt_temp_adc);
                break;
        case BATT_TEMP_ADC_CAL:
                i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
                               s3c_bat_info.bat_info.batt_temp_adc_cal);
                break;
        case BATT_VOL_ADC_AVER:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			s3c_bat_info.bat_info.batt_vol_adc_aver);
		break;
	case BATT_CHARGING_SOURCE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			s3c_bat_info.bat_info.charging_source);
		break;
#ifdef __BATTERY_COMPENSATION__
	case BATT_DEV_STATE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "0x%08x\n",
			s3c_bat_info.device_state);
		break;
	case BATT_COMPENSATION:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n",
			batt_compensation);
		break;
#endif /* __BATTERY_COMPENSATION__ */

        default:
                i = -EINVAL;
        }

        return i;
}


static void s3c_bat_set_vol_cal(int batt_cal)
{
	int max_cal = 4096;

	if (!batt_cal)
		return;

	if (batt_cal >= max_cal) {
		dev_err(dev, "%s: invalid battery_cal(%d)\n", __func__, batt_cal);
		return;
	}

	batt_max = batt_cal + BATT_MAXIMUM;
	batt_off = batt_cal + BATT_OFF;

	// Set cal update flag.
	battery_cal_updated = 1;
}


static ssize_t s3c_bat_store(struct device *dev,
			     struct device_attribute *attr,
			     const char *buf, size_t count)
{
	int x = 0;
	int ret = 0;
	const ptrdiff_t off = attr - s3c_battery_attrs;

        switch (off) {
        case BATT_VOL_ADC_CAL:
		if (sscanf(buf, "%d\n", &x) == 1) {
			s3c_bat_info.bat_info.batt_vol_adc_cal = x;
			s3c_bat_set_vol_cal(x);
			ret = count;
		}
		dev_info(dev, "%s: batt_vol_adc_cal = %d\n", __func__, x);
                break;
        case BATT_TEMP_ADC_CAL:
		if (sscanf(buf, "%d\n", &x) == 1) {
			s3c_bat_info.bat_info.batt_temp_adc_cal = x;
			ret = count;
		}
		dev_info(dev, "%s: batt_temp_adc_cal = %d\n", __func__, x);
                break;
#ifdef __BATTERY_COMPENSATION__
	case BATT_VIBRATOR:
		if (sscanf(buf, "%d\n", &x) == 1) {
			s3c_bat_set_compesation(x, OFFSET_VIBRATOR_ON,
					COMPENSATE_VIBRATOR);
			ret = count;
		}
		dev_info(dev, "%s: vibrator = %d\n", __func__, x);
                break;
	case BATT_CAMERA:
		if (sscanf(buf, "%d\n", &x) == 1) {
			s3c_bat_set_compesation(x, OFFSET_CAMERA_ON,
					COMPENSATE_CAMERA);
			ret = count;
		}
		dev_info(dev, "%s: camera = %d\n", __func__, x);
                break;
	case BATT_MP3:
		if (sscanf(buf, "%d\n", &x) == 1) {
			s3c_bat_set_compesation(x, OFFSET_MP3_PLAY,
					COMPENSATE_MP3);
			ret = count;
		}
		dev_info(dev, "%s: mp3 = %d\n", __func__, x);
                break;
	case BATT_VIDEO:
		if (sscanf(buf, "%d\n", &x) == 1) {
			s3c_bat_set_compesation(x, OFFSET_VIDEO_PLAY,
					COMPENSATE_VIDEO);
			ret = count;
		}
		dev_info(dev, "%s: video = %d\n", __func__, x);
                break;
	case BATT_VOICE_CALL_2G:
		if (sscanf(buf, "%d\n", &x) == 1) {
			if(x)
				call_state |= OFFSET_VOICE_CALL_2G;
			else
				call_state &= ~OFFSET_VOICE_CALL_2G;

			s3c_bat_set_compesation(x, OFFSET_VOICE_CALL_2G,
					COMPENSATE_VOICE_CALL_2G);
			ret = count;
		}
		dev_info(dev, "%s: voice call 2G = %d\n", __func__, x);
                break;
	case BATT_VOICE_CALL_3G:
		if (sscanf(buf, "%d\n", &x) == 1) {
			if(x)
				call_state |= OFFSET_VOICE_CALL_3G;
			else
				call_state &= ~OFFSET_VOICE_CALL_3G;

			s3c_bat_set_compesation(x, OFFSET_VOICE_CALL_3G,
					COMPENSATE_VOICE_CALL_3G);
			ret = count;
		}
		dev_info(dev, "%s: voice call 3G = %d\n", __func__, x);
                break;
	case BATT_DATA_CALL:
//		if (sscanf(buf, "%d\n", &x) == 1) {
//			s3c_bat_set_compesation(x, OFFSET_DATA_CALL,
//					COMPENSATE_DATA_CALL);
//			ret = count;
//		}
/* To remove TDMA noise, set the EAR_SEL gpio set high when data call */
//		data_call = x;
//		wm8994_data_call_hp_switch();
//		dev_info(dev, "%s: data call = %d\n", __func__, x);
                break;
#endif /* __BATTERY_COMPENSATION__ */

        default:
                ret = -EINVAL;
        }

	return ret;
}


#ifdef __BATTERY_COMPENSATION__
void s3c_bat_set_compensation_for_drv(int mode, int offset)
{
	switch(offset) {
	case OFFSET_VIBRATOR_ON:
		dev_dbg(dev, "%s: vibrator = %d\n", __func__, mode);
		s3c_bat_set_compesation(mode, offset, COMPENSATE_VIBRATOR);
		break;
	case OFFSET_LCD_ON:
		dev_dbg(dev, "%s: LCD On = %d\n", __func__, mode);
		s3c_bat_set_compesation(mode, offset, COMPENSATE_LCD);
		break;
	case OFFSET_CAM_FLASH:
		dev_dbg(dev, "%s: flash = %d\n", __func__, mode);
		s3c_bat_set_compesation(mode, offset, COMPENSATE_CAM_FALSH);
		break;
	default:
		break;
	}
}
EXPORT_SYMBOL(s3c_bat_set_compensation_for_drv);
#endif /* __BATTERY_COMPENSATION__ */





static enum power_supply_property s3c_battery_properties[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_CAPACITY,
};

static enum power_supply_property s3c_power_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static char *supply_list[] = {
	"battery",
};

static struct power_supply s3c_power_supplies[] = {
	{
		.name = "battery",
		.type = POWER_SUPPLY_TYPE_BATTERY,
		.properties = s3c_battery_properties,
		.num_properties = ARRAY_SIZE(s3c_battery_properties),
		.get_property = s3c_bat_get_property,
	},
	{
		.name = "usb",
		.type = POWER_SUPPLY_TYPE_USB,
		.supplied_to = supply_list,
		.num_supplicants = ARRAY_SIZE(supply_list),
		.properties = s3c_power_properties,
		.num_properties = ARRAY_SIZE(s3c_power_properties),
		.get_property = s3c_power_get_property,
	},
	{
		.name = "ac",
		.type = POWER_SUPPLY_TYPE_MAINS,
		.supplied_to = supply_list,
		.num_supplicants = ARRAY_SIZE(supply_list),
		.properties = s3c_power_properties,
		.num_properties = ARRAY_SIZE(s3c_power_properties),
		.get_property = s3c_power_get_property,
	},
};


static int s3c_cable_status_update(int status)
{
	int ret = 0;
	charger_type_t source = CHARGER_BATTERY;

	dev_dbg(dev, "%s\n", __func__);

	if(!s3c_battery_initial)
		return -EPERM;

	switch(status) {
	case CHARGER_BATTERY:
		dev_dbg(dev, "%s: cable NOT PRESENT\n", __func__);
		s3c_bat_info.bat_info.charging_source = CHARGER_BATTERY;
		break;
	case CHARGER_USB:
		dev_dbg(dev, "%s: cable USB\n", __func__);
		s3c_bat_info.bat_info.charging_source = CHARGER_USB;
		break;
	case CHARGER_AC:
		dev_dbg(dev, "%s: cable AC\n", __func__);
		s3c_bat_info.bat_info.charging_source = CHARGER_AC;
		break;
	case CHARGER_DISCHARGE:
		dev_dbg(dev, "%s: Discharge\n", __func__);
		s3c_bat_info.bat_info.charging_source = CHARGER_DISCHARGE;
		break;
	default:
		dev_err(dev, "%s: Not supported status\n", __func__);
		ret = -EINVAL;
	}
	source = s3c_bat_info.bat_info.charging_source;

	if (source == CHARGER_USB || source == CHARGER_AC) {
		wake_lock(&vbus_wake_lock);
	}
	else {
		wake_lock_timeout(&vbus_wake_lock, HZ / 2);
	}

	/* if the power source changes, all power supplies may change state */
	power_supply_changed(&s3c_power_supplies[CHARGER_BATTERY]);
	/*
	power_supply_changed(&s3c_power_supplies[CHARGER_USB]);
	power_supply_changed(&s3c_power_supplies[CHARGER_AC]);
	*/
	dev_dbg(dev, "%s: call power_supply_changed\n", __func__);
	return ret;
}


static void s3c_bat_status_update(struct power_supply *bat_ps)
{
	int old_level, old_temp, old_is_full;
	int temp_val, level_val;
	dev_dbg(dev, "%s ++\n", __func__);

	if(!s3c_battery_initial)
		return;

	mutex_lock(&work_lock);
	old_temp = s3c_bat_info.bat_info.batt_temp;
	old_level = s3c_bat_info.bat_info.level;
	old_is_full = s3c_bat_info.bat_info.batt_is_full;

//    printk("---> battery : s3c_bat_status_update : level : %d\n",s3c_bat_info.bat_info.level );

	temp_val = s3c_get_bat_temp(bat_ps);
	level_val = s3c_get_bat_level(bat_ps);

//    printk("---> battery : s3c_bat_status_update : new level : %d\n",level_val );


	if(temp_val == -EINVAL || level_val < 0)  // if adc read fails, then skip battery status update
	{
		printk("%s: ADC value is invalid, skip update!! (temp:%d, vol:%d)\n", __func__, temp_val, level_val);
		mutex_unlock(&work_lock);
		return ;
	}

	s3c_bat_info.bat_info.batt_temp = temp_val;
	s3c_bat_info.bat_info.level = level_val;

	if (!s3c_bat_info.bat_info.charging_enabled &&
			!s3c_bat_info.bat_info.batt_is_full &&
			!battery_cal_updated) {  // If cal update, then skip this.
		if (s3c_bat_info.bat_info.level > old_level)
			s3c_bat_info.bat_info.level = old_level;
	}
	s3c_bat_info.bat_info.batt_vol = s3c_get_bat_vol(bat_ps);

	if (old_level != s3c_bat_info.bat_info.level
			|| old_temp != s3c_bat_info.bat_info.batt_temp
			|| old_is_full != s3c_bat_info.bat_info.batt_is_full
			|| force_update) {
		force_update = 0;
		power_supply_changed(bat_ps);
		dev_dbg(dev, "%s: call power_supply_changed\n", __func__);
	}

	// After update, clear cal update flag.
	if(battery_cal_updated)
		battery_cal_updated = 0;

	mutex_unlock(&work_lock);
	dev_dbg(dev, "%s --\n", __func__);
}

static void s3c_cable_check_status(void)
{
	charger_type_t status = 0;

	mutex_lock(&work_lock);

#ifdef __USING_MAX8998_CHARGER__
	// Get status from FSA9480 driver
	if (get_vdcin_status())
	{
		if (s3c_get_bat_health() != POWER_SUPPLY_HEALTH_GOOD)
			{
			dev_info(dev, "%s: Unhealth battery state!\n", __func__);
			status = CHARGER_DISCHARGE;
			s3c_set_chg_en(0);
			goto __end__;
		}

		if (get_usb_power_state())
			status = CHARGER_USB;
		else
			status = CHARGER_AC;

		s3c_set_chg_en(1);
		low_batt_power_off = 0;  // Clear low batt flag
		dev_dbg(dev, "%s: status : %s\n", __func__,
				(status == CHARGER_USB) ? "USB" : "AC");
	} else {
		status = CHARGER_BATTERY;
		s3c_set_chg_en(0);
	}
#else /* __USING_MAX8998_CHARGER__ */
	if (!gpio_get_value(gpio_ta_connected)) {
		if (s3c_get_bat_health() != POWER_SUPPLY_HEALTH_GOOD) {
			dev_info(dev, "%s: Unhealth battery state!\n", __func__);
			status = CHARGER_DISCHARGE;
			s3c_set_chg_en(0);
			goto __end__;
		}

		if (get_usb_power_state())
			status = CHARGER_USB;
		else
			status = CHARGER_AC;

		s3c_set_chg_en(1);
		dev_dbg(dev, "%s: status : %s\n", __func__,
				(status == CHARGER_USB) ? "USB" : "AC");
	} else {
		status = CHARGER_BATTERY;
		s3c_set_chg_en(0);
	}
#endif /* __USING_MAX8998_CHARGER__ */

__end__:
	dev_dbg(dev, "%s: charging %s\n", __func__,
			s3c_bat_info.bat_info.charging_enabled?"disabled":"enabled");

	s3c_cable_status_update(status);
	mutex_unlock(&work_lock);
}


static void s3c_bat_work(struct work_struct *work)
{
	dev_dbg(dev, "%s\n", __func__);

	s3c_bat_status_update(&s3c_power_supplies[CHARGER_BATTERY]);
}

/*
static void s3c_cable_work(struct work_struct *work)
{
	dev_dbg(dev, "%s\n", __func__);
	s3c_cable_check_status();
}
*/

void s3c_cable_work(struct work_struct *work)
{
	printk("%s OK!\n", __func__);
	s3c_cable_check_status();
}
EXPORT_SYMBOL(s3c_cable_work);



#ifdef CONFIG_PM
static int s3c_bat_suspend(struct platform_device *pdev,
		pm_message_t state)
{
//	dev_info(dev, "%s\n", __func__);

	if (s3c_bat_info.polling)
		del_timer_sync(&polling_timer);

	flush_scheduled_work();

	resume_from_sleep = 0;  // clear flag

#ifndef __USING_MAX8998_CHARGER__
	disable_irq(IRQ_TA_CONNECTED_N);
	disable_irq(IRQ_TA_CHG_N);
#endif /* __USING_MAX8998_CHARGER__ */

	return 0;
}

static int s3c_bat_resume(struct platform_device *pdev)
{
	dev_info(dev, "%s\n", __func__);

#ifndef __USING_MAX8998_CHARGER__
	enable_irq(IRQ_TA_CONNECTED_N);
	enable_irq(IRQ_TA_CHG_N);
#endif /* __USING_MAX8998_CHARGER__ */

	resume_from_sleep = 1;  // set flag

	schedule_work(&bat_work);
	schedule_work(&cable_work);

	if (s3c_bat_info.polling)
		mod_timer(&polling_timer,
			  jiffies + msecs_to_jiffies(s3c_bat_info.polling_interval));
	return 0;
}
#else
#define s3c_bat_suspend NULL
#define s3c_bat_resume NULL
#endif /* CONFIG_PM */


static void polling_timer_func(unsigned long unused)
{
	dev_dbg(dev, "%s\n", __func__);
	schedule_work(&bat_work);

	mod_timer(&polling_timer,
		  jiffies + msecs_to_jiffies(s3c_bat_info.polling_interval));
}


static void cable_timer_func(unsigned long unused)
{
	dev_info(dev, "%s : intr cnt = %d\n", __func__, cable_intr_cnt);
	cable_intr_cnt = 0;
	schedule_work(&cable_work);
}


#ifdef __USING_MAX8998_CHARGER__
void cable_status_changed(void)
{
	if (!s3c_battery_initial)
		return ;

	s3c_bat_info.bat_info.batt_is_full = 0;

	cable_intr_cnt++;
	if (timer_pending(&cable_timer))
		del_timer(&cable_timer);

	cable_timer.expires = jiffies + msecs_to_jiffies(50);
	add_timer(&cable_timer);

	/*
	 * Wait a bit before reading ac/usb line status and setting charger,
	 * because ac/usb status readings may lag from irq.
	 */
	if (s3c_bat_info.polling)
		mod_timer(&polling_timer,
			  jiffies + msecs_to_jiffies(s3c_bat_info.polling_interval));

	return ;
}
EXPORT_SYMBOL(cable_status_changed);

void charging_status_changed(void)
{
	if (!s3c_battery_initial)
		return ;

	if (s3c_bat_info.bat_info.charging_enabled &&
			s3c_get_bat_health() == POWER_SUPPLY_HEALTH_GOOD) {
		s3c_set_chg_en(0);
		s3c_bat_info.bat_info.batt_is_full = 1;
		force_update = 1;
		full_charge_flag = 1;
	}

	schedule_work(&bat_work);
	/*
	 * Wait a bit before reading ac/usb line status and setting charger,
	 * because ac/usb status readings may lag from irq.
	 */
	if (s3c_bat_info.polling)
		mod_timer(&polling_timer,
			  jiffies + msecs_to_jiffies(s3c_bat_info.polling_interval));

	return ;
}
EXPORT_SYMBOL(charging_status_changed);

#else /* __USING_MAX8998_CHARGER__ */
static irqreturn_t s3c_cable_changed_isr(int irq, void *power_supply)
{
	dev_dbg(dev, "%s: irq=0x%x, gpio_ta_connected=%x\n", __func__, irq,
			gpio_get_value(gpio_ta_connected));

	if (!s3c_battery_initial)
		return IRQ_HANDLED;

	s3c_bat_info.bat_info.batt_is_full = 0;

	cable_intr_cnt++;
	if (timer_pending(&cable_timer))
		del_timer(&cable_timer);

	cable_timer.expires = jiffies + msecs_to_jiffies(50);
	add_timer(&cable_timer);

	/*
	 * Wait a bit before reading ac/usb line status and setting charger,
	 * because ac/usb status readings may lag from irq.
	 */
	if (s3c_bat_info.polling)
		mod_timer(&polling_timer,
			  jiffies + msecs_to_jiffies(s3c_bat_info.polling_interval));

	return IRQ_HANDLED;
}

static irqreturn_t s3c_cable_charging_isr(int irq, void *power_supply)
{
	int chg_ing = gpio_get_value(gpio_chg_ing);
	dev_dbg(dev, "%s: irq=0x%x, gpio_chg_ing=%d\n", __func__, irq, chg_ing);

	if (!s3c_battery_initial)
		return IRQ_HANDLED;

	if (chg_ing && !gpio_get_value(gpio_ta_connected) &&
			s3c_bat_info.bat_info.charging_enabled &&
			s3c_get_bat_health() == POWER_SUPPLY_HEALTH_GOOD) {
		s3c_set_chg_en(0);
		s3c_bat_info.bat_info.batt_is_full = 1;
		force_update = 1;
		full_charge_flag = 1;
	}

	schedule_work(&bat_work);
	/*
	 * Wait a bit before reading ac/usb line status and setting charger,
	 * because ac/usb status readings may lag from irq.
	 */
	if (s3c_bat_info.polling)
		mod_timer(&polling_timer,
			  jiffies + msecs_to_jiffies(s3c_bat_info.polling_interval));

	return IRQ_HANDLED;
}
#endif /* __USING_MAX8998_CHARGER__ */

extern int adc_init_completed;
static int __devinit s3c_bat_probe(struct platform_device *pdev)
{
	int i;
	int ret = 0;

	dev = &pdev->dev;
	dev_info(dev, "%s\n", __func__);

	s3c_bat_info.present = 1;
	s3c_bat_info.polling = 1;
	s3c_bat_info.polling_interval = POLLING_INTERVAL;
	s3c_bat_info.device_state = 0;

	s3c_bat_info.bat_info.batt_vol_adc_aver = 0;

	s3c_bat_info.bat_info.batt_id = 0;
	s3c_bat_info.bat_info.batt_vol = 0;
	s3c_bat_info.bat_info.batt_vol_adc = 0;
	s3c_bat_info.bat_info.batt_vol_adc_cal = 0;
	s3c_bat_info.bat_info.batt_temp = 0;
	s3c_bat_info.bat_info.batt_temp_adc = 0;
	s3c_bat_info.bat_info.batt_temp_adc_cal = 0;
	s3c_bat_info.bat_info.batt_current = 0;
	s3c_bat_info.bat_info.level = 100;
	s3c_bat_info.bat_info.charging_source = CHARGER_BATTERY;
	s3c_bat_info.bat_info.charging_enabled = 0;
	s3c_bat_info.bat_info.batt_health = POWER_SUPPLY_HEALTH_GOOD;

	memset(adc_sample, 0x00, sizeof adc_sample);

	batt_max = BATT_CAL + BATT_MAXIMUM;
	batt_off = BATT_CAL + BATT_OFF;

#ifdef __BATTERY_COMPENSATION__
	batt_compensation = 0;
#endif /* __BATTERY_COMPENSATION__ */

	if(apollo_get_remapped_hw_rev_pin() >= 4)  // Above Rev0.6
		has_temp_adc_channel = 1;
	else  // others
		has_temp_adc_channel = 0;

	INIT_WORK(&bat_work, s3c_bat_work);
	INIT_WORK(&cable_work, s3c_cable_work);

	/* init power supplier framework */
	for (i = 0; i < ARRAY_SIZE(s3c_power_supplies); i++) {
		ret = power_supply_register(&pdev->dev,
				&s3c_power_supplies[i]);
		if (ret) {
			dev_err(dev, "Failed to register"
					"power supply %d,%d\n", i, ret);
			goto __end__;
		}
	}

	/* create sec detail attributes */
	s3c_bat_create_attrs(s3c_power_supplies[CHARGER_BATTERY].dev);

	/* Request IRQ */
#ifndef __USING_MAX8998_CHARGER__
	set_irq_type(IRQ_TA_CONNECTED_N, IRQ_TYPE_EDGE_BOTH);
	ret = request_irq(IRQ_TA_CONNECTED_N, s3c_cable_changed_isr,
			  IRQF_DISABLED,
			  DRIVER_NAME,
			  &s3c_power_supplies[CHARGER_BATTERY]);
	if(ret)
		goto __end__;

	set_irq_type(IRQ_TA_CHG_N, IRQ_TYPE_EDGE_BOTH);
	ret = request_irq(IRQ_TA_CHG_N, s3c_cable_charging_isr,
			  IRQF_DISABLED,
			  DRIVER_NAME,
			  &s3c_power_supplies[CHARGER_BATTERY]);

	if (ret)
		goto __ta_connected_irq_failed__;
#endif /* __USING_MAX8998_CHARGER__ */

	if (s3c_bat_info.polling) {
		dev_dbg(dev, "%s: will poll for status\n",
				__func__);
		setup_timer(&polling_timer, polling_timer_func, 0);
		mod_timer(&polling_timer,
			  jiffies + msecs_to_jiffies(s3c_bat_info.polling_interval));
	}

	setup_timer(&cable_timer, cable_timer_func, 0);

	s3c_battery_initial = 1;
	force_update = 0;
	full_charge_flag = 0;

	// LCD is already ON while boot up.
	s3c_bat_set_compesation(1, OFFSET_LCD_ON, COMPENSATE_LCD);

	s3c_bat_status_update(&s3c_power_supplies[CHARGER_BATTERY]);

	s3c_cable_check_status();

__end__:
	return ret;

#ifndef __USING_MAX8998_CHARGER__
__ta_connected_irq_failed__:
	free_irq(IRQ_TA_CONNECTED_N, &s3c_power_supplies[CHARGER_BATTERY]);
	return ret;
#endif /* __USING_MAX8998_CHARGER__ */
}

static int __devexit s3c_bat_remove(struct platform_device *pdev)
{
	int i;
	dev_info(dev, "%s\n", __func__);

	if (s3c_bat_info.polling)
		del_timer_sync(&polling_timer);

#ifndef __USING_MAX8998_CHARGER__
	free_irq(IRQ_TA_CONNECTED_N, &s3c_power_supplies[CHARGER_BATTERY]);
	free_irq(IRQ_TA_CHG_N, &s3c_power_supplies[CHARGER_BATTERY]);
#endif /* __USING_MAX8998_CHARGER__ */

	for (i = 0; i < ARRAY_SIZE(s3c_power_supplies); i++) {
		power_supply_unregister(&s3c_power_supplies[i]);
	}

	return 0;
}

static struct platform_driver s3c_bat_driver = {
	.driver.name	= DRIVER_NAME,
	.driver.owner	= THIS_MODULE,
	.probe		= s3c_bat_probe,
	.remove		= __devexit_p(s3c_bat_remove),
	.suspend		= s3c_bat_suspend,
	.resume		= s3c_bat_resume,
};

/* Initailize GPIO */
static void s3c_bat_init_hw(void)
{
#ifndef __USING_MAX8998_CHARGER__
	s3c_gpio_cfgpin(gpio_ta_connected,
			S3C_GPIO_SFN(gpio_ta_connected_af));
	s3c_gpio_setpull(gpio_ta_connected, S3C_GPIO_PULL_UP);

	s3c_gpio_cfgpin(gpio_chg_ing,
			S3C_GPIO_SFN(gpio_chg_ing_af));
	s3c_gpio_setpull(gpio_chg_ing, S3C_GPIO_PULL_UP);

	s3c_gpio_cfgpin(gpio_chg_en,
			S3C_GPIO_SFN(gpio_chg_en_af));
	s3c_gpio_setpull(gpio_chg_en, S3C_GPIO_PULL_NONE);
	gpio_set_value_ex(gpio_chg_en, GPIO_LEVEL_HIGH);
#endif /* __USING_MAX8998_CHARGER__ */
}


static int __init s3c_bat_init(void)
{
	pr_info("%s\n", __func__);
	s3c_bat_init_hw();

	wake_lock_init(&vbus_wake_lock, WAKE_LOCK_SUSPEND, "vbus_present");

	wake_lock_init(&wake_lock_for_off, WAKE_LOCK_SUSPEND, "wake_lock_off");


	return platform_driver_register(&s3c_bat_driver);
}


static void __exit s3c_bat_exit(void)
{
	pr_info("%s\n", __func__);

	platform_driver_unregister(&s3c_bat_driver);
}


module_init(s3c_bat_init);
module_exit(s3c_bat_exit);


MODULE_AUTHOR("Minsung Kim <ms925.kim@samsung.com>");
MODULE_DESCRIPTION("S3C6410 battery driver");
MODULE_LICENSE("GPL");


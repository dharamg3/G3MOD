/*
 * linux/drivers/power/s5p6442_battery.h
 *
 * Battery measurement code for S5P6442 platform.
 *
 * Copyright (C) 2009 Samsung Electronics.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#define DRIVER_NAME	"apollo-battery"

/*
 * Apollo Rev00 board Battery Table (from saturn)
 */
#define VOLTAGE_CAL		(0)	// temp calibration value
#define BATT_CAL			(2115 + VOLTAGE_CAL)	/* 3.65V */
#if 0  // Not used.
#define BATT_MAXIMUM		735		/* 4.20V */
#define BATT_FULL			605		/* 4.10V */
#define BATT_SAFE_RECHARGE	605		/* 4.10V */
#define BATT_ALMOST_FULL	485		/* 4.01V */
#define BATT_HIGH			255		/* 3.84V */
#define BATT_MED			135		/* 3.75V */
#define BATT_LOW			80		/* 3.71V */
#define BATT_CRITICAL		0		/* 3.65V */ 
#define BATT_MINIMUM		(-80)	/* 3.59V */
#define BATT_OFF			(-335)	/* 3.40V */
#endif
#define BATT_MAXIMUM		605		/* 4.119V */
#define BATT_FULL			460		/* 4.021V */
#define BATT_SAFE_RECHARGE	460		/* 4.021V */
#define BATT_ALMOST_FULL	285		/* 3.881V */
#define BATT_HIGH			132		/* 3.777V */
#define BATT_MED			92		/* 3.730V */
#define BATT_LOW			(1)		/* 3.680V */
#define BATT_CRITICAL		(-74)	/* 3.624V */ 
#define BATT_MINIMUM		(-114)	/* 3.601V */
#define BATT_OFF			(-356)	/* 3.385V */


/*
 * Apollo Rev06 board Temperature Table
 */
 const int temper_table[][2] =  {
	/* ADC, Temperature (C) */
	{ 530,		-50	},
	{ 555,		-40	},
	{ 580,		-30	},
	{ 605,		-20	},
	{ 631,		-10	},
	{ 656,		0	},
	{ 684,		10	},
	{ 713,		20	},
	{ 742,		30	},
	{ 770,		40	},
	{ 799,		50	},
	{ 828,		60	},
	{ 857,		70	},
	{ 886,		80	},
	{ 915,		90	},
	{ 944,		100	},
	{ 978,		110	},
	{ 1012,		120	},
	{ 1046,		130	},
	{ 1080,		140	},
	{ 1115,		150	},
	{ 1150,		160	},
	{ 1185,		170	},
	{ 1220,		180	},
	{ 1255,		190	},
	{ 1291,		200	},
	{ 1327,		210	},
	{ 1364,		220	},
	{ 1401,		230	},
	{ 1437,		240	},
	{ 1474,		250	},
	{ 1510,		260	},
	{ 1546,		270	},
	{ 1583,		280	},
	{ 1619,		290	},
	{ 1656,		300	},
	{ 1692,		310	},
	{ 1729,		320	},
	{ 1765,		330	},
	{ 1802,		340	},
	{ 1839,		350	},
	{ 1876,		360	},
	{ 1914,		370	},
	{ 1952,		380	},
	{ 1990,		390	},
	{ 2028,		400	},
	{ 2063,		410	},
	{ 2099,		420	},
	{ 2135,		430	},
	{ 2171,		440	},
	{ 2207,		450	},
	{ 2241,		460	},
	{ 2276,		470	},
	{ 2311,		480	},
	{ 2346,		490	},
	{ 2381,		500	},
	{ 2414,		510	},
	{ 2447,		520	},
	{ 2480,		530	},
	{ 2513,		540	},
	{ 2546,		550	},
	{ 2575,		560	},
	{ 2604,		570	},
	{ 2642,		580	},
	{ 2662,		590	},
	{ 2692,		600	},
	{ 2721,		610	},
	{ 2751,		620	},
	{ 2764,		630	},
	{ 2810,		640	},
	{ 2840,		650	},
};

#define TEMP_HIGH_BLOCK		temper_table[68][0]
#define TEMP_HIGH_RECOVER	temper_table[63][0]
#define TEMP_LOW_BLOCK		temper_table[0][0]
#define TEMP_LOW_RECOVER	temper_table[5][0]

//for Apollo TSP
#define TSP_TEMP_HIGH_ENTRY		1876	//36 degree
#define TSP_TEMP_HIGH_CLEAR		1765	//33 degree
#define TSP_TEMP_LOW_ENTRY		650
#define TSP_TEMP_LOW_CLEAR		750


/*
 * Apollo Rev00 board ADC channel
 */
 #if 0
typedef enum s3c_adc_channel {
	S3C_ADC_EAR = 0,
	S3C_ADC_CHG_CURRENT,
	S3C_ADC_VOLTAGE,
	S3C_ADC_TEMPERATURE,
	ENDOFADC
} adc_channel_type;
#else  // for apollo new emul (QSC)
typedef enum s3c_adc_channel {
	S3C_ADC_VOLTAGE = 1,
	S3C_ADC_CHG_CURRENT,
	S3C_ADC_EAR,
	S3C_ADC_TEMPERATURE,
	ENDOFADC
} adc_channel_type;
#endif

#ifndef CONFIG_CHARGER_MAX8998
#define IRQ_TA_CONNECTED_N	IRQ_EINT(8)
#define IRQ_TA_CHG_N			IRQ_EINT(9)

const unsigned int gpio_ta_connected	= GPIO_TA_nCONNECTED;
const unsigned int gpio_ta_connected_af	= GPIO_TA_nCONNECTED_STATE;
const unsigned int gpio_chg_ing		= GPIO_TA_nCHG;
const unsigned int gpio_chg_ing_af	= GPIO_TA_nCHG_STATE;
const unsigned int gpio_chg_en		= GPIO_TA_EN;
const unsigned int gpio_chg_en_af	= GPIO_TA_EN_STATE;
#endif

/******************************************************************************
 * Battery driver features
 * ***************************************************************************/
/* #define __TEMP_ADC_VALUE__ */  // for temporary adc value
/* #define __CHECK_BATTERY_V_F__ */  // for checking VF ADC is valid
/* #define __BATTERY_V_F__ */  // for reading VF ADC
#define __BATTERY_COMPENSATION__  // for battery compensation
#define __TEST_DEVICE_DRIVER__  // for just test
#define __TEST_MODE_INTERFACE__  // for supporting test mode
/* #define __CHECK_CHG_CURRENT__ */  // for checking charging current ADC
/* #define __CHECK_RECHARGING__  */ // for checking recharging condition
#if CONFIG_FUEL_GAUGE_MAX17040
#define __FUEL_GAUGE_IC__  // using Fuel Guage IC
#endif 
#if CONFIG_CHARGER_MAX8998
#define __USING_MAX8998_CHARGER__
#endif
#define	__CHECK_TSP_TEMP_NOTIFICATION__		// for Apollo TSP
/*****************************************************************************/

#define TOTAL_CHARGING_TIME	(6*60*60*1000)	/* 6 hours */
#define TOTAL_RECHARGING_TIME	(3*30*60*1000)	/* 1.5 hours */

#ifdef __CHECK_BATTERY_V_F__
#define BATT_VF_MAX		5
#define BATT_VF_MIN		0
#endif /* __CHECK_BATTERY_V_F__ */

#ifdef __CHECK_CHG_CURRENT__
#define CURRENT_OF_FULL_CHG		65
#endif /* __CHECK_CHG_CURRENT__ */

#ifdef __BATTERY_COMPENSATION__
#define COMPENSATE_VIBRATOR		25
#define COMPENSATE_CAMERA		50
#define COMPENSATE_MP3			25
#define COMPENSATE_VIDEO			30
#define COMPENSATE_VOICE_CALL_2G	0  // removed
#define COMPENSATE_VOICE_CALL_3G	0  // removed
#define COMPENSATE_DATA_CALL		150
#define COMPENSATE_LCD			40
#define COMPENSATE_TA				0
#define COMPENSATE_CAM_FALSH		0
#define COMPENSATE_BOOTING		50
#endif /* __BATTERY_COMPENSATION__ */

#ifdef __FUEL_GAUGE_IC__
#define SOC_LB_FOR_POWER_OFF		27

#define FULL_CHARGE_COND_VOLTAGE	4000
#define RECHARGE_COND_VOLTAGE		4150
#endif /* __FUEL_GAUGES_IC__ */


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

#define BATT_MAXIMUM		605		/* 4.1V */
#define BATT_OFF		(-114)		/* 3.6V */


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



typedef enum s3c_adc_channel {
	S3C_ADC_VOLTAGE = 1,
	S3C_ADC_CHG_CURRENT,
	S3C_ADC_EAR,
	S3C_ADC_TEMPERATURE,
	ENDOFADC
} adc_channel_type;

#ifndef CONFIG_CHARGER_MAX8998
#define IRQ_TA_CONNECTED_N	IRQ_EINT(8)
#define IRQ_TA_CHG_N			IRQ_EINT(9)

const unsigned int gpio_ta_connected_af	= GPIO_TA_nCONNECTED_STATE;
const unsigned int gpio_chg_ing		= GPIO_TA_nCHG;
const unsigned int gpio_chg_ing_af	= GPIO_TA_nCHG_STATE;
const unsigned int gpio_chg_en		= GPIO_TA_EN;
const unsigned int gpio_chg_en_af	= GPIO_TA_EN_STATE;
#endif

/******************************************************************************
 * Battery driver features
 * ***************************************************************************/
#define __BATTERY_COMPENSATION__  // for battery compensation
#if CONFIG_CHARGER_MAX8998
#define __USING_MAX8998_CHARGER__
#endif
#define	__CHECK_TSP_TEMP_NOTIFICATION__		// for Apollo TSP
/*****************************************************************************/

#define TOTAL_CHARGING_TIME	(6*60*60*1000)	/* 6 hours */
#define TOTAL_RECHARGING_TIME	(3*60*60*1000)	/* 3 hours */

#ifdef __BATTERY_COMPENSATION__
#define COMPENSATE_VIBRATOR		25
#define COMPENSATE_CAMERA		50
#define COMPENSATE_MP3			25
#define COMPENSATE_VIDEO		30
#define COMPENSATE_VOICE_CALL_2G	0  
#define COMPENSATE_VOICE_CALL_3G	0
#define COMPENSATE_DATA_CALL		150
#define COMPENSATE_LCD			40
#define COMPENSATE_TA			0
#define COMPENSATE_CAM_FALSH		0
#define COMPENSATE_BOOTING		50
#endif /* __BATTERY_COMPENSATION__ */


/*
 * linux/drivers/i2c/MAX8998.c
 *
 * Battery measurement code for SEC
 *
 * based on max8906.c
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
//#include <linux/i2c/maximi2c.h>
#include <linux/i2c/pmic.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/workqueue.h>
#include <asm/mach-types.h>
#include <mach/hardware.h>
#include <plat/gpio-cfg.h>
#include <linux/leds.h>


#ifdef PREFIX
#undef PREFIX
#endif
#define PREFIX "MAX8998: "

static struct i2c_client *g_client;
struct max8998_state *max8998_state;
struct max8998_state{
	struct i2c_client	*client;	
};

static struct workqueue_struct *max8998_workqueue;
static struct work_struct max8998_work;

int cable_status = 0;  // this variable can be used to check cable connection
EXPORT_SYMBOL(cable_status);

int pmic_init = 0;  // this variable can be used to check whether pmic initialization is finished or not.
EXPORT_SYMBOL(pmic_init);


extern void cable_status_changed(void);
extern void charging_status_changed(void);
extern void set_low_batt_flag(void);


max8998_reg_type max8998pm[ENDOFPM + 1] =
{	
	/* addr     mask       clear      shift   */
	{0x00,      0x80,       0x7F,       0x07 }, // PWRONR
	{0x00,      0x40,       0xBF,       0x06 }, // PWRONF    
	{0x00,      0x20,       0xDF,       0x05 }, // JIGR    
	{0x00,      0x10,       0xEF,       0x04 }, // JIGF  
	{0x00,      0x08,       0xF7,       0x03 }, // DCINR  
	{0x00,      0x04,       0xFB,       0x02 }, // DCINF  

	{0x01,      0x08,       0xF7,       0x03 }, // ALARM0
	{0x01,      0x04,       0xFB,       0x02 }, // ALARM1
	{0x01,      0x02,       0xFD,       0x01 }, // SMPLEVNT
	{0x01,      0x01,       0xFE,       0x00 }, // WTSREVNT

	{0x02,      0x80,       0x7F,       0x07 }, // CHGFAULT
	{0x02,      0x20,       0xDF,       0x05 }, // DONER
	{0x02,      0x10,       0xEF,       0x04 }, // CHGRSTF
	{0x02,      0x08,       0xF7,       0x03 }, // DCINOVPR
	{0x02,      0x04,       0xFB,       0x02 }, // TOPOFFR
	{0x02,      0x01,       0xFE,       0x00 }, // ONKEY1S

	{0x03,      0x02,       0xFD,       0x01 }, // LOBAT2
	{0x03,      0x01,       0xFE,       0x00 }, // LOBAT1

	{0x04,      0x80,       0x7F,       0x07 }, // PWRONRM
	{0x04,      0x40,       0xBF,       0x06 }, // PWRONFM
	{0x04,      0x20,       0xDF,       0x05 }, // JIGRM
	{0x04,      0x10,       0xEF,       0x04 }, // JIGFM
	{0x04,      0x08,       0xF7,       0x03 }, // DCINRM
	{0x04,      0x04,       0xFB,       0x02 }, // DCINFM

	{0x05,      0x08,       0xF7,       0x03 }, // ALARM0M
	{0x05,      0x04,       0xFB,       0x02 }, // ALARM1M
	{0x05,      0x02,       0xFD,       0x01 }, // SMPLEVNTM
	{0x05,      0x01,       0xFE,       0x00 }, // WTSREVNTM

	{0x06,      0x80,       0x7F,       0x07 }, // CHGFAULTM
	{0x06,      0x20,       0xDF,       0x05 }, // DONERM
	{0x06,      0x10,       0xEF,       0x04 }, // CHGRSTFM
	{0x06,      0x08,       0xF7,       0x03 }, // DCINOVPRM
	{0x06,      0x04,       0xFB,       0x02 }, // TOPOFFRM
	{0x06,      0x01,       0xFE,       0x00 }, // ONKEY1SM

	{0x07,      0x02,       0xFD,       0x01 }, // LOBAT2M
	{0x07,      0x01,       0xFE,       0x00 }, // LOBAT1M

	{0x08,      0x80,       0x7F,       0x07 }, // PWRON_status
	{0x08,      0x40,       0xBF,       0x06 }, // JIG_status
	{0x08,      0x20,       0xDF,       0x05 }, // ALARM0_status
	{0x08,      0x10,       0xEF,       0x04 }, // ALARM1_status
	{0x08,      0x08,       0xF7,       0x03 }, // SMPLEVNT_status
	{0x08,      0x04,       0xFB,       0x02 }, // WTSREVNT_status
	{0x08,      0x02,       0xFD,       0x01 }, // LOBAT2_status
	{0x08,      0x01,       0xFE,       0x00 }, // LOBAT1_status

	{0x09,      0x40,       0xBF,       0x06 }, // DONE_status
	{0x09,      0x20,       0xDF,       0x05 }, // VDCINOK_status
	{0x09,      0x10,       0xEF,       0x04 }, // DETBAT_status
	{0x09,      0x08,       0xF7,       0x03 }, // CHGON_status
	{0x09,      0x04,       0xFB,       0x02 }, // FCHG_status
	{0x09,      0x02,       0xFD,       0x01 }, // PQL_status

	{0x0A,      0x80,       0x7F,       0x07 }, // PWRONM_status
	{0x0A,      0x40,       0xBF,       0x06 }, // JIGM_status
	{0x0A,      0x20,       0xDF,       0x05 }, // ALARM0M_status
	{0x0A,      0x10,       0xEF,       0x04 }, // ALARM1M_status
	{0x0A,      0x08,       0xF7,       0x03 }, // SMPLEVNTM_status
	{0x0A,      0x04,       0xFB,       0x02 }, // WTSREVNTM_status
	{0x0A,      0x02,       0xFD,       0x01 }, // LOBAT2M_status
	{0x0A,      0x01,       0xFE,       0x00 }, // LOBAT1M_status

	{0x0B,      0x40,       0xBF,       0x06 }, // DONEM_status
	{0x0B,      0x20,       0xDF,       0x05 }, // VDCINOKM_status
	{0x0B,      0x10,       0xEF,       0x04 }, // DETBATM_status
	{0x0B,      0x08,       0xF7,       0x03 }, // CHGONM_status
	{0x0B,      0x04,       0xFB,       0x02 }, // FCHGM_status
	{0x0B,      0x02,       0xFD,       0x01 }, // PQLM_status

	{0x0C,      0xE0,       0x1F,       0x05 }, // TP
	{0x0C,      0x18,       0xE7,       0x03 }, // RSTR
	{0x0C,      0x07,       0xF8,       0x00 }, // ICH

	{0x0D,      0xC0,       0x3F,       0x06 }, // ESAFEOUT
	{0x0D,      0x30,       0xCF,       0x04 }, // FT
	{0x0D,      0x08,       0xF7,       0x03 }, // BATTSL
	{0x0D,      0x06,       0xF9,       0x01 }, // TMP
	{0x0D,      0x01,       0xFE,       0x00 }, // CHGENB

	/* ADISCHG_EN1 register */
	{ 0x0E,  0x80,  0x7F,  0x07 }, /* LDO2_ADEN */
	{ 0x0E,  0x40,  0xBF,  0x06 }, /* LDO3_ADEN */
	{ 0x0E,  0x20,  0xDF,  0x05 }, /* LDO4_ADEN */
	{ 0x0E,  0x10,  0xEF,  0x04 }, /* LDO5_ADEN */
	{ 0x0E,  0x08,  0xF7,  0x03 }, /* LDO6_ADEN */
	{ 0x0E,  0x04,  0xFB,  0x02 }, /* LDO7_ADEN */
	{ 0x0E,  0x02,  0xFD,  0x01 }, /* LDO8_ADEN */
	{ 0x0E,  0x01,  0xFE,  0x00 }, /* LDO9_ADEN */

	/* ADISCHG_EN2 register */
	{ 0x0F,  0x80,  0x7F,  0x07 }, /* LDO10_ADEN */
	{ 0x0F,  0x40,  0xBF,  0x06 }, /* LDO11_ADEN */
	{ 0x0F,  0x20,  0xDF,  0x05 }, /* LDO12_ADEN */
	{ 0x0F,  0x10,  0xEF,  0x04 }, /* LDO13_ADEN */
	{ 0x0F,  0x08,  0xF7,  0x03 }, /* LDO14_ADEN */
	{ 0x0F,  0x04,  0xFB,  0x02 }, /* LDO15_ADEN */
	{ 0x0F,  0x02,  0xFD,  0x01 }, /* LDO16_ADEN */
	{ 0x0F,  0x01,  0xFE,  0x00 }, /* LDO17_ADEN */


	/* ADISCHG_EN3 register */	
	{ 0x10,  0x20,  0xDF,  0x05 }, /* SAFEOUT1_ADEN  */
	{ 0x10,  0x10,  0xEF,  0x04 }, /* SAFEOUT2_ADEN */
	{ 0x10,  0x08,  0xF7,  0x03 }, /* SD1_ADEN */
	{ 0x10,  0x04,  0xFB,  0x02 }, /* SD2_ADEN */
	{ 0x10,  0x02,  0xFD,  0x01 }, /* SD3_ADEN */
	{ 0x10,  0x01,  0xFE,  0x00 }, /* SD3_ADEN */

	/* ONOFF1 register */
	/* slave_addr              addr   mask   clear  shift */
	{ 0x11,  0x80,  0x7F,  0x07 }, /* EN1 */
	{ 0x11,  0x40,  0xBF,  0x06 }, /* EN2 */
	{ 0x11,  0x20,  0xDF,  0x05 }, /* EN3 */
	{ 0x11,  0x10,  0xEF,  0x04 }, /* EN4 */
	{ 0x11,  0x08,  0xF7,  0x03 }, /* ELDO2 */
	{ 0x11,  0x04,  0xFB,  0x02 }, /* ELDO3 */
	{ 0x11,  0x02,  0xFD,  0x01 }, /* ELDO4 */
	{ 0x11,  0x01,  0xFE,  0x00 }, /* ELDO5 */
	/* ONOFF2 register */
	{ 0x12,  0x80,  0x7F,  0x07 }, /* ELDO6 */
	{ 0x12,  0x40,  0xBF,  0x06 }, /* ELDO7 */
	{ 0x12,  0x20,  0xDF,  0x05 }, /* ELDO8 */
	{ 0x12,  0x10,  0xEF,  0x04 }, /* ELDO9 */
	{ 0x12,  0x08,  0xF7,  0x03 }, /* ELD10 */
	{ 0x12,  0x04,  0xFB,  0x02 }, /* ELD11 */
	{ 0x12,  0x02,  0xFD,  0x01 }, /* ELD12 */
	{ 0x12,  0x01,  0xFE,  0x00 }, /* ELD13 */

	/* ONOFF3 register */
	{ 0x13,  0x80,  0x7F,  0x07 }, /* ELD14 */
	{ 0x13,  0x40,  0xBF,  0x06 }, /* ELD15 */
	{ 0x13,  0x20,  0xDF,  0x05 }, /* ELD16 */
	{ 0x13,  0x10,  0xEF,  0x04 }, /* ELD17 */
	{ 0x13,  0x08,  0xF7,  0x03 }, /* EPWRHOLD */
	{ 0x13,  0x04,  0xFB,  0x02 }, /* ENBATTMON */
	{ 0x13,  0x02,  0xFD,  0x01 }, /* ELBCNFG2 */
	{ 0x13,  0x01,  0xFE,  0x00 }, /* ELBCNFG1 */

	/* ONOFF4 register */
	{ 0x14,  0x80,  0x7F,  0x07 }, /* ENVICHG */
	{ 0x14,  0x40,  0xBF,  0x06 }, /* EN32kHzCP */
	{ 0x14,  0x20,  0xDF,  0x05 }, /* EN32kHzAP */
	{ 0x14,  0x0F,  0xF0,  0x00 }, /* RAMP */

	{ 0x15,  0x1F,  0xE0,  0x00 }, /* DVSARM1 */
	{ 0x16,  0x1F,  0xE0,  0x00 }, /* DVSARM2 */
	{ 0x17,  0x1F,  0xE0,  0x00 }, /* DVSARM3 */
	{ 0x18,  0x1F,  0xE0,  0x00 }, /* DVSARM4 */

	{ 0x19,  0x1F,  0xE0,  0x00 }, /* DVSINT2 */
	{ 0x1A,  0x1F,  0xE0,  0x00 }, /* DVSINT1 */

	{ 0x1B,  0x1F,  0xE0,  0x00 }, /* BUCK3 */

	{ 0x1C,  0x0F,  0xF0,  0x00 }, /* BUCK4 */


	{ 0x1D,      0xF0,       0x0F,       0x04 }, // LDO2
	{ 0x1D,      0x0F,       0xF0,       0x00 }, // LDO3

	{ 0x1E,      0x1F,       0xE0,       0x00 }, // LDO4

	{ 0x1F,      0x1F,       0xE0,       0x00 }, // LDO5

	{ 0x20,      0x1F,       0xE0,       0x00 }, // LDO6

	{ 0x21,      0x1F,       0xE0,       0x00 }, // LDO7

	{ 0x22,      0x70,       0x8F,       0x04 }, // LDO8
	{ 0x22,      0x02,       0xFC,       0x00 }, // LDO9

	{ 0x23,      0xE0,       0x1F,       0x05 }, // LDO10
	{ 0x23,      0x1F,       0xE0,       0x00 }, // LDO11

	{ 0x24,      0x1F,       0xE0,       0x00 }, // LDO12

	{ 0x25,      0x1F,       0xE0,       0x00 }, // LDO13

	{ 0x26,      0x1F,       0xE0,       0x00 }, // LDO14

	{ 0x27,      0x1F,       0xE0,       0x00 }, // LDO15

	{ 0x28,      0x1F,       0xE0,       0x00 }, // LDO16

	{ 0x29,      0x1F,       0xE0,       0x00 }, // LDO17

	{ 0x2A,      0x07,       0xF8,       0x00 }, // BKCHR

	{ 0x2B,      0x30,       0xCF,       0x04 }, // LBHYST1
	{ 0x2B,      0x07,       0xF8,       0x00 }, // LBTH1

	{ 0x2C,      0x30,       0xCF,       0x04 }, // LBHYST2
	{ 0x2C,      0x07,       0xF8,       0x00 }, // LBTH2

	{ 0x3F,      0xF0,       0x0F,       0x04 }, // DASH
	{ 0x3F,      0x0F,       0xF0,       0x00 }, // MASKREV
};


const byte buck4_voltage[VCC_MAX] = {
    0xFF,    // VCC_0p750 = 0, // 0 0.75V
    0xFF,    // VCC_0p775,
    BUCK4_0p800,    // VCC_0p800,
    0xFF,    // VCC_0p825,
    0xFF,    // VCC_0p850,
    0xFF,    // VCC_0p875,
    BUCK4_0p900,    // VCC_0p900,
    0xFF,    // VCC_0p925,
    0xFF,    // VCC_0p950,
    0xFF,    // VCC_0p975,
    BUCK4_1p000,    // VCC_1p000,
    0xFF,    // VCC_1p025,
    0xFF,    // VCC_1p050,
    0xFF,    // VCC_1p075,
    BUCK4_1p100,    // VCC_1p100,
    0xFF,    // VCC_1p125,
    0xFF,    // VCC_1p150,
    0xFF,    // VCC_1p175,
    BUCK4_1p200,    // VCC_1p200,
    0xFF,    // VCC_1p225,
    0xFF,    // VCC_1p250,
    0xFF,    // VCC_1p275,
    BUCK4_1p300,    // VCC_1p300,
    0xFF,    // VCC_1p325
    0xFF,    // VCC_1p350,
    0xFF,    // VCC_1p375,
    BUCK4_1p400,    // VCC_1p400,
    0xFF,    // VCC_1p425,
    0xFF,    // VCC_1p450,
    0xFF,    // VCC_1p475,
    BUCK4_1p500,   // VCC_1p500,
    BUCK4_1p600,   // VCC_1p600,
    BUCK4_1p700,   // VCC_1p700,
    BUCK4_1p800,   // VCC_1p800,
    BUCK4_1p900,   // VCC_1p900,
    BUCK4_2p000,   // VCC_2p000,
    BUCK4_2p100,   // VCC_2p100,
    BUCK4_2p200,   // VCC_2p200,
    BUCK4_2p300,   // VCC_2p300,
    0xFF,    // VCC_2p400,
    0xFF,    // VCC_2p500,
    0xFF,    // VCC_2p600,
    0xFF,    // VCC_2p700,
    0xFF,    // VCC_2p800,
    0xFF,    // VCC_2p900,
    0xFF,    // VCC_3p000,
    0xFF,    // VCC_3p100,
    0xFF,    // VCC_3p200,
    0xFF,    // VCC_3p300,
    0xFF,    // VCC_3p400,
    0xFF,    // VCC_3p500,
    0xFF    // VCC_3p600,
};

const byte ldo2_3_voltage[VCC_MAX] = {
    0xFF,    // VCC_0p750 = 0, // 0 0.75V
    0xFF,    // VCC_0p775,
    LDO2_3_0p800,    // VCC_0p800,
    0xFF,    // VCC_0p825,
    LDO2_3_0p850,    // VCC_0p850,
    0xFF,    // VCC_0p875,
    LDO2_3_0p900,    // VCC_0p900,
    0xFF,    // VCC_0p925,
    LDO2_3_0p950,    // VCC_0p950,
    0xFF,    // VCC_0p975,
    LDO2_3_1p000,    // VCC_1p000,
    0xFF,    // VCC_1p025,
    LDO2_3_1p050,    // VCC_1p050,
    0xFF,    // VCC_1p075,
    LDO2_3_1p100,    // VCC_1p100,
    0xFF,    // VCC_1p125,
    LDO2_3_1p150,    // VCC_1p150,
    0xFF,    // VCC_1p175,
    LDO2_3_1p200,    // VCC_1p200,
    0xFF,    // VCC_1p225,
    LDO2_3_1p250,    // VCC_1p250,
    0xFF,    // VCC_1p275,
    LDO2_3_1p300,    // VCC_1p300,
    0xFF,    // VCC_1p325
    0xFF,    // VCC_1p350,
    0xFF,    // VCC_1p375,
    0xFF,    // VCC_1p400,
    0xFF,    // VCC_1p425,
    0xFF,    // VCC_1p450,
    0xFF,    // VCC_1p475,
    0xFF,   // VCC_1p500,
    0xFF,   // VCC_1p600,
    0xFF,   // VCC_1p700,
    0xFF,   // VCC_1p800,
    0xFF,   // VCC_1p900,
    0xFF,   // VCC_2p000,
    0xFF,   // VCC_2p100,
    0xFF,   // VCC_2p200,
    0xFF,   // VCC_2p300,
    0xFF,    // VCC_2p400,
    0xFF,    // VCC_2p500,
    0xFF,    // VCC_2p600,
    0xFF,    // VCC_2p700,
    0xFF,    // VCC_2p800,
    0xFF,    // VCC_2p900,
    0xFF,    // VCC_3p000,
    0xFF,    // VCC_3p100,
    0xFF,    // VCC_3p200,
    0xFF,    // VCC_3p300,
    0xFF,    // VCC_3p400,
    0xFF,    // VCC_3p500,
    0xFF    // VCC_3p600,
};

const byte buck3_ldo4to7_11_17_voltage[VCC_MAX] = {
    0xFF,    // VCC_0p750 = 0, // 0 0.75V
    0xFF,    // VCC_0p775,
    0xFF,    // VCC_0p800,
    0xFF,    // VCC_0p825,
    0xFF,    // VCC_0p850,
    0xFF,    // VCC_0p875,
    0xFF,    // VCC_0p900,
    0xFF,    // VCC_0p925,
    0xFF,    // VCC_0p950,
    0xFF,    // VCC_0p975,
    0xFF,    // VCC_1p000,
    0xFF,    // VCC_1p025,
    0xFF,    // VCC_1p050,
    0xFF,    // VCC_1p075,
    0xFF,    // VCC_1p100,
    0xFF,    // VCC_1p125,
    0xFF,    // VCC_1p150,
    0xFF,    // VCC_1p175,
    0xFF,    // VCC_1p200,
    0xFF,    // VCC_1p225,
    0xFF,    // VCC_1p250,
    0xFF,    // VCC_1p275,
    0xFF,    // VCC_1p300,
    0xFF,    // VCC_1p325
    0xFF,    // VCC_1p350,
    0xFF,    // VCC_1p375,
    0xFF,    // VCC_1p400,
    0xFF,    // VCC_1p425,
    0xFF,    // VCC_1p450,
    0xFF,    // VCC_1p475,
    0xFF,   // VCC_1p500,
    BUCK3_LDO4TO7_11_17_1p600,   // VCC_1p600,
    BUCK3_LDO4TO7_11_17_1p700,   // VCC_1p700,
    BUCK3_LDO4TO7_11_17_1p800,   // VCC_1p800,
    BUCK3_LDO4TO7_11_17_1p900,   // VCC_1p900,
    BUCK3_LDO4TO7_11_17_2p000,   // VCC_2p000,
    BUCK3_LDO4TO7_11_17_2p100,   // VCC_2p100,
    BUCK3_LDO4TO7_11_17_2p200,   // VCC_2p200,
    BUCK3_LDO4TO7_11_17_2p300,   // VCC_2p300,
    BUCK3_LDO4TO7_11_17_2p400,    // VCC_2p400,
    BUCK3_LDO4TO7_11_17_2p500,    // VCC_2p500,
    BUCK3_LDO4TO7_11_17_2p600,    // VCC_2p600,
    BUCK3_LDO4TO7_11_17_2p700,    // VCC_2p700,
    BUCK3_LDO4TO7_11_17_2p800,    // VCC_2p800,
    BUCK3_LDO4TO7_11_17_2p900,    // VCC_2p900,
    BUCK3_LDO4TO7_11_17_3p000,    // VCC_3p000,
    BUCK3_LDO4TO7_11_17_3p100,    // VCC_3p100,
    BUCK3_LDO4TO7_11_17_3p200,    // VCC_3p200,
    BUCK3_LDO4TO7_11_17_3p300,    // VCC_3p300,
    BUCK3_LDO4TO7_11_17_3p400,    // VCC_3p400,
    BUCK3_LDO4TO7_11_17_3p500,    // VCC_3p500,
    BUCK3_LDO4TO7_11_17_3p600    // VCC_3p600,
};

const byte ldo8_voltage[VCC_MAX] = {
    0xFF,    // VCC_0p750 = 0, // 0 0.75V
    0xFF,    // VCC_0p775,
    0xFF,    // VCC_0p800,
    0xFF,    // VCC_0p825,
    0xFF,    // VCC_0p850,
    0xFF,    // VCC_0p875,
    0xFF,    // VCC_0p900,
    0xFF,    // VCC_0p925,
    0xFF,    // VCC_0p950,
    0xFF,    // VCC_0p975,
    0xFF,    // VCC_1p000,
    0xFF,    // VCC_1p025,
    0xFF,    // VCC_1p050,
    0xFF,    // VCC_1p075,
    0xFF,    // VCC_1p100,
    0xFF,    // VCC_1p125,
    0xFF,    // VCC_1p150,
    0xFF,    // VCC_1p175,
    0xFF,    // VCC_1p200,
    0xFF,    // VCC_1p225,
    0xFF,    // VCC_1p250,
    0xFF,    // VCC_1p275,
    0xFF,    // VCC_1p300,
    0xFF,    // VCC_1p325
    0xFF,    // VCC_1p350,
    0xFF,    // VCC_1p375,
    0xFF,    // VCC_1p400,
    0xFF,    // VCC_1p425,
    0xFF,    // VCC_1p450,
    0xFF,    // VCC_1p475,
    0xFF,   // VCC_1p500,
    0xFF,   // VCC_1p600,
    0xFF,   // VCC_1p700,
    0xFF,   // VCC_1p800,
    0xFF,   // VCC_1p900,
    0xFF,   // VCC_2p000,
    0xFF,   // VCC_2p100,
    0xFF,   // VCC_2p200,
    0xFF,   // VCC_2p300,
    0xFF,    // VCC_2p400,
    0xFF,    // VCC_2p500,
    0xFF,    // VCC_2p600,
    0xFF,    // VCC_2p700,
    0xFF,    // VCC_2p800,
    0xFF,    // VCC_2p900,
    LDO8_3p000,    // VCC_3p000,
    LDO8_3p100,    // VCC_3p100,
    LDO8_3p200,    // VCC_3p200,
    LDO8_3p300,    // VCC_3p300,
    LDO8_3p400,    // VCC_3p400,
    LDO8_3p500,    // VCC_3p500,
    LDO8_3p600    // VCC_3p600,
};

const byte ldo9_voltage[VCC_MAX] = {
    0xFF,    // VCC_0p750 = 0, // 0 0.75V
    0xFF,    // VCC_0p775,
    0xFF,    // VCC_0p800,
    0xFF,    // VCC_0p825,
    0xFF,    // VCC_0p850,
    0xFF,    // VCC_0p875,
    0xFF,    // VCC_0p900,
    0xFF,    // VCC_0p925,
    0xFF,    // VCC_0p950,
    0xFF,    // VCC_0p975,
    0xFF,    // VCC_1p000,
    0xFF,    // VCC_1p025,
    0xFF,    // VCC_1p050,
    0xFF,    // VCC_1p075,
    0xFF,    // VCC_1p100,
    0xFF,    // VCC_1p125,
    0xFF,    // VCC_1p150,
    0xFF,    // VCC_1p175,
    0xFF,    // VCC_1p200,
    0xFF,    // VCC_1p225,
    0xFF,    // VCC_1p250,
    0xFF,    // VCC_1p275,
    0xFF,    // VCC_1p300,
    0xFF,    // VCC_1p325
    0xFF,    // VCC_1p350,
    0xFF,    // VCC_1p375,
    0xFF,    // VCC_1p400,
    0xFF,    // VCC_1p425,
    0xFF,    // VCC_1p450,
    0xFF,    // VCC_1p475,
    0xFF,   // VCC_1p500,
    0xFF,   // VCC_1p600,
    0xFF,   // VCC_1p700,
    0xFF,   // VCC_1p800,
    0xFF,   // VCC_1p900,
    0xFF,   // VCC_2p000,
    0xFF,   // VCC_2p100,
    0xFF,   // VCC_2p200,
    0xFF,   // VCC_2p300,
    0xFF,    // VCC_2p400,
    0xFF,    // VCC_2p500,
    0xFF,    // VCC_2p600,
    0xFF,    // VCC_2p700,
    LDO9_2p800,    // VCC_2p800,
    LDO9_2p900,    // VCC_2p900,
    LDO9_3p000,    // VCC_3p000,
    LDO9_3p100,    // VCC_3p100,
    0xFF,    // VCC_3p200,
    0xFF,    // VCC_3p300,
    0xFF,    // VCC_3p400,
    0xFF,    // VCC_3p500,
    0xFF    // VCC_3p600,
};

const byte ldo10_voltage[VCC_MAX] = {
    0xFF,    // VCC_0p750 = 0, // 0 0.75V
    0xFF,    // VCC_0p775,
    0xFF,    // VCC_0p800,
    0xFF,    // VCC_0p825,
    0xFF,    // VCC_0p850,
    0xFF,    // VCC_0p875,
    0xFF,    // VCC_0p900,
    0xFF,    // VCC_0p925,
    LDO10_0p950,    // VCC_0p950,
    0xFF,    // VCC_0p975,
    LDO10_1p000,    // VCC_1p000,
    0xFF,    // VCC_1p025,
    LDO10_1p050,    // VCC_1p050,
    0xFF,    // VCC_1p075,
    LDO10_1p100,    // VCC_1p100,
    0xFF,    // VCC_1p125,
    LDO10_1p150,    // VCC_1p150,
    0xFF,    // VCC_1p175,
    LDO10_1p200,    // VCC_1p200,
    0xFF,    // VCC_1p225,
    LDO10_1p250,    // VCC_1p250,
    0xFF,    // VCC_1p275,
    LDO10_1p300,    // VCC_1p300,
    0xFF,    // VCC_1p325
    0xFF,    // VCC_1p350,
    0xFF,    // VCC_1p375,
    0xFF,    // VCC_1p400,
    0xFF,    // VCC_1p425,
    0xFF,    // VCC_1p450,
    0xFF,    // VCC_1p475,
    0xFF,   // VCC_1p500,
    0xFF,   // VCC_1p600,
    0xFF,   // VCC_1p700,
    0xFF,   // VCC_1p800,
    0xFF,   // VCC_1p900,
    0xFF,   // VCC_2p000,
    0xFF,   // VCC_2p100,
    0xFF,   // VCC_2p200,
    0xFF,   // VCC_2p300,
    0xFF,    // VCC_2p400,
    0xFF,    // VCC_2p500,
    0xFF,    // VCC_2p600,
    0xFF,    // VCC_2p700,
    0xFF,    // VCC_2p800,
    0xFF,    // VCC_2p900,
    0xFF,    // VCC_3p000,
    0xFF,    // VCC_3p100,
    0xFF,    // VCC_3p200,
    0xFF,    // VCC_3p300,
    0xFF,    // VCC_3p400,
    0xFF,    // VCC_3p500,
    0xFF    // VCC_3p600,
};

const byte ldo12_13_voltage[VCC_MAX] = {
    0xFF,    // VCC_0p750 = 0, // 0 0.75V
    0xFF,    // VCC_0p775,
    LDO12_13_0p800,    // VCC_0p800,
    0xFF,    // VCC_0p825,
    0xFF,    // VCC_0p850,
    0xFF,    // VCC_0p875,
    LDO12_13_0p900,    // VCC_0p900,
    0xFF,    // VCC_0p925,
    0xFF,    // VCC_0p950,
    0xFF,    // VCC_0p975,
    LDO12_13_1p000,    // VCC_1p000,
    0xFF,    // VCC_1p025,
    0xFF,    // VCC_1p050,
    0xFF,    // VCC_1p075,
    LDO12_13_1p100,    // VCC_1p100,
    0xFF,    // VCC_1p125,
    0xFF,    // VCC_1p150,
    0xFF,    // VCC_1p175,
    LDO12_13_1p200,    // VCC_1p200,
    0xFF,    // VCC_1p225,
    0xFF,    // VCC_1p250,
    0xFF,    // VCC_1p275,
    LDO12_13_1p300,    // VCC_1p300,
    0xFF,    // VCC_1p325
    0xFF,    // VCC_1p350,
    0xFF,    // VCC_1p375,
    LDO12_13_1p400,    // VCC_1p400,
    0xFF,    // VCC_1p425,
    0xFF,    // VCC_1p450,
    0xFF,    // VCC_1p475,
    LDO12_13_1p500,   // VCC_1p500,
    LDO12_13_1p600,   // VCC_1p600,
    LDO12_13_1p700,   // VCC_1p700,
    LDO12_13_1p800,   // VCC_1p800,
    LDO12_13_1p900,   // VCC_1p900,
    LDO12_13_2p000,   // VCC_2p000,
    LDO12_13_2p100,   // VCC_2p100,
    LDO12_13_2p200,   // VCC_2p200,
    LDO12_13_2p300,   // VCC_2p300,
    LDO12_13_2p400,    // VCC_2p400,
    LDO12_13_2p500,    // VCC_2p500,
    LDO12_13_2p600,    // VCC_2p600,
    LDO12_13_2p700,    // VCC_2p700,
    LDO12_13_2p800,    // VCC_2p800,
    LDO12_13_2p900,    // VCC_2p900,
    LDO12_13_3p000,    // VCC_3p000,
    LDO12_13_3p100,    // VCC_3p100,
    LDO12_13_3p200,    // VCC_3p200,
    LDO12_13_3p300,    // VCC_3p300,
    0xFF,    // VCC_3p400,
    0xFF,    // VCC_3p500,
    0xFF    // VCC_3p600,
};

const byte ldo14_15_voltage[VCC_MAX] = {
    0xFF,    // VCC_0p750 = 0, // 0 0.75V
    0xFF,    // VCC_0p775,
    0xFF,    // VCC_0p800,
    0xFF,    // VCC_0p825,
    0xFF,    // VCC_0p850,
    0xFF,    // VCC_0p875,
    0xFF,    // VCC_0p900,
    0xFF,    // VCC_0p925,
    0xFF,    // VCC_0p950,
    0xFF,    // VCC_0p975,
    0xFF,    // VCC_1p000,
    0xFF,    // VCC_1p025,
    0xFF,    // VCC_1p050,
    0xFF,    // VCC_1p075,
    0xFF,    // VCC_1p100,
    0xFF,    // VCC_1p125,
    0xFF,    // VCC_1p150,
    0xFF,    // VCC_1p175,
    LDO14_15_1p200,    // VCC_1p200,
    0xFF,    // VCC_1p225,
    0xFF,    // VCC_1p250,
    0xFF,    // VCC_1p275,
    LDO14_15_1p300,    // VCC_1p300,
    0xFF,    // VCC_1p325
    0xFF,    // VCC_1p350,
    0xFF,    // VCC_1p375,
    LDO14_15_1p400,    // VCC_1p400,
    0xFF,    // VCC_1p425,
    0xFF,    // VCC_1p450,
    0xFF,    // VCC_1p475,
    LDO14_15_1p500,   // VCC_1p500,
    LDO14_15_1p600,   // VCC_1p600,
    LDO14_15_1p700,   // VCC_1p700,
    LDO14_15_1p800,   // VCC_1p800,
    LDO14_15_1p900,   // VCC_1p900,
    LDO14_15_2p000,   // VCC_2p000,
    LDO14_15_2p100,   // VCC_2p100,
    LDO14_15_2p200,   // VCC_2p200,
    LDO14_15_2p300,   // VCC_2p300,
    LDO14_15_2p400,    // VCC_2p400,
    LDO14_15_2p500,    // VCC_2p500,
    LDO14_15_2p600,    // VCC_2p600,
    LDO14_15_2p700,    // VCC_2p700,
    LDO14_15_2p800,    // VCC_2p800,
    LDO14_15_2p900,    // VCC_2p900,
    LDO14_15_3p000,    // VCC_3p000,
    LDO14_15_3p100,    // VCC_3p100,
    LDO14_15_3p200,    // VCC_3p200,
    LDO14_15_3p300,    // VCC_3p300,
    0xFF,    // VCC_3p400,
    0xFF,    // VCC_3p500,
    0xFF    // VCC_3p600,
};


/*===========================================================================

      P O W E R     M A N A G E M E N T     S E C T I O N

===========================================================================*/
/*===========================================================================

FUNCTION Set_MAX8998_PM_REG                                

DESCRIPTION
    This function write the value at the selected register in the PM section.

INPUT PARAMETERS
    reg_num :   selected register in the register address.
    value   :   the value for reg_num.

RETURN VALUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 
    Set_MAX8998_PM_REG(CHGENB, onoff);

===========================================================================*/
boolean Set_MAX8998_PM_REG(max8998_pm_section_type reg_num, byte value)
{
	byte reg_buff;

	if(!pmic_init)
	{
		printk("[PMIC] %s - MAX8998 initialization is not finished yet!!\n", __func__);
		return FALSE;
	}

	if(reg_num >= ENDOFPM)
	{       
		pr_err(PREFIX "%s:E: invalid reg_num: %d\n", __func__, reg_num);        
		return FALSE;
	}

	if(pmic_read(I2C_SLAVE_ADDR_PM_MAX8998, max8998pm[reg_num].addr, &reg_buff, (byte)1) != PMIC_PASS)
	{
		// Error - I2C read error
		pr_err(PREFIX "%s:E: I2C read error \n", __func__);         
		return FALSE; // return error
	}

	reg_buff = (reg_buff & max8998pm[reg_num].clear) | (value << max8998pm[reg_num].shift);
	if(pmic_write(I2C_SLAVE_ADDR_PM_MAX8998, max8998pm[reg_num].addr, &reg_buff, (byte)1) != PMIC_PASS)
	{
		pr_err(PREFIX "%s:E: I2C write error \n", __func__);         
		return FALSE;
	}

	return TRUE;
}
EXPORT_SYMBOL(Set_MAX8998_PM_REG);


/*===========================================================================

FUNCTION Get_MAX8998_PM_REG                                

DESCRIPTION
    This function read the value at the selected register in the PM section.

INPUT PARAMETERS
    reg_num :   selected register in the register address.

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE
    reg_buff :  the value of selected register.
                reg_buff is aligned to the right side of the return value.

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
byte Get_MAX8998_PM_REG(max8998_pm_section_type reg_num, byte *reg_buff)
{
	byte temp_buff;

	if(!pmic_init)
	{
		printk("[PMIC] %s - MAX8998 initialization is not finished yet!!\n", __func__);
		return FALSE;
	}

	if(pmic_read(I2C_SLAVE_ADDR_PM_MAX8998, max8998pm[reg_num].addr, &temp_buff, (byte)1) != PMIC_PASS)
	{        
		pr_err(PREFIX "%s:E: I2C read error \n", __func__); 
		return FALSE; // return error
	}

	*reg_buff = (byte)((temp_buff & max8998pm[reg_num].mask) >> max8998pm[reg_num].shift);
	return TRUE;
}
EXPORT_SYMBOL(Get_MAX8998_PM_REG);


/*===========================================================================

FUNCTION Set_MAX8998_PM_ADDR                                

DESCRIPTION
    This function write the value at the selected register address
    in the PM section.

INPUT PARAMETERS
    byte reg_addr    : the register address.
    byte *reg_buff   : the array for data of register to write.
 	byte length      : the number of the register 

RETURN VALUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8998_PM_ADDR(byte reg_addr, byte *reg_buff, byte length)
{
    if(reg_addr > ID_REG)
    {
		// Error - Invalid Write Register
		return FALSE;
    }

    if(pmic_write(I2C_SLAVE_ADDR_PM_MAX8998, (byte)reg_addr, reg_buff, length) != PMIC_PASS)
    {
        // Error - I2C write error
		return FALSE;
    }

	return TRUE;
}


/*===========================================================================

FUNCTION Get_MAX8998_PM_ADDR                                

DESCRIPTION
    This function read the value at the selected register address
    in the PM section.

INPUT PARAMETERS
    byte reg_addr   : the register address.
    byte *reg_buff  : the array for data of register to write.
 	byte length     : the number of the register 

RETURN VALUE
    byte *reg_buff : the pointer parameter for data of sequential registers
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Get_MAX8998_PM_ADDR(byte reg_addr, byte *reg_buff, byte length)
{
    if(reg_addr > ID_REG)
    {
        // Error - Invalid Write Register
		return FALSE; // return error;
    }
    if(pmic_read(I2C_SLAVE_ADDR_PM_MAX8998, (byte)reg_addr, reg_buff, length) != PMIC_PASS)
    {
        // Error - I2C write error
		return FALSE; // return error
    }

	return TRUE;
}


// DVSARM voltage control function
boolean change_vcc_arm(int voltage)
{
	byte reg_value = 0;

	if (voltage < VCC_MIN_VOLTAGE || voltage > VCC_MAX_VOLTAGE) {
		printk("[PMIC] %s : invalid voltage(%d)\n", __func__, voltage);
		return FALSE;
	}

	if (voltage % 25) {
		printk("[PMIC] %s : invalid voltage(%d)\n", __func__, voltage);
		return FALSE;
	}

	reg_value = (byte)((voltage -750) / 25);
//	printk("[PMIC] %s : voltage(%d), reg_value(0x%02x)\n", __func__, voltage, reg_value);

	if (!Set_MAX8998_PM_REG(DVSARM1, reg_value)) {
		printk("[PMIC] %s : DVSARM voltage setting failed(%d)\n", __func__, reg_value);
		return FALSE;
	}

	return TRUE;
}
EXPORT_SYMBOL(change_vcc_arm);


// DVSINT voltage control function
boolean change_vcc_internal(int voltage)
{
	byte reg_value = 0;

	if (voltage < VCC_MIN_VOLTAGE || voltage > VCC_MAX_VOLTAGE) {
		printk("[PMIC] %s : invalid voltage(%d)\n", __func__, voltage);
		return FALSE;
	}

	if (voltage % 25) {
		printk("[PMIC] %s : invalid voltage(%d)\n", __func__, voltage);
		return FALSE;
	}

	reg_value = (byte)((voltage -750) / 25);
//	printk("[PMIC] %s : voltage(%d), reg_value(0x%02x)\n", __func__, voltage, reg_value);

	if (!Set_MAX8998_PM_REG(DVSINT1, reg_value)) {
		printk("[PMIC] %s : DVSARM voltage setting failed(%d)\n", __func__, reg_value);
		return FALSE;
	}

	return TRUE;
}
EXPORT_SYMBOL(change_vcc_internal);



/*===========================================================================

FUNCTION Set_MAX8998_PM_ONOFF_CNTL                                

DESCRIPTION
    Control ON/OFF register for Bucks, LDOs, PWRHOLD, BATTMON, LBCNFGs,
    32kHzAP, 32kHzCP and VICHG

INPUT PARAMETERS
    byte onoff_reg  : selected register (ONOFF1_REG, ONOFF2_REG, ONOFF3_REG or ONOFF4_REG)
    byte cntl_mask  : mask bit, selected LDO and BUCK
    byte status     : turn on or off (0 - off, 1 - on)

RETURN VALUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 
    Turn on LDO5 and BUCK1:

    Set_MAX8998_PM_ONOFF_CNTL(ONOFF1_REG, (ONOFF1_EN1_M | ONOFF1_ELDO5_M), 1);

===========================================================================*/
void Set_MAX8998_PM_ONOFF_CNTL(byte onoff_reg, byte cntl_item, byte status)
{
    byte reg_value;
    byte set_value;

    if( (onoff_reg < ONOFF1_REG) || (onoff_reg > ONOFF4_REG) )
    {
        // Error - Invalid onoff control
        return; // return error
    }

    if (status == 0)
    {
        // off condition
        set_value = 0;
    }
    else if (status == 1)
    {
        // on condition
        set_value = cntl_item;
    }
    else
    {
      // Error - this condition is not defined
      return;
    }

    Get_MAX8998_PM_ADDR(onoff_reg, &reg_value, 1);

    reg_value = ((reg_value & ~cntl_item) | set_value);

    Set_MAX8998_PM_ADDR(onoff_reg, &reg_value, 1);
}


/*===========================================================================

FUNCTION Set_MAX8998_PM_ADISCHG_EN                                

DESCRIPTION
    Control Active discharge for BUCKs, LDOs, and SAFEOUTs

INPUT PARAMETERS
    byte onoff_reg  : selected register (ADISCHG_EN1_REG, ADISCHG_EN2_REG or ADISCHG_EN3_REG)
    byte cntl_mask  : mask bit, selected LDO and BUCK
    byte status     : turn on or off (0 - disable, 1 - enable)
  
RETURN VALUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 
    Set_MAX8998_PM_ADISCHG_EN(SD1DIS_M, 1);

===========================================================================*/
void Set_MAX8998_PM_ADISCHG_EN(byte adischg_en_reg, byte cntl_item, byte status)
{
    byte reg_value;
    byte set_value;

	if( (adischg_en_reg < ADISCHG_EN1_REG) || (adischg_en_reg > ADISCHG_EN3_REG) )
    {
        // Error - Invalid onoff control
        return; // return error
    }

    if (status == 0)
    {
        // off condition
        set_value = 0;
    }
    else if (status == 1)
    {
        // on condition
        set_value = cntl_item;
    }
    else
    {
      // Error - this condition is not defined
      return;
    }

	Get_MAX8998_PM_ADDR(adischg_en_reg, &reg_value, 1);

    reg_value = ((reg_value & ~cntl_item) | set_value);

	Set_MAX8998_PM_ADDR(adischg_en_reg, &reg_value, 1);
}


/*===========================================================================

FUNCTION Set_MAX8998_PM_RAMP_RATE

DESCRIPTION
    Control ramp rate when buck uses DVS.

INPUT PARAMETERS
    ramp_rate_type ramp_rate : ramp rate (from RAMP_1mVpus to RAMP_12mVpus)
  
RETURN VALUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 
    Set_MAX8998_PM_SET_RAMP_RATE(RAMP_11mVpus);

===========================================================================*/
void Set_MAX8998_PM_RAMP_RATE(ramp_rate_type ramp_rate)
{
    if (ramp_rate > RAMP_MAX)
    {
        // Error - Invalid value
        return;
    }

    Set_MAX8998_PM_REG(RAMP, (byte)ramp_rate);
}


/*===========================================================================

FUNCTION Set_MAX8998_PM_BUCK1_2_Voltage                                

DESCRIPTION
    Control Output program register for BUCK1,2

INPUT PARAMETERS
    max8998_pm_section_type ldo_num : Register number
    out_voltage_type set_value      : Output voltage

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8998_PM_BUCK1_2_Voltage(max8998_pm_section_type ldo_num, out_voltage_type set_value)
{
    boolean ret_val;

    if ((byte)set_value > (byte)VCC_1p500) {
        // Invalid voltage for Buck1,2
        return FALSE;
    }
    ret_val = Set_MAX8998_PM_REG(ldo_num, (byte)set_value);
    return ret_val;
}


/*===========================================================================

FUNCTION Set_MAX8998_PM_BUCK4_Voltage                                

DESCRIPTION
    Control Output program register for BUCK4

INPUT PARAMETERS
    max8998_pm_section_type ldo_num : Register number
    out_voltage_type set_value      : Output voltage

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8998_PM_BUCK4_Voltage(max8998_pm_section_type ldo_num, out_voltage_type set_value)
{
    boolean ret_val;

    if (buck4_voltage[set_value] == 0xFF) {
        // Invalid voltage for Buck1,2
        return FALSE;
    }
    ret_val = Set_MAX8998_PM_REG(ldo_num, (byte)buck4_voltage[set_value]);
    return ret_val;
}


/*===========================================================================

FUNCTION Set_MAX8998_PM_LDO2_3_Voltage                                

DESCRIPTION
    Control Output program register for LDO2,3

INPUT PARAMETERS
    max8998_pm_section_type ldo_num : Register number
    out_voltage_type set_value      : Output voltage

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8998_PM_LDO2_3_Voltage(max8998_pm_section_type ldo_num, out_voltage_type set_value)
{
    boolean ret_val;

    if (ldo2_3_voltage[set_value] == 0xFF) {
        // Invalid voltage for Buck1,2
        return FALSE;
    }
    ret_val = Set_MAX8998_PM_REG(ldo_num, (byte)ldo2_3_voltage[set_value]);
    return ret_val;
}


/*===========================================================================

FUNCTION Set_MAX8998_PM_BUCK3_LDO4TO7_11_17_Voltage                                

DESCRIPTION
    Control Output program register for BUCK3, LDO4~7,11,17

INPUT PARAMETERS
    max8998_pm_section_type ldo_num : Register number
    out_voltage_type set_value      : Output voltage

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8998_PM_BUCK3_LDO4TO7_11_17_Voltage(max8998_pm_section_type ldo_num, out_voltage_type set_value)
{
    boolean ret_val;

    if (buck3_ldo4to7_11_17_voltage[set_value] == 0xFF) {
        // Invalid voltage for Buck1,2
        return FALSE;
    }
    ret_val = Set_MAX8998_PM_REG(ldo_num, (byte)buck3_ldo4to7_11_17_voltage[set_value]);
    return ret_val;
}


/*===========================================================================

FUNCTION Set_MAX8998_PM_LDO8_Voltage                                

DESCRIPTION
    Control Output program register for LDO8

INPUT PARAMETERS
    max8998_pm_section_type ldo_num : Register number
    out_voltage_type set_value      : Output voltage

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8998_PM_LDO8_Voltage(max8998_pm_section_type ldo_num, out_voltage_type set_value)
{
    boolean ret_val;

    if (ldo8_voltage[set_value] == 0xFF) {
        // Invalid voltage for Buck1,2
        return FALSE;
    }
    ret_val = Set_MAX8998_PM_REG(ldo_num, (byte)ldo8_voltage[set_value]);
    return ret_val;
}


/*===========================================================================

FUNCTION Set_MAX8998_PM_LDO9_Voltage                                

DESCRIPTION
    Control Output program register for LDO9

INPUT PARAMETERS
    max8998_pm_section_type ldo_num : Register number
    out_voltage_type set_value      : Output voltage

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8998_PM_LDO9_Voltage(max8998_pm_section_type ldo_num, out_voltage_type set_value)
{
    boolean ret_val;

    if (ldo9_voltage[set_value] == 0xFF) {
        // Invalid voltage for Buck1,2
        return FALSE;
    }
    ret_val = Set_MAX8998_PM_REG(ldo_num, (byte)ldo9_voltage[set_value]);
    return ret_val;
}


/*===========================================================================

FUNCTION Set_MAX8998_PM_LDO10_Voltage                                

DESCRIPTION
    Control Output program register for LDO10

INPUT PARAMETERS
    max8998_pm_section_type ldo_num : Register number
    out_voltage_type set_value      : Output voltage

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8998_PM_LDO10_Voltage(max8998_pm_section_type ldo_num, out_voltage_type set_value)
{
    boolean ret_val;

    if (ldo10_voltage[set_value] == 0xFF) {
        // Invalid voltage for Buck1,2
        return FALSE;
    }
    ret_val = Set_MAX8998_PM_REG(ldo_num, (byte)ldo10_voltage[set_value]);
    return ret_val;
}


/*===========================================================================

FUNCTION Set_MAX8998_PM_LDO12_13_Voltage                                

DESCRIPTION
    Control Output program register for LDO12,13

INPUT PARAMETERS
    max8998_pm_section_type ldo_num : Register number
    out_voltage_type set_value      : Output voltage

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8998_PM_LDO12_13_Voltage(max8998_pm_section_type ldo_num, out_voltage_type set_value)
{
    boolean ret_val;

    if (ldo12_13_voltage[set_value] == 0xFF) {
        // Invalid voltage for Buck1,2
        return FALSE;
    }
    ret_val = Set_MAX8998_PM_REG(ldo_num, (byte)ldo12_13_voltage[set_value]);
    return ret_val;
}


/*===========================================================================

FUNCTION Set_MAX8998_PM_LDO14_15_Voltage                                

DESCRIPTION
    Control Output program register for LDO14, 15

INPUT PARAMETERS
    max8998_pm_section_type ldo_num : Register number
    out_voltage_type set_value      : Output voltage

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8998_PM_LDO14_15_Voltage(max8998_pm_section_type ldo_num, out_voltage_type set_value)
{
    boolean ret_val;

    if (ldo14_15_voltage[set_value] == 0xFF) {
        // Invalid voltage for Buck1,2
        return FALSE;
    }
    ret_val = Set_MAX8998_PM_REG(ldo_num, (byte)ldo14_15_voltage[set_value]);
    return ret_val;
}


/*===========================================================================

FUNCTION Set_MAX8998_PM_OUTPUT_Voltage                                

DESCRIPTION
    Control Output program register for OUT1 - OUT19 except OUT12 and OUT13

INPUT PARAMETERS
    max8998_pm_section_type ldo_num : Register number
    out_voltage_type set_value      : Output voltage

RETURN VALUE
    boolean : 0 = FALSE
              1 = TRUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
boolean Set_MAX8998_PM_OUTPUT_Voltage(max8998_pm_section_type ldo_num, out_voltage_type set_value)
{
    boolean ret_val;

    if( (ldo_num < DVSARM1) || (ldo_num > LDO17) )
    {
        // Invalid buck or ldo register
        return FALSE; // return error
    }

    switch(ldo_num) {
    case DVSARM1:
    case DVSARM2:
    case DVSARM3:
    case DVSARM4:
    case DVSINT1:
    case DVSINT2:
        ret_val = Set_MAX8998_PM_BUCK1_2_Voltage(ldo_num, set_value);
        break;
    case BUCK4:
        ret_val = Set_MAX8998_PM_BUCK4_Voltage(ldo_num, set_value);
        break;
    case LDO2:
    case LDO3:
        ret_val = Set_MAX8998_PM_LDO2_3_Voltage(ldo_num, set_value);
        break;
    case BUCK3:
    case LDO4:
    case LDO5:
    case LDO6:
    case LDO7:
    case LDO11:
    case LDO17:
        ret_val = Set_MAX8998_PM_BUCK3_LDO4TO7_11_17_Voltage(ldo_num, set_value);
        break;
    case LDO8:
        ret_val = Set_MAX8998_PM_LDO8_Voltage(ldo_num, set_value);
        break;
    case LDO9:
        ret_val = Set_MAX8998_PM_LDO9_Voltage(ldo_num, set_value);
        break;
    case LDO10:
        ret_val = Set_MAX8998_PM_LDO10_Voltage(ldo_num, set_value);
        break;
    case LDO12:
    case LDO13:
        ret_val = Set_MAX8998_PM_LDO12_13_Voltage(ldo_num, set_value);
        break;
    case LDO14:
    case LDO15:
        ret_val = Set_MAX8998_PM_LDO14_15_Voltage(ldo_num, set_value);
        break;
    default:
        ret_val = FALSE;
        // Invalid BUCK or LDO number
        break;
    }
    return ret_val;
}


/*===========================================================================

FUNCTION Set_MAX8998_PM_LBCNFG

DESCRIPTION
    Control low battery configuration.

INPUT PARAMETERS
    max8998_pm_section_type cntl_item : control item (LBHYST1,2 or LBTH1,2)
    lbcnfg_type             lbcnfg    : control value
                                        (LBHYST_xxx or LBTH_xxx)
RETURN VALUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 
    

===========================================================================*/
void Set_MAX8998_PM_LBCNFG(max8998_pm_section_type cntl_item, lbcnfg_type lbcnfg)
{
    byte set_value;

    if ((cntl_item < LBHYST1) || (cntl_item > LBTH2))
    {
        // Error - Invalid register
        return;
    }

    if ( (cntl_item == LBTH1) || (cntl_item == LBTH2) )
    {
        set_value = (byte) lbcnfg;
    }
    else
    {
        set_value = (byte) lbcnfg - LBTH_2p9V;
    }

    Set_MAX8998_PM_REG(cntl_item, set_value);
}


/*===========================================================================*/
boolean set_pmic(pmic_pm_type pm_type, int value)
{
	boolean rc = FALSE;
	switch (pm_type) {
	case VCC_ARM:
		rc = change_vcc_arm(value);
		break;
	case VCC_INT:
		rc = change_vcc_internal(value);
		break;
	default:
		pr_err(PREFIX "%s:E: invalid pm_type: %d\n", __func__, pm_type);
		rc = FALSE;
		break;
	}
	return rc;
}

boolean get_pmic(pmic_pm_type pm_type, int *value)
{
	boolean rc = FALSE;
	byte reg_buff;
	*value = 0;

	switch (pm_type) {
	case VCC_ARM:
		if(! Get_MAX8998_PM_REG(DVSARM1, &reg_buff)) {
			pr_err(PREFIX "%s:VCC_ARM: get pmic reg fail\n",
					__func__);
			return FALSE;
		}
		*value = (reg_buff * 25 + 750);
		break;
	case VCC_INT:
		if(!Get_MAX8998_PM_REG(DVSINT1, &reg_buff))
		{
			pr_err(PREFIX "%s:VCC_INT: get pmic reg fail\n",
					__func__);
			return FALSE;
		}
		*value = (reg_buff * 25 + 750);
		break;
	default:
		pr_err(PREFIX "%s:E: invalid pm_type: %d\n", __func__, pm_type);
		rc = FALSE;
		break;
	}
        return rc;
}

/*===========================================================================

      I N I T    R O U T I N E

===========================================================================*/
extern void pmic_gpio_init(void);

void camera_ldo_control(int onoff)
{
	if(onoff)
	{
		Set_MAX8998_PM_REG(EN4, 1); // CAM_D_1.2V
		mdelay(200);
		//[sm.kim 2009.12.15 camera doesn't need these ldos - apollo new emul
		//Set_MAX8998_PM_REG(ELDO11, 1); //Set_MAX8998_PM_REG(ELDO11, 1);
		//Set_MAX8998_PM_REG(ELDO12, 1); //CAM_D_1.8V
		//Set_MAX8998_PM_REG(ELDO13, 1); //CAM_A_2.8V
		//Set_MAX8998_PM_REG(ELDO14, 1); //CAM_IO_1.8V
		//]
		Set_MAX8998_PM_REG(ELDO15, 1); //CAM_D_2.8V
		//Set_MAX8998_PM_REG(ELDO17, 1); //VMOT_3.3V	- apollo new emul

	}
	else
	{
		//Set_MAX8998_PM_REG(ELDO17, 0); //VMOT_3.3V	- apollo new emul
		//[sm.kim 2009.12.15 camera doesn't need these ldos - apollo new emul
		//Set_MAX8998_PM_REG(ELDO11, 0); //Set_MAX8998_PM_REG(ELDO11, 1);
		//Set_MAX8998_PM_REG(ELDO12, 0); //CAM_D_1.8V
		//Set_MAX8998_PM_REG(ELDO13, 0); //CAM_A_2.8V
		//Set_MAX8998_PM_REG(ELDO14, 0); //CAM_IO_1.8V
		//]
		Set_MAX8998_PM_REG(ELDO15, 0); //CAM_D_2.8V
		mdelay(300);
		Set_MAX8998_PM_REG(EN4, 0); // CAM_D_1.2V
	}
}

#ifdef CONFIG_MACH_APOLLO
static void apollo_button_brightness_set(struct led_classdev *led_cdev, enum led_brightness value)
{
	//mutex_lock(&apollo_button_backlight_lock);
	if(value > 0)
		{
		// ON
		if(TRUE != Set_MAX8998_PM_REG(ELDO17, 1) )			// VKEYLED 3.3V
			{
			printk("%s, error: LDO17 turn on\n", __func__);
			}	
		}
	else
		{
		// OFF
		if(TRUE != Set_MAX8998_PM_REG(ELDO17, 0) )			// VKEYLED 3.3V
			{
			printk("%s, error: LDO17 turn off\n", __func__);
			}	
		}
	//mutex_unlock(&apollo_button_backlight_lock);
}

static struct led_classdev apollo_button_backlight_led  = {
	.name		= "button-backlight",
	.brightness_set = apollo_button_brightness_set,
};
#endif


void display_max8998_reg(unsigned char addr)
{
	byte data;

	if(!pmic_init)
	{
		printk("[PMIC] %s - MAX8998 initialization is not finished yet!!\n", __func__);
		return ;
	}

	// Read requested register
	if(pmic_read(I2C_SLAVE_ADDR_PM_MAX8998, (u8)addr, &data, (byte)1)) ;

	printk("[PMIC] %s - addr(0x%02x), data(0x%02x)\n", __func__, (u8)addr, data);
}
EXPORT_SYMBOL(display_max8998_reg);


void max8998_safeout_enable(unsigned char mask)
{
	byte data;

	if(!pmic_init)
	{
		printk("[PMIC] %s - MAX8998 initialization is not finished yet!!\n", __func__);
		return ;
	}
	
	if(pmic_read(I2C_SLAVE_ADDR_PM_MAX8998, CHGR2, &data, (byte)1)) ;
	data &= ~(0x3 << 6);
	data |= (u8)mask;
	if(pmic_write(I2C_SLAVE_ADDR_PM_MAX8998, CHGR2, &data, (byte)1)) ;
}
EXPORT_SYMBOL(max8998_safeout_enable);


int get_vdcin_status(void)
{
	byte data;

	if(!pmic_init)
	{
		printk("[PMIC] %s - MAX8998 initialization is not finished yet!!\n", __func__);
		return ;
	}
	
	if(pmic_read(I2C_SLAVE_ADDR_PM_MAX8998, STAT2_REG, &data, (byte)1)) ;
	printk("[PMIC] %s: STAT2(0x%02x)\n", __func__, data);

	if(data & VDCINOK_STATUS_BIT)
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL(get_vdcin_status);


void display_mask(int sel)
{
	byte mask[4], stat_m[2];

	// Read irq mask register
	if(pmic_read(I2C_SLAVE_ADDR_PM_MAX8998, IRQ1_REG_M, mask, (byte)4)) ;

	if(pmic_read(I2C_SLAVE_ADDR_PM_MAX8998, STAT1_REG_M, stat_m, (byte)2)) ;

	if(sel == 0)
		printk("[PMIC] IRQ1M(0x%02x), IRQ2M(0x%02x), IRQ3M(0x%02x), IRQ4M(0x%02x), STAT1M(0x%02x), STAT2M(0x%02x)\n",
			mask[0], mask[1], mask[2], mask[3], stat_m[0], stat_m[1]);
	else
		printk("[PMIC] IRQ1M(0x%02x), IRQ2M(0x%02x), IRQ3M(0x%02x), IRQ4M(0x%02x)\n",
			mask[0], mask[1], mask[2], mask[3]);
}


static irqreturn_t MAX8998_INT_HANDLER(int irq, void *dev_id)
{
	printk("[PMIC] %s - IRQ detected!!\n", __func__);

	queue_work(max8998_workqueue, &max8998_work);

	return IRQ_HANDLED;
}


static void handle_max8998_interrupt(struct work_struct *ignore)
{
	byte buff[4], stat[2], chgr[2];
//	byte mask[4];
	int i;

	// Read irq register
	if(pmic_read(I2C_SLAVE_ADDR_PM_MAX8998, IRQ1_REG, buff, (byte)4))
		goto _error_;

	// Read irq mask register
//	if(pmic_read(I2C_SLAVE_ADDR_PM_MAX8998, IRQ1_REG_M, mask, (byte)4))
//		goto _error_;

	// Remove masked interrupt
//	for(i=0; i<4; i++)
//		buff[i] &= ~(mask[i]);

	// Read status register
	if(pmic_read(I2C_SLAVE_ADDR_PM_MAX8998, STAT1_REG, stat, (byte)2))
		goto _error_;

	printk("[PMIC] IRQ1(0x%02x), IRQ2(0x%02x), IRQ3(0x%02x), IRQ4(0x%02x), STAT1(0x%02x), STAT2(0x%02x)\n",
		buff[0], buff[1], buff[2], buff[3], stat[0], stat[1] );


	// Update current cable status (whether DCIN is valid or not)
	cable_status = ( ((buff[0] & DCINR_MASK_BIT) || (stat[1] & VDCINOK_STATUS_BIT)) ? 1 : 0 );


	/* ********************************************************************
	 *  Now, let's handle the irq that triggers IRQ\ signal to AP.
	 *  If you need to handle more interrup,
	 *    1. Modify interrupt mask bit that defined in Max8998.h (IRQ1_M, ...)
	 *    2. Append handling codes for the irq that you want to handle.
	 *********************************************************************/
	 
	// Handle irq0 interrupt
#if 0
	if(buff[0] & PWRONR_MASK_BIT)  // PWRON key rising edge is detected.
		{}
	else if(buff[0] & PWRONF_MASK_BIT)  // PWRON key falling edge is detected.
		{}
	if(buff[0] & JIGR_MASK_BIT)  // JIG rising edge is detected.
		{}
	else if(buff[0] & JIGF_MASK_BIT)  // JIG falling edge is detected.
		{}
#endif
	if(buff[0] & DCINR_MASK_BIT)  // DCIN rising edge is detected.
		{
		// Set fast charge current to 475mA
		Set_MAX8998_PM_REG(ICH, 0x2);

		// Set top-off threshold under 15%
		Set_MAX8998_PM_REG(TP, 0x1);

		// Disable fast timer
		Set_MAX8998_PM_REG(FT, 0x3);

		printk("[PMIC] DCIN Rising edge detected!!\n");

		// Notify event to battery driver
		cable_status_changed();

		Set_MAX8998_PM_REG(TOPOFFRM, 0x0);
		}
	else if(buff[0] & DCINF_MASK_BIT)  // DCIN falling edge is detected.
		{
		printk("[PMIC] DCIN Falling edge detected!!\n");

		Set_MAX8998_PM_REG(TOPOFFRM, 0x1);

		// Notify event to battery driver
		cable_status_changed();
		}

	// Handle irq1 interrupt
#if 0
	if(buff[1] & ALARM0_MASK_BIT)  // ALARM0 rising edge is detected.
		{}
	if(buff[1] & ALARM1_MASK_BIT)  // ALARM1 rising edge is detected.
		{}
	if(buff[1] & SMPLEVNT_MASK_BIT)  // SMPL rising edge is detected.
		{}
	if(buff[1] & WTSREVNT_MASK_BIT)  // WTSR rising edge is detected.
		{}
#endif

	// Handle irq2 interrupt
//	if(buff[2] & CHGFAULT_MASK_BIT)  // Fast Charge timer is expired or the pregual timer is expired.
//		{}
	if((buff[2] & DONER_MASK_BIT) && cable_status)  // Charger Done is detected.
		{}
	if((buff[2] & CHGRSTF_MASK_BIT) && cable_status)  // Charger restart threshold falling edge is detected.
		{}
//	if(buff[2] & DCINOVP_MASK_BIT)  // DCIN voltage exceeds OVP threshold.
//		{}
	if((buff[2] & TOPOFF_MASK_BIT) && cable_status)  // TOP-OFF threshold is detected.
		{
		// Notify fully charged event to battery driver.
		printk("[PMIC] TOP-OFF threshold is detected!!\n");
		charging_status_changed();
		}
//	if(buff[2] & ONKEY1S_MASK_BIT)  // PWRON high for longer than 1sec.
//		{}

	// Handle irq3 interrupt
	if(buff[3] & LOBAT2_MASK_BIT)  // Low Battery 2nd warning is detected.
		{
		printk("[PMIC] Low Battery 2nd warning is detected!!\n");
		set_low_batt_flag();
		}
	if(buff[3] & LOBAT1_MASK_BIT)  // Low Battery 1st warning is detected.
		{
		printk("[PMIC] Low Battery 1st warning is detected!!\n");
		}

//	if(pmic_read(I2C_SLAVE_ADDR_PM_MAX8998, CHGR1, chgr, (byte)2))
//		goto _error_;
//	printk("[PMIC] After Interrupt - CHGR1(0x%02x), CHGR2(0x%02x)\n", chgr[0], chgr[1]);

	display_mask(1);

	return ;

_error_:
	pr_err("[PMIC] %s - I2C read/write failed\n", __func__);
	return ;
}


// Clear irq status before request max8998 interrupt.
static void prepare_handle_interrupt(void)
{
	byte buff[4], mask[4], stat_m[2];

	// Mask interrupt
	mask[0] = (byte)IRQ1_M;
	mask[1] = (byte)IRQ2_M;
	mask[2] = (byte)IRQ3_M;
	mask[3] = (byte)IRQ4_M;
	if(pmic_write(I2C_SLAVE_ADDR_PM_MAX8998, IRQ1_REG_M, mask, (byte)4))
		goto _error_;

	stat_m[0] = stat_m[1] = 0xff;
	if(pmic_write(I2C_SLAVE_ADDR_PM_MAX8998, STAT1_REG_M, stat_m, (byte)2))
		goto _error_;

	// Clear irq status (read IRQ registers)
	if(pmic_read(I2C_SLAVE_ADDR_PM_MAX8998, IRQ1_REG, buff, (byte)4))
		goto _error_;

#if 0
	// Verify mask registers
	display_mask(0);
#endif

	return ;

_error_:
	pr_err("[PMIC] %s - I2C read/write failed\n", __func__);
	return ;
}


/*===========================================================================

FUNCTION MAX8998_PM_init

DESCRIPTION
    When power up, MAX8998_PM_init will initialize the MAX8998 for each part

INPUT PARAMETERS

RETURN VALUE

DEPENDENCIES
SIDE EFFECTS
EXAMPLE 

===========================================================================*/
void MAX8998_PM_init(void)
{
	pmic_gpio_init();

	// Gpio setting for AP_PMIC_IRQ (EINT7)
	s3c_gpio_cfgpin(GPIO_AP_PMIC_IRQ, S3C_GPIO_SFN(GPIO_AP_PMIC_IRQ_STATE));
	s3c_gpio_setpull(GPIO_AP_PMIC_IRQ, S3C_GPIO_PULL_NONE);

	// Turn LDO7 on (VADC_3.3V) - for apollo new emul (QSC)
	Set_MAX8998_PM_REG(ELDO7, 1);
	Set_MAX8998_PM_REG(LDO7, 0x11);
	
//	Set_MAX8998_PM_REG(DVSARM1, 0xE);  // DVSARM1 -> 1.1V
//	Set_MAX8998_PM_REG(DVSINT1, 0xE);  // DVSINT1 -> 1.1V
	Set_MAX8998_PM_REG(LDO2, 0x6);  // VALIVE -> 1.1V
	Set_MAX8998_PM_REG(LDO3, 0x6);  // VUSB -> 1.1V
	Set_MAX8998_PM_REG(ELDO6, 0);  // LDO6 -> Off
	Set_MAX8998_PM_REG(LDO10, 0x5);  // VPLL -> 1.2V

	// Turn Battery Monitor, Charger Current  Monitor ON (ADC)
	Set_MAX8998_PM_REG(ENBATTMON, 1);
	Set_MAX8998_PM_REG(ENVICHG, 1);
	Set_MAX8998_PM_REG(ELBCNFG1, 1);
	Set_MAX8998_PM_REG(ELBCNFG2, 1);

	// Set Low Battery Threshold Voltage
	Set_MAX8998_PM_REG(LBHYST1, 0x0);  // 100mV
	Set_MAX8998_PM_REG(LBTH1, 0x7);  // 3.57V
	Set_MAX8998_PM_REG(LBHYST2, 0x0);  // 100mV
	Set_MAX8998_PM_REG(LBTH2, 0x5);  // 3.4V

	// I2C enable bits of BUCK1 & 2 are off. Becuase they must be controlled only by PWREN
	// BUCK1 is VARM, BUCK2 is VDDINT & BUCK3 is VDDMEM
	Set_MAX8998_PM_ONOFF_CNTL(ONOFF1_REG, ONOFF1_EN1_M | ONOFF1_EN2_M, 0);

	// I2C enable bit of LDO10 is off. Becuase it must be controlled only by PWREN (VPLL)
	Set_MAX8998_PM_ONOFF_CNTL(ONOFF2_REG, ONOFF2_ELDO10_M, 0);

	// Target voltage for DVSARM1, 2, 3, 4 & DVSINT1, 2 are set by DVFS
	camera_ldo_control(1);

	// Set voltage level of LDO11, LDO12, LDO13, LDO14 for LCD/TSP - apollo new emul
	Set_MAX8998_PM_BUCK3_LDO4TO7_11_17_Voltage(LDO11, VCC_3p000);		// VLCD 3.0V
	Set_MAX8998_PM_LDO12_13_Voltage(LDO12, VCC_1p800);					// VLCD 1.8V
	Set_MAX8998_PM_LDO12_13_Voltage(LDO13, VCC_2p800);					// VTOUCH 2.8V
	Set_MAX8998_PM_LDO14_15_Voltage(LDO14, VCC_3p000);					// VTOUCH 3.0V
	Set_MAX8998_PM_LDO14_15_Voltage(LDO15, VCC_3p300);					// VPWM 3.3V
	Set_MAX8998_PM_BUCK3_LDO4TO7_11_17_Voltage(LDO17, VCC_3p000);		// VKEYLED 3.0V
}


/*============================================================================*/
/* MAX8998 I2C Interface                                                      */
/*============================================================================*/

#include <linux/i2c.h>

#define I2C_GPIO3_DEVICE_ID	4 /* mach-($DEVICE_NAME).c */

//static struct i2c_driver MAX8998_driver;
//static struct i2c_client *MAX8998_i2c_client = NULL;

static int MAX8998_read(struct i2c_client *client, u8 reg, u8 *data, u8 length)
{
	int ret, i;
	u8 buf[length], offset;
	struct i2c_msg msg[2];

	//printk("%s: MAX8998_read +++++\n", __func__);

	offset = reg;

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = &offset;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = length;
	msg[1].buf = buf;

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret != 2) 
		return -EIO;

	for(i=0; i<length; i++)
		*(data++) = buf[i];
	
	return 0;
}


static int MAX8998_write(struct i2c_client *client, u8 reg, u8 *data, u8 length)
{
	int ret, i;
	u8 buf[length+1];
	struct i2c_msg msg[1];

	buf[0] = reg;
	for(i=0; i<length; i++)
		buf[i+1] = *(data++);

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = length+1;
	msg[0].buf = buf;

	ret = i2c_transfer(client->adapter, msg, 1);
	if (ret != 1) 
		return -EIO;

	return 0;
}


/*===========================================================================

FUNCTION pmic_read                                

DESCRIPTION
    It does the following: When we need to read a specific register in Power management section, This is used
INPUT PARAMETERS
    byte slave : slave address
 	byte reg : Register address 
 	byte data : data 
 	byte length : the number of the register 
RETURN VALUE
	PMIC_PASS : Operation was successful
	PMIC_FAIL : Read operation failed
DEPENDENCIES
SIDE EFFECTS
EXAMPLE 
	pmic_read(SLAVE_ADDR_PM, IRQ1_ADDR, &irq1_reg, length);
===========================================================================*/
unsigned int pmic_read(u8 slaveaddr, u8 reg, u8 *data, u8 length)
{
//	struct i2c_client *client;
#if 0
	pr_info("%s -> reg 0x%02x, data 0x%02x\n", __func__, reg, *data);
#endif	
//	if (slaveaddr == I2C_SLAVE_ADDR_PM_MAX8998)
//		client = MAX8998_i2c_client;
//	else 
//		return PMIC_FAIL;

	if (MAX8998_read(g_client, reg, data, length) < 0)
	{ 
		pr_err(KERN_ERR "%s -> Failed! (reg 0x%02x, data 0x%02x)\n", __func__, reg, *data);
		return PMIC_FAIL;
	}
	return PMIC_PASS;
}


/*===========================================================================

FUNCTION pmic_write                                

DESCRIPTION
    It does the following: When we need to write a specific register in Power management section, This is used.
INPUT PARAMETERS
    byte slave : slave address
 	byte reg : Register address 
 	byte data : data 
 	byte length : the number of the register 
RETURN VALUE
	PMIC_PASS : Operation was successful
	PMIC_FAIL : Write operation failed
DEPENDENCIES
SIDE EFFECTS
EXAMPLE 
	reg = ONOFF1_ADDR;
	pmic_onoff_buf[i] = (( pmic_onoff_buf[i] & ~(mask))|(mask & data));
	
	if (pmic_write(SLAVE_ADDR_PM, reg, &pmic_onoff_buf[i], length) != PMIC_PASS) {
		MSG_HIGH("Write Vreg control failed, reg 0x%x", reg, 0, 0);
	}
===========================================================================*/
unsigned int pmic_write(u8 slaveaddr, u8 reg, u8 *data, u8 length)
{
//	struct i2c_client *client;
#if 0
	pr_info("%s -> reg 0x%02x, data 0x%02x\n", __func__, reg, *data);
#endif	
//	if (slaveaddr == I2C_SLAVE_ADDR_PM_MAX8998)
//		client = MAX8998_i2c_client;
//	else 
//		return PMIC_FAIL;

	if (MAX8998_write(g_client, reg, data, length) < 0) { 
		pr_err(KERN_ERR "%s -> Failed! (reg 0x%02x, data 0x%02x)\n", __func__, reg, *data);
		return PMIC_FAIL;
	}	

	return PMIC_PASS;
}


unsigned int pmic_tsc_write(u8 slaveaddr, u8 *cmd)
{
	return 0;
}


unsigned int pmic_tsc_read(u8 slaveaddr, u8 *cmd)
{
	return 0;
}

static int MAX8998_probe (struct i2c_client *client,const struct i2c_device_id *id)
{
	int ret = 0;
	byte stat2;
	struct max8998_state *max8998;

	INIT_WORK(&max8998_work, handle_max8998_interrupt);
	max8998_workqueue = create_singlethread_workqueue("max8998_workqueue");
	if(!max8998_workqueue)
		return -ENOMEM;

	max8998 = kzalloc(sizeof(struct max8998_state), GFP_KERNEL);
	if (max8998 == NULL) {
		pr_err("%s: failed to allocate memory\n", __func__);
		return -ENOMEM;
	}

	max8998->client = client;
	i2c_set_clientdata(client, max8998);
	g_client = client;

	pmic_init = 1;  // client attachment finished now.

	MAX8998_PM_init();

	prepare_handle_interrupt();

	mdelay(10);

	// Check cable status
	if(pmic_read(I2C_SLAVE_ADDR_PM_MAX8998, STAT2_REG, &stat2, (byte)1))
		goto error;
	cable_status = ((stat2 & VDCINOK_STATUS_BIT) ? 1 : 0 );
	
	if(cable_status)
	{
// [ Notice : If charger is not connected, charge current and top-off threshold setting doesn't work.
		// Set fast charge current to 475mA
		Set_MAX8998_PM_REG(ICH, 0x2);
	
		// Set top-off threshold under 20% (for test)
		Set_MAX8998_PM_REG(TP, 0x2);
	
		Set_MAX8998_PM_REG(TOPOFFRM, 0x0);
// ]
	}

	set_irq_type(IRQ_PMIC_STATUS_INT, IRQ_TYPE_EDGE_FALLING);
	ret = request_irq(IRQ_PMIC_STATUS_INT, MAX8998_INT_HANDLER,
			  IRQF_DISABLED,
			  PMIC_DRIVER_NAME,
			  NULL);
	if(ret)
	{
		printk("[PMIC] %s - failed to request irq (%d)\n", __func__, ret);
		goto error;
	}

#ifdef CONFIG_MACH_APOLLO
	ret = led_classdev_register(&client->dev, &apollo_button_backlight_led);
	if (ret < 0)
		printk("%s fail: led_classdev_register\n", __func__);
#endif

error:
	return ret;
}

static int MAX8998_remove(struct i2c_client *client)
{
	free_irq(IRQ_PMIC_STATUS_INT, NULL);

#ifdef CONFIG_MACH_APOLLO
	led_classdev_unregister(&apollo_button_backlight_led);
#endif

	return 0;
}


static int MAX8998_command(struct i2c_client *client, unsigned int cmd, void *arg)
{
	return 0;
}


#define TEST_PMIC_SUSPEND_CONTROL
#ifdef TEST_PMIC_SUSPEND_CONTROL
static int MAX8998_resume(void)
{
	Set_MAX8998_PM_REG(EN4, 1);

//	Set_MAX8998_PM_REG(ELDO6, 1);
	Set_MAX8998_PM_REG(ELDO15, 1);  
//	Set_MAX8998_PM_REG(ELDO16, 1);

	return 0;
}

static int MAX8998_suspend(void)
{
	Set_MAX8998_PM_REG(EN4, 0);

//	Set_MAX8998_PM_REG(ELDO6, 0);
	Set_MAX8998_PM_REG(ELDO15, 0);
//	Set_MAX8998_PM_REG(ELDO16, 0);

	return 0;
}
#endif


struct i2c_device_id MAX8998_ids[] = {
	 		{ PMIC_DRIVER_NAME, 0},
			{ }
 };

MODULE_DEVICE_TABLE(i2c, MAX8998_ids);

static struct i2c_driver MAX8998_i2c_driver = {
	.driver = {
		.name = PMIC_DRIVER_NAME,
	},
	.probe = MAX8998_probe,
	.remove = __devexit_p (MAX8998_remove),
	.command = MAX8998_command,
	.id_table = MAX8998_ids,
#ifdef TEST_PMIC_SUSPEND_CONTROL
	.resume = MAX8998_resume,
	.suspend = MAX8998_suspend,
#endif
};

static int pmic_init_status = 0;
int is_pmic_initialized(void)
{
	return pmic_init_status;
}


static int __init MAX8998_init(void)
{
	int err;

	printk("MAX8998_init\n");

	if ( (err = i2c_add_driver(&MAX8998_i2c_driver)) ) 
	{
		printk("MAX8998 Driver registration failed, module not inserted.\n");
		return err;
	}
	pmic_init_status = 1;

	return 0;
}


static void __exit MAX8998_exit(void)
{
	i2c_del_driver(&MAX8998_i2c_driver);
	pmic_init_status = 0;
}

//subsys_initcall(max8998_pmic_init);
module_init(MAX8998_init);
module_exit(MAX8998_exit);


MODULE_AUTHOR("Minsung Kim <ms925.kim@samsung.com>");
MODULE_DESCRIPTION("MAX8998 Driver");
MODULE_LICENSE("GPL");


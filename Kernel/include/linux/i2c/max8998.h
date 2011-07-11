
/**************************************************************************

Module Name:  max8998.h

Abstract:


**************************************************************************/


#ifndef __LINUX_MAX8998_H
#define __LINUX_MAX8998_H

#define TRUE   1   /* Boolean true value. */
#define FALSE  0   /* Boolean false value. */
#ifndef NULL
#define NULL   0
#endif

typedef  unsigned char      boolean;     /* Boolean value type. */

typedef  unsigned long int  uint32;      /* Unsigned 32 bit value */
typedef  unsigned short     uint16;      /* Unsigned 16 bit value */
typedef  unsigned char      uint8;       /* Unsigned 8  bit value */

typedef  signed long int    int32;       /* Signed 32 bit value */
typedef  signed short       int16;       /* Signed 16 bit value */
typedef  signed char        int8;        /* Signed 8  bit value */

/* This group are the deprecated types.  Their use should be
** discontinued and new code should use the types above
*/
typedef  unsigned char     byte;         /* Unsigned 8  bit value type. */
typedef  unsigned short    word;         /* Unsinged 16 bit value type. */
typedef  unsigned long     dword;        /* Unsigned 32 bit value type. */

typedef  unsigned char     uint1;        /* Unsigned 8  bit value type. */
typedef  unsigned short    uint2;        /* Unsigned 16 bit value type. */
typedef  unsigned long     uint4;        /* Unsigned 32 bit value type. */

typedef  signed char       int1;         /* Signed 8  bit value type. */
typedef  signed short      int2;         /* Signed 16 bit value type. */
typedef  long int          int4;         /* Signed 32 bit value type. */

typedef  signed long       sint31;       /* Signed 32 bit value */
typedef  signed short      sint15;       /* Signed 16 bit value */
typedef  signed char       sint7;        /* Signed 8  bit value */

/*
// rama 
PM part Slave address = 0xAC (SRAD = 1)
PM part Slave address = 0xBC (SRAD = floating)
PM part Slave address = 0xCC (SRAD = 0)
*/


/* MAX8998 PMIC slave address (8-bit aligned) */
#define I2C_SLAVE_ADDR_PM_MAX8998		0xCC		// select one slave address of AC, BC or CC
#define I2C_SLAVE_ADDR_RTC_MAX8998		0x0C

#define PMIC_DRIVER_NAME				"MAX8998"

#define IRQ_PMIC_STATUS_INT				IRQ_EINT7

/* MAX8998 Register info */
typedef const struct {
    const byte  addr;
    const byte  mask;
    const byte  clear;
    const byte  shift;
} max8998_reg_type;

/* IRQ routine */
typedef struct {
    void (*irq_ptr)(void);
} max8998_irq_table_type;

typedef enum {
    //ONOFF1 - 0x00
    START_IRQ = 0,
    PWRONR = START_IRQ,
    PWRONF,
    JIGR,
    JIGF,
    DCINR,
    DCINF,
                      
    ALARM0,
    ALARM1,
    SMPLEVNT,
    WTSREVNT,
                      
    CHGFAULT,
    DONER,
    CHGRSTF,
    DCINOVPR,
    TOPOFFR,
    ONKEY1S,
                      
    LOBAT2,
    LOBAT1,
    ENDOFIRQ = LOBAT1,

    START_IRQMASK,
    PWRONRM = START_IRQMASK,
    PWRONFM,
    JIGRM,
    JIGFM,
    DCINRM,
    DCINFM,
                      
    ALARM0M,
    ALARM1M,
    SMPLEVNTM,
    WTSREVNTM,
                      
    CHGFAULTM,
    DONERM,
    CHGRSTFM,
    DCINOVPRM,
    TOPOFFRM,
    ONKEY1SM,
                      
    LOBAT2M,
    LOBAT1M,
    END_IRQMASK = LOBAT1M,
           
    PWRON_status,
    JIG_status,
    ALARM0_status,
    ALARM1_status,
    SMPLEVNT_status,
    WTSREVNT_status,
    LOBAT2_status,
    LOBAT1_status,
                      
    DONE_status,
    VDCINOK_status,
    DETBAT_status,
    CHGON_status,
    FCHG_status,
    PQL_status,
                      
    PWRONM_status,
    JIGM_status,
    ALARM0M_status,
    ALARM1M_status,
    SMPLEVNTM_status,
    WTSREVNTM_status,
    LOBAT2M_status,
    LOBAT1M_status,
                      
    DONEM_status,
    VDCINOKM_status,
    DETBATM_status,
    CHGONM_status,
    FCHGM_status,
    PQLM_status,
                      
    TP,
    RSTR,
    ICH,
            
    ESAFEOUT,
    FT,
    BATTSL,
    TMP,
    CHGENB,
                      
    LDO2DIS,
    LDO3DIS,
    LDO4DIS,
    LDO5DIS,
    LDO6DIS,
    LDO7DIS,
    LDO8DIS,
    LDO9DIS,
    LDO10DIS,
    LDO11DIS,
    LDO12DIS,
    LDO13DIS,
    LDO14DIS,
    LDO15DIS,
    LDO16DIS,
    LDO17DIS,
    SAFEOUT1DIS,
    SAFEOUT2DIS,
    SD1DIS,
    SD2DIS,
    SD3DIS,
    SD4DIS,
                      
    EN1,
    EN2,
    EN3,
    EN4,
    ELDO2,
    ELDO3,
    ELDO4,
    ELDO5,
    ELDO6,
    ELDO7,
    ELDO8,
    ELDO9,
    ELDO10,
    ELDO11,
    ELDO12,
    ELDO13,
    ELDO14,
    ELDO15,
    ELDO16,
    ELDO17,
    EPWRHOLD,
    ENBATTMON,
    ELBCNFG2,
    ELBCNFG1,
                      
    EN32KHZAP,
    EN32KHZCP,
    ENVICHG,
    RAMP,
             
    DVSARM1,
    DVSARM2,
    DVSARM3,
    DVSARM4,
    DVSINT1,
    DVSINT2,
    BUCK3,
    BUCK4,
    LDO2,
    LDO3,
    LDO4,
    LDO5,
    LDO6,
    LDO7,
    LDO8,
    LDO9,
    LDO10,
    LDO11,
    LDO12,
    LDO13,
    LDO14,
    LDO15,
    LDO16,
    LDO17,
    BKCHR,
         
    LBHYST1,
    LBTH1,
           
    LBHYST2,
    LBTH2,
           
    DASH,
    MASKREV,

    ENDOFPM = MASKREV
} max8998_pm_section_type;

// RAMP
typedef enum {
    RAMP_1mVpus = 0,
    RAMP_2mVpus,
    RAMP_3mVpus,
    RAMP_4mVpus,
    RAMP_5mVpus,
    RAMP_6mVpus,
    RAMP_7mVpus,
    RAMP_8mVpus,
    RAMP_9mVpus,
    RAMP_10mVpus,
    RAMP_11mVpus,
    RAMP_12mVpus,
    RAMP_MAX = RAMP_12mVpus
} ramp_rate_type;

#define VCC_MIN_VOLTAGE	750
#define VCC_MAX_VOLTAGE	1525

// OUT voltage setting
typedef enum {
    VCC_0p750 = 0, // 0 0.75V
    VCC_0p775,
    VCC_0p800,
    VCC_0p825,
    VCC_0p850,
    VCC_0p875,
    VCC_0p900,
    VCC_0p925,
    VCC_0p950,
    VCC_0p975,
    VCC_1p000,
    VCC_1p025,
    VCC_1p050,
    VCC_1p075,
    VCC_1p100,
    VCC_1p125,
    VCC_1p150,
    VCC_1p175,
    VCC_1p200,
    VCC_1p225,
    VCC_1p250,
    VCC_1p275,
    VCC_1p300,
    VCC_1p325,
    VCC_1p350,
    VCC_1p375,
    VCC_1p400,
    VCC_1p425,
    VCC_1p450,
    VCC_1p475,
    VCC_1p500,

    VCC_1p600,
    VCC_1p700,
    VCC_1p800,
    VCC_1p900,
    VCC_2p000,
    VCC_2p100,
    VCC_2p200,
    VCC_2p300,
    VCC_2p400,
    VCC_2p500,
    VCC_2p600,
    VCC_2p700,
    VCC_2p800,
    VCC_2p900,
    VCC_3p000,
    VCC_3p100,
    VCC_3p200,
    VCC_3p300,
    VCC_3p400,
    VCC_3p500,
    VCC_3p600,
    VCC_MAX
} out_voltage_type;

/*
// BUCK1,2 voltage
typedef enum {
    BUCK1_2_0p750 = 0, // 0 0.75V
    BUCK1_2_0p775,
    BUCK1_2_0p800,
    BUCK1_2_0p825,
    BUCK1_2_0p850,
    BUCK1_2_0p875,
    BUCK1_2_0p900,
    BUCK1_2_0p925,
    BUCK1_2_0p950,
    BUCK1_2_0p975,
    BUCK1_2_1p000,
    BUCK1_2_1p025,
    BUCK1_2_1p050,
    BUCK1_2_1p075,
    BUCK1_2_1p100,
    BUCK1_2_1p125,
    BUCK1_2_1p150,
    BUCK1_2_1p175,
    BUCK1_2_1p200,
    BUCK1_2_1p225,
    BUCK1_2_1p250,
    BUCK1_2_1p275,
    BUCK1_2_1p300,
    BUCK1_2_1p325
    BUCK1_2_1p350,
    BUCK1_2_1p375,
    BUCK1_2_1p400,
    BUCK1_2_1p425,
    BUCK1_2_1p450,
    BUCK1_2_1p475,
    BUCK1_2_1p500
} buck1_2_voltage_type;
*/

// BUCK4 voltage
typedef enum {
    BUCK4_0p800,
    BUCK4_0p900,
    BUCK4_1p000,
    BUCK4_1p100,
    BUCK4_1p200,
    BUCK4_1p300,
    BUCK4_1p400,
    BUCK4_1p500,
    BUCK4_1p600,
    BUCK4_1p700,
    BUCK4_1p800,
    BUCK4_1p900,
    BUCK4_2p000,
    BUCK4_2p100,
    BUCK4_2p200,
    BUCK4_2p300
} buck4_voltage_type;


// LDO2,3 voltage
typedef enum {
    LDO2_3_0p800,
    LDO2_3_0p850,
    LDO2_3_0p900,
    LDO2_3_0p950,
    LDO2_3_1p000,
    LDO2_3_1p050,
    LDO2_3_1p100,
    LDO2_3_1p150,
    LDO2_3_1p200,
    LDO2_3_1p250,
    LDO2_3_1p300
} ldo2_3_voltage_type;



// BUCK3, LDO4~7, 11, 17 voltage
typedef enum {
    BUCK3_LDO4TO7_11_17_1p600,
    BUCK3_LDO4TO7_11_17_1p700,
    BUCK3_LDO4TO7_11_17_1p800,
    BUCK3_LDO4TO7_11_17_1p900,
    BUCK3_LDO4TO7_11_17_2p000,
    BUCK3_LDO4TO7_11_17_2p100,
    BUCK3_LDO4TO7_11_17_2p200,
    BUCK3_LDO4TO7_11_17_2p300,
    BUCK3_LDO4TO7_11_17_2p400,
    BUCK3_LDO4TO7_11_17_2p500,
    BUCK3_LDO4TO7_11_17_2p600,
    BUCK3_LDO4TO7_11_17_2p700,
    BUCK3_LDO4TO7_11_17_2p800,
    BUCK3_LDO4TO7_11_17_2p900,
    BUCK3_LDO4TO7_11_17_3p000,
    BUCK3_LDO4TO7_11_17_3p100,
    BUCK3_LDO4TO7_11_17_3p200,
    BUCK3_LDO4TO7_11_17_3p300,
    BUCK3_LDO4TO7_11_17_3p400,
    BUCK3_LDO4TO7_11_17_3p500,
    BUCK3_LDO4TO7_11_17_3p600
} buck3_ldo4to7_11_17_voltage_type;


// LDO8 voltage
typedef enum {
    LDO8_3p000,
    LDO8_3p100,
    LDO8_3p200,
    LDO8_3p300,
    LDO8_3p400,
    LDO8_3p500,
    LDO8_3p600
} ldo8_voltage_type;



// LDO9 voltage
typedef enum {
    LDO9_2p800,
    LDO9_2p900,
    LDO9_3p000,
    LDO9_3p100
} ldo9_voltage_type;


// LDO10 voltage
typedef enum {
    LDO10_0p950,
    LDO10_1p000,
    LDO10_1p050,
    LDO10_1p100,
    LDO10_1p150,
    LDO10_1p200,
    LDO10_1p250,
    LDO10_1p300
} ldo10_voltage_type;


// LDO12,13 voltage
typedef enum {
    LDO12_13_0p800,
    LDO12_13_0p900,
    LDO12_13_1p000,
    LDO12_13_1p100,
    LDO12_13_1p200,
    LDO12_13_1p300,
    LDO12_13_1p400,
    LDO12_13_1p500,
    LDO12_13_1p600,
    LDO12_13_1p700,
    LDO12_13_1p800,
    LDO12_13_1p900,
    LDO12_13_2p000,
    LDO12_13_2p100,
    LDO12_13_2p200,
    LDO12_13_2p300,
    LDO12_13_2p400,
    LDO12_13_2p500,
    LDO12_13_2p600,
    LDO12_13_2p700,
    LDO12_13_2p800,
    LDO12_13_2p900,
    LDO12_13_3p000,
    LDO12_13_3p100,
    LDO12_13_3p200,
    LDO12_13_3p300
} ldo12_13_voltage_type;


// LDO14,15 voltage
typedef enum {
    LDO14_15_1p200,
    LDO14_15_1p300,
    LDO14_15_1p400,
    LDO14_15_1p500,
    LDO14_15_1p600,
    LDO14_15_1p700,
    LDO14_15_1p800,
    LDO14_15_1p900,
    LDO14_15_2p000,
    LDO14_15_2p100,
    LDO14_15_2p200,
    LDO14_15_2p300,
    LDO14_15_2p400,
    LDO14_15_2p500,
    LDO14_15_2p600,
    LDO14_15_2p700,
    LDO14_15_2p800,
    LDO14_15_2p900,
    LDO14_15_3p000,
    LDO14_15_3p100,
    LDO14_15_3p200,
    LDO14_15_3p300
} ldo14_15_voltage_type;


// LBCNFG
typedef enum {
    LBHYST_100mV,
    LBHYST_200mV,
    LBHYST_300mV,
    LBHYST_400mV,
    LBTH_2p9V,
    LBTH_3p0V,
    LBTH_3p1V,
    LBTH_3p2V,
    LBTH_3p3V,
    LBTH_3p4V,
    LBTH_3p5V,
    LBTH_3p57V
} lbcnfg_type;

// IRQ Mask register setting
//---------------------------------------------------------------------------
//  IRQ Registers ( Address 0x00 )
//---------------------------------------------------------------------------
#define IRQ1_REG	0x00
#define IRQ2_REG	0x01
#define IRQ3_REG	0x02
#define IRQ4_REG	0x03

//---------------------------------------------------------------------------
//  IRQ Mask Register ( Address 0x05 ~ 0x08 )
//---------------------------------------------------------------------------
#define IRQ1_REG_M  0x04
#define PWRONR_MASK_BIT     0x80
#define PWRONF_MASK_BIT     0x40
#define JIGR_MASK_BIT       0x20
#define JIGF_MASK_BIT       0x10
#define DCINR_MASK_BIT      0x08
#define DCINF_MASK_BIT      0x04
#define IRQ1_BIT1_MASK_BIT  0x02
#define IRQ1_BIT0_MASK_BIT  0x01

#define IRQ2_REG_M  0x05
#define IRQ2_BIT7_MASK_BIT  0x80
#define IRQ2_BIT6_MASK_BIT  0x40
#define IRQ2_BIT5_MASK_BIT  0x20
#define IRQ2_BIT4_MASK_BIT  0x10
#define ALARM0_MASK_BIT     0x08
#define ALARM1_MASK_BIT     0x04
#define SMPLEVNT_MASK_BIT   0x02
#define WTSREVNT_MASK_BIT   0x01

#define IRQ3_REG_M  0x06
#define CHGFAULT_MASK_BIT   0x80
#define IRQ3_BIT6_MASK_BIT  0x40
#define DONER_MASK_BIT      0x20
#define CHGRSTF_MASK_BIT    0x10
#define DCINOVP_MASK_BIT    0x08
#define TOPOFF_MASK_BIT     0x04
#define IRQ3_BIT1_MASK_BIT  0x02
#define ONKEY1S_MASK_BIT    0x01

#define IRQ4_REG_M  0x07
#define IRQ4_BIT7_MASK_BIT  0x80
#define IRQ4_BIT6_MASK_BIT  0x40
#define IRQ4_BIT5_MASK_BIT  0x20
#define IRQ4_BIT4_MASK_BIT  0x10
#define IRQ4_BIT3_MASK_BIT  0x08
#define IRQ4_BIT2_MASK_BIT  0x04
#define LOBAT2_MASK_BIT     0x02
#define LOBAT1_MASK_BIT     0x01

// initialize the mask register as belows.
// If MASK bit is set, then the rising edge interrupt detection for the MASK bit is masked.
// So, IRQ\ pin is not asserted.
// If you want to clear some bit, please check the max8998_irq_table[] for interrupt service routine.
#define IRQ1_M ( PWRONR_MASK_BIT | PWRONF_MASK_BIT | JIGR_MASK_BIT | JIGF_MASK_BIT | \
		IRQ1_BIT1_MASK_BIT | IRQ1_BIT0_MASK_BIT )
#define IRQ2_M ( IRQ2_BIT7_MASK_BIT | IRQ2_BIT6_MASK_BIT | IRQ2_BIT5_MASK_BIT | IRQ2_BIT4_MASK_BIT | \
		ALARM0_MASK_BIT | ALARM1_MASK_BIT | SMPLEVNT_MASK_BIT | WTSREVNT_MASK_BIT )
#define IRQ3_M ( CHGFAULT_MASK_BIT | IRQ3_BIT6_MASK_BIT | DONER_MASK_BIT | CHGRSTF_MASK_BIT | \
		DCINOVP_MASK_BIT | TOPOFF_MASK_BIT | IRQ3_BIT1_MASK_BIT | ONKEY1S_MASK_BIT )
#define IRQ4_M ( IRQ4_BIT7_MASK_BIT | IRQ4_BIT6_MASK_BIT | IRQ4_BIT5_MASK_BIT | IRQ4_BIT4_MASK_BIT | \
		IRQ4_BIT3_MASK_BIT | IRQ4_BIT2_MASK_BIT ) // | LOBAT2_MASK_BIT | LOBAT1_MASK_BIT )

// Status register
#define STAT1_REG		0x08
#define STAT2_REG		0x09
#define DONE_STATUS_BIT	0x40
#define VDCINOK_STATUS_BIT	0x20
#define DETBAT_STATUS_BIT	0x10
#define CHGON_STATUS_BIT	0x08
#define FCHG_STATUS_BIT	0x04
#define PQL_STATUS_BT		0x02
#define STAT1_REG_M	0x0A
#define STAT2_REG_M	0x0B

// PM register
#define ONOFF1_REG      0x11
#define ONOFF1_ELDO5_M  0x01
#define ONOFF1_ELDO4_M  0x02
#define ONOFF1_ELDO3_M  0x04
#define ONOFF1_ELDO2_M  0x08
#define ONOFF1_EN4_M    0x10
#define ONOFF1_EN3_M    0x20
#define ONOFF1_EN2_M    0x40
#define ONOFF1_EN1_M    0x80

#define ONOFF2_REG      0x12
#define ONOFF2_ELDO13_M 0x01
#define ONOFF2_ELDO12_M 0x02
#define ONOFF2_ELDO11_M 0x04
#define ONOFF2_ELDO10_M 0x08
#define ONOFF2_ELDO9_M  0x10
#define ONOFF2_ELDO8_M  0x20
#define ONOFF2_ELDO7_M  0x40
#define ONOFF2_ELDO6_M  0x80

#define ONOFF3_REG          0x13
#define ONOFF3_ELBCNFG1_M   0x01
#define ONOFF3_ELBCNFG2_M   0x02
#define ONOFF3_ENBATTMON_M  0x04
#define ONOFF3_EPWRHOLD_M   0x08
#define ONOFF3_ELDO17_M     0x10
#define ONOFF3_ELDO16_M     0x20
#define ONOFF3_ELDO15_M     0x40
#define ONOFF3_ELDO14_M     0x80

#define ONOFF4_REG          0x14
#define ONOFF3_ENVICHG_M    0x20
#define ONOFF3_EN32kHzCP_M  0x40
#define ONOFF3_EN32kHzAP_M  0x80

#define ADISCHG_EN1_REG 0x0E
#define LDO9DIS_M       0x01
#define LDO8DIS_M       0x02
#define LDO7DIS_M       0x04
#define LDO6DIS_M       0x08
#define LDO5DIS_M       0x10
#define LDO4DIS_M       0x20
#define LDO3DIS_M       0x40
#define LDO2DIS_M       0x80

#define ADISCHG_EN2_REG 0x0F
#define LDO17DIS_M      0x01
#define LDO16DIS_M      0x02
#define LDO15DIS_M      0x04
#define LDO14DIS_M      0x08
#define LDO13DIS_M      0x10
#define LDO12DIS_M      0x20
#define LDO11DIS_M      0x40
#define LDO10DIS_M      0x80

#define ADISCHG_EN3_REG 0x10
#define SD4DIS_M         0x01
#define SD3DIS_M         0x02
#define SD2DIS_M         0x04
#define SD1DIS_M         0x08
#define SAFEOUT2DIS_M    0x10
#define SAFEOUT1DIS_M    0x20

#define BUCK1_DVS12_REG 0x04
#define BUCK1_DVS34_REG 0x05
#define BUCK2_DVS12_REG 0x06
#define BUCK3_REG       0x07
#define LDO23_REG       0x08
#define LDO4_REG        0x09
#define LDO5_REG        0x0A
#define LDO6_REG        0x0B
#define LDO7_REG        0x0C
#define LDO8_BKCHR_REG  0x0D
#define LDO9_REG        0x0E
#define LBCNFG2_REG      0x2C // modified by youngeun.park

#define CHGR1           	0x0C
#define CHGR2			0x0D
#define ESAFEOUT1		0x80
#define ESAFEOUT2		0x40

#define ID_REG          0x3F

typedef enum {
    PMIC_PASS = 0,
    PMIC_FAIL
} pmic_status_type;

#if 0
/* MAX8998 each register info */
typedef const struct {
	const byte  slave_addr;
	const byte  addr;
} max8998_register_type;

/* MAX8998 each function info */
typedef const struct {
	const byte  slave_addr;
	const byte  addr;
	const byte  mask;
	const byte  clear;
	const byte  shift;
} max8998_function_type;
#endif

/*===========================================================================

      P O W E R     M A N A G E M E N T     S E C T I O N

===========================================================================*/

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
	pmic_read(SLAVE_ADDR_PM, IRQ1_ADDR, &irq1_reg);
===========================================================================*/
extern pmic_status_type pmic_read(byte slave, byte reg, byte *data, byte length);


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
	
	if (pmic_write(SLAVE_ADDR_PM, reg, &pmic_onoff_buf[i]) != PMIC_PASS) {
		MSG_HIGH("Write Vreg control failed, reg 0x%x", reg, 0, 0);
	}
===========================================================================*/
extern pmic_status_type pmic_write(byte slave, byte reg, byte *data, byte length);


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
extern boolean Set_MAX8998_PM_REG(max8998_pm_section_type reg_num, byte value);


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
extern boolean Get_MAX8998_PM_REG(max8998_pm_section_type reg_num, byte *reg_buff);


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
extern boolean Set_MAX8998_PM_ADDR(byte reg_addr, byte *reg_buff, byte length);


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
extern boolean Get_MAX8998_PM_ADDR(byte reg_addr, byte *reg_buff, byte length);


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
extern void Set_MAX8998_PM_ONOFF_CNTL(byte onoff_reg, byte cntl_item, byte status);


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
extern void Set_MAX8998_PM_ADISCHG_EN(byte adischg_en_reg, byte cntl_item, byte status);


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
extern void Set_MAX8998_PM_RAMP_RATE(ramp_rate_type ramp_rate);


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
extern boolean Set_MAX8998_PM_BUCK1_2_Voltage(max8998_pm_section_type ldo_num, out_voltage_type set_value);


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
extern boolean Set_MAX8998_PM_BUCK4_Voltage(max8998_pm_section_type ldo_num, out_voltage_type set_value);


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
extern boolean Set_MAX8998_PM_LDO2_3_Voltage(max8998_pm_section_type ldo_num, out_voltage_type set_value);


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
extern boolean Set_MAX8998_PM_BUCK3_LDO4TO7_11_17_Voltage(max8998_pm_section_type ldo_num, out_voltage_type set_value);


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
extern boolean Set_MAX8998_PM_LDO8_Voltage(max8998_pm_section_type ldo_num, out_voltage_type set_value);


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
extern boolean Set_MAX8998_PM_LDO9_Voltage(max8998_pm_section_type ldo_num, out_voltage_type set_value);


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
extern boolean Set_MAX8998_PM_LDO10_Voltage(max8998_pm_section_type ldo_num, out_voltage_type set_value);


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
extern boolean Set_MAX8998_PM_LDO12_13_Voltage(max8998_pm_section_type ldo_num, out_voltage_type set_value);


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
extern boolean Set_MAX8998_PM_LDO14_15_Voltage(max8998_pm_section_type ldo_num, out_voltage_type set_value);


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
extern boolean Set_MAX8998_PM_OUTPUT_Voltage(max8998_pm_section_type ldo_num, out_voltage_type set_value);


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
extern void Set_MAX8998_PM_LBCNFG(max8998_pm_section_type cntl_item, lbcnfg_type lbcnfg);



#endif /* __LINUX_MAX8998_H */


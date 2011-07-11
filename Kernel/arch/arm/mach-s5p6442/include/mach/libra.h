/*
 *  arch/arm/mach-s5p6442/include/mach/libra.h
 *
 *  Author:		Samsung Electronics
 *  Created:	08, Sep, 2009
 *  Copyright:	Samsung Electronics Co.Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#ifndef MACH_LIBRA_H

#define MACH_LIBRA_H

#define CONFIG_RESERVED_MEM_CMM_JPEG_MFC_POST_CAMERA

/*
 * GPIO Configuration
 */

#include <mach/gpio.h>
#include <mach/memory.h>

#define GPIO_LEVEL_LOW      		0
#define GPIO_LEVEL_HIGH     		1
#define GPIO_LEVEL_NONE			2

#define GPIO_DRV_SR_1X			0
#define GPIO_DRV_SR_2X			1
#define GPIO_DRV_SR_3X			2
#define GPIO_DRV_SR_4X			3

/*   GPA0 */
#define GPIO_BT_UART_RXD			S5P64XX_GPA0(0)
#define GPIO_BT_UART_RXD_STATE		2

#define GPIO_BT_UART_TXD			S5P64XX_GPA0(1)
#define GPIO_BT_UART_TXD_STATE		2

#define GPIO_BT_UART_CTS			S5P64XX_GPA0(2)
#define GPIO_BT_UART_CTS_STATE		2

#define GPIO_BT_UART_RTS			S5P64XX_GPA0(3)
#define GPIO_BT_UART_RTS_STATE		2

#define GPIO_GPS_UART_RXD			S5P64XX_GPA0(4)
#define GPIO_GPS_UART_RXD_STATE		2

#define GPIO_GPS_UART_TXD			S5P64XX_GPA0(5)
#define GPIO_GPS_UART_TXD_STATE		2

#define GPIO_GPS_UART_CTS			S5P64XX_GPA0(6)
#define GPIO_GPS_UART_CTS_STATE		2

#define GPIO_GPS_UART_RTS			S5P64XX_GPA0(7)
#define GPIO_GPS_UART_RTS_STATE		2

/*   GPA1 */

#define GPIO_AP_COM_RXD				S5P64XX_GPA1(0)			// FLM_SEL H: AP_FLM_RXD, FLM_SEL L: AP_RXD
#define GPIO_AP_COM_RXD_STATE		2

#define GPIO_AP_COM_TXD				S5P64XX_GPA1(1)			// FLM_SEL H: AP_FLM_TXD, FLM_SEL L: AP_TXD
#define GPIO_AP_COM_TXD_STATE		2

/* GPB */

#define GPIO_CAM_VGA_nSTBY			S5P64XX_GPB(0)			// H: Active,	L: StandBy
#define GPIO_CAM_VGA_nSTBY_STATE	1	

#define GPIO_MSENSE_nRST			S5P64XX_GPB(1)			// H: Active,	L: StandBy
#define GPIO_MSENSE_nRST_STATE		1

#define GPIO_CAM_VGA_nRST			S5P64XX_GPB(2)			// H: Nothing,	L: Reset
#define GPIO_CAM_VGA_nRST_STATE		1

#define GPIO_BT_nRST				S5P64XX_GPB(3)
#define GPIO_BT_nRST_STATE			1

/* GPC0 */
#define GPIO_CODEC_I2S_CLK			S5P64XX_GPC0(0)
#define GPIO_CODEC_I2S_CLK_STATE	2

/* GPC0(1) : TP */

#define GPIO_CODEC_I2S_WS			S5P64XX_GPC0(2)
#define GPIO_CODEC_I2S_WS_STATE		2

#define GPIO_CODEC_I2S_DI			S5P64XX_GPC0(3)
#define GPIO_CODEC_I2S_DI_STATE		2

#define GPIO_CODEC_I2S_DO			S5P64XX_GPC0(4)
#define GPIO_CODEC_I2S_DO_STATE		2

/* GPC1 */
#define GPIO_REC_PCM_CLK			S5P64XX_GPC1(0)
#define GPIO_REC_PCM_CLK_STATE		3

/* GPC1(1) : N.C */

#define GPIO_REC_PCM_SYNC			S5P64XX_GPC1(2)
#define GPIO_REC_PCM_SYNC_STATE		3

#define GPIO_REC_PCM_IN				S5P64XX_GPC1(3)
#define GPIO_REC_PCM_IN_STATE		3

#define GPIO_REC_PCM_OUT			S5P64XX_GPC1(4)
#define GPIO_REC_PCM_OUT_STATE		3


/*GPD0 */
#define GPIO_VIBTONE_PWM			S5P64XX_GPD1(0)
#define GPIO_VIBTONE_PWM_STATE		2

/* GPD0(1) : N.C */

/*GPD1 */
#define GPIO_CAM_SDA_2_8V			S5P64XX_GPD1(0)
#define GPIO_CAM_SDA_2_8V_STATE		2

#define GPIO_CAM_SCL_2_8V			S5P64XX_GPD1(1)
#define GPIO_CAM_SCL_2_8V_STATE		2

#define GPIO_AP_I2C_SDA_2_8V				S5P64XX_GPD1(2)
#define GPIO_AP_I2C_SDA_2_8V_STATE		2

#define GPIO_AP_I2C_SCL_2_8V				S5P64XX_GPD1(3)
#define GPIO_AP_I2C_SCL_2_8V_STATE		2

#define GPIO_AP_SDA_2_8V				S5P64XX_GPD1(4)
#define GPIO_AP_SDA_2_8V_STATE			1 //2

#define GPIO_AP_SCL_2_8V					S5P64XX_GPD1(5)
#define GPIO_AP_SCL_2_8V_STATE			1 //2

/*GPE0 */
#define GPIO_CAM_PCLK				S5P64XX_GPE0(0)
#define GPIO_CAM_PCLK_STATE			2

#define GPIO_CAM_VSYNC				S5P64XX_GPE0(1)
#define GPIO_CAM_VSYNC_STATE		2

#define GPIO_CAM_HSYNC				S5P64XX_GPE0(2)
#define GPIO_CAM_HSYNC_STATE		2

#define GPIO_CAM_D_0				S5P64XX_GPE0(3)
#define GPIO_CAM_D_0_STATE			2

#define GPIO_CAM_D_1				S5P64XX_GPE0(4)
#define GPIO_CAM_D_1_STATE			2

#define GPIO_CAM_D_2				S5P64XX_GPE0(5)
#define GPIO_CAM_D_2_STATE			2

#define GPIO_CAM_D_3				S5P64XX_GPE0(6)
#define GPIO_CAM_D_3_STATE			2

#define GPIO_CAM_D_4				S5P64XX_GPE0(7)
#define GPIO_CAM_D_4_STATE			2

/*GPE1 */
#define GPIO_CAM_D_5				S5P64XX_GPE1(0)
#define GPIO_CAM_D_5_STATE			2

#define GPIO_CAM_D_6				S5P64XX_GPE1(1)
#define GPIO_CAM_D_6_STATE			2

#define GPIO_CAM_D_7				S5P64XX_GPE1(2)
#define GPIO_CAM_D_7_STATE			2

#define GPIO_CAM_MCLK				S5P64XX_GPE1(3)
#define GPIO_CAM_MCLK_STATE			2

#define GPIO_TOUCH_RST				S5P64XX_GPE1(4)				// H: Nothing,	L: Reset
#define GPIO_TOUCH_RST_STATE		1


/*GPF0 */
#define GPIO_LCD_HSYNC				S5P64XX_GPF0(0)
#define GPIO_LCD_HSYNC_STATE		2

#define GPIO_LCD_VSYNC				S5P64XX_GPF0(1)
#define GPIO_LCD_VSYNC_STATE		2

#define GPIO_LCD_VDEN				S5P64XX_GPF0(2)				// H: Enable,	L: Disable
#define GPIO_LCD_VDEN_STATE			2

#define GPIO_LCD_MCLK				S5P64XX_GPF0(3)
#define GPIO_LCD_MCLK_STATE			2

/* GPF0(4) : N.C */

/* GPF0(5) : N.C */

#define GPIO_LCD_D_0				S5P64XX_GPF0(6)
#define GPIO_LCD_D_0_STATE			2

#define GPIO_LCD_D_1				S5P64XX_GPF0(7)
#define GPIO_LCD_D_1_STATE			2

/*GPF1 */
#define GPIO_LCD_D_2				S5P64XX_GPF1(0)
#define GPIO_LCD_D_2_STATE			2

#define GPIO_LCD_D_3				S5P64XX_GPF1(1)
#define GPIO_LCD_D_3_STATE			2

#define GPIO_LCD_D_4				S5P64XX_GPF1(2)
#define GPIO_LCD_D_4_STATE			2

#define GPIO_LCD_D_5				S5P64XX_GPF1(3)
#define GPIO_LCD_D_5_STATE			2

/* GPF1(4) : N.C */

/* GPF1(5) : N.C */

#define GPIO_LCD_D_6				S5P64XX_GPF1(6)
#define GPIO_LCD_D_6_STATE			2

#define GPIO_LCD_D_7				S5P64XX_GPF1(7)
#define GPIO_LCD_D_7_STATE			2

/*GPF2 */
#define GPIO_LCD_D_8				S5P64XX_GPF2(0)
#define GPIO_LCD_D_8_STATE			2

#define GPIO_LCD_D_9				S5P64XX_GPF2(1)
#define GPIO_LCD_D_9_STATE			2

#define GPIO_LCD_D_10				S5P64XX_GPF2(2)
#define GPIO_LCD_D_10_STATE			2

#define GPIO_LCD_D_11				S5P64XX_GPF2(3)
#define GPIO_LCD_D_11_STATE			2

/* GPF2(4) : N.C */

/* GPF2(5) : N.C */

#define GPIO_LCD_D_12				S5P64XX_GPF2(6)
#define GPIO_LCD_D_12_STATE			2

#define GPIO_LCD_D_13				S5P64XX_GPF2(7)
#define GPIO_LCD_D_13_STATE			2

/*GPF3 */
#define GPIO_LCD_D_14				S5P64XX_GPF3(0)
#define GPIO_LCD_D_14_STATE			2

#define GPIO_LCD_D_15				S5P64XX_GPF3(1)
#define GPIO_LCD_D_15_STATE			2

#define GPIO_LCD_D_16				S5P64XX_GPF3(2)
#define GPIO_LCD_D_16_STATE			2

#define GPIO_LCD_D_17				S5P64XX_GPF3(3)
#define GPIO_LCD_D_17_STATE			2

/* GPF3(4) : N.C */

/* GPF3(5) : N.C */


/*GPG0 */   //SD 1
#define GPIO_NAND_CLK				S5P64XX_GPG0(0)
#define GPIO_NAND_CLK_STATE			2

#define GPIO_NAND_CMD				S5P64XX_GPG0(1)
#define GPIO_NAND_CMD_STATE			2

#define GPIO_TOUCH_INT				S5P64XX_GPG0(2)
#define GPIO_TOUCH_INT_STATE			0xF

#define GPIO_NAND_D_0				S5P64XX_GPG0(3)
#define GPIO_NAND_D_0_STATE			2

#define GPIO_NAND_D_1				S5P64XX_GPG0(4)
#define GPIO_NAND_D_1_STATE			2

#define GPIO_NAND_D_2				S5P64XX_GPG0(5)
#define GPIO_NAND_D_2_STATE			2

#define GPIO_NAND_D_3				S5P64XX_GPG0(6)
#define GPIO_NAND_D_3_STATE			2

/* GPG1 */ //SD 2
#define GPIO_WLAN_SDIO_CLK			S5P64XX_GPG1(0)
#define GPIO_WLAN_SDIO_CLK_STATE	2

#define GPIO_WLAN_SDIO_CMD			S5P64XX_GPG1(1)
#define GPIO_WLAN_SDIO_CMD_STATE	2

#define GPIO_WLAN_nRST				S5P64XX_GPG1(2)
#define GPIO_WLAN_nRST_STATE		1			// H: ON,		L: OFF

#define GPIO_WLAN_SDIO_D_0			S5P64XX_GPG1(3)
#define GPIO_WLAN_SDIO_D_0_STATE	2

#define GPIO_WLAN_SDIO_D_1			S5P64XX_GPG1(4)
#define GPIO_WLAN_SDIO_D_1_STATE	2

#define GPIO_WLAN_SDIO_D_2			S5P64XX_GPG1(5)
#define GPIO_WLAN_SDIO_D_2_STATE	2

#define GPIO_WLAN_SDIO_D_3			S5P64XX_GPG1(6)
#define GPIO_WLAN_SDIO_D_3_STATE	2


/* GPG2 */ //SD 3
#define GPIO_T_FLASH_CLK			S5P64XX_GPG2(0)
#define GPIO_T_FLASH_CLK_STATE		2

#define GPIO_T_FLASH_CMD			S5P64XX_GPG2(1)
#define GPIO_T_FLASH_CMD_STATE		2

#define GPIO_CAM_3M_nSTBY			S5P64XX_GPG2(2)		
#define GPIO_CAM_3M_nSTBY_STATE		1

#define GPIO_T_FLASH_D_0			S5P64XX_GPG2(3)	
#define GPIO_T_FLASH_D_0_STATE		2

#define GPIO_T_FLASH_D_1			S5P64XX_GPG2(4)	
#define GPIO_T_FLASH_D_1_STATE		2

#define GPIO_T_FLASH_D_2			S5P64XX_GPG2(5)	
#define GPIO_T_FLASH_D_2_STATE		2

#define GPIO_T_FLASH_D_3			S5P64XX_GPG2(6)	
#define GPIO_T_FLASH_D_3_STATE		2

/* GPH0 */
#define GPIO_AP_PS_HOLD				S5P64XX_GPH0(0)			// H: Hold,	L: Release
#define GPIO_AP_PS_HOLD_STATE		3			

#define GPIO_ACC_INT				S5P64XX_GPH0(1)			// H: Nothing,	L: Power KEY Detect
#define GPIO_ACC_INT_STATE			0XF

#define GPIO_BUCK_1_EN_A			S5P64XX_GPH0(2)			// H: Active,	L: Not Active
#define GPIO_BUCK_1_EN_A_STATE		1

#define GPIO_BUCK_1_EN_B			S5P64XX_GPH0(3)			// used for check Phone reset
#define GPIO_BUCK_1_EN_B_STATE		1

#define GPIO_BUCK_2_EN				S5P64XX_GPH0(4)			// used for check Phone reset
#define GPIO_BUCK_2_EN_STATE		1

#define GPIO_FLM_SEL				S5P64XX_GPH0(5)			// H: Nothing,	L: Interrupt
#define GPIO_FLM_SEL_STATE			1

#define GPIO_EARJACK_DET			S5P64XX_GPH0(6)			// H: Nothing,	L: Detect
#define GPIO_EARJACK_DET_STATE		0XF

#define GPIO_AP_PMIC_IRQ			S5P64XX_GPH0(7)			// H: Nothing,	L: Interrupt
#define GPIO_AP_PMIC_IRQ_STATE		0XF

/* GPH1 */
#define GPIO_TA_nCONNECTED			S5P64XX_GPH1(0)			// H: Detect	,	L: Nothing
#define GPIO_TA_nCONNECTED_STATE	1

#define GPIO_TA_nCHG				S5P64XX_GPH1(1)			// H: Recongniton,		L: Nothing
#define GPIO_TA_nCHG_STATE			1

#define GPIO_PDA_ACTIVE				S5P64XX_GPH1(2)
#define GPIO_PDA_ACTIVE_STATE		0

#define GPIO_nINT_ONEDRAM_AP		S5P64XX_GPH1(3)			// H: Enable,	L: Disable
#define GPIO_nINT_ONEDRAM_AP_STATE	0

#define GPIO_RESET_REQ_N			S5P64XX_GPH1(4)			// H: Nothing,	L: Reset
#define GPIO_RESET_REQ_N_STATE		1

#define GPIO_CODEC_LDO_EN			S5P64XX_GPH1(5)			// H: Active,	L: StandBy
#define GPIO_CODEC_LDO_EN_STATE	 	1	

#define GPIO_TA_nSTAT				S5P64XX_GPH1(6)			// H: Nothing,	L: Reset
#define GPIO_TA_nSTAT_STATE			1

#define GPIO_PHONE_ACTIVE			S5P64XX_GPH1(7)			// H: FLM I/F,	L: USIM I/F
#define GPIO_PHONE_ACTIVE_STATE		1



/*GPH2 */
#define GPIO_T_FLASH_DETECT			S5P64XX_GPH2(0)			// H: Detect	,	L: Nothing
#define GPIO_T_FLASH_DETECT_STATE	0XF

#define GPIO_MSENSE_IRQ				S5P64XX_GPH2(1)
#define GPIO_MSENSE_IRQ_STATE		0XF

#define GPIO_EAR_SEND_END			S5P64XX_GPH2(2)
#define GPIO_EAR_SEND_END_STATE		0XF

#define GPIO_nHALL_ON				S5P64XX_GPH2(3)		// H: WakeUp		L: Nothing	
#define GPIO_nHALL_ON_STATE			0XF

#define GPIO_WLAN_HOST_WAKE			S5P64XX_GPH2(4)		// H: WakeUp		L: Nothing	
#define GPIO_WLAN_HOST_WAKE_STATE	0XF

#define GPIO_BT_HOST_WAKE			S5P64XX_GPH2(5)		// H: Nothing,	L: Interrupt
#define GPIO_BT_HOST_WAKE_STATE		0XF

#define GPIO_nPOWER				 	S5P64XX_GPH2(6)		// H: Nothing,	L: Interrupt
#define GPIO_nPOWER_STATE			0XF

#define GPIO_JACK_nINT				S5P64XX_GPH2(7)		// H: Nothing,	L: Interrupt
#define GPIO_JACK_nINT_STATE		0XF

/*GPH3 */
#define GPIO_KEY_ROW0				S5P64XX_GPH3(0)
#define GPIO_KEY_ROW0_STATE			3

#define GPIO_KEY_ROW1				S5P64XX_GPH3(1)
#define GPIO_KEY_ROW1_STATE			3

#define GPIO_KEY_ROW2				S5P64XX_GPH3(2)
#define GPIO_KEY_ROW2_STATE			3

#define GPIO_KEY_ROW3				S5P64XX_GPH3(3)
#define GPIO_KEY_ROW3_STATE			3

#define GPIO_KEY_ROW4				S5P64XX_GPH3(4)
#define GPIO_KEY_ROW4_STATE			3

#define GPIO_KEY_ROW5				S5P64XX_GPH3(5)
#define GPIO_KEY_ROW5_STATE			3

#define GPIO_KEY_ROW6				S5P64XX_GPH3(6)
#define GPIO_KEY_ROW6_STATE			3

#define GPIO_KEY_ROW7				S5P64XX_GPH3(7)
#define GPIO_KEY_ROW7_STATE			3

/*GPJ0 */  

#define GPIO_KEY_COL0				S5P64XX_GPJ0(0)
#define GPIO_KEY_COL0_STATE			1

#define GPIO_KEY_COL1				S5P64XX_GPJ0(1)
#define GPIO_KEY_COL1_STATE			1

#define GPIO_KEY_COL2				S5P64XX_GPJ0(2)
#define GPIO_KEY_COL2_STATE			1

#define GPIO_KEY_COL3				S5P64XX_GPJ0(3)
#define GPIO_KEY_COL3_STATE			1

#define GPIO_KEY_COL4				S5P64XX_GPJ0(4)
#define GPIO_KEY_COL4_STATE			1

#define GPIO_KEY_COL5				S5P64XX_GPJ0(5)
#define GPIO_KEY_COL5_STATE			1

#define GPIO_KEY_COL6				S5P64XX_GPJ0(6)
#define GPIO_KEY_COL6_STATE			1

#define GPIO_KEY_COL7				S5P64XX_GPJ0(7)
#define GPIO_KEY_COL7_STATE			1

/*GPJ1 */  

#define GPIO_PHONE_ON				S5P64XX_GPJ1(0)	
#define GPIO_PHONE_ON_STATE			1

#define GPIO_HAPTIC_EN				S5P64XX_GPJ1(1)	
#define GPIO_HAPTIC_EN_STATE		1

#define GPIO_USB_SEL30				S5P64XX_GPJ1(2)	
#define GPIO_USB_SEL30_STATE		1

#define GPIO_MLCD_ON				S5P64XX_GPJ1(3)	
#define GPIO_MLCD_ON_STATE			1

#define GPIO_ADC_EN					S5P64XX_GPJ1(4)	
#define GPIO_ADC_EN_STATE			1

#define GPIO_CAM_3M_nRST			S5P64XX_GPJ1(5)	
#define GPIO_CAM_3M_nRST_STATE		1


/*GPJ2 */  // DPRAM Data

#define GPIO_CP_RST					S5P64XX_GPJ2(0)	
#define GPIO_CP_RST_STATE			1

#define GPIO_GPS_nRST				S5P64XX_GPJ2(1)	
#define GPIO_GPS_nRST_STATE			1

#define GPIO_GPS_PWR_EN				S5P64XX_GPJ2(2)	
#define GPIO_GPS_PWR_EN_STATE		1

#define GPIO_TA_CURRENT_SEL_AP			S5P64XX_GPJ2(3)	
#define GPIO_TA_CURRENT_SEL_AP_STATE	1

#define GPIO_FM_INT					S5P64XX_GPJ2(4)	
#define GPIO_FM_INT_STATE			0

#define GPIO_FM_BUS_nRST			S5P64XX_GPJ2(5)	
#define GPIO_FM_BUS_nRST_STATE		1

#define GPIO_BT_WAKE				S5P64XX_GPJ2(6)	
#define GPIO_BT_WAKE_STATE			1

#define GPIO_WLAN_WAKE				S5P64XX_GPJ2(7)	
#define GPIO_WLAN_WAKE_STATE		1


/*GPJ3 */  // DPRAM Data

#define GPIO_WLAN_BT_EN				S5P64XX_GPJ3(0)	
#define GPIO_WLAN_BT_EN_STATE		1

#define GPIO_EAR_USB_SW				S5P64XX_GPJ3(1)	
#define GPIO_EAR_USB_SW_STATE		1

// GPJ1(5) : N.C

#define GPIO_BOOT_MODE				S5P64XX_GPJ3(3)	
#define GPIO_BOOT_MODE_STATE		0

#define GPIO_TOUCH_EN				S5P64XX_GPJ3(4)	
#define GPIO_TOUCH_EN_STATE			1

#define GPIO_ALS_nRST				S5P64XX_GPJ3(5)	
#define GPIO_ALS_nRST_STATE			1

#define GPIO_EAR_ADC_SEL1			S5P64XX_GPJ3(6)	
#define GPIO_EAR_ADC_SEL1_STATE		1

#define GPIO_EAR_ADC_SEL2			S5P64XX_GPJ3(7)	
#define GPIO_EAR_ADC_SEL2_STATE		1


/* GPJ4 */ //DPRAM IF

#define GPIO_AP_PMIC_SDA			S5P64XX_GPJ4(0)
#define GPIO_AP_PMIC_SDA_STATE		1

#define GPIO_KEY_LED_ON				S5P64XX_GPJ4(1)
#define GPIO_KEY_LED_ON_STATE		1

#define GPIO_MICBIAS_EN				S5P64XX_GPJ4(2)
#define GPIO_MICBIAS_EN_STATE		1

#define GPIO_AP_PMIC_SCL			S5P64XX_GPJ4(3)
#define GPIO_AP_PMIC_SCL_STATE		1

#define GPIO_TV_EN					S5P64XX_GPJ4(4)
#define GPIO_TV_EN_STATE			1


// MP0_1(0~7)

// MP0_1(0) : N.C

#define GPIO_DISPLAY_CS				S5P64XX_MP01(1)
#define GPIO_DISPLAY_CS_STATE		1

#define GPIO_AP_NANDCS				S5P64XX_MP01(2)
#define GPIO_AP_NANDCS_STATE		4

// MP0_2(0~3)

// MP0_3(0~4)

// MP0_4(0~7)
#define GPIO_USB_SEL				S5P64XX_MP04(0)
#define GPIO_USB_SEL_STATE			1

#define GPIO_DISPLAY_CLK			S5P64XX_MP04(1)
#define GPIO_DISPLAY_CLK_STATE		1

// MP0_4(2) : N.C	

#define GPIO_DISPLAY_SI				S5P64XX_MP04(3)
#define GPIO_DISPLAY_SI_STATE		1

#define GPIO_LCD_ID					S5P64XX_MP04(4)
#define GPIO_LCD_ID_STATE			1

#define GPIO_TA_EN					S5P64XX_MP04(5)
#define GPIO_TA_EN_STATE			1

#define GPIO_GPS_CLK_ON				S5P64XX_MP04(6)
#define GPIO_GPS_CLK_ON_STATE		1

#define GPIO_HW_REV_MODE0			S5P64XX_MP04(7)
#define GPIO_HW_REV_MODE0_STATE		0


// MP0_5(0~7)
#define GPIO_HW_REV_MODE1			S5P64XX_MP05(0)	// H: Codec,		L: CP
#define GPIO_HW_REV_MODE1_STATE		0

#define GPIO_HW_REV_MODE2			S5P64XX_MP05(1)
#define GPIO_HW_REV_MODE2_STATE		0

#define GPIO_AP_SCL					S5P64XX_MP05(2)
#define GPIO_AP_SCL_STATE			1

#define GPIO_AP_SDA					S5P64XX_MP05(3)
#define GPIO_AP_SDA_STATE			1

// MP0_5(4) : N.C	

#define GPIO_MLCD_RST				S5P64XX_MP05(5)
#define GPIO_MLCD_RST_STATE			1

// MP0_5(6) : N.C	

#define GPIO_UART_SEL				S5P64XX_MP05(7)
#define GPIO_UART_SEL_STATE			1

// MP0_6(0~7)

// MP0_7(0~7)		

#if 0
#define GPIO_ROW(x) S5P64XX_GPH3(x)
#define GPIO_COL(x) S5P64XX_GPH2(x)
#else
#define GPIO_ROW(x) S5P64XX_GPH3(x)
#define GPIO_COL(x) S5P64XX_GPJ0(x)
#endif

#endif	/* MACH_LIBRA_H */


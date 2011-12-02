/* arch/arm/plat-s5p64xx/include/plat/power_clk_gating.h
 *
 *
 *  Copyright (C) 2009 Samsung Electronics.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef __POWER_CLK_GATING_H

#define __POWER_CLK_GATING_H __FILE__ 
#include <mach/hardware.h>

#ifdef CONFIG_S5P64XX_POWER_GATING

#define S5P6442_POWER_GATING_MFC
#define S5P6442_POWER_GATING_G3D
#define S5P6442_POWER_GATING_LCD
#define S5P6442_POWER_GATING_TV
#define S5P6442_POWER_GATING_CAM
//#define S5P6442_POWER_GATING_AUDIO
#if !defined(CONFIG_S5P64XX_LPAUDIO) && !defined(CONFIG_SND_S5P_RP)
#define S5P6442_POWER_GATING_IROM
#endif
#define S5P6442_LP_MODE         0
#define S5P6442_ACTIVE_MODE     1

#define S5P6442_TOP_ID		0
#define S5P6442_MFC_ID		1
#define S5P6442_G3D_ID		2
#define S5P6442_LCD_ID		3
#define S5P6442_TV_ID		4
#define S5P6442_CAM_ID		5
#define S5P6442_AUDIO_ID	7
#define S5P6442_IROM_ID		20

extern void s5p6442_pwrgate_init(void);
extern int s5p6442_pwrgate_config(unsigned int blkID, unsigned int flag);
extern void s5p6442_idle_pm_gpiocfg(unsigned int blkID, unsigned int flag);
extern int s5p6442_blkpower_state(unsigned int blkID);
#endif /* CONFIG_S5P64XX_POWER_GATING */

#ifdef CONFIG_S5P64XX_CLOCK_GATING

#define S5P6442_CLK_GATING_MFC
#define S5P6442_CLK_GATING_G3D
#define S5P6442_CLK_GATING_LCD
#define S5P6442_CLK_GATING_TV
#define S5P6442_CLK_GATING_CAM
//#define S5P6442_CLK_GATING_AUDIO

#define S5P6442_G3D_CLKBLK			0
#define S5P6442_MFC_CLKBLK			1
#define S5P6442_CAM_CLKBLK			2
#define S5P6442_LCD_CLKBLK			3
#define S5P6442_TV_CLKBLK			4
#define S5P6442_GENERAL_CLKBLK	5
#define S5P6442_FILE_CLKBLK			6
#define S5P6442_ARMSUB_CLKBLK		7
#define S5P6442_AUDIO_CLKBLK		8 

extern void s5p6442_clkgate_init(void);
extern int s5p6442_clkgate_config(unsigned int clkblkID, unsigned int flag);	
#endif /* CONFIG_S5P64XX_CLOCK_GATING */
#endif /* CONFIG_PM */

/* arch/arm/plat-s5p64xx/power_clk_gating.c
 *
 *  Copyright (C) 2009 Samsung Electronics.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
 
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <plat/regs-clock.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/power_clk_gating.h>

//extern void s3c_fimc0_cfg_gpio(struct platform_device *dev);
extern int cam_idle_gpio_cfg(int flag);
#ifdef CONFIG_S5P64XX_CLOCK_GATING

#ifdef CONFIG_S5P64XX_CLOCK_GATING_DEBUG
static char *clkblock_name[] = {"G3D","MFC", "CAM", "LCD", "TV", "GENERAL", "FILE", "ARMSUB", "AUDIO"};
#endif /* CONFIG_S5P64XX_CLOCK_GATING_DEBUG */

int s5p6442_clkgate_config(unsigned int clkblkID, unsigned int flag)	
{
	u32 tmp;

	tmp = __raw_readl(S5P_CLKGATE_BLOCK);

	if(flag == S5P6442_ACTIVE_MODE) {  
		tmp |= (1 << clkblkID);
		__raw_writel(tmp, S5P_CLKGATE_BLOCK);
#ifdef CONFIG_S5P64XX_CLOCK_GATING_DEBUG
		printk("===== Block-%s clock ON CLKGATE_BLOCK : %x \n",  clkblock_name[clkblkID], tmp);
#endif /* CONFIG_S5P64XX_CLOCK_GATING_DEBUG */		
	}
	else if(flag == S5P6442_LP_MODE) { 
		tmp &= ~(1 << clkblkID);
		__raw_writel(tmp, S5P_CLKGATE_BLOCK);
#ifdef CONFIG_S5P64XX_CLOCK_GATING_DEBUG
		printk("===== Block-%s clock OFF CLKGATE_BLOCK : %x \n",  clkblock_name[clkblkID], tmp);
#endif /* CONFIG_S5P64XX_CLOCK_GATING_DEBUG */		
	}
	return 0;
}

void s5p6442_clkgate_init(void)
{
	u32 tmp;

	tmp = __raw_readl(S5P_CLKGATE_BLOCK);

#ifdef S5P6442_CLK_GATING_MFC
	tmp &= ~S5P_CLKGATE_BLOCK_MFC;
#endif /* S5P6442_CLK_GATING_MFC */

#ifdef S5P6442_CLK_GATING_G3D
	tmp &= ~S5P_CLKGATE_BLOCK_G3D;
#endif /* S5P6442_CLK_GATING_G3D */

#ifdef S5P6442_CLK_GATING_TV
	tmp &= ~S5P_CLKGATE_BLOCK_TV;
#endif /* S5P6442_CLK_GATING_TV */

#ifdef S5P6442_CLK_GATING_CAM
	tmp &= ~S5P_CLKGATE_BLOCK_CAM;
#endif /* S5P6442_CLK_GATING_CAM */

#ifdef S5P6442_CLK_GATING_AUDIO
	tmp &= ~S5P_CLKGATE_BLOCK_AUDIO;
#endif /* S5P6442_CLK_GATING_AUDIO */

	__raw_writel(tmp, S5P_CLKGATE_BLOCK);

/* IP0 clk Gating */
	tmp = __raw_readl(S5P_CLKGATE_IP0);
	tmp &= ~(S5P_CLKGATE_IP0_JPEG | S5P_CLKGATE_IP0_ROTATOR | S5P_CLKGATE_IP0_FIMC2);
	__raw_writel(tmp, S5P_CLKGATE_IP0);

/* IP1 clk Gating */
	tmp = __raw_readl(S5P_CLKGATE_IP1);
        tmp &= ~(S5P_CLKGATE_IP1_VP | S5P_CLKGATE_IP1_MIXER | S5P_CLKGATE_IP1_TVENC | S5P_CLKGATE_IP1_USBOTG);
        __raw_writel(tmp, S5P_CLKGATE_IP1);

/* IP2 clk Gating */
	tmp = __raw_readl(S5P_CLKGATE_IP2);
        tmp &= ~(S5P_CLKGATE_IP2_SECSS | S5P_CLKGATE_IP2_HSMMC0 | S5P_CLKGATE_IP2_HSMMC1 | S5P_CLKGATE_IP2_HSMMC2);
        __raw_writel(tmp, S5P_CLKGATE_IP2);

/* IP3 clk Gating */
	tmp = __raw_readl(S5P_CLKGATE_IP3);
	tmp &= ~(S5P_CLKGATE_IP3_PCM0 | S5P_CLKGATE_IP3_PCM1 | /*S5P_CLKGATE_IP3_UART2 |*/ S5P_CLKGATE_IP3_I2S1);
        __raw_writel(tmp, S5P_CLKGATE_IP3); 

	return;
}
#endif /* CONFIG_S5P64XX_CLOCK_GATING */

#ifdef CONFIG_S5P64XX_POWER_GATING

static spinlock_t power_lock;
static int clkblk_hash_map[8]={100, 1, 0, 3, 4, 2, 100, 8};  //100 is arbitrary large number

#ifdef CONFIG_S5P64XX_POWER_GATING_DEBUG
static char *block_name[] = {"IROM","MFC", "G3D", "LCD", "TV", "CAM", "RESERVED", "AUDIO"};
#endif /* CONFIG_S5P64XX_POWER_GATING_DEBUG */

unsigned int gpio_lcd_save[2 * 4];

int s5p6442_blkpower_state(unsigned int blkID)
{
	unsigned int normal_cfg;
	unsigned long flags;
	
	spin_lock_irqsave(&power_lock, flags);
	normal_cfg = __raw_readl(S5P_NORMAL_CFG);
	normal_cfg &= (1 << blkID);
	spin_unlock_irqrestore(&power_lock, flags);
	return normal_cfg;
}

void s5p6442_idle_pm_gpiocfg(unsigned int blkID, unsigned int flag)
{
	u32 tmp;

	if(flag == S5P6442_ACTIVE_MODE) {   
		 if(blkID == S5P6442_CAM_ID) {
		//	cam_idle_gpio_cfg(1);  //capture driver should properly control the camera gpio. it is disabled in the assumption that the capture driver does the power on/off properly.
		 /*	s3c_fimc0_cfg_gpio(NULL);
			s3c_gpio_cfgpin(GPIO_CAM_3M_nSTBY, S3C_GPIO_SFN(GPIO_CAM_3M_nSTBY_STATE));
			gpio_set_value(GPIO_CAM_3M_nSTBY, GPIO_LEVEL_HIGH);
			msleep(1);
			s3c_gpio_cfgpin(GPIO_CAM_3M_nRST, S3C_GPIO_SFN(GPIO_CAM_3M_nRST_STATE));
			gpio_set_value(GPIO_CAM_3M_nRST, GPIO_LEVEL_HIGH);
			msleep(40);*/
		 }		
	}
	else if (flag ==S5P6442_LP_MODE) {
		if(blkID == S5P6442_LCD_ID) {
			__raw_writel(0, S5P64XX_GPF0CON);
			__raw_writel(0, S5P64XX_GPF0DAT);
			__raw_writel(0X5555, S5P64XX_GPF0PUD);
			__raw_writel(0, S5P64XX_GPF0DRV);
			
			__raw_writel(0, S5P64XX_GPF1CON);
			__raw_writel(0, S5P64XX_GPF1DAT);
			__raw_writel(0X5555, S5P64XX_GPF1PUD);
			__raw_writel(0, S5P64XX_GPF1DRV);

			__raw_writel(0, S5P64XX_GPF2CON);
			__raw_writel(0, S5P64XX_GPF2DAT);
			__raw_writel(0X5555, S5P64XX_GPF2PUD);
			__raw_writel(0, S5P64XX_GPF2DRV);

			__raw_writel(0, S5P64XX_GPF3CON);
			__raw_writel(0, S5P64XX_GPF3DAT);
			__raw_writel(0X5555, S5P64XX_GPF3PUD);
			__raw_writel(0, S5P64XX_GPF3DRV);			
		}
		else if(blkID == S5P6442_CAM_ID) {	
		//	cam_idle_gpio_cfg(0);////capture driver should properly control the camera gpio. it is disabled in the assumption that the capture driver does the power on/off properly.
/*			gpio_set_value(GPIO_CAM_3M_nSTBY, GPIO_LEVEL_LOW);
			msleep(1);
			gpio_set_value(GPIO_CAM_3M_nRST, GPIO_LEVEL_LOW);	
			msleep(1);
			
			__raw_writel(0, S5P64XX_GPE0CON);
			__raw_writel(0, S5P64XX_GPE0DAT);
			__raw_writel(0X5555, S5P64XX_GPE0PUD);

			tmp = __raw_readl(S5P64XX_GPE1CON);
			tmp &= (0XF << 16);
			__raw_writel(tmp, S5P64XX_GPE1CON);

			tmp = __raw_readl(S5P64XX_GPE1DAT);
			tmp &= (0X1 << 4);
			__raw_writel(tmp, S5P64XX_GPE1DAT);

			tmp = __raw_readl(S5P64XX_GPE1PUD);
			tmp &= (0X3 << 8);
			tmp |= (0x55);
			__raw_writel(tmp, S5P64XX_GPE1PUD);		*/
		}
	}
	return;
}

int s5p6442_pwrgate_config(unsigned int blkID, unsigned int flag)	
{
	unsigned int normal_cfg;
	unsigned int config;
	unsigned int blk_pwr_stat;
	unsigned int tmp;
	int timeout;
	int ret = 0; 
	unsigned long flags; 

	/* Wait max 100 ms */
	timeout = 100 * 10;  //delay in 100 Us

	config = (1 << blkID);
	
	if(blkID == S5P6442_IROM_ID)
		blkID = 0;
			
		
	
	spin_lock_irqsave(&power_lock, flags);
	normal_cfg = __raw_readl(S5P_NORMAL_CFG);
	   
	if(flag == S5P6442_ACTIVE_MODE) {   
		if(!(normal_cfg & config)) {
			normal_cfg |= config;
			__raw_writel(normal_cfg, S5P_NORMAL_CFG);
#ifdef CONFIG_S5P64XX_CLOCK_GATING
			tmp = clkblk_hash_map[blkID];
			if(tmp < 32) {
				s5p6442_clkgate_config(tmp, flag);
			}			
#endif /* CONFIG_S5P64XX_CLOCK_GATING */
			while (!((blk_pwr_stat = __raw_readl(S5P_BLK_PWR_STAT)) & config)) {
				if(timeout == 0) {
					printk(KERN_ERR "config %x: blk power never ready.\n", config);
					ret = 1;
					goto s5p_wait_blk_pwr_stable_end;
				}

				timeout--;
				udelay(100);
			}					
#ifdef CONFIG_S5P64XX_POWER_GATING_DEBUG
			printk("===== Block-%s Power ON NORMAL_CFG : %x \n",  block_name[blkID], normal_cfg);
#endif /* CONFIG_S5P64XX_POWER_GATING_DEBUG */
		}		
	}
	else if(flag == S5P6442_LP_MODE) {
		if(normal_cfg & config) {
#ifdef CONFIG_S5P64XX_CLOCK_GATING
			tmp = clkblk_hash_map[blkID];
			if(tmp < 32) {
				s5p6442_clkgate_config(tmp, flag);
			}		
#endif /* CONFIG_S5P64XX_CLOCK_GATING */
			normal_cfg &= ~config;
			__raw_writel(normal_cfg, S5P_NORMAL_CFG);
			while (((blk_pwr_stat = __raw_readl(S5P_BLK_PWR_STAT)) & config)) {
				if(timeout == 0) {
					printk(KERN_ERR "config %x: blk power never ready.\n", config);
					ret = 1;
					goto s5p_wait_blk_pwr_stable_end;
				}

				timeout--;
				udelay(100);
			}		
#ifdef CONFIG_S5P64XX_POWER_GATING_DEBUG
			printk("===== Block-%s Power OFF NORMAL_CFG : %x \n",  block_name[blkID], normal_cfg);
#endif /* CONFIG_S5P64XX_POWER_GATING_DEBUG */
		}
	}
s5p_wait_blk_pwr_stable_end:	
	spin_unlock_irqrestore(&power_lock, flags);	
	return ret;
}

void s5p6442_pwrgate_init(void)
{
	u32 tmp;

	spin_lock_init(&power_lock);	
	s5p6442_clkgate_init();

	tmp = __raw_readl(S5P_NORMAL_CFG);

#ifdef S5P6442_POWER_GATING_MFC
	tmp &= ~(1 << S5P6442_MFC_ID);
#endif /* S5P6442_POWER_GATING_MFC */

#ifdef S5P6442_POWER_GATING_G3D
	tmp &= ~(1 << S5P6442_G3D_ID);
#endif /* S5P6442_POWER_GATING_G3D */

#ifdef S5P6442_POWER_GATING_TV
	tmp &= ~(1 << S5P6442_TV_ID);
#endif /* S5P6442_POWER_GATING_TV */

#ifdef S5P6442_POWER_GATING_CAM
	tmp &= ~(1 << S5P6442_CAM_ID);
#endif /* S5P6442_POWER_GATING_CAM */

#ifdef S5P6442_POWER_GATING_AUDIO
	tmp &= ~(1 << S5P6442_AUDIO_ID);
#endif /* S5P6442_POWER_GATING_AUDIO */

#ifdef S5P6442_POWER_GATING_IROM
	tmp &= ~(1 << S5P6442_IROM_ID);
#endif
	__raw_writel(tmp, S5P_NORMAL_CFG);
	
	return;
}
#endif /* CONFIG_S5P64XX_POWER_GATING */
/* linux/arch/arm/mach-s5p6442/setup-sdhci.c
 *
 * Copyright 2008 Simtec Electronics
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 * Copyright (C) 2010 Samsung Electronics Co. Ltd.
 *
 * S5P6442 - Helper functions for settign up SDHCI device(s) (HSMMC)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/io.h>

#include <linux/mmc/card.h>
#include <linux/mmc/host.h>

#include <mach/gpio.h>
#include <mach/regs-irq.h>
#include <mach/hardware.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>
#include <plat/regs-sdhci.h>
#include <plat/sdhci.h>

/* clock sources for the mmc bus clock, order as for the ctrl2[5..4] */
char *s5p6442_hsmmc_clksrcs[3] = {
	[0] = "hsmmc",
	[1] = "hsmmc",
	[2] = "hsmmc",
};

void s5p6442_setup_sdhci0_cfg_gpio(struct platform_device *dev, int width)
{
	unsigned int gpio = 0;
	unsigned int end = 0;

	/* Clock Setting */
	s3c_gpio_cfgpin(GPIO_NAND_CLK, S3C_GPIO_SFN(GPIO_NAND_CLK_STATE));
	s3c_gpio_setpull(GPIO_NAND_CLK, S3C_GPIO_PULL_NONE);

	/* Command Line Setting */
	s3c_gpio_cfgpin(GPIO_NAND_CMD, S3C_GPIO_SFN(GPIO_NAND_CMD_STATE));
	s3c_gpio_setpull(GPIO_NAND_CMD, S3C_GPIO_PULL_NONE);

	/* Data Line Setting */
	switch(width)
	{
		case 1:
			end = GPIO_NAND_D_0;
			break;
		case 2:
			end = GPIO_NAND_D_1;
			break;
		case 3:
			end = GPIO_NAND_D_2;
			break;
		case 4:
			end = GPIO_NAND_D_3;
			break;
		default :
			width = 0;
			break;
	}

	if(width !=0)
	{
	        for (gpio = GPIO_NAND_D_0; gpio <= end; gpio++) 
		{
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
		}
	}
}

void s5p6442_setup_sdhci0_cfg_card(struct platform_device *dev,
				    void __iomem *r,
				    struct mmc_ios *ios,
				    struct mmc_card *card)
{
	u32 ctrl2, ctrl3 = (S3C_SDHCI_CTRL3_FCSEL1 | S3C_SDHCI_CTRL3_FCSEL0);

	/* don't need to alter anything acording to card-type */
	//writel(S3C64XX_SDHCI_CONTROL4_DRIVE_2mA, r + S3C64XX_SDHCI_CONTROL4);

	ctrl2 = readl(r + S3C_SDHCI_CONTROL2);
	ctrl2 &= S3C_SDHCI_CTRL2_SELBASECLK_MASK;
	ctrl2 |= (S3C64XX_SDHCI_CTRL2_ENSTAASYNCCLR |
		  S3C64XX_SDHCI_CTRL2_ENCMDCNFMSK |
//		  S3C_SDHCI_CTRL2_ENFBCLKRX |
 		  S3C_SDHCI_CTRL2_DFCNT_NONE |
		  S3C_SDHCI_CTRL2_ENCLKOUTHOLD);

	/* Workaround for MMC+ cards */
	if (ios->timing == MMC_TIMING_MMC_HS && ios->clock != 26 * 1000000)
		ctrl3 = 0; 

	/* SCLK_MMC base clock selected */
	ctrl2 |= (2 << S3C_SDHCI_CTRL2_SELBASECLK_SHIFT);

	writel(ctrl2, r + S3C_SDHCI_CONTROL2);
	writel(ctrl3, r + S3C_SDHCI_CONTROL3);
}

void s5p6442_setup_sdhci1_cfg_gpio(struct platform_device *dev, int width)
{
	unsigned int gpio = 0;
	unsigned int end = 0;

	/* Clock Setting */
	s3c_gpio_cfgpin(GPIO_WLAN_SDIO_CLK, S3C_GPIO_SFN(GPIO_WLAN_SDIO_CLK_STATE));
	s3c_gpio_setpull(GPIO_WLAN_SDIO_CLK, S3C_GPIO_PULL_NONE);

	/* Command Line Setting */
	s3c_gpio_cfgpin(GPIO_WLAN_SDIO_CMD, S3C_GPIO_SFN(GPIO_WLAN_SDIO_CMD_STATE));
	s3c_gpio_setpull(GPIO_WLAN_SDIO_CMD, S3C_GPIO_PULL_NONE);

	/* Data Line Setting */
	switch(width)
	{
		case 1:
			end = GPIO_WLAN_SDIO_D_0;
			break;
		case 2:
			end = GPIO_WLAN_SDIO_D_1;
			break;
		case 3:
			end = GPIO_WLAN_SDIO_D_2;
			break;
		case 4:
			end = GPIO_WLAN_SDIO_D_3;
			break;
		default :
			width = 0;
			break;
	}

	if(width !=0)
	{
	        for (gpio = GPIO_WLAN_SDIO_D_0; gpio <= end; gpio++) 
		{
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
		}
	}
}

void s5p6442_setup_sdhci2_cfg_gpio(struct platform_device *dev, int width)
{
	unsigned int gpio = 0;
	unsigned int end = 0;

	/* Clock Setting */
	s3c_gpio_cfgpin(GPIO_T_FLASH_CLK, S3C_GPIO_SFN(GPIO_T_FLASH_CLK_STATE));
	s3c_gpio_setpull(GPIO_T_FLASH_CLK, S3C_GPIO_PULL_NONE);

	/* Command Line Setting */
	s3c_gpio_cfgpin(GPIO_T_FLASH_CMD, S3C_GPIO_SFN(GPIO_T_FLASH_CMD_STATE));
	s3c_gpio_setpull(GPIO_T_FLASH_CMD, S3C_GPIO_PULL_NONE);

	/* Data Line Setting */
	switch(width)
	{
		case 1:
			end = GPIO_T_FLASH_D_0;
			break;
		case 2:
			end = GPIO_T_FLASH_D_1;
			break;
		case 3:
			end = GPIO_T_FLASH_D_2;
			break;
		case 4:
			end = GPIO_T_FLASH_D_3;
			break;
		default :
			width = 0;
			break;
	}

	if(width !=0)
	{
	        for (gpio = GPIO_T_FLASH_D_0; gpio <= end; gpio++) 
		{
			s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(2));
			s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
		}
	}

	/* for external card detection */
	s3c_gpio_cfgpin(GPIO_T_FLASH_DETECT, S3C_GPIO_SFN(GPIO_T_FLASH_DETECT_STATE));
	s3c_gpio_setpull(GPIO_T_FLASH_DETECT, S3C_GPIO_PULL_NONE);

	/* Drive Stregth Setting */
	s3c_gpio_set_drv_str(GPIO_T_FLASH_CLK, GPIO_DRV_SR_3X);
	s3c_gpio_set_drv_str(GPIO_T_FLASH_CMD, GPIO_DRV_SR_3X);
	s3c_gpio_set_drv_str(GPIO_T_FLASH_D_0, GPIO_DRV_SR_3X);
	s3c_gpio_set_drv_str(GPIO_T_FLASH_D_1, GPIO_DRV_SR_3X);
	s3c_gpio_set_drv_str(GPIO_T_FLASH_D_2, GPIO_DRV_SR_3X);
	s3c_gpio_set_drv_str(GPIO_T_FLASH_D_3, GPIO_DRV_SR_3X);
}

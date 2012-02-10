/* drivers
 *
 * Copyright (c) 2008 Samsung Electronics
 *
 * Samsung BCM4325 Bluetooth power sequence through GPIO of S3C6410.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 * Revision History
 * ===============
 * 0.0 Initial version WLAN power wakeup
 * 
 */
#include <linux/delay.h>
#include <linux/spinlock.h>

#include <mach/gpio.h>
#include <mach/hardware.h>

#include <plat/gpio-cfg.h>
#include <plat/sdhci.h>
#include <plat/devs.h>

#ifdef CUSTOMER_HW_SAMSUNG

static unsigned long flags = 0;
static spinlock_t regon_lock = SPIN_LOCK_UNLOCKED;

extern void s5p_config_gpio_alive_table(int array_size, int (*gpio_table)[4]);

static int wlan_gpio_table[][4] = {	
	{GPIO_WLAN_nRST, GPIO_WLAN_nRST_STATE, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_HOST_WAKE, GPIO_WLAN_HOST_WAKE_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN},
	{GPIO_WLAN_WAKE, GPIO_WLAN_WAKE_STATE, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},	
};

static int wlan_sdio_on_table[][4] = {
	{GPIO_WLAN_SDIO_CLK, GPIO_WLAN_SDIO_CLK_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_CMD, GPIO_WLAN_SDIO_CMD_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D_0, GPIO_WLAN_SDIO_D_0_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D_1, GPIO_WLAN_SDIO_D_1_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D_2, GPIO_WLAN_SDIO_D_2_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D_3, GPIO_WLAN_SDIO_D_3_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
};

static int wlan_sdio_off_table[][4] = {
	{GPIO_WLAN_SDIO_CLK, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_CMD, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D_0, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D_1, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D_2, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D_3, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
};

void bcm_wlan_power_on(int flag)
{
	if (flag != 1)
		return;
	
	s5p_config_gpio_alive_table(ARRAY_SIZE(wlan_gpio_table), wlan_gpio_table);
	
	printk("[WIFI] Device powering ON\n");
	s5p_config_gpio_alive_table(ARRAY_SIZE(wlan_sdio_on_table), wlan_sdio_on_table);
/* 
 *	PROTECT this check under spinlock.. No other thread should be touching
 *	GPIO_BT_REG_ON at this time.. If BT is operational, don't touch it. 
 */
	spin_lock_irqsave(&regon_lock, flags);
	
	/* need delay between v_bat & reg_on for 2 cycle @ 38.4MHz */
	udelay(5);
	gpio_set_value(GPIO_WLAN_BT_EN, GPIO_LEVEL_HIGH);
	s3c_gpio_pdn_cfgpin(GPIO_WLAN_BT_EN, S3C_GPIO_SLP_OUT1);
	
	gpio_set_value(GPIO_WLAN_nRST, GPIO_LEVEL_HIGH);
	s3c_gpio_pdn_cfgpin(GPIO_WLAN_nRST, S3C_GPIO_SLP_OUT1);
	
	printk(KERN_DEBUG "[WIFI] GPIO_WLAN_BT_EN = %d, GPIO_WLAN_nRST = %d\n", 
			gpio_get_value(GPIO_WLAN_BT_EN), gpio_get_value(GPIO_WLAN_nRST));

	spin_unlock_irqrestore(&regon_lock, flags);
    
	/* mmc_rescan*/    
	sdhci_s3c_force_presence_change(&s3c_device_hsmmc1);
}

void bcm_wlan_power_off(int flag)
{
	if (flag != 1)
		return;
	
	printk("[WIFI] Device powering OFF\n");

	/* PROTECT this check under spinlock.. No other thread should be touching
	   GPIO_BT_REG_ON at this time.. If BT is operational, don't touch it. */
	spin_lock_irqsave(&regon_lock, flags);
	
	/* need delay between v_bat & reg_on for 2 cycle @ 38.4MHz */
	udelay(5);
	if (gpio_get_value(GPIO_BT_nRST) == 0) {
		gpio_set_value(GPIO_WLAN_BT_EN, GPIO_LEVEL_LOW);	
		s3c_gpio_pdn_cfgpin(GPIO_WLAN_BT_EN, S3C_GPIO_SLP_OUT0);		
	}
	
	gpio_set_value(GPIO_WLAN_nRST, GPIO_LEVEL_LOW);
	s3c_gpio_pdn_cfgpin(GPIO_WLAN_nRST, S3C_GPIO_SLP_OUT0);
	
	printk(KERN_DEBUG "[WIFI] GPIO_WLAN_BT_EN = %d, GPIO_WLAN_nRST = %d\n", 
			gpio_get_value(GPIO_WLAN_BT_EN), gpio_get_value(GPIO_WLAN_nRST));

	spin_unlock_irqrestore(&regon_lock, flags);

	s5p_config_gpio_alive_table(ARRAY_SIZE(wlan_sdio_off_table), wlan_sdio_off_table);
	
	/* mmc_rescan*/    
	sdhci_s3c_force_presence_change(&s3c_device_hsmmc1);
}
#endif /* CUSTOMER_HW_SAMSUNG */

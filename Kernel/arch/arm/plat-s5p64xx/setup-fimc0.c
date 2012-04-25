/* linux/arch/arm/plat-s5p64xx/setup-fimc0.c
 *
 * Copyright 2009 Samsung Electronics
 *	Jinsung Yang <jsgood.yang@samsung.com>
 *	http://samsungsemi.com/
 *
 * Base S5P64XX FIMC controller 0 gpio configuration
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <plat/clock.h>
#include <plat/gpio-cfg.h>
#include <plat/gpio-bank-e0.h>
#include <plat/gpio-bank-e1.h>
#include <plat/gpio-bank-j0.h>
#include <plat/gpio-bank-j1.h>
#include <plat/regs-gpio.h>
#include <asm/io.h>
#include <mach/map.h>

struct platform_device; /* don't need the contents */

void s3c_fimc0_cfg_gpio(struct platform_device *dev)
{
	int i;

	s3c_gpio_cfgpin(S5P64XX_GPE0(0), S5P64XX_GPE0_0_CAM_A_PCLK);
	s3c_gpio_cfgpin(S5P64XX_GPE0(1), S5P64XX_GPE0_1_CAM_A_VSYNC);
	s3c_gpio_cfgpin(S5P64XX_GPE0(2), S5P64XX_GPE0_2_CAM_A_HREF);
	s3c_gpio_cfgpin(S5P64XX_GPE0(3), S5P64XX_GPE0_3_CAM_A_DATA_0);
	s3c_gpio_cfgpin(S5P64XX_GPE0(4), S5P64XX_GPE0_4_CAM_A_DATA_1);
	s3c_gpio_cfgpin(S5P64XX_GPE0(5), S5P64XX_GPE0_5_CAM_A_DATA_2);
	s3c_gpio_cfgpin(S5P64XX_GPE0(6), S5P64XX_GPE0_6_CAM_A_DATA_3);
	s3c_gpio_cfgpin(S5P64XX_GPE0(7), S5P64XX_GPE0_7_CAM_A_DATA_4);
	s3c_gpio_cfgpin(S5P64XX_GPE1(0), S5P64XX_GPE1_0_CAM_A_DATA_5);
	s3c_gpio_cfgpin(S5P64XX_GPE1(1), S5P64XX_GPE1_1_CAM_A_DATA_6);
	s3c_gpio_cfgpin(S5P64XX_GPE1(2), S5P64XX_GPE1_2_CAM_A_DATA_7);
	s3c_gpio_cfgpin(S5P64XX_GPE1(3), S5P64XX_GPE1_3_CAM_A_CLKOUT);
	//s3c_gpio_cfgpin(S5P64XX_GPE1(4), S5P64XX_GPE1_4_CAM_A_FIELD);
	
	for (i = 0; i < 8; i++)
		s3c_gpio_setpull(S5P64XX_GPE0(i), S3C_GPIO_PULL_NONE);

	for (i = 0; i < 4; i++)
		s3c_gpio_setpull(S5P64XX_GPE1(i), S3C_GPIO_PULL_NONE);

	/* drive strength to 4mA */
	writel(1 << 6, S5P64XX_GPE1DRV);
}

int s3c_fimc_clk_on(struct platform_device *pdev, struct clk *clk)
{
	struct clk *lclk = NULL, *lclk_parent = NULL;
	int err;

	lclk = clk_get(&pdev->dev, "lclk_fimc");
	if (IS_ERR(lclk)) {
		dev_err(&pdev->dev, "failed to get local clock\n");
		goto err_clk1;
	}

	if (lclk->set_parent) {
		lclk_parent = clk_get(&pdev->dev, "mout_mpll");
		if (IS_ERR(lclk_parent)) {
			dev_err(&pdev->dev, "failed to get parent of local clock\n");
			goto err_clk2;
		}

		lclk->parent = lclk_parent;

		err = lclk->set_parent(lclk, lclk_parent);
		if (err) {
			dev_err(&pdev->dev, "failed to set parent of local clock\n");
			goto err_clk3;
		}
#if 0
		if (lclk->set_rate) {
			lclk->set_rate(lclk, 133000000);
			dev_info(&pdev->dev, "set local clock rate to 133000000\n");
		}
#endif

		clk_put(lclk_parent);
	}

	clk_put(lclk);

	/* be able to handle clock on/off only with this clock */
	clk = clk_get(&pdev->dev, "fimc");
	if (IS_ERR(clk)) {
		dev_err(&pdev->dev, "failed to get interface clock\n");
		goto err_clk3;
	}

	clk_enable(clk);

	return 0;

err_clk3:
	clk_put(lclk_parent);

err_clk2:
	clk_put(lclk);

err_clk1:
	return -EINVAL;
}

int s3c_fimc0_clk_off(struct platform_device *pdev, struct clk *clk)
{
	clk_disable(clk);
	clk_put(clk);

	clk = NULL;

	return 0;
}

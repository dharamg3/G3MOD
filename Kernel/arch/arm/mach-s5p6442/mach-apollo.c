/* linux/arch/arm/mach-s5p6442/mach-apollo.c
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 * Copyright 2010 Samsung Electronics Co. Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/partitions.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/pwm_backlight.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/bootmem.h>
#include <linux/reboot.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include <mach/hardware.h>
#include <mach/map.h>
#include <mach/regs-mem.h>
#include <mach/param.h>

#include <asm/irq.h>
#include <asm/mach-types.h>
#include <asm/setup.h>

#include <plat/regs-serial.h>
#include <plat/fimc.h>
#include <plat/iic.h>
#include <plat/fb.h>

#include <plat/nand.h>
#include <plat/partition.h>
#include <plat/s5p6442.h>
#include <plat/clock.h>
#include <plat/regs-clock.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/adcts.h>
#include <plat/ts.h>
#include <plat/pm.h>
#include <plat/pll.h>
#include <plat/regs-rtc.h>
#include <plat/spi.h>
#include <plat/regs-iic.h>

#include <mach/gpio.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-cfg.h>

#include <plat/sdhci.h> //cjh for WIFI

#include <linux/android_pmem.h>
#include <plat/reserved_mem.h>

// for fimc driver
#include <linux/videodev2.h>
#include <media/s5k4ca_platform.h>

#include <plat/regs-fimc.h>
#include <plat/regs-clock.h>
#include <plat/fimc.h>

#include <linux/i2c/max8998.h>

#ifdef CONFIG_SEC_HEADSET
#include <mach/sec_headset.h>
#endif

#ifdef CONFIG_USB_SUPPORT
#include <plat/regs-usb-otg-hs.h>
#include <linux/usb/ch9.h>

/* S3C_USB_CLKSRC 0: EPLL 1: CLK_48M */
#define S3C_USB_CLKSRC	1
#define OTGH_PHY_CLK_VALUE      (0x02)  /* UTMI Interface, Cristal, 12Mhz clk for PLL */
#endif

#ifdef CONFIG_PHONET
#include <linux/modemctl.h>
#include <linux/onedram.h>
#endif

#ifdef CONFIG_KERNEL_PANIC_DUMP
#define S3C64XX_KERNEL_PANIC_DUMP_SIZE 0x8000	/* 32KB */

void *S3C64XX_KERNEL_PANIC_DUMP_ADDR;
#endif

#define UCON S3C2410_UCON_DEFAULT | S3C2410_UCON_UCLK
#define ULCON S3C2410_LCON_CS8 | S3C2410_LCON_PNONE | S3C2410_LCON_STOPB
#define UFCON S3C2410_UFCON_RXTRIG8 | S3C2410_UFCON_FIFOMODE

struct class *sec_class;
EXPORT_SYMBOL(sec_class);

struct device *switch_dev;
EXPORT_SYMBOL(switch_dev);

void (*sec_set_param_value)(int idx, void *value);
EXPORT_SYMBOL(sec_set_param_value);

void (*sec_get_param_value)(int idx, void *value);
EXPORT_SYMBOL(sec_get_param_value);

void s5p6442_init_gpio(void);

#if defined(CONFIG_MMC_SDHCI_S3C)
extern void s3c_sdhci_set_platdata(void);
#endif

extern void s3c64xx_reserve_bootmem(void);
#if! defined (CONFIG_HIGH_RES_TIMERS)
extern struct sys_timer s5p64xx_timer;
#else
#if defined (CONFIG_HRT_RTC)
extern struct sys_timer s5p64xx_timer;
#else
extern struct sys_timer sec_timer;
#endif
#endif

void __iomem		*Cam_i2c0_regs = 0x0; // Add for camera preview issue
#if defined(CONFIG_SPI_CNTRLR_0) || defined(CONFIG_SPI_CNTRLR_1)
static void s3c_cs_suspend(int pin, pm_message_t pm)
{
	/* Whatever need to be done */
}

static void s3c_cs_resume(int pin)
{
	/* Whatever need to be done */
}

static void cs_set(int pin, int lvl)
{
	unsigned int val;

	val = __raw_readl(S5P64XX_GPNDAT);
	val &= ~(1<<pin);

	if(lvl == CS_HIGH)
	   val |= (1<<pin);

	__raw_writel(val, S5P64XX_GPNDAT);
}

static void s3c_cs_setF13(int pin, int lvl)
{
	cs_set(13, lvl);
}

static void s3c_cs_setF14(int pin, int lvl)
{
	cs_set(14, lvl);
}

static void s3c_cs_setF15(int pin, int lvl)
{
	cs_set(15, lvl);
}

static void cs_cfg(int pin, int pull)
{
	unsigned int val;

	val = __raw_readl(S5P64XX_GPNCON);
	val &= ~(3<<(pin*2));
	val |= (1<<(pin*2)); /* Output Mode */
	__raw_writel(val, S5P64XX_GPNCON);

	val = __raw_readl(S5P64XX_GPNPUD);
	val &= ~(3<<(pin*2));

	if(pull == CS_HIGH)
	   val |= (2<<(pin*2));	/* PullUp */
	else
	   val |= (1<<(pin*2)); /* PullDown */

	__raw_writel(val, S5P64XX_GPNPUD);
}

static void s3c_cs_configF13(int pin, int mode, int pull)
{
	cs_cfg(13, pull);
}

static void s3c_cs_configF14(int pin, int mode, int pull)
{
	cs_cfg(14, pull);
}

static void s3c_cs_configF15(int pin, int mode, int pull)
{
	cs_cfg(15, pull);
}

static void s3c_cs_set(int pin, int lvl)
{
	if(lvl == CS_HIGH)
	   s3c_gpio_setpin(pin, 1);
	else
	   s3c_gpio_setpin(pin, 0);
}

static void s3c_cs_config(int pin, int mode, int pull)
{
	s3c_gpio_cfgpin(pin, mode);

	if(pull == CS_HIGH)
	   s3c_gpio_setpull(pin, S3C_GPIO_PULL_UP);
	else
	   s3c_gpio_setpull(pin, S3C_GPIO_PULL_DOWN);
}
#endif

#if defined(CONFIG_SPI_CNTRLR_0)
static struct s3c_spi_pdata s3c_slv_pdata_0[] __initdata = {
	[0] = {	/* Slave-0 */
		.cs_level     = CS_FLOAT,
		.cs_pin       = S5P64XX_GPC(3),
		.cs_mode      = S5P64XX_GPC_OUTPUT(3),
		.cs_set       = s3c_cs_set,
		.cs_config    = s3c_cs_config,
		.cs_suspend   = s3c_cs_suspend,
		.cs_resume    = s3c_cs_resume,
	},
	[1] = {	/* Slave-1 */
		.cs_level     = CS_FLOAT,
		.cs_pin       = S5P64XX_GPF(13),
		.cs_mode      = S5P64XX_GPF_OUTPUT(13),
		.cs_set       = s3c_cs_setF13,
		.cs_config    = s3c_cs_configF13,
		.cs_suspend   = s3c_cs_suspend,
		.cs_resume    = s3c_cs_resume,
	},
};
#endif

struct platform_device sec_device_battery = {
		.name	= "apollo-battery",
		.id		= -1,
};

static struct s5p6442_pmem_setting pmem_setting = {
        .pmem_start = RESERVED_PMEM_START,
        .pmem_size = RESERVED_PMEM,
        .pmem_gpu1_start = GPU1_RESERVED_PMEM_START,
        .pmem_gpu1_size = RESERVED_PMEM_GPU1,
#if 1
        .pmem_render_start = RENDER_RESERVED_PMEM_START,
        .pmem_render_size = RESERVED_PMEM_RENDER,
        .pmem_stream_start = STREAM_RESERVED_PMEM_START,
        .pmem_stream_size = RESERVED_PMEM_STREAM,
        .pmem_preview_start = PREVIEW_RESERVED_PMEM_START,
        .pmem_preview_size = RESERVED_PMEM_PREVIEW,
        .pmem_picture_start = PICTURE_RESERVED_PMEM_START,
        .pmem_picture_size = RESERVED_PMEM_PICTURE,
        .pmem_jpeg_start = JPEG_RESERVED_PMEM_START,
        .pmem_jpeg_size = RESERVED_PMEM_JPEG,
        .pmem_skia_start = SKIA_RESERVED_PMEM_START,
        .pmem_skia_size = RESERVED_PMEM_SKIA,
#endif
};


#if defined(CONFIG_SPI_CNTRLR_1)
static struct s3c_spi_pdata s3c_slv_pdata_1[] __initdata = {
	[0] = {	/* Slave-0 */
		.cs_level     = CS_FLOAT,
		.cs_pin       = S5P64XX_GPC(7),
		.cs_mode      = S5P64XX_GPC_OUTPUT(7),
		.cs_set       = s3c_cs_set,
		.cs_config    = s3c_cs_config,
		.cs_suspend   = s3c_cs_suspend,
		.cs_resume    = s3c_cs_resume,
	},
	[1] = {	/* Slave-1 */
		.cs_level     = CS_FLOAT,
		.cs_pin       = S5P64XX_GPF(14),
		.cs_mode      = S5P64XX_GPF_OUTPUT(14),
		.cs_set       = s3c_cs_setF14,
		.cs_config    = s3c_cs_configF14,
		.cs_suspend   = s3c_cs_suspend,
		.cs_resume    = s3c_cs_resume,
	},
	[2] = {	/* Slave-2 */
		.cs_level     = CS_FLOAT,
		.cs_pin       = S5P64XX_GPF(15),
		.cs_mode      = S5P64XX_GPF_OUTPUT(15),
		.cs_set       = s3c_cs_setF15,
		.cs_config    = s3c_cs_configF15,
		.cs_suspend   = s3c_cs_suspend,
		.cs_resume    = s3c_cs_resume,
	},
};
#endif

static struct s3c2410_uartcfg smdk6442_uartcfgs[] __initdata = {
	[0] = {
		.hwport	     = 0,
		.flags	     = 0,
		.ucon	     = S3C64XX_UCON_DEFAULT,
		.ulcon	     = S3C64XX_ULCON_DEFAULT,
		.ufcon	     = S3C64XX_UFCON_DEFAULT,
	},
	[1] = {
		.hwport	     = 1,
		.flags	     = 0,
		.ucon	     = S3C64XX_UCON_DEFAULT,
		.ulcon	     = S3C64XX_ULCON_DEFAULT,
		.ufcon	     = S3C64XX_UFCON_DEFAULT,
	},
	[2] = {
		.hwport	     = 2,
		.flags	     = 0,
		.ucon	     = S3C64XX_UCON_DEFAULT,
		.ulcon	     = S3C64XX_ULCON_DEFAULT,
		.ufcon	     = S3C64XX_UFCON_DEFAULT,
	},
};

#ifdef CONFIG_FB_S3C_S6D04D1
struct platform_device sec_device_backlight = {
	.name   = "s6d04d1-backlight",
	.id     = -1,
};
#endif

#if (CONFIG_BOARD_REVISION == CONFIG_APOLLO_EMUL)
#define GPIO_DRV_STRENGTH
#endif
#if defined(CONFIG_FB_S3C_LMS300)||defined(CONFIG_FB_S3C_S6D04D1)
static void display_cfg_gpio(struct platform_device *pdev)
{
	/* LDI port Init */

#ifdef USE_GPIO_ENABLE_FOR_LCD_POWER
	s3c_gpio_cfgpin(GPIO_MLCD_ON, S3C_GPIO_SFN(GPIO_MLCD_ON_STATE)); //MLCD_ON
    gpio_set_value(GPIO_MLCD_ON, GPIO_LEVEL_HIGH);
	s3c_gpio_setpull(GPIO_MLCD_ON, S3C_GPIO_PULL_NONE);	
#endif

	gpio_set_value(GPIO_MLCD_RST, GPIO_LEVEL_HIGH);		//sm.kim 20091210 set high for preventing lcd off on booting
	s3c_gpio_cfgpin(GPIO_MLCD_RST, S3C_GPIO_SFN(GPIO_MLCD_RST_STATE));	/* LCD RESET */
	s3c_gpio_setpull(GPIO_MLCD_RST, S3C_GPIO_PULL_NONE);
#ifdef GPIO_DRV_STRENGTH
	s3c_gpio_set_drv_str(GPIO_MLCD_RST, GPIO_DRV_SR_4X);	
#endif

#if defined(CONFIG_FB_S3C_LMS300)
	s3c_gpio_cfgpin(GPIO_ALS_nRST, S3C_GPIO_SFN(GPIO_ALS_nRST_STATE));      /* BL RESET */
    gpio_set_value(GPIO_ALS_nRST, GPIO_LEVEL_HIGH);
    s3c_gpio_setpull(GPIO_ALS_nRST, S3C_GPIO_PULL_NONE);
#ifdef GPIO_DRV_STRENGTH
	s3c_gpio_set_drv_str(GPIO_ALS_nRST, GPIO_DRV_SR_4X);	
#endif
#endif

	s3c_gpio_cfgpin(GPIO_LCD_ID, S3C_GPIO_SFN(GPIO_LCD_ID_STATE));      /* LCD ID */
    s3c_gpio_setpull(GPIO_LCD_ID, S3C_GPIO_PULL_NONE);
        
	s3c_gpio_cfgpin(GPIO_DISPLAY_CS, S3C_GPIO_SFN(GPIO_DISPLAY_CS_STATE));	/* SPI CS */
	gpio_set_value(GPIO_DISPLAY_CS, GPIO_LEVEL_HIGH);
	s3c_gpio_setpull(GPIO_DISPLAY_CS, S3C_GPIO_PULL_NONE);

#ifdef GPIO_DRV_STRENGTH
	s3c_gpio_set_drv_str(GPIO_DISPLAY_CS, GPIO_DRV_SR_4X);        
#else
	s3c_gpio_set_drv_str(GPIO_DISPLAY_CS, GPIO_DRV_SR_2X);        
#endif

	s3c_gpio_cfgpin(GPIO_DISPLAY_CLK, S3C_GPIO_SFN(GPIO_DISPLAY_CLK_STATE));	/* SPI Clock */
	gpio_set_value(GPIO_DISPLAY_CLK, GPIO_LEVEL_HIGH);
	s3c_gpio_setpull(GPIO_DISPLAY_CLK, S3C_GPIO_PULL_NONE);


	s3c_gpio_cfgpin(GPIO_DISPLAY_SI, S3C_GPIO_SFN(GPIO_DISPLAY_SI_STATE));	/* SPI Data */
	gpio_set_value(GPIO_DISPLAY_SI, GPIO_LEVEL_HIGH);
	s3c_gpio_setpull(GPIO_DISPLAY_SI, S3C_GPIO_PULL_NONE);
#ifdef GPIO_DRV_STRENGTH
	s3c_gpio_set_drv_str(GPIO_DISPLAY_SI, GPIO_DRV_SR_4X);       
#else
	s3c_gpio_set_drv_str(GPIO_DISPLAY_SI, GPIO_DRV_SR_2X);       
#endif


#if (CONFIG_BOARD_REVISION == CONFIG_APOLLO_EMUL)
	 writel(0x10, S5P_MDNIE_SEL);		//only for EVT0(6442)
#endif

     udelay(200);


	 s3c_gpio_cfgpin(GPIO_LCD_HSYNC, S3C_GPIO_SFN(GPIO_LCD_HSYNC_STATE));
	 s3c_gpio_setpull(GPIO_LCD_HSYNC, S3C_GPIO_PULL_NONE);

	 s3c_gpio_cfgpin(GPIO_LCD_VSYNC, S3C_GPIO_SFN(GPIO_LCD_VSYNC_STATE));
	 s3c_gpio_setpull(GPIO_LCD_VSYNC, S3C_GPIO_PULL_NONE);

	 s3c_gpio_cfgpin(GPIO_LCD_VDEN, S3C_GPIO_SFN(GPIO_LCD_VDEN_STATE));
	 s3c_gpio_setpull(GPIO_LCD_VDEN, S3C_GPIO_PULL_NONE);

	 s3c_gpio_cfgpin(GPIO_LCD_MCLK, S3C_GPIO_SFN(GPIO_LCD_MCLK_STATE));
	 s3c_gpio_setpull(GPIO_LCD_MCLK, S3C_GPIO_PULL_NONE);

	 s3c_gpio_cfgpin(GPIO_LCD_D_0, S3C_GPIO_SFN(GPIO_LCD_D_0_STATE));
	 s3c_gpio_setpull(GPIO_LCD_D_0, S3C_GPIO_PULL_NONE);

	 s3c_gpio_cfgpin(GPIO_LCD_D_1, S3C_GPIO_SFN(GPIO_LCD_D_1_STATE));
	 s3c_gpio_setpull(GPIO_LCD_D_1, S3C_GPIO_PULL_NONE);

	 s3c_gpio_cfgpin(GPIO_LCD_D_2, S3C_GPIO_SFN(GPIO_LCD_D_2_STATE));
	 s3c_gpio_setpull(GPIO_LCD_D_2, S3C_GPIO_PULL_NONE);

	 s3c_gpio_cfgpin(GPIO_LCD_D_3, S3C_GPIO_SFN(GPIO_LCD_D_3_STATE));
	 s3c_gpio_setpull(GPIO_LCD_D_3, S3C_GPIO_PULL_NONE);

	 s3c_gpio_cfgpin(GPIO_LCD_D_4, S3C_GPIO_SFN(GPIO_LCD_D_4_STATE));
	 s3c_gpio_setpull(GPIO_LCD_D_4, S3C_GPIO_PULL_NONE);

	 s3c_gpio_cfgpin(GPIO_LCD_D_5, S3C_GPIO_SFN(GPIO_LCD_D_5_STATE));
	 s3c_gpio_setpull(GPIO_LCD_D_5, S3C_GPIO_PULL_NONE);

	 s3c_gpio_cfgpin(GPIO_LCD_D_6, S3C_GPIO_SFN(GPIO_LCD_D_6_STATE));
	 s3c_gpio_setpull(GPIO_LCD_D_6, S3C_GPIO_PULL_NONE);

	 s3c_gpio_cfgpin(GPIO_LCD_D_7, S3C_GPIO_SFN(GPIO_LCD_D_7_STATE));
	 s3c_gpio_setpull(GPIO_LCD_D_7, S3C_GPIO_PULL_NONE);

	 s3c_gpio_cfgpin(GPIO_LCD_D_8, S3C_GPIO_SFN(GPIO_LCD_D_8_STATE));
	 s3c_gpio_setpull(GPIO_LCD_D_8, S3C_GPIO_PULL_NONE);

	 s3c_gpio_cfgpin(GPIO_LCD_D_9, S3C_GPIO_SFN(GPIO_LCD_D_9_STATE));
	 s3c_gpio_setpull(GPIO_LCD_D_9, S3C_GPIO_PULL_NONE);

	 s3c_gpio_cfgpin(GPIO_LCD_D_10, S3C_GPIO_SFN(GPIO_LCD_D_10_STATE));
	 s3c_gpio_setpull(GPIO_LCD_D_10, S3C_GPIO_PULL_NONE);

	 s3c_gpio_cfgpin(GPIO_LCD_D_11, S3C_GPIO_SFN(GPIO_LCD_D_11_STATE));
	 s3c_gpio_setpull(GPIO_LCD_D_11, S3C_GPIO_PULL_NONE);

	 s3c_gpio_cfgpin(GPIO_LCD_D_12, S3C_GPIO_SFN(GPIO_LCD_D_12_STATE));
	 s3c_gpio_setpull(GPIO_LCD_D_12, S3C_GPIO_PULL_NONE);

	 s3c_gpio_cfgpin(GPIO_LCD_D_13, S3C_GPIO_SFN(GPIO_LCD_D_13_STATE));
	 s3c_gpio_setpull(GPIO_LCD_D_13, S3C_GPIO_PULL_NONE);

	 s3c_gpio_cfgpin(GPIO_LCD_D_14, S3C_GPIO_SFN(GPIO_LCD_D_14_STATE));
	 s3c_gpio_setpull(GPIO_LCD_D_14, S3C_GPIO_PULL_NONE);

	 s3c_gpio_cfgpin(GPIO_LCD_D_15, S3C_GPIO_SFN(GPIO_LCD_D_15_STATE));
	 s3c_gpio_setpull(GPIO_LCD_D_15, S3C_GPIO_PULL_NONE);

	 s3c_gpio_cfgpin(GPIO_LCD_D_16, S3C_GPIO_SFN(GPIO_LCD_D_16_STATE));
	 s3c_gpio_setpull(GPIO_LCD_D_16, S3C_GPIO_PULL_NONE);

	 s3c_gpio_cfgpin(GPIO_LCD_D_17, S3C_GPIO_SFN(GPIO_LCD_D_17_STATE));
	 s3c_gpio_setpull(GPIO_LCD_D_17, S3C_GPIO_PULL_NONE);

	 s3c_gpio_cfgpin(GPIO_LCD_D_18, S3C_GPIO_SFN(GPIO_LCD_D_18_STATE));
	 s3c_gpio_setpull(GPIO_LCD_D_18, S3C_GPIO_PULL_NONE);

	 s3c_gpio_cfgpin(GPIO_LCD_D_19, S3C_GPIO_SFN(GPIO_LCD_D_19_STATE));
	 s3c_gpio_setpull(GPIO_LCD_D_19, S3C_GPIO_PULL_NONE);

	 s3c_gpio_cfgpin(GPIO_LCD_D_20, S3C_GPIO_SFN(GPIO_LCD_D_20_STATE));
	 s3c_gpio_setpull(GPIO_LCD_D_20, S3C_GPIO_PULL_NONE);

	 s3c_gpio_cfgpin(GPIO_LCD_D_21, S3C_GPIO_SFN(GPIO_LCD_D_21_STATE));
	 s3c_gpio_setpull(GPIO_LCD_D_21, S3C_GPIO_PULL_NONE);

	 s3c_gpio_cfgpin(GPIO_LCD_D_22, S3C_GPIO_SFN(GPIO_LCD_D_22_STATE));
	 s3c_gpio_setpull(GPIO_LCD_D_22, S3C_GPIO_PULL_NONE);

	 s3c_gpio_cfgpin(GPIO_LCD_D_23, S3C_GPIO_SFN(GPIO_LCD_D_23_STATE));
	 s3c_gpio_setpull(GPIO_LCD_D_23, S3C_GPIO_PULL_NONE);
	 

#ifdef GPIO_DRV_STRENGTH
	s3c_gpio_set_drv_str(GPIO_LCD_HSYNC, GPIO_DRV_SR_4X);
	s3c_gpio_set_drv_str(GPIO_LCD_VSYNC, GPIO_DRV_SR_4X);
	s3c_gpio_set_drv_str(GPIO_LCD_VDEN, GPIO_DRV_SR_4X);
	s3c_gpio_set_drv_str(GPIO_LCD_MCLK, GPIO_DRV_SR_4X);

	s3c_gpio_set_drv_str(GPIO_LCD_D_0, GPIO_DRV_SR_4X);
	s3c_gpio_set_drv_str(GPIO_LCD_D_1, GPIO_DRV_SR_4X);
	s3c_gpio_set_drv_str(GPIO_LCD_D_2, GPIO_DRV_SR_4X);
	s3c_gpio_set_drv_str(GPIO_LCD_D_3, GPIO_DRV_SR_4X);
	s3c_gpio_set_drv_str(GPIO_LCD_D_4, GPIO_DRV_SR_4X);
	s3c_gpio_set_drv_str(GPIO_LCD_D_5, GPIO_DRV_SR_4X);
	s3c_gpio_set_drv_str(GPIO_LCD_D_6, GPIO_DRV_SR_4X);
	s3c_gpio_set_drv_str(GPIO_LCD_D_7, GPIO_DRV_SR_4X);
	s3c_gpio_set_drv_str(GPIO_LCD_D_8, GPIO_DRV_SR_4X);
	s3c_gpio_set_drv_str(GPIO_LCD_D_9, GPIO_DRV_SR_4X);
	s3c_gpio_set_drv_str(GPIO_LCD_D_10, GPIO_DRV_SR_4X);
	s3c_gpio_set_drv_str(GPIO_LCD_D_11, GPIO_DRV_SR_4X);
	s3c_gpio_set_drv_str(GPIO_LCD_D_12, GPIO_DRV_SR_4X);
	s3c_gpio_set_drv_str(GPIO_LCD_D_13, GPIO_DRV_SR_4X);
	s3c_gpio_set_drv_str(GPIO_LCD_D_14, GPIO_DRV_SR_4X);
	s3c_gpio_set_drv_str(GPIO_LCD_D_15, GPIO_DRV_SR_4X);
	s3c_gpio_set_drv_str(GPIO_LCD_D_16, GPIO_DRV_SR_4X);
	s3c_gpio_set_drv_str(GPIO_LCD_D_17, GPIO_DRV_SR_4X);
	s3c_gpio_set_drv_str(GPIO_LCD_D_18, GPIO_DRV_SR_4X);
	s3c_gpio_set_drv_str(GPIO_LCD_D_19, GPIO_DRV_SR_4X);
	s3c_gpio_set_drv_str(GPIO_LCD_D_20, GPIO_DRV_SR_4X);
	s3c_gpio_set_drv_str(GPIO_LCD_D_21, GPIO_DRV_SR_4X);
	s3c_gpio_set_drv_str(GPIO_LCD_D_22, GPIO_DRV_SR_4X);
	s3c_gpio_set_drv_str(GPIO_LCD_D_23, GPIO_DRV_SR_4X);
#else
	s3c_gpio_set_drv_str(GPIO_LCD_HSYNC, GPIO_DRV_SR_2X);
	s3c_gpio_set_drv_str(GPIO_LCD_VSYNC, GPIO_DRV_SR_2X);
	s3c_gpio_set_drv_str(GPIO_LCD_VDEN, GPIO_DRV_SR_2X);
	s3c_gpio_set_drv_str(GPIO_LCD_MCLK, GPIO_DRV_SR_2X);
#endif

}

static struct s3c_platform_fb s6d04d1_data __initdata = {
	.hw_ver	= 0x62,
	.clk_name = "sclk_lcd",//"lcd",
	.nr_wins = 5,
	.default_win = CONFIG_FB_S3C_DEFAULT_WINDOW,
	.swap = FB_SWAP_HWORD | FB_SWAP_WORD,

	.cfg_gpio = display_cfg_gpio,
#ifdef CONFIG_FB_S3C_LCD_INIT
	.backlight_on = s6d04d1_backlight_on,
	.reset_lcd = NULL,
#else
	//SEC: sm.kim 20091210 s3cfb_probe doesn't need to initialize LCD
	.backlight_on = NULL,
	.reset_lcd = NULL,
#endif
};

#endif

static int apollo_saved_lcd_id_value=1;

static void check_lcd_id(void)
{
	int data;
	data = gpio_get_value(GPIO_LCD_ID);
	printk("%s lcd_id=%d\n", __func__, data);
	apollo_saved_lcd_id_value = data;
}

int apollo_get_saved_lcd_id_value(void)
{
	return apollo_saved_lcd_id_value;
}

EXPORT_SYMBOL(apollo_get_saved_lcd_id_value);

static int apollo_hw_rev_pin_value=-1;

static void check_hw_rev_pin(void)
{
	int data;
	apollo_hw_rev_pin_value = 0;

	// set gpio configuration
	s3c_gpio_cfgpin(GPIO_HW_REV_MODE2, GPIO_HW_REV_MODE2_STATE);
	s3c_gpio_cfgpin(GPIO_HW_REV_MODE1, GPIO_HW_REV_MODE1_STATE);
	s3c_gpio_cfgpin(GPIO_HW_REV_MODE0, GPIO_HW_REV_MODE0_STATE);

	data = gpio_get_value(GPIO_HW_REV_MODE2);
	if(data)
		apollo_hw_rev_pin_value |= (0x1<<2);
	data = gpio_get_value(GPIO_HW_REV_MODE1);
	if(data)
		apollo_hw_rev_pin_value |= (0x1<<1);
	data = gpio_get_value(GPIO_HW_REV_MODE0);
	if(data)
		apollo_hw_rev_pin_value |= (0x1<<0);
		
	printk("%s : hw_rev_pin=0x%x\n", __func__, apollo_hw_rev_pin_value);
}

int apollo_get_hw_rev_pin_value(void)
{
	return apollo_hw_rev_pin_value;
}

EXPORT_SYMBOL(apollo_get_hw_rev_pin_value);

int apollo_get_remapped_hw_rev_pin(void)
{
	int revision;
	
	switch(apollo_hw_rev_pin_value)
		{
//		case 0:		// apollo Emul or rev0.4 (memory 4-1-1)
//			revision = 0;
//			break;
//		case 1:		// apollo rev0.2 or rev0.3
//			revision = 1;
//			break;
		case 2:		// apollo rev0.4 (memory 4-2-1)
			revision = 2;
			break;
		case 6:		// apollo rev0.5
			revision = 3;
			break;
		case 4:		// apollo rev0.6
			revision = 4;
			break;
		case 5:		// apollo rev0.7
			revision = 5;
			break;
		case 7:		// apollo rev0.9b
			revision = 6;
			break;
		case 3:		// apollo OPEN rev0.7
			revision = 5;
			break;
		case 1:		// apollo OPEN rev0.9b
			revision = 6;
			break;
		case 0:		// apollo OPEN rev1.0a
			revision = 7;
			break;
		default:
			revision = 8;
			break;
		}

	return revision;
}

EXPORT_SYMBOL(apollo_get_remapped_hw_rev_pin);

int apollo_get_hw_type()
{
	int type;
	
	switch(apollo_hw_rev_pin_value)
		{
		//case 0:		// apollo Emul or rev0.4 (memory 4-1-1)
		//case 1:		// apollo rev0.2 or rev0.3
		case 2:		// apollo rev0.4 (memory 4-2-1)
		case 6:		// apollo rev0.5
		case 4:		// apollo rev0.6
		case 5:		// apollo rev0.7
		case 7:		// apollo rev0.9b
			type = 0;
			break;
		case 3:		// apollo OPEN rev0.7
		case 1:		// apollo OPEN rev0.9b
		case 0:		// apollo OPEN rev1.0a
			type = 1;
			break;
		default:
			type = 0;
			break;
		}

	return type;
}
EXPORT_SYMBOL(apollo_get_hw_type);
static struct i2c_gpio_platform_data i2c3_platdata = {
        .sda_pin                = GPIO_AP_SDA,
        .scl_pin                = GPIO_AP_SCL,
        .udelay                 = 2,    /* 250KHz */
        .sda_is_open_drain      = 0,
        .scl_is_open_drain      = 0,
        .scl_is_output_only     = 1,
};

static struct platform_device s3c_device_i2c3 = {
        .name                           = "i2c-gpio",
        .id                                     = 3,
        .dev.platform_data      = &i2c3_platdata,
};


static struct i2c_gpio_platform_data i2c4_platdata = {
        .sda_pin                = GPIO_AP_PMIC_SDA,
        .scl_pin                = GPIO_AP_PMIC_SCL,
        .udelay                 = 2,    /* 250KHz */
        .sda_is_open_drain      = 0,
        .scl_is_open_drain      = 0,
        .scl_is_output_only     = 1,
};

static struct platform_device s3c_device_i2c4 = {
        .name                           = "i2c-gpio",
        .id                                     = 4,
        .dev.platform_data      = &i2c4_platdata,
};

/*
static  struct  i2c_gpio_platform_data  i2c5_platdata = {
        .sda_pin                = GPIO_AP_PMIC_SDA,
        .scl_pin                = GPIO_AP_PMIC_SCL,
        .udelay                 = 2,    // 250KHz 
        .sda_is_open_drain      = 0,
        .scl_is_open_drain      = 0,
        .scl_is_output_only     = 0,
};

static struct platform_device s3c_device_i2c5 = {
        .name                           = "i2c-gpio",
        .id                                     = 5,
        .dev.platform_data      = &i2c5_platdata,
};
*/

//static struct i2c_gpio_platform_data i2c5_platdata = {
//        .sda_pin                = GPIO_CAM_SDA_2_8V,
//        .scl_pin                = GPIO_CAM_SCL_2_8V,
//        .udelay                 = 2,    /* 250KHz */
//        .sda_is_open_drain      = 0,
//        .scl_is_open_drain      = 0,
//        .scl_is_output_only     = 1,
//};

//static struct platform_device s3c_device_i2c5 = {
//        .name                           = "i2c-gpio",
//        .id                                     = 5,
//        .dev.platform_data      = &i2c5_platdata,
//};

//static struct i2c_gpio_platform_data i2c6_platdata = {
//        .sda_pin                = GPIO_TSP_SDA_2_8V,
//        .scl_pin                = GPIO_TSP_SCL_2_8V,
//        .udelay                 = 1,    /* change I2C speed to avoid error in TSP  */
//        .sda_is_open_drain      = 0,
//        .scl_is_open_drain      = 0,
//        .scl_is_output_only     = 0,
//};

//static struct platform_device s3c_device_i2c6 = {
//        .name                           = "i2c-gpio",
//        .id                                     = 6,
//        .dev.platform_data      = &i2c6_platdata,
//};

/*VNVS:START 16-NOV'09 ---- added i2c7_platdata for FM Radio I2C paths*/
static struct i2c_gpio_platform_data i2c7_platdata = {
        .sda_pin                = GPIO_FM_SDA_2_8V,
        .scl_pin                = GPIO_FM_SCL_2_8V,
        .udelay                 = 2,           /*250KHz*/
	.sda_is_open_drain      = 0,
        .scl_is_open_drain      = 0,
        .scl_is_output_only     = 0,
};

static struct platform_device s3c_device_i2c7 = {
        .name                           = "i2c-gpio",
        .id                                     = 7,
        .dev.platform_data      = &i2c7_platdata,
};
/*VNVS:END*/
/*VNVS:START 16-NOV'09 ---- added i2c8_platdata for Accelerometer,Magnetic Sensors I2C paths*/
static struct i2c_gpio_platform_data i2c8_platdata = {
        .sda_pin                = GPIO_AP_SDA_2_8V,
        .scl_pin                = GPIO_AP_SCL_2_8V,
        .udelay                 = 2,           /*250KHz*/
	.sda_is_open_drain      = 0,
        .scl_is_open_drain      = 0,
        .scl_is_output_only     = 0,
};

static struct platform_device s3c_device_i2c8 = {
        .name                           = "i2c-gpio",
        .id                                     = 8,
        .dev.platform_data      = &i2c8_platdata,
};
/*VNVS:END*/

/*VNVS:START 25-MAR'10 ---- added i2c9_platdata for Proximity Sensor I2C path*/
static struct i2c_gpio_platform_data i2c9_platdata = {
        .sda_pin                = GPIO_SENSOR_SDA,
        .scl_pin                = GPIO_SENSOR_SCL,
        .udelay                 = 2,           /*250KHz*/
		.sda_is_open_drain      = 0,
        .scl_is_open_drain      = 0,
        .scl_is_output_only     = 0,
};

static struct platform_device s3c_device_i2c9 = {
        .name                	= "i2c-gpio",
        .id                   	= 9,
        .dev.platform_data      = &i2c9_platdata,
};
/*VNVS:END*/

struct platform_device sec_device_ts = {
        .name   = "qt602240-ts",
        .id             = -1,
};

struct platform_device sec_device_rfkill = {
        .name   = "bcm4329_rfkill",
        .id             = -1,
};

struct platform_device sec_device_btsleep = {
        .name   = "bt_sleep",
        .id             = -1,
};

struct platform_device sec_device_proximity = {
        .name   = "proximity_cm3607",
        .id     = -1,
};

//#define GPIO_PWR_I2C_SDA        S5P64XX_GPD1(4)
//#define GPIO_PWR_I2C_SCL        S5P64XX_GPD1(5)

/*
struct platform_device sec_device_dpram = {
	.name	= "dpram-device",
	.id		= -1,
};
*/

#ifdef CONFIG_RTC_DRV_VIRTUAL
static struct platform_device samsung_virtual_rtc_device = {
    .name           = "virtual_rtc",
    .id             = -1,
};
#endif

#ifdef CONFIG_SEC_HEADSET
static struct sec_headset_port sec_headset_port[] = {
        {
		{ // HEADSET detect info
			.eint		=IRQ_EINT6, 
			.gpio		= GPIO_EARJACK_DET,   
			.gpio_af	= GPIO_EARJACK_DET_STATE  , 
			.low_active 	= 0
		},
		{ // SEND/END info
			.eint		= IRQ_EINT(18),
			.gpio		= GPIO_EAR_SEND_END, 
			.gpio_af	= GPIO_EAR_SEND_END_STATE, 
			.low_active	= 1
		}			
        }
};

static struct sec_headset_platform_data sec_headset_data = {
        .port           = sec_headset_port,
        .nheadsets      = ARRAY_SIZE(sec_headset_port),
};

static struct platform_device sec_device_headset = {
        .name           = "sec_headset",
        .id             = -1,
        .dev            = {
                .platform_data  = &sec_headset_data,
        },
};
#endif // CONFIG_SEC_HEADSET

/* S5P-RP - RP on Audio Sub-System */
#ifdef CONFIG_SND_S5P_RP
static struct resource s5p_rp_resource[] = {
};

static u64 s5p_device_rp_dmamask = 0xffffffffUL;
struct platform_device s5p_device_rp = {
	.name             = "s5p-rp",
	.id               = -1,
	.num_resources    = ARRAY_SIZE(s5p_rp_resource),
	.resource         = s5p_rp_resource,
	.dev		  = {
		.dma_mask = &s5p_device_rp_dmamask,
		.coherent_dma_mask = 0xffffffffUL
	}
};
#endif


struct map_desc smdk6442_iodesc[] = {};

#ifdef CONFIG_S5P64XX_ADCTS
static struct s3c_adcts_plat_info s3c_adcts_cfgs __initdata = {
	.channel = {
		{ /* 0 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		},{ /* 1 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		},{ /* 2 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		},{ /* 3 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		},{ /* 4 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		},{ /* 5 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		},{ /* 6 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		},{ /* 7 */
			.delay = 0xFF,
			.presc = 49,
			.resol = S3C_ADCCON_RESSEL_12BIT,
		},
	},
};
#endif

static struct platform_device *smdk6442_devices[] __initdata = {
//	&s3c_device_android_usb,
//	&s3c_device_usb_mass_storage,

#ifdef CONFIG_MTD_ONENAND
        &s3c_device_onenand,
#endif

	&s3c_device_keypad,
	&s3c_device_fb,

	&s3c_device_wdt,
	&s3c_device_rtc,
	&s3c_device_i2c0,
//	&s3c_device_i2c1,
	&s3c_device_i2c2,
#if 1	
	&s3c_device_i2c3,
	&s3c_device_i2c4,
//	&s3c_device_i2c5, /* for fsa9480 */
//	&s3c_device_i2c6, 
	&s3c_device_i2c7,
	&s3c_device_i2c8,
	&s3c_device_i2c9,
#endif

//	&s3c_device_lcd,
//	&s3c_device_nand,
//	&s3c_device_hsmmc0,
	&s3c_device_hsmmc1,		// SDIO for WLAN
	&s3c_device_hsmmc2,
	&s3c_device_fimc0,
	&s3c_device_fimc1,
	&s3c_device_fimc2,
	&s3c_device_jpeg,

#ifdef CONFIG_S5P6442_MFC
        &s3c_device_mfc,
#endif

#ifdef CONFIG_S5P64XX_ADCTS
	&s3c_device_adcts,
#endif

#ifdef CONFIG_HAVE_PWM
	&s3c_device_timer[0],
	&s3c_device_timer[1],
#endif

#ifdef CONFIG_FB_S3C_LMS300
 //       &s3c_device_spi_gpio,
#endif
#ifdef CONFIG_FB_S3C_S6D04D1
	&sec_device_backlight, 
#endif
	&sec_device_battery,
#ifdef CONFIG_S5P64XX_TS_C_TYPE
	&sec_device_ts,
#endif
#ifdef CONFIG_S5P64XX_TS_R_TYPE
	&s3c_device_ts,
#endif
#ifdef CONFIG_SEC_HEADSET
	&sec_device_headset,	
#endif

#if defined CONFIG_USB_GADGET_S3C_OTGD_HS 
	&s3c_device_usbgadget,
#endif

#if defined CONFIG_VIDEO_G3D
	&s3c_device_g3d,
#endif

#if defined CONFIG_VIDEO_G2D
	&s3c_device_g2d,
#endif

#ifdef CONFIG_SND_S5P_RP
	&s5p_device_rp,
#endif
//	&sec_device_dpram,
	&sec_device_rfkill,
	&sec_device_btsleep,
	&sec_device_proximity,

#ifdef CONFIG_RTC_DRV_VIRTUAL
	&samsung_virtual_rtc_device,
#endif

};

/*
static struct i2c_board_info i2c_devs0[] __initdata = {
	{ I2C_BOARD_INFO("24c08", 0x50), },
	//{ I2C_BOARD_INFO("WM8580", 0x10), },
};
*/

static struct i2c_board_info i2c_devs1[] __initdata = {
	{ I2C_BOARD_INFO("24c128", 0x57), },	/* Samsung S524AD0XD1 */
	{ I2C_BOARD_INFO("WM8580", 0x1a), },
	//{ I2C_BOARD_INFO("WM8580", 0x1b), },
};

#define IRQ_TOUCH_INT IRQ_EINT_GROUP(13,2) 

static struct i2c_board_info i2c_devs2[] __initdata = {
    {
        I2C_BOARD_INFO("qt602240_ts", 0x4a),
        .irq = IRQ_TOUCH_INT,
    },
};

#define IRQ_FSA9480_INTB	IRQ_EINT(23)

static struct i2c_board_info i2c_devs4[] __initdata = {
	{ 
		I2C_BOARD_INFO("Max 8998 I2C", (0x0c>>1)), 
	},
	{
		I2C_BOARD_INFO("MAX8998", (0xcc >> 1)),
		.irq = IRQ_PMIC_STATUS_INT,
	},
	{
		I2C_BOARD_INFO("fsa9480", (0x4A >> 1)),
		.irq = IRQ_FSA9480_INTB,
	},
};

/*
#define IRQ_FSA9480_INTB	IRQ_EINT(23)

static struct i2c_board_info i2c_devs5[] __initdata = {
	{
		I2C_BOARD_INFO("fsa9480", (0x4A >> 1)),
		.irq = IRQ_FSA9480_INTB,
	},
};
*/

static struct i2c_board_info i2c_devs7[] __initdata = {
	{
		I2C_BOARD_INFO("Si4709", (0x20 >> 1)),
	},
};

#define IRQ_COMPASS_INT IRQ_EINT(2) /* EINT(2) */

static struct i2c_board_info i2c_devs8[] __initdata = {
    {
        I2C_BOARD_INFO("ak8973", 0x1c),
        .irq = IRQ_COMPASS_INT,         
   },
   {
        I2C_BOARD_INFO("bma020", 0x38),
    },
};

//#define PROX_IRQ				IRQ_EINT(25)		
static struct i2c_board_info i2c_devs9[] __initdata = {
    {
        I2C_BOARD_INFO("gp2a_prox", (0x88 >> 1)),
//        .irq = PROX_IRQ,
    },
};

//techwin camera device
static struct s3c2410_platform_i2c camera_i2c_data __initdata = {
	.flags		= 0,
	.bus_num	= 0,
	.slave_addr	= (0x78 >> 1),
	.frequency	= 400*1000,
	.sda_delay	= S3C2410_IICLC_SDA_DELAY5 | S3C2410_IICLC_FILTER_ON,
};

//quantum touch device
static struct s3c2410_platform_i2c touch_i2c_data __initdata = {
	.flags		= 0,
	.bus_num	= 2,
	.slave_addr	= 0x4A,
	.frequency	= 400*1000,
	.sda_delay	= S3C2410_IICLC_SDA_DELAY5 | S3C2410_IICLC_FILTER_ON,
};

#if !defined CONFIG_S5P64XX_ADCTS
static struct s3c_adc_mach_info s3c_adc_platform = {
	/* s3c6442 support 12-bit resolution */
	.delay	= 	10000,
	.presc 	= 	49,
	.resolution = 	12,
};
#endif

static int ldo38_onoff_state = 0;
#define LDO38_USB_BIT    (0x1)
#define LDO38_TVOUT_BIT  (0x2)
static DEFINE_MUTEX(ldo38_mutex);

static int ldo38_control(int module, bool onoff)
{
    mutex_lock(&ldo38_mutex);
    if (onoff)
    {
        if (!ldo38_onoff_state)
        {
            Set_MAX8998_PM_REG(ELDO3, 1);
            msleep(1);
            Set_MAX8998_PM_REG(ELDO8, 1);
            msleep(1);
        }
        printk(KERN_INFO "%s : turn ON LDO3 and LDO8 (cur_stat=%d, req=%d)\n",__func__,ldo38_onoff_state,module);
        ldo38_onoff_state |= module;
    }
    else
    {
        printk(KERN_INFO "%s : turn OFF LDO3 and LDO8 (cur_stat=%d, req=%d)\n",__func__,ldo38_onoff_state,module);
        ldo38_onoff_state &= ~module;
        if (!ldo38_onoff_state)
        {
            Set_MAX8998_PM_REG(ELDO8, 0);
            msleep(1);
            Set_MAX8998_PM_REG(ELDO3, 0);
        }
    }
    mutex_unlock(&ldo38_mutex);	
	return 0;
}

int ldo38_control_by_usb(bool onoff)
{
    ldo38_control(LDO38_USB_BIT, onoff);
}

/*
 * Pin configuration for CAM_EN, CAM_RST
 */
static void apollo_cam_pin_config(void)
{
	u32 cfg;

	/* XMMC0ClK(GPG0[0]) is used as CAM_EN */
	cfg = readl(S5P64XX_GPG0CON);
	cfg &= ~(S5P64XX_GPG0_CONMASK(0));
	cfg |= S5P64XX_GPG0_OUTPUT(0);
	writel(cfg, S5P64XX_GPG0CON);
	
	/* XMSMADDR_13(GPJ1[5] is used as CAM_RST */
	cfg = readl(S5P64XX_GPJ1CON);
	cfg &= ~(S5P64XX_GPJ1_CONMASK(5));
	cfg |= S5P64XX_GPJ1_OUTPUT(5);
	writel(cfg, S5P64XX_GPJ1CON);

	/* XMMC0DATA_0(GPG0[3] is uesd as CAM_STBY */
	cfg = readl(S5P64XX_GPG0CON);
	cfg &= ~(S5P64XX_GPG0_CONMASK(3));
	cfg |= S5P64XX_GPG0_OUTPUT(3);
	writel(cfg, S5P64XX_GPG0CON);

	/* To make i2c lines low configuring them as GPIO */
	/* Pins are SCL(GPD1[1]) and SDA(GPD1[0]) */
	cfg = readl(S5P64XX_GPD1CON);
	cfg &= ~(S5P64XX_GPD1_CONMASK(0));
	cfg |= S5P64XX_GPD1_OUTPUT(0);
	writel(cfg, S5P64XX_GPD1CON);

	cfg = readl(S5P64XX_GPD1CON);
	cfg &= ~(S5P64XX_GPD1_CONMASK(1));
	cfg |= S5P64XX_GPD1_OUTPUT(1);
	writel(cfg, S5P64XX_GPD1CON);
}

/*
 * External camera reset
 * Because the most of cameras take i2c bus signal, so that
 * you have to reset at the boot time for other i2c slave devices.
 * This function also called at fimc_init_camera()
 * Do optimization for cameras on your platform.
*/
static int univ6442_cam0_power(int onoff)
{
	/* Camera A */
	u32 cfg;

	if (!onoff) {
		/* Mask the output of mclk */
		cfg = readl(S5P_CLK_SRC_MASK0) & ~(1 << 3);
		writel(cfg, S5P_CLK_SRC_MASK0);
		udelay(10);

		/* CAM_3M_nRST Low */
		cfg = readl(S5P64XX_GPJ1DAT);
		cfg &= ~(0x1 << 5);
		writel(cfg, S5P64XX_GPJ1DAT);
		udelay(10);

		/* CAM_EN Low */
		cfg = readl(S5P64XX_GPG0DAT);
		cfg &= ~(0x1 << 0);
		writel(cfg, S5P64XX_GPG0DAT);
		udelay(10);

		/* make SCL and SDA low */
		cfg = readl(S5P64XX_GPD1DAT);
		cfg &= ~(0x1 << 0);
		writel(cfg, S5P64XX_GPD1DAT);

		cfg = readl(S5P64XX_GPD1DAT);
		cfg &= ~(0x1 << 1);
		writel(cfg, S5P64XX_GPD1DAT);

		/* Make STBY low while powering off */
		cfg = readl(S5P64XX_GPG0DAT);
		cfg &= ~(0x1 << 3);
		writel(cfg, S5P64XX_GPG0DAT);

		writel(0x0, Cam_i2c0_regs);	

		msleep(50);
		printk("s5k4ca cam power off\n");
	} else {
		/* Make STBY high first when powering on */
		cfg = readl(S5P64XX_GPG0DAT);
		cfg |= (0x1 << 3);
		writel(cfg, S5P64XX_GPG0DAT);

		/* make SCL and SDA high */
		cfg = readl(S5P64XX_GPD1DAT);
		cfg |= (0x1 << 0);
		writel(cfg, S5P64XX_GPD1DAT);

		cfg = readl(S5P64XX_GPD1DAT);
		cfg |= (0x1 << 1);
		writel(cfg, S5P64XX_GPD1DAT);

		/* CAM_EN High */
		cfg = readl(S5P64XX_GPG0DAT);
		cfg |= (0x1 << 0);
		writel(cfg, S5P64XX_GPG0DAT);
		udelay(100);
	
		/* Unmask mclk */
		cfg = readl(S5P_CLK_SRC_MASK0) | (1 << 3);
		writel(cfg, S5P_CLK_SRC_MASK0);
		udelay(100);
	
		/* CAM_3M_nRST High */
		cfg = readl(S5P64XX_GPJ1DAT);
		cfg |= (0x1 << 5);
		writel(cfg, S5P64XX_GPJ1DAT);
		msleep(50);
		printk("s5k4ca cam power on\n");
	}

	return 0;
}

int cam_idle_gpio_cfg(int flag)
{	
 	univ6442_cam0_power(flag); 
    return 0;
}

/*
 * Guide for Camera Configuration for SMDKC110
 * ITU channel must be set as A or B
 *
 * NOTE1: if the S5K4EA is enabled, all other cameras must be disabled
 * NOTE2: currently, only 1 MIPI camera must be enabled
 * NOTE3: it is possible to use both one ITU cam and one MIPI cam except for S5K4EA case
 * 
*/
#define CAM_ITU_CH_A
#define S5K4CA_ENABLED

/* External camera module setting */


#ifdef S5K4CA_ENABLED
static struct s5k4ca_platform_data s5k4ca_plat = {
	.default_width = 1024,
	.default_height = 768,
	.pixelformat = V4L2_PIX_FMT_UYVY,  //NV21 support //rama
//	.pixelformat = V4L2_PIX_FMT_VYUY,
	.freq = 24000000,
	.is_mipi = 0,
};

static struct i2c_board_info  __initdata s5k4ca_i2c_info = {
	I2C_BOARD_INFO("S5K4CA", (0x78 >> 1)),
	.platform_data = &s5k4ca_plat,
};

static struct s3c_platform_camera __initdata s5k4ca = {
#ifdef CAM_ITU_CH_A
	.id		= CAMERA_PAR_A,
#else
	.id		= CAMERA_PAR_B,
#endif
	.type		= CAM_TYPE_ITU,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422       = CAM_ORDER422_8BIT_CBYCRY,//NV21 support //rama
//	.order422	= CAM_ORDER422_8BIT_CRYCBY,
	.i2c_busnum	= 0,
	.info		= &s5k4ca_i2c_info,
	.pixelformat    = V4L2_PIX_FMT_UYVY, //NV21 support //rama
//	.pixelformat	= V4L2_PIX_FMT_VYUY,
	.srclk_name	= "mout_epll",
#ifdef CAM_ITU_CH_A
	.clk_name	= "sclk_cam0",
#else
	.clk_name	= "sclk_cam1",
#endif
	.clk_rate	= 24000000,
	.line_length	= 1920,

//giridhar: introducing prevew and camera width and height
	.width_preview	= 1024,
	.height_preview	= 768,
	.window_preview	= {
		.left	= 0,
		.top	= 0,
		.width	= 1024,
		.height	= 768,
	},
	.width_capture	= 2048,
	.height_capture	= 1536,
	.window_capture	= {
		.left	= 0,
		.top	= 0,
		.width	= 2048,
		.height	= 1536,
	},
//giridhar
	.width		= 1024,
	.height		= 768,
	.window		= {
		.left	= 0,
		.top	= 0,
		.width	= 1024,
		.height	= 768,
	},

	/* Polarity */
	.inv_pclk	= 0,
	.inv_vsync 	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,

	.initialized 	= 0,
	.cam_power	= univ6442_cam0_power,

};
#endif


/* Interface setting */
static struct s3c_platform_fimc __initdata fimc_plat = {
#if defined(S5K4EA_ENABLED) || defined(S5K6AA_ENABLED)
	.default_cam	= CAMERA_CSI_C,
#else

#ifdef CAM_ITU_CH_A
	.default_cam	= CAMERA_PAR_A,
#else
	.default_cam	= CAMERA_PAR_B,
#endif

#endif
	.camera		= {
#ifdef S5K3BA_ENABLED
		&s5k3ba,
#endif
#ifdef S5K4BA_ENABLED
		&s5k4ba,
#endif
#ifdef S5K4EA_ENABLED
		&s5k4ea,
#endif
#ifdef S5K6AA_ENABLED
		&s5k6aa,
#endif

#ifdef S5K4CA_ENABLED
		&s5k4ca,
#endif

	}
};


#if defined(CONFIG_HAVE_PWM)
static struct platform_pwm_backlight_data smdk_backlight_data = {
	.pwm_id		= 1,
	.max_brightness	= 255,
	.dft_brightness	= 255,
	.pwm_period_ns	= 78770,
};

static struct platform_device smdk_backlight_device = {
	.name		= "pwm-backlight",
	.dev		= {
		.parent = &s3c_device_timer[1].dev,
		.platform_data = &smdk_backlight_data,
	},
};

static void __init smdk_backlight_register(void)
{
	int ret = platform_device_register(&smdk_backlight_device);
	if (ret)
		printk(KERN_ERR "smdk: failed to register backlight device: %d\n", ret);
}
#else
#define smdk_backlight_register()	do { } while (0)
#endif

#ifdef CONFIG_PHONET
  /**/
/* onedram */
static void onedram_cfg_gpio(void)
{
	//unsigned gpio_onedram_int_ap = S5PC11X_GPH1(3);
	s3c_gpio_cfgpin(GPIO_nINT_ONEDRAM_AP, S3C_GPIO_SFN(GPIO_nINT_ONEDRAM_AP_STATE));
	s3c_gpio_setpull(GPIO_nINT_ONEDRAM_AP, S3C_GPIO_PULL_UP);
	set_irq_type(GPIO_nINT_ONEDRAM_AP, IRQ_TYPE_LEVEL_LOW);
}

static struct onedram_platform_data onedram_data = {
		.cfg_gpio = onedram_cfg_gpio,
		};

static struct resource onedram_res[] = {
	[0] = {
		//.start = (S5P64XX_PA_SDRAM + 0x05000000),
		//.end = (S5P64XX_PA_SDRAM+ 0x05000000 + SZ_16M - 1),
		.start = (0x40000000 + 0x05000000),
		.end = (0x40000000+ 0x05000000 + SZ_16M - 1),
		.flags = IORESOURCE_MEM,
		},
	[1] = {
		.start = IRQ_EINT11,
		.end = IRQ_EINT11,
		.flags = IORESOURCE_IRQ,
		},
	};

static struct platform_device onedram = {
		.name = "onedram",
		.id = -1,
		.num_resources = ARRAY_SIZE(onedram_res),
		.resource = onedram_res,
		.dev = {
			.platform_data = &onedram_data,
			},
		};

/* Modem control */
static void modemctl_cfg_gpio(void);

static struct modemctl_platform_data mdmctl_data = {
	.name = "msm",
	.gpio_phone_on = GPIO_PHONE_ON,
	.gpio_phone_active = GPIO_PHONE_ACTIVE,
	.gpio_pda_active = GPIO_PDA_ACTIVE,
	.gpio_cp_reset = GPIO_CP_RST,
	//.gpio_usim_boot = GPIO_USIM_BOOT,
	.gpio_flm_sel = GPIO_FLM_SEL,
	.gpio_sim_ndetect = GPIO_SIM_nDETECT,		/* Galaxy S does not include SIM detect pin */
	.cfg_gpio = modemctl_cfg_gpio,
};

static struct resource mdmctl_res[] = {
	[0] = {
		.start = IRQ_EINT15,
		.end = IRQ_EINT15,
		.flags = IORESOURCE_IRQ,
		},
	[1] = {
		.start = IRQ_EINT8,
		.end = IRQ_EINT8,
		.flags = IORESOURCE_IRQ,
		},
	};

static struct platform_device modemctl = {
		.name = "modemctl",
		.id = -1,
		.num_resources = ARRAY_SIZE(mdmctl_res),
		.resource = mdmctl_res,
		.dev = {
			.platform_data = &mdmctl_data,
			},
		};

static void modemctl_cfg_gpio(void)
{
	int err = 0;
	
	unsigned gpio_phone_on = mdmctl_data.gpio_phone_on;
	unsigned gpio_phone_active = mdmctl_data.gpio_phone_active;
	unsigned gpio_cp_rst = mdmctl_data.gpio_cp_reset;
	unsigned gpio_pda_active = mdmctl_data.gpio_pda_active;
	unsigned gpio_sim_ndetect = mdmctl_data.gpio_sim_ndetect;
	unsigned gpio_flm_sel = mdmctl_data.gpio_flm_sel;
#if 0
	unsigned gpio_usim_boot = mdmctl_data.gpio_usim_boot;
#endif

	err = gpio_request(gpio_phone_on, "PHONE_ON");
	if (err) {
		printk("fail to request gpio %s\n","PHONE_ON");
	} else {
		gpio_direction_output(gpio_phone_on, GPIO_LEVEL_LOW);
		s3c_gpio_setpull(gpio_phone_on, S3C_GPIO_PULL_NONE);
	}
	err = gpio_request(gpio_cp_rst, "CP_RST");
	if (err) {
		printk("fail to request gpio %s\n","CP_RST");
	} else {
		gpio_direction_output(gpio_cp_rst, GPIO_LEVEL_LOW);
		s3c_gpio_setpull(gpio_cp_rst, S3C_GPIO_PULL_NONE);
	}
	err = gpio_request(gpio_pda_active, "PDA_ACTIVE");
	if (err) {
		printk("fail to request gpio %s\n","PDA_ACTIVE");
	} else {
		gpio_direction_output(gpio_pda_active, GPIO_LEVEL_LOW);
		s3c_gpio_setpull(gpio_pda_active, S3C_GPIO_PULL_NONE);
	}
	err = gpio_request(gpio_flm_sel, "FLM_SEL");
	if (err) {
		printk("fail to request gpio %s\n","FLM_SEL");
	} else {
		gpio_direction_output(gpio_flm_sel, GPIO_LEVEL_LOW);
		s3c_gpio_setpull(gpio_flm_sel, S3C_GPIO_PULL_NONE);
	}
#if 0
	err = gpio_request(gpio_usim_boot, "USIM_BOOT");
	if (err) {
		printk("fail to request gpio %s\n","USIM_BOOT");
	} else {
		gpio_direction_output(gpio_usim_boot, GPIO_LEVEL_LOW);
		s3c_gpio_setpull(gpio_usim_boot, S3C_GPIO_PULL_NONE);
	}
#endif
	s3c_gpio_cfgpin(gpio_phone_active, S3C_GPIO_SFN(0xF));
	s3c_gpio_setpull(gpio_phone_active, S3C_GPIO_PULL_NONE);
	set_irq_type(gpio_phone_active, IRQ_TYPE_EDGE_BOTH);

	s3c_gpio_cfgpin(gpio_sim_ndetect, S3C_GPIO_SFN(0xF));
	s3c_gpio_setpull(gpio_sim_ndetect, S3C_GPIO_PULL_NONE);
	set_irq_type(gpio_sim_ndetect, IRQ_TYPE_EDGE_BOTH);
}
#endif

void smdk6442_setup_sdhci0 (void);

#ifdef CONFIG_KERNEL_PANIC_DUMP
static void __init s5p64xx_allocate_memory_regions(void)
{
	void *addr;
	unsigned long size;

	size = S3C64XX_KERNEL_PANIC_DUMP_SIZE;
	addr = alloc_bootmem(size);
	S3C64XX_KERNEL_PANIC_DUMP_ADDR = addr;

}
#endif

static void __init smdk6442_map_io(void)
{
	s3c_device_nand.name = "s5p6442-nand";

	s5p64xx_init_io(smdk6442_iodesc, ARRAY_SIZE(smdk6442_iodesc));
	s3c24xx_init_clocks(12000000);
	s3c24xx_init_uarts(smdk6442_uartcfgs, ARRAY_SIZE(smdk6442_uartcfgs));

	s3c64xx_reserve_bootmem();
#ifdef CONFIG_MTD_ONENAND
        s3c_device_onenand.name = "s5p6442-onenand";
#endif
#ifdef CONFIG_KERNEL_PANIC_DUMP
	s5p64xx_allocate_memory_regions();
#endif
}

int apollo_wm8994_init(void)
{
 	int err;
	u32 temp;
	temp = __raw_readl(S5P_CLK_OUT);

	// In univ6442 board, CLK_OUT is connected with WM8994 MCLK(main clock)
	// CLK_OUT : bypass external crystall clock
	temp &= 0xFFFE0FFF; // clear bits CLKSEL[16:12]
#ifdef CONFIG_SND_WM8994_MASTER_MODE
	temp |= (0x11 << 12);   // crystall : CLKSEL 10001 = OSSCLK
	printk("\n[WM8994 Master Mode] : using crystall clock(CLK_OUT) to drive WM8994 \n");
#else
	temp |= (0x02 << 12);   // epll : CLKSEL 00010 = FOUTEPLL
	temp |= (0x5 << 20);    // DIVVAL(Divide rateio = DIVVAL + 1 = 6)
#endif

	printk("CLKOUT reg is %x\n",temp);
	 __raw_writel(temp, S5P_CLK_OUT);
	temp = __raw_readl(S5P_CLK_OUT);
	printk("CLKOUT reg is %x\n",temp);


	err = gpio_request( GPIO_CODEC_LDO_EN, "CODEC_LDO_EN");

	if (err) {
		printk(KERN_ERR "failed to request GPIO_CODEC_LDO_EN for "
			"codec control\n");
		return err;
	}
	udelay(50);
	gpio_direction_output( GPIO_CODEC_LDO_EN , 1);
        gpio_set_value(GPIO_CODEC_LDO_EN, 1);
	udelay(50);
        gpio_set_value(GPIO_CODEC_LDO_EN, 0);
	udelay(50);
        gpio_set_value(GPIO_CODEC_LDO_EN, 1);


	//select the headset??
	 err = gpio_request( GPIO_EARJACK_DET, "EAR3.5_SW");

        if (err) {
                printk(KERN_ERR "failed to request GPIO_EARJACK_DET for "
                        "codec control\n");
                return err;
        }
	gpio_direction_output( GPIO_EARJACK_DET , 1);
	gpio_set_value(GPIO_EARJACK_DET, 0);

	return 0;
}


void s3c_setup_uart_cfg_gpio(unsigned char port)
{
#if 1
	if (port == 0) {
		s3c_gpio_cfgpin(GPIO_BT_UART_RXD, S3C_GPIO_SFN(GPIO_BT_UART_RXD_STATE));
		s3c_gpio_setpull(GPIO_BT_UART_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_BT_UART_TXD, S3C_GPIO_SFN(GPIO_BT_UART_TXD_STATE));
		s3c_gpio_setpull(GPIO_BT_UART_TXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_BT_UART_CTS, S3C_GPIO_SFN(GPIO_BT_UART_CTS_STATE));
		s3c_gpio_setpull(GPIO_BT_UART_CTS, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_BT_UART_RTS, S3C_GPIO_SFN(GPIO_BT_UART_RTS_STATE));
		s3c_gpio_setpull(GPIO_BT_UART_RTS, S3C_GPIO_PULL_NONE);
		
	}
	else if (port == 1) {
		s3c_gpio_cfgpin(GPIO_AP_COM_RXD, S3C_GPIO_SFN(GPIO_AP_COM_RXD_STATE));
		s3c_gpio_setpull(GPIO_AP_COM_RXD, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_AP_COM_TXD, S3C_GPIO_SFN(GPIO_AP_COM_TXD_STATE));
		s3c_gpio_setpull(GPIO_AP_COM_TXD, S3C_GPIO_PULL_NONE);
	}
	else if (port == 2) {
		s3c_gpio_cfgpin(GPIO_AP_FLM_RXD_2_8V, S3C_GPIO_SFN(GPIO_AP_FLM_RXD_2_8V_STATE));
		s3c_gpio_setpull(GPIO_AP_FLM_RXD_2_8V, S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(GPIO_AP_FLM_TXD_2_8V, S3C_GPIO_SFN(GPIO_AP_FLM_TXD_2_8V_STATE));
		s3c_gpio_setpull(GPIO_AP_FLM_TXD_2_8V, S3C_GPIO_PULL_NONE);
	}
#endif	
}
EXPORT_SYMBOL(s3c_setup_uart_cfg_gpio);

extern void arch_reset(char mode);
extern int cable_status;

// for checking JIG status
extern int get_usb_cable_state(void);

#define	JIG_USB_ON		(0x1 <<0)
#define	JIG_USB_OFF		(0x1 <<1)
#define	JIG_UART_ON		(0x1 <<2)
#define	JIG_UART_OFF		(0x1 <<3)

static void apollo_pm_power_off(void)
{

	int cnt = 0;

	int mode = REBOOT_MODE_NONE;
	char reset_mode = 'r';

	if (cable_status) 
	{	
		/* Reboot Charging */
		mode = REBOOT_MODE_CHARGING;
		SEC_SET_PARAM(__REBOOT_MODE, mode);

		/* Watchdog Reset */
		arch_reset(reset_mode);
	}
	else 
	{	
		/* Power Off or Reboot */
		SEC_SET_PARAM(__REBOOT_MODE, mode);

		if ((get_usb_cable_state() >> 8) & (JIG_USB_ON | JIG_USB_OFF | JIG_UART_ON | JIG_UART_OFF)) {
			/* Watchdog Reset */
			arch_reset(reset_mode);
		}
		else 
		{
			/* POWER_N -> Input */
			gpio_direction_input(GPIO_nPOWER);
			gpio_direction_input(GPIO_PHONE_ACTIVE);

			if (!gpio_get_value(GPIO_nPOWER) || gpio_get_value(GPIO_PHONE_ACTIVE))
			{
				/* Wait Power Button Release */
				while (gpio_get_value(GPIO_PHONE_ACTIVE)) 
				{
					if (cnt++ < 5) {
						printk(KERN_EMERG "%s: GPIO_PHONE_ACTIVE is High(%d)\n", __func__, cnt);
						mdelay(1000);

					} 
					else {
						printk(KERN_EMERG "%s: GPIO_PHONE_ACTIVE timed-out!!!\n", __func__);

						/* (PHONE_RST: Output Low) -> (nPHONE_RST: Low) -> (MSM_PSHOLD: Low) -> CP PMIC Off */
						gpio_direction_output(GPIO_CP_RST, GPIO_LEVEL_HIGH);
						s3c_gpio_setpull(GPIO_CP_RST, S3C_GPIO_PULL_NONE);
						gpio_set_value(GPIO_CP_RST, GPIO_LEVEL_LOW);
						break;
					}
				}
				
				while (!gpio_get_value(GPIO_nPOWER));
			}

			if (cable_status) 
			{
				mode = REBOOT_MODE_CHARGING;
				SEC_SET_PARAM(__REBOOT_MODE, mode);
				
				/* Watchdog Reset */
				arch_reset(reset_mode);
			}			
			else
			{
				SEC_SET_PARAM(__REBOOT_MODE, mode);
				
				/* Setting DATA[8] to 0 in the PS_HOLD_CONTROL register */
				writel(readl(S5P_PS_HOLD_CONTROL) & ~(0x1 << 8), S5P_PS_HOLD_CONTROL);
				
				printk(KERN_EMERG "%s: should NOT be reached here!\n", __func__);
			}	
		}
	}

	
	while (1);
}

static int uart_current_owner = 0;

static ssize_t uart_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (uart_current_owner)
		return sprintf(buf, "%s[UART Switch] Current UART owner = PDA \n", buf);
	else
		return sprintf(buf, "%s[UART Switch] Current UART owner = MODEM \n", buf);
}

static ssize_t uart_switch_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int switch_sel = 1;
	
	SEC_GET_PARAM(__SWITCH_SEL, switch_sel);
	
	if (strncmp(buf, "PDA", 3) == 0 || strncmp(buf, "pda", 3) == 0) {
		gpio_set_value(GPIO_UART_SEL, GPIO_LEVEL_HIGH);
		uart_current_owner = 1;
		switch_sel |= UART_SEL_MASK;
		printk("[UART Switch] Path : PDA\n");
	}
	if (strncmp(buf, "MODEM", 5) == 0 || strncmp(buf, "modem", 5) == 0) {
		gpio_set_value(GPIO_UART_SEL, GPIO_LEVEL_LOW);	
		uart_current_owner = 0;	
		switch_sel &= ~UART_SEL_MASK;
		printk("[UART Switch] Path : MODEM\n");
	}
	
	SEC_SET_PARAM(__SWITCH_SEL, switch_sel)

	return size;
}

static DEVICE_ATTR(uart_sel, S_IRUGO |S_IWUGO | S_IRUSR | S_IWUSR, uart_switch_show, uart_switch_store);

static int apollo_notifier_call(struct notifier_block *this, unsigned long code, void *_cmd)
{
	int mode = REBOOT_MODE_NONE;

	if ((code == SYS_RESTART) && _cmd) {
		if (!strcmp((char *)_cmd, "arm11_fota"))
			mode = REBOOT_MODE_ARM11_FOTA;
		else if (!strcmp((char *)_cmd, "arm9_fota"))
			mode = REBOOT_MODE_ARM9_FOTA;
		else if (!strcmp((char *)_cmd, "recovery")) 
			mode = REBOOT_MODE_RECOVERY;
		else if (!strcmp((char *)_cmd, "download")) 
			mode = REBOOT_MODE_DOWNLOAD;
	}

	SEC_SET_PARAM(__REBOOT_MODE, mode);
	
	return NOTIFY_DONE;
}

static struct notifier_block apollo_reboot_notifier = {
	.notifier_call = apollo_notifier_call,
};

static void apollo_phone_reset_gpio_drvstr_set(void)
{
	s3c_gpio_set_drv_str(GPIO_CP_RST, GPIO_DRV_SR_4X);	
}

static void apollo_switch_init(void)
{
	sec_class = class_create(THIS_MODULE, "sec");
	if (IS_ERR(sec_class))
		pr_err("Failed to create class(sec)!\n");

	switch_dev = device_create(sec_class, NULL, 0, NULL, "switch");
	if (IS_ERR(switch_dev))
		pr_err("Failed to create device(switch)!\n");

	if (gpio_is_valid(GPIO_UART_SEL)) {
		if (gpio_request(GPIO_UART_SEL, "UART_SEL"))
			printk(KERN_ERR "Failed to request GPIO_UART_SEL!\n");
//		gpio_direction_output(GPIO_UART_SEL, 1);
	}
	s3c_gpio_setpull(GPIO_UART_SEL, S3C_GPIO_PULL_NONE);
	s3c_gpio_pdn_cfgpin(GPIO_UART_SEL, S3C_GPIO_SLP_PREV);
	s3c_gpio_set_pdn_pud(GPIO_UART_SEL, S3C_GPIO_PULL_NONE);

	// Set initial value by current UART_SEL pin status.
	if(gpio_get_value(GPIO_UART_SEL))
		uart_current_owner = 1;
	else
		uart_current_owner = 0;

	if (device_create_file(switch_dev, &dev_attr_uart_sel) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_uart_sel.attr.name);
};

static void __init smdk6442_machine_init(void)
{
//	s3c_device_nand.dev.platform_data = &s3c_nand_mtd_part_info;

	Cam_i2c0_regs =  ioremap(0xEC100004,0x4);
	/* read hw revision pin */
	check_hw_rev_pin();

	s5p6442_init_gpio();

#ifdef CONFIG_FB_S3C_LMS300
	spi_register_board_info(spi_board_info, ARRAY_SIZE(spi_board_info));
	s3cfb_set_platdata(&lms300_data);
#elif defined(CONFIG_FB_S3C_S6D04D1)
//	spi_register_board_info(spi_board_info, ARRAY_SIZE(spi_board_info));
	s3cfb_set_platdata(&s6d04d1_data);
#endif

	s3c_i2c0_set_platdata(&camera_i2c_data);
	s3c_i2c1_set_platdata(NULL);
	s3c_i2c2_set_platdata(&touch_i2c_data);		//quantum touch device

#ifdef CONFIG_S5P64XX_ADCTS
	s3c_adcts_set_platdata (&s3c_adcts_cfgs);
#endif

//	i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));
	i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));
	i2c_register_board_info(2, i2c_devs2, ARRAY_SIZE(i2c_devs2));
	i2c_register_board_info(4, i2c_devs4, ARRAY_SIZE(i2c_devs4));
//	i2c_register_board_info(5, i2c_devs5, ARRAY_SIZE(i2c_devs5)); /* for fsa9480 */
	i2c_register_board_info(7, i2c_devs7, ARRAY_SIZE(i2c_devs7)); /* for Si4709 */	
	i2c_register_board_info(8, i2c_devs8, ARRAY_SIZE(i2c_devs8));
	i2c_register_board_info(9, i2c_devs9, ARRAY_SIZE(i2c_devs9));

	// 091014 Univ6442 Sound (beta3)
	//apollo_wm8994_init();
	
	/* fimc */
	s3c_fimc0_set_platdata(&fimc_plat);
	s3c_fimc1_set_platdata(&fimc_plat);
	s3c_fimc2_set_platdata(&fimc_plat);

	apollo_cam_pin_config();

	/* Power up the camera only when it is needed. Powering down now */
	univ6442_cam0_power(0);

	/* fb */
//	s3cfb_set_platdata(&fb_data);

	platform_add_devices(smdk6442_devices, ARRAY_SIZE(smdk6442_devices));

	s5p6442_pm_init();

	pm_power_off = apollo_pm_power_off;

	register_reboot_notifier(&apollo_reboot_notifier);

	apollo_switch_init();

//	smdk_backlight_register();
	s5p6442_add_mem_devices (&pmem_setting);	

#ifdef CONFIG_PHONET
	platform_device_register(&modemctl);
	platform_device_register(&onedram);
#endif

	/* lcd */
	check_lcd_id();

	apollo_phone_reset_gpio_drvstr_set();
}

static void __init smdk6442_fixup(struct machine_desc *desc,
					struct tag *tags, char **cmdline,
					struct meminfo *mi)
{

	mi->bank[0].start = PHYS_OFFSET_DDR;
#ifndef CONFIG_MTD_ONENAND
	if(__raw_readl(0x20001000)==0x100)
		mi->bank[0].size = PHYS_SIZE_DDR_256M;		
	else
		mi->bank[0].size = PHYS_SIZE_DDR_128M;
#else
     mi->bank[0].size = PHYS_SIZE_DDR_256M;
#endif 
	mi->bank[0].node = 0;

	mi->nr_banks = 1;
}

MACHINE_START(APOLLO, "GT-I5800")
	.phys_io	= S3C_PA_UART & 0xfff00000,
	.io_pg_offst	= (((u32)S3C_VA_UART) >> 18) & 0xfffc,
	.boot_params	= S5P64XX_PA_SDRAM + 0x100,
	.fixup		= smdk6442_fixup,
	.init_irq	= s5p6442_init_irq,
	.map_io		= smdk6442_map_io,
	.init_machine	= smdk6442_machine_init,
#if! defined (CONFIG_HIGH_RES_TIMERS)
	.timer		= &s5p64xx_timer,
#else
#ifdef CONFIG_HRT_RTC
	.timer		= &s5p64xx_timer,
#else
	.timer		= &sec_timer,
#endif
#endif
MACHINE_END

#ifdef CONFIG_USB_SUPPORT
/* Initializes OTG Phy. */
void otg_phy_init(void) {
	writel(1, S5P_USB_PHY_CONTROL);
	writel(0xa0, S3C_USBOTG_PHYPWR);		/* Power up */
        writel(OTGH_PHY_CLK_VALUE, S3C_USBOTG_PHYCLK);
	writel(0x1, S3C_USBOTG_RSTCON);
	udelay(50);
	writel(0x0, S3C_USBOTG_RSTCON);
	udelay(50);
}
EXPORT_SYMBOL(otg_phy_init);

/* USB Control request data struct must be located here for DMA transfer */
struct usb_ctrlrequest usb_ctrl __attribute__((aligned(8)));
EXPORT_SYMBOL(usb_ctrl);

/* OTG PHY Power Off */
void otg_phy_off(void) {
	writel(readl(S3C_USBOTG_PHYPWR)|(0x1F<<1), S3C_USBOTG_PHYPWR);
	writel(0, S5P_USB_PHY_CONTROL);
}
EXPORT_SYMBOL(otg_phy_off);

#endif

#if defined(CONFIG_RTC_DRV_S3C)
/* RTC common Function for samsung APs*/
unsigned int s3c_rtc_set_bit_byte(void __iomem *base, uint offset, uint val)
{
	writeb(val, base + offset);

	return 0;
}

unsigned int s3c_rtc_read_alarm_status(void __iomem *base)
{
	return 1;
}

void s3c_rtc_set_pie(void __iomem *base, uint to)
{
	unsigned int tmp;

	tmp = readw(base + S3C2410_RTCCON) & ~S3C_RTCCON_TICEN;

        if (to)
                tmp |= S3C_RTCCON_TICEN;

        writew(tmp, base + S3C2410_RTCCON);
}

void s3c_rtc_set_freq_regs(void __iomem *base, uint freq, uint s3c_freq)
{
	unsigned int tmp;

        tmp = readw(base + S3C2410_RTCCON) & (S3C_RTCCON_TICEN | S3C2410_RTCCON_RTCEN );
        writew(tmp, base + S3C2410_RTCCON);
        s3c_freq = freq;
        tmp = (32768 / freq)-1;
        writel(tmp, base + S3C2410_TICNT);
}

void s3c_rtc_enable_set(struct platform_device *pdev,void __iomem *base, int en)
{
	unsigned int tmp;

	if (!en) {
		tmp = readw(base + S3C2410_RTCCON);
		writew(tmp & ~ (S3C2410_RTCCON_RTCEN | S3C_RTCCON_TICEN), base + S3C2410_RTCCON);
	} else {
		/* re-enable the device, and check it is ok */
		if ((readw(base+S3C2410_RTCCON) & S3C2410_RTCCON_RTCEN) == 0){
			dev_info(&pdev->dev, "rtc disabled, re-enabling\n");

			tmp = readw(base + S3C2410_RTCCON);
			writew(tmp|S3C2410_RTCCON_RTCEN, base+S3C2410_RTCCON);
		}

		if ((readw(base + S3C2410_RTCCON) & S3C2410_RTCCON_CNTSEL)){
			dev_info(&pdev->dev, "removing RTCCON_CNTSEL\n");

			tmp = readw(base + S3C2410_RTCCON);
			writew(tmp& ~S3C2410_RTCCON_CNTSEL, base+S3C2410_RTCCON);
		}

		if ((readw(base + S3C2410_RTCCON) & S3C2410_RTCCON_CLKRST)){
			dev_info(&pdev->dev, "removing RTCCON_CLKRST\n");

			tmp = readw(base + S3C2410_RTCCON);
			writew(tmp & ~S3C2410_RTCCON_CLKRST, base+S3C2410_RTCCON);
		}
	}
}
#endif

#if defined(CONFIG_KEYPAD_S3C) || defined (CONFIG_KEYPAD_S3C_MODULE)
void s3c_setup_keypad_cfg_gpio(int rows, int columns)
{
	unsigned int gpio;
	unsigned int end;

	end = GPIO_ROW(rows);
	
	/* Set all the necessary GPH2 pins to special-function 0 */
	for (gpio = GPIO_ROW(0); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(3));
        s3c_gpio_setpull(gpio, S3C_GPIO_PULL_UP);
	}

	/* drive strength to max */

	end = GPIO_COL(columns);

	/* Set all the necessary GPK pins to special-function 0 */
	for (gpio = GPIO_COL(0); gpio < end; gpio++) {
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(1));
		//gpio_set_value(gpio, GPIO_LEVEL_HIGH);
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
	}
}

EXPORT_SYMBOL(s3c_setup_keypad_cfg_gpio);

void s3c_keypad_suspend_cfg_gpio(int rows, int columns)
{
        unsigned int gpio;
        unsigned int end;

        end = GPIO_ROW(rows);


        /* Set all the necessary GPH2 pins to special-function 0 */
        for (gpio = GPIO_ROW(0); gpio < end; gpio++) {
                s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0));
        s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
        }

        /* drive strength to max */

        end = GPIO_COL(columns);

        /* Set all the necessary GPK pins to special-function 0 */
        for (gpio = GPIO_COL(0); gpio < end; gpio++) {
                s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0));
                s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
        }
}

EXPORT_SYMBOL(s3c_keypad_suspend_cfg_gpio);
#endif

void pmic_gpio_init(void)
{
    // DVSARMx all are set as 1.2V which is default value
    // DVSARM1 is selected to set 1.2V for VARM
    s3c_gpio_cfgpin(GPIO_BUCK_1_EN_A, S3C_GPIO_SFN(GPIO_BUCK_1_EN_A_STATE));
    gpio_set_value(GPIO_BUCK_1_EN_A, GPIO_LEVEL_LOW);
    s3c_gpio_cfgpin(GPIO_BUCK_1_EN_B, S3C_GPIO_SFN(GPIO_BUCK_1_EN_B_STATE));
    gpio_set_value(GPIO_BUCK_1_EN_B, GPIO_LEVEL_LOW);

    // DVSINTx all are set as 1.2V which is default value
    // DVSINT1 is selected to set 1.2V for VINT
    s3c_gpio_cfgpin(GPIO_BUCK_2_EN, S3C_GPIO_SFN(GPIO_BUCK_2_EN_STATE));
    gpio_set_value(GPIO_BUCK_2_EN, GPIO_LEVEL_LOW);
}
EXPORT_SYMBOL(pmic_gpio_init);


static int s5p6442_gpio_alive_table[][4] = {
/********************************* ALIVE PART ************************************************/
	/* GPH0 */
	{ GPIO_AP_PS_HOLD, 0x3, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_ACC_INT, 0x0, GPIO_LEVEL_LOW, S3C_GPIO_PULL_DOWN},
	{ GPIO_BUCK_1_EN_A, 0x1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_BUCK_1_EN_B, 0x1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_BUCK_2_EN, 0x1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_FLM_SEL, 0x1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_EARJACK_DET, 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_AP_PMIC_IRQ, 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},

	
	/* GPH1 */
	{ GPIO_SIM_nDETECT, 0xf, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{ GPIO_CP_RST, 0x0, GPIO_LEVEL_LOW, S3C_GPIO_PULL_UP},
	{ GPIO_PDA_ACTIVE, 0x1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE}, 
	{ GPIO_nINT_ONEDRAM_AP, 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_RESET_REQ_N, 0x0, GPIO_LEVEL_LOW, S3C_GPIO_PULL_DOWN},
	//{ GPIO_CODEC_LDO_EN, 0x1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ S5P64XX_GPH1(6), 0x1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE}, 	
	{ GPIO_PHONE_ACTIVE, 0xf, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},

	/* GPH2 */
	{ GPIO_T_FLASH_DETECT, 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_MSENSE_IRQ, 0x0, GPIO_LEVEL_LOW, S3C_GPIO_PULL_DOWN},
	{ GPIO_EAR_SEND_END, 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ S5P64XX_GPH2(3), 0x1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_WLAN_HOST_WAKE, 0x1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_BT_HOST_WAKE, 0x1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_nPOWER, 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_JACK_nINT, 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},

	/* GPH3 */
	{ GPIO_KEY_ROW0, 0x3, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
//	{ GPIO_KEY_ROW1, 0x3, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_PS_OUT, 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_KEY_ROW2, 0x3, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_KEY_ROW3, 0x3, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ S5P64XX_GPH3(4), 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},  // VOL_UP
	{ S5P64XX_GPH3(5), 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},  // VOL_DOWN
//	{ S5P64XX_GPH3(6), 0x1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
//	{ S5P64XX_GPH3(7), 0x1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},	

};

// GPIO Alive table for H/W Rev0.5
static int s5p6442_gpio_alive_table_05[][4] = {
/********************************* ALIVE PART ************************************************/
	/* GPH0 */
	{ GPIO_AP_PS_HOLD, 0x3, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_ACC_INT, 0x0, GPIO_LEVEL_LOW, S3C_GPIO_PULL_DOWN},
	{ GPIO_BUCK_1_EN_A, 0x1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_BUCK_1_EN_B, 0x1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_BUCK_2_EN, 0x1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_FLM_SEL, 0x1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_EARJACK_DET, 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_AP_PMIC_IRQ, 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},

	
	/* GPH1 */
	{ GPIO_SIM_nDETECT, 0xf, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{ GPIO_CP_RST, 0x0, GPIO_LEVEL_LOW, S3C_GPIO_PULL_UP},
	{ GPIO_PDA_ACTIVE, 0x1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE}, 
	{ GPIO_nINT_ONEDRAM_AP, 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_RESET_REQ_N, 0x0, GPIO_LEVEL_LOW, S3C_GPIO_PULL_DOWN},
	//{ GPIO_CODEC_LDO_EN, 0x1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ S5P64XX_GPH1(6), 0x1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE}, 	
	{ GPIO_PHONE_ACTIVE, 0xf, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},

	/* GPH2 */
	{ GPIO_T_FLASH_DETECT, 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_MSENSE_IRQ, 0x0, GPIO_LEVEL_LOW, S3C_GPIO_PULL_DOWN},
	{ GPIO_EAR_SEND_END, 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ S5P64XX_GPH2(3), 0x1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	//{ GPIO_WLAN_HOST_WAKE, 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	//{ GPIO_BT_HOST_WAKE, 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_nPOWER, 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_JACK_nINT, 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},

	/* GPH3 */
	{ GPIO_KEY_ROW0, 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},  // OK
	{ GPIO_PS_OUT, 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_KEY_ROW2, 0x0, GPIO_LEVEL_LOW, S3C_GPIO_PULL_UP},  // CAPTURE
	{ GPIO_KEY_ROW3, 0x0, GPIO_LEVEL_LOW, S3C_GPIO_PULL_UP},  // NC
	{ S5P64XX_GPH3(4), 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},  // VOL_UP
	{ S5P64XX_GPH3(5), 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},  // VOL_DOWN
//	{ S5P64XX_GPH3(6), 0x1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
//	{ S5P64XX_GPH3(7), 0x1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},

};

static int s5p6442_gpio_alive_table_10[][4] = {
	{ S5P64XX_GPH3(6), 0x0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{ S5P64XX_GPH3(7), 0x0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
};

static int s5p6442_gpio_alive_table_ex[][4] = {
	{ S5P64XX_GPH3(6), 0x1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ S5P64XX_GPH3(7), 0x1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
};

// GPIO Alive table for H/W Open
static int s5p6442_gpio_alive_table_open[][4] = {
/********************************* ALIVE PART ************************************************/
	/* GPH0 */
	{ GPIO_AP_PS_HOLD, 0x3, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_ACC_INT, 0x0, GPIO_LEVEL_LOW, S3C_GPIO_PULL_DOWN},
	{ GPIO_BUCK_1_EN_A, 0x1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_BUCK_1_EN_B, 0x1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_BUCK_2_EN, 0x1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_FLM_SEL, 0x1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_EARJACK_DET, 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_AP_PMIC_IRQ, 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},

	
	/* GPH1 */
	{ GPIO_SIM_nDETECT, 0xf, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{ GPIO_CP_RST, 0x0, GPIO_LEVEL_LOW, S3C_GPIO_PULL_UP},
	{ GPIO_PDA_ACTIVE, 0x1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE}, 
	{ GPIO_nINT_ONEDRAM_AP, 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_RESET_REQ_N, 0x0, GPIO_LEVEL_LOW, S3C_GPIO_PULL_DOWN},
	//{ GPIO_CODEC_LDO_EN, 0x1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ S5P64XX_GPH1(6), 0x1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE}, 	
	{ GPIO_PHONE_ACTIVE, 0xf, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},

	/* GPH2 */
	{ GPIO_T_FLASH_DETECT, 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_MSENSE_IRQ, 0x0, GPIO_LEVEL_LOW, S3C_GPIO_PULL_DOWN},
	{ GPIO_EAR_SEND_END, 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ S5P64XX_GPH2(3), 0x1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	//{ GPIO_WLAN_HOST_WAKE, 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	//{ GPIO_BT_HOST_WAKE, 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_nPOWER, 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_JACK_nINT, 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},

	/* GPH3 */
	{ GPIO_KEY_ROW0, 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},  // OK
	{ GPIO_PS_OUT, 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{ GPIO_KEY_ROW2, 0x0, GPIO_LEVEL_LOW, S3C_GPIO_PULL_UP},  // CAPTURE
	{ GPIO_KEY_ROW3, 0x0, GPIO_LEVEL_LOW, S3C_GPIO_PULL_UP},  // NC
	{ S5P64XX_GPH3(4), 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},  // VOL_UP
	{ S5P64XX_GPH3(5), 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},  // VOL_DOWN
	{ S5P64XX_GPH3(6), 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},  // BACK
	{ S5P64XX_GPH3(7), 0xf, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},  // MENU

};

static int s5p6442_gpio_table[][6] = {
	/** OFF PART **/
	/* GPA0 */
	{ GPIO_BT_UART_RXD, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_BT_UART_TXD, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_BT_UART_CTS, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_BT_UART_RTS, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE },
	{ GPIO_AP_COM_RXD, GPIO_AP_COM_RXD_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_AP_COM_TXD, GPIO_AP_COM_TXD_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	/* NC */{ S5P64XX_GPA0(6), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	/* NC */{ S5P64XX_GPA0(7), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },

	/* GPA1 */
	{ GPIO_AP_FLM_RXD_2_8V, 2, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_AP_FLM_TXD_2_8V, 2, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },

	/* GPB */
	/* NC */{ S5P64XX_GPB(0), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	/* NC */{ S5P64XX_GPB(1), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	/* NC */{ S5P64XX_GPB(2), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	/* NC */{ S5P64XX_GPB(3), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },

	/* GPC0 */
	{ GPIO_CODEC_I2S_CLK, GPIO_CODEC_I2S_CLK_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	/* NC */{ S5P64XX_GPC0(1), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_CODEC_I2S_WS, GPIO_CODEC_I2S_WS_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_CODEC_I2S_DI, GPIO_CODEC_I2S_DI_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_CODEC_I2S_DO, GPIO_CODEC_I2S_DO_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	
	/* GPC1 */
	/* NC */{S5P64XX_GPC1(0), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	/* NC */{S5P64XX_GPC1(1), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	/* NC */{S5P64XX_GPC1(2), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	/* NC */{S5P64XX_GPC1(3), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	/* NC */{S5P64XX_GPC1(4), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	
	/* GPD0 */
	{ GPIO_VIBTONE_PWM, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	/* NC */{S5P64XX_GPD0(1) , 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },

	/* GPD1 - I2C */
	{ GPIO_CAM_SDA_2_8V, 2, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_CAM_SCL_2_8V, 2, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	
	{ GPIO_AP_SDA_2_8V, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_AP_SCL_2_8V, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },

	{ GPIO_TSP_SDA_2_8V, 2, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_TSP_SCL_2_8V, 2, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },

	/* GPE0 */
	{ GPIO_CAM_PCLK, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_CAM_VSYNC, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_CAM_HSYNC, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_CAM_D_0, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_CAM_D_1, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_CAM_D_2, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_CAM_D_3, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_CAM_D_4, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },

	/* GPE1 */
	{ GPIO_CAM_D_5, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_CAM_D_6, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_CAM_D_7, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_CAM_MCLK, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_TOUCH_RST, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_UP },
	
	/* GPF0 */
	{ GPIO_LCD_HSYNC, GPIO_LCD_HSYNC_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_LCD_VSYNC, GPIO_LCD_VSYNC_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_LCD_VDEN,   GPIO_LCD_VDEN_STATE,   GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_LCD_MCLK,   GPIO_LCD_MCLK_STATE,   GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_LCD_D_0, GPIO_LCD_D_0_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_LCD_D_1, GPIO_LCD_D_1_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },

	/* GPF1 */
	{ GPIO_LCD_D_2, GPIO_LCD_D_2_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_LCD_D_3, GPIO_LCD_D_3_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_LCD_D_4, GPIO_LCD_D_4_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_LCD_D_5, GPIO_LCD_D_5_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_LCD_D_6, GPIO_LCD_D_6_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_LCD_D_7, GPIO_LCD_D_7_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_LCD_D_8, GPIO_LCD_D_8_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_LCD_D_9, GPIO_LCD_D_9_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	
	/* GPF2 */
	{ GPIO_LCD_D_10, GPIO_LCD_D_10_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_LCD_D_11, GPIO_LCD_D_11_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_LCD_D_12, GPIO_LCD_D_12_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_LCD_D_13, GPIO_LCD_D_13_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_LCD_D_14, GPIO_LCD_D_14_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_LCD_D_15, GPIO_LCD_D_15_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_LCD_D_16, GPIO_LCD_D_16_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_LCD_D_17, GPIO_LCD_D_17_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },

	 /* GPF3 */
	{ GPIO_LCD_D_18, GPIO_LCD_D_18_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_LCD_D_19, GPIO_LCD_D_19_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_LCD_D_20, GPIO_LCD_D_20_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_LCD_D_21, GPIO_LCD_D_21_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_LCD_D_22, GPIO_LCD_D_22_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_LCD_D_23, GPIO_LCD_D_23_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	
	/* GPG0 */
	{ GPIO_CAM_EN, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	/* NC */{S5P64XX_GPG0(1), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_TOUCH_INT, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_CAM_3M_nSTBY, GPIO_CAM_3M_nSTBY_STATE, GPIO_LEVEL_LOW, S3C_GPIO_SLP_OUT0, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	/* NC */{S5P64XX_GPG0(4), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
//	{GPIO_UART_SEL, 1, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE }, // check
	/* NC */{S5P64XX_GPG0(6), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },

	/* GPG1 */
	{ GPIO_WLAN_SDIO_CLK, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },
	{ GPIO_WLAN_SDIO_CMD, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_WLAN_nRST, GPIO_WLAN_nRST_STATE, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_WLAN_SDIO_D_0, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_WLAN_SDIO_D_1, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_WLAN_SDIO_D_2, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_WLAN_SDIO_D_3, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },

	/* GPG2 */
	{ GPIO_T_FLASH_CLK, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_T_FLASH_CMD, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	/* NC */{S5P64XX_GPG2(2), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_T_FLASH_D_0, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },	
	{ GPIO_T_FLASH_D_1, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_T_FLASH_D_2, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_T_FLASH_D_3, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },

	/*GPJ0 */ 
	{ GPIO_KEY_COL0, GPIO_KEY_COL0_STATE, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
	{ GPIO_KEY_COL1, GPIO_KEY_COL1_STATE, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
	{ GPIO_KEY_COL2, GPIO_KEY_COL2_STATE, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
	{ GPIO_KEY_COL3, GPIO_KEY_COL3_STATE, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
	{ GPIO_FM_SCL_2_8V, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_FM_SDA_2_8V, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	/* NC */{S5P64XX_GPJ0(6), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	/* NC */{S5P64XX_GPJ0(7), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
      
	/*GPJ1 */  
	{ GPIO_PHONE_ON, GPIO_PHONE_ON_STATE, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_HAPTIC_EN, GPIO_HAPTIC_EN_STATE, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
	/* NC */{S5P64XX_GPJ1(2), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	/* NC */{S5P64XX_GPJ1(3), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	/* NC */{S5P64XX_GPJ1(4), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_CAM_3M_nRST, GPIO_CAM_3M_nRST_STATE, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	
	/*GPJ2 */
	{ GPIO_CODEX_XTAL_EN, 0, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{GPIO_MICBIAS2_EN, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE },
	/* TP */{ GPIO_TA_CURRENT_SEL_AP, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_FM_INT, GPIO_FM_INT_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_UP },
	{ GPIO_FM_BUS_nRST, GPIO_FM_BUS_nRST_STATE, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_UP },
	{ GPIO_BT_WAKE, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_WLAN_WAKE, GPIO_WLAN_WAKE_STATE, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },

	/*GPJ3 */
	{ GPIO_WLAN_BT_EN, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE},
	{GPIO_MIC_SEL, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE },
	{ GPIO_EAR_SEL, GPIO_EAR_SEL_STATE, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_PREV, S3C_GPIO_PULL_NONE}, 
	{ GPIO_BOOT_MODE, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
//	{ GPIO_PS_OUT, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
//	{ GPIO_PS_EN, GPIO_PS_EN_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE },
//  { GPIO_SENSOR_SDA, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
//	{ GPIO_SENSOR_SCL, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
//	/* NC */{S5P64XX_GPJ3(6), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_BT_nRST, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
#ifdef USE_GPIO_ENABLE_FOR_LCD_POWER
	{ GPIO_TOUCH_EN, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE },
	{ GPIO_ALS_nRST, GPIO_ALS_nRST_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
#endif

	/* GPJ4 */
	{ GPIO_AP_PMIC_SDA, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	/* NC */{S5P64XX_GPJ4(1), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	/* NC */{S5P64XX_GPJ4(2), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_AP_PMIC_SCL, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_MSENSE_nRST, GPIO_MSENSE_nRST_STATE, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE },

	/* MP0_1 */
	{ GPIO_DISPLAY_CS, GPIO_DISPLAY_CS_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_AP_NANDCS, GPIO_AP_NANDCS_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE },
//	/* NC */{S5P64XX_MP01(3), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
//	/* NC */{S5P64XX_MP01(4), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
//	/* NC */{S5P64XX_MP01(5), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
//	/* NC */{S5P64XX_MP01(6), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
//	/* NC */{S5P64XX_MP01(7), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },

	/* MP0_3 */
//	/* NC */{S5P64XX_MP03(2), 1, GPIO_LEVEL_HIGH, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT1, S3C_GPIO_PULL_NONE },

	/* MP0_4 */
//	/* NC */{S5P64XX_MP04(0), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_DISPLAY_CLK, GPIO_DISPLAY_CLK_STATE, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_DISPLAY_SI, GPIO_DISPLAY_SI_STATE, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_LCD_ID, GPIO_LCD_ID_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
//	/* NC */{S5P64XX_MP04(5), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
//	/* NC */{S5P64XX_MP04(6), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_HW_REV_MODE0, GPIO_HW_REV_MODE0_STATE, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	
	/* MP0_5 */
	{ GPIO_HW_REV_MODE1, GPIO_HW_REV_MODE1_STATE, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_HW_REV_MODE2, GPIO_HW_REV_MODE2_STATE, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_AP_SCL, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	{ GPIO_AP_SDA, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_NONE },
	/* NC */{S5P64XX_MP05(4), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	{ GPIO_MLCD_RST, GPIO_MLCD_RST_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
	/* NC */{S5P64XX_MP05(6), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_UP },
	/* NC */{S5P64XX_MP05(7), 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },

//	{ GPIO_USB_SEL30, GPIO_USB_SEL30_STATE, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
#ifdef USE_GPIO_ENABLE_FOR_LCD_POWER
//	{ GPIO_MLCD_ON, GPIO_MLCD_ON_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_OUT0, S3C_GPIO_PULL_NONE },
#endif
//	{ GPIO_ADC_EN, GPIO_ADC_EN_STATE, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE, S3C_GPIO_SLP_INPUT, S3C_GPIO_PULL_DOWN },

};

void s5p_config_gpio_alive_table(int array_size, int (*gpio_table)[4])
{
	u32 i, gpio;

	pr_debug("%s: ++\n", __func__);

	for (i = 0; i < array_size; i++) {
		gpio = gpio_table[i][0];
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(gpio_table[i][1]));
		s3c_gpio_setpull(gpio, gpio_table[i][3]);
		if (gpio_table[i][2] != GPIO_LEVEL_NONE)
			gpio_set_value(gpio, gpio_table[i][2]);
		}
	pr_debug("%s: --\n", __func__);
}
EXPORT_SYMBOL(s5p_config_gpio_alive_table);

void s5p_config_gpio_table(int array_size, int (*gpio_table)[6])
{
	u32 i, gpio;

	pr_debug("%s: ++\n", __func__);
	for (i = 0; i < array_size; i++) {
		gpio = gpio_table[i][0];		
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(gpio_table[i][1]));
		s3c_gpio_setpull(gpio, gpio_table[i][3]);
		s3c_gpio_pdn_cfgpin(gpio, gpio_table[i][4]);
		s3c_gpio_set_pdn_pud(gpio, gpio_table[i][5]);
		if (gpio_table[i][2] != GPIO_LEVEL_NONE)
			gpio_set_value(gpio, gpio_table[i][2]);
		
	}

/* 
 * If HW_REV <=4
 * 		S5P64XX_GPJ3(5) is GPIO_PS_EN
 * 		S5P64XX_GPJ3(6) is No Connection 
 * If HW_REV >4 
 * 		S5P64XX_GPJ3(5) is GPIO_SENSOR_SDA
 * 		S5P64XX_GPJ3(6) is GPIO_SENSOR_SCL 
 */
	if(apollo_get_remapped_hw_rev_pin()<=4)
	{		
		gpio = S5P64XX_GPJ3(5);		
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(1));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_DOWN);
		s3c_gpio_pdn_cfgpin(gpio, S3C_GPIO_SLP_OUT1);
		s3c_gpio_set_pdn_pud(gpio, S3C_GPIO_PULL_NONE);	
		
		gpio = S5P64XX_GPJ3(6);		
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(1));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
		s3c_gpio_pdn_cfgpin(gpio, S3C_GPIO_SLP_OUT0);
		s3c_gpio_set_pdn_pud(gpio, S3C_GPIO_PULL_NONE);
		gpio_set_value(gpio, GPIO_LEVEL_LOW);	
	}
	else
	{
		gpio = S5P64XX_GPJ3(5);		
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
		s3c_gpio_pdn_cfgpin(gpio, S3C_GPIO_SLP_INPUT);
		s3c_gpio_set_pdn_pud(gpio, S3C_GPIO_PULL_NONE);	
		
		gpio = S5P64XX_GPJ3(6);		
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(0));
		s3c_gpio_setpull(gpio, S3C_GPIO_PULL_NONE);
		s3c_gpio_pdn_cfgpin(gpio, S3C_GPIO_SLP_INPUT);
		s3c_gpio_set_pdn_pud(gpio, S3C_GPIO_PULL_NONE);			
	}

	pr_debug("%s: --\n", __func__);
}
EXPORT_SYMBOL(s5p_config_gpio_table);

unsigned int s5p_pm_config_wakeup_source(void)
{
		unsigned int tmp;
		unsigned int eint_wakeup_mask;
    /* Set wakeup mask regsiter */
	    	tmp = 0xFFFF;
		tmp &= ~(1 << 5);   // keypad
		tmp &= ~(1 << 1);
		tmp &= ~(1 << 2);
		__raw_writel(tmp , S5P_WAKEUP_MASK);


		//Save the normal mode configuration of WAKE_UP sources and make EXT Key as a
		//wake up source from suspend mode(Praveen)
		eint_wakeup_mask = __raw_readl(S5P_EINT_WAKEUP_MASK);
		tmp = 0x00000000;
		tmp  |= (1 << 14);
		tmp  |= (1 << 19);
//		tmp  |= (1 << 21);
		tmp  |= (1 << 26);
		tmp  |= (1 << 27);

		__raw_writel(tmp, S5P_EINT_WAKEUP_MASK);
		return eint_wakeup_mask;
}

void s5p_config_sleep_gpio(void)
{
	if(apollo_get_hw_type() == 1)
	{
		// Open
		s5p_config_gpio_alive_table(ARRAY_SIZE(s5p6442_gpio_alive_table_open),
				s5p6442_gpio_alive_table_open);
	}
	else
	{
		if(apollo_get_remapped_hw_rev_pin() >= 0x3) {  // Above Rev 0.5
			s5p_config_gpio_alive_table(ARRAY_SIZE(s5p6442_gpio_alive_table_05),
					s5p6442_gpio_alive_table_05);
		}
		else {
			s5p_config_gpio_alive_table(ARRAY_SIZE(s5p6442_gpio_alive_table),
					s5p6442_gpio_alive_table);
		}

		if(apollo_get_remapped_hw_rev_pin() == 6) {  // Rev 0.9b, Rev 1.0
			s5p_config_gpio_alive_table(ARRAY_SIZE(s5p6442_gpio_alive_table_10),
					s5p6442_gpio_alive_table_10);
		}
		else {
			s5p_config_gpio_alive_table(ARRAY_SIZE(s5p6442_gpio_alive_table_ex),
					s5p6442_gpio_alive_table_ex);
		}		
	}

	return;
}
EXPORT_SYMBOL(s5p_config_sleep_gpio);

void s5p6442_init_gpio(void)
{
	s5p_config_gpio_table(ARRAY_SIZE(s5p6442_gpio_table),
			s5p6442_gpio_table);

	__raw_writel(0x00, (S5P64XX_GPA0_BASE + 0x2d0));  // for reducing sleep current (MP0_2)	// 20101014 0x20 > 0x00
	__raw_writel(0x90, (S5P64XX_GPA0_BASE + 0x2f0));  // for reducing sleep current (MP0_3)	// 20101014 0x290 > 0x90
	
	s5p_config_sleep_gpio();  // alive part gpio setting
}


//*************************** WIFI ****************************

void s3c_config_gpio_alive_table(int array_size, int (*gpio_table)[4])
{
	u32 i, gpio;

	for (i = 0; i < array_size; i++) {
		gpio = gpio_table[i][0];
		s3c_gpio_cfgpin(gpio, S3C_GPIO_SFN(gpio_table[i][1]));
		s3c_gpio_setpull(gpio, gpio_table[i][3]);
		if (gpio_table[i][2] != GPIO_LEVEL_NONE)
			gpio_set_value(gpio, gpio_table[i][2]);
	}
}

static unsigned long wlan_reglock_flags = 0;
static spinlock_t wlan_reglock = SPIN_LOCK_UNLOCKED;

static unsigned int wlan_gpio_table[][4] = {	
	{GPIO_WLAN_nRST, GPIO_WLAN_nRST_STATE, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_HOST_WAKE, GPIO_WLAN_HOST_WAKE_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN},
	{GPIO_WLAN_WAKE, GPIO_WLAN_WAKE_STATE, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},	
};

static unsigned int wlan_sdio_on_table[][4] = {
	{GPIO_WLAN_SDIO_CLK, GPIO_WLAN_SDIO_CLK_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_CMD, GPIO_WLAN_SDIO_CMD_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D_0, GPIO_WLAN_SDIO_D_0_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D_1, GPIO_WLAN_SDIO_D_1_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D_2, GPIO_WLAN_SDIO_D_2_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D_3, GPIO_WLAN_SDIO_D_3_STATE, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
};

static unsigned int wlan_sdio_off_table[][4] = {
/*cjh Aries
	{GPIO_WLAN_SDIO_CLK, 1, GPIO_LEVEL_LOW, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_CMD, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D0, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D1, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D2, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
	{GPIO_WLAN_SDIO_D3, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_NONE},
*/

// Apollo Eclair
	{GPIO_WLAN_SDIO_CLK, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN},
	{GPIO_WLAN_SDIO_CMD, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN},
	{GPIO_WLAN_SDIO_D_0, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN},
	{GPIO_WLAN_SDIO_D_1, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN},
	{GPIO_WLAN_SDIO_D_2, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN},
	{GPIO_WLAN_SDIO_D_3, 0, GPIO_LEVEL_NONE, S3C_GPIO_PULL_DOWN},
};

void wlan_setup_power(int on, int flag)
{
	printk(/*KERN_INFO*/ "%s %s", __func__, on ? "on" : "down");
	if (flag != 1) {	
		printk(/*KERN_DEBUG*/ "(on=%d, flag=%d)\n", on, flag);
//For Starting/Stopping Tethering service
#if 1
		if (on)
			gpio_set_value(GPIO_WLAN_nRST, GPIO_LEVEL_HIGH);
		else
			gpio_set_value(GPIO_WLAN_nRST, GPIO_LEVEL_LOW);		
#endif 
		return;
	}	
	printk(/*KERN_INFO*/ " --enter\n");
		
	if (on) {		
		s3c_config_gpio_alive_table(ARRAY_SIZE(wlan_gpio_table), wlan_gpio_table);
		s3c_config_gpio_alive_table(ARRAY_SIZE(wlan_sdio_on_table), wlan_sdio_on_table);
		
		/* PROTECT this check under spinlock.. No other thread should be touching
		 * GPIO_BT_REG_ON at this time.. If BT is operational, don't touch it. */
		spin_lock_irqsave(&wlan_reglock, wlan_reglock_flags);	
		/* need delay between v_bat & reg_on for 2 cycle @ 38.4MHz */
		udelay(5);
		
		gpio_set_value(GPIO_WLAN_BT_EN, GPIO_LEVEL_HIGH);
//cjh		s3c_gpio_slp_cfgpin(GPIO_WLAN_BT_EN, S3C_GPIO_SLP_OUT1);
		s3c_gpio_pdn_cfgpin(GPIO_WLAN_BT_EN, S3C_GPIO_SLP_OUT1);
		
		gpio_set_value(GPIO_WLAN_nRST, GPIO_LEVEL_HIGH);
//cjh		s3c_gpio_slp_cfgpin(GPIO_WLAN_nRST, S3C_GPIO_SLP_OUT1);
		s3c_gpio_pdn_cfgpin(GPIO_WLAN_nRST, S3C_GPIO_SLP_OUT1);
		
		printk(KERN_DEBUG "WLAN: GPIO_WLAN_BT_EN = %d, GPIO_WLAN_nRST = %d\n", 
			   gpio_get_value(GPIO_WLAN_BT_EN), gpio_get_value(GPIO_WLAN_nRST));
		
		spin_unlock_irqrestore(&wlan_reglock, wlan_reglock_flags);
	}
	else {
		/* PROTECT this check under spinlock.. No other thread should be touching
		 * GPIO_BT_REG_ON at this time.. If BT is operational, don't touch it. */
		spin_lock_irqsave(&wlan_reglock, wlan_reglock_flags);	
		/* need delay between v_bat & reg_on for 2 cycle @ 38.4MHz */
		udelay(5);
		
		if (gpio_get_value(GPIO_BT_nRST) == 0) {
			gpio_set_value(GPIO_WLAN_BT_EN, GPIO_LEVEL_LOW);	
//cjh			s3c_gpio_slp_cfgpin(GPIO_WLAN_BT_EN, S3C_GPIO_SLP_OUT0);
			s3c_gpio_pdn_cfgpin(GPIO_WLAN_BT_EN, S3C_GPIO_SLP_OUT0);
		}
		
		gpio_set_value(GPIO_WLAN_nRST, GPIO_LEVEL_LOW);
//cjh		s3c_gpio_slp_cfgpin(GPIO_WLAN_nRST, S3C_GPIO_SLP_OUT0);
		s3c_gpio_pdn_cfgpin(GPIO_WLAN_nRST, S3C_GPIO_SLP_OUT0);
		
		printk(KERN_DEBUG "WLAN: GPIO_WLAN_BT_EN = %d, GPIO_WLAN_nRST = %d\n", 
			   gpio_get_value(GPIO_WLAN_BT_EN), gpio_get_value(GPIO_WLAN_nRST));
		
		spin_unlock_irqrestore(&wlan_reglock, wlan_reglock_flags);
		
		s3c_config_gpio_alive_table(ARRAY_SIZE(wlan_sdio_off_table), wlan_sdio_off_table);		
	}
	
	/* mmc_rescan*/
	sdhci_s3c_force_presence_change(&s3c_device_hsmmc1);	
}
EXPORT_SYMBOL(wlan_setup_power);


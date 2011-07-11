/* linux/arch/arm/plat-s5p64xx/devs.c
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * Base S5P64XX resource and device definitions
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
#include <linux/platform_device.h>

#include <asm/mach/arch.h>
#include <asm/mach/irq.h>
#include <mach/hardware.h>
#include <mach/map.h>

#include <plat/devs.h>
#include <plat/adcts.h>
#include <linux/android_pmem.h>
#include <plat/reserved_mem.h>

/* Android Gadget */
#include <linux/usb/android_composite.h>
/* Default vendor and product IDs, overridden by platform data */
#define DRIVER_DESC		"Android Composite USB"
#define DRIVER_VERSION	__DATE__
/* if you want to use VTP function, please enable below Feature : VTP_MODE*/
//#define VTP_MODE

/* Default vendor and product IDs, overridden by platform data */
#define S3C_VENDOR_ID		0x04e8	/* Samsung */
//#define ADB_PRODUCT_ID 	0x6601	/* Swallowtail*/
//#define ADB_PRODUCT_ID 	0x681C	/* S3C6410 Swallowtail*/
#define S3C_KIES_PRODUCT_ID 	0x6877	/* S3C6410 Swallowtail*/
#define S3C_MTP_PRODUCT_ID 	0x68A9	/* S3C6410 Swallowtail*/

#define S3C_ADB_PRODUCT_ID 	0x681C	/* S3C6410 Swallowtail*/
#define S3C_PRODUCT_ID		0x681D
#define S3C_RNDIS_PRODUCT_ID	0x6881
#define S3C_RNDIS_VENDOR_NUM	0x0525	/* NetChip */
#define S3C_RNDIS_PRODUCT_NUM	0xa4a2	/* Ethernet/RNDIS Gadget */
#define S3C_CDC_VENDOR_NUM		0x0525	/* NetChip */
#define S3C_CDC_PRODUCT_NUM		0xa4a1	/* Linux-USB Ethernet Gadget */

#define MAX_USB_SERIAL_NUM	17

static char *usb_functions_ums[] = {
	"usb_mass_storage",
};

static char *usb_functions_rndis[] = {
	"rndis",
};

static char *usb_functions_rndis_adb[] = {
	"rndis",
	"adb",
};
static char *usb_functions_ums_adb[] = {
	"usb_mass_storage",
	"adb",
};

static char *usb_functions_all[] = {
#ifdef CONFIG_USB_ANDROID_RNDIS
	"rndis",
#endif
	"usb_mass_storage",
	"adb",
#ifdef CONFIG_USB_ANDROID_ACM
	"acm",
#endif
};
static struct android_usb_product usb_products[] = {
	{
		.product_id	= S3C_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_ums),
		.functions	= usb_functions_ums,
	},
	{
		.product_id	= S3C_ADB_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_ums_adb),
		.functions	= usb_functions_ums_adb,
	},
	/*
	{
		.product_id	= S3C_RNDIS_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis),
		.functions	= usb_functions_rndis,
	},
	{
		.product_id	= S3C_RNDIS_ADB_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis_adb),
		.functions	= usb_functions_rndis_adb,
	},
	*/
};
// serial number should be changed as real device for commercial release
static char device_serial[MAX_USB_SERIAL_NUM]="0123456789ABCDEF";
/* standard android USB platform data */

// Information should be changed as real product for commercial release
static struct android_usb_platform_data android_usb_pdata = {
	.vendor_id		= S3C_VENDOR_ID,
	.product_id		= S3C_PRODUCT_ID,
	.manufacturer_name	= "Android",//"Samsung",
	.product_name		= "Android",//"Samsung SMDKV210",
	.serial_number		= device_serial,
	.num_products 		= ARRAY_SIZE(usb_products),
	.products 		= usb_products,
	.num_functions 		= ARRAY_SIZE(usb_functions_all),
	.functions 		= usb_functions_all,
};

struct platform_device s3c_device_android_usb = {
	.name	= "android_usb",
	.id	= -1,
	.dev	= {
		.platform_data	= &android_usb_pdata,
	},
};
EXPORT_SYMBOL(s3c_device_android_usb);

static struct usb_mass_storage_platform_data ums_pdata = {
	.vendor			= "Android   ",//"Samsung",
	.product		= "UMS Composite",//"SMDKV210",
	.release		= 1,
};
struct platform_device s3c_device_usb_mass_storage= {
	.name	= "usb_mass_storage",
	.id	= -1,
	.dev	= {
		.platform_data = &ums_pdata,
	},
};
EXPORT_SYMBOL(s3c_device_usb_mass_storage);

/* SMC9115 LAN via ROM interface */

static struct resource s3c_smc911x_resources[] = {
      [0] = {
              .start  = S5P64XX_PA_SMC9115,
              .end    = S5P64XX_PA_SMC9115 + S5P64XX_SZ_SMC9115 - 1,
              .flags  = IORESOURCE_MEM,
      },
      [1] = {
              .start = IRQ_EINT11,
              .end   = IRQ_EINT11,
              .flags = IORESOURCE_IRQ,
        },
};

struct platform_device s3c_device_smc911x = {
      .name           = "smc911x",
      .id             =  -1,
      .num_resources  = ARRAY_SIZE(s3c_smc911x_resources),
      .resource       = s3c_smc911x_resources,
};

/* NAND Controller */

static struct resource s3c_nand_resource[] = {
	[0] = {
		.start = S5P64XX_PA_NAND,
		.end   = S5P64XX_PA_NAND + S5P64XX_SZ_NAND - 1,
		.flags = IORESOURCE_MEM,
	}
};

struct platform_device s3c_device_nand = {
	.name		  = "s3c-nand",
	.id		  = -1,
	.num_resources	  = ARRAY_SIZE(s3c_nand_resource),
	.resource	  = s3c_nand_resource,
};

EXPORT_SYMBOL(s3c_device_nand);

/* USB Device (Gadget)*/

static struct resource s3c_usbgadget_resource[] = {
	[0] = {
		.start = S3C_PA_OTG,
		.end   = S3C_PA_OTG + S3C_SZ_OTG - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_OTG,
		.end   = IRQ_OTG,
		.flags = IORESOURCE_IRQ,
	}
};

struct platform_device s3c_device_usbgadget = {
	.name		  = "s3c-usbgadget",
	.id		  = -1,
	.num_resources	  = ARRAY_SIZE(s3c_usbgadget_resource),
	.resource	  = s3c_usbgadget_resource,
};

EXPORT_SYMBOL(s3c_device_usbgadget);

/* USB Device (OTG hcd)*/

static struct resource s3c_usb_otghcd_resource[] = {
	[0] = {
		.start = S3C_PA_OTG,
		.end   = S3C_PA_OTG + S3C_SZ_OTG - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_OTG,
		.end   = IRQ_OTG,
		.flags = IORESOURCE_IRQ,
	}
};

static u64 s3c_device_usb_otghcd_dmamask = 0xffffffffUL;

struct platform_device s3c_device_usb_otghcd = {
	.name		= "s3c_otghcd",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_usb_otghcd_resource),
	.resource	= s3c_usb_otghcd_resource,
        .dev              = {
                .dma_mask = &s3c_device_usb_otghcd_dmamask,
                .coherent_dma_mask = 0xffffffffUL
        }
};

EXPORT_SYMBOL(s3c_device_usb_otghcd);

/* JPEG controller  */
static struct resource s3c_jpeg_resource[] = {
        [0] = {
                .start = S5P64XX_PA_JPEG,
                .end   = S5P64XX_PA_JPEG + S3C_SZ_JPEG - 1,
                .flags = IORESOURCE_MEM,
        },
        [1] = {
                .start = IRQ_JPEG,
                .end   = IRQ_JPEG,
                .flags = IORESOURCE_IRQ,
        }

};

struct platform_device s3c_device_jpeg = {
        .name             = "s3c-jpg",
        .id               = -1,
        .num_resources    = ARRAY_SIZE(s3c_jpeg_resource),
        .resource         = s3c_jpeg_resource,
};

EXPORT_SYMBOL(s3c_device_jpeg);

/* LCD Controller */

static struct resource s3c_lcd_resource[] = {
	[0] = {
		.start = S5P64XX_PA_LCD,
		.end   = S5P64XX_PA_LCD + SZ_1M - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_DISPCON1,
		.end   = IRQ_DISPCON2,
		.flags = IORESOURCE_IRQ,
	}
};

static u64 s3c_device_lcd_dmamask = 0xffffffffUL;

struct platform_device s3c_device_lcd = {
	.name		  = "s3c-lcd",
	.id		  = -1,
	.num_resources	  = ARRAY_SIZE(s3c_lcd_resource),
	.resource	  = s3c_lcd_resource,
	.dev              = {
		.dma_mask		= &s3c_device_lcd_dmamask,
		.coherent_dma_mask	= 0xffffffffUL
	}
};

/* MFC controller */
static struct resource s3c_mfc_resource[] = {
        [0] = {
                .start = S5P64XX_PA_MFC,
                //.end   = S5P64XX_PA_MFC + SZ_4K - 1,
                .end   = S5P64XX_PA_MFC + SZ_1M - 1,
                .flags = IORESOURCE_MEM,
                },
        [1] = {
                .start = IRQ_MFC,
                .end   = IRQ_MFC,
                .flags = IORESOURCE_IRQ,
        }
};
        
struct platform_device s3c_device_mfc = {
        .name             = "s3c-mfc",
        .id               = -1,
        .num_resources    = ARRAY_SIZE(s3c_mfc_resource),
        .resource         = s3c_mfc_resource
};

EXPORT_SYMBOL(s3c_device_mfc);

#ifdef CONFIG_S5P64XX_ADCTS
/* ADCTS */
static struct resource s3c_adcts_resource[] = {
	[0] = {
		.start = S3C_PA_ADC,
		.end   = S3C_PA_ADC + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_PENDN,
		.end   = IRQ_PENDN,
		.flags = IORESOURCE_IRQ,
	},
	[2] = {
		.start = IRQ_ADC,
		.end   = IRQ_ADC,
		.flags = IORESOURCE_IRQ,
	}

};

struct platform_device s3c_device_adcts = {
	.name		  = "s3c-adcts",
	.id		  = -1,
	.num_resources	  = ARRAY_SIZE(s3c_adcts_resource),
	.resource	  = s3c_adcts_resource,
};

void __init s3c_adcts_set_platdata(struct s3c_adcts_plat_info *pd)
{
	struct s3c_adcts_plat_info *npd;

	npd = kmalloc(sizeof(*npd), GFP_KERNEL);
	if (npd) {
		memcpy(npd, pd, sizeof(*npd));
		s3c_device_adcts.dev.platform_data = npd;
	} else {
		printk(KERN_ERR "no memory for ADC platform data\n");
	}
}
EXPORT_SYMBOL(s3c_device_adcts);

#else
/* ADC : Old ADC driver */
static struct resource s3c_adc_resource[] = {
	[0] = {
		.start = S3C_PA_ADC,
		.end   = S3C_PA_ADC + SZ_4K - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_PENDN,
		.end   = IRQ_PENDN,
		.flags = IORESOURCE_IRQ,
	},
	[2] = {
		.start = IRQ_ADC,
		.end   = IRQ_ADC,
		.flags = IORESOURCE_IRQ,
	}

};

struct platform_device s3c_device_adc = {
	.name		  = "s3c-adc",
	.id		  = -1,
	.num_resources	  = ARRAY_SIZE(s3c_adc_resource),
	.resource	  = s3c_adc_resource,
};

void __init s3c_adc_set_platdata(struct s3c_adc_mach_info *pd)
{
	struct s3c_adc_mach_info *npd;

	npd = kmalloc(sizeof(*npd), GFP_KERNEL);
	if (npd) {
		memcpy(npd, pd, sizeof(*npd));
		s3c_device_adc.dev.platform_data = npd;
	} else {
		printk(KERN_ERR "no memory for ADC platform data\n");
	}
}
EXPORT_SYMBOL(s3c_device_adc);

#endif

static struct resource s3c_rtc_resource[] = {
	[0] = {
		.start = S3C_PA_RTC,
		.end   = S3C_PA_RTC + 0xff,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_RTC_ALARM,
		.end   = IRQ_RTC_ALARM,
		.flags = IORESOURCE_IRQ,
	},
	[2] = {
		.start = IRQ_RTC_TIC,
		.end   = IRQ_RTC_TIC,
		.flags = IORESOURCE_IRQ
	}
};

struct platform_device s3c_device_rtc = {
	.name		  = "s3c2410-rtc",
	.id		  = -1,
	.num_resources	  = ARRAY_SIZE(s3c_rtc_resource),
	.resource	  = s3c_rtc_resource,
};

EXPORT_SYMBOL(s3c_device_rtc);

/* Watchdog */
static struct resource s3c_wdt_resource[] = {
	[0] = {
		.start = S5P64XX_PA_WATCHDOG,
		.end   = S5P64XX_PA_WATCHDOG + S5P64XX_SZ_WATCHDOG - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_WDT,
		.end   = IRQ_WDT,
		.flags = IORESOURCE_IRQ,
	}
};

struct platform_device s3c_device_wdt = {
	.name             = "s3c2410-wdt",
	.id               = -1,
	.num_resources    = ARRAY_SIZE(s3c_wdt_resource),
	.resource         = s3c_wdt_resource,
};

EXPORT_SYMBOL(s3c_device_wdt);

/* Keypad interface */
static struct resource s3c_keypad_resource[] = {
	[0] = {
		.start = S3C_PA_KEYPAD,
		.end   = S3C_PA_KEYPAD+ S3C_SZ_KEYPAD - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_KEYPAD,
		.end   = IRQ_KEYPAD,
		.flags = IORESOURCE_IRQ,
	}
};

struct platform_device s3c_device_keypad = {
	.name		  = "s3c-keypad",
	.id		  = -1,
	.num_resources	  = ARRAY_SIZE(s3c_keypad_resource),
	.resource	  = s3c_keypad_resource,
};

/* 3D interface */
static struct resource s3c_g3d_resource[] = {
  [0] = {
    .start = S3C64XX_PA_G3D,
    .end   = S3C64XX_PA_G3D + S3C64XX_SZ_G3D - 1,
    .flags = IORESOURCE_MEM,
  },
  [1] = {
    .start = IRQ_3D,
    .end   = IRQ_3D ,
    .flags = IORESOURCE_IRQ,
  }
};

struct platform_device s3c_device_g3d = {
  .name             = "s3c-g3d",
  .id               = -1,
  .num_resources    = ARRAY_SIZE(s3c_g3d_resource),
  .resource         = s3c_g3d_resource
};


EXPORT_SYMBOL(s3c_device_g3d);


/* 2D interface */
static struct resource s3c_g2d_resource[] = {
  [0] = {
    .start = S5P64XX_PA_G2D,
    .end   = S5P64XX_PA_G2D + S5P64XX_SZ_G2D - 1,
    .flags = IORESOURCE_MEM,
  },
  [1] = {
    .start = IRQ_2D,
    .end   = IRQ_2D ,
    .flags = IORESOURCE_IRQ,
  }
};

struct platform_device s3c_device_g2d = {
  .name             = "s3c-g2d",
  .id               = 0,
  .num_resources    = ARRAY_SIZE(s3c_g2d_resource),
  .resource         = s3c_g2d_resource
};

EXPORT_SYMBOL(s3c_device_g2d);

/* OneNAND Controller */
static struct resource s3c_onenand_resource[] = {
        [0] = {
                .start = S5P64XX_PA_ONENAND,
                .end   = S5P64XX_PA_ONENAND + S5P64XX_SZ_ONENAND - 1,
                .flags = IORESOURCE_MEM,
        }
};

struct platform_device s3c_device_onenand = {
        .name           = "onenand",
        .id             = -1,
        .num_resources  = ARRAY_SIZE(s3c_onenand_resource),
        .resource       = s3c_onenand_resource,
};

EXPORT_SYMBOL(s3c_device_onenand);



static struct android_pmem_platform_data pmem_pdata = {
	.name		= "pmem",
	.no_allocator	= 1,
	.cached		= 1,
};
 
static struct android_pmem_platform_data pmem_gpu1_pdata = {
	.name		= "pmem_gpu1",
	.no_allocator	= 1,
	.cached		= 1,
	.buffered	= 1,
};

static struct android_pmem_platform_data pmem_render_pdata = {
	.name		= "pmem_render",
	.no_allocator	= 1,
	.cached		= 0,
};

static struct android_pmem_platform_data pmem_stream_pdata = {
	.name		= "pmem_stream",
	.no_allocator	= 1,
	.cached		= 0,
};

static struct android_pmem_platform_data pmem_stream2_pdata = {
	.name		= "pmem_stream2",
	.no_allocator	= 1,
	.cached		= 0,
};

static struct android_pmem_platform_data pmem_preview_pdata = {
	.name		= "pmem_preview",
	.no_allocator	= 1,
	.cached		= 0,
};

static struct android_pmem_platform_data pmem_picture_pdata = {
	.name		= "pmem_picture",
	.no_allocator	= 1,
	.cached		= 0,
};

static struct android_pmem_platform_data pmem_jpeg_pdata = {
	.name		= "pmem_jpeg",
	.no_allocator	= 1,
	.cached		= 0,
};

static struct android_pmem_platform_data pmem_skia_pdata = {
	.name		= "pmem_skia",
	.no_allocator	= 1,
	.cached		= 0,
};
 
static struct platform_device pmem_device = {
	.name		= "android_pmem",
	.id		= 0,
	.dev		= { .platform_data = &pmem_pdata },
};
 
static struct platform_device pmem_gpu1_device = {
	.name		= "android_pmem",
	.id		= 1,
	.dev		= { .platform_data = &pmem_gpu1_pdata },
};

static struct platform_device pmem_render_device = {
	.name		= "android_pmem",
	.id		= 2,
	.dev		= { .platform_data = &pmem_render_pdata },
};

static struct platform_device pmem_stream_device = {
	.name		= "android_pmem",
	.id		= 3,
	.dev		= { .platform_data = &pmem_stream_pdata },
};

static struct platform_device pmem_stream2_device = {
	.name		= "android_pmem",
	.id		= 4,
	.dev		= { .platform_data = &pmem_stream2_pdata },
};

static struct platform_device pmem_preview_device = {
	.name		= "android_pmem",
	.id		= 5,
	.dev		= { .platform_data = &pmem_preview_pdata },
};

static struct platform_device pmem_picture_device = {
	.name		= "android_pmem",
	.id		= 6,
	.dev		= { .platform_data = &pmem_picture_pdata },
};

static struct platform_device pmem_jpeg_device = {
	.name		= "android_pmem",
	.id		= 7,
	.dev		= { .platform_data = &pmem_jpeg_pdata },
};

static struct platform_device pmem_skia_device = {
	.name		= "android_pmem",
	.id		= 8,
	.dev		= { .platform_data = &pmem_skia_pdata },
};

void __init s5p6442_add_mem_devices(struct s5p6442_pmem_setting *setting)
{
	if (setting->pmem_size) {
		pmem_pdata.start = setting->pmem_start;
		pmem_pdata.size = setting->pmem_size;
		platform_device_register(&pmem_device);
	}

	if (setting->pmem_gpu1_size) {
		pmem_gpu1_pdata.start = setting->pmem_gpu1_start;
		pmem_gpu1_pdata.size = setting->pmem_gpu1_size;
		platform_device_register(&pmem_gpu1_device);
	}
 
	if (setting->pmem_render_size) {
		pmem_render_pdata.start = setting->pmem_render_start;
		pmem_render_pdata.size = setting->pmem_render_size;
		platform_device_register(&pmem_render_device);
	}

	if (setting->pmem_stream_size) {
		pmem_stream_pdata.start = setting->pmem_stream_start;
	        pmem_stream_pdata.size = setting->pmem_stream_size;
	        platform_device_register(&pmem_stream_device);
	}

	if (setting->pmem_stream_size) {
		pmem_stream2_pdata.start = setting->pmem_stream_start;
		pmem_stream2_pdata.size = setting->pmem_stream_size;
		platform_device_register(&pmem_stream2_device);
	}

	if (setting->pmem_preview_size) {
		pmem_preview_pdata.start = setting->pmem_preview_start;
		pmem_preview_pdata.size = setting->pmem_preview_size;
		platform_device_register(&pmem_preview_device);
	}

	if (setting->pmem_picture_size) {
		pmem_picture_pdata.start = setting->pmem_picture_start;
		pmem_picture_pdata.size = setting->pmem_picture_size;
		platform_device_register(&pmem_picture_device);
	}

	if (setting->pmem_jpeg_size) {
		pmem_jpeg_pdata.start = setting->pmem_jpeg_start;
		pmem_jpeg_pdata.size = setting->pmem_jpeg_size;
		platform_device_register(&pmem_jpeg_device);
	}

	if (setting->pmem_skia_size) {
		pmem_skia_pdata.start = setting->pmem_skia_start;
		pmem_skia_pdata.size = setting->pmem_skia_size;
		platform_device_register(&pmem_skia_device);
	}
}

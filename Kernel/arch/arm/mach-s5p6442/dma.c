/* linux/arch/arm/mach-s5p6442/dma.c
 *
 * Copyright (c) 2003-2005,2006 Samsung Electronics
 *
 * S5P6442 DMA selection
 *
 * http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sysdev.h>
#include <linux/serial_core.h>

#include <asm/dma.h>
#include <mach/dma.h>
#include <asm/io.h>

#include <plat/dma.h>
#include <plat/cpu.h>


/* DMAC */
#define MAP0(x) { \
		[0]	= (x) | DMA_CH_VALID,	\
		[1]	= (x) | DMA_CH_VALID,	\
		[2]	= (x) | DMA_CH_VALID,	\
		[3]	= (x) | DMA_CH_VALID,	\
		[4]	= (x) | DMA_CH_VALID,	\
		[5]     = (x) | DMA_CH_VALID,	\
		[6]	= (x) | DMA_CH_VALID,	\
		[7]     = (x) | DMA_CH_VALID,	\
		[8]	= (x),	\
		[9]	= (x),	\
		[10]	= (x),	\
		[11]	= (x),	\
		[12]	= (x),	\
		[13]    = (x),	\
		[14]	= (x),	\
		[15]    = (x),	\
	}

/* Peri-DMAC 0 */
#define MAP1(x) { \
		[0]	= (x),	\
		[1]	= (x),	\
		[2]	= (x),	\
		[3]	= (x),	\
		[4]	= (x),	\
		[5]     = (x),	\
		[6]	= (x),	\
		[7]     = (x),	\
		[8]	= (x) | DMA_CH_VALID,	\
		[9]	= (x) | DMA_CH_VALID,	\
		[10]	= (x) | DMA_CH_VALID,	\
		[11]	= (x) | DMA_CH_VALID,	\
		[12]	= (x) | DMA_CH_VALID,	\
		[13]    = (x) | DMA_CH_VALID,	\
		[14]	= (x) | DMA_CH_VALID,	\
		[15]    = (x) | DMA_CH_VALID,	\
	}


/* DMA request sources  */
#define S3C_DMA0_UART0CH0	0
#define S3C_DMA0_UART0CH1	1
#define S3C_DMA0_UART1CH0	2
#define S3C_DMA0_UART1CH1	3
#define S3C_DMA0_UART2CH0	4
#define S3C_DMA0_UART2CH1	5
#define S3C_DMA0_UART3CH0	6
#define S3C_DMA0_UART3CH1	7
#define S3C_DMA0_PCM0_TX	12
#define S3C_DMA0_PCM0_RX	11
#define S3C_DMA0_I2S0_TX	10
#define S3C_DMA0_I2S0_RX	13
#define S3C_DMA0_SPI0_TX	14
#define S3C_DMA0_SPI0_RX	15
#define S3C_DMA0_SPI1_TX	20
#define S3C_DMA0_SPI1_RX	21
#define S3C_DMA0_GPS		24
#define S3C_DMA0_PWM		29
#define S3C_DMA0_EXTERNAL	31

/* DMA request sources  */
#define S3C_DMA1_I2S0_RX	9	
#define S3C_DMA1_I2S0_TX	10
#define S3C_DMA1_SPI0_RX	16	
#define S3C_DMA1_SPI0_TX	17	

#define S3C_DMA_M2M		0


static struct s3c_dma_map __initdata s5p6442_dma_mappings[] = {

	[DMACH_SPI0_IN] = {
		.name		= "spi0-in",
		.channels	= MAP1(S3C_DMA1_SPI0_RX),
		.hw_addr.from	= S3C_DMA1_SPI0_RX,
	},
	[DMACH_SPI0_OUT] = {
		.name		= "spi0-out",
		.channels	= MAP1(S3C_DMA1_SPI0_RX),
		.hw_addr.to	= S3C_DMA1_SPI0_RX,
	},
        [DMACH_I2S_V50_IN] = {           
		.name           = "i2s-v50-in", 
		.channels       = MAP1(S3C_DMA1_I2S0_RX),
		.hw_addr.from   = S3C_DMA1_I2S0_RX,     
		.sdma_sel       = 1 << S3C_DMA1_I2S0_RX,
	},
	[DMACH_I2S_V50_OUT] = {            
		.name           = "i2s-v50-out", 
		.channels       = MAP1(S3C_DMA1_I2S0_TX), 
		.hw_addr.to     = S3C_DMA1_I2S0_TX,      
		.sdma_sel       = 1 << S3C_DMA1_I2S0_TX,
	}, 

};

static void s5p6442_dma_select(struct s3c2410_dma_chan *chan,
			       struct s3c_dma_map *map)
{
	chan->map = map;
}

static struct s3c_dma_selection __initdata s5p6442_dma_sel = {
	.select		= s5p6442_dma_select,
	.dcon_mask	= 0,
	.map		= s5p6442_dma_mappings,
	.map_size	= ARRAY_SIZE(s5p6442_dma_mappings),
};

static int __init s5p6442_dma_add(struct sys_device *sysdev)
{
	s3c_dma_init(S3C_DMA_CHANNELS, IRQ_MDMA, 0x1000);
	return s3c_dma_init_map(&s5p6442_dma_sel);
}

static struct sysdev_driver s5p6442_dma_driver = {
	.add	= s5p6442_dma_add,
};

static int __init s5p6442_dma_init(void)
{
	return sysdev_driver_register(&s5p6442_sysclass, &s5p6442_dma_driver);
}

arch_initcall(s5p6442_dma_init);



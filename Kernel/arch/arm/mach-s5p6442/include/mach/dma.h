/* linux/arch/arm/mach-s5p6442/include/mach/dma.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *      Ben Dooks <ben@simtec.co.uk>
 *      http://armlinux.simtec.co.uk/
 *
 * S5P6442 - DMA support
 */

#ifndef __ASM_ARCH_DMA_H
#define __ASM_ARCH_DMA_H __FILE__

#include <mach/s3c-dma.h>

#define S3C_DMA_CONTROLLERS        	(2)
#define S3C_CHANNELS_PER_DMA       	(8)
#define S3C_CANDIDATE_CHANNELS_PER_DMA  (32)
#define S3C_DMA_CHANNELS		(S3C_DMA_CONTROLLERS*S3C_CHANNELS_PER_DMA)

#endif /* __ASM_ARCH_IRQ_H */

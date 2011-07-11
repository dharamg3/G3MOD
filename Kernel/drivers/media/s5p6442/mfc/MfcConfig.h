/*
 *  drivers/media/s5p6442/mfc/MfcConfig.h
 *
 *  Copyright 2010 Samsung Electronics Co.Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#ifndef __SAMSUNG_SYSLSI_APDEV_MFC_CONFIG_H__
#define __SAMSUNG_SYSLSI_APDEV_MFC_CONFIG_H__


#include <linux/version.h>
#include <asm/memory.h>
#include <mach/memory.h>
#include <mach/hardware.h>
#include <plat/reserved_mem.h>


//#define ZERO_COPY

#ifdef ZERO_COPY
#define ZERO_COPY_HDR_SIZE 32
#else
#define ZERO_COPY_HDR_SIZE 0
#endif

// Physical Base Address for the MFC Host I/F Registers
#define S3C6400_BASEADDR_MFC_SFR			0xF1000000 //0x7e002000


// Physical Base Address for the MFC Shared Buffer
//   Shared Buffer = {CODE_BUF, WORK_BUF, PARA_BUF}
#define S3C6400_BASEADDR_MFC_BITPROC_BUF	MFC_RESERVED_MEM_START


// Physical Base Address for the MFC Data Buffer
//   Data Buffer = {STRM_BUF, FRME_BUF}
#define S3C6400_BASEADDR_MFC_DATA_BUF		(MFC_RESERVED_MEM_START + 0x116000)


// Physical Base Address for the MFC Host I/F Registers
#define S3C6400_BASEADDR_POST_SFR			0x77000000


/////////////////////////////////
/////						/////
/////	MFC BITPROC_BUF		/////
/////						/////
/////////////////////////////////

//the following three buffers have fixed size
//firware buffer is to download boot code and firmware code
#define MFC_CODE_BUF_SIZE	81920	// It is fixed depending on the MFC'S FIRMWARE CODE (refer to 'Prism_S_V133.h' file)

//working buffer is uded for video codec operations by MFC
#define MFC_WORK_BUF_SIZE	1048576 // 1024KB = 1024 * 1024


//Parameter buffer is allocated to store yuv frame address of output frame buffer.
#define MFC_PARA_BUF_SIZE	8192  //Stores the base address of Y , Cb , Cr for each decoded frame

#define MFC_BITPROC_BUF_SIZE		\
							(	MFC_CODE_BUF_SIZE + \
								MFC_PARA_BUF_SIZE    + \
								MFC_WORK_BUF_SIZE	)


/////////////////////////////////
/////						/////
/////	MFC DATA_BUF		/////
/////						/////
/////////////////////////////////

// RainAde : for test 6442 spec.
#if 1
#define MAX_WIDTH	832	// 832 = 800 + 16*2 (DivX Padding size = 16)
#define MAX_HEIGHT	512	// 512 = 480 + 16*2 (DivX Padding size = 16)
#else
#define MAX_WIDTH	720
#define MAX_HEIGHT	480
#endif

// MFC Driver supports 4 instances MAX.
// yj : 4 -> 3 (It's because Linbuf size is changed from 200K to 300K for DMC request)
// RainAde : 3->2 (It's because Linebuf size is changed from 300K to 500K for 864x600
// RainAde : for test 6442 spec.
#if 1
#define MFC_NUM_INSTANCES_MAX	2
#else
#define MFC_NUM_INSTANCES_MAX	3
#endif


// Determine if 'Post Rotate Mode' is enabled.
// If it is enabled, the memory size of SD YUV420(720x576x1.5 bytes) is required more.
// In case of linux driver, reserved buffer size will be changed.

//#define MFC_ROTATE_ENABLE		1
#define MFC_ROTATE_ENABLE		0

#define MFC_LINE_RING_SHARE		1	// Determine if the LINE_BUF is shared with RING_BUF

/*
 * stream buffer size must be a multiple of 512bytes 
 * becasue minimun data transfer unit between stream buffer and internal bitstream handling block 
 * in MFC core is 512bytes
 */
// RainAde : for test 6442 spec.
#if 1
#define MFC_LINE_BUF_SIZE_PER_INSTANCE		(512000)
#else
#define MFC_LINE_BUF_SIZE_PER_INSTANCE		(307200)
#endif

#define MFC_LINE_BUF_SIZE		(MFC_NUM_INSTANCES_MAX * MFC_LINE_BUF_SIZE_PER_INSTANCE)


// RainAde : for test 6442 spec.
#if 1
//#define MFC_FRAM_BUF_SIZE		(MAX_WIDTH*MAX_HEIGHT*3*3)	// reference buffer count (usually, BP has 4 ref frame)
#define MFC_FRAM_BUF_SIZE		(MAX_WIDTH*MAX_HEIGHT*3*9)	// reference buffer count 16 + current buffer 1 + display buffer 2 = 18 buffers
#else
#define MFC_FRAM_BUF_SIZE		(MAX_WIDTH*MAX_HEIGHT*3*4)
#endif


// RainAde : usually, ring buf take 3times of linebuf
#if 1
#define MFC_RING_BUF_SIZE		(MFC_LINE_BUF_SIZE_PER_INSTANCE  *  3)		// 768,000 Bytes
#else
#define MFC_RING_BUF_SIZE		((307200)  *  3)		// 768,000 Bytes
#endif

#define MFC_RING_BUF_PARTUNIT_SIZE	(MFC_RING_BUF_SIZE / 3)	// RING_BUF consists of 3 PARTs.


#if (MFC_LINE_RING_SHARE == 1)

#define MFC_STRM_BUF_SIZE			(MFC_LINE_BUF_SIZE)

#elif (MFC_LINE_RING_SHARE == 0)

#define MFC_STRM_BUF_SIZE			(MFC_LINE_BUF_SIZE + MFC_RING_BUF_SIZE)

#endif


/*
 * If Mp4DbkOn is enabled, the memory size of DATA_BUF is required more.
 * 
 * 2009.5.12 by yj (yunji.kim@samsung.com)
 */
#define MFC_DBK_BUF_SIZE		((176*144*3) >> 1)
#define MFC_DATA_BUF_SIZE		(MFC_STRM_BUF_SIZE + MFC_FRAM_BUF_SIZE + MFC_DBK_BUF_SIZE)	


#if (MFC_RING_BUF_PARTUNIT_SIZE & 0x1FF)
#error "MFC_RING_BUF_PARTUNIT_SIZE value must be 512-byte aligned."
#endif

#endif /* __SAMSUNG_SYSLSI_APDEV_MFC_CONFIG_H__ */

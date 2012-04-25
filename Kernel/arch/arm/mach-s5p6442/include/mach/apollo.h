/*
 *  arch/arm/mach-s5p6442/include/mach/apollo.h
 *
 *  Author:		Samsung Electronics
 *  Created:	19, Nov, 2009
 *  Copyright:	Samsung Electronics Co.Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#ifndef MACH_APOLLO_H

#define MACH_APOLLO_H

#define CONFIG_APOLLO_REV00	0x00
#define CONFIG_APOLLO_REV01	0x01
#define CONFIG_APOLLO_REV02	0x02
#define CONFIG_APOLLO_REV03	0x03
#define CONFIG_APOLLO_REV04	0x04
#define CONFIG_APOLLO_REV05	0x05
#define CONFIG_APOLLO_REV06	0x06
#define CONFIG_APOLLO_REV07	0x07
#define CONFIG_APOLLO_REV08	0x08
#define CONFIG_APOLLO_REV09	0x09

#define CONFIG_APOLLO_EMUL	0xFF

#if defined (CONFIG_BOARD_REVISION )
#if ( CONFIG_BOARD_REVISION == CONFIG_APOLLO_EMUL)
#include "apollo_EMUL.h"
#elif ( CONFIG_BOARD_REVISION == CONFIG_APOLLO_REV00)
#include "apollo_REV00.h"
#elif ( CONFIG_BOARD_REVISION == CONFIG_APOLLO_REV01)
#include "apollo_REV01.h"
#elif ( CONFIG_BOARD_REVISION == CONFIG_APOLLO_REV02)
#include "apollo_REV02.h"
#elif ( CONFIG_BOARD_REVISION == CONFIG_APOLLO_REV03)
#include "apollo_REV03.h"
#elif ( CONFIG_BOARD_REVISION == CONFIG_APOLLO_REV04)
#include "apollo_REV04.h"
#elif ( CONFIG_BOARD_REVISION == CONFIG_APOLLO_REV05)
#include "apollo_REV05.h"
#elif ( CONFIG_BOARD_REVISION == CONFIG_APOLLO_REV06)
#include "apollo_REV06.h"
#elif ( CONFIG_BOARD_REVISION == CONFIG_APOLLO_REV07)
#include "apollo_REV07.h"
#endif
#else
// If board revision is not defined, use default EMUL board header
#include "apollo_EMUL.h"
#endif

/*
 * Partition Information
 */

#define BOOT_PART_ID			0x0
//#define CSA_PART_ID	/* is not set */
#define SBL_PART_ID				0x1
#define PARAM_PART_ID			0x2
#define TEMP_PART_ID			0x3		/* Temp Area for FOTA */
#define KERNEL_PART_ID			0x4
#define MODEM_IMG_PART_ID		0x5
//#define RAMDISK_PART_ID	/* is not set */
#define FILESYSTEM_PART_ID		0x6
#define FILESYSTEM1_PART_ID	0x7
#define FILESYSTEM2_PART_ID	0x8
#define MODEM_EFS_PART_ID		0x9		/* Modem EFS Area for OneDRAM */

#endif	/* MACH_APOLLO_H */


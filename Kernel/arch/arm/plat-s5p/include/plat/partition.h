/* linux/arch/arm/plat-s3c/include/plat/partition.h
 *
 * Copyright (c) 2008 Samsung Electronics
 *
 * Partition information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <asm/mach/flash.h>
#if 0
struct mtd_partition s3c_partition_info[] = {
        {
                .name		= "Bootloader",
                .offset		= 0,
                .size		= (512*SZ_1K),
                .mask_flags	= MTD_CAP_NANDFLASH,
        },
        {
                .name		= "Kernel",
                .offset		= (512*SZ_1K),
                .size		= (4*SZ_1M) - (512*SZ_1K),
                .mask_flags	= MTD_CAP_NANDFLASH,
        },
#if defined(CONFIG_SPLIT_ROOT_FILESYSTEM)
        {
                .name		= "Rootfs",
                .offset		= (4*SZ_1M),
                .size		= (48*SZ_1M),
        },
#endif
        {
                .name		= "File System",
                .offset		= MTDPART_OFS_APPEND,
                .size		= MTDPART_SIZ_FULL,
        }
};

struct s3c_nand_mtd_info s3c_nand_mtd_part_info = {
	.chip_nr = 1,
	.mtd_part_nr = ARRAY_SIZE(s3c_partition_info),
	.partition = s3c_partition_info,
};

struct flash_platform_data s3c_onenand_data = {
	.parts		= s3c_partition_info,
	.nr_parts	= ARRAY_SIZE(s3c_partition_info),
};
#endif

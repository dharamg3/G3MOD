#ifndef _ASM_ARM_ARCH_RESERVED_MEM_H
#define _ASM_ARM_ARCH_RESERVED_MEM_H

#include <linux/types.h>
#include <linux/list.h>
#include <asm/setup.h>

#define ONEDRAM_END_ADDR 			(PHYS_OFFSET_ONEDRAM + PHYS_SIZE_ONEDRAM)


#define RESERVED_MEM_FIMC0		(8 * 1024 * 1024) //Camera-1 //6MB is enough for Android Camera, but Samsung camera needs 8MB. 
#define RESERVED_MEM_FIMC1		(0 * 1024 * 1024) //PP  //Reserved Mem is not required when the FIMC is in Output mode. 
#define RESERVED_MEM_FIMC2		(0 * 1024 * 1024) // Plz don't use FIMC2 in input mode. if u want to use it in input-mode...plz allocate memory for it.  
#define RESERVED_MEM_JPEG		(0)
#define RESERVED_MEM_CMM        (0 * 1024 * 1024) 
#define RESERVED_MEM_MFC		(14 * 1024 * 1024)
#define RESERVED_PMEM_JPEG      (3 * 1024 * 1024)
#define RESERVED_PMEM_RENDER	(2 * 1024 * 1024)
#define RESERVED_PMEM_STREAM    (4 * 1024 * 1024)
#define RESERVED_PMEM_PICTURE	(1 * 1024 * 1024)
#define RESERVED_PMEM_SKIA      (0)
#define RESERVED_MEM_G3D 		(40 * 1024 * 1024)
#define RESERVED_MEM_G2D		(0 * 1024 * 1024) 
#define RESERVED_PMEM_GPU1		(0 * 1024 * 1024)
#define RESERVED_PMEM			(8 * 1024 * 1024)

#define RESERVED_PMEM_PREVIEW    (2 * 1024 * 1024) // Shared with FIMC0

#if defined(CONFIG_RESERVED_MEM_CMM_JPEG_MFC_POST_CAMERA)

#define FIMC0_RESERVED_MEM_START	(ONEDRAM_END_ADDR - RESERVED_MEM_FIMC0)
#define FIMC1_RESERVED_MEM_START    (FIMC0_RESERVED_MEM_START - RESERVED_MEM_FIMC1)
#define FIMC2_RESERVED_MEM_START	(FIMC1_RESERVED_MEM_START - RESERVED_MEM_FIMC2)
#define CMM_RESERVED_MEM_START		(FIMC2_RESERVED_MEM_START - RESERVED_MEM_CMM)
#define MFC_RESERVED_MEM_START		(CMM_RESERVED_MEM_START - RESERVED_MEM_MFC)
#define JPEG_RESERVED_PMEM_START    (MFC_RESERVED_MEM_START - RESERVED_PMEM_JPEG)
#define RENDER_RESERVED_PMEM_START  (JPEG_RESERVED_PMEM_START - RESERVED_PMEM_RENDER)
#define STREAM_RESERVED_PMEM_START  (RENDER_RESERVED_PMEM_START - RESERVED_PMEM_STREAM)
#define PICTURE_RESERVED_PMEM_START (STREAM_RESERVED_PMEM_START - RESERVED_PMEM_PICTURE)
#define SKIA_RESERVED_PMEM_START    (PICTURE_RESERVED_PMEM_START - RESERVED_PMEM_SKIA)
#define G3D_RESERVED_MEM_START      (SKIA_RESERVED_PMEM_START - RESERVED_MEM_G3D)
#define G2D_RESERVED_MEM_START      (G3D_RESERVED_MEM_START - RESERVED_MEM_G2D)
#define GPU1_RESERVED_PMEM_START    (G2D_RESERVED_MEM_START - RESERVED_PMEM_GPU1)
#define RESERVED_PMEM_START		    (GPU1_RESERVED_PMEM_START - RESERVED_PMEM)
#define PHYS_UNRESERVED_SIZE        (RESERVED_PMEM_START - PHYS_OFFSET_ONEDRAM)

/* for shared memory */
#define PREVIEW_RESERVED_PMEM_START     (FIMC0_RESERVED_MEM_START)

#if (PHYS_UNRESERVED_SIZE > PHYS_SIZE_ONEDRAM)
#error PHYS_UNRESERVED_SIZE shold be less then PHYS_SIZE_ONEDRAM
#endif 

#else
#define PHYS_UNRESERVED_SIZE		(ONEDRAM_END_ADDR - PHYS_OFFSET_ONEDRAM)

#endif 

struct s5p6442_pmem_setting{
        resource_size_t pmem_start;
        resource_size_t pmem_size;
        resource_size_t pmem_gpu1_start;
        resource_size_t pmem_gpu1_size;
        resource_size_t pmem_render_start;
        resource_size_t pmem_render_size;
        resource_size_t pmem_stream_start;
        resource_size_t pmem_stream_size;
        resource_size_t pmem_preview_start;
        resource_size_t pmem_preview_size;
        resource_size_t pmem_picture_start;
        resource_size_t pmem_picture_size;
        resource_size_t pmem_jpeg_start;
        resource_size_t pmem_jpeg_size;
        resource_size_t pmem_skia_start;
        resource_size_t pmem_skia_size;
};
 
void s5p6442_add_mem_devices (struct s5p6442_pmem_setting *setting);

#endif /* _ASM_ARM_ARCH_RESERVED_MEM_H */


/*
 *  drivers/media/s5p6442/mfc/MfcMemory.h
 *
 *  Copyright 2010 Samsung Electronics Co.Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#ifndef __SAMSUNG_SYSLSI_APDEV_MFC_MEMORY_H__
#define __SAMSUNG_SYSLSI_APDEV_MFC_MEMORY_H__


#ifdef __cplusplus
extern "C" {
#endif


void* Phy2Vir_AddrMapping(unsigned int phy_addr, int mem_size);
void* Mem_Alloc(unsigned int size);
void Mem_Free(void *addr);
void* Mem_Cpy(void *dst, const void *src, int size);
void* Mem_Set(void *target, int val, int size);
int Copy_From_User(void *to, const void *from, unsigned long n);
int Copy_To_User(void *to, const void *from, unsigned long n);


#ifdef __cplusplus
}
#endif

#endif /* __SAMSUNG_SYSLSI_APDEV_MFC_MEMORY_H__ */

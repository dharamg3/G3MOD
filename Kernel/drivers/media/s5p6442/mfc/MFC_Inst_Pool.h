/*
 *  drivers/media/s5p6442/mfc/MFC_Inst_Pool.h
 *
 *  Copyright 2010 Samsung Electronics Co.Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#ifndef __SAMSUNG_SYSLSI_APDEV_MFC_INST_POOL_H__
#define __SAMSUNG_SYSLSI_APDEV_MFC_INST_POOL_H__


#ifdef __cplusplus
extern "C" {
#endif


int MfcInstPool_NumAvail(void);

int MfcInstPool_Occupy(void);
int MfcInstPool_Release(int instance_no);

void MfcInstPool_OccupyAll(void);
void MfcInstPool_ReleaseAll(void);

#ifdef __cplusplus
}
#endif

#endif /* __SAMSUNG_SYSLSI_APDEV_MFC_INST_POOL_H__ */

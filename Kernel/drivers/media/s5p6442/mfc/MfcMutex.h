/*
 *  drivers/media/s5p6442/mfc/MfcMutex.h
 *
 *  Copyright 2010 Samsung Electronics Co.Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#ifndef __SAMSUNG_SYSLSI_APDEV_MFC_MUTEX_H__
#define __SAMSUNG_SYSLSI_APDEV_MFC_MUTEX_H__


#include "MfcTypes.h"


#ifdef __cplusplus
extern "C" {
#endif


BOOL MFC_Mutex_Create(void);
void MFC_Mutex_Delete(void);
BOOL MFC_Mutex_Lock(void);
BOOL MFC_Mutex_Release(void);


#ifdef __cplusplus
}
#endif

#endif /* __SAMSUNG_SYSLSI_APDEV_MFC_MUTEX_H__ */

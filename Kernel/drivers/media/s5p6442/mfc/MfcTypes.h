/*
 *  drivers/media/s5p6442/mfc/MfcTypes.h
 *
 *  Copyright 2010 Samsung Electronics Co.Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#ifndef __SAMSUNG_SYSLSI_APDEV_MFC_TYPE_H__
#define __SAMSUNG_SYSLSI_APDEV_MFC_TYPE_H__

#ifdef __cplusplus
extern "C" {
#endif


#ifdef _WIN32_WCE
#include <windows.h>

#else

#include <linux/types.h>

typedef enum {FALSE, TRUE} BOOL;

#endif


#ifdef __cplusplus
}
#endif

#endif /* __SAMSUNG_SYSLSI_APDEV_MFC_TYPE_H__ */

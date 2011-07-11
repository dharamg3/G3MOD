/*
 *  drivers/media/s5p6442/mfc/BitProcBuf.h
 *
 *  Copyright 2010 Samsung Electronics Co.Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#ifndef __SAMSUNG_SYSLSI_APDEV_BIT_PROC_BUF_H__
#define __SAMSUNG_SYSLSI_APDEV_BIT_PROC_BUF_H__

#include "MfcTypes.h"

#ifdef __cplusplus
extern "C" {
#endif


BOOL MfcBitProcBufMemMapping(void);
volatile unsigned char* GetBitProcBufVirAddr(void);
unsigned char* GetParamBufVirAddr(void);
void MfcFirmwareIntoCodeBuf(void);


#ifdef __cplusplus
}
#endif

#endif /* __SAMSUNG_SYSLSI_APDEV_BIT_PROC_BUF_H__ */

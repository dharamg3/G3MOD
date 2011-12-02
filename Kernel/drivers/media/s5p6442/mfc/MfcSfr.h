/*
 *  drivers/media/s5p6442/mfc/MfcSfr.h
 *
 *  Copyright 2010 Samsung Electronics Co.Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#ifndef __SAMSUNG_SYSLSI_APDEV_MFC_SFR_H__
#define __SAMSUNG_SYSLSI_APDEV_MFC_SFR_H__

#include "MfcTypes.h"
#include "Mfc.h"

#ifdef __cplusplus
extern "C" {
#endif

int MFC_Sleep(void);
int MFC_Wakeup(void);
BOOL MfcIssueCmd(int inst_no, MFC_CODECMODE codec_mode, MFC_COMMAND mfc_cmd);
int GetFirmwareVersion(void);
BOOL MfcSfrMemMapping(void);
volatile S3C6400_MFC_SFR* GetMfcSfrVirAddr(void);
void* MfcGetCmdParamRegion(void);
	
void MfcReset(void);
void MfcClearIntr(void);
unsigned int MfcIntrReason(void);
void MfcSetEos(int buffer_mode);
void MfcStreamEnd(void);
void MfcFirmwareIntoCodeDownReg(void);
void MfcStartBitProcessor(void);
void MfcStopBitProcessor(void);
void MfcConfigSFR_BITPROC_BUF(void);
void MfcConfigSFR_CTRL_OPTS(void);

#ifdef __cplusplus
}
#endif

#endif /* __SAMSUNG_SYSLSI_APDEV_MFC_SFR_H__ */

/*
 *  drivers/media/s5p6442/mfc/FramBufMgr.h
 *
 *  Copyright 2010 Samsung Electronics Co.Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#ifndef __SAMSUNG_SYSLSI_APDEV_FRAM_BUF_MGR_H__
#define __SAMSUNG_SYSLSI_APDEV_FRAM_BUF_MGR_H__


#include "MfcTypes.h"


#ifdef __cplusplus
extern "C" {
#endif


BOOL FramBufMgrInit(unsigned char *pBufBase, int nBufSize);
void FramBufMgrFinal(void);
unsigned char* FramBufMgrCommit(int idx_commit, int commit_size);
void FramBufMgrFree(int idx_commit);
unsigned char* FramBufMgrGetBuf(int idx_commit);
int FramBufMgrGetBufSize(int idx_commit);
void FramBufMgrPrintCommitInfo(void);


#ifdef __cplusplus
}
#endif

#endif /* __SAMSUNG_SYSLSI_APDEV_FRAM_BUF_MGR_H__ */

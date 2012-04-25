/*
 *  drivers/media/s5p6442/mfc/MfcIntrNotification.h
 *
 *  Copyright 2010 Samsung Electronics Co.Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#ifndef __SAMSUNG_SYSLSI_APDEV_MFC_INTR_NOTIFICATION_H__
#define __SAMSUNG_SYSLSI_APDEV_MFC_INTR_NOTIFICATION_H__


#ifdef __cplusplus
extern "C" {
#endif


int  SendInterruptNotification(int intr_type);
int  WaitInterruptNotification(void);


#ifdef __cplusplus
}
#endif


#define WAIT_INT_NOTI_TIMEOUT		(-99)


#endif /* __SAMSUNG_SYSLSI_APDEV_MFC_INTR_NOTIFICATION_H__ */

/*
 *  drivers/media/s5p6442/mfc/MfcIntrNotification.c
 *
 *  Copyright 2010 Samsung Electronics Co.Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */
#include <linux/sched.h>
#include <linux/wait.h>
#include "s3c-mfc.h"
#include "MfcIntrNotification.h"
#include "MfcSfr.h"


extern wait_queue_head_t	WaitQueue_MFC;
static unsigned int  		gIntrType = 0;

int SendInterruptNotification(int intr_type)
{
	gIntrType = intr_type;
	wake_up_interruptible(&WaitQueue_MFC);
	
	return 0;
}

int WaitInterruptNotification(void)
{
	if(interruptible_sleep_on_timeout(&WaitQueue_MFC, 500) == 0)
	{
		MfcStreamEnd();
		return WAIT_INT_NOTI_TIMEOUT; 
	}
	
	return gIntrType;
}

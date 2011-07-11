/*
 *  drivers/media/s5p6442/mfc/MfcMutex.c
 *
 *  Copyright 2010 Samsung Electronics Co.Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/wait.h>

#include "MfcConfig.h"
#include "MfcMutex.h"
#include "MfcTypes.h"

extern wait_queue_head_t	WaitQueue_MFC;
static struct mutex	*hMutex = NULL;


BOOL MFC_Mutex_Create(void)
{
	hMutex = (struct mutex *)kmalloc(sizeof(struct mutex), GFP_KERNEL);
	if (hMutex == NULL)
		return FALSE;

	mutex_init(hMutex);

	return TRUE;
}

void MFC_Mutex_Delete(void)
{
	if (hMutex == NULL)
		return;

	mutex_destroy(hMutex);
}

BOOL MFC_Mutex_Lock(void)
{
	mutex_lock(hMutex);

	return TRUE;
}

BOOL MFC_Mutex_Release(void)
{
	mutex_unlock(hMutex);

	return TRUE;
}


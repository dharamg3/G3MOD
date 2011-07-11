/*
 *  drivers/media/s5p6442/mfc/LogMsg.c
 *
 *  Copyright 2010 Samsung Electronics Co.Ltd.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include <stdarg.h>

#include <linux/string.h>
#include <linux/kernel.h>
#include <asm/param.h>
#include <linux/delay.h>

#include "LogMsg.h"

//#define DEBUG

static const LOG_LEVEL	log_level = LOG_ERROR;
static const char	*modulename = "MFC_DRV";
static const char	*level_str[] = {"TRACE", "WARNING", "ERROR"};

void LOG_MSG(LOG_LEVEL level, const char *func_name, const char *msg, ...)
{
	char	buf[256];
	va_list	argptr;

#ifndef DEBUG
	if (level < log_level)
		return;
#endif // DEBUG

	sprintf(buf, "[%s: %s] %s: ", modulename, level_str[level], func_name);

	va_start(argptr, msg);
	vsprintf(buf + strlen(buf), msg, argptr);

	printk(buf);
		
	va_end(argptr);
}

void Sleep(unsigned int ms)
{
	udelay(ms);
}

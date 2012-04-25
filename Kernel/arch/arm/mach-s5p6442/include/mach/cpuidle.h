/* arch/arm/mach-s5p64420/include/mach/cpuidle.h
 *
 * Copyright 2010 Samsung Electronics
 *	Jaecheol Lee <jc.lee@samsung>
 *
 * S5P6442 - CPUIDLE support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define NORMAL_MODE	0
#define LPAUDIO_MODE	1

extern int previous_idle_mode;
extern int idle2_lock_count;
extern int s5p6442_setup_lpaudio(unsigned int mode);
extern void s5p6442_set_lpaudio_lock(int flag);
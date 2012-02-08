/* /arch/arm/plat-s5p6442/include/plat/s5P6442-dvfs.h
 *
 * Copyright (c) 2009 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

int set_voltage(unsigned int freq_index, bool);
int s5p6442_clk_set_rate(unsigned int target_freq, unsigned int index );

#ifndef __PLAT_S5P6442_DVFS_H
#define __PLAT_S5P6442_DVFS_H __FILE__
#include <linux/cpufreq.h>

extern int *FakeShmoo_UV_mV_Ptr; // Stored voltage table from cpufreq sysfs

extern unsigned int s5p6442_cpufreq_index;

#define MAXIMUM_FREQ 1600000
//#undef USE_DVS
#define USE_DVS


extern struct cpufreq_frequency_table freq_tab[];
extern u32 s5p_cpu_clk_tab[][10];


extern void s5p6442_preclock(void);
extern void s5p6442_postclock(void);

#define KHZ_T		1000

#define MPU_CLK		"clk_cpu"
#define INDX_ERROR  65535


//extern int set_voltage(unsigned int, bool);
extern unsigned int s5p6442_target_frq(unsigned int pred_freq, int flag);
extern void set_dvfs_level(int flag);
extern void set_dvfs_perf_level(void);
extern void set_dvfs_doclk_level(int flag);
extern void s5p6442_changeDivider(u32, u32);


extern unsigned int frequency_match[][4];
extern void s5p6442_clock800(void);


extern int cpufreq_set_policy(unsigned int cpu, const char *buf);
extern char cpufreq_governor_name[CPUFREQ_NAME_LEN];
extern void cpufreq_get_cpufreq_name(unsigned int cpu);
extern void dvfs_set_max_freq_lock(void);
extern void dvfs_set_max_freq_unlock(void);
#endif /* __PLAT_S3C64XX_DVFS_H */

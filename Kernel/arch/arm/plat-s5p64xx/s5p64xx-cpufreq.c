/*
 *  linux/arch/arm/plat-s3c64xx/s3c64xx-cpufreq.c
 *
 *  CPU frequency scaling for S3C64XX
 *
 *  Copyright (C) 2008 Samsung Electronics
 *
 *  Based on cpu-sa1110.c, Copyright (C) 2001 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/cpufreq.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>

//#include <mach/hardware.h>
#include <asm/system.h>

#include <mach/map.h>
#include <plat/regs-clock.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-bank-f.h>

#define USE_FREQ_TABLE
#define USE_DVS
#define VERY_HI_RATE	532*1000*1000
#define APLL_GEN_CLK	532*1000	//khz
#define KHZ_T		1000

#define MPU_CLK		"clk_cpu"

/* definition for power setting function */
extern int set_power(unsigned int freq);
extern void ltc3714_init(void);

#define ARM_LE	0
#define INT_LE	1

//#define CLK_PROBING

/* frequency */
static struct cpufreq_frequency_table s5p6442_freq_table[] = {
	{APLL_GEN_CLK, APLL_GEN_CLK},
	{APLL_GEN_CLK, APLL_GEN_CLK/2},
	{APLL_GEN_CLK, APLL_GEN_CLK/4},
	{0, CPUFREQ_TABLE_END},
};

/* TODO: Add support for SDRAM timing changes */

int s5p6442_verify_speed(struct cpufreq_policy *policy)
{
#ifndef USE_FREQ_TABLE
	struct clk *mpu_clk;
#endif

	if (policy->cpu)
		return -EINVAL;
#ifdef USE_FREQ_TABLE
	return cpufreq_frequency_table_verify(policy, s5p6442_freq_table);
#else
	cpufreq_verify_within_limits(policy, policy->cpuinfo.min_freq,
				     policy->cpuinfo.max_freq);
	mpu_clk = clk_get(NULL, MPU_CLK);
	if (IS_ERR(mpu_clk))
		return PTR_ERR(mpu_clk);

	policy->min = clk_round_rate(mpu_clk, policy->min * KHZ_T) / KHZ_T;
	policy->max = clk_round_rate(mpu_clk, policy->max * KHZ_T) / KHZ_T;

	cpufreq_verify_within_limits(policy, policy->cpuinfo.min_freq,
				     policy->cpuinfo.max_freq);

	clk_put(mpu_clk);

	return 0;
#endif
}

unsigned int s5p6442_getspeed(unsigned int cpu)
{
	struct clk * mpu_clk;
	unsigned long rate;

	if (cpu)
		return 0;

	mpu_clk = clk_get(NULL, MPU_CLK);
	if (IS_ERR(mpu_clk))
		return 0;
	rate = clk_get_rate(mpu_clk) / KHZ_T;

	clk_put(mpu_clk);

	return rate;
}

static int s5p6442_target(struct cpufreq_policy *policy,
		       unsigned int target_freq,
		       unsigned int relation)
{
	struct clk * mpu_clk;
	struct cpufreq_freqs freqs;
	int ret = 0;
	unsigned long arm_clk;
	unsigned int index;

	mpu_clk = clk_get(NULL, MPU_CLK);
	if (IS_ERR(mpu_clk))
		return PTR_ERR(mpu_clk);

	freqs.old = s5p6442_getspeed(0);
#ifdef USE_FREQ_TABLE
	if (cpufreq_frequency_table_target(policy, s5p6442_freq_table, target_freq, relation, &index))
		return -EINVAL;

	arm_clk = s5p6442_freq_table[index].frequency;

	freqs.new = arm_clk;
#else
	freqs.new = clk_round_rate(mpu_clk, target_freq * KHZ_T) / KHZ_T;
#endif
	freqs.cpu = 0;

	cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);
#ifdef USE_DVS
	if(freqs.new < freqs.old){
		/* frequency scaling */
		ret = clk_set_rate(mpu_clk, target_freq * KHZ_T);
		if(ret != 0)
			printk("frequency scaling error\n");
		/* voltage scaling */
		set_power(freqs.new);
	}else{
		/* voltage scaling */
		set_power(freqs.new);

		/* frequency scaling */
		ret = clk_set_rate(mpu_clk, target_freq * KHZ_T);
		if(ret != 0)
			printk("frequency scaling error\n");
	}


#else
	ret = clk_set_rate(mpu_clk, target_freq * KHZ_T);
	if(ret != 0)
		printk("frequency scaling error\n");

#endif
	cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);
	clk_put(mpu_clk);
	return ret;
}

static int __init s5p6442_cpu_init(struct cpufreq_policy *policy)
{
	struct clk * mpu_clk;

#ifdef USE_DVS
	ltc3714_init();
#endif

#ifdef CLK_PROBING
	__raw_writel((__raw_readl(S5P64XX_GPFCON)&~(0x3<<28))|(0x3<<28), S5P64XX_GPFCON);
	__raw_writel((__raw_readl(S3C_CLK_OUT)&~(0xf<<12)), S3C_CLK_OUT);
#endif
	mpu_clk = clk_get(NULL, MPU_CLK);
	if (IS_ERR(mpu_clk))
		return PTR_ERR(mpu_clk);

	if (policy->cpu != 0)
		return -EINVAL;
	policy->cur = policy->min = policy->max = s5p6442_getspeed(0);
#ifdef USE_FREQ_TABLE
	cpufreq_frequency_table_get_attr(s5p6442_freq_table, policy->cpu);
#else
	policy->cpuinfo.min_freq = clk_round_rate(mpu_clk, 0) / KHZ_T;
	policy->cpuinfo.max_freq = clk_round_rate(mpu_clk, VERY_HI_RATE) / KHZ_T;
#endif
	policy->cpuinfo.transition_latency = 40000;	//1us


	clk_put(mpu_clk);
#ifdef USE_FREQ_TABLE
	return cpufreq_frequency_table_cpuinfo(policy, s5p6442_freq_table);
#else
	return 0;
#endif
}

static struct cpufreq_driver s5p6442_driver = {
	.flags		= CPUFREQ_STICKY,
	.verify		= s5p6442_verify_speed,
	.target		= s5p6442_target,
	.get		= s5p6442_getspeed,
	.init		= s5p6442_cpu_init,
	.name		= "s5p6442",
};

static int __init s5p6442_cpufreq_init(void)
{
	return cpufreq_register_driver(&s5p6442_driver);
}

arch_initcall(s5p6442_cpufreq_init);

/*
 *  linux/arch/arm/plat-s5p6442/s5p6442-cpufreq.c
 *
 *  CPU frequency scaling for S5P6442
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
#include <linux/i2c/pmic.h>
#include <plat/clock.h>
#include <asm/system.h>
#include <plat/s5p6442-dvfs.h>
#include <plat/regs-clock.h>
#include <linux/io.h>
#include <plat/map.h>

unsigned int S5P6442_MAXFREQLEVEL = 3;
unsigned int S5P6442_MAXFREQLEVEL_ONLYCPU = 3;
static unsigned int s5p6442_cpufreq_level = 3;
unsigned int s5p6442_cpufreq_index = 0;
static spinlock_t dvfs_lock;

#define CLIP_LEVEL(a, b) (a > b ? b : a)

static struct cpufreq_frequency_table freq_table_532MHz[] = {
	{0, 533*KHZ_T},
	{1, (533*KHZ_T)/2},
	{2, (533*KHZ_T)/3},
	{3, (533*KHZ_T)/4},
#ifdef SYSCLK_CHANGE
#ifdef USE_DVFS_AL1_LEVEL
	{4, (533*KHZ_T)/4},
	{5, (533*KHZ_T)/8},
	{6, CPUFREQ_TABLE_END},
#else /* USE_DVFS_AL1_LEVEL */
	{4, (533*KHZ_T)/8},
	{5, CPUFREQ_TABLE_END},
#endif /* USE_DVFS_AL1_LEVEL */
#else /* SYSCLK_CHANGE */
	{4, CPUFREQ_TABLE_END},
#endif /* SYSCLK_CHANGE */
};

static struct cpufreq_frequency_table freq_table_666MHz[] = {
	{0, 667*KHZ_T},
	{1, (667*KHZ_T)/2},
	{2, (667*KHZ_T)/3},
	{3, (667*KHZ_T)/4},
#ifdef SYSCLK_CHANGE
	{4, (667*KHZ_T)/5},
	{5, (667*KHZ_T)/8},
	{6, CPUFREQ_TABLE_END},
#else /* SYSCLK_CHANGE */
	{4, CPUFREQ_TABLE_END},
#endif /* SYSCLK_CHANGE */
};

static unsigned char transition_state_666MHz[][2] = {
	{1, 0},
	{2, 0},
	{3, 1},
#ifdef SYSCLK_CHANGE
	{4, 2},
	{5, 3},
	{5, 4}
#else /* SYSCLK_CHANGE */
	{3, 2},
#endif /* SYSCLK_CHANGE */
};

static struct cpufreq_frequency_table freq_table_666_166MHz[] = {
        {0, 667*KHZ_T},
        {1, (667*KHZ_T)/2},
        {2, (667*KHZ_T)/3},
        {3, (667*KHZ_T)/4},
#ifdef SYSCLK_CHANGE
#ifdef USE_DVFS_AL1_LEVEL
        {4, (667*KHZ_T)/4},
        {5, (667*KHZ_T)/8},
        {6, CPUFREQ_TABLE_END},
#else /* USE_DVFS_AL1_LEVEL */
        {4, (667*KHZ_T)/8},
        {5, CPUFREQ_TABLE_END},
#endif /* USE_DVFS_AL1_LEVEL */
#else /* SYSCLK_CHANGE */
        {4, CPUFREQ_TABLE_END},
#endif /* SYSCLK_CHANGE */
};

static unsigned char transition_state_666_166MHz[][2] = {
        {1, 0},
        {2, 0},
        {3, 1},
#ifdef SYSCLK_CHANGE
#ifdef USE_DVFS_AL1_LEVEL
        {4, 2},
        {5, 3},
        {5, 4},
#else /* USE_DVFS_AL1_LEVEL */
        {4, 2},
        {4, 3},
#endif /* USE_DVFS_AL1_LEVEL */
#else /* SYSCLK_CHANGE */
        {3, 2},
#endif /* SYSCLK_CHANGE */
};

static unsigned char transition_state_532MHz[][2] = {
	{1, 0},
	{2, 0},
	{3, 1},
#ifdef SYSCLK_CHANGE
#ifdef USE_DVFS_AL1_LEVEL
	{4, 2},
	{5, 3},
	{5, 4},
#else
	{4, 2},
	{4, 3},
#endif /* USE_DVFS_AL1_LEVEL */
#else
	{3, 2},
#endif /* SYSCLK_CHANGE */
};

/* frequency voltage matching table */
unsigned int frequency_match_532MHz[][4] = {
/* frequency, Mathced VDD ARM voltage , Matched VDD INT*/
	{533000, 1100, 1200, 0},
	{266000, 1000, 1200, 1},
	{177000, 1000, 1200, 2},
	{133000, 1000, 1200, 3},
#ifdef SYSCLK_CHANGE
#ifdef USE_DVFS_AL1_LEVEL
	{133000, 1000, 1100, 4},
	{66000, 1000, 1100, 5},
#else
	{66000, 1000, 1100, 4},
#endif /* USE_DVFS_AL1_LEVEL */
#endif /* SYSCLK_CHANGE */
};

/* frequency voltage matching table */
unsigned int frequency_match_666MHz[][4] = {
/* frequency, Mathced VDD ARM voltage , Matched VDD INT*/
	{666000, 1100, 1100, 0},   // 1200 1200
	{333000, 1000, 1100, 1},   // 1050 1200
	{222000, 1000, 1100, 2},
	{166000, 1000, 1000, 3},   // 1000 1100
#ifdef SYSCLK_CHANGE
	{166000, 1000, 1000, 4},  // 1000 1100
	{83000, 900, 1000, 5},   // 1000 1100
#endif /* SYSCLK_CHANGE */
};

/* frequency voltage matching table */
unsigned int frequency_match_666_166MHz[][4] = {
/* frequency, Mathced VDD ARM voltage , Matched VDD INT*/
        {666000, 1200, 1200, 0},
        {333000, 1050, 1200, 1},
        {222000, 1000, 1100, 2},
        {166000, 1000, 1100, 3},
#ifdef SYSCLK_CHANGE
#ifdef USE_DVFS_AL1_LEVEL
        {166000, 1000, 1100, 4},
        {83000, 1000, 1100, 5},
#else /* USE_DVFS_AL1_LEVEL */
        {83000, 1000, 1100, 4},
#endif /* USE_DVFS_AL1_LEVEL */
#endif /* SYSCLK_CHANGE */
};

extern int is_pmic_initialized(void);
unsigned int (*frequency_match[3])[4] = {
	frequency_match_532MHz,
	frequency_match_666MHz,
	frequency_match_666_166MHz,
};

static unsigned char (*transition_state[3])[2] = {
	transition_state_532MHz,
	transition_state_666MHz,
	transition_state_666_166MHz,
	
};

static struct cpufreq_frequency_table *s5p6442_freq_table[] = {
	freq_table_532MHz,
	freq_table_666MHz,
	freq_table_666_166MHz,
};

int set_max_freq_flag = 0;
int dvfs_change_quick = 0;
void set_dvfs_perf_level(void)
{
	//spin_lock(&dvfs_lock);
	unsigned long flag;	
	if(spin_trylock_irqsave(&dvfs_lock,flag)) {	

	/* if some user event (keypad, touchscreen) occur, freq will be raised to 532MHz */
	/* maximum frequency :532MHz(0), 266MHz(1) */
	s5p6442_cpufreq_index = 0;
	dvfs_change_quick = 1;
	spin_unlock_irqrestore(&dvfs_lock,flag);
	}
}
EXPORT_SYMBOL(set_dvfs_perf_level);

static int dvfs_level_count = 0;
void set_dvfs_level(int flag)
{
//to do
	unsigned long irq_flags;
#if 1
	if(set_max_freq_flag){
	  dvfs_level_count = (flag == 0)?(dvfs_level_count + 1):(dvfs_level_count - 1);
	  return;
	}	
	if(spin_trylock_irqsave(&dvfs_lock,irq_flags)){	
	if(flag == 0) {
		if (dvfs_level_count > 0) {
			dvfs_level_count++;	
			spin_unlock_irqrestore(&dvfs_lock,irq_flags);
			return;
		}
		s5p6442_cpufreq_level = S5P6442_MAXFREQLEVEL_ONLYCPU;
		dvfs_level_count++;
	}
	else {
		if (dvfs_level_count > 1) {
			dvfs_level_count--;
			spin_unlock_irqrestore(&dvfs_lock,irq_flags);
			return;
		}
		s5p6442_cpufreq_level = S5P6442_MAXFREQLEVEL;
		dvfs_level_count--;
	}
	spin_unlock_irqrestore(&dvfs_lock,irq_flags);
	}
#endif
}
EXPORT_SYMBOL(set_dvfs_level);



void set_dvfs_doclk_level(int flag)
{
//	printk("####################set_dvfs_doclk_level::flag %d\n", flag);
#if 0
	spin_lock(&dvfs_lock);

	if(flag == 0) {
		s5p6442_cpufreq_level = S5P6442_MAXFREQLEVEL - 1;	
	}
	else {	  
		s5p6442_cpufreq_level = S5P6442_MAXFREQLEVEL;		
	}

	spin_unlock(&dvfs_lock);
#endif
	return;

}
EXPORT_SYMBOL(set_dvfs_doclk_level);

#ifdef USE_DVS
int get_voltage(pmic_pm_type pm_type)
{
	int volatge = 0;
	if((pm_type == VCC_ARM) || (pm_type == VCC_INT))
		get_pmic(pm_type, &volatge);

	return volatge;
}

int set_voltage(unsigned int freq_index, bool force)
{
	static int index = 0;
	unsigned int arm_voltage, int_voltage;
	unsigned int vcc_arm, vcc_int;
	unsigned int arm_delay, int_delay, delay;
	
	if (!force)
		if(index == freq_index)
			return 0;
		
	index = freq_index;
	
	vcc_arm = get_voltage(VCC_ARM);
	vcc_int = get_voltage(VCC_INT);
	
	arm_voltage = frequency_match[S5P6442_FREQ_TAB][index][1];
	int_voltage = frequency_match[S5P6442_FREQ_TAB][index][2];

	if( FakeShmoo_UV_mV_Ptr != NULL ) {
		arm_voltage -= FakeShmoo_UV_mV_Ptr[index];
		int_voltage -= FakeShmoo_UV_mV_Ptr[index];
	}

#if 1 // future work
	arm_delay = ((abs(vcc_arm - arm_voltage) / 50) * 5) + 10;
	int_delay = ((abs(vcc_int - int_voltage) / 50) * 5) + 10;

	delay = arm_delay > int_delay ? arm_delay : int_delay;
#else
	delay = 50;
#endif

	if(arm_voltage != vcc_arm) {
		set_pmic(VCC_ARM, arm_voltage);
	}

	if(int_voltage != vcc_int) {
		set_pmic(VCC_INT, int_voltage);
	}


	udelay(delay);

	return 0;
}
#endif	/* USE_DVS */

unsigned int s5p6442_target_frq(unsigned int pred_freq, 
				int flag)
{
	int index; 
	unsigned int freq;
	struct cpufreq_frequency_table *freq_tab = s5p6442_freq_table[S5P6442_FREQ_TAB];

	spin_lock(&dvfs_lock);

	if(freq_tab[0].frequency < pred_freq) {
	   index = 0;	
	   goto s5p6442_target_frq_end;
	}

	if((flag != 1)&&(flag != -1)) {
		printk(KERN_ERR "s5p6442_target_frq: flag error!!!!!!!!!!!!!");
	}

	index = s5p6442_cpufreq_index;
	
	if(freq_tab[index].frequency == pred_freq) {	
		if(flag == 1)
			index = transition_state[S5P6442_FREQ_TAB][index][1];
		else
			index = transition_state[S5P6442_FREQ_TAB][index][0];
	}
	else if(flag == -1) {
		index = 1;
	}
	else {
		index = 0; 
	}
s5p6442_target_frq_end:
	index = CLIP_LEVEL(index, s5p6442_cpufreq_level);
	s5p6442_cpufreq_index = index;
	
	freq = freq_tab[index].frequency;
	spin_unlock(&dvfs_lock);
	return freq;
}

int s5p6442_target_freq_index(unsigned int freq)
{
	int index = 0;
	
	struct cpufreq_frequency_table *freq_tab = s5p6442_freq_table[S5P6442_FREQ_TAB];

	if(freq >= freq_tab[index].frequency) {
		goto s5p6442_target_freq_index_end;
	}

	/*Index might have been calculated before calling this function.
	check and early return if it is already calculated*/
	if(freq_tab[s5p6442_cpufreq_index].frequency == freq) {		
		return s5p6442_cpufreq_index;
	}

	while((freq < freq_tab[index].frequency) &&
			(freq_tab[index].frequency != CPUFREQ_TABLE_END)) {
		index++;
	}

	if(index > 0) {
		if(freq != freq_tab[index].frequency) {
			index--;
		}
	}

	if(freq_tab[index].frequency == CPUFREQ_TABLE_END) {
		index--;
	}

s5p6442_target_freq_index_end:
	spin_lock(&dvfs_lock);	
	index = CLIP_LEVEL(index, s5p6442_cpufreq_level);
	spin_unlock(&dvfs_lock);
	s5p6442_cpufreq_index = index;
	
	return index; 
} 

int s5p6442_verify_speed(struct cpufreq_policy *policy)
{
	if(policy->cpu)
		return -EINVAL;

	return cpufreq_frequency_table_verify(policy, s5p6442_freq_table[S5P6442_FREQ_TAB]);
}

extern unsigned long s5p_fclk_get_rate(void);
unsigned int s5p6442_getspeed(unsigned int cpu)
{
	struct clk * mpu_clk;
	unsigned long rate;

	if(cpu)
		return 0;

	mpu_clk = clk_get(NULL, MPU_CLK);
	if (IS_ERR(mpu_clk))
		return 0;

	rate = (s5p_fclk_get_rate() / (KHZ_T * KHZ_T)) * KHZ_T;
	clk_put(mpu_clk);

	return rate;
}

static int s5p6442_target(struct cpufreq_policy *policy,
		       unsigned int target_freq,
		       unsigned int relation)
{
	struct clk * mpu_clk;
	struct cpufreq_freqs freqs;
	static int prevIndex = 0;
	int ret = 0;
	unsigned long arm_clk;
	unsigned int index;

#if 0 // future work
	if(!is_pmic_initialized())
		return ret;
#endif
	mpu_clk = clk_get(NULL, MPU_CLK);
	if(IS_ERR(mpu_clk))
		return PTR_ERR(mpu_clk);

	freqs.old = s5p6442_getspeed(0);

	if(freqs.old == s5p6442_freq_table[S5P6442_FREQ_TAB][0].frequency) {
		prevIndex = 0;
	}
	
	index = s5p6442_target_freq_index(target_freq);
	if(index == INDX_ERROR) {
		printk(KERN_ERR "s5p6442_target: INDX_ERROR \n");
		return -EINVAL;
	}
	
	if(prevIndex == index)
		return ret;

	arm_clk = s5p6442_freq_table[S5P6442_FREQ_TAB][index].frequency;
	freqs.new = arm_clk;
	freqs.cpu = 0;
//	freqs.new_hclk = 166000;
 /* 
	if(index > S5P6442_MAXFREQLEVEL_ONLYCPU) {
		freqs.new_hclk = 66000;         
	} 
*/

	target_freq = arm_clk;

	cpufreq_notify_transition(&freqs, CPUFREQ_PRECHANGE);

#ifdef USE_DVS
	if(prevIndex < index) {
		/* frequency scaling */
		ret = clk_set_rate(mpu_clk, target_freq * KHZ_T);
		if(ret != 0) {
			printk(KERN_ERR "frequency scaling error\n");
			ret = -EINVAL;
			goto s5p6442_target_end;
		}
		/* voltage scaling */
		set_voltage(index, false);
	}
	else {
		/* voltage scaling */
		set_voltage(index, false);
		/* frequency scaling */
		ret = clk_set_rate(mpu_clk, target_freq * KHZ_T);
		if(ret != 0) {
			printk(KERN_ERR "frequency scaling error\n");
			ret = -EINVAL;
			goto s5p6442_target_end;
		}
	}
#else
	ret = clk_set_rate(mpu_clk, target_freq * KHZ_T);
	if(ret != 0) {
		printk("frequency scaling error\n");
		ret = -EINVAL;
		goto s5p6442_target_end;
	}
#endif	/* USE_DVS */
	cpufreq_notify_transition(&freqs, CPUFREQ_POSTCHANGE);
	prevIndex = index;
	clk_put(mpu_clk);
s5p6442_target_end:
	return ret;
}

void dvfs_set_max_freq_lock(void)
{
	//Interrupts must be enabled when this function is called.
   //there may be race..but no problem
	//don't use locks...locks might cause soft lockup here
   struct cpufreq_frequency_table *freq_tab = s5p6442_freq_table[S5P6442_FREQ_TAB];
	set_max_freq_flag = 1;
	s5p6442_cpufreq_level = 0;
	s5p6442_target(NULL, freq_tab[0].frequency, 1);
	dvfs_change_quick = 1;   //better to have this flag because we are not using locks. 
	return; 
	
}

void dvfs_set_max_freq_unlock(void)
{
        set_max_freq_flag = 0;
	if (dvfs_level_count > 0) {
        	s5p6442_cpufreq_level = S5P6442_MAXFREQLEVEL_ONLYCPU;
	}
	else {
		s5p6442_cpufreq_level = S5P6442_MAXFREQLEVEL;
	}
	return;

}

unsigned int get_min_cpufreq(void)
{
	return (s5p6442_freq_table[S5P6442_FREQ_TAB][S5P6442_MAXFREQLEVEL].frequency);
}

static int __init s5p6442_cpu_init(struct cpufreq_policy *policy)
{
	struct clk * mpu_clk;
	u32 mux_stat0;

	mpu_clk = clk_get(NULL, MPU_CLK);
	if(IS_ERR(mpu_clk))
		return PTR_ERR(mpu_clk);

	if(policy->cpu != 0)
		return -EINVAL;
	policy->cur = policy->min = policy->max = s5p6442_getspeed(0);

	if(policy->max > MAXIMUM_FREQ) {
		 mux_stat0 = __raw_readl(S5P_CLK_MUX_STAT0);
	   if(((mux_stat0 & S5P_CLK_MUX_STAT0_MUXD0_MASK) >> S5P_CLK_MUX_STAT0_MUXD0_SHIFT) == 0X2){
//	    if(clk_hd1.rate < (134 * 1024 * 1024)){  //FIX ME
		S5P6442_FREQ_TAB = 1; 
		S5P6442_MAXFREQLEVEL_ONLYCPU = 3;
#ifdef SYSCLK_CHANGE
		S5P6442_MAXFREQLEVEL = 5;
#else /* SYSCLK_CHANGE */
		S5P6442_MAXFREQLEVEL = S5P6442_MAXFREQLEVEL_ONLYCPU;
#endif /* SYSCLK_CHANGE */
	}
	else {
		S5P6442_FREQ_TAB = 2; 
                S5P6442_MAXFREQLEVEL_ONLYCPU = 4;
#ifdef SYSCLK_CHANGE
#ifdef USE_DVFS_AL1_LEVEL
                S5P6442_MAXFREQLEVEL = 5;
#else /* USE_DVFS_AL1_LEVEL */
                S5P6442_MAXFREQLEVEL = 4;
#endif /* USE_DVFS_AL1_LEVEL */
#else /* SYSCLK_CHANGE */
                S5P6442_MAXFREQLEVEL = 3;
#endif /* SYSCLK_CHANGE */
		}

//#endif
	}
	else {
		S5P6442_FREQ_TAB = 0;
		S5P6442_MAXFREQLEVEL_ONLYCPU = 3;
#ifdef SYSCLK_CHANGE
#ifdef USE_DVFS_AL1_LEVEL
		S5P6442_MAXFREQLEVEL = 5;
#else /* USE_DVFS_AL1_LEVEL */
		S5P6442_MAXFREQLEVEL = 4;
#endif /* USE_DVFS_AL1_LEVEL */
#else /* SYSCLK_CHANGE */
		S5P6442_MAXFREQLEVEL = 3;
#endif /* SYSCLK_CHANGE */
	}
        //printk("#####%s::max_freq %d FREQ_TAB %d\n", __FUNCTION__, policy->max, S5P6442_FREQ_TAB);
	s5p6442_cpufreq_level = S5P6442_MAXFREQLEVEL;

	cpufreq_frequency_table_get_attr(s5p6442_freq_table[S5P6442_FREQ_TAB], policy->cpu);

	policy->cpuinfo.transition_latency = 20000;

	clk_put(mpu_clk);

	spin_lock_init(&dvfs_lock);

	return cpufreq_frequency_table_cpuinfo(policy, s5p6442_freq_table[S5P6442_FREQ_TAB]);
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

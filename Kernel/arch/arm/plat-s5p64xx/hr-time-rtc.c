/*
 * linux/arch/arm/plat-s5p64xx/hr-time-rtc.c
 *
 * S5P64XX Timers
 *
 * Copyright (c) 2006 Samsung Electronics
 *
 *
 * S5P64XX (and compatible) HRT support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/io.h>

#include <asm/system.h>
#include <mach/hardware.h>
#include <asm/leds.h>
#include <asm/irq.h>
#include <asm/mach/irq.h>
#include <asm/mach/time.h>
#include <asm/mach-types.h>
#include <mach/map.h>
#include <plat/regs-timer.h>
#include <plat/regs-rtc.h>
#include <plat/regs-sys-timer.h>
#include <mach/regs-irq.h>
#include <mach/tick.h>

#include <plat/clock.h>
#include <plat/cpu.h>

void __iomem *rtc_base = S3C_VA_RTC;
static struct clk *clk_event;
static struct clk *clk_sched;
static int tick_timer_mode;	/* 0: oneshot, 1: autoreload */
static int rtc_rate;

#define RTC_CLOCK	(32768)
#define RTC_DEFAULT_TICK	((RTC_CLOCK / HZ) - 1)
/*
 * Helper functions
 * s5p64xx_systimer_read() : Read from System timer register
 * s5p64xx_systimer_write(): Write to System timer register
 *
 */
static unsigned int s5p64xx_systimer_read(unsigned int *reg_offset)
{
	return __raw_readl(reg_offset);
}

static unsigned int s5p64xx_systimer_write(unsigned int *reg_offset,
					unsigned int value)
{
	unsigned int temp_regs;

	__raw_writel(value, reg_offset);

	if (reg_offset == S3C_SYSTIMER_TCON) {
		while(!(__raw_readl(S3C_SYSTIMER_INT_CSTAT) &
				S3C_SYSTIMER_INT_TCON));
		temp_regs = __raw_readl(S3C_SYSTIMER_INT_CSTAT);
		temp_regs |= S3C_SYSTIMER_INT_TCON;
		__raw_writel(temp_regs, S3C_SYSTIMER_INT_CSTAT);

	} else if (reg_offset == S3C_SYSTIMER_ICNTB) {
		while(!(__raw_readl(S3C_SYSTIMER_INT_CSTAT) &
				S3C_SYSTIMER_INT_ICNTB));
		temp_regs = __raw_readl(S3C_SYSTIMER_INT_CSTAT);
		temp_regs |= S3C_SYSTIMER_INT_ICNTB;
		__raw_writel(temp_regs, S3C_SYSTIMER_INT_CSTAT);

	} else if (reg_offset == S3C_SYSTIMER_TCNTB) {
		while(!(__raw_readl(S3C_SYSTIMER_INT_CSTAT) &
				S3C_SYSTIMER_INT_TCNTB));
		temp_regs = __raw_readl(S3C_SYSTIMER_INT_CSTAT);
		temp_regs |= S3C_SYSTIMER_INT_TCNTB;
		__raw_writel(temp_regs, S3C_SYSTIMER_INT_CSTAT);
	}

	return 0;
}

static void s5p64xx_rtc_set_tick(int enabled)
{
	unsigned int tmp;

	tmp = __raw_readl(rtc_base + S3C2410_RTCCON) & ~S3C_RTCCON_TICEN;
	if (enabled)
		tmp |= S3C_RTCCON_TICEN;
	__raw_writel(tmp, rtc_base + S3C2410_RTCCON);
}

static void s5p64xx_tick_timer_setup(void);

static void s5p64xx_tick_timer_start(unsigned long load_val,
					int autoreset)
{
	unsigned int tmp;

	tmp = __raw_readl(rtc_base + S3C2410_RTCCON) &
		~(S3C_RTCCON_TICEN | S3C2410_RTCCON_RTCEN);
	__raw_writel(tmp, rtc_base + S3C2410_RTCCON);

	__raw_writel(load_val, rtc_base + S3C2410_TICNT);

	tmp |= S3C_RTCCON_TICEN;

	__raw_writel(tmp, rtc_base + S3C2410_RTCCON);
}

static inline void s5p64xx_tick_timer_stop(void)
{
	unsigned int tmp;

	tmp = __raw_readl(rtc_base + S3C2410_RTCCON) &
		~(S3C_RTCCON_TICEN | S3C2410_RTCCON_RTCEN);
	
	__raw_writel(tmp, rtc_base + S3C2410_RTCCON);

}

static void s5p64xx_sched_timer_start(unsigned long load_val,
					int autoreset)
{
	unsigned long tcon;
	unsigned long tcnt;
	unsigned long tcfg;

	/* clock configuration setting and enable */
	struct clk *clk;

	tcnt = TICK_MAX;  /* default value for tcnt */

	/* initialize system timer clock */
	tcfg = s5p64xx_systimer_read(S3C_SYSTIMER_TCFG);

	tcfg &= ~S3C_SYSTIMER_TCLK_MASK;
	tcfg |= S3C_SYSTIMER_TCLK_USB;

	s5p64xx_systimer_write(S3C_SYSTIMER_TCFG, tcfg);

	/* TCFG must not be changed at run-time. 
	 * If you want to change TCFG, stop timer(TCON[0] = 0) 
	 * */
	s5p64xx_systimer_write(S3C_SYSTIMER_TCON, 0);

	/* read the current timer configuration bits */
	tcon = s5p64xx_systimer_read(S3C_SYSTIMER_TCON);
	tcfg = s5p64xx_systimer_read(S3C_SYSTIMER_TCFG);

	clk = clk_get(NULL, "systimer");
	if (IS_ERR(clk))
		panic("failed to get clock[%s] for system timer", "systimer");

	clk_enable(clk);

	clk_put(clk);
	
	tcfg &= ~S3C_SYSTIMER_TCLK_MASK;
	tcfg |= S3C_SYSTIMER_TCLK_USB;
	tcfg &= ~S3C_SYSTIMER_PRESCALER_MASK;

	/* check to see if timer is within 16bit range... */
	if (tcnt > TICK_MAX) {
		panic("setup_timer: cannot configure timer!");
		return;
	}

	s5p64xx_systimer_write(S3C_SYSTIMER_TCFG, tcfg);

	s5p64xx_systimer_write(S3C_SYSTIMER_TCNTB, tcnt);

	/* set timer con */
	tcon =  S3C_SYSTIMER_START | S3C_SYSTIMER_AUTO_RELOAD;
	s5p64xx_systimer_write(S3C_SYSTIMER_TCON, tcon);

}

/*
 * RTC tick : count down to zero, interrupt, reload
 */
static int s5p64xx_tick_set_next_event(unsigned long cycles,
				   struct clock_event_device *evt)
{
	//printk(KERN_INFO "%d\n", cycles);
	if  (cycles == 0 )	/* Should be larger than 0 */
		cycles = 1;
	s5p64xx_tick_timer_start(cycles, 0);
	return 0;
}

static void s5p64xx_tick_set_mode(enum clock_event_mode mode,
			      struct clock_event_device *evt)
{
	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		tick_timer_mode = 1;
		break;
	case CLOCK_EVT_MODE_ONESHOT:
		s5p64xx_tick_timer_stop();
		tick_timer_mode = 0;
		break;
	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
		break;
	case CLOCK_EVT_MODE_RESUME:
		s5p64xx_tick_timer_setup();
		s5p64xx_sched_timer_start(~0, 1);
		break;
	}
}

static struct clock_event_device clockevent_tick_timer = {
	.name		= "S5PC110 event timer",
	.features       = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
	.shift		= 32,
	.set_next_event	= s5p64xx_tick_set_next_event,
	.set_mode	= s5p64xx_tick_set_mode,
};

irqreturn_t s5p64xx_tick_timer_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *evt = &clockevent_tick_timer;
	
	__raw_writel(S3C_INTP_TIC, rtc_base + S3C_INTP);
	/* In case of oneshot mode */
	if (tick_timer_mode == 0)
		s5p64xx_tick_timer_stop();
		
	evt->event_handler(evt);

	return IRQ_HANDLED;
}

static struct irqaction s5p64xx_tick_timer_irq = {
	.name		= "rtc-tick",
	.flags		= IRQF_DISABLED | IRQF_TIMER | IRQF_IRQPOLL,
	.handler	= s5p64xx_tick_timer_interrupt,
};

static void  s5p64xx_init_dynamic_tick_timer(unsigned long rate)
{
	tick_timer_mode = 1;
	
	s5p64xx_tick_timer_stop();

	s5p64xx_tick_timer_start((rate / HZ) - 1, 1);

	clockevent_tick_timer.mult = div_sc(rate, NSEC_PER_SEC,
					    clockevent_tick_timer.shift);
	clockevent_tick_timer.max_delta_ns =
		clockevent_delta2ns(-1, &clockevent_tick_timer);
	clockevent_tick_timer.min_delta_ns =
		clockevent_delta2ns(1, &clockevent_tick_timer);

	clockevent_tick_timer.cpumask = cpumask_of(0);
	clockevents_register_device(&clockevent_tick_timer);
	
	printk(KERN_INFO "mult[%d]\n", clockevent_tick_timer.mult);
	printk(KERN_INFO "max_delta_ns[%d]\n", clockevent_tick_timer.max_delta_ns);
	printk(KERN_INFO "min_delta_ns[%d]\n", clockevent_tick_timer.min_delta_ns);
	printk(KERN_INFO "rate[%d]\n", rate);
	printk(KERN_INFO "HZ[%d]\n", HZ);
}


/*
 * ---------------------------------------------------------------------------
 * SYSTEM TIMER ... free running 32-bit clock source and scheduler clock
 * ---------------------------------------------------------------------------
 */

static cycle_t s5p64xx_sched_timer_read(void)
{

	return (cycle_t)~__raw_readl(S3C_SYSTIMER_TCNTO);
}

struct clocksource clocksource_s5p64xx = {
	.name		= "clock_source_systimer",
	.rating		= 300,
	.read		= s5p64xx_sched_timer_read,
	.mask		= CLOCKSOURCE_MASK(32),
	.shift		= 20,
	.flags		= CLOCK_SOURCE_IS_CONTINUOUS,
};

unsigned long long sched_clock(void)
{
	static int first = 1;
	static cycle_t saved_ticks;
	static int saved_ticks_valid;
	static unsigned long long base;
	static unsigned long long last_result;

	unsigned long irq_flags;
	static cycle_t last_ticks;
	cycle_t ticks;
	static unsigned long long result;

	local_irq_save(irq_flags);

	last_ticks = saved_ticks;
	saved_ticks = ticks = s5p64xx_sched_timer_read();

	if(!saved_ticks_valid)
	{
		saved_ticks_valid = 1;
		last_ticks = ticks;
		base -= clocksource_cyc2ns(ticks, clocksource_s5p64xx.mult, clocksource_s5p64xx.shift);
	}
	if(ticks < last_ticks)
	{
		if (first)
			first = 0;
		else
		{
			base += clocksource_cyc2ns(ticks, clocksource_s5p64xx.mult, clocksource_s5p64xx.shift);
			base += clocksource_cyc2ns(1, clocksource_s5p64xx.mult, clocksource_s5p64xx.shift);
		}
	}

	last_result = result = clocksource_cyc2ns(ticks, clocksource_s5p64xx.mult, clocksource_s5p64xx.shift) + base;

	local_irq_restore(irq_flags);

	return result;
}

static void s5p64xx_init_clocksource(unsigned long rate)
{
	static char err[] __initdata = KERN_ERR
			"%s: can't register clocksource!\n";

	clocksource_s5p64xx.mult
		= clocksource_khz2mult(rate/1000, clocksource_s5p64xx.shift);


	s5p64xx_sched_timer_start(~0, 1);

	if (clocksource_register(&clocksource_s5p64xx))
		printk(err, clocksource_s5p64xx.name);
}

/*
 *  Event/Sched Timer initialization
 */
static void s5p64xx_timer_setup(void)
{
	unsigned long rate;
	/* Setup event timer using XrtcXTI */
	if (clk_event == NULL)
		clk_event = clk_get(NULL, "XrtcXTI");
	
	if (IS_ERR(clk_event))
		panic("failed to get clock for event timer");

	rate = clk_get_rate(clk_event);
	s5p64xx_init_dynamic_tick_timer(rate);

	/* Setup sched-timer using XusbXTI */
	if (clk_sched == NULL)
		clk_sched = clk_get(NULL, "XusbXTI");
	if (IS_ERR(clk_sched))
		panic("failed to get clock for sched-timer");
	rate = clk_get_rate(clk_sched);
	s5p64xx_init_clocksource(rate);
}


static void s5p64xx_tick_timer_setup(void)
{
	unsigned long rate;

	rate = clk_get_rate(clk_event);
	s5p64xx_tick_timer_start((rate / HZ) - 1, 1);
}


static void __init s5p64xx_timer_init(void)
{
	s5p64xx_timer_setup();
	setup_irq(IRQ_RTC_TIC, &s5p64xx_tick_timer_irq);
}


struct sys_timer s5p64xx_timer = {
	.init		= s5p64xx_timer_init,
};


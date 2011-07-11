/*
 * arch/arm/mach-s5p6442/cpuidle.c
 * 
 * Copyright (c) Samsung Electronics Co. Ltd
 *
 * CPU idle driver for S5P6442
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/cpuidle.h>
#include <linux/io.h>
#include <asm/proc-fns.h>
#include <asm/cacheflush.h>
#include <linux/dma-mapping.h>

#include <mach/map.h>
#include <mach/regs-irq.h>
#include <plat/regs-clock.h>
#include <plat/pm.h>
#include <plat/regs-serial.h>
#include <mach/cpuidle.h>
#include <plat/devs.h>
#include <plat/regs-hsmmc.h>
#include <plat/regs-gpio.h>

#define S5P6442_MAX_STATES	1

int previous_idle_mode = NORMAL_MODE;

/* For saving & restoring VIC register before entering
 * idle2 mode
 **/
static unsigned long vic_regs[9];

/* Simple data structure from struct sdhci_host
 * User only ioaddr field
 * */
struct sdhci_host_simple {
	const char      *hw_name;
	unsigned int    quirks;
	int             irq;
	int		irq_cd;
	void __iomem *  ioaddr;
};

/* If SD/MMC interface is working: return = 1 or not 0 */
static int check_sdmmc_op(unsigned int ch)
{
	struct platform_device *pdev;
	struct sdhci_host_simple *host;
	unsigned int reg;

	switch (ch) {
	case 0:
		pdev = &s3c_device_hsmmc0;
		break;
	case 1:
		pdev = &s3c_device_hsmmc1;
		break;
	case 2: 
		pdev = &s3c_device_hsmmc2;
		break;
	default:
		printk(KERN_ERR "No suitable ch # for SDMMC[%d]\n", ch);
		break;
	}

	if (pdev == NULL)
		return 0;

	host = platform_get_drvdata(pdev);

	/* Check CMDINHDAT[1] and CMDINHCMD [0] */
	reg = readl(host->ioaddr + S3C_HSMMC_PRNSTS); 
      
	if ( reg & (S3C_HSMMC_CMD_INHIBIT | S3C_HSMMC_DATA_INHIBIT) ) {
		// printk(KERN_INFO "sdmmc[%d] is working\n", ch);
		return 1;
	} else {
		return 0;
	}
}

static int loop_sdmmc_check(void)
{
	unsigned int iter;

	for (iter = 1; iter < 3; iter++) {
		if (check_sdmmc_op(iter))
		return 1;
	}
	return 0;
}

/* Before entering, idle2 mode GPIO Powe Down Mode
 * Configuration register has to be set with same state
 * in Normal Mode
 **/
#define GPIO_OFFSET		0x20
#define GPIO_CON_PDN_OFFSET	0x10
#define GPIO_PUD_PDN_OFFSET	0x14
#define GPIO_PUD_OFFSET		0x08

static void s5p6442_gpio_pdn_conf(void)
{
	void __iomem *gpio_base = S5P64XX_GPA0_BASE;
	unsigned int val;

	do {
		/* Keep the previous state in idle2 mode */
		__raw_writel(0xffff, gpio_base + GPIO_CON_PDN_OFFSET);

		/* Pull up-down state in idle2 is same as normal */
		val = __raw_readl(gpio_base + GPIO_PUD_OFFSET);
		__raw_writel(val, gpio_base + GPIO_PUD_PDN_OFFSET);

		gpio_base += GPIO_OFFSET;

	} while (gpio_base <= S5P64XX_MP18_BASE);
}

static void s5p6442_enter_idle(void)
{
	unsigned long tmp;

	tmp = __raw_readl(S5P_IDLE_CFG);
	tmp &= ~((3<<30)|(3<<28)|(1<<0));
	tmp |= ((2<<30)|(2<<28));
	__raw_writel(tmp, S5P_IDLE_CFG);

	tmp = __raw_readl(S5P_PWR_CFG);
	tmp &= S5P_CFG_WFI_CLEAN;
	__raw_writel(tmp, S5P_PWR_CFG);

	cpu_do_idle();
}

static void s5p6442_enter_idle2(void)
{
	unsigned regs_save[16];
	unsigned long tmp;

	/* store the physical address of the register recovery block */
	s5p6442_sleep_save_phys = virt_to_phys(regs_save);

	/* ensure INF_REG0  has the resume address */
	__raw_writel(virt_to_phys(s5p6442_cpu_resume), S5P_INFORM0);

	/* Save current VIC_INT_ENABLE register*/
	vic_regs[0] = __raw_readl(S5P64XX_VIC0INTSELECT);
	vic_regs[1] = __raw_readl(S5P64XX_VIC1INTSELECT);
	vic_regs[2] = __raw_readl(S5P64XX_VIC2INTSELECT);
	vic_regs[3] = __raw_readl(S5P64XX_VIC0INTENABLE);
	vic_regs[4] = __raw_readl(S5P64XX_VIC1INTENABLE);
	vic_regs[5] = __raw_readl(S5P64XX_VIC2INTENABLE);
	vic_regs[6] = __raw_readl(S5P64XX_VIC0SOFTINT);
	vic_regs[7] = __raw_readl(S5P64XX_VIC1SOFTINT);
	vic_regs[8] = __raw_readl(S5P64XX_VIC2SOFTINT);

	/* Disable all interrupt through VIC */
	__raw_writel(0xffffffff, S5P64XX_VIC0INTENCLEAR);
	__raw_writel(0xffffffff, S5P64XX_VIC1INTENCLEAR);
	__raw_writel(0xffffffff, S5P64XX_VIC2INTENCLEAR);
	__raw_writel(0xffffffff, S5P64XX_VIC0SOFTINTCLEAR);
	__raw_writel(0xffffffff, S5P64XX_VIC1SOFTINTCLEAR);
	__raw_writel(0xffffffff, S5P64XX_VIC2SOFTINTCLEAR);

	/* GPIO Power Down Control */
	s5p6442_gpio_pdn_conf();

	/* Wakeup source configuration for idle2 */
	tmp = __raw_readl(S5P_WAKEUP_MASK);
	/* RTC TICK, I2S, Keypad are enabled as wakeup sources */
	tmp |= 0xffff;
	tmp &= ~((1<<2) | (1<<13) | (1<<5));
	__raw_writel(tmp, S5P_WAKEUP_MASK);

	/* Clear wakeup status register */
	tmp = __raw_readl(S5P_WAKEUP_STAT);
	__raw_writel(tmp, S5P_WAKEUP_STAT);

	/* IDLE config register set */
	/* Memory retention off */
	/* Memory LP mode       */
	tmp = __raw_readl(S5P_IDLE_CFG);
	tmp &= ~(0xf << 28);
	tmp |= ((1<<30) | (1<<28) | (1<<0));
	__raw_writel(tmp, S5P_IDLE_CFG);

	/* Power mode register configuration */
	tmp = __raw_readl(S5P_PWR_CFG);
	tmp &= S5P_CFG_WFI_CLEAN;
	tmp |= S5P_CFG_WFI_IDLE;
	__raw_writel(tmp, S5P_PWR_CFG);

	/* SYSC INT Disable */
	tmp = __raw_readl(S5P_OTHERS);
	tmp |= S5P_OTHER_SYSC_INTOFF;
	tmp |= (0x3 << 8);
	__raw_writel(tmp, S5P_OTHERS);

	/* Entering idle2 mode with WFI instruction */
	if (s5p6442_cpu_save(regs_save) == 0) {
		flush_cache_all();
		pm_cpu_sleep();
	}

	/* restore the cpu state */
	cpu_init();

	tmp = __raw_readl(S5P_IDLE_CFG);
	tmp &= ~((3<<30)|(3<<28)|(1<<0));
	tmp |= ((2<<30)|(2<<28));
	__raw_writel(tmp, S5P_IDLE_CFG);

	/* Release retention GPIO/MMC/UART IO */
	tmp = __raw_readl(S5P_OTHERS);
	tmp |= ((1<<31) | (1<<30) | (1<<29) | (1<<28));
	tmp &= ~(0x3 << 8);
	__raw_writel(tmp, S5P_OTHERS);

	__raw_writel(vic_regs[0], S5P64XX_VIC0INTSELECT);
	__raw_writel(vic_regs[1], S5P64XX_VIC1INTSELECT);
	__raw_writel(vic_regs[2], S5P64XX_VIC2INTSELECT);
	__raw_writel(vic_regs[3], S5P64XX_VIC0INTENABLE);
	__raw_writel(vic_regs[4], S5P64XX_VIC1INTENABLE);
	__raw_writel(vic_regs[5], S5P64XX_VIC2INTENABLE);
	__raw_writel(vic_regs[6], S5P64XX_VIC0SOFTINT);
	__raw_writel(vic_regs[7], S5P64XX_VIC1SOFTINT);
	__raw_writel(vic_regs[7], S5P64XX_VIC2SOFTINT);

	tmp = __raw_readl(S5P_WAKEUP_STAT);
	__raw_writel(tmp, S5P_WAKEUP_STAT);
}	

static struct cpuidle_driver s5p6442_idle_driver = {
	.name =         "s5p6442_idle",
	.owner =        THIS_MODULE,
};

static DEFINE_PER_CPU(struct cpuidle_device, s5p6442_cpuidle_device);

/* Actual code that puts the SoC in different idle states */
static int s5p6442_enter_idle_normal(struct cpuidle_device *dev)
{
	struct timeval before, after;
	int idle_time;

	local_irq_disable();
	do_gettimeofday(&before);

	s5p6442_enter_idle();

	do_gettimeofday(&after);
	local_irq_enable();
	idle_time = (after.tv_sec - before.tv_sec) * USEC_PER_SEC +
			(after.tv_usec - before.tv_usec);
	return idle_time;
}

static spinlock_t idle2_lock;
int idle2_lock_count = 0;
/* 
 * flag = 0 : allow to enter lpaudio (idle, idle2) mode.
 * flag = 1 : not allow to enter lpaudio (idle, idle2) mode.
 */
void s5p6442_set_lpaudio_lock(int flag)
{
	spin_lock(&idle2_lock);

	if (flag == 1)
		idle2_lock_count++;
	else
		idle2_lock_count--;

	spin_unlock(&idle2_lock);
}
EXPORT_SYMBOL(s5p6442_set_lpaudio_lock);

#ifdef CONFIG_SND_S5P_RP
/* s5p_rp_is_running is from s5p_rp driver */
extern int s5p_rp_is_running;
#endif

static int s5p6442_idle_bm_check(void)
{
#ifdef CONFIG_SND_S5P_RP
	if (loop_sdmmc_check() || !has_audio_wake_lock() || idle2_lock_count != 0 || !s5p_rp_is_running)
#else
	if (loop_sdmmc_check() || !has_audio_wake_lock() || idle2_lock_count != 0)
#endif
		return 1;
	else
		return 0;
}

/* Actual code that puts the SoC in different idle states */
static int s5p6442_enter_idle_lpaudio(struct cpuidle_device *dev)
{
	struct timeval before, after;
	int idle_time;

	local_irq_disable();
	do_gettimeofday(&before);

	s5p6442_enter_idle2();

	do_gettimeofday(&after);
	local_irq_enable();
	idle_time = (after.tv_sec - before.tv_sec) * USEC_PER_SEC +
			(after.tv_usec - before.tv_usec);
	return idle_time;
}

static int s5p6442_enter_idle_bm(struct cpuidle_device *dev,
				struct cpuidle_state *state)
{
	if (s5p6442_idle_bm_check())
		return s5p6442_enter_idle_normal(dev);
	else
		return s5p6442_enter_idle_lpaudio(dev);
}

int s5p6442_setup_lpaudio(unsigned int mode)
{
	struct cpuidle_device *device;

	int ret = 0;

	cpuidle_pause_and_lock();
	device = &per_cpu(s5p6442_cpuidle_device, smp_processor_id());
	cpuidle_disable_device(device);

	switch (mode) {
	case NORMAL_MODE:
		device->state_count = 1;
		/* Wait for interrupt state */
		device->states[0].enter = s5p6442_enter_idle_normal;
		device->states[0].exit_latency = 1;	/* uS */
		device->states[0].target_residency = 10000;
		device->states[0].flags = CPUIDLE_FLAG_TIME_VALID;
		strcpy(device->states[0].name, "IDLE");
		strcpy(device->states[0].desc, "ARM clock gating - WFI");
		break;
	case LPAUDIO_MODE:
		device->state_count = 1;
		/* Wait for interrupt state */
		device->states[0].enter = s5p6442_enter_idle_bm;
		device->states[0].exit_latency = 1;	/* uS */
		device->states[0].target_residency = 5000;
		device->states[0].flags = CPUIDLE_FLAG_TIME_VALID |
						CPUIDLE_FLAG_CHECK_BM;
		strcpy(device->states[0].name, "DEEP IDLE");
		strcpy(device->states[0].desc, "S5P6442 idle2");

		break;
	default:
		printk(KERN_ERR "Can't find cpuidle mode %d\n", mode);
		device->state_count = 1;

		/* Wait for interrupt state */
		device->states[0].enter = s5p6442_enter_idle_normal;
		device->states[0].exit_latency = 1;	/* uS */
		device->states[0].target_residency = 10000;
		device->states[0].flags = CPUIDLE_FLAG_TIME_VALID;
		strcpy(device->states[0].name, "IDLE");
		strcpy(device->states[0].desc, "ARM clock gating - WFI");
		break;
	}
	cpuidle_enable_device(device);
	cpuidle_resume_and_unlock();

	return ret;
}
EXPORT_SYMBOL(s5p6442_setup_lpaudio);

/* Initialize CPU idle by registering the idle states */
static int s5p6442_init_cpuidle(void)
{
	struct cpuidle_device *device;

	cpuidle_register_driver(&s5p6442_idle_driver);

	device = &per_cpu(s5p6442_cpuidle_device, smp_processor_id());
	device->state_count = 1;

	/* Wait for interrupt state */
	device->states[0].enter = s5p6442_enter_idle_normal;
	device->states[0].exit_latency = 1;	/* uS */
	device->states[0].target_residency = 10000;
	device->states[0].flags = CPUIDLE_FLAG_TIME_VALID;
	strcpy(device->states[0].name, "IDLE");
	strcpy(device->states[0].desc, "ARM clock gating - WFI");

	if (cpuidle_register_device(device)) {
		printk(KERN_ERR "s5p6442_init_cpuidle: Failed registering\n");
		return -EIO;
	}

	spin_lock_init(&idle2_lock);

	return 0;
}

device_initcall(s5p6442_init_cpuidle);

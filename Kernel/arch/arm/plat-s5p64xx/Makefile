# arch/arm/plat-s5p64xx/Makefile
#
# Copyright 2008 Openmoko, Inc.
# Copyright 2008 Simtec Electronics
#
# Licensed under GPLv2

obj-y				:=
obj-m				:=
obj-n				:= dummy.o
obj-				:=

# Core files

obj-y				+= dev-uart.o devs.o
obj-y				+= cpu.o
ifndef CONFIG_S5P64XX_HIGH_RES_TIMERS
obj-y				+= s5p64xx-time.o
else
ifdef CONFIG_HRT_RTC
obj-y				+= hr-time-rtc.o
else
obj-y				+= hr-time.o
endif
endif
obj-y				+= irq.o
obj-y				+= irq-eint.o
ifdef CONFIG_LITE
ifdef CONFIG_PERFORMANCE
ifdef CONFIG_ULTIMATE
ifdef CONFIG_EXTREME
obj-y				+= clock-ext.o
else
obj-y				+= clock-ult.o
endif
else
obj-y				+= clock-perf.o
endif
else
obj-y				+= clock-lite.o
endif
else
obj-y				+= clock.o
endif
obj-y				+= gpiolib.o
obj-y				+= bootmem.o

# CPU support

obj-$(CONFIG_CPU_S5P6442_INIT)	+= s5p6442-init.o
ifdef CONFIG_LITE
ifdef CONFIG_PERFORMANCE
ifdef CONFIG_ULTIMATE
ifdef CONFIG_EXTREME
obj-$(CONFIG_CPU_S5P6442_CLOCK)	+= s5p6442-clock-ext.o
obj-$(CONFIG_CPU_FREQ)         += s5p6442-dvfs-ext.o changefreq-ext.o
else
obj-$(CONFIG_CPU_S5P6442_CLOCK)	+= s5p6442-clock-ult.o
obj-$(CONFIG_CPU_FREQ)         += s5p6442-dvfs-ult.o changefreq-ult.o
endif
else
obj-$(CONFIG_CPU_S5P6442_CLOCK)	+= s5p6442-clock-perf.o
obj-$(CONFIG_CPU_FREQ)         += s5p6442-dvfs-perf.o changefreq-perf.o
endif
else
obj-$(CONFIG_CPU_S5P6442_CLOCK)	+= s5p6442-clock-lite.o
obj-$(CONFIG_CPU_FREQ)         += s5p6442-dvfs-lite.o changefreq-lite.o
endif
else
obj-$(CONFIG_CPU_S5P6442_CLOCK)	+= s5p6442-clock.o
obj-$(CONFIG_CPU_FREQ)         += s5p6442-dvfs.o changefreq.o
endif

obj-$(CONFIG_PM)                += pm.o
obj-$(CONFIG_PM)                += sleep.o
obj-$(CONFIG_PM)                +=power_clk_gating.o
# Device setup

obj-$(CONFIG_S5P64XX_SETUP_I2C0) += setup-i2c0.o
obj-$(CONFIG_S5P64XX_SETUP_I2C1) += setup-i2c1.o
obj-$(CONFIG_S5P64XX_SETUP_I2C2) += setup-i2c2.o
obj-$(CONFIG_S5P64XX_DEV_POST)   += dev-post.o
obj-$(CONFIG_S5P64XX_SETUP_POST) += setup-post.o
obj-$(CONFIG_S5P64XX_ADCTS) 	 += s5p64xx-adcts.o
obj-$(CONFIG_HAVE_PWM) 		 += pwm.o
obj-$(CONFIG_S5P64XX_PWM)        += pwm-s5p6442.o

obj-$(CONFIG_SPI_S5P6442)   += dev-spi.o

# Device setup - FIMC
obj-$(CONFIG_S5P64XX_DEV_FIMC0)	  += dev-fimc0.o
obj-$(CONFIG_S5P64XX_DEV_FIMC1)	  += dev-fimc1.o
obj-$(CONFIG_S5P64XX_DEV_FIMC2)	  += dev-fimc2.o
obj-$(CONFIG_S5P64XX_SETUP_FIMC0) += setup-fimc0.o
obj-$(CONFIG_S5P64XX_SETUP_FIMC1) += setup-fimc1.o
obj-$(CONFIG_S5P64XX_SETUP_FIMC2) += setup-fimc2.o

# Device setup - FIMD
obj-$(CONFIG_S5P64XX_DEV_FB)	  += dev-fb.o


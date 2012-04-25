# arch/arm/mach-s5p6442/Makefile
#
# Copyright 2008 Openmoko, Inc.
# Copyright 2008 Simtec Electronics
# Copyright 2010 Samsung Electronics Co. Ltd.
#
# Licensed under GPLv2

obj-y				:=
obj-m				:=
obj-n				:=
obj-				:=

# Core support for S5P6442 system

obj-$(CONFIG_CPU_S5P6442)	+= cpu.o
obj-$(CONFIG_CPU_S5P6442)	+= dma.o

# Helper and device support

obj-$(CONFIG_S5P6442_SETUP_SDHCI)	+= setup-sdhci.o

obj-$(CONFIG_PM)    += pm.o

# machine support
obj-$(CONFIG_MACH_SMDK6442)	+= mach-smdk6442.o
obj-$(CONFIG_MACH_APOLLO)    += mach-apollo.o apollo-rfkill.o

# Button EINT support
obj-$(CONFIG_MACH_SMDK6442)	+= button-smdk6442.o

#sec headset support
obj-$(CONFIG_SEC_HEADSET)	+= sec_headset.o

obj-$(CONFIG_CPU_IDLE)		+= cpuidle.o

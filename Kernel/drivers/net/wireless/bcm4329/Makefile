# bcm4329
DHDCFLAGS = -DLINUX -DBCMDRIVER -DBCMDONGLEHOST -DDHDTHREAD                   \
	-DBCMWPA2 -DBCMWAPI_WPI -DUNRELEASEDCHIP -DCONFIG_WIRELESS_EXT        \
	-DDHD_GPL -DDHD_SCHED -DBDC -DTOE -DDHD_BCMEVENTS                     \
	-DSHOW_EVENTS -DSDIO_ISR_THREAD -DBCMSDIO -DDHD_GPL                   \
	-DBCMLXSDMMC -DBCMPLATFORM_BUS -DEMBEDDED_PLATFORM -DOEM_ANDROID      \
	-DARP_OFFLOAD_SUPPORT -DPKT_FILTER_SUPPORT -DMMC_SDIO_FORCE_PULLUP    \
	-DMMC_SDIO_BROKEN_CCCR_REV -DCUSTOMER_HW4 -DCUSTOM_RESET_GPIO_NUM=148  \
	-DSOFTAP -Wall -Wstrict-prototypes                                    \
	-Idrivers/net/wireless/bcm4329 -Idrivers/net/wireless/bcm4329/include

#  -DTOE -DDHD_SCHED -DMMC_SDIO_FORCE_PULLUP    -DBCMWAPI_WPI  -DARP_OFFLOAD_SUPPORT -DPKT_FILTER_SUPPORT 

DHDOFILES = dhd_linux.o \
         dhd_common.o \
         dhd_cdc.o \
         dhd_linux_sched.o\
         dhd_sdio.o \
         dhd_custom_gpio.o \
         aiutils.o \
         bcmutils.o \
         bcmwifi.o \
         hndpmu.o \
         linux_osl.o \
         sbutils.o \
         siutils.o \
         bcmsdh_sdmmc.o \
         bcmsdh.o \
         bcmsdh_linux.o \
         bcmsdh_sdmmc_linux.o \
         wlgpio.o \
         wl_iw.o

obj-$(CONFIG_BCM4329) += dhd.o
dhd-objs += $(DHDOFILES)
EXTRA_CFLAGS = $(DHDCFLAGS)
EXTRA_LDFLAGS += --strip-debug 

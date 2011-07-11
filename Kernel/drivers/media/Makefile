#
# Makefile for the kernel multimedia device drivers.
#

obj-y += common/ video/

obj-$(CONFIG_VIDEO_DEV) += radio/
obj-$(CONFIG_DVB_CORE)  += dvb/
obj-$(CONFIG_S5P6442_MFC)       += s5p6442/mfc/
obj-$(CONFIG_VIDEO_FIMC)	+= s5p6442/fimc_drv/ 
obj-$(CONFIG_VIDEO_JPEG_V2)	+= s5p6442/jpeg_v2/ 
obj-$(CONFIG_VIDEO_G3D)		+= s5p6442/g3d_drv/
obj-$(CONFIG_VIDEO_G2D)         += s5p6442/g2d_drv/

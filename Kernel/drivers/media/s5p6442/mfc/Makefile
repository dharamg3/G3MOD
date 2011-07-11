#################################################
# Makefile for MFC Driver
# 2010 (C) Samsung Electronics 

#################################################

KBUILD_CFLAGS += -DDIVX_ENABLE
EXTRA_CFLAGS += -DLINUX
EXTRA_CFLAGS += -DDIVX_ENABLE

s3c_mfc-y 		:= Prism_S_V13F2.o BitProcBuf.o DataBuf.o FramBufMgr.o \
			 LogMsg.o MFC_HW_Init.o MFC_Inst_Pool.o MFC_Instance.o MfcMemory.o MfcMutex.o MfcSfr.o 	\
			 s3c-mfc.o MfcIntrNotification.o MfcSetConfig.o

obj-$(CONFIG_S5P6442_MFC)		:= s3c_mfc.o 






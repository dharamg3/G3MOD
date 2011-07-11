svnet-y := main.o pdp.o

svnet-y += sipc4.o

ifeq ($(CONFIG_TARGET_LOCALE_KOR),y)
svnet-y := $(svnet-y:main.o=main_kor.o)
svnet-y := $(svnet-y:sipc4.o=sipc4_kor.o)
endif

obj-m	+= svnet.o


KERNELDIR := ../../

XVM = sub-projects/allocators/xvmalloc-kmod
EXTRA_CFLAGS	:=	-Wall

obj-m		+=	zram.o
zram-objs	:=	zram_drv.o zram_sysfs.o $(XVM)/xvmalloc.o

all:
	make -C $(KERNELDIR) M=$(PWD) modules

clean:
	make -C $(KERNELDIR) M=$(PWD) clean
	@rm -rf *.ko

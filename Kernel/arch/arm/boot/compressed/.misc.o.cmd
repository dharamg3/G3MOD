cmd_arch/arm/boot/compressed/misc.o := /root/CodeSourcery/Sourcery_G++_Lite/bin/arm-none-eabi-gcc -Wp,-MD,arch/arm/boot/compressed/.misc.o.d  -nostdinc -isystem /root/CodeSourcery/Sourcery_G++_Lite/bin/../lib/gcc/arm-none-eabi/4.5.2/include -Iinclude  -I/root/Desktop/Dharam/Kernel/arch/arm/include -include include/linux/autoconf.h -D__KERNEL__ -mlittle-endian -Iarch/arm/mach-s5p6442/include -Iarch/arm/plat-s5p64xx/include -Iarch/arm/plat-s5p/include -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -Werror-implicit-function-declaration -Wno-format-security -fno-delete-null-pointer-checks -fsingle-precision-constant -O2 -fno-reorder-blocks -fno-tree-ch -marm -fno-omit-frame-pointer -mapcs -mno-sched-prolog -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables -D__LINUX_ARM_ARCH__=6 -march=armv6k -mtune=arm1136j-s -msoft-float -Uarm -Wframe-larger-than=1024 -fno-stack-protector -fno-omit-frame-pointer -fno-optimize-sibling-calls -Wdeclaration-after-statement -Wno-pointer-sign -fno-strict-overflow -fno-dwarf2-cfi-asm -fconserve-stack -fpic -fno-builtin -Dstatic=   -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(misc)"  -D"KBUILD_MODNAME=KBUILD_STR(misc)"  -c -o arch/arm/boot/compressed/misc.o arch/arm/boot/compressed/misc.c

deps_arch/arm/boot/compressed/misc.o := \
  arch/arm/boot/compressed/misc.c \
    $(wildcard include/config/debug/icedcc.h) \
    $(wildcard include/config/cpu/v6.h) \
    $(wildcard include/config/cpu/v7.h) \
    $(wildcard include/config/cpu/xscale.h) \
  include/linux/compiler.h \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
  include/linux/compiler-gcc.h \
    $(wildcard include/config/arch/supports/optimized/inlining.h) \
    $(wildcard include/config/optimize/inlining.h) \
  include/linux/compiler-gcc4.h \
  include/linux/types.h \
    $(wildcard include/config/uid16.h) \
    $(wildcard include/config/lbdaf.h) \
    $(wildcard include/config/phys/addr/t/64bit.h) \
    $(wildcard include/config/64bit.h) \
  /root/Desktop/Dharam/Kernel/arch/arm/include/asm/types.h \
  include/asm-generic/int-ll64.h \
  /root/Desktop/Dharam/Kernel/arch/arm/include/asm/bitsperlong.h \
  include/asm-generic/bitsperlong.h \
  include/linux/posix_types.h \
  include/linux/stddef.h \
  /root/Desktop/Dharam/Kernel/arch/arm/include/asm/posix_types.h \
  /root/Desktop/Dharam/Kernel/arch/arm/include/asm/string.h \
  arch/arm/mach-s5p6442/include/mach/uncompress.h \
  arch/arm/mach-s5p6442/include/mach/map.h \
  arch/arm/plat-s5p/include/plat/map-base.h \
  arch/arm/plat-s5p/include/plat/uncompress.h \
    $(wildcard include/config/s3c/lowlevel/uart/port.h) \
    $(wildcard include/config/s3c/boot/watchdog.h) \
    $(wildcard include/config/s3c/boot/error/reset.h) \
    $(wildcard include/config/s3c/boot/uart/force/fifo.h) \
  arch/arm/plat-s5p/include/plat/regs-serial.h \
    $(wildcard include/config/plat/s3c64xx.h) \
    $(wildcard include/config/plat/s5pc1xx.h) \
    $(wildcard include/config/plat/s5p64xx.h) \
    $(wildcard include/config/plat/s5pc11x.h) \
    $(wildcard include/config/cpu/s5pc110.h) \
    $(wildcard include/config/cpu/s5p6442.h) \
    $(wildcard include/config/cpu/s3c6400.h) \
    $(wildcard include/config/cpu/s3c6410.h) \
    $(wildcard include/config/cpu/s5pc100.h) \
  arch/arm/plat-s5p/include/plat/regs-watchdog.h \
  arch/arm/boot/compressed/../../../../lib/inflate.c \

arch/arm/boot/compressed/misc.o: $(deps_arch/arm/boot/compressed/misc.o)

$(deps_arch/arm/boot/compressed/misc.o):

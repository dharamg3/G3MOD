cmd_arch/arm/plat-s5p64xx/sleep.o := /root/CodeSourcery/Sourcery_G++_Lite/bin/arm-none-eabi-gcc -Wp,-MD,arch/arm/plat-s5p64xx/.sleep.o.d  -nostdinc -isystem /root/CodeSourcery/Sourcery_G++_Lite/bin/../lib/gcc/arm-none-eabi/4.5.2/include -Iinclude  -I/root/Desktop/Dharam/Kernel/arch/arm/include -include include/linux/autoconf.h -D__KERNEL__ -mlittle-endian -Iarch/arm/mach-s5p6442/include -Iarch/arm/plat-s5p64xx/include -Iarch/arm/plat-s5p/include -D__ASSEMBLY__ -mabi=aapcs-linux -mno-thumb-interwork -funwind-tables  -D__LINUX_ARM_ARCH__=6 -march=armv6k -mtune=arm1136j-s -include asm/unified.h -msoft-float       -c -o arch/arm/plat-s5p64xx/sleep.o arch/arm/plat-s5p64xx/sleep.S

deps_arch/arm/plat-s5p64xx/sleep.o := \
  arch/arm/plat-s5p64xx/sleep.S \
  /root/Desktop/Dharam/Kernel/arch/arm/include/asm/unified.h \
    $(wildcard include/config/arm/asm/unified.h) \
    $(wildcard include/config/thumb2/kernel.h) \
  include/linux/linkage.h \
  include/linux/compiler.h \
    $(wildcard include/config/trace/branch/profiling.h) \
    $(wildcard include/config/profile/all/branches.h) \
    $(wildcard include/config/enable/must/check.h) \
    $(wildcard include/config/enable/warn/deprecated.h) \
  /root/Desktop/Dharam/Kernel/arch/arm/include/asm/linkage.h \
  /root/Desktop/Dharam/Kernel/arch/arm/include/asm/assembler.h \
    $(wildcard include/config/cpu/feroceon.h) \
    $(wildcard include/config/trace/irqflags.h) \
    $(wildcard include/config/smp.h) \
  /root/Desktop/Dharam/Kernel/arch/arm/include/asm/ptrace.h \
    $(wildcard include/config/cpu/endian/be8.h) \
    $(wildcard include/config/arm/thumb.h) \
  /root/Desktop/Dharam/Kernel/arch/arm/include/asm/hwcap.h \
  arch/arm/mach-s5p6442/include/mach/map.h \
  arch/arm/plat-s5p/include/plat/map-base.h \
  arch/arm/plat-s5p64xx/include/plat/regs-gpio.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-a0.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-a1.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-b.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-c0.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-c1.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-d0.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-d1.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-e0.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-e1.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-f0.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-f1.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-f2.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-f3.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-g0.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-g1.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-g2.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-j0.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-j1.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-j2.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-j3.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-j4.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-h0.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-h1.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-h2.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-h3.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-mp01.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-mp02.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-mp03.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-mp04.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-mp05.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-mp06.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-mp07.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-mp10.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-mp11.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-mp12.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-mp13.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-mp14.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-mp15.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-mp16.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-mp17.h \
  arch/arm/plat-s5p64xx/include/plat/gpio-bank-mp18.h \

arch/arm/plat-s5p64xx/sleep.o: $(deps_arch/arm/plat-s5p64xx/sleep.o)

$(deps_arch/arm/plat-s5p64xx/sleep.o):

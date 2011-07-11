	.arch armv6k
	.fpu softvfp
	.eabi_attribute 20, 1
	.eabi_attribute 21, 1
	.eabi_attribute 23, 3
	.eabi_attribute 24, 1
	.eabi_attribute 25, 1
	.eabi_attribute 26, 2
	.eabi_attribute 30, 2
	.eabi_attribute 18, 4
	.file	"bounds.c"
@ GNU C (Sourcery G++ Lite 2011.03-42) version 4.5.2 (arm-none-eabi)
@	compiled by GNU C version 4.3.2, GMP version 4.3.2, MPFR version 2.4.2, MPC version 0.8.1
@ GGC heuristics: --param ggc-min-expand=100 --param ggc-min-heapsize=131072
@ options passed:  -nostdinc -Iinclude
@ -I/root/Desktop/Dharam/Kernel/arch/arm/include
@ -Iarch/arm/mach-s5p6442/include -Iarch/arm/plat-s5p64xx/include
@ -Iarch/arm/plat-s5p/include -iprefix
@ /root/CodeSourcery/Sourcery_G++_Lite/bin/../lib/gcc/arm-none-eabi/4.5.2/
@ -isysroot /root/CodeSourcery/Sourcery_G++_Lite/bin/../arm-none-eabi
@ -D__USES_INITFINI__ -D__KERNEL__ -D__LINUX_ARM_ARCH__=6 -Uarm
@ -DKBUILD_STR(s)=#s -DKBUILD_BASENAME=KBUILD_STR(bounds)
@ -DKBUILD_MODNAME=KBUILD_STR(bounds) -D__CS_SOURCERYGXX_MAJ__=2011
@ -D__CS_SOURCERYGXX_MIN__=3 -D__CS_SOURCERYGXX_REV__=42 -isystem
@ /root/CodeSourcery/Sourcery_G++_Lite/bin/../lib/gcc/arm-none-eabi/4.5.2/include
@ -include include/linux/autoconf.h -MD kernel/.bounds.s.d kernel/bounds.c
@ -mlittle-endian -marm -mapcs -mno-sched-prolog -mabi=aapcs-linux
@ -mno-thumb-interwork -march=armv6k -mtune=arm1136j-s -msoft-float
@ -auxbase-strip kernel/bounds.s -O2 -Wall -Wundef -Wstrict-prototypes
@ -Wno-trigraphs -Werror-implicit-function-declaration -Wno-format-security
@ -Wframe-larger-than=1024 -Wdeclaration-after-statement -Wno-pointer-sign
@ -p -fno-strict-aliasing -fno-common -fno-delete-null-pointer-checks
@ -fsingle-precision-constant -fno-reorder-blocks -fno-tree-ch
@ -funwind-tables -fno-stack-protector -fno-omit-frame-pointer
@ -fno-optimize-sibling-calls -fno-strict-overflow -fno-dwarf2-cfi-asm
@ -fconserve-stack -fverbose-asm -fremove-local-statics
@ options enabled:  -falign-loops -fargument-alias -fauto-inc-dec
@ -fbranch-count-reg -fcaller-saves -fcprop-registers -fcrossjumping
@ -fcse-follow-jumps -fdefer-pop -fearly-inlining
@ -feliminate-unused-debug-types -fexpensive-optimizations
@ -fextension-elimination -fforward-propagate -ffunction-cse -fgcse
@ -fgcse-lm -fguess-branch-probability -fident -fif-conversion
@ -fif-conversion2 -findirect-inlining -finline
@ -finline-functions-called-once -finline-small-functions -fipa-cp
@ -fipa-pure-const -fipa-reference -fipa-sra -fira-share-save-slots
@ -fira-share-spill-slots -fivopts -fkeep-static-consts
@ -fleading-underscore -fmath-errno -fmerge-constants -fmerge-debug-strings
@ -fmove-loop-invariants -foptimize-register-move -fpeephole -fpeephole2
@ -fprofile -fpromote-loop-indices -freg-struct-return -fregmove
@ -frename-registers -freorder-functions -frerun-cse-after-loop
@ -fsched-critical-path-heuristic -fsched-dep-count-heuristic
@ -fsched-group-heuristic -fsched-interblock -fsched-last-insn-heuristic
@ -fsched-rank-heuristic -fsched-spec -fsched-spec-insn-heuristic
@ -fsched-stalled-insns-dep -fschedule-insns -fschedule-insns2
@ -fsection-anchors -fshow-column -fsigned-zeros
@ -fsingle-precision-constant -fsplit-ivs-in-unroller -fsplit-wide-types
@ -fstrict-volatile-bitfields -fthread-jumps -ftoplevel-reorder
@ -ftrapping-math -ftree-builtin-call-dce -ftree-ccp -ftree-copy-prop
@ -ftree-copyrename -ftree-cselim -ftree-dce -ftree-dominator-opts
@ -ftree-dse -ftree-forwprop -ftree-fre -ftree-if-to-switch-conversion
@ -ftree-loop-im -ftree-loop-ivcanon -ftree-loop-optimize
@ -ftree-parallelize-loops= -ftree-phiprop -ftree-pre -ftree-pta
@ -ftree-reassoc -ftree-scev-cprop -ftree-sink -ftree-slp-vectorize
@ -ftree-sra -ftree-switch-conversion -ftree-ter -ftree-vect-loop-version
@ -ftree-vrp -funit-at-a-time -funroll-loops -funwind-tables -fverbose-asm
@ -fweb -fzero-initialized-in-bss -mapcs-frame -mlittle-endian
@ -munaligned-access

@ Compiler executable checksum: f905e5265c5e2bdb166d278b4b873d0b

	.text
	.align	2
	.global	foo
	.type	foo, %function
foo:
	.fnstart
.LFB0:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0
	.movsp ip
	mov	ip, sp	@,
	stmfd	sp!, {fp, ip, lr, pc}	@,
	.pad #4
	.save {fp, ip, lr}
	.setfp fp, ip, #-4
	sub	fp, ip, #4	@,,
	push	{lr}
	bl	__gnu_mcount_nc
@ 16 "kernel/bounds.c" 1
	
->NR_PAGEFLAGS #23 __NR_PAGEFLAGS	@
@ 0 "" 2
@ 17 "kernel/bounds.c" 1
	
->MAX_NR_ZONES #2 __MAX_NR_ZONES	@
@ 0 "" 2
	ldmfd	sp, {fp, sp, pc}	@
	.fnend
	.size	foo, .-foo
	.ident	"GCC: (Sourcery G++ Lite 2011.03-42) 4.5.2"

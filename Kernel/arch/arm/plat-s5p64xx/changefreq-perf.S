/* linux/arch/arm/plat-s5p64xx/changefreq.S
 *
 * Copyright 2009 Samsung Electronics
 *	
 *	http://samsung.samsungsemi.com/
 *
 * S5P6422 CPU clock change code
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/linkage.h>
#include <asm/assembler.h>
#include <mach/map.h>

#define I_Bit	(0x80)
#define F_Bit	(0x40)
#define R1_BP	(0x800)	

	.text

ENTRY(s5p6442_preclock)

	stmfd	sp!, {r0-r8, r14}

	bl		System_DisableIRQ
	bl		System_EnableBP

        ldr     r8, =S3C_VA_SYS		         @ 0xE0100000
 
        /* Set Lock Time */
        ldr     r1, =0xe10                      @ Locktime : 0xe10 = 3600
        str     r1, [r8, #0x000]                @ APLL_LOCK
        //str     r1, [r8, #0x008]                @ MPLL_LOCK
        //str     r1, [r8, #0x010]                @ EPLL_LOCK
        //str     r1, [r8, #0x020]                @ VPLL_LOCK

	ldmfd   	sp!, {r0-r8, r14}
	bx		lr

ENTRY(s5p6442_postclock)

	stmfd	sp!, {r0-r8, r14} 

	ldr     r8, =S3C_VA_SYS		         @ 0xE0100000
        /* Set Source Clock */
        ldr     r1, =0x00001111                    @ A, M, E, VPLL Muxing
        str     r1, [r8, #0x200]                @ CLK_SRC0
 
        /* wait at least 200us to stablize all clock */
        mov     r2, #0x10000
1:      subs    r2, r2, #1
        bne     1b
 
	bl		System_DisableBP
	bl		System_EnableIRQ

	ldmfd   	sp!, {r0-r8, r14}
	bx		lr

	/*---------------------------------
	 *	s5p6442_changeDivider
	 *--------------------------------- */
ENTRY(s5p6442_changeDivider)

	stmfd	sp!, {r0-r5, r14}

	mov		r5, r0
//	bl		System_DisableIRQ
//	bl		System_EnableBP

	mov		r2, #0
	mov		r3, #0
loopcd:
	add 		r3, r3, #1
	mov 		r4, #0
	mcr 		p15, 0, r2, c7, c10, 4 /* data synchronization barrier instruction */
	mcr 		p15, 0, r2, c7, c10, 5 /* data memory barrier operation */
	cmp 		r3, #2
	streq 		r5, [r1]

loop1000:
	add 		r4, r4, #1
	cmp 		r4, #0x100
	bne 		loop1000
	cmp 		r3, #2
	bne 		loopcd

//	bl		System_DisableBP
//	bl		System_EnableIRQ

	ldmfd   	sp!, {r0-r5, r14}
	bx		lr
		
	/*---------------------
	 *	Disable IRQ
	 *-------------------- */
System_DisableIRQ:
	mrs		r0,cpsr
	orr		r0,r0,#I_Bit
	msr		cpsr_cxsf,r0
	bx		lr

	/*---------------------
	 *	Enable IRQ
 	 *-------------------- */
System_EnableIRQ:
	mrs		r0,cpsr
	bic		r0,r0,#I_Bit
	msr		cpsr_cxsf,r0
	bx		lr

	/*---------------------------------
	 *	Enable Branch Prediction
	 *-------------------------------- */
System_EnableBP:
	mrc		p15,0,r0,c1,c0,0
	orr		r0,r0,#R1_BP
	mcr		p15,0,r0,c1,c0,0
	bx		lr

	/*---------------------------------
	 *	Disable Branch Prediction
	 *-------------------------------- */
System_DisableBP:
	mrc		p15,0,r0,c1,c0,0
	bic		r0,r0,#R1_BP
	mcr		p15,0,r0,c1,c0,0
	bx		lr

	.data

	.end


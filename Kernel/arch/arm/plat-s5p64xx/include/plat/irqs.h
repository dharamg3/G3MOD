/* linux/arch/arm/plat-s5p64xx/include/plat/irqs.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *      Ben Dooks <ben@simtec.co.uk>
 *      http://armlinux.simtec.co.uk/
 *
 * S5P64XX - Common IRQ support
 */

#ifndef __ASM_PLAT_S5P64XX_IRQS_H
#define __ASM_PLAT_S5P64XX_IRQS_H __FILE__

/* we keep the first set of CPU IRQs out of the range of
 * the ISA space, so that the PC104 has them to itself
 * and we don't end up having to do horrible things to the
 * standard ISA drivers....
 *
 * note, since we're using the VICs, our start must be a
 * mulitple of 32 to allow the common code to work
 */

#define S3C_IRQ_OFFSET	(32)

#define S3C_IRQ(x)	((x) + S3C_IRQ_OFFSET)

#define S3C_VIC0_BASE	S3C_IRQ(0)
#define S3C_VIC1_BASE	S3C_IRQ(32)
#define S3C_VIC2_BASE	S3C_IRQ(64)

/* UART interrupts, each UART has 4 intterupts per channel so
 * use the space between the ISA and S3C main interrupts. Note, these
 * are not in the same order as the S3C24XX series! */

#define IRQ_S3CUART_BASE0	(16)
#define IRQ_S3CUART_BASE1	(20)
#define IRQ_S3CUART_BASE2	(24)

#define UART_IRQ_RXD		(0)
#define UART_IRQ_ERR		(1)
#define UART_IRQ_TXD		(2)
#define UART_IRQ_MODEM		(3)

#define IRQ_S3CUART_RX0		(IRQ_S3CUART_BASE0 + UART_IRQ_RXD)
#define IRQ_S3CUART_TX0		(IRQ_S3CUART_BASE0 + UART_IRQ_TXD)
#define IRQ_S3CUART_ERR0	(IRQ_S3CUART_BASE0 + UART_IRQ_ERR)

#define IRQ_S3CUART_RX1		(IRQ_S3CUART_BASE1 + UART_IRQ_RXD)
#define IRQ_S3CUART_TX1		(IRQ_S3CUART_BASE1 + UART_IRQ_TXD)
#define IRQ_S3CUART_ERR1	(IRQ_S3CUART_BASE1 + UART_IRQ_ERR)

#define IRQ_S3CUART_RX2		(IRQ_S3CUART_BASE2 + UART_IRQ_RXD)
#define IRQ_S3CUART_TX2		(IRQ_S3CUART_BASE2 + UART_IRQ_TXD)
#define IRQ_S3CUART_ERR2	(IRQ_S3CUART_BASE2 + UART_IRQ_ERR)

/* VIC based IRQs */

#define S5P64XX_IRQ_VIC0(x)	(S3C_VIC0_BASE + (x))
#define S5P64XX_IRQ_VIC1(x)	(S3C_VIC1_BASE + (x))
#define S5P64XX_IRQ_VIC2(x)	(S3C_VIC2_BASE + (x))

/* VIC0 */
#define IRQ_EINT0 	S5P64XX_IRQ_VIC0(0)
#define IRQ_EINT1 	S5P64XX_IRQ_VIC0(1)
#define IRQ_EINT2 	S5P64XX_IRQ_VIC0(2)
#define IRQ_EINT3 	S5P64XX_IRQ_VIC0(3)
#define IRQ_EINT4 	S5P64XX_IRQ_VIC0(4)
#define IRQ_EINT5 	S5P64XX_IRQ_VIC0(5)
#define IRQ_EINT6 	S5P64XX_IRQ_VIC0(6)
#define IRQ_EINT7 	S5P64XX_IRQ_VIC0(7)
#define IRQ_EINT8 	S5P64XX_IRQ_VIC0(8)
#define IRQ_EINT9 	S5P64XX_IRQ_VIC0(9)
#define IRQ_EINT10 	S5P64XX_IRQ_VIC0(10)
#define IRQ_EINT11 	S5P64XX_IRQ_VIC0(11)
#define IRQ_EINT12 	S5P64XX_IRQ_VIC0(12)
#define IRQ_EINT13 	S5P64XX_IRQ_VIC0(13)
#define IRQ_EINT14 	S5P64XX_IRQ_VIC0(14)
#define IRQ_EINT15 	S5P64XX_IRQ_VIC0(15)
#define IRQ_EINT16_31 	S5P64XX_IRQ_VIC0(16)
#define IRQ_BATF 	S5P64XX_IRQ_VIC0(17)
#define IRQ_MDMA 	S5P64XX_IRQ_VIC0(18)
#define IRQ_PDMA 	S5P64XX_IRQ_VIC0(19)
//#define IRQ_Reserved 	S5P64XX_IRQ_VIC0(20)
#define IRQ_TIMER0 	S5P64XX_IRQ_VIC0(21)
#define IRQ_TIMER1 	S5P64XX_IRQ_VIC0(22)
#define IRQ_TIMER2 	S5P64XX_IRQ_VIC0(23)
#define IRQ_TIMER3 	S5P64XX_IRQ_VIC0(24)
#define IRQ_TIMER4 	S5P64XX_IRQ_VIC0(25)
#define IRQ_SYSTIMER 	S5P64XX_IRQ_VIC0(26)
#define IRQ_WDT 	S5P64XX_IRQ_VIC0(27)
#define IRQ_RTC_ALARM 	S5P64XX_IRQ_VIC0(28)
#define IRQ_RTC_TIC 	S5P64XX_IRQ_VIC0(29)
#define IRQ_GPIOINT 	S5P64XX_IRQ_VIC0(30)
//#define IRQ_Reserved 	S5P64XX_IRQ_VIC0(31)
#define IRQ_VIC0_UNUSED	(1<<20)|(1<<31)

/* VIC1 */
#define IRQ_nPMUIRQ 	S5P64XX_IRQ_VIC1(0)
//#define IRQ_Reserved 	S5P64XX_IRQ_VIC1(1)
//#define IRQ_Reserved 	S5P64XX_IRQ_VIC1(2)
//#define IRQ_Reserved 	S5P64XX_IRQ_VIC1(3)
//#define IRQ_Reserved 	S5P64XX_IRQ_VIC1(4)
//#define IRQ_Reserved 	S5P64XX_IRQ_VIC1(5)
//#define IRQ_Reserved 	S5P64XX_IRQ_VIC1(6)
#define IRQ_ONENAND 	S5P64XX_IRQ_VIC1(7)
//#define IRQ_Reserved 	S5P64XX_IRQ_VIC1(8)
//#define IRQ_Reserved 	S5P64XX_IRQ_VIC1(9)
#define IRQ_UART0 	S5P64XX_IRQ_VIC1(10)
#define IRQ_UART1 	S5P64XX_IRQ_VIC1(11)
#define IRQ_UART2 	S5P64XX_IRQ_VIC1(12)
//#define IRQ_Reserved 	S5P64XX_IRQ_VIC1(13)
//#define IRQ_Reserved 	S5P64XX_IRQ_VIC1(14)
#define IRQ_SPI0 	S5P64XX_IRQ_VIC1(15)
//#define IRQ_Reserved 	S5P64XX_IRQ_VIC1(16)
//#define IRQ_Reserved 	S5P64XX_IRQ_VIC1(17)
//#define IRQ_Reserved 	S5P64XX_IRQ_VIC1(18)
#define IRQ_I2C0 	S5P64XX_IRQ_VIC1(19)
#define IRQ_I2C1 	S5P64XX_IRQ_VIC1(20)
#define IRQ_I2C2 	S5P64XX_IRQ_VIC1(21)
//#define IRQ_Reserved 	S5P64XX_IRQ_VIC1(22)
//#define IRQ_Reserved 	S5P64XX_IRQ_VIC1(23)
#define IRQ_OTG 	S5P64XX_IRQ_VIC1(24)
#define IRQ_MSM 	S5P64XX_IRQ_VIC1(25)
#define IRQ_HSMMC0 	S5P64XX_IRQ_VIC1(26)
#define IRQ_HSMMC1 	S5P64XX_IRQ_VIC1(27)
#define IRQ_HSMMC2 	S5P64XX_IRQ_VIC1(28)
#define IRQ_COMMRX 	S5P64XX_IRQ_VIC1(29)
#define IRQ_COMMTX 	S5P64XX_IRQ_VIC1(30)
//#define IRQ_Reserved 	S5P64XX_IRQ_VIC1(31)
#define IRQ_VIC1_UNUSED	(0x3f<<1)|(0x3<<8)|(0x3<<13)|(0x7<<16)|(0x3<<22)|(0x1<<31)

/* VIC2 */
#define IRQ_LCD0 	S5P64XX_IRQ_VIC2(0)
#define IRQ_LCD1 	S5P64XX_IRQ_VIC2(1)
#define IRQ_LCD2 	S5P64XX_IRQ_VIC2(2)
#define IRQ_LCD3 	S5P64XX_IRQ_VIC2(3)
#define IRQ_ROTATOR 	S5P64XX_IRQ_VIC2(4)
#define IRQ_FIMC0 	S5P64XX_IRQ_VIC2(5)
#define IRQ_FIMC1 	S5P64XX_IRQ_VIC2(6)
#define IRQ_FIMC2 	S5P64XX_IRQ_VIC2(7)
#define IRQ_JPEG 	S5P64XX_IRQ_VIC2(8)
#define IRQ_2D 		S5P64XX_IRQ_VIC2(9)
#define IRQ_3D 		S5P64XX_IRQ_VIC2(10)
#define IRQ_Mixer 	S5P64XX_IRQ_VIC2(11)
//#define IRQ_Reserved 	S5P64XX_IRQ_VIC2(12)
//#define IRQ_Reserved 	S5P64XX_IRQ_VIC2(13)
#define IRQ_MFC 	S5P64XX_IRQ_VIC2(14)
#define IRQ_TVENC 	S5P64XX_IRQ_VIC2(15)
#define IRQ_I2S0 	S5P64XX_IRQ_VIC2(16)
#define IRQ_I2S1 	S5P64XX_IRQ_VIC2(17)
//#define IRQ_Reserved 	S5P64XX_IRQ_VIC2(18)
#define IRQ_RP 		S5P64XX_IRQ_VIC2(19)
#define IRQ_PCM0 	S5P64XX_IRQ_VIC2(20)
#define IRQ_PCM1 	S5P64XX_IRQ_VIC2(21)
//#define IRQ_Reserved 	S5P64XX_IRQ_VIC2(22)
#define IRQ_ADC 	S5P64XX_IRQ_VIC2(23)
#define IRQ_PENDN 	S5P64XX_IRQ_VIC2(24)
#define IRQ_KEYPAD 	S5P64XX_IRQ_VIC2(25)
//#define IRQ_Reserved 	S5P64XX_IRQ_VIC2(26)
#define IRQ_SSS_INT 	S5P64XX_IRQ_VIC2(27)
#define IRQ_SSS_HASH 	S5P64XX_IRQ_VIC2(28)
//#define IRQ_Reserved 	S5P64XX_IRQ_VIC2(29)
//#define IRQ_Reserved 	S5P64XX_IRQ_VIC2(30)
#define IRQ_VIC_END 	S5P64XX_IRQ_VIC2(31)	// Just define end of IRQ
#define IRQ_VIC2_UNUSED	(0x3<<12)|(0x1<<18)|(0x1<<22)|(0x1<<26)|(0x7<<29)  //rama
//#define IRQ_VIC2_UNUSED	(0x1<<9)|(0x3<<12)|(0x1<<18)|(0x1<<22)|(0x1<<26)|(0x7<<29)

/* Backward compatibility */
#define IRQ_DISPCON0	IRQ_LCD0
#define IRQ_DISPCON1	IRQ_LCD1
#define IRQ_DISPCON2	IRQ_LCD2
#define IRQ_DISPCON3	IRQ_LCD3
#define IRQ_TC		IRQ_PENDN
#define IRQ_IIC		IRQ_I2C0
#define IRQ_IIC1	IRQ_I2C1
#define IRQ_IIC2	IRQ_I2C2

#define IRQ_TIMER0_VIC	IRQ_TIMER0
#define IRQ_TIMER1_VIC	IRQ_TIMER1
#define IRQ_TIMER2_VIC	IRQ_TIMER2
#define IRQ_TIMER3_VIC	IRQ_TIMER3
#define IRQ_TIMER4_VIC	IRQ_TIMER4

#if 1
/* Since the IRQ_EINT(x) are a linear mapping on current s5p64xx series
 * we just defined them as an IRQ_EINT(x) macro from S3C_IRQ_EINT_BASE
 * which we place after the pair of VICs. */
/*rama: this code needs to be verified */
#define S3C_IRQ_EINT_BASE       (IRQ_VIC_END + 1)

#define S3C_EINT(x)		((x) + S3C_IRQ_EINT_BASE)
#define IRQ_EINT(x)		S3C_EINT(x)

/* Next the external interrupt groups. These are similar to the IRQ_EINT(x)
 * that they are sourced from the GPIO pins but with a different scheme for
 * priority and source indication.
 *
 * The IRQ_EINT(x) can be thought of as 'group 0' of the available GPIO
 * interrupts, but for historical reasons they are kept apart from these
 * next interrupts.
 *
 * Use IRQ_EINT_GROUP(group, offset) to get the number for use in the
 * machine specific support files.
 */

#define IRQ_EINT_GROUP1_NR	(8)   //GPA0
#define IRQ_EINT_GROUP2_NR	(2)   //GPA1
#define IRQ_EINT_GROUP3_NR	(4)   //GPB
#define IRQ_EINT_GROUP4_NR	(5)   //GPC1
#define IRQ_EINT_GROUP5_NR	(2)   //GPD0
#define IRQ_EINT_GROUP6_NR	(6)   //GPD1
#define IRQ_EINT_GROUP7_NR	(8)   //GPE0
#define IRQ_EINT_GROUP8_NR	(5)   //GPE1
#define IRQ_EINT_GROUP9_NR	(8)   //GPF0
#define IRQ_EINT_GROUP10_NR        (8)   //GPF1
#define IRQ_EINT_GROUP11_NR        (8)   //GPF2
#define IRQ_EINT_GROUP12_NR        (6)   //GPF3
#define IRQ_EINT_GROUP13_NR        (7)   //GPG0
#define IRQ_EINT_GROUP14_NR        (7)   //GPG1
#define IRQ_EINT_GROUP15_NR        (7)   //GPG2
#define IRQ_EINT_GROUP16_NR        (8)   //GPJ0
#define IRQ_EINT_GROUP17_NR        (6)   //GPJ1
#define IRQ_EINT_GROUP18_NR        (8)   //GPJ2
#define IRQ_EINT_GROUP19_NR        (8)   //GPJ3
#define IRQ_EINT_GROUP20_NR        (5)   //GPJ4
//#define IRQ_EINT_GROUP21_NR        (5)   //GPJ4


#define IRQ_EINT_GROUP_BASE	S3C_EINT(32)
#define IRQ_EINT_GROUP1_BASE	(IRQ_EINT_GROUP_BASE + 0x00)
#define IRQ_EINT_GROUP2_BASE	(IRQ_EINT_GROUP1_BASE + IRQ_EINT_GROUP1_NR)
#define IRQ_EINT_GROUP3_BASE	(IRQ_EINT_GROUP2_BASE + IRQ_EINT_GROUP2_NR)
#define IRQ_EINT_GROUP4_BASE	(IRQ_EINT_GROUP3_BASE + IRQ_EINT_GROUP3_NR)
#define IRQ_EINT_GROUP5_BASE	(IRQ_EINT_GROUP4_BASE + IRQ_EINT_GROUP4_NR)
#define IRQ_EINT_GROUP6_BASE	(IRQ_EINT_GROUP5_BASE + IRQ_EINT_GROUP5_NR)
#define IRQ_EINT_GROUP7_BASE	(IRQ_EINT_GROUP6_BASE + IRQ_EINT_GROUP6_NR)
#define IRQ_EINT_GROUP8_BASE	(IRQ_EINT_GROUP7_BASE + IRQ_EINT_GROUP7_NR)
#define IRQ_EINT_GROUP9_BASE	(IRQ_EINT_GROUP8_BASE + IRQ_EINT_GROUP8_NR)
#define IRQ_EINT_GROUP10_BASE    (IRQ_EINT_GROUP9_BASE + IRQ_EINT_GROUP9_NR)
#define IRQ_EINT_GROUP11_BASE    (IRQ_EINT_GROUP10_BASE + IRQ_EINT_GROUP10_NR)
#define IRQ_EINT_GROUP12_BASE    (IRQ_EINT_GROUP11_BASE + IRQ_EINT_GROUP11_NR)
#define IRQ_EINT_GROUP13_BASE    (IRQ_EINT_GROUP12_BASE + IRQ_EINT_GROUP12_NR)
#define IRQ_EINT_GROUP14_BASE    (IRQ_EINT_GROUP13_BASE + IRQ_EINT_GROUP13_NR)
#define IRQ_EINT_GROUP15_BASE    (IRQ_EINT_GROUP14_BASE + IRQ_EINT_GROUP14_NR)
#define IRQ_EINT_GROUP16_BASE    (IRQ_EINT_GROUP15_BASE + IRQ_EINT_GROUP15_NR)
#define IRQ_EINT_GROUP17_BASE    (IRQ_EINT_GROUP16_BASE + IRQ_EINT_GROUP16_NR)
#define IRQ_EINT_GROUP18_BASE    (IRQ_EINT_GROUP17_BASE + IRQ_EINT_GROUP17_NR)
#define IRQ_EINT_GROUP19_BASE    (IRQ_EINT_GROUP18_BASE + IRQ_EINT_GROUP18_NR)
#define IRQ_EINT_GROUP20_BASE    (IRQ_EINT_GROUP19_BASE + IRQ_EINT_GROUP19_NR)
//#define IRQ_EINT_GROUP21_BASE    (IRQ_EINT_GROUP20_BASE + IRQ_EINT_GROUP20_NR)


#define IRQ_EINT_GROUP(group, no)	(IRQ_EINT_GROUP##group##_BASE + (no))

/* Set the default NR_IRQS */
#define NR_IRQS (IRQ_EINT_GROUP20_BASE + IRQ_EINT_GROUP20_NR + 1)
//#define NR_IRQS	(IRQ_EINT_GROUP21_BASE + IRQ_EINT_GROUP21_NR + 1)

#else

#define S3C_IRQ_EINT_BASE	(IRQ_VIC_END + 1)

#define S3C_EINT(x)		((x) + S3C_IRQ_EINT_BASE)
#define IRQ_EINT(x)		S3C_EINT(x)

#define NR_IRQS 		(IRQ_EINT(31)+1)

#endif


#endif /* __ASM_PLAT_S5P64XX_IRQS_H */


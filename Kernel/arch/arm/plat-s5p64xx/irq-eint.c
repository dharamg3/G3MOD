/* arch/arm/plat-s5p64xx/irq-eint.c
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *      Ben Dooks <ben@simtec.co.uk>
 *      http://armlinux.simtec.co.uk/
 *
 * S5P64XX - Interrupt handling for IRQ_EINT(x)
 * @History: External Interrupt Gropus(GPIO)
 *  * rama <v.meka@samsung.com> <11-16-2009>
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>

#include <asm/hardware/vic.h>

#include <plat/regs-irqtype.h>

#include <mach/map.h>
#include <plat/cpu.h>

#include <mach/gpio.h>
#include <mach/regs-irq.h>
#include <plat/regs-gpio.h>

#include <plat/gpio-cfg.h>

#define NUM_OF_GROUPS 20
static unsigned int IRQ_EINT_BASE[NUM_OF_GROUPS + 1] = 
		{ IRQ_EINT_GROUP1_BASE, IRQ_EINT_GROUP2_BASE, IRQ_EINT_GROUP3_BASE, IRQ_EINT_GROUP4_BASE,
		  IRQ_EINT_GROUP5_BASE, IRQ_EINT_GROUP6_BASE, IRQ_EINT_GROUP7_BASE, IRQ_EINT_GROUP8_BASE,
		  IRQ_EINT_GROUP9_BASE, IRQ_EINT_GROUP10_BASE, IRQ_EINT_GROUP11_BASE, IRQ_EINT_GROUP12_BASE,
		  IRQ_EINT_GROUP13_BASE, IRQ_EINT_GROUP14_BASE, IRQ_EINT_GROUP15_BASE, IRQ_EINT_GROUP16_BASE,
		  IRQ_EINT_GROUP17_BASE, IRQ_EINT_GROUP18_BASE, IRQ_EINT_GROUP19_BASE, IRQ_EINT_GROUP20_BASE,
		  IRQ_EINT_GROUP20_BASE + IRQ_EINT_GROUP20_NR};

static unsigned int *S5P64XX_GPIO_EINT_CON[NUM_OF_GROUPS] =
		{S5P64XX_GROUP1_INT_CON, S5P64XX_GROUP2_INT_CON, S5P64XX_GROUP3_INT_CON, S5P64XX_GROUP4_INT_CON,
		S5P64XX_GROUP5_INT_CON, S5P64XX_GROUP6_INT_CON, S5P64XX_GROUP7_INT_CON, S5P64XX_GROUP8_INT_CON,
		S5P64XX_GROUP9_INT_CON, S5P64XX_GROUP10_INT_CON, S5P64XX_GROUP11_INT_CON, S5P64XX_GROUP12_INT_CON,
		S5P64XX_GROUP13_INT_CON, S5P64XX_GROUP14_INT_CON, S5P64XX_GROUP15_INT_CON, S5P64XX_GROUP16_INT_CON,
		S5P64XX_GROUP17_INT_CON, S5P64XX_GROUP18_INT_CON, S5P64XX_GROUP19_INT_CON, S5P64XX_GROUP20_INT_CON};

static unsigned int *S5P64XX_GPIO_EINT_MASK[NUM_OF_GROUPS] =
		{S5P64XX_GROUP1_INT_MASK, S5P64XX_GROUP2_INT_MASK, S5P64XX_GROUP3_INT_MASK, S5P64XX_GROUP4_INT_MASK,
		S5P64XX_GROUP5_INT_MASK, S5P64XX_GROUP6_INT_MASK, S5P64XX_GROUP7_INT_MASK, S5P64XX_GROUP8_INT_MASK,
		S5P64XX_GROUP9_INT_MASK, S5P64XX_GROUP10_INT_MASK, S5P64XX_GROUP11_INT_MASK, S5P64XX_GROUP12_INT_MASK,
		S5P64XX_GROUP13_INT_MASK, S5P64XX_GROUP14_INT_MASK, S5P64XX_GROUP15_INT_MASK, S5P64XX_GROUP16_INT_MASK,
		S5P64XX_GROUP17_INT_MASK, S5P64XX_GROUP18_INT_MASK, S5P64XX_GROUP19_INT_MASK, S5P64XX_GROUP20_INT_MASK};

static unsigned int *S5P64XX_GPIO_EINT_PEND[NUM_OF_GROUPS] =
		{S5P64XX_GROUP1_INT_PEND, S5P64XX_GROUP2_INT_PEND, S5P64XX_GROUP3_INT_PEND, S5P64XX_GROUP4_INT_PEND,
		S5P64XX_GROUP5_INT_PEND, S5P64XX_GROUP6_INT_PEND, S5P64XX_GROUP7_INT_PEND, S5P64XX_GROUP8_INT_PEND,
		S5P64XX_GROUP9_INT_PEND, S5P64XX_GROUP10_INT_PEND, S5P64XX_GROUP11_INT_PEND, S5P64XX_GROUP12_INT_PEND,
		S5P64XX_GROUP13_INT_PEND, S5P64XX_GROUP14_INT_PEND, S5P64XX_GROUP15_INT_PEND, S5P64XX_GROUP16_INT_PEND,
		S5P64XX_GROUP17_INT_PEND, S5P64XX_GROUP18_INT_PEND, S5P64XX_GROUP19_INT_PEND, S5P64XX_GROUP20_INT_PEND};

static inline void s3c_irq_eint_mask(unsigned int irq)
{
	u32 mask;
	
	mask = __raw_readl(S5P64XX_EINTMASK(eint_mask_reg(irq)));
	mask |= eint_irq_to_bit(irq);
	__raw_writel(mask, S5P64XX_EINTMASK(eint_mask_reg(irq)));
	
}

static void s3c_irq_eint_unmask(unsigned int irq)
{
	u32 mask;
	
	mask = __raw_readl(S5P64XX_EINTMASK(eint_mask_reg(irq)));
	mask &= ~(eint_irq_to_bit(irq));
	__raw_writel(mask, S5P64XX_EINTMASK(eint_mask_reg(irq)));
}

static inline void s3c_irq_eint_ack(unsigned int irq)
{
	__raw_writel(eint_irq_to_bit(irq), S5P64XX_EINTPEND(eint_pend_reg(irq)));
}

static void s3c_irq_eint_maskack(unsigned int irq)
{
	/* compiler should in-line these */
	s3c_irq_eint_mask(irq);
	s3c_irq_eint_ack(irq);
}

static int s3c_irq_eint_set_type(unsigned int irq, unsigned int type)
{
	int offs = eint_offset(irq);
	int shift;
	u32 ctrl, mask;
	u32 newvalue = 0;

	switch (type) {
	case IRQ_TYPE_NONE:
		printk(KERN_WARNING "No edge setting!\n");
		break;

	case IRQ_TYPE_EDGE_RISING:
		newvalue = S5P_EXTINT_RISEEDGE;
		break;

	case IRQ_TYPE_EDGE_FALLING:
		newvalue = S5P_EXTINT_FALLEDGE;
		break;

	case IRQ_TYPE_EDGE_BOTH:
		newvalue = S5P_EXTINT_BOTHEDGE;
		break;

	case IRQ_TYPE_LEVEL_LOW:
		newvalue = S5P_EXTINT_LOWLEV;
		break;

	case IRQ_TYPE_LEVEL_HIGH:
		newvalue = S5P_EXTINT_HILEV;
		break;

	default:
		printk(KERN_ERR "No such irq type %d", type);
		return -1;
	}

	shift = (offs & 0x7) * 4;
	mask = 0x7 << shift;

	ctrl = __raw_readl(S5P64XX_EINTCON(eint_conf_reg(irq)));
	ctrl &= ~mask;
	ctrl |= newvalue << shift;
	__raw_writel(ctrl, S5P64XX_EINTCON(eint_conf_reg(irq)));

#if 0
	if((0 <= offs) && (offs < 8))
		s3c_gpio_cfgpin(S5P64XX_GPH0(offs&0x7), 0x2<<((offs&0x7)*4));
	else if((8 <= offs) && (offs < 16))
		s3c_gpio_cfgpin(S5P64XX_GPH1(offs&0x7), 0x2<<((offs&0x7)*4));
	else if((16 <= offs) && (offs < 24))
		s3c_gpio_cfgpin(S5P64XX_GPH2(offs&0x7), 0x2<<((offs&0x7)*4));
	else if((24 <= offs) && (offs < 32))
		s3c_gpio_cfgpin(S5P64XX_GPH3(offs&0x7), 0x2<<((offs&0x7)*4));
	else
		printk(KERN_ERR "No such irq number %d", offs);
#endif
	return 0;
}

#ifdef CONFIG_PM
unsigned int s3c_irqwake_eintallow		= 0xffff0000L;
unsigned int s3c_irqwake_eintmask		= 0xffffffffL;

int s3c_irq_eint_set_wake(unsigned int irq, unsigned int on)
{
	unsigned long irqbit = 1 << (irq - S3C_IRQ_EINT_BASE);

	if (!(s3c_irqwake_eintallow & irqbit))
		return -ENOENT;

	printk(KERN_INFO "wake %s for irq %d\n",
	       on ? "enabled" : "disabled", irq);

	if (!on)
		s3c_irqwake_eintmask |= irqbit;
	else
		s3c_irqwake_eintmask &= ~irqbit;

	return 0;
}
#else
#define s3c_irq_eint_set_wake NULL
#endif

static struct irq_chip s3c_irq_eint = {
	.name		= "s3c-eint",
	.mask		= s3c_irq_eint_mask,
	.unmask		= s3c_irq_eint_unmask,
	.mask_ack	= s3c_irq_eint_maskack,
	.ack		= s3c_irq_eint_ack,
	.set_type	= s3c_irq_eint_set_type,
	.set_wake	= s3c_irq_eint_set_wake,
};

static int s5p_get_eint_group(unsigned int irq)
{
	int group = 0xFFFF;

#if 1
	if((irq < IRQ_EINT_GROUP1_BASE) ||
		(irq >= NR_IRQS)){
			printk(KERN_ERR "%s::unknown interrupt number!!!! irq %d  NR_IRQS %d  IRQ_EINT_GROUP1_BASE %d\n", __FUNCTION__,
										irq, NR_IRQS, IRQ_EINT_GROUP1_BASE);
			return 0xFFFF;
		}
#endif

	if( (irq < IRQ_EINT_GROUP14_BASE) && (irq >= IRQ_EINT_GROUP13_BASE)){
		group = 13; //GPG0
	}
	else if((irq < IRQ_EINT_GROUP19_BASE) && (irq >= IRQ_EINT_GROUP18_BASE)){
		group = 18; //GPJ2
	}
	else if((irq < IRQ_EINT_GROUP2_BASE) && (irq >= IRQ_EINT_GROUP1_BASE)){
		group = 1; //GPA0
	}
	else if((irq < IRQ_EINT_GROUP3_BASE) && (irq >= IRQ_EINT_GROUP2_BASE)){
		group = 2; //GPA1
	}
	else if((irq < IRQ_EINT_GROUP4_BASE) && (irq >= IRQ_EINT_GROUP3_BASE)){
		group = 3; //GPB
	}
	else if((irq < IRQ_EINT_GROUP5_BASE) && (irq >= IRQ_EINT_GROUP4_BASE)){
		group = 4; //GPC1
	}
	else if((irq < IRQ_EINT_GROUP6_BASE) && (irq >= IRQ_EINT_GROUP5_BASE)){
		group = 5; //GPD0
	}
	else if((irq < IRQ_EINT_GROUP7_BASE) && (irq >= IRQ_EINT_GROUP6_BASE)){
		group = 6; //GPD1
	}
	else if((irq < IRQ_EINT_GROUP8_BASE) && (irq >= IRQ_EINT_GROUP7_BASE)){
		group = 7; //GPE0
	}
	else if((irq < IRQ_EINT_GROUP9_BASE) && (irq >= IRQ_EINT_GROUP8_BASE)){
		group = 8; //GPE1
	}
	else if((irq < IRQ_EINT_GROUP10_BASE) && (irq >= IRQ_EINT_GROUP9_BASE)){
		group = 9; //GPF0
	}
	else if((irq < IRQ_EINT_GROUP11_BASE) && (irq >= IRQ_EINT_GROUP10_BASE)){
		group = 10;  //GPF1
	}
	else if((irq < IRQ_EINT_GROUP12_BASE) && (irq >= IRQ_EINT_GROUP11_BASE)){
		group = 11; //GPF2
	}
	else if((irq < IRQ_EINT_GROUP13_BASE) && (irq >= IRQ_EINT_GROUP12_BASE)){
		group = 12;  //GPF3
	}
	else if((irq < IRQ_EINT_GROUP15_BASE) && (irq >= IRQ_EINT_GROUP14_BASE)){
		group = 14; //GPG1
	}
	else if((irq < IRQ_EINT_GROUP16_BASE) && (irq >= IRQ_EINT_GROUP15_BASE)){
		group = 15; //GPG2
	}
	else if((irq < IRQ_EINT_GROUP17_BASE) && (irq >= IRQ_EINT_GROUP16_BASE)){
		group = 16; //GPJ0
	}
	else if((irq < IRQ_EINT_GROUP18_BASE) && (irq >= IRQ_EINT_GROUP17_BASE)){
		group = 17; //GPJ1
	}
	else if((irq < IRQ_EINT_GROUP20_BASE) && (irq >= IRQ_EINT_GROUP19_BASE)){
		group = 19; //GPJ3
	}
	else if((irq < (NR_IRQS - 1)) && (irq >= IRQ_EINT_GROUP20_BASE)){	
		group = 20; //GPJ4	
	}
	
	group = ((group > 0)?(group -1):0);
	

	return group;
	
}

static inline void s5p_irq_eint_mask(unsigned int irq)
{
        u32 mask;
	u32 group;	
	int offs;

	group = s5p_get_eint_group(irq);
	offs  = irq - IRQ_EINT_BASE[group];

        mask = __raw_readl(S5P64XX_GPIO_EINT_MASK[group]);
        mask |= (1 << offs);
        __raw_writel(mask, S5P64XX_GPIO_EINT_MASK[group]);

}

static void s5p_irq_eint_unmask(unsigned int irq)
{
       u32 mask;
	u32 group;	
	int offs;

	group = s5p_get_eint_group(irq);
	offs  = irq - IRQ_EINT_BASE[group];

        mask = __raw_readl(S5P64XX_GPIO_EINT_MASK[group]);
        mask &= ~(1 << offs);
        __raw_writel(mask, S5P64XX_GPIO_EINT_MASK[group]);
}

static inline void s5p_irq_eint_ack(unsigned int irq)
{
	u32 group;	
	int offs;

	group = s5p_get_eint_group(irq);
	offs  = irq - IRQ_EINT_BASE[group];

        __raw_writel((1 << offs), S5P64XX_GPIO_EINT_PEND[group] );
}

static void s5p_irq_eint_maskack(unsigned int irq)
{
        /* compiler should in-line these */
        s5p_irq_eint_mask(irq);
        s5p_irq_eint_ack(irq);
}

static int s5p_irq_eint_set_type(unsigned int irq, unsigned int type)
{
        int offs ;
        int shift;
        u32 ctrl, mask;
        u32 newvalue = 0;
	u32 group;

	group = s5p_get_eint_group(irq);
	offs  = irq - IRQ_EINT_BASE[group];
        
	switch (type) {
        case IRQ_TYPE_NONE:
                printk(KERN_WARNING "No edge setting!\n");
                break;

        case IRQ_TYPE_EDGE_RISING:
                newvalue = S5P_EXTINT_RISEEDGE;
                break;

        case IRQ_TYPE_EDGE_FALLING:
                newvalue = S5P_EXTINT_FALLEDGE;
                break;

        case IRQ_TYPE_EDGE_BOTH:
                newvalue = S5P_EXTINT_BOTHEDGE;
                break;

        case IRQ_TYPE_LEVEL_LOW:
                newvalue = S5P_EXTINT_LOWLEV;
                break;

        case IRQ_TYPE_LEVEL_HIGH:
                newvalue = S5P_EXTINT_HILEV;
                break;

        default:
                printk(KERN_ERR "No such irq type %d", type);
                return -1;
        }

        shift = (offs & 0x7) * 4;
        mask = 0x7 << shift;

        ctrl = __raw_readl(S5P64XX_GPIO_EINT_CON[group]);
        ctrl &= ~mask;
        ctrl |= newvalue << shift;
        __raw_writel(ctrl, S5P64XX_GPIO_EINT_CON[group]);

	return 0;
}
static struct irq_chip s5p_irq_eint = {
        .name           = "s5p-eint",
        .mask           = s5p_irq_eint_mask,
        .unmask         = s5p_irq_eint_unmask,
        .mask_ack       = s5p_irq_eint_maskack,
        .ack            = s5p_irq_eint_ack,
        .set_type       = s5p_irq_eint_set_type,
};



static inline void s5p_irq_demux_eint(unsigned int start, unsigned int end, int group)
{
        u32 status = __raw_readl(S5P64XX_GPIO_EINT_PEND[group]);
        u32 mask = __raw_readl(S5P64XX_GPIO_EINT_MASK[group]);
        unsigned int irq;

        status &= ~mask;
        status &= 0x7F;

	  for (irq = start; irq <= end; irq++){        
                if (status & 1)
                        generic_handle_irq(irq);

                status >>= 1;
        }
}




static void s5p_irq_demux_eint_group1_20(unsigned int irq, struct irq_desc *desc)
{
	u32 group;	
	int offs;
	
	for(group = 0; group < NUM_OF_GROUPS; group++){
        	s5p_irq_demux_eint(IRQ_EINT_BASE[group], (IRQ_EINT_BASE[group + 1] -1), group);
	}
}

/* s3c_irq_demux_eint
 *
 * This function demuxes the IRQ from the group0 external interrupts,
 * from IRQ_EINT(0) to IRQ_EINT(15). It is designed to be inlined into
 * the specific handlers s3c_irq_demux_eintX_Y.
 */
static inline void s3c_irq_demux_eint(unsigned int start, unsigned int end)
{
	u32 status = __raw_readl(S5P64XX_EINTPEND((start >> 3)));
	u32 mask = __raw_readl(S5P64XX_EINTMASK((start >> 3)));
	unsigned int irq;

	status &= ~mask;
	status &= (1 << (end - start + 1)) - 1;

	for (irq = IRQ_EINT(start); irq <= IRQ_EINT(end); irq++) {
		if (status & 1)
			generic_handle_irq(irq);

		status >>= 1;
	}
}

static void s3c_irq_demux_eint16_31(unsigned int irq, struct irq_desc *desc)
{
	s3c_irq_demux_eint(16, 23);
	s3c_irq_demux_eint(24, 31);
}

/*---------------------------- EINT0 ~ EINT15 -------------------------------------*/
static void s3c_irq_vic_eint_mask(unsigned int irq)
{
	void __iomem *base = get_irq_chip_data(irq);
	
	s3c_irq_eint_mask(irq);
	
	irq &= 31;
	writel(1 << irq, base + VIC_INT_ENABLE_CLEAR);
}


static void s3c_irq_vic_eint_unmask(unsigned int irq)
{
	void __iomem *base = get_irq_chip_data(irq);
	
	s3c_irq_eint_unmask(irq);
	
	irq &= 31;
	writel(1 << irq, base + VIC_INT_ENABLE);
}


static inline void s3c_irq_vic_eint_ack(unsigned int irq)
{
	__raw_writel(eint_irq_to_bit(irq), S5P64XX_EINTPEND(eint_pend_reg(irq)));
}


static void s3c_irq_vic_eint_maskack(unsigned int irq)
{
	/* compiler should in-line these */
	s3c_irq_vic_eint_mask(irq);
	s3c_irq_vic_eint_ack(irq);
}


static struct irq_chip s3c_irq_vic_eint = {
	.name	= "s3c_vic_eint",
	.mask	= s3c_irq_vic_eint_mask,
	.unmask	= s3c_irq_vic_eint_unmask,
	.mask_ack = s3c_irq_vic_eint_maskack,
	.ack = s3c_irq_vic_eint_ack,
	.set_type = s3c_irq_eint_set_type,
};


int __init s5p64xx_init_irq_eint(void)
{
	int irq;

	for (irq = IRQ_EINT0; irq <= IRQ_EINT15; irq++) {
		set_irq_chip(irq, &s3c_irq_vic_eint);
	}

	for (irq = IRQ_EINT(16); irq <= IRQ_EINT(31); irq++) {
		set_irq_chip(irq, &s3c_irq_eint);
		set_irq_handler(irq, handle_level_irq);
		set_irq_flags(irq, IRQF_VALID);
	}

	set_irq_chained_handler(IRQ_EINT16_31, s3c_irq_demux_eint16_31);

	//for (irq = IRQ_EINT_GROUP13_BASE; irq < (IRQ_EINT_GROUP13_BASE + IRQ_EINT_GROUP13_NR) ; irq++) {
	for (irq = IRQ_EINT_GROUP1_BASE; irq < (NR_IRQS - 1) ; irq++) {
                set_irq_chip(irq, &s5p_irq_eint);
                set_irq_handler(irq, handle_level_irq);
                set_irq_flags(irq, IRQF_VALID);
        }	

	set_irq_chained_handler(IRQ_GPIOINT, s5p_irq_demux_eint_group1_20);

	return 0;
}

arch_initcall(s5p64xx_init_irq_eint);

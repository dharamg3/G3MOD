/* arch/arm/plat-s5p64xx/include/plat/s5p6442.h
 *
 * Copyright 2008 Openmoko,  Inc.
 * Copyright 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * Header file for s5p6442 cpu support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifdef CONFIG_CPU_S5P6442

extern void s5p6442_common_init_uarts(struct s3c2410_uartcfg *cfg, int no);
extern void s5p6442_register_clocks(void);
extern void s5p6442_setup_clocks(void);

extern  int s5p6442_init(void);
extern void s5p6442_init_irq(void);
extern void s5p6442_map_io(void);
extern void s5p6442_init_clocks(int xtal);

#define s5p6442_init_uarts s5p6442_common_init_uarts

#else
#define s5p6442_init_clocks NULL
#define s5p6442_init_uarts NULL
#define s5p6442_map_io NULL
#define s5p6442_init NULL
#endif

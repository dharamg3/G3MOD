/* 
	s5p_leds.h
*/

#ifndef __ASM_ARCH_S5P_LEDS_H
#define __ASM_ARCH_S5P_LEDS_H "s5p_leds.h"

#define S5P6442_LEDF_ACTLOW	(1<<0)	
#define S5P6442_LEDF_TRISTATE	(1<<1)	

struct s5p6442_led_platdata {
  unsigned int id;
	unsigned int		 flags;

	char			*name;
	char			*def_trigger;
};


#endif /* __ASM_ARCH_LEDSGPIO_H */

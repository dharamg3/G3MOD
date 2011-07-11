/* linux/drivers/input/keyboard/s3c-keypad.h 
 *
 * Driver header for Samsung SoC keypad.
 *
 * Kim Kyoungil, Copyright (c) 2006-2009 Samsung Electronics
 *      http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */


#ifndef _S3C_KEYPAD_H_
#define _S3C_KEYPAD_H_

#include <mach/regs-irq.h>

static void __iomem *key_base;


#if CONFIG_BOARD_REVISION == CONFIG_APOLLO_EMUL
#define KEYPAD_COLUMNS  4
#define KEYPAD_ROWS     4
#define MAX_KEYPAD_NR   KEYPAD_ROWS * KEYPAD_COLUMNS
#define MAX_KEYMASK_NR  32
#elif CONFIG_BOARD_REVISION >= CONFIG_APOLLO_REV00
#define KEYPAD_COLUMNS  4
#define KEYPAD_ROWS     4
#define MAX_KEYPAD_NR   KEYPAD_ROWS * KEYPAD_COLUMNS
#define MAX_KEYMASK_NR  32
#else
#define KEYPAD_COLUMNS	3
#define KEYPAD_ROWS	3
#define MAX_KEYPAD_NR	9	/* 3*3 */
#define MAX_KEYMASK_NR  32
#endif

#define KEY_DUMP		247
#define KEY_SENDEND		248
#define KEY_ENDCALL		249
#define KEY_FOCUS		250
#define KEY_HOLD		251
#define KEY_CAMPOWER	252
#define KEY_SILENT 		253
#define KEY_INTERNET 	254
#define KEY_NOT_DEFINE 	KEY_MAX

#ifndef CONFIG_ANDROID
int keypad_keycode[] = {
		1, 2, KEY_1, KEY_Q, KEY_A, 6, 7, KEY_LEFT,
		9, 10, KEY_2, KEY_W, KEY_S, KEY_Z, KEY_RIGHT, 16,
		17, 18, KEY_3, KEY_E, KEY_D, KEY_X, 23, KEY_UP,
		25, 26, KEY_4, KEY_R, KEY_F, KEY_C, 31, 32,
		33, KEY_O, KEY_5, KEY_T, KEY_G, KEY_V, KEY_DOWN, KEY_BACKSPACE,
		KEY_P, KEY_0, KEY_6, KEY_Y, KEY_H, KEY_SPACE, 47, 48,
		KEY_M, KEY_L, KEY_7, KEY_U, KEY_J, KEY_N, 55, KEY_ENTER,
		KEY_LEFTSHIFT, KEY_9, KEY_8, KEY_I, KEY_K, KEY_B, 63, KEY_COMMA
	};
#else

#if CONFIG_BOARD_REVISION == CONFIG_APOLLO_EMUL
int keypad_keycode[] = {
                KEY_OK, 		KEY_F1, 	KEY_F2, 		KEY_UP, 
                KEY_SEARCH, 	KEY_HOME, 	KEY_BACK, 		KEY_DOWN, 
                KEY_MENU, 		KEY_BACK, 	KEY_LEFT, 		KEY_RIGHT, 
                KEY_ZOOMOUT, 	KEY_ZOOMIN, KEY_HOME, 	KEY_MAIL, 
};
int keypad_keycode_alt[] = {
                KEY_OK, 		KEY_OK, 	KEY_OK, 		KEY_DOWN,
                KEY_SEARCH, 	KEY_HOME, 	KEY_BACK, 		KEY_UP, 
                KEY_MENU, 		KEY_BACK, 	KEY_RIGHT, 		KEY_LEFT,
                KEY_ZOOMIN, 	KEY_ZOOMOUT, KEY_HOME, 		KEY_MAIL, 
};
/*** old emul board ***
int keypad_keycode[] = {
                KEY_OK, KEY_F1, KEY_F2, KEY_UP, KEY_Q, KEY_W, KEY_E, KEY_NOT_DEFINE,
                KEY_SEND, KEY_HOME, KEY_BACK, KEY_DOWN, KEY_R, KEY_T, KEY_Y, KEY_NOT_DEFINE,
                KEY_HOLD, KEY_END, KEY_LEFT, KEY_RIGHT, KEY_U, KEY_I, KEY_O, KEY_NOT_DEFINE,
                KEY_ZOOMOUT, KEY_ZOOMIN, KEY_CAMPOWER, KEY_MAIL, KEY_P, KEY_A, KEY_S, KEY_NOT_DEFINE,
                KEY_FOCUS, KEY_CAMERA, KEY_VOLUMEDOWN, KEY_VOLUMEUP, KEY_D, KEY_F, KEY_G, KEY_NOT_DEFINE,
                KEY_H, KEY_J, KEY_K, KEY_L, KEY_DELETE, KEY_FN, KEY_Z, KEY_NOT_DEFINE,
                KEY_X, KEY_C, KEY_V, KEY_B, KEY_N, KEY_M, KEY_DOT, KEY_NOT_DEFINE,
                KEY_ENTER, KEY_LEFTSHIFT, KEY_SILENT, KEY_QUESTION, KEY_SPACE, KEY_COMMA, KEY_INTERNET, KEY_NOT_DEFINE
};
*/
#elif CONFIG_BOARD_REVISION >= CONFIG_APOLLO_REV00
#if 0
int keypad_keycode[] = {
                KEY_UP, 		KEY_LEFT, 		KEY_VOLUMEUP, 		KEY_VOLUMEDOWN, 
                KEY_DOWN, 		KEY_RIGHT, 		KEY_NOT_DEFINE, 	KEY_NOT_DEFINE, 
                KEY_OK, 		KEY_OK, 		KEY_NOT_DEFINE, 	KEY_NOT_DEFINE, 	//two OK for apollo hw0.3
                KEY_NOT_DEFINE, KEY_NOT_DEFINE, KEY_NOT_DEFINE, 	KEY_NOT_DEFINE, 
};
#elif 0
// for EVT1 emul
int keypad_keycode[] = {
                KEY_UP, 		KEY_LEFT, 		KEY_VOLUMEUP, 		KEY_VOLUMEDOWN, 
                KEY_DOWN, 		KEY_RIGHT, 		KEY_NOT_DEFINE, 	KEY_VOLUMEUP, 
                KEY_OK, 		KEY_OK, 		KEY_VOLUMEDOWN, 	KEY_SEARCH, 	//two OK for apollo hw0.3
                KEY_MENU, 		KEY_HOME, 		KEY_BACK, 			KEY_NOT_DEFINE, 
};
#else
// apollo has only thress keys (Volume up, Volume down, Home)
int keypad_keycode[] = {
                KEY_NOT_DEFINE, KEY_NOT_DEFINE, KEY_VOLUMEUP, 		KEY_VOLUMEDOWN, 
                KEY_NOT_DEFINE, KEY_NOT_DEFINE, KEY_NOT_DEFINE, 	KEY_VOLUMEUP, 
                KEY_HOME, 		KEY_HOME, 		KEY_VOLUMEDOWN, 	KEY_SEARCH, 	//two Home for apollo hw0.3
                KEY_MENU, 		KEY_HOME, 		KEY_BACK, 			KEY_NOT_DEFINE, 
};
#endif
#else
int keypad_keycode[] = {
			1,2,3,4,5,6,7,8,
                9,10,11,12,13,14,15,16,
                17,18,19,20,21,22,23,24
};	
#endif

#endif //CONFIG_ANDROID

#if defined(CONFIG_CPU_S3C6410)
#define KEYPAD_DELAY		(50)
#elif defined(CONFIG_CPU_S5PC100)
#define KEYPAD_DELAY		(600)
#elif defined(CONFIG_CPU_S5PC110)
#define KEYPAD_DELAY		(600)
#elif defined(CONFIG_CPU_S5P6442)
#define KEYPAD_DELAY		(50)
#endif

#if defined(CONFIG_CPU_S5P6442)
#define KEYIFCOL_CLEAR          ((readl(key_base+S3C_KEYIFCOL) & ~0xffff)| 0xFF00) 
#else
#define	KEYIFCOL_CLEAR		(readl(key_base+S3C_KEYIFCOL) & ~0xffff)
#endif
#define	KEYIFCON_CLEAR		(readl(key_base+S3C_KEYIFCON) & ~0x1f)
#define KEYIFFC_DIV		(readl(key_base+S3C_KEYIFFC) | 0x1)

struct s3c_keypad_slide
{
    int     eint;
    int     gpio;
    int     gpio_af;
    int     state_upset;
};

struct s3c_keypad_special_key
{
    int     mask_low;
    int     mask_high;
    int     keycode;
};

struct s3c_keypad_gpio_key
{
        int     eint;
        int     gpio;
        int     gpio_af;
	int	keycode;
	int     state_upset;
};

struct s3c_keypad_extra 
{
	int 				board_num;
	int 	*keycon_matrix;
	struct s3c_keypad_slide			*slide;
	struct s3c_keypad_special_key	*special_key;
	int		special_key_num;
	struct s3c_keypad_gpio_key	*gpio_key;
	int				gpio_key_num;
	int				wakeup_by_keypad;
};

struct s3c_keypad {
	struct input_dev *dev;
        struct s3c_keypad_extra *extra;
	int nr_rows;	
	int no_cols;
	int total_keys; 
	int keycodes[MAX_KEYPAD_NR];
};

#if CONFIG_BOARD_REVISION == CONFIG_APOLLO_EMUL
struct s3c_keypad_gpio_key gpio_key_s5P6442[] = {	
	{ IRQ_EINT(22),  GPIO_nPOWER,   GPIO_nPOWER_STATE,   KEY_ENDCALL, 1},
};

struct s3c_keypad_extra s3c_keypad_extra[] = {
	{0x0000, NULL, NULL, 0, &gpio_key_s5P6442[0], 1, 1},
};
#elif CONFIG_BOARD_REVISION >= CONFIG_APOLLO_REV00
struct s3c_keypad_gpio_key gpio_key_s5P6442[] = {	
	{ IRQ_EINT(22),  GPIO_nPOWER,   GPIO_nPOWER_STATE,   KEY_ENDCALL, 1},
};

struct s3c_keypad_gpio_key gpio_key_s5P6442_04[] = {	
	{ IRQ_EINT(22),  GPIO_nPOWER,   GPIO_nPOWER_STATE,   KEY_ENDCALL, 1},
	{ IRQ_EINT(28),  S5P64XX_GPH3(4),   GPIO_nPOWER_STATE,   KEY_VOLUMEUP, 1},
	{ IRQ_EINT(29),  S5P64XX_GPH3(5),   GPIO_nPOWER_STATE,   KEY_VOLUMEDOWN, 1},
};

struct s3c_keypad_gpio_key gpio_key_s5P6442_05[] = {	
	{ IRQ_EINT(22),  GPIO_nPOWER,   GPIO_nPOWER_STATE,   KEY_ENDCALL, 1},
	{ IRQ_EINT(24),  S5P64XX_GPH3(0),   GPIO_nPOWER_STATE,   KEY_HOME, 1},
	{ IRQ_EINT(28),  S5P64XX_GPH3(4),   GPIO_nPOWER_STATE,   KEY_VOLUMEUP, 1},
	{ IRQ_EINT(29),  S5P64XX_GPH3(5),   GPIO_nPOWER_STATE,   KEY_VOLUMEDOWN, 1},
};

struct s3c_keypad_gpio_key gpio_key_s5P6442_open[] = {	
	{ IRQ_EINT(22),  GPIO_nPOWER,   GPIO_nPOWER_STATE,   KEY_ENDCALL, 1},
	{ IRQ_EINT(24),  S5P64XX_GPH3(0),   GPIO_nPOWER_STATE,   KEY_HOME, 1},
	{ IRQ_EINT(28),  S5P64XX_GPH3(4),   GPIO_nPOWER_STATE,   KEY_VOLUMEUP, 1},
	{ IRQ_EINT(29),  S5P64XX_GPH3(5),   GPIO_nPOWER_STATE,   KEY_VOLUMEDOWN, 1},
	{ IRQ_EINT(30),  S5P64XX_GPH3(6),   GPIO_nPOWER_STATE,   KEY_BACK, 1},
	{ IRQ_EINT(31),  S5P64XX_GPH3(7),   GPIO_nPOWER_STATE,   KEY_MENU, 1},
};

struct s3c_keypad_extra s3c_keypad_extra[] = {
	{0xFFFF, NULL, NULL, NULL, 0, &gpio_key_s5P6442_05[0], 4, 1},		// default (apollo hw_rev0.5)
	//[ not support hw_rev0.2 and hw_rev0.4 more
	//{0x0001, &keypad_keycode[0], NULL, NULL, 0, &gpio_key_s5P6442[0], 1, 1},		// apollo hw_rev0.2
	//{0x0000, &keypad_keycode[0], NULL, NULL, 0, &gpio_key_s5P6442_04[0], 3, 1},		// apollo hw_rev0.4
	//]
	{0x0002, &keypad_keycode[0], NULL, NULL, 0, &gpio_key_s5P6442_04[0], 3, 1},		// apollo hw_rev0.4	(4-2-1 memory)
	{0x0006, NULL, NULL, NULL, 0, &gpio_key_s5P6442_05[0], 4, 1},		// apollo hw_rev0.5
	{0x0003, NULL, NULL, NULL, 0, &gpio_key_s5P6442_open[0], 6, 1},		// apollo open hw_rev0.7
	{0x0001, NULL, NULL, NULL, 0, &gpio_key_s5P6442_open[0], 6, 1},		// apollo open hw_rev0.9b
	{0x0000, NULL, NULL, NULL, 0, &gpio_key_s5P6442_open[0], 6, 1},		// apollo open hw_rev1.0a
};

#define __KEYPAD_CHECK_HW_REV_PIN__

#define __KEYPAD_FILTER_SETTING_FOR_GPIO_KEY__
static inline void set_eint_filter()
{
	// S5P64XX_GPH3(0), S5P64XX_GPH3(4), S5P64XX_GPH3(5)
	// set debouncing filter as max
	u32 val;

	val = __raw_readl(S5P64XX_EINT3FLTCON0);
	val &= ~0xff;
	val |= 0xff;
	__raw_writel(val, S5P64XX_EINT3FLTCON0);

	val = __raw_readl(S5P64XX_EINT3FLTCON1);
	val &= ~0xffffffff;
	val |= 0xffffffff;
	__raw_writel(val, S5P64XX_EINT3FLTCON1);
}
static inline void reset_eint_filter()
{
	// S5P64XX_GPH3(0), S5P64XX_GPH3(4), S5P64XX_GPH3(5)
	// clear debouncing filter as max
	u32 val;
	val = __raw_readl(S5P64XX_EINT3FLTCON0);
	val &= ~0xff;
	val |= 0x80;
	__raw_writel(val, S5P64XX_EINT3FLTCON0);
	
	val = __raw_readl(S5P64XX_EINT3FLTCON1);
	val &= ~0xffffffff;
	val |= 0x80808080;
	__raw_writel(val, S5P64XX_EINT3FLTCON1);
}
#define __KEYPAD_REMOVE_TOUCH_KEY_INTERFERENCE__
static unsigned interfered_mask = 0x333;		// 5-Navi keys
static unsigned interfered_gpio = S5P64XX_GPH3(0);		// Home key
#else
#endif

extern void s3c_setup_keypad_cfg_gpio(int rows, int columns);
extern void s3c_keypad_suspend_cfg_gpio(int rows, int columns);

// use if key column gpios isn't keypad i/f
#define __KEYPAD_SCAN_CTRL_USING_GPIO_INPUT_MODE__

// allow software key for MMICheck
#define __KEYPAD_PSEUDO_KYECODE__
int pseudo_keycode[] = { KEY_CAMERA, KEY_OK, KEY_DOWN, KEY_UP, KEY_LEFT, KEY_RIGHT };

#endif				/* _S3C_KEYIF_H_ */

/*
 * HDC1100 LCD Panel Driver for the Samsung Universal board
 *
 * Author: Kyuhyeok Jang  <kyuhyeok.jang@samsung.com>
 *
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <linux/wait.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <mach/hardware.h>
#include <plat/gpio-cfg.h>
#include <linux/i2c.h>
#include <linux/leds.h>
#include "s3cfb.h"

//sm.kim: for pwm test
#define __USE_PWM_BD60910GU__

static int bd60910gu_set_brightness(int level);
static int backlight_i2c_write(u8 reg, u8 data);
static int lms300_init(void);

static int lcd_power = OFF;
static int backlight_power = OFF;
static int backlight_level = 0x63;
//#define NAOS_SETTINGS

/* FIXME: will be moved to platform data */
static struct s3cfb_lcd lms300 = {
	.width = 240,
	.height = 400,
	.bpp = 16,
	.freq = 60,
#ifdef NAOS_SETTINGS
	.timing = {
		.h_fp = 8,
		.h_bp = 24,
		.h_sw = 12,
		.v_fp = 8,
		.v_fpe = 1,
		.v_bp = 8,
		.v_bpe = 1,
		.v_sw = 2,
	},
	.polarity = {
		.rise_vclk = 1,
		.inv_hsync = 1,
		.inv_vsync = 1,
		.inv_vden = 1,
	},
#else
	.timing = {
		.h_fp = 8,
		.h_bp = 24,
		.h_sw = 12,
		.v_fp = 8,
		.v_fpe = 1,
		.v_bp = 8,
		.v_bpe = 1,
		.v_sw = 2,
        },
        .polarity = {
                .rise_vclk = 0,
                .inv_hsync = 0,
                .inv_vsync = 0,
                .inv_vden = 0,
        },

#endif
};



void s3cfb_set_lcd_info(struct s3cfb_global *ctrl)
{
	lms300.init_ldi = NULL;
	ctrl->lcd = &lms300;
}

/*
 * Serial Interface
 */

#define LCD_CSX_HIGH	gpio_set_value(GPIO_DISPLAY_CS, GPIO_LEVEL_HIGH);
#define LCD_CSX_LOW		gpio_set_value(GPIO_DISPLAY_CS, GPIO_LEVEL_LOW);

#define LCD_SCL_HIGH	gpio_set_value(GPIO_DISPLAY_CLK, GPIO_LEVEL_HIGH);
#define LCD_SCL_LOW		gpio_set_value(GPIO_DISPLAY_CLK, GPIO_LEVEL_LOW);

#define LCD_SDI_HIGH	gpio_set_value(GPIO_DISPLAY_SI, GPIO_LEVEL_HIGH);
#define LCD_SDI_LOW		gpio_set_value(GPIO_DISPLAY_SI, GPIO_LEVEL_LOW);

#define DEFAULT_USLEEP	10	

#define POWCTL			0xF3
#define VCMCTL			0xF4
#define SRCCTL			0xF5
#define SLPOUT			0x11
#define MADCTL			0x36
#define COLMOD			0x3A
#define DISCTL			0xF2
#define IFCTL			0xF6
#define GATECTL			0xFD
#define WRDISBV			0x51
#define WRCABCMB		0x5E
#define MIECTL1			0xCA
#define BCMODE			0xCB
#define MIECTL2			0xCC
#define MIDCTL3			0xCD
#define RPGAMCTL		0xF7
#define RNGAMCTL		0xF8
#define GPGAMCTL		0xF9
#define GNGAMCTL		0xFA
#define BPGAMCTL		0xFB
#define BNGAMCTL		0xFC
#define CASET			0x2A
#define PASET			0x2B
#define RAMWR           0x2C
#define WRCTRLD			0x53
#define WRCABC			0x55
#define DISPON			0x29
#define DISPOFF			0x28
#define SLPIN			0x10

struct setting_table {
	u8 command;	
	u8 parameters;
	u8 parameter[15];
	s32 wait;
};

#ifdef NAOS_SETTINGS
static struct setting_table power_on_setting_table[] = {
	{   POWCTL,  7, { 0x80, 0x00, 0x00, 0x09, 0x33, 0x75, 0x75, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{   VCMCTL,  5, { 0x57, 0x57, 0x6C, 0x6C, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{   SRCCTL,  5, { 0x12, 0x00, 0x03, 0xF0, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{   SLPOUT,  0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 120 },
	{   MADCTL,  1, { 0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },		
	{   COLMOD,  1, { 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  30 },		
	{   DISCTL, 11, { 0x15, 0x15, 0x03, 0x08, 0x08, 0x08, 0x08, 0x10, 0x04, 0x16, 0x16, 0x00, 0x00, 0x00, 0x00 },   0 },	
	{    IFCTL,  4, { 0x00, 0x81, 0x30, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },	
	{  GATECTL,  2, { 0x22, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },	
	{  WRDISBV,  1, { 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	//{  WRDISBV,  1, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },	
	{ WRCABCMB,  1, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },	
	{  MIECTL1,  3, { 0x80, 0x80, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },	0 },	
	{   BCMODE,  1, { 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },	0 },	
	{  MIECTL2,  3, { 0x20, 0x01, 0x8F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },	0 },	
	{  MIDCTL3,  2, { 0x7C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },	0 },	
	{ RPGAMCTL, 15, { 0x00, 0x35, 0x00, 0x02, 0x10, 0x0E, 0x11, 0x1A, 0x24, 0x22, 0x19, 0x0F, 0x00, 0x22, 0x22 },   0 },	
	{ RNGAMCTL, 15, { 0x2B, 0x00, 0x00, 0x02, 0x10, 0x0E, 0x11, 0x1A, 0x24, 0x22, 0x19, 0x0F, 0x22, 0x22, 0x22 },   0 },	
	{ GPGAMCTL, 15, { 0x00, 0x35, 0x00, 0x02, 0x10, 0x0C, 0x0D, 0x14, 0x2C, 0x2D, 0x2B, 0x1F, 0x13, 0x22, 0x22 },   0 },	
	{ GNGAMCTL, 15, { 0x2B, 0x00, 0x00, 0x02, 0x10, 0x0C, 0x0D, 0x14, 0x2C, 0x2D, 0x2B, 0x1F, 0x13, 0x22, 0x22 },   0 },	
	{ BPGAMCTL, 15, { 0x21, 0x35, 0x00, 0x02, 0x10, 0x14, 0x18, 0x21, 0x1E, 0x20, 0x19, 0x0F, 0x00, 0x22, 0x22 },   0 },	
	{ BNGAMCTL, 15, { 0x2B, 0x21, 0x00, 0x02, 0x10, 0x14, 0x18, 0x21, 0x1E, 0x20, 0x19, 0x0F, 0x00, 0x22, 0x22 },   0 },	
	{    CASET,  4, { 0x00, 0x00, 0x00, 0xEF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },	0 },	
	{    PASET,  4, { 0x00, 0x00, 0x01, 0x8F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },	0 },	
	{    RAMWR,  0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },	0 },	
	{  WRCTRLD,  1, { 0x2C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },	0 },	
	{   WRCABC,  1, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },	0 },	
	{   DISPON,  0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  50 },	
};

#else
static struct setting_table power_on_setting_table[] = {
	{   POWCTL,  7, { 0x80, 0x00, 0x00, 0x02, 0x44, 0x3c, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{   VCMCTL,  5, { 0x33, 0x33, 0x5a, 0x5a, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{   SRCCTL,  5, { 0x12, 0x00, 0x03, 0xF0, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{   SLPOUT,  0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 120 },
	{   MADCTL,  1, { 0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },	
	{   COLMOD,  1, { 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  30 },	
	{   DISCTL, 11, { 0x15, 0x15, 0x03, 0x08, 0x08, 0x08, 0x08, 0x10, 0x04, 0x16, 0x16, 0x00, 0x00, 0x00, 0x00 },   0 },	
	{    IFCTL,  4, { 0x00, 0x81, 0x30, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },	
	{  GATECTL,  2, { 0x22, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },	
	{  WRDISBV,  1, { 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 }, //BRIGHTNESS	
	{ WRCABCMB,  1, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },	
	{  MIECTL1,  3, { 0x80, 0x80, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },	0 },	
	{   BCMODE,  1, { 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },	0 },	
	{  MIECTL2,  3, { 0x20, 0x01, 0x8F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },	0 },	
//	{  MIDCTL3,  2, { 0x7C, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },	0 },	
	{  MIDCTL3,  2, { 0x2C, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{ RPGAMCTL, 15, { 0x00, 0x25, 0x2F, 0x1B, 0x21, 0x27, 0x2C, 0x35, 0x09, 0x0E, 0x19, 0x15, 0x15, 0x22, 0x22 },   0 },	
	{ RNGAMCTL, 15, { 0x25, 0x00, 0x2F, 0x1B, 0x21, 0x27, 0x2C, 0x35, 0x09, 0x0E, 0x19, 0x15, 0x15, 0x22, 0x22 },   0 },	
	{ GPGAMCTL, 15, { 0x00, 0x25, 0x0A, 0x24, 0x21, 0x27, 0x2C, 0x34, 0x0B, 0x08, 0x17, 0x15, 0x15, 0x22, 0x22 },   0 },	
	{ GNGAMCTL, 15, { 0x25, 0x00, 0x0A, 0x24, 0x21, 0x27, 0x2C, 0x34, 0x0B, 0x08, 0x17,  0x15, 0x15, 0x22, 0x22 },   0 },	
	{ BPGAMCTL, 15, { 0x00, 0x23, 0x2A, 0x09, 0x23, 0x25, 0x27, 0x2C, 0x16, 0x1A, 0x3D, 0x15, 0x15, 0x22, 0x22 },   0 },	
	{ BNGAMCTL, 15, { 0x23, 0x00, 0x2A, 0x09, 0x23, 0x25, 0x27, 0x2C, 0x16, 0x1A, 0x3D, 0x15, 0x15, 0x22, 0x22 },   0 },	
	{    CASET,  4, { 0x00, 0x00, 0x00, 0xEF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },	0 },	
	{    PASET,  4, { 0x00, 0x00, 0x01, 0x8F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },	0 },	
	{    RAMWR,  0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },	0 },	
	{  WRCTRLD,  1, { 0x2C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },	0 },	
	{   WRCABC,  1, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },	0 },	
	{   DISPON,  0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  50 },	
};
#endif
#define POWER_ON_SETTINGS	(int)(sizeof(power_on_setting_table)/sizeof(struct setting_table))

static struct setting_table power_off_setting_table[] = {
	{  WRCTRLD,  1, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 },
	{  DISPOFF,  0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },  40 },
	{    SLPIN,  0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 120 },
};

#define POWER_OFF_SETTINGS	(int)(sizeof(power_off_setting_table)/sizeof(struct setting_table))

static struct setting_table backlight_setting_table = 
	{ WRDISBV,  1, { 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },   0 };

static void setting_table_write(struct setting_table *table)
{
	s32 i, j;

	LCD_CSX_HIGH
	udelay(DEFAULT_USLEEP);
	LCD_SCL_HIGH 
	udelay(DEFAULT_USLEEP);

	/* Write Command */
	LCD_CSX_LOW
	udelay(DEFAULT_USLEEP);
	LCD_SCL_LOW 
	udelay(DEFAULT_USLEEP);		
	LCD_SDI_LOW 
	udelay(DEFAULT_USLEEP);
	
	LCD_SCL_HIGH 
	udelay(DEFAULT_USLEEP); 

   	for (i = 7; i >= 0; i--) { 
		LCD_SCL_LOW
		udelay(DEFAULT_USLEEP);
		if ((table->command >> i) & 0x1)
			LCD_SDI_HIGH
		else
			LCD_SDI_LOW
		udelay(DEFAULT_USLEEP);	
		LCD_SCL_HIGH
		udelay(DEFAULT_USLEEP);	
	}

	LCD_CSX_HIGH
	udelay(DEFAULT_USLEEP);	

	/* Write Parameter */
	if ((table->parameters) > 0) {
	for (j = 0; j < table->parameters; j++) {
		LCD_CSX_LOW 
		udelay(DEFAULT_USLEEP); 	
		
		LCD_SCL_LOW 
		udelay(DEFAULT_USLEEP); 	
		LCD_SDI_HIGH 
		udelay(DEFAULT_USLEEP);
		LCD_SCL_HIGH 
		udelay(DEFAULT_USLEEP); 	

		for (i = 7; i >= 0; i--) { 
			LCD_SCL_LOW
			udelay(DEFAULT_USLEEP);	
			if ((table->parameter[j] >> i) & 0x1)
				LCD_SDI_HIGH
			else
				LCD_SDI_LOW
			udelay(DEFAULT_USLEEP);	
			LCD_SCL_HIGH
			udelay(DEFAULT_USLEEP);					
		}

			LCD_CSX_HIGH
			udelay(DEFAULT_USLEEP);	
	}
	}

	msleep(table->wait);
}

/*
 *	LCD Power Handler
 */



void lcd_power_ctrl(s32 value)
{
	s32 i;	
	u8 data;
	
	if (value) {
	
		/* Power On Sequence */
		gpio_set_value(GPIO_MLCD_ON, GPIO_LEVEL_HIGH);
		msleep(10);	

		/* Reset Deasseert */
		gpio_set_value(GPIO_MLCD_RST, GPIO_LEVEL_HIGH);
		msleep(10);	
	
		/* Reset Asseert */
		gpio_set_value(GPIO_MLCD_RST, GPIO_LEVEL_LOW);
		msleep(10);	

		/* Reset Deasseert */
		gpio_set_value(GPIO_MLCD_RST, GPIO_LEVEL_HIGH);
		msleep(20);


		for (i = 0; i < POWER_ON_SETTINGS; i++)
			setting_table_write(&power_on_setting_table[i]);	

		// brightness setting
	#ifdef __USE_PWM_BD60910GU__
		//printk("%s brightness:0x%x\n", __func__, backlight_level);
		bd60910gu_set_brightness((int)((backlight_level * 3)/4));	//1//1~127 => 1~100	
	#endif

	}
	else {

		/* Power Off Sequence */
	
		for (i = 0; i < POWER_OFF_SETTINGS; i++)
			setting_table_write(&power_off_setting_table[i]);	

		/* Reset Assert */
		gpio_set_value(GPIO_MLCD_RST, GPIO_LEVEL_LOW);		

		gpio_set_value(GPIO_MLCD_ON, GPIO_LEVEL_LOW);
	
	}

	lcd_power = value;
}
#define MAX_BRIGHTNESS_LEVEL 100

static int bd60910gu_backlight_init(void)
{
	//backlight init
                /* Reset Deasseert */
		gpio_set_value(GPIO_ALS_nRST, GPIO_LEVEL_HIGH);
        msleep(10);

                /* Reset Asseert */
		gpio_set_value(GPIO_ALS_nRST, GPIO_LEVEL_LOW);
        msleep(10);

                /* Reset Deasseert */
		gpio_set_value(GPIO_ALS_nRST, GPIO_LEVEL_HIGH);
        msleep(10);

#ifdef __USE_PWM_BD60910GU__
		backlight_i2c_write(0x03, 0x70);		// LED Current setting at register mode (0x63 = 20.0mA)
		backlight_i2c_write(0x08, 0x00);		// LED Current transition	TLH=THL: minimum setting(0.256ms)
        backlight_i2c_write(0x01, 0x19);		// LED, ALC Control 	OVP=27V(7LED), WPWMEN=on......Normal mode
#else
		backlight_i2c_write(0x03, 0x63);		// LED Current setting at register mode (0x63 = 20.0mA)
		backlight_i2c_write(0x08, 0x00);		// LED Current transition	TLH=THL: minimum setting(0.256ms)
        backlight_i2c_write(0x01, 0x11);		// LED, ALC Control 	OVP=27V(7LED), WPWMEN=off......Normal mode
#endif

		return 0;
}

static int bd60910gu_backlight_on(void)
{
#ifdef __USE_PWM_BD60910GU__
		backlight_i2c_write(0x03, 0x70);		// LED Current setting at register mode (0x63 = 20.0mA)	
		backlight_i2c_write(0x08, 0x00);		// LED Current transition	TLH=THL: minimum setting(0.256ms)
        backlight_i2c_write(0x01, 0x19);		// LED, ALC Control 	OVP=27V(7LED), WPWMEN=on......Normal mode
#else        
		backlight_i2c_write(0x03, 0x63);		// LED Current setting at register mode (0x63 = 20.0mA)	
		backlight_i2c_write(0x08, 0x00);		// LED Current transition	TLH=THL: minimum setting(0.256ms)
        backlight_i2c_write(0x01, 0x11);		// LED, ALC Control 	OVP=27V(7LED), WPWMEN=off......Normal mode
#endif
		return 0;
}

static int bd60910gu_backlight_off(void)
{
		backlight_i2c_write(0x01, 0x00);		// LED, ALC Control		LED off
		return 0;
}

#ifdef __USE_PWM_BD60910GU__
static int bd60910gu_set_brightness(int level)
{
	unsigned int led_val;

	if(level > MAX_BRIGHTNESS_LEVEL)
		level = MAX_BRIGHTNESS_LEVEL;

	led_val = (0xFF * level) / MAX_BRIGHTNESS_LEVEL;

	if(led_val > 0xFF)
		led_val = 0xFF;			//	led_val must be less than or equal to 0x7F (0~127)

	backlight_setting_table.parameter[0] = led_val;

	//printk("%s brightness:0x%x\n", __func__, backlight_setting_table.parameter[0]);
	setting_table_write(&backlight_setting_table);	

	return 0;
}

#else        
static int bd60910gu_set_brightness(int level)
{
	unsigned int led_val;

		if(level > MAX_BRIGHTNESS_LEVEL)
			level = MAX_BRIGHTNESS_LEVEL;
		
		led_val = (0x7F * level) / MAX_BRIGHTNESS_LEVEL;

		if(led_val > 0x7F)
			led_val = 0x7F;			//	led_val must be less than or equal to 0x7F (0~127)

		backlight_i2c_write(0x03, led_val);

		return 0;

}

#endif	//__USE_PWM_BD60910GU__

static int __init lms300_init(void)
{  

        printk("#####################  %s : START ##################\n",__FUNCTION__);
	gpio_set_value(GPIO_MLCD_ON, GPIO_LEVEL_HIGH);
        msleep(10);

        bd60910gu_backlight_init();
       	 backlight_power = ON; 
  //LCD power on      
        lcd_power_ctrl(1);

	return 0;
}

void s3cfb_set_lcd_power(int value)
{
	printk("#####%s: value %d \n", __FUNCTION__, value);
        lcd_power_ctrl(value);
}
EXPORT_SYMBOL(s3cfb_set_lcd_power);

int s3cfb_get_lcd_power(void)
{
  return lcd_power;
}

EXPORT_SYMBOL(s3cfb_get_lcd_power);

void s3cfb_set_backlight_power(int value)
{
	printk("#####%s: value %d \n", __FUNCTION__, value); 
	 if ((value < OFF) ||    /* Invalid Value */
                (value > ON) ||
                (value == backlight_power))     /* Same Value */
                return;
	if(value == ON){
	    if (lcd_power == OFF)
                        lcd_power_ctrl(ON);
	   bd60910gu_backlight_on();
	}
	else{
	   bd60910gu_backlight_off();
	   lcd_power_ctrl(OFF);
	}

	backlight_power = value;
}
EXPORT_SYMBOL(s3cfb_set_backlight_power);

int s3cfb_get_backlight_power(void)
{
	return backlight_power;
}
EXPORT_SYMBOL(s3cfb_get_backlight_power);

void s3cfb_set_backlight_level(int value)
{
	//printk("#####%s: value %d \n", __FUNCTION__, value);
	if (value == backlight_level)     /* Same Value */
                return;

        if (backlight_power)
                bd60910gu_set_brightness((int)((value * 3)/4));

        backlight_level = value;

}
EXPORT_SYMBOL(s3cfb_set_backlight_level);

int s3cfb_get_backlight_level(void)
{
	return backlight_level;
}
EXPORT_SYMBOL(s3cfb_get_backlight_level);

static void bd60910gu_set_backlight_level(int value)
{
	//printk("#####%s: value %d \n", __FUNCTION__, value);
	if (value == backlight_level)     /* Same Value */
		return;

	if(value)
		{
		if(!backlight_power)
			{
			s3cfb_set_backlight_power(ON);
			}
		}
	else
		{
		if(backlight_power)
			{
			s3cfb_set_backlight_power(OFF);
			}
		}

	bd60910gu_set_brightness((int)((value * 3)/4));		// 1~127 => 1~100

	backlight_level = value;
}

static void bd60910gu_brightness_set(struct led_classdev *led_cdev, enum led_brightness value)
{
	//mutex_lock(&ams320fs01_backlight_lock);
	bd60910gu_set_backlight_level(value/2);		// 1~255 => 1~127
	//mutex_unlock(&ams320fs01_backlight_lock);
}

static struct led_classdev bd60910gu_backlight_led  = {
	.name		= "lcd-backlight",
	.brightness = 255,
	.brightness_set = bd60910gu_brightness_set,
};

#include <linux/i2c.h>

#define I2C_GPIO3_DEVICE_ID     3 /* mach-($DEVICE_NAME).c */
#define I2C_SLAVE_ADDR_BD60910GU  0xEC	

static struct i2c_driver bd60910gu_driver;
static struct i2c_client *bd60910gu_i2c_client = NULL;

static unsigned short bd60910gu_normal_i2c[] = { I2C_CLIENT_END };
static unsigned short bd60910gu_ignore[] = { I2C_CLIENT_END };
static unsigned short bd60910gu_probe[] = { I2C_GPIO3_DEVICE_ID,
        (I2C_SLAVE_ADDR_BD60910GU >> 1), I2C_CLIENT_END };

static struct i2c_client_address_data bd60910gu_addr_data = {
        .normal_i2c = bd60910gu_normal_i2c,
        .ignore         = bd60910gu_ignore,
        .probe          = bd60910gu_probe,
};

static int bd60910gu_read(struct i2c_client *client, u8 reg, u8 *data)
{
        int ret;
        u8 buf[1];
        struct i2c_msg msg[2];

        buf[0] = reg;

        msg[0].addr = client->addr;
        msg[0].flags = 0;
        msg[0].len = 1;
        msg[0].buf = buf;

        msg[1].addr = client->addr;
        msg[1].flags = I2C_M_RD;
        msg[1].len = 1;
        msg[1].buf = buf;

        ret = i2c_transfer(client->adapter, msg, 2);
        if (ret != 2)
                return -EIO;

        *data = buf[0];

        return 0;
}

static int bd60910gu_write(struct i2c_client *client, u8 reg, u8 data)
{
        int ret;
        u8 buf[2];
        struct i2c_msg msg[1];

        buf[0] = reg;
        buf[1] = data;

        msg[0].addr = client->addr;
        msg[0].flags = 0;
        msg[0].len = 2;
        msg[0].buf = buf;

        ret = i2c_transfer(client->adapter, msg, 1);
        if (ret != 1)
                return -EIO;

        return 0;
}


static int backlight_i2c_write(u8 reg, u8 data)
{
	if(bd60910gu_i2c_client != NULL){
		bd60910gu_write(bd60910gu_i2c_client, reg, data);		
	}
	else
	   printk("########%s : clinet is NULL!!\n", __FUNCTION__);

	return 0;	

}

static int backlight_i2c_read(u8 reg, u8 *data)
{
        if(bd60910gu_i2c_client != NULL){
                bd60910gu_read(bd60910gu_i2c_client, reg, data);
        }
        else
           printk("########%s : clinet is NULL!!\n", __FUNCTION__);

	return 0;

}

static int bd60910gu_attach(struct i2c_adapter *adap, int addr, int kind)
{
        struct i2c_client *c;
        int ret;

	 printk("#######%s :START \n", __FUNCTION__);
        c = kmalloc(sizeof(*c), GFP_KERNEL);
        if (!c)
                return -ENOMEM;

        memset(c, 0, sizeof(struct i2c_client));

        strcpy(c->name, bd60910gu_driver.driver.name);
        c->addr = addr;
        c->adapter = adap;
        c->driver = &bd60910gu_driver;

        if ((ret = i2c_attach_client(c)))
                goto error;

        bd60910gu_i2c_client = c;

		ret = led_classdev_register(&adap->dev, &bd60910gu_backlight_led);
		if (ret < 0)
			printk("%s fail: led_classdev_register\n", __func__);

	printk("#######%s :END \n", __FUNCTION__);

error:
        return ret;
}

static int bd60910gu_attach_adapter(struct i2c_adapter *adap)
{
   int ret;
	printk("########### %s #############\n", __FUNCTION__);
        ret = i2c_probe(adap, &bd60910gu_addr_data, bd60910gu_attach);
        printk("########### %s : ret %d \n", __FUNCTION__, ret);
	return ret;
}

static int bd60910gu_detach_client(struct i2c_client *client)
{
	led_classdev_unregister(&bd60910gu_backlight_led); 

	printk("########### %s #############\n", __FUNCTION__);
        i2c_detach_client(client);
        return 0;
}

static int bd60910gu_command(struct i2c_client *client, unsigned int cmd, void *arg)
{
        return 0;
}

static struct i2c_driver bd60910gu_driver = {
        .driver = {
                .name = "bd60910gu",
        },
        .attach_adapter = bd60910gu_attach_adapter,
        .detach_client = bd60910gu_detach_client,
        .command = bd60910gu_command
};

static int __init BD60910GU_init(void)
{
	printk("########### %s #############\n", __FUNCTION__);
        return i2c_add_driver(&bd60910gu_driver);
}

static void __exit BD60910GU_exit(void)
{
        i2c_del_driver(&bd60910gu_driver);
}

module_init(BD60910GU_init);
module_exit(BD60910GU_exit);

#ifdef CONFIG_FB_S3C_LCD_INIT
//SEC: sm.kim 20091210 remove init call; bootloader turn on lcd
late_initcall(lms300_init);
#endif
//module_init(lms300_init);
//module_exit(lms300_exit);

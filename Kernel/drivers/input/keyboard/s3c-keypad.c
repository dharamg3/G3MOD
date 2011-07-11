/* drivers/input/keyboard/s3c-keypad.c
 *
 * Driver core for Samsung SoC onboard UARTs.
 *
 * Kim Kyoungil, Copyright (c) 2006-2009 Samsung Electronics
 *      http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/clk.h>

#include <linux/io.h>
#include <mach/hardware.h>
#include <asm/delay.h>
#include <asm/irq.h>

#include <plat/regs-gpio.h>
#include <plat/regs-keypad.h>
#include <linux/irq.h>
#include <plat/gpio-cfg.h>
//nclude "drivers/base/bash.h"
#include "s3c-keypad.h"

#undef S3C_KEYPAD_DEBUG 
//#define S3C_KEYPAD_DEBUG 

#ifdef CONFIG_CPU_IDLE
#include <mach/cpuidle.h>
#endif /* CONFIG_CPU_IDLE */

#ifdef CONFIG_CPU_FREQ
#include <plat/s5p6442-dvfs.h>
#endif /* CONFIG_CPU_FREQ */

#ifdef S3C_KEYPAD_DEBUG
#define DPRINTK(x...) printk("S3C-Keypad " x)
#else
#define DPRINTK(x...)		/* !!!! */
#endif

#define PRINTK(x...) printk("S3C-Keypad " x)

#define DEVICE_NAME "s3c-keypad"

#define TRUE 1
#define FALSE 0

#define FIRST_SCAN_INTERVAL    	(1)
#define SCAN_INTERVAL    	(HZ/50)

#if CONFIG_BOARD_REVISION == CONFIG_APOLLO_EMUL
extern int apollo_get_saved_lcd_id_value();
#endif

#ifdef __KEYPAD_CHECK_HW_REV_PIN__
extern int apollo_get_hw_rev_pin_value();
#endif

extern int wakeup_flag_for_key;

// apollo devices don't have key matrix
static int use_key_matrix=1;

static struct timer_list keypad_timer;
//sm.kim: 20090114 use timer_pending instead of flag
//static int is_timer_on = FALSE;
static struct clk *keypad_clock;
static unsigned prevmask_low = 0, prevmask_high = 0;

//for modemctl
struct input_dev *g_input_dev;
void report_dummykey()
{
	if(wakeup_flag_for_key== 249)
	{
		input_report_key(g_input_dev, 249,1);
		input_report_key(g_input_dev, 249,0);
	}

	wakeup_flag_for_key = 0;
}
EXPORT_SYMBOL(report_dummykey);

#if defined(__KEYPAD_REMOVE_TOUCH_KEY_INTERFERENCE__)
int check_press_key_interfered()
{
	if(use_key_matrix)
		return (0 != (prevmask_low & interfered_mask));
	else
		return !gpio_get_value(interfered_gpio);	// Home key
}
EXPORT_SYMBOL(check_press_key_interfered);
#endif

#if 0
static int keypad_scan(u32 *keymask_low, u32 *keymask_high)
{
	printk("keypad_scan\n");
	int i,j = 0;
	u32 cval,rval;
	for (i=0; i<KEYPAD_COLUMNS; i++) {
                cval = (KEYCOL_DMASK & ~(1 << i)) | (1 << (i+ 8));   // clear that column number and 
                printk("[keypad] cval = 0x%x,\n", cval);
//		cval = (KEYCOL_DMASK & ~(1 << i)) | (1 << (i+ KEYPAD_ROWS));                
                writel(cval, key_base+S3C_KEYIFCOL);               // make that Normal output.
                                                                 // others shuld be High-Z output.
                udelay(KEYPAD_DELAY);
                rval = ~(readl(key_base+S3C_KEYIFROW)) & ((1<<KEYPAD_ROWS)-1) ;
				printk("[keypad] reg_row = 0x%x, rval = 0x%x\n",readl(key_base+S3C_KEYIFROW), rval); 																   
#if (KEYPAD_COLUMNS>4)  
                 if (i < 4)
                 {
                    *keymask_low |= (rval << (i * KEYPAD_COLUMNS));
 					printk("[keypad] keymask_low = 0x%x, rval = 0x%x\n",*keymask_low, rval); 																   
                 }
                 else 
                 {
                    *keymask_high |= (rval << ((i-4) * KEYPAD_COLUMNS));
	 				printk("[keypad] keymask_high = 0x%x, rval = 0x%x\n",*keymask_high, rval); 																   
                 }
#else   
                 *keymask_low |= (rval << (i * KEYPAD_COLUMNS));
#endif
    }
 
            writel(KEYIFCOL_CLEAR, key_base+S3C_KEYIFCOL);

	return 0;
}

#elif defined(__KEYPAD_SCAN_CTRL_USING_GPIO_INPUT_MODE__)
static const u32 keycol_con[] = {
		0x00000001,
		0x00000010,
		0x00000100,
		0x00001000,
		0x00010000,
		0x00100000,
		0x01000000,
		0x10000000
};

#define KEYCOL_DATMASK				((1<<KEYPAD_COLUMNS)-1)
#define KEYCOL_CONMASK				((1<<(4*KEYPAD_COLUMNS))-1)
#define KEYCOL_WRITEDAT(x,regs)		__raw_writel((x)|(__raw_readl(regs)&~KEYCOL_DATMASK), (regs))
#define KEYCOL_WRITECON(x,regs)		__raw_writel((x)|(__raw_readl(regs)&~KEYCOL_CONMASK), (regs))
					
static int keypad_scan(u32 *keymask_low, u32 *keymask_high)
{
	int i = 0;
	u32 cval,rval;
	//u32 col_num=0;
	//u32 row_value=0;

	DPRINTK("%s, DATMASK:%x, CONMASK:%x\n", __func__, KEYCOL_DATMASK, KEYCOL_CONMASK);

	//__raw_writel(0,S5P64XX_GPJ0DAT);
	KEYCOL_WRITEDAT(0, S5P64XX_GPJ0DAT);
	//row_value =  ~(readl(key_base+S3C_KEYIFROW)) & ((1<<KEYPAD_ROWS)-1);

	for(i=0; i < 4; i++)
		{
		cval = (1 << i);                                                                                                   
		//__raw_writel(keycol_con[i],S5P64XX_GPJ0CON);
		KEYCOL_WRITECON(keycol_con[i], S5P64XX_GPJ0CON);
		DPRINTK("%s, gpj0con:%x\n", __func__, __raw_readl(S5P64XX_GPJ0CON));
		//__raw_writel(cval,S5P64XX_GPJ0DAT);                                                                                                     
		udelay(KEYPAD_DELAY);
		rval = ~(readl(key_base+S3C_KEYIFROW)) & ((1<<KEYPAD_ROWS)-1);
		DPRINTK("####rval %x cval %x \n", rval, cval);
		//if(row_value != rval)
		*keymask_low |= (rval << (i * KEYPAD_ROWS));
		}
		
#if (KEYPAD_COLUMNS>4)  
	for(i=4; i < 8; i++)
		{
		cval = (1 << i);
		//__raw_writel(keycol_con[i],S5P64XX_GPJ0CON);
		KEYCOL_WRITECON(keycol_con[i], S5P64XX_GPJ0CON);
		DPRINTK("%s, gpj0con:%x\n", __func__, __raw_readl(S5P64XX_GPJ0CON));
		//__raw_writel(cval,S5P64XX_GPJ0DAT);                                                                                                   
		udelay(KEYPAD_DELAY);
		rval = ~(readl(key_base+S3C_KEYIFROW)) & ((1<<KEYPAD_ROWS)-1);
		DPRINTK("####rval %x cval %x \n", rval, cval);
		//if(row_value != rval)
		*keymask_high |= (rval << ((i - 4) * KEYPAD_ROWS));
		}
#endif

	//TODO: add code preventing ghost key
	//

//	__raw_writel(0x11111111, S5P64XX_GPJ0CON);			// all output
	KEYCOL_WRITECON(0x11111111, S5P64XX_GPJ0CON);		// all output
	DPRINTK("%s, gpj0con:%x\n", __func__, __raw_readl(S5P64XX_GPJ0CON));
	//writel(KEYIFCOL_CLEAR, key_base+S3C_KEYIFCOL);

	return 0;

}

#else
static int keypad_scan(u32 *keymask_low, u32 *keymask_high)
{
	int i = 0;
	u32 cval,rval;
	u32 col_num=0;
	u32 row_value=0;

	__raw_writel(0,S5P64XX_GPJ0DAT);
	row_value =  ~(readl(key_base+S3C_KEYIFROW)) & ((1<<KEYPAD_ROWS)-1);

	for (i=0; i < 4; i++) {
		cval = (1 << i);
		__raw_writel(cval,S5P64XX_GPJ0DAT);
		udelay(KEYPAD_DELAY);
		rval = ~(readl(key_base+S3C_KEYIFROW)) & ((1<<KEYPAD_ROWS)-1);
		DPRINTK("####rval %x cval %x \n", rval, cval);
		if(row_value != rval)
			*keymask_low |= ((row_value ^ rval) << (i * KEYPAD_COLUMNS));

	}

	for (i=4; i < 8; i++) {
		cval = (1 << i);
		__raw_writel(cval,S5P64XX_GPJ0DAT);
		udelay(KEYPAD_DELAY);
		rval = ~(readl(key_base+S3C_KEYIFROW)) & ((1<<KEYPAD_ROWS)-1);
		DPRINTK("####rval %x cval %x \n", rval, cval);
		if(row_value != rval)
			*keymask_high |= ((row_value ^ rval) << ((i - 4) * KEYPAD_COLUMNS));

	}


	 
	writel(KEYIFCOL_CLEAR, key_base+S3C_KEYIFCOL);

	return 0;

}



#endif

#ifdef CONFIG_S5P64XX_LPAUDIO
static void change_idle_mode(struct work_struct *dummy)
{
	int ret = 0;
	
	if (previous_idle_mode == LPAUDIO_MODE) {
		ret = s5p6442_setup_lpaudio(NORMAL_MODE);
		previous_idle_mode = NORMAL_MODE;
	}
	if (ret)
		printk(KERN_ERR "Error changing cpuidle device\n");
}
static DECLARE_WORK(idlemode, change_idle_mode);
#endif /* CONFIG_S5P64XX_LPAUDIO */

static void process_input_report (struct s3c_keypad *keypad_data, u32 prevmask, u32 keymask, u32 index)
{
	struct input_dev             *dev = keypad_data->dev;
	int i=0;
	u32 press_mask = ((keymask ^ prevmask) & keymask); 
	u32 release_mask = ((keymask ^ prevmask) & prevmask); 

	i = 0;
	while (press_mask) {
		if (press_mask & 1) {
			input_report_key(dev, ((int*)dev->keycode)[i+index],1);
#ifdef CONFIG_S5P64XX_LPAUDIO
			if (((int*)dev->keycode)[i+index] != KEY_VOLUMEUP &&
				((int*)dev->keycode)[i+index] != KEY_VOLUMEDOWN) {
				schedule_work(&idlemode);
			}
#endif /* CONFIG_S5P6442_LPAUDIO */
			PRINTK(": Pressed (index: %d, Keycode: %d)\n", i+index, ((int*)dev->keycode)[i+index]);
		}
		press_mask >>= 1;
		i++;
	}

	i = 0;
	while (release_mask) {
		if (release_mask & 1) {
			input_report_key(dev, ((int*)dev->keycode)[i+index],0);
			PRINTK(": Released (index: %d, Keycode: %d)\n", i+index, ((int*)dev->keycode)[i+index]);
		}
		release_mask >>= 1;
		i++;
	}
}

static inline void process_special_key (struct s3c_keypad *s3c_keypad, u32 keymask_low, u32 keymask_high)
{
	struct input_dev              *dev = s3c_keypad->dev;
	struct s3c_keypad_extra       *extra = s3c_keypad->extra;
	struct s3c_keypad_special_key *special_key = extra->special_key;
	static int prev_bitmask = 0;
	int i;

	for (i=0; i<extra->special_key_num; i++, special_key+=1)
	{
	    if (keymask_low == special_key->mask_low 
		    && keymask_high == special_key->mask_high 
		    && !(prev_bitmask & (1<<i))) 
		{
            input_report_key(dev, special_key->keycode, 1);
			PRINTK(": Pressed (Keycode: %d, SPECIAL KEY)\n", special_key->keycode);
			prev_bitmask |= (1<<i);
			continue;
		}
		if ((prev_bitmask & (1<<i)) 
 		    && keymask_low == 0 
	    	&& keymask_high == 0)
		{
	       	input_report_key(dev, special_key->keycode, 0);
			PRINTK(": Released (Keycode: %d, SPECIAL KEY)\n", special_key->keycode);
			prev_bitmask ^= (1<<i);
		}
	}
}

static void keypad_timer_handler(unsigned long data)
{
	u32 keymask_low = 0, keymask_high = 0;
	struct s3c_keypad *pdata = (struct s3c_keypad *)data;

	keypad_scan(&keymask_low, &keymask_high);

	process_special_key(pdata, keymask_low, keymask_high);

	if (keymask_low != prevmask_low) {
		DPRINTK(":low mask prev(%x), cur(%x)\n", prevmask_low, keymask_low);
		process_input_report (pdata, prevmask_low, keymask_low, 0);
		prevmask_low = keymask_low;
	}

	if (keymask_high != prevmask_high) {
		DPRINTK(":high mask prev(%x), cur(%x)\n", prevmask_high, keymask_high);
		process_input_report (pdata, prevmask_high, keymask_high, 32);
		prevmask_high = keymask_high;
	}

	if (keymask_low | keymask_high) {
                mod_timer(&keypad_timer,jiffies + HZ/10);
//		mod_timer(&keypad_timer,jiffies + SCAN_INTERVAL);
	} else {
		writel(KEYIFCON_INIT, key_base+S3C_KEYIFCON);
	}	
}


// for factory process /////////////////////////

static int s3c_keypad_show_key_pressed(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i;
	int ret=0;
	int num_gpiokeys;
	struct s3c_keypad_gpio_key * gpiokeys;
	const char pressed_string[] = "PRESS";
	const char released_string[] = "RELEASE";
	struct s3c_keypad *keypad = (struct s3c_keypad *)dev_get_drvdata(dev);
	struct s3c_keypad_extra *extra = (struct s3c_keypad_extra *)keypad->extra;

	if(prevmask_low || prevmask_high)
		{
		ret=1;
		goto out;
		}

	num_gpiokeys = extra->gpio_key_num;
	gpiokeys = extra->gpio_key;

	for(i=0; i<num_gpiokeys; i++)
		{
		int state = gpio_get_value(gpiokeys[i].gpio);
		if(state != gpiokeys[i].state_upset)
			{
			PRINTK("detect GPIO KEY(%d)\n", gpiokeys[i].gpio);
			ret=1;
			break;
			}
		}
out:
	if(ret)
		return snprintf(buf, PAGE_SIZE, "%s\n", pressed_string);
	else
		return snprintf(buf, PAGE_SIZE, "%s\n", released_string);

}

static int s3c_keypad_store_key_pressed(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
	return len;
}

static int s3c_keypad_show_what_key(struct device *dev, struct device_attribute *attr, char *buf)
{

	return snprintf(buf, PAGE_SIZE, "h:0x%x l:0x%x\n", prevmask_high, prevmask_low);
}

static int s3c_keypad_store_what_key(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
	return len;
}

static DEVICE_ATTR(key_pressed, 0644,
                        s3c_keypad_show_key_pressed,
                        s3c_keypad_store_key_pressed);

static DEVICE_ATTR(what_key, 0644,
                        s3c_keypad_show_what_key,
                        s3c_keypad_store_what_key);

static irqreturn_t s3c_keypad_isr(int irq, void *dev_id)
{
#ifdef  CONFIG_CPU_FREQ
	set_dvfs_perf_level();
#endif
	/* disable keypad interrupt and schedule for keypad timer handler */
	writel(readl(key_base+S3C_KEYIFCON) & ~(INT_F_EN|INT_R_EN), key_base+S3C_KEYIFCON);

	keypad_timer.expires = jiffies + (HZ/100);
 //       keypad_timer.expires = jiffies + FIRST_SCAN_INTERVAL;
	if( !timer_pending(&keypad_timer) ){
		add_timer(&keypad_timer);
	} else {
		mod_timer(&keypad_timer,keypad_timer.expires);
	}
	/*Clear the keypad interrupt status*/
	writel(KEYIFSTSCLR_CLEAR, key_base+S3C_KEYIFSTSCLR);

	return IRQ_HANDLED;
}

static irqreturn_t slide_int_handler(int irq, void *dev_id)
{
	struct s3c_keypad       *s3c_keypad = (struct s3c_keypad *) dev_id;
	struct s3c_keypad_slide *slide      = s3c_keypad->extra->slide;
	int state;

#ifdef  CONFIG_CPU_FREQ
	set_dvfs_perf_level();
#endif
	state = gpio_get_value(slide->gpio) ^ slide->state_upset;
	DPRINTK(": changed Slide state (%d)\n", state);

	input_report_switch(s3c_keypad->dev, SW_LID, state);
	input_sync(s3c_keypad->dev);

	return IRQ_HANDLED;
}

extern void qt602240_release_all_finger();

static irqreturn_t gpio_int_handler(int irq, void *dev_id)
{
	struct s3c_keypad          *s3c_keypad = (struct s3c_keypad *) dev_id;
	struct input_dev           *dev = s3c_keypad->dev;
	struct s3c_keypad_extra    *extra = s3c_keypad->extra;
	struct s3c_keypad_gpio_key *gpio_key = extra->gpio_key;
	int i,state;

   	DPRINTK(": gpio interrupt (IRQ: %d)\n", irq);
#ifdef  CONFIG_CPU_FREQ
	set_dvfs_perf_level();
#endif
	for (i=0; i<extra->gpio_key_num; i++)
	{
		if (gpio_key[i].eint == irq)
		{
			gpio_key = &gpio_key[i];
			break;
		}
	}

	if (gpio_key != NULL)
	{
		state = gpio_get_value(gpio_key->gpio);
       	        DPRINTK(": gpio state (%d, %d)\n", state , state ^ gpio_key->state_upset);
		state ^= gpio_key->state_upset;

/*        if(wakeup_flag_for_key && !state)
        {
            input_report_key(dev, gpio_key->keycode, 1);
            PRINTK(": Guess pressed (Keycode: %d, GPIO KEY)\n", gpio_key->keycode);
        }
*/
        if(state) {
#ifdef CONFIG_S5P64XX_LPAUDIO
                     //   if (((int*)dev->keycode)[i+index] != KEY_VOLUMEUP &&
                       //         ((int*)dev->keycode)[i+index] != KEY_VOLUMEDOWN) {
                                schedule_work(&idlemode);
                       // }
#endif /* CONFIG_S5P6442_LPAUDIO */

            input_report_key(dev, gpio_key->keycode, 1);
            PRINTK(": Pressed (Keycode: %d, GPIO KEY)\n", gpio_key->keycode);
			 if(gpio_key->keycode == KEY_ENDCALL)
			 	{
			 	//For apollo: 
			 	//Some application(ex. indicator) can have wrong status about touch input after sleep with a finger pressed.
			 	//so, it generates pseudo touch up events.
#ifdef APOLLO_TBD
			 	qt602240_release_all_finger();
#endif
			 	}		
        }
        else  {
			input_report_key(dev, gpio_key->keycode, 0);
			PRINTK(": Released (Keycode: %d, GPIO KEY)\n", gpio_key->keycode);
		}
	}

    wakeup_flag_for_key = 0;

	return IRQ_HANDLED;
}

static int __init s3c_keypad_probe(struct platform_device *pdev)
{
	struct resource *res, *keypad_mem, *keypad_irq;
	struct input_dev *input_dev;
	struct s3c_keypad *s3c_keypad;
	int ret, size;
	int key;
        struct s3c_keypad_extra    	*extra = NULL;
	struct s3c_keypad_slide    	*slide = NULL;
	struct s3c_keypad_special_key    *special_key;
        struct s3c_keypad_gpio_key 	*gpio_key;
	int i;
        char * input_dev_name;
	
	printk("##########s3c_keypad_probe \n");
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev,"no memory resource specified\n");
		return -ENOENT;
	}

	size = (res->end - res->start) + 1;

	keypad_mem = request_mem_region(res->start, size, pdev->name);
	if (keypad_mem == NULL) {
		dev_err(&pdev->dev, "failed to get memory region\n");
		ret = -ENOENT;
		goto err_req;
	}

	key_base = ioremap(res->start, size);
	if (key_base == NULL) {
		printk(KERN_ERR "Failed to remap register block\n");
		ret = -ENOMEM;
		goto err_map;
	}

	keypad_clock = clk_get(&pdev->dev, "keypad");
	if (IS_ERR(keypad_clock)) {
		dev_err(&pdev->dev, "failed to find keypad clock source\n");
		ret = PTR_ERR(keypad_clock);
		goto err_clk;
	}

	s3c_keypad = kzalloc(sizeof(struct s3c_keypad), GFP_KERNEL);
	input_dev = input_allocate_device();
         input_dev_name = (char *)kmalloc(sizeof("s3c-keypad-xxxx"), GFP_KERNEL);
	if (!s3c_keypad || !input_dev || !input_dev_name) {
		ret = -ENOMEM;
		goto err_alloc;
	}

#ifdef __KEYPAD_CHECK_HW_REV_PIN__
	DPRINTK(": hw_rev_pin 0x%04x\n", apollo_get_hw_rev_pin_value());
	for (i=0; i<sizeof(s3c_keypad_extra)/sizeof(struct s3c_keypad_extra); i++)
	{
		if (s3c_keypad_extra[i].board_num == apollo_get_hw_rev_pin_value()) {
			extra = &s3c_keypad_extra[i];
			//sprintf(input_dev_name, "%s%s%04x", DEVICE_NAME, "-rev", system_rev); 
			PRINTK(": board rev 0x%04x is detected!\n", s3c_keypad_extra[i].board_num);
			break;
		}
	}
#endif

	if(!extra) {
		extra = &s3c_keypad_extra[0];
	}
	sprintf(input_dev_name, "%s", DEVICE_NAME); 			//default revison 		
	
	PRINTK(": input device name: %s.\n", input_dev_name);

	// check whether using key matrix
	if(extra->keycon_matrix)
		use_key_matrix = 1;
	else
		use_key_matrix = 0;

	if(use_key_matrix)
	{
		clk_enable(keypad_clock);
	}

	platform_set_drvdata(pdev, s3c_keypad);
	s3c_keypad->dev = input_dev;
	g_input_dev = input_dev;
        s3c_keypad->extra = extra;
	slide = extra->slide;
	special_key = extra->special_key;
        gpio_key = extra->gpio_key;

	if(use_key_matrix)
	{
		writel(KEYIFCON_INIT, key_base+S3C_KEYIFCON);
		writel(KEYIFFC_DIV, key_base+S3C_KEYIFFC);

		/* Set GPIO Port for keypad mode and pull-up disable*/
		s3c_setup_keypad_cfg_gpio(KEYPAD_ROWS, KEYPAD_COLUMNS);

		writel(KEYIFCOL_CLEAR, key_base+S3C_KEYIFCOL);
		KEYCOL_WRITEDAT(0, S5P64XX_GPJ0DAT);		// columns is gpio output

		/* create and register the input driver */
		//set_bit(EV_KEY, input_dev->evbit);
		//set_bit(EV_REP, input_dev->evbit);
		s3c_keypad->nr_rows = KEYPAD_ROWS;
		s3c_keypad->no_cols = KEYPAD_COLUMNS;
		s3c_keypad->total_keys = MAX_KEYPAD_NR;

		for(key = 0; key < s3c_keypad->total_keys; key++){
			if(keypad_keycode[key] != KEY_NOT_DEFINE)
	            input_set_capability(input_dev, EV_KEY, keypad_keycode[key]);
	        }
	}
//	for(key = 0; key < s3c_keypad->total_keys; key++){
//		code = s3c_keypad->keycodes[key] = keypad_keycode[key];
/*		if(code<=0)
			continue;
		set_bit(code & KEY_MAX, input_dev->keybit);*/
//	}

	for (i=0; i<extra->special_key_num; i++ ){
        	input_set_capability(input_dev, EV_KEY, (special_key+i)->keycode);
	}

        for (i=0; i<extra->gpio_key_num; i++ ){
        	input_set_capability(input_dev, EV_KEY, (gpio_key+i)->keycode);
	}

	if (extra->slide != NULL)
        	input_set_capability(input_dev, EV_SW, SW_LID);

	input_dev->name = DEVICE_NAME;
	input_dev->phys = "s3c-keypad/input0";
	
	input_dev->id.bustype = BUS_HOST;
	input_dev->id.vendor = 0x0001;
	input_dev->id.product = 0x0001;
	input_dev->id.version = 0x0001;

#if CONFIG_BOARD_REVISION == CONFIG_APOLLO_EMUL
	if(apollo_get_saved_lcd_id_value())
		input_dev->keycode = keypad_keycode_alt;
	else
		input_dev->keycode = keypad_keycode;
#else
	input_dev->keycode = keypad_keycode;
#endif

	/* Scan timer init */
	init_timer(&keypad_timer);
	keypad_timer.function = keypad_timer_handler;
	keypad_timer.data = (unsigned long)s3c_keypad;

	if(use_key_matrix)
	{
		/* For IRQ_KEYPAD */
		keypad_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
		if (keypad_irq == NULL) {
			dev_err(&pdev->dev, "no irq resource specified\n");
			ret = -ENOENT;
			goto err_irq;
		}
	}
	if (slide != NULL)
	{

		s3c_gpio_cfgpin(slide->gpio, S3C_GPIO_SFN(slide->gpio_af));
		s3c_gpio_setpull(slide->gpio, S3C_GPIO_PULL_NONE);

		set_irq_type(slide->eint, IRQ_TYPE_EDGE_BOTH);

		ret = request_irq(slide->eint, slide_int_handler, IRQF_DISABLED,
		    "s3c_keypad gpio key", (void *)s3c_keypad);
		if (ret) {
			printk(KERN_ERR "request_irq(%d) failed (IRQ for SLIDE) !!!\n", slide->eint);
			ret = -EIO;
			goto err_irq;
		}
	}

        for (i=0; i<extra->gpio_key_num; i++, gpio_key+=1)
	{
		s3c_gpio_cfgpin(gpio_key->gpio, S3C_GPIO_SFN(gpio_key->gpio_af));
		s3c_gpio_setpull(gpio_key->gpio, S3C_GPIO_PULL_NONE);

		set_irq_type(gpio_key->eint, IRQ_TYPE_EDGE_BOTH);

		ret = request_irq(gpio_key->eint, gpio_int_handler, IRQF_DISABLED,
			    "s3c_keypad gpio key", (void *)s3c_keypad);
		if (ret) {
			printk(KERN_ERR "request_irq(%d) failed (IRQ for GPIO KEY) !!!\n", gpio_key->eint);
			ret = -EIO;
			goto err_irq;
		}

	}
#ifdef __KEYPAD_FILTER_SETTING_FOR_GPIO_KEY__
	set_eint_filter();
#endif

	if(use_key_matrix)
	{
		ret = request_irq(keypad_irq->start, s3c_keypad_isr, IRQF_SAMPLE_RANDOM,
			DEVICE_NAME, (void *) pdev);
		if (ret) {
			printk("request_irq failed (IRQ_KEYPAD) !!!\n");
			ret = -EIO;
			goto err_irq;
		}
	}

	ret = input_register_device(input_dev);
	if (ret) {
		printk("Unable to register s3c-keypad input device!!!\n");
		goto out;
	}

	// for factory process
    ret = device_create_file(&(pdev->dev), &dev_attr_key_pressed);
    if (ret < 0)
            printk(KERN_WARNING "s3c-keypad: failed to add entries\n");

    ret = device_create_file(&(pdev->dev), &dev_attr_what_key);
    if (ret < 0)
            printk(KERN_WARNING "s3c-keypad: failed to add entries\n");

	keypad_timer.expires = jiffies + (HZ/10);

	if(use_key_matrix)
	{
		if( !timer_pending(&keypad_timer) ){
			add_timer(&keypad_timer);
		} else {
			mod_timer(&keypad_timer,keypad_timer.expires);
		}
	}

	printk( DEVICE_NAME " Initialized\n");
	return 0;

out:
	if(use_key_matrix)
	{
		free_irq(keypad_irq->start, input_dev);
		free_irq(keypad_irq->end, input_dev);
	}

	if (slide != NULL)
		free_irq(extra->slide->eint, s3c_keypad);

    gpio_key = extra->gpio_key;
	for (i=0; i<extra->gpio_key_num; i++, gpio_key+=1)
		free_irq(gpio_key->eint, s3c_keypad);

err_irq:
	input_free_device(input_dev);
	kfree(s3c_keypad);
	
err_alloc:
	if(use_key_matrix)
	{
		clk_disable(keypad_clock);
		clk_put(keypad_clock);
	}

err_clk:
	iounmap(key_base);

err_map:
	release_resource(keypad_mem);
	kfree(keypad_mem);

err_req:
	return ret;
}

static int s3c_keypad_remove(struct platform_device *pdev)
{
	struct s3c_keypad *s3c_keypad = platform_get_drvdata(pdev);
	struct input_dev *input_dev = s3c_keypad->dev;
    struct s3c_keypad_extra *extra = s3c_keypad->extra;
	struct s3c_keypad_slide *slide = extra->slide;
    struct s3c_keypad_gpio_key *gpio_key = extra->gpio_key;

	if(use_key_matrix)
	{
		writel(KEYIFCON_CLEAR, key_base+S3C_KEYIFCON);

		if(keypad_clock) {
			clk_disable(keypad_clock);
			clk_put(keypad_clock);
			keypad_clock = NULL;
		}
	}

	if (slide)
	        free_irq(slide->eint, (void *) s3c_keypad);

        if (gpio_key)
	{
		int i;
		for (i=0; i<extra->gpio_key_num; i++, gpio_key+=1)
        		free_irq(gpio_key->eint, (void *) s3c_keypad);
	}

	input_unregister_device(input_dev);
	iounmap(key_base);
	kfree(pdev->dev.platform_data);

	if(use_key_matrix)
	{
		free_irq(IRQ_KEYPAD, (void *) pdev);
		del_timer(&keypad_timer);	
	}

	printk(DEVICE_NAME " Removed.\n");
	return 0;
}

#ifdef CONFIG_PM
static unsigned int keyifcon, keyiffc;

static int s3c_keypad_suspend(struct platform_device *dev, pm_message_t state)
{
        struct s3c_keypad *s3c_keypad = platform_get_drvdata(dev);
	struct s3c_keypad_extra *extra = s3c_keypad->extra;	

	DPRINTK("suspend\n");

	if(use_key_matrix)
	{
		keyifcon = readl(key_base+S3C_KEYIFCON);
		keyiffc = readl(key_base+S3C_KEYIFFC);

	        writel(KEYIFCON_CLEAR, key_base+S3C_KEYIFCON);	
		
		if (!extra->wakeup_by_keypad)
		{
	            s3c_keypad_suspend_cfg_gpio(KEYPAD_ROWS, KEYPAD_COLUMNS);
		}
		
		disable_irq(IRQ_KEYPAD);
	}

#if 0
        if (gpio_key)
	{
		int i;
		for (i=0; i<extra->gpio_key_num; i++, gpio_key+=1)
        		disable_irq(gpio_key->eint);
	}
#endif

#ifdef __KEYPAD_FILTER_SETTING_FOR_GPIO_KEY__
	reset_eint_filter();
#endif

	if(use_key_matrix)
		clk_disable(keypad_clock);

	return 0;
}

extern int charger_wakeup;

static int s3c_keypad_resume(struct platform_device *dev)
{
    struct s3c_keypad *s3c_keypad = platform_get_drvdata(dev);

	DPRINTK("resume\n");

	if(wakeup_flag_for_key== 249)
	{
		input_report_key(s3c_keypad->dev, 249, 1);
		input_report_key(s3c_keypad->dev, 249, 0);
	}
	
	if(wakeup_flag_for_key== 102)
	{
		input_report_key(s3c_keypad->dev, 102, 1);
		input_report_key(s3c_keypad->dev, 102, 0);
	}	
    	wakeup_flag_for_key = 0;

	if(use_key_matrix)
	{
	    s3c_setup_keypad_cfg_gpio(KEYPAD_ROWS, KEYPAD_COLUMNS);
		clk_enable(keypad_clock);

		del_timer (&keypad_timer);

		prevmask_low = 0;
		prevmask_high = 0;
		s3c_keypad_isr (0, NULL);	
		enable_irq(IRQ_KEYPAD);
	}
#ifdef __KEYPAD_FILTER_SETTING_FOR_GPIO_KEY__
	set_eint_filter();
#endif



	// If charger is connected, then report fake HOME key event.
	if(charger_wakeup)
	{
			input_report_key(s3c_keypad->dev, KEY_HOME, 1);	// temporary by SYS.LSI
			input_report_key(s3c_keypad->dev, KEY_HOME, 0);	// temporary by SYS.LSI
	}
		
#if 0
    if (gpio_key)
    {
        int i;

        //PRINTK(": Pseudo pressed (Keycode: %d, GPIO KEY)\n", gpio_key->keycode);
		for (i=0; i<extra->gpio_key_num; i++, gpio_key+=1)
		{
        		enable_irq(gpio_key->eint);
			gpio_int_handler (gpio_key->eint, (void *) s3c_keypad);
		}
	}
#endif

	if(use_key_matrix)
	{
		writel(keyifcon, key_base+S3C_KEYIFCON);
		writel(keyiffc, key_base+S3C_KEYIFFC);

		 /* Set GPIO Port for keypad mode and pull-up disable*/
	//        s3c_setup_keypad_cfg_gpio(KEYPAD_ROWS, KEYPAD_COLUMNS);
		writel(KEYIFCOL_CLEAR, key_base+S3C_KEYIFCOL);
		KEYCOL_WRITEDAT(0, S5P64XX_GPJ0DAT);		// columns is gpio output
	}

	return 0;
}
#else
#define s3c_keypad_suspend NULL
#define s3c_keypad_resume  NULL
#endif /* CONFIG_PM */

static struct platform_driver s3c_keypad_driver = {
	.probe		= s3c_keypad_probe,
	.remove		= s3c_keypad_remove,
	.suspend	= s3c_keypad_suspend,
	.resume		= s3c_keypad_resume,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "s3c-keypad",
	},
};

static int __init s3c_keypad_init(void)
{
	int ret;

	ret = platform_driver_register(&s3c_keypad_driver);
	
	if(!ret)
	   printk(KERN_INFO "S3C Keypad Driver\n");

	return ret;
}

static void __exit s3c_keypad_exit(void)
{
	platform_driver_unregister(&s3c_keypad_driver);
}

module_init(s3c_keypad_init);
module_exit(s3c_keypad_exit);

MODULE_AUTHOR("Samsung");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("KeyPad interface for Samsung S3C");

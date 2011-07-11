/*
 *	HEADSET device detection driver.
 *
 *	Copyright (C) 2009 Samsung Electronics, Inc.
 *
 *	Authors:
 *		Uk Kim <w0806.kim@samsung.com>
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; version 2 of the License.
 */

#include <linux/module.h>
#include <linux/sysdev.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/switch.h>
#include <linux/input.h>
#include <linux/timer.h>
#include <linux/wakelock.h>

#include <mach/hardware.h>
#include <mach/apollo.h>
#include <mach/gpio.h>
#include <plat/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/irqs.h>
#include <asm/mach-types.h>

#include <mach/sec_headset.h>

#define CONFIG_DEBUG_SEC_HEADSET
#define SUBJECT "HEADSET_DRIVER"

#ifdef CONFIG_DEBUG_SEC_HEADSET
#define SEC_HEADSETDEV_DBG(format,...)\
	printk ("[ "SUBJECT " (%s,%d) ] " format "\n", __func__, __LINE__, ## __VA_ARGS__);

#else
#define DEBUG_LOG(format,...)
#endif

#define KEYCODE_SENDEND 248

#define DETECTION_CHECK_COUNT	3
#define	DETECTION_CHECK_TIME	get_jiffies_64() + (HZ/10)// 1000ms / 20 = 50ms
#define	SEND_END_ENABLE_TIME	get_jiffies_64() + (HZ*2)// 1000ms * 2 = 2sec

#define SEND_END_CHECK_COUNT	3
#define SEND_END_CHECK_TIME get_jiffies_64() + (HZ/15) //1000ms / 15 = 67ms

static struct platform_driver sec_headset_driver;

struct class *headset_class;
EXPORT_SYMBOL(headset_class);
static struct device *headset_selector_fs;				// Sysfs device, this is used for communication with Cal App.
EXPORT_SYMBOL(headset_selector_fs);
extern int s3c_adc_get_adc_data(int channel);
extern int apollo_get_remapped_hw_rev_pin();

struct sec_headset_info {
	struct sec_headset_port port;
	struct input_dev *input;
};

static struct sec_headset_info *hi;

struct switch_dev switch_headset_detection = {
		.name = "h2w",
};

struct switch_dev switch_sendend = {
		.name = "send_end",
};

static struct timer_list headset_detect_timer;
static struct timer_list send_end_key_event_timer;

static unsigned int current_headset_type_status;
static unsigned int headset_detect_timer_token;
static unsigned int send_end_key_timer_token;
static unsigned int send_end_irq_token;
static unsigned short int headset_status;
static struct wake_lock headset_sendend_wake_lock;

short int get_headset_status()
{
	SEC_HEADSETDEV_DBG(" headset_status %d", current_headset_type_status);

	if( current_headset_type_status == SEC_HEADSET_4_POLE_DEVICE 
		|| current_headset_type_status == SEC_HEADSET_3_POLE_DEVICE)
	{
		headset_status = 1;
	}
	else
	{
		headset_status = 0;
	}
	
	return headset_status;
}

EXPORT_SYMBOL(get_headset_status);

static void headset_input_selector(int headset_type_status)
{
	SEC_HEADSETDEV_DBG("headset_type_status = 0X%x", headset_type_status);
	
	switch(headset_type_status)
	{
		case SEC_HEADSET_3_POLE_DEVICE:
		case SEC_HEADSET_4_POLE_DEVICE:	
		{
			//gpio_set_value(GPIO_EAR_USB_SW, GPIO_LEVEL_LOW);	// Low:headset, High: uUSB(TV_OUT)
			break;
		}
		case SEC_TVOUT_DEVICE :
		{
			//gpio_set_value(GPIO_EAR_USB_SW, GPIO_LEVEL_HIGH);	// Low:headset, High: uUSB(TV_OUT)
			break;
		}
		case SEC_UNKNOWN_DEVICE:
		{
			printk("unknown headset device attached. User must select headset device type\n");
			break;
		}
		default :
			printk(KERN_ERR "wrong selector value\n");
			break;			
	}
}

//WORK QUEING FUNCTION
static void headset_type_detect_change(struct work_struct *ignored)
{
	int adc = 0;
	struct sec_gpio_info   *det_headset = &hi->port.det_headset;
	struct sec_gpio_info   *send_end = &hi->port.send_end;
	int state = gpio_get_value(det_headset->gpio) ^ det_headset->low_active;

//	gpio_set_value(GPIO_MICBIAS_EN, 1);
//	gpio_set_value(GPIO_ADC_EN,1); // ADC_EN
	
	msleep(100);
	
	if (state)
	{
#if (CONFIG_BOARD_REVISION != CONFIG_APOLLO_EMUL)
		gpio_set_value(GPIO_MICBIAS2_EN, 1);
		mdelay(200);
#endif
		adc = s3c_adc_get_adc_data(SEC_HEADSET_ADC_CHANNEL);
		SEC_HEADSETDEV_DBG("headset detect : ADC value = %d\n", adc);

		if(adc < 0)
		{
			printk(KERN_ERR "[Headset] ADC driver unavailable \n");
			wake_unlock(&headset_sendend_wake_lock);
			return ;
		}
		
		if(adc < 5)
		{
			printk("3 pole headset attatched : adc = %d\n", adc);
#if (CONFIG_BOARD_REVISION != CONFIG_APOLLO_EMUL)
			gpio_set_value(GPIO_MICBIAS2_EN, 0);
#endif
			current_headset_type_status = SEC_HEADSET_3_POLE_DEVICE;
			switch_set_state(&switch_headset_detection, current_headset_type_status);
		}
        else if((adc < 3000 && adc >=1000) || (adc > 180 && adc < 200))
		{
			printk("4 pole  headset attached : adc = %d\n", adc);			
			enable_irq (send_end->eint);
			send_end_irq_token++;			
			current_headset_type_status = SEC_HEADSET_4_POLE_DEVICE;
			switch_set_state(&switch_headset_detection, current_headset_type_status);			
		}
/*		else if(adc >=2500 && adc <2700)
		{
			printk("TV_out headset attached : adc = %d\n", adc);
			current_headset_type_status = SEC_TVOUT_DEVICE;
			gpio_set_value(GPIO_MICBIAS_EN, 0);
			switch_set_state(&switch_headset_detection, current_headset_type_status);			
		}*/
		else
		{
			printk("headset detected but unknown device : adc = %d\n", adc);
#if (CONFIG_BOARD_REVISION != CONFIG_APOLLO_EMUL)
			gpio_set_value(GPIO_MICBIAS2_EN, 0);
#endif
			current_headset_type_status = SEC_UNKNOWN_DEVICE;
			switch_set_state(&switch_headset_detection, current_headset_type_status);			
		}			
			switch_set_state(&switch_headset_detection, current_headset_type_status);
			headset_input_selector(current_headset_type_status);
			
	}
	else
	{
		printk(KERN_ALERT "Error : mic bias enable complete but headset detached!!\n");
		current_headset_type_status = SEC_HEADSET_NO_DEVICE;
		gpio_set_value(GPIO_MICBIAS_EN, 0);
	}	

	// gpio_set_value(GPIO_ADC_EN,0); // ADC_DISABLE
	msleep(500);
	wake_unlock(&headset_sendend_wake_lock);
}

static DECLARE_DELAYED_WORK(detect_headset_type_work, headset_type_detect_change);

static void headset_detect_change(struct work_struct *ignored)
{
	struct sec_gpio_info   *det_headset = &hi->port.det_headset;
	struct sec_gpio_info   *send_end = &hi->port.send_end;
	int state;
	SEC_HEADSETDEV_DBG("");
	del_timer(&headset_detect_timer);
	cancel_delayed_work_sync(&detect_headset_type_work);
	state = gpio_get_value(det_headset->gpio) ^ det_headset->low_active;

	if (state && !send_end_irq_token)
	{		
		wake_lock(&headset_sendend_wake_lock);
		SEC_HEADSETDEV_DBG("HEADSET dev attached timer start\n");
		headset_detect_timer_token = 0;
		headset_detect_timer.expires = DETECTION_CHECK_TIME;
		add_timer(&headset_detect_timer);
	}
	else if(!state)
	{
		current_headset_type_status = SEC_HEADSET_NO_DEVICE;
		switch_set_state(&switch_headset_detection, current_headset_type_status);
		printk("HEADSET dev detached %d \n", send_end_irq_token);			
		headset_status = SEC_HEADSET_NO_DEVICE;
		if(send_end_irq_token > 0)
		{
#if (CONFIG_BOARD_REVISION != CONFIG_APOLLO_EMUL)
			gpio_set_value(GPIO_MICBIAS2_EN, 0);
#endif
			disable_irq (send_end->eint);
			send_end_irq_token--;
		}
		wake_unlock(&headset_sendend_wake_lock);
	}
	else
		SEC_HEADSETDEV_DBG("Headset state does not valid. or send_end event");

}

static void sendend_switch_change(struct work_struct *ignored)
{

	struct sec_gpio_info   *det_headset = &hi->port.det_headset;
	struct sec_gpio_info   *send_end = &hi->port.send_end;
	int state, headset_state;
	SEC_HEADSETDEV_DBG("");
	del_timer(&send_end_key_event_timer);
	send_end_key_timer_token = 0;
	
	headset_state = gpio_get_value(det_headset->gpio) ^ det_headset->low_active;
	state = gpio_get_value(send_end->gpio) ^ send_end->low_active;

	if(headset_state && send_end_irq_token)//headset connect && send irq enable
	{
		if(!state)
		{
			SEC_HEADSETDEV_DBG(KERN_ERR "sendend isr work queue\n");
			switch_set_state(&switch_sendend, state);
			input_report_key(hi->input, KEYCODE_SENDEND, state);
			input_sync(hi->input);
			printk("SEND/END %s.\n", "released");
			msleep(500);
			wake_unlock(&headset_sendend_wake_lock);
		}else
		{
			wake_lock(&headset_sendend_wake_lock);
			send_end_key_event_timer.expires = SEND_END_CHECK_TIME; 
			add_timer(&send_end_key_event_timer);
			switch_set_state(&switch_sendend, state);
			SEC_HEADSETDEV_DBG("SEND/END %s.timer start \n", "pressed");
		}

	}else
	{
		SEC_HEADSETDEV_DBG("SEND/END Button is %s but headset disconnect or irq disable.\n", state?"pressed":"released");
	}
}

static DECLARE_WORK(headset_detect_work, headset_detect_change);
static DECLARE_WORK(sendend_switch_work, sendend_switch_change);

//IRQ Handler
static irqreturn_t detect_irq_handler(int irq, void *dev_id)
{
	SEC_HEADSETDEV_DBG("headset isr");
	schedule_work(&headset_detect_work);
	return IRQ_HANDLED;
}
 
static void headset_detect_timer_handler(unsigned long arg)
{
	struct sec_gpio_info *det_headset = &hi->port.det_headset;
	int state;
	
	state = gpio_get_value(det_headset->gpio) ^ det_headset->low_active;

	if(state)
	{
		SEC_HEADSETDEV_DBG("headset_detect_timer_token is %d\n", headset_detect_timer_token);
		if(headset_detect_timer_token < DETECTION_CHECK_COUNT)
		{
			headset_detect_timer.expires = DETECTION_CHECK_TIME;
			add_timer(&headset_detect_timer);
			headset_detect_timer_token++;
		}
		else if(headset_detect_timer_token == DETECTION_CHECK_COUNT)
		{
			headset_detect_timer.expires = SEND_END_ENABLE_TIME;
//			add_timer(&headset_detect_timer);
			gpio_set_value(GPIO_MICBIAS_EN, 1);
			headset_detect_timer_token = 0;
//			schedule_delayed_work(&detect_headset_type_work,200);
			schedule_work(&detect_headset_type_work);
		}
		else if(headset_detect_timer_token == 4)
		{
//			gpio_set_value(GPIO_MICBIAS_EN, 1); 
//			schedule_delayed_work(&ear_adc_cal_work, 200);
#if (CONFIG_BOARD_REVISION != CONFIG_APOLLO_EMUL)
			schedule_work(&detect_headset_type_work);
#endif
			SEC_HEADSETDEV_DBG("mic bias enable add work queue \n");
			headset_detect_timer_token = 0;
		}
		else
			printk(KERN_ALERT "wrong headset_detect_timer_token count %d", headset_detect_timer_token);
	}
	else
		printk(KERN_ALERT "headset detach!! %d", headset_detect_timer_token);
}


static void send_end_key_event_timer_handler(unsigned long arg)
{
	struct sec_gpio_info   *det_headset = &hi->port.det_headset;
	struct sec_gpio_info   *send_end = &hi->port.send_end;
	int sendend_state, headset_state = 0;
	
	headset_state = gpio_get_value(det_headset->gpio) ^ det_headset->low_active;
	sendend_state = gpio_get_value(send_end->gpio) ^ send_end->low_active;

	if(headset_state && sendend_state)
	{
		if(send_end_key_timer_token < SEND_END_CHECK_COUNT)
		{	
			send_end_key_timer_token++;
			send_end_key_event_timer.expires = SEND_END_CHECK_TIME; 
			add_timer(&send_end_key_event_timer);
			SEC_HEADSETDEV_DBG("SendEnd Timer Restart %d", send_end_key_timer_token);
		}
		else if(send_end_key_timer_token == SEND_END_CHECK_COUNT)
		{
			printk("SEND/END is pressed\n");
			input_report_key(hi->input, KEYCODE_SENDEND, 1);
			input_sync(hi->input);
			send_end_key_timer_token = 0;
		}
		else
			printk(KERN_ALERT "[HEADSET]wrong timer counter %d\n", send_end_key_timer_token);
	}else
			printk(KERN_ALERT "[HEADSET]GPIO Error\n");
}

static irqreturn_t send_end_irq_handler(int irq, void *dev_id)
{
   struct sec_gpio_info   *det_headset = &hi->port.det_headset;
   int headset_state;

	SEC_HEADSETDEV_DBG("sendend isr");
	del_timer(&send_end_key_event_timer);
	headset_state = gpio_get_value(det_headset->gpio) ^ det_headset->low_active;

	if (headset_state)
	{
		schedule_work(&sendend_switch_work);		
	}
		  
	return IRQ_HANDLED;
}
//USER can select headset type if driver can't check the headset type
static int strtoi(char *buf)
{
	int ret;
	ret = buf[0]-48;
	return ret;
}
static ssize_t select_headset_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk(KERN_INFO "[HEADSET] %s : operate nothing\n", __FUNCTION__);

	return 0;
}
static ssize_t select_headset_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int value = 0;
	struct sec_gpio_info   *det_headset = &hi->port.det_headset;
	struct sec_gpio_info   *send_end = &hi->port.send_end;
	int state = gpio_get_value(det_headset->gpio) ^ det_headset->low_active;	

	SEC_HEADSETDEV_DBG("buf = %s", buf);
	SEC_HEADSETDEV_DBG("buf size = %d", sizeof(buf));
	SEC_HEADSETDEV_DBG("buf size = %d", strlen(buf));

	if(state)
	{
		if(current_headset_type_status != SEC_UNKNOWN_DEVICE)
		{
			printk(KERN_ERR "user can't select headset device if current_headset_status isn't unknown status");
			return -1;
		}
		
		if(sizeof(buf)!=1)
		{
			printk("input error\n");
			printk("Must be stored ( 1,2,4)\n");
			return -1;		
		}

		value = strtoi(buf);
		SEC_HEADSETDEV_DBG("User  selection : 0X%x", value);
		
		switch(value)
		{
			case SEC_HEADSET_3_POLE_DEVICE:
			{
				current_headset_type_status = SEC_HEADSET_3_POLE_DEVICE;			
				switch_set_state(&switch_headset_detection, current_headset_type_status);
				headset_input_selector(current_headset_type_status);
				break;
			}
			case SEC_HEADSET_4_POLE_DEVICE:
			{
				enable_irq (send_end->eint);
				send_end_irq_token++;			
				current_headset_type_status = SEC_HEADSET_4_POLE_DEVICE;
				switch_set_state(&switch_headset_detection, current_headset_type_status);
				headset_input_selector(current_headset_type_status);
				break;
			}
			case SEC_TVOUT_DEVICE:
			{
				current_headset_type_status = SEC_TVOUT_DEVICE;
				gpio_set_value(GPIO_MICBIAS_EN, 0);
				switch_set_state(&switch_headset_detection, current_headset_type_status);
				headset_input_selector(current_headset_type_status);
				break;
			}			
		}
	}
	else
	{
		printk(KERN_ALERT "Error : mic bias enable complete but headset detached!!\n");
		current_headset_type_status = SEC_HEADSET_NO_DEVICE;
		gpio_set_value(GPIO_MICBIAS_EN, 0);
	}

	return size;
}
static DEVICE_ATTR(select_headset, S_IRUGO | S_IWUSR | S_IWOTH | S_IXOTH, select_headset_show, select_headset_store);

static int sec_headset_probe(struct platform_device *pdev)
{
	int ret;
	struct sec_headset_platform_data *pdata = pdev->dev.platform_data;
	struct sec_gpio_info   *det_headset;
	struct sec_gpio_info   *send_end;
	struct input_dev	   *input;
	current_headset_type_status = SEC_HEADSET_NO_DEVICE;
	
	printk(KERN_INFO "SEC HEADSET: Registering headset driver\n");
	
	hi = kzalloc(sizeof(struct sec_headset_info), GFP_KERNEL);
	if (!hi)
		return -ENOMEM;

	memcpy (&hi->port, pdata->port, sizeof(struct sec_headset_port));

	input = hi->input = input_allocate_device();
	if (!input)
	{
		ret = -ENOMEM;
		printk(KERN_ERR "SEC HEADSET: Failed to allocate input device.\n");
		goto err_request_input_dev;
	}

	input->name = "sec_headset";
	set_bit(EV_SYN, input->evbit);
	set_bit(EV_KEY, input->evbit);
	set_bit(KEYCODE_SENDEND, input->keybit);

	ret = input_register_device(input);
	if (ret < 0)
	{
		printk(KERN_ERR "SEC HEADSET: Failed to register driver\n");
		goto err_register_input_dev;
	}
	
	init_timer(&headset_detect_timer);
	headset_detect_timer.function = headset_detect_timer_handler;

	init_timer(&send_end_key_event_timer);
	send_end_key_event_timer.function = send_end_key_event_timer_handler;

	SEC_HEADSETDEV_DBG("registering switch_sendend switch_dev sysfs sec_headset");
	
	ret = switch_dev_register(&switch_headset_detection);
	if (ret < 0) 
	{
		printk(KERN_ERR "SEC HEADSET: Failed to register switch device\n");
		goto err_switch_dev_register;
	}

	ret = switch_dev_register(&switch_sendend);
	if (ret < 0)
	{
		printk(KERN_ERR "SEC HEADSET: Failed to register switch sendend device\n");
		goto err_switch_dev_register;
	}

	//Create HEADSET Device file in Sysfs
	headset_class = class_create(THIS_MODULE, "headset");
	if(IS_ERR(headset_class))
	{
		printk(KERN_ERR "Failed to create class(sec_headset)\n");
	}

	headset_selector_fs = device_create(headset_class, NULL, 0, NULL, "headset_selector");
	if (IS_ERR(headset_selector_fs))
		printk(KERN_ERR "Failed to create device(sec_headset)!= %ld\n", IS_ERR(headset_selector_fs));

	if (device_create_file(headset_selector_fs, &dev_attr_select_headset) < 0)
		printk(KERN_ERR "Failed to create device file(%s)!\n", dev_attr_select_headset.attr.name);	

	//GPIO configuration
	send_end = &hi->port.send_end;
	s3c_gpio_cfgpin(send_end->gpio, S3C_GPIO_SFN(send_end->gpio_af));
	s3c_gpio_setpull(send_end->gpio, S3C_GPIO_PULL_NONE);
	set_irq_type(send_end->eint, IRQ_TYPE_EDGE_BOTH);

	ret = request_irq(send_end->eint, send_end_irq_handler, IRQF_DISABLED, "sec_headset_send_end", NULL);

	SEC_HEADSETDEV_DBG("sended isr send=0X%x, ret =%d", send_end->eint, ret);
	if (ret < 0)
	{
		printk(KERN_ERR "SEC HEADSET: Failed to register send/end interrupt.\n");
		goto err_request_send_end_irq;
	}

	disable_irq(send_end->eint);

	det_headset = &hi->port.det_headset;
	s3c_gpio_cfgpin(det_headset->gpio, S3C_GPIO_SFN(det_headset->gpio_af));
	s3c_gpio_setpull(det_headset->gpio, S3C_GPIO_PULL_NONE);
	set_irq_type(det_headset->eint, IRQ_TYPE_EDGE_BOTH);
	
	if(apollo_get_remapped_hw_rev_pin() >= 3)
		det_headset->low_active = 1;

	ret = request_irq(det_headset->eint, detect_irq_handler, IRQF_DISABLED, "sec_headset_detect", NULL);

	SEC_HEADSETDEV_DBG("det isr det=0X%x, ret =%d", det_headset->eint, ret);
	if (ret < 0) 
	{
		printk(KERN_ERR "SEC HEADSET: Failed to register detect interrupt.\n");
		goto err_request_detect_irq;
	}

#if 0
	// EAR_SEL
	// EAR_ADC_SEL1,EAR_ADC_SEL2
	// L,H = EAR_ADC(default)
	// H,L = EAR_MIC+
	// H,H = TV_OUT
	if(gpio_is_valid(GPIO_EAR_ADC_SEL1))
	{
		if(gpio_request(GPIO_EAR_ADC_SEL1, "GPJ3_6"))
			printk(KERN_ERR "Failed to request GPIO_EAR35_SW! \n");
		gpio_direction_output(GPIO_EAR_ADC_SEL1, GPIO_LEVEL_LOW);			
	}

	if(gpio_is_valid(GPIO_EAR_ADC_SEL2))
	{
		if(gpio_request(GPIO_EAR_ADC_SEL2,   "GPJ3_7"))
			printk(KERN_ERR "Failed to request GPIO_EAR_SEL!\n");
		gpio_direction_output(GPIO_EAR_ADC_SEL2,GPIO_LEVEL_HIGH);
	}

	// ADC_EN :  ADC_CONNECTED(HIGH), ADC_DISCONNECTED(LOW)
	if(gpio_is_valid(GPIO_ADC_EN))
	{
		if(gpio_request(GPIO_ADC_EN,   "GPJ1_4"))
			printk(KERN_ERR "Failed to request GPIO_EAR_SEL!\n");
		gpio_direction_output(GPIO_ADC_EN,GPIO_LEVEL_LOW);
	}
#endif
	// EAR_SEL : LOW(3.5pi), HIGH(uUSB)
	if(gpio_is_valid(GPIO_EAR_SEL))
	{
		if(gpio_request(GPIO_EAR_SEL,   "GPJ3[2]"))
			printk(KERN_ERR "Failed to request GPIO_EAR_SEL!\n");
		gpio_direction_output(GPIO_EAR_SEL,GPIO_LEVEL_LOW);
	}


	wake_lock_init(&headset_sendend_wake_lock, WAKE_LOCK_SUSPEND, "sec_headset");

	schedule_work(&headset_detect_work);
	
	return 0;

err_request_send_end_irq:
	free_irq(det_headset->eint, 0);
err_request_detect_irq:
	switch_dev_unregister(&switch_headset_detection);
err_switch_dev_register:
	input_unregister_device(input);
err_register_input_dev:
	input_free_device(input);
err_request_input_dev:
	kfree (hi);

	return ret;	
}

static int sec_headset_remove(struct platform_device *pdev)
{
	SEC_HEADSETDEV_DBG("");
	input_unregister_device(hi->input);
	free_irq(hi->port.det_headset.eint, 0);
	free_irq(hi->port.send_end.eint, 0);
	switch_dev_unregister(&switch_headset_detection);
	return 0;
}

#ifdef CONFIG_PM
static int sec_headset_suspend(struct platform_device *pdev, pm_message_t state)
{
//	SEC_HEADSETDEV_DBG("");
	flush_scheduled_work();
	return 0;
}
static int sec_headset_resume(struct platform_device *pdev)
{
	SEC_HEADSETDEV_DBG("");
	schedule_work(&headset_detect_work);
	schedule_work(&sendend_switch_work);
	return 0;
}
#else
#define s3c_headset_resume	NULL
#define s3c_headset_suspend	NULL
#endif

static int __init sec_headset_init(void)
{
	SEC_HEADSETDEV_DBG("");
	return platform_driver_register(&sec_headset_driver);
}

static void __exit sec_headset_exit(void)
{
	platform_driver_unregister(&sec_headset_driver);
}

static struct platform_driver sec_headset_driver = {
	.probe		= sec_headset_probe,
	.remove		= sec_headset_remove,
	.suspend	= sec_headset_suspend,
	.resume		= sec_headset_resume,
	.driver		= {
		.name		= "sec_headset",
		.owner		= THIS_MODULE,
	},
};

module_init(sec_headset_init);
module_exit(sec_headset_exit);

MODULE_AUTHOR("Uk Kim <w0806.kim@samsung.com>");
MODULE_DESCRIPTION("SEC HEADSET detection driver");
MODULE_LICENSE("GPL");

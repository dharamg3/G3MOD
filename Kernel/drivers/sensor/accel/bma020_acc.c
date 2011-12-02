#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/wakelock.h>
#include <linux/input.h>

#include "bma020_acc.h"

// this proc file system's path is "/proc/driver/bma020"
// usage :	(at the path) type "cat bma020" , it will show short information for current accelation
// 			use it for simple working test only
#define	ACC_ENABLED 1
/* create bma020 object */
bma020_t bma020;

/* create bma020 registers object */
bma020regs_t bma020regs;
//#define BMA020_PROC_FS

#ifdef BMA020_PROC_FS

static void bma_acc_enable(void);
static void bma_acc_disable(void);

#include <linux/proc_fs.h>

#define DRIVER_PROC_ENTRY		"driver/bma020"
#if 0
static int bma020_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{

	bma020acc_t acc;
	bma020_set_mode( BMA020_MODE_NORMAL );
	bma020_read_accel_xyz(&acc);
	p += sprintf(p,"[BMA020]\nX axis: %d\nY axis: %d\nZ axis: %d\n" , acc.x, acc.y, acc.z);
	len = (p - page) - off;
	if (len < 0) {
		len = 0;
	}

	*eof = (len <= count) ? 1 : 0;
	*start = page + off;
	return len;
}
#else

static int bma020_proc_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	char *p = page;
	int len = 0;
	
	mutex_lock(&bma020.power_lock);
	printk("bma222_proc_read\n");
	bma020_set_mode( BMA020_MODE_NORMAL );

	bma020.state |= ACC_ENABLED;
	bma_acc_enable();

	mutex_unlock(&bma020.power_lock);

	len = (p - page) - off;
	if (len < 0) {
		len = 0;
	}

	printk("bma_proc_read: success full\n");

	*eof = (len <= count) ? 1 : 0;
	*start = page + off;
	return len;
}
#endif

#endif	//BMA020_PROC_FS

/* add by inter.park */
//extern void enable_acc_pins(void);

struct class *acc_class;

/* no use */
//static int bma020_irq_num = NO_IRQ;


#if 0
static irqreturn_t bma020_acc_isr( int irq, void *unused, struct pt_regs *regs )
{
	printk( "bma020_acc_isr event occur!!!\n" );
	
	return IRQ_HANDLED;
}
#endif


int bma020_open (struct inode *inode, struct file *filp)
{
	gprintk("start\n");
	return 0;
}

ssize_t bma020_read(struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	return 0;
}

ssize_t bma020_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	return 0;
}

int bma020_release (struct inode *inode, struct file *filp)
{
	return 0;
}

#if 0
int bma020_ioctl(struct inode *inode, struct file *filp, unsigned int ioctl_num,  unsigned long arg)
{
	bma020acc_t accels;
	unsigned int arg_data; 
	int err = 0;
	
	gprintk("start\n");
	switch( ioctl_num )
	{
		case IOCTL_BMA020_GET_ACC_VALUE :
			{
				bma020_read_accel_xyz( &accels );

				gprintk( "acc data x = %d  /  y =  %d  /  z = %d\n", accels.x, accels.y, accels.z );
				
				if( copy_to_user( (bma020acc_t*)arg, &accels, sizeof(bma020acc_t) ) )
				{
					err = -EFAULT;
				}   

			}
			break;
		
		case IOC_SET_ACCELEROMETER :  
			{
				if( copy_from_user( (unsigned int*)&arg_data, (unsigned int*)arg, sizeof(unsigned int) ) )
				{
				
				}
				if( arg_data == BMA020_POWER_ON )
				{
					printk( "ioctl : bma020 power on\n" );
					bma020_set_mode( BMA020_MODE_NORMAL );
				}
				else
				{
					printk( "ioctl : bma020 power off\n" );
					bma020_set_mode( BMA020_MODE_SLEEP );
				}
			}
			break;
		default : 
			break;
	}
	return err;
	
}
#endif


int bma020_ioctl(struct inode *inode, struct file *filp, unsigned int cmd,  unsigned long arg)
{
	int err = 0;
	unsigned char data[6];

	/* check cmd */
	if(_IOC_TYPE(cmd) != BMA150_IOC_MAGIC)
	{
#if DEBUG       
		printk("cmd magic type error\n");
#endif
		return -ENOTTY;
	}
	if(_IOC_NR(cmd) > BMA150_IOC_MAXNR)
	{
#if DEBUG
		printk("cmd number error\n");
#endif
		return -ENOTTY;
	}

	if(_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE,(void __user*)arg, _IOC_SIZE(cmd));
	else if(_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void __user*)arg, _IOC_SIZE(cmd));
	if(err)
	{
#if DEBUG
		printk("cmd access_ok error\n");
#endif
		return -EFAULT;
	}
	#if 0
	/* check bam150_client */
	if( bma150_client == NULL)
	{
#if DEBUG
		printk("I2C driver not install\n");
#endif
		return -EFAULT;
	}
	#endif

	switch(cmd)
	{
		case BMA150_READ_ACCEL_XYZ:
			err = bma020_read_accel_xyz((bma020acc_t*)data);
			if(copy_to_user((bma020acc_t*)arg,(bma020acc_t*)data,6)!=0)
			{
#if DEBUG
				printk("copy_to error\n");
#endif
				err = -EFAULT;
				break;
			}
			break;
		case BMA150_SET_RANGE:
			if(copy_from_user(data,(unsigned char*)arg,1)!=0)
			{
#if DEBUG           
				printk("[BMA150] copy_from_user error\n");
#endif
				err = -EFAULT;
				break;
			}
			err = bma020_set_range(*data);
			break;
		
		case BMA150_SET_MODE:
			if(copy_from_user(data,(unsigned char*)arg,1)!=0)
			{
#if DEBUG           
				printk("[BMA150] copy_from_user error\n");
#endif
				err = -EFAULT;
				break;
			}
			err = bma020_set_mode(*data);
			break;

		case BMA150_SET_BANDWIDTH:
			if(copy_from_user(data,(unsigned char*)arg,1)!=0)
			{
#if DEBUG
				printk("[BMA150] copy_from_user error\n");
#endif
				err = -EFAULT;
				break;
			}
			err = bma020_set_bandwidth(*data);
			break;
		
		default:
			break;
	}
	return err;
}

struct file_operations acc_fops =
{
	.owner   = THIS_MODULE,
	.read    = bma020_read,
	.write   = bma020_write,
	.open    = bma020_open,
	.ioctl   = bma020_ioctl,
	.release = bma020_release,
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static void bma020_early_suspend(struct early_suspend *handler)
{
	printk( "%s : Set MODE SLEEP\n", __func__ );
	bma020_set_mode( BMA020_MODE_SLEEP );
}

static void bma020_late_resume(struct early_suspend *handler)
{
	printk( "%s : Set MODE NORMAL\n", __func__ );
	bma020_set_mode( BMA020_MODE_NORMAL );
}
#endif /* CONFIG_HAS_EARLYSUSPEND */ 

void bma020_chip_init(void)
{
	/*assign register memory to bma020 object */
	bma020.image = &bma020regs;

	bma020.bma020_bus_write = i2c_acc_bma020_write;
	bma020.bma020_bus_read  = i2c_acc_bma020_read;

#ifdef CONFIG_HAS_EARLYSUSPEND
	bma020.early_suspend.suspend = bma020_early_suspend;
	bma020.early_suspend.resume = bma020_late_resume;
	register_early_suspend(&bma020.early_suspend);
#endif

	/*call init function to set read write functions, read registers */
	bma020_init( &bma020 );

	/* from this point everything is prepared for sensor communication */


	/* set range to 2G mode, other constants: 
	 * 	   			4G: BMA020_RANGE_4G, 
	 * 	    		8G: BMA020_RANGE_8G */

	bma020_set_range(BMA020_RANGE_2G); 

	/* set bandwidth to 25 HZ */
	bma020_set_bandwidth(BMA020_BW_25HZ);

	/* for interrupt setting */
//	bma020_set_low_g_threshold( BMA020_HG_THRES_IN_G(0.35, 2) );

//	bma020_set_interrupt_mask( BMA020_INT_LG );

}

/*For testing, sysfs -showing x,y,z values*/
/////////////////////////////////////////////////////////////////////////////////////
static void bma_acc_enable(void)
{
	printk("starting poll timer, delay %lldns\n", ktime_to_ns(bma020.acc_poll_delay));
	hrtimer_start(&bma020.timer, bma020.acc_poll_delay, HRTIMER_MODE_REL);
}

static void bma_acc_disable(void)
{
	printk("cancelling poll timer\n");
	hrtimer_cancel(&bma020.timer);
	cancel_work_sync(&bma020.work_acc);
}

static ssize_t poll_delay_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%lld\n", ktime_to_ns(bma020.acc_poll_delay));
}


static ssize_t poll_delay_store(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
	int64_t new_delay;
	int err;

	err = strict_strtoll(buf, 10, &new_delay);
	if (err < 0)
		return err;

	printk("new delay = %lldns, old delay = %lldns\n",
		    new_delay, ktime_to_ns(bma020.acc_poll_delay));
	mutex_lock(&bma020.power_lock);
	if (new_delay != ktime_to_ns(bma020.acc_poll_delay)) {
		bma_acc_disable();
		bma020.acc_poll_delay = ns_to_ktime(new_delay);
		if (bma020.state & ACC_ENABLED) {
			bma_acc_enable();
		}
	}
	mutex_unlock(&bma020.power_lock);

	return size;
}

static ssize_t acc_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", (bma020.state & ACC_ENABLED) ? 1 : 0);
}


static ssize_t acc_enable_store(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
	bool new_value;

	if (sysfs_streq(buf, "1"))
		new_value = true;
	else if (sysfs_streq(buf, "0"))
		new_value = false;
	else {
		pr_err("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	mutex_lock(&bma020.power_lock);
	printk("new_value = %d, old state = %d\n", new_value, (bma020.state & ACC_ENABLED) ? 1 : 0);
	if (new_value && !(bma020.state & ACC_ENABLED)) {
		bma020.state |= ACC_ENABLED;
		bma_acc_enable();
	} else if (!new_value && (bma020.state & ACC_ENABLED)) {
		bma_acc_disable();
		bma020.state = 0;
	}
	mutex_unlock(&bma020.power_lock);
	return size;
}

static DEVICE_ATTR(poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
		   poll_delay_show, poll_delay_store);

static struct device_attribute dev_attr_acc_enable =
	__ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
	       acc_enable_show, acc_enable_store);

static struct attribute *acc_sysfs_attrs[] = {
	&dev_attr_acc_enable.attr,
	&dev_attr_poll_delay.attr,
	NULL
};

static struct attribute_group acc_attribute_group = {
	.attrs = acc_sysfs_attrs,
};
///////////////////////////////////////////////////////////////////////////////////

static void bma_work_func_acc(struct work_struct *work)
{
	bma020acc_t acc;
	int err;
		
	err = bma020_read_accel_xyz(&acc);
	
	input_report_abs(bma020.acc_input_dev, ABS_X, acc.x);
	input_report_abs(bma020.acc_input_dev, ABS_Y, acc.y);
	input_report_abs(bma020.acc_input_dev, ABS_Z, acc.z);
	input_sync(bma020.acc_input_dev);
}

/* This function is for light sensor.  It operates every a few seconds.
 * It asks for work to be done on a thread because i2c needs a thread
 * context (slow and blocking) and then reschedules the timer to run again.
 */
static enum hrtimer_restart bma_timer_func(struct hrtimer *timer)
{
	queue_work(bma020.wq, &bma020.work_acc);
	hrtimer_forward_now(&bma020.timer, bma020.acc_poll_delay);
	return HRTIMER_RESTART;
}
static ssize_t bma020_show_accel_value(struct device *dev,struct device_attribute *attr, char *buf)
{   
	bma020acc_t accel; 
	
	bma020_set_mode(BMA020_MODE_NORMAL);
	bma020_set_range(BMA020_RANGE_2G); 
	bma020_set_bandwidth(BMA020_BW_100HZ);
	
	bma020_read_accel_xyz( &accel);	
    return sprintf(buf, "%d,%d,%d\n", accel.x,accel.y,accel.z);
}
static DEVICE_ATTR(acc_file, S_IRUGO, bma020_show_accel_value, NULL);

int bma020_acc_start(void)
{
	int result,err;
	struct input_dev *input_dev;

	struct device *dev_t;
	
	bma020acc_t accels; 
	
	result = register_chrdev( ACC_DEV_MAJOR, ACC_DEV_NAME, &acc_fops);

	if (result < 0) 
	{
		return result;
	}
	
	acc_class = class_create (THIS_MODULE, ACC_DEV_NAME);
	
	if (IS_ERR(acc_class)) 
	{
		unregister_chrdev( ACC_DEV_MAJOR, ACC_DEV_NAME);
		return PTR_ERR( acc_class );
	}

	dev_t = device_create( acc_class, NULL, MKDEV(ACC_DEV_MAJOR, 0), "%s", ACC_DEV_NAME);

	if (IS_ERR(dev_t)) 
	{
		return PTR_ERR(dev_t);
	}
	
	/*For testing*/
	if (device_create_file(dev_t, &dev_attr_acc_file) < 0)
		printk("Failed to create device file %s \n", dev_attr_acc_file.attr.name);
	
	mutex_init(&bma020.power_lock);

	/* hrtimer settings.  we poll for light values using a timer. */
	hrtimer_init(&bma020.timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	bma020.acc_poll_delay = ns_to_ktime(240 * NSEC_PER_MSEC);
	bma020.timer.function = bma_timer_func;

	/* the timer just fires off a work queue request.  we need a thread
	   to read the i2c (can be slow and blocking). */
	bma020.wq = create_singlethread_workqueue("bma_wq");
	if (!bma020.wq) {
		err = -ENOMEM;
		printk("%s: could not create workqueue\n", __func__);
		goto err_create_workqueue;
	}
	/* this is the thread function we run on the work queue */
	INIT_WORK(&bma020.work_acc, bma_work_func_acc);

	/* allocate lightsensor-level input_device */
	input_dev = input_allocate_device();
	if (!input_dev) {
		printk("%s: could not allocate input device\n", __func__);
		err = -ENOMEM;
		goto err_input_allocate_device_light;
	}
	input_set_drvdata(input_dev, &bma020);
	input_dev->name = "accel";


	set_bit(EV_ABS, input_dev->evbit);	
	/* acceleration x-axis */
	input_set_capability(input_dev, EV_ABS, ABS_X);
	input_set_abs_params(input_dev, ABS_X, -1024, 1024, 0, 0);
	/* acceleration y-axis */
	input_set_capability(input_dev, EV_ABS, ABS_Y);
	input_set_abs_params(input_dev, ABS_Y, -1024, 1024, 0, 0);
	/* acceleration z-axis */
	input_set_capability(input_dev, EV_ABS, ABS_Z);
	input_set_abs_params(input_dev, ABS_Z, -1024, 1024, 0, 0);

	printk("registering lightsensor-level input device\n");
	err = input_register_device(input_dev);
	if (err < 0) {
		printk("%s: could not register input device\n", __func__);
		input_free_device(input_dev);
		goto err_input_register_device_light;
	}
	bma020.acc_input_dev = input_dev;


	err = sysfs_create_group(&input_dev->dev.kobj,&acc_attribute_group);
	if (err) {
		printk("Creating bma020 attribute group failed");
		goto error_device;
	}
//////////////////////////////////////////////////////////////////////////////
	
	result = i2c_acc_bma020_init();

	if(result)
	{
		return result;
	}

	bma020_chip_init();

	gprintk("[BMA020] read_xyz ==========================\n");
	bma020_read_accel_xyz( &accels );
	gprintk("[BMA020] x = %d  /  y =  %d  /  z = %d\n", accels.x, accels.y, accels.z );

	gprintk("[BMA020] ===================================\n");	

#ifdef BMA020_PROC_FS
	create_proc_read_entry(DRIVER_PROC_ENTRY, 0, 0, bma020_proc_read, NULL);
#endif	//BMA020_PROC_FS

	bma020_set_mode(BMA020_MODE_SLEEP);
	gprintk("[BMA020] set_mode BMA020_MODE_SLEEP\n");
	
	return 0;
error_device:
	sysfs_remove_group(&input_dev->dev.kobj, &acc_attribute_group);
err_input_register_device_light:
	input_unregister_device(bma020.acc_input_dev);
err_input_allocate_device_light:	
	destroy_workqueue(bma020.wq);
err_create_workqueue:
	mutex_destroy(&bma020.power_lock);
exit:
	return err;
}

void bma020_acc_end(void)
{
	unregister_chrdev( ACC_DEV_MAJOR, ACC_DEV_NAME);	
	i2c_acc_bma020_exit();
	device_destroy( acc_class, MKDEV(ACC_DEV_MAJOR, 0) );
	class_destroy( acc_class );
	unregister_early_suspend(&bma020.early_suspend);
}


static int bma020_accelerometer_probe( struct platform_device* pdev )
{
/* not use interrupt */
#if 0	
	int ret;

	//enable_acc_pins();
	/*
	mhn_gpio_set_direction(MFP_ACC_INT, GPIO_DIR_IN);
	mhn_mfp_set_pull(MFP_ACC_INT, MFP_PULL_HIGH);
	*/

	bma020_irq_num = platform_get_irq(pdev, 0);
	ret = request_irq(bma020_irq_num, (void *)bma020_acc_isr, IRQF_DISABLED, pdev->name, NULL);
	if(ret) {
		printk("[BMA020 ACC] isr register error\n");
		return ret;
	}

	//set_irq_type (bma020_irq_num, IRQT_BOTHEDGE);
	
	/* if( request_irq( IRQ_GPIO( MFP2GPIO(MFP_ACC_INT) ), (void *) bma020_acc_isr, 0, "BMA020_ACC_ISR", (void *)0 ) )
	if(
	{
		printk ("[BMA020 ACC] isr register error\n" );
	}
	else
	{
		printk( "[BMA020 ACC] isr register success!!!\n" );
	}*/
	
	// set_irq_type ( IRQ_GPIO( MFP2GPIO(MFP_ACC_INT) ), IRQT_BOTHEDGE );

	/* if interrupt don't register Process don't stop for polling mode */ 

#endif 
	return bma020_acc_start();
}


static int bma020_accelerometer_suspend( struct platform_device* pdev, pm_message_t state )
{
	printk(" %s \n",__func__); 
	bma020_set_mode( BMA020_MODE_SLEEP );

	if (bma020.state & ACC_ENABLED) 
		bma_acc_disable();
	return 0;
}


static int bma020_accelerometer_resume( struct platform_device* pdev )
{
	printk(" %s \n",__func__); 

	if (bma020.state & ACC_ENABLED)
		bma_acc_enable();
	
	bma020_set_mode( BMA020_MODE_NORMAL );
	return 0;
}


static struct platform_device *bma020_accelerometer_device;

static struct platform_driver bma020_accelerometer_driver = {
	.probe 	 = bma020_accelerometer_probe,
	.suspend = bma020_accelerometer_suspend,
	.resume  = bma020_accelerometer_resume,
	.driver  = {
		.name = "bma020-accelerometer", 
	}
};


static int __init bma020_acc_init(void)
{
	int result;

	result = platform_driver_register( &bma020_accelerometer_driver );

	if( result ) 
	{
		return result;
	}

	bma020_accelerometer_device  = platform_device_register_simple( "bma020-accelerometer", -1, NULL, 0 );
	
	if( IS_ERR( bma020_accelerometer_device ) )
	{
		return PTR_ERR( bma020_accelerometer_device );
	}

	return 0;
}


static void __exit bma020_acc_exit(void)
{
	gprintk("start\n");
	bma020_acc_end();

//	free_irq(bma020_irq_num, NULL);
//	free_irq( IRQ_GPIO( MFP2GPIO( MFP_ACC_INT ) ), (void*)0 );

	platform_device_unregister( bma020_accelerometer_device );
	platform_driver_unregister( &bma020_accelerometer_driver );
}


module_init( bma020_acc_init );
module_exit( bma020_acc_exit );

MODULE_AUTHOR("inter.park");
MODULE_DESCRIPTION("accelerometer driver for BMA020");
MODULE_LICENSE("GPL");

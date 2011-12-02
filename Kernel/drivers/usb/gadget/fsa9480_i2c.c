#include <linux/interrupt.h>
#include <linux/irq.h>

#include <plat/pm.h>
#include <plat/s5p6442-dvfs.h>
#include <linux/i2c/pmic.h>
#include <linux/switch.h>
  
#include <mach/param.h>
#include <mach/hardware.h>
#include "fsa9480_i2c.h"

#include <linux/wakelock.h>


static struct wake_lock fsa9480_wake_lock;

extern void max8998_safeout_enable(unsigned char mask);
extern void set_dvfs_perf_level(void);

extern struct device *switch_dev;
//extern unsigned char ftm_sleep;
extern int pmic_init;
static int g_tethering;
static int g_dock;


int oldusbstatus=0;
#define FSA9480UCX		0x4A
#define DRIVER_NAME  "usb_mass_storage"

struct i2c_driver fsa9480_i2c_driver;
static struct i2c_client *fsa9480_i2c_client = NULL;

struct fsa9480_state {
	struct i2c_client *client;
};

static struct i2c_device_id fsa9480_id[] = {
	{"fsa9480", 0},
	{}
};

static int fsa9480_init = 0;
static int pm_check_finished = 0;

static int usb_path = 0;
static int usb_power = 2;
static int usb_state = 0;
static char bFactoryReset = 0;

int samsung_kies_mtp_mode_flag;
int usb_menu_path = 0;
int old_usb_menu_path = 0;
int usb_connected = 0;
int mtp_mode_on = 0;
int inaskonstatus = 0;
EXPORT_SYMBOL(usb_connected);

static struct timer_list fsa9480_init_timer;
static struct timer_list param_check_timer;
static wait_queue_head_t usb_detect_waitq;
static struct workqueue_struct *fsa9480_workqueue;
static struct work_struct fsa9480_work;
static struct work_struct fsa9480_irq_work;  // Added by iks.kim
struct switch_dev indicator_dev;

/* MODEM USB_SEL Pin control */
/* 1 : PDA, 2 : MODEM */
#define SWITCH_PDA		1
#define SWITCH_MODEM		2


/********************************************************************/
/* function definitions                                                                             */
/********************************************************************/

//by ss1
//refer to drivers/usb/gadget/s3c-udc-otg-hs.c	
void s3c_udc_power_up(void);
void s3c_udc_power_down(void);
void fsa9480_SetAutoSWMode(void);
void pm_check(void);
static void usb_sel(int sel);
int fsa9480_read(struct i2c_client *client, u8 reg, u8 *data);

int wait_condition = 0;
int ask_on_kies=0;
static bool ta_connection = false;

extern int usb_switch_select(int enable);
extern void 	cable_status_changed();
extern void s3c_cable_work();

void UsbIndicator(u8 state)
{
	switch_set_state(&indicator_dev, state);
}

void get_usb_serial(char *usb_serial_number)
{
	char temp_serial_number[13] = {0};

	u32 serial_number=0;
	
	serial_number = (system_serial_high << 16) + (system_serial_low >> 16);

	sprintf(temp_serial_number,"6442%08x",serial_number);
	strcpy(usb_serial_number,temp_serial_number);
}


int available_PM_Set(void)
{
//	DEBUG_FSA9480("[FSA9480] %s\n", __func__);
	if(driver_find("MAX8998", &i2c_bus_type) && pmic_init)
		return 1;
	return 0;
}


int get_usb_power_state(void)
{
	struct i2c_client *client = fsa9480_i2c_client;
	u8 deviceType1, deviceType2;

	DEBUG_FSA9480("[FSA9480] %s\n ", __func__);

	if(usb_power !=1 )
	{
		wait_condition = 0;
		wait_event_interruptible_timeout(usb_detect_waitq, wait_condition , 2 * HZ);
		wake_lock_timeout(&fsa9480_wake_lock, 3*HZ);

	}

	if (usb_power == 2) 
	{
		while(!pm_check_finished)
		{
			wait_event_interruptible_timeout(usb_detect_waitq, wait_condition , 2 * HZ); 
			wake_lock_timeout(&fsa9480_wake_lock, 3*HZ);
		}

		fsa9480_read(client, REGISTER_DEVICETYPE1, &deviceType1);
		fsa9480_read(client, REGISTER_DEVICETYPE2, &deviceType2);

		if (deviceType1 ==0x40)
			return 0;
		else if (deviceType1 == CRA_USB|| (deviceType2 & CRB_JIG_USB))
			return 1;
	}

	if(usb_power==2 && ta_connection)
	{
		printk("[FSA9480] usb_power = 2 & taconnection is true \n");
		return 0;
	}

	return usb_connected;
}


int get_usb_cable_state(void)
{
//	DEBUG_FSA9480("[FSA9480] %s - usb_state(0x%2x)\n ", __func__, usb_state);
	return usb_state;
}
EXPORT_SYMBOL(get_usb_cable_state);


int get_jig_cable_state(void)
{
	if( (usb_state>>8) & (CRB_JIG_USB_ON |CRB_JIG_USB_OFF | CRB_JIG_UART_ON | CRB_JIG_UART_OFF) )
		return 1;
	else
		return 0;
}
EXPORT_SYMBOL(get_jig_cable_state);


void fsa9480_s3c_udc_on(void)
{	
	//	DEBUG_FSA9480("[FSA9480] %s\n ", __func__);
	usb_power = 1;

	//SEC_BSP_WONSUK_20090806 
	//resolve Power ON/OFF panic issue. do not wake_up function before FSA9480 probe.
	if(&usb_detect_waitq == NULL)
	{
		printk("[FSA9480] fsa9480_s3c_udc_on : usb_detect_waitq is NULL\n");
	}
	else
	{
		wait_condition = 1;
		wake_up_interruptible(&usb_detect_waitq);
	}

	if (mtp_mode_on == 1)
		samsung_kies_mtp_mode_flag = 1;
	else
		samsung_kies_mtp_mode_flag = 0;

	/* LDO control (MAX8998) */
	if(!Set_MAX8998_PM_REG(ELDO3, 1) || !Set_MAX8998_PM_REG(ELDO8, 1))
		printk("[FSA9480]%s : Fail to LDO ON\n ", __func__);

}


void fsa9480_s3c_udc_off(void)
{
//	DEBUG_FSA9480("[FSA9480] %s\n ", __func__);

	usb_power = 0;

	/* LDO control (MAX8998) */
	if(!Set_MAX8998_PM_REG(ELDO3, 0) || !Set_MAX8998_PM_REG(ELDO8, 0))
		printk("[FSA9480] %s : Fail to LDO OFF\n ", __func__);

}


int fsa9480_read(struct i2c_client *client, u8 reg, u8 *data)
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret < 0)
		return -EIO;

	*data = ret & 0xff;
	return 0;
}

static int fsa9480_write(struct i2c_client *client, u8 reg, u8 data)
{
	return i2c_smbus_write_byte_data(client, reg, data);
}

void InvokeGADConsole(void)
{
	printk("GAD>register_uart(\":s3c_serial_driver_gad\")");
	printk("\n");
	printk("GAD>register_fb(\"fb_draw_rect\")");
	printk("\n");
	printk("GAD>console()");
	printk("\n");
}

unsigned int IsVolumeUpPressed(void)
{
	unsigned int VolUpKeyPressed = 0;		// 0: non-pressed	1: pressed

	VolUpKeyPressed = !gpio_get_value(S5P64XX_GPH3(4));	// vol_up is an active low pin

	printk("%s : gpio_get_value(S5P64XX_GPH3(4)) : 0x%08x \n", __FUNCTION__, gpio_get_value(S5P64XX_GPH3(4)));

	return VolUpKeyPressed;
}

bool CheckGADCondition(void)
{
	bool bRet = FALSE;	

	if(IsVolumeUpPressed() == TRUE){
		bRet = TRUE;
	}

	return bRet;
}

void GADSupport(void)
{
	bool bRet = FALSE;
	
	bRet = CheckGADCondition();
	if(bRet){
		printk("CheckGADCondition : TRUE ,  InvokeGADConsole() \n");
//		EnablePDAUart();
		InvokeGADConsole();
	}
}

/* called by udc */
/* UART <-> USB switch (for UART/USB JIG) */
void fsa9480_check_usb_connection(void)
{
	struct i2c_client *client = fsa9480_i2c_client;
	u8 control, int1, deviceType1, deviceType2, manual1, manual2, pData, adc, carkitint1;
	bool bInitConnect = false;

	if (bFactoryReset) {
		SEC_GET_PARAM(__SWITCH_SEL, usb_menu_path);
		usb_menu_path = USBSTATUS_SAMSUNG_KIES | FSA9480_USB;
		SEC_SET_PARAM(__SWITCH_SEL, usb_menu_path);

		s3c_udc_power_down();

//		enable_irq(IRQ_FSA9480_INTB);
		return;
	}

	if ((usb_menu_path & FSA9480_USB) && (usb_power == 2)) {
		if ((usb_menu_path & ~(0x3)) == USBSTATUS_SAMSUNG_KIES)
			usb_switch_select(USBSTATUS_ACMONLY);
		else
			usb_switch_select(usb_menu_path & ~(0x3));
	}
	
//	DEBUG_FSA9480("[FSA9480] %s\n ", __func__);

	fsa9480_read(client, REGISTER_INTERRUPT1, &int1); // interrupt clear
	fsa9480_read(client, REGISTER_DEVICETYPE1, &deviceType1);
	fsa9480_read(client, REGISTER_ADC, &adc);

	//in case of carkit we should process the extra interrupt. 1: attach interrupt, 2: carkit interrupt
	//  (interrupt_cr3 & 0x0) case is when power on after TA is inserted.
	if (((int1 & ATTACH) || !(int1 & 0x0)) && 
			(( deviceType1 & CRA_CARKIT) || adc == CEA936A_TYPE_1_CHARGER ))
	{
		DEBUG_FSA9480("[FSA9480] %s : Carkit is inserted! 1'st resolve insert interrupt\n ",__func__);
		fsa9480_write(client, REGISTER_CARKITSTATUS, 0x02);  //use only carkit charger

		fsa9480_read(client, REGISTER_CARKITINT1, &carkitint1);    // make INTB to high
		DEBUG_FSA9480("[FSA9480] %s : Carkit int1 is 0x%02x\n ",__func__, carkitint1);

//		enable_irq(IRQ_FSA9480_INTB);
		return;
	}

	fsa9480_read(client, REGISTER_CONTROL, &control);
	fsa9480_read(client, REGISTER_INTERRUPT2, &pData); // interrupt clear
	fsa9480_read(client, REGISTER_DEVICETYPE2, &deviceType2);
	fsa9480_read(client, REGISTER_MANUALSW1, &manual1);
	fsa9480_read(client, REGISTER_MANUALSW2, &manual2);

	printk("[FSA9480] C:0x%02x, I1:0x%02x, D1:0x%02x, D2:0x%02x\n", 
				control, int1, deviceType1, deviceType2);
/* DEBUG */
#if 1
	DEBUG_FSA9480("[FSA9480] CONTROL is 0x%02x\n ",control);
	DEBUG_FSA9480("[FSA9480] INTERRUPT1 is 0x%02x\n ",int1);
	DEBUG_FSA9480("[FSA9480] INTERRUPT2 is 0x%02x\n ",pData);
	DEBUG_FSA9480("[FSA9480] DEVICETYPE1 is 0x%02x\n ",deviceType1);
	DEBUG_FSA9480("[FSA9480] DEVICETYPE2 is 0x%02x\n ",deviceType2);
	DEBUG_FSA9480("[FSA9480] MANUALSW1 is 0x%02x\n ",manual1);
	DEBUG_FSA9480("[FSA9480] MANUALSW2 is 0x%02x\n ",manual2);
#endif

	/* TA Connection */
	if(deviceType1 ==0x40 || deviceType1 == 0x20)
	{
		wait_condition = 1;
		ta_connection = true;
		wake_up_interruptible(&usb_detect_waitq);
	}
	
	usb_state = (deviceType2 << 8) | (deviceType1 << 0);
	
	/* Disconnect cable */
	if (deviceType1 == DEVICE_TYPE_NC && deviceType2 == DEVICE_TYPE_NC) {
		ta_connection = false;
		usb_connected = 0;
		DEBUG_FSA9480("[FSA9480] Cable is not connected\n ");

		/* reset manual2 s/w register */
		fsa9480_write(client, REGISTER_MANUALSW2, 0x00);

		/* auto mode settings */
		fsa9480_write(client, REGISTER_CONTROL, 0x1E); 

		/* power down mode */
		mtp_mode_on = 0;
		inaskonstatus = 0;
		
		if(ask_on_kies)
		{
			ask_on_kies = 0;
			usb_menu_path = old_usb_menu_path;

		}

		if (usb_power == 1) {
			if (indicator_dev.state != 1) {
				printk("indicator state : %d\n", indicator_dev.state);
				indicator_dev.state = 1;
			}
			
			s3c_udc_power_down();
		}

		UsbIndicator(0);
		s3c_cable_work();

		//return ;	// MIDAS@Remove return to turn off the udc power
	}
	
	/* USB Detected */
	if (deviceType1 == CRA_USB|| (deviceType2 & CRB_JIG_USB)) 
	{
		/* Manual Mode for USB */
		DEBUG_FSA9480("[FSA9480] MANUAL MODE - d1:0x%02x, d2:0x%02x\n", deviceType1, deviceType2);
		if (control != 0x1A) 
		{ 	
			fsa9480_write(client, REGISTER_CONTROL, 0x1A); 
		}

		/* reset manual2 s/w register */
		manual2 &= ~(0x1f);

		if(deviceType1 == CRA_USB)
		{
			/* ID Switching : ID connected to bypass port */
			manual2 |= 0x2;
//			DEBUG_FSA9480("[FSA9480] 1 :Device type is 0x%x, manual2 (0x%x)\n ", deviceType1, manual2);
		}
		else if(deviceType2 & CRB_JIG_USB_ON)
		{	
			/* BOOT_SW : High , JIG_ON : GND */
			manual2 |= (0x1 << 3) | (0x1 << 2);
//			DEBUG_FSA9480("[FSA9480] 2 :Device type is 0x%x, manual2 (0x%x)\n ", deviceType2, manual2);
		}
		else if(deviceType2 & CRB_JIG_USB_OFF)
		{
			/* BOOT_SW : LOW , JIG_ON : GND */
			manual2 |= (0x1 << 2);
//			DEBUG_FSA9480("[FSA9480] 3 :Device type is 0x%x, manual2 (0x%x)\n ", deviceType2, manual2);
		}

		fsa9480_write(client, REGISTER_MANUALSW2, manual2);

		/* connect cable */
		if(usb_path == SWITCH_PDA && usb_power !=1)
		{
//			DEBUG_FSA9480("[FSA9480] 4 :Manual1 (0x%x), usb_power (%d)\n ", manual1, usb_power);
			bInitConnect = true;
			usb_connected = 1;

			fsa9480_write(client, REGISTER_MANUALSW1, 0x24); // PDA Port

			/* USB Debugging Mode */
			if (usb_menu_path & USBSTATUS_ADB) 
			{
				s3c_udc_power_up();
				UsbIndicator(1);
			} 
			/* KIES Mode */
			else if (usb_menu_path & USBSTATUS_SAMSUNG_KIES)
			{		
				usb_switch_select(USBSTATUS_ACMONLY);

				s3c_udc_power_up();
				wake_up_interruptible(&usb_detect_waitq);
				UsbIndicator(1);
			} 
			/* Media player Mode */
			else if (usb_menu_path & USBSTATUS_MTPONLY)
			{
				mtp_mode_on = 1;

				if (usb_power == 1) 
				{
					s3c_udc_power_down();
				}
				wait_condition = 1;
				wake_up_interruptible(&usb_detect_waitq);
			} 
			/* AskOn Mode */
			else if (usb_menu_path & USBSTATUS_ASKON)
			{
				if (inaskonstatus)
				{
					if (!mtp_mode_on)
					{
						s3c_udc_power_up();
					}
					
					if(ask_on_kies)
					{
						old_usb_menu_path = usb_menu_path;
						usb_menu_path = USBSTATUS_SAMSUNG_KIES;
					}
				}
				else
				{
					if (usb_power == 1) 
					{
						s3c_udc_power_down();
					}
					wait_condition = 1;
					wake_up_interruptible(&usb_detect_waitq);

					usb_switch_select(USBSTATUS_ASKON);
				}
			}
			else if (usb_menu_path & USBSTATUS_UMS)
			{
				s3c_udc_power_up();
				UsbIndicator(1);
			}
			/* Power ON */
			else 
			{
				s3c_udc_power_up();
			}

//			UsbIndicator(1);
		}
		else if(usb_path == SWITCH_MODEM)
		{
			fsa9480_write(client, REGISTER_MANUALSW1, 0x90); // V_Audio port (Modem USB)
			usb_sel(SWITCH_MODEM);
		}
	}
	/* Auto mode settings */
	else
	{
		/* Auto Mode except usb */
		DEBUG_FSA9480("[FSA9480] AUTO MODE.. d1:0x%02x, d2:0x%02x\n", deviceType1, deviceType2);
		if (control != 0x1E) 
		{ 
			fsa9480_write(client, REGISTER_CONTROL, 0x1E);
		}

		if(int1 & ATTACH && deviceType2 & CRB_JIG_UART_ON){
			GADSupport();		//20100329 minho4.kim : for GAD(debugging tool) console mode....
		}
	}

	/* initialization driver */
	if(usb_power == 2 && bInitConnect ==false )
	{
		fsa9480_s3c_udc_off();
	}

/*	if(irq)
		enable_irq(IRQ_FSA9480_INTB);*/
}

static ssize_t factoryreset_value_store(struct device *dev, struct device_attribute *attr,	const char *buf, size_t size)
{
	DEBUG_FSA9480("[FSA9480] %s - (%s)\n ", __func__, buf);
	if(strncmp(buf, "FACTORYRESET", 12) == 0 || strncmp(buf, "factoryreset", 12) == 0) {
		bFactoryReset = 1;

		if (sec_get_param_value) {
		SEC_GET_PARAM(__SWITCH_SEL, usb_menu_path);
		usb_menu_path = USBSTATUS_SAMSUNG_KIES | FSA9480_USB;
		SEC_SET_PARAM(__SWITCH_SEL, usb_menu_path);
	}
	}

	return size;
}

static DEVICE_ATTR(FactoryResetValue, S_IRUGO |S_IWUGO | S_IRUSR | S_IWUSR, NULL, factoryreset_value_store);


/* HW PATH Control : AP USB <-> CP USB */
static void usb_sel(int sel)
{
	DEBUG_FSA9480("[FSA9480] %s - sel(%d)\n ", __func__, sel);

	// VBUS control (for Apollo)
	if (sel == SWITCH_PDA) { // PDA
		max8998_safeout_enable(ESAFEOUT1);  // USB_VBUS_AP enable
	} else { // MODEM
		max8998_safeout_enable(ESAFEOUT2);  // USB_VBUS_CP enable
	}

	usb_path = sel;
}

/* FSA9480 PATH Control : AP USB <-> CP USB */
void usb_switch_mode(int sel)
{
	struct i2c_client *client = fsa9480_i2c_client;
	if (sel == SWITCH_PDA)
	{
		DEBUG_FSA9480("[FSA9480] Path : PDA\n");
		usb_sel(SWITCH_PDA);
		fsa9480_write(client, REGISTER_MANUALSW1, 0x24); // PDA Port
	}
	else if (sel == SWITCH_MODEM) 
	{
		DEBUG_FSA9480("[FSA9480] Path : MODEM\n");
		usb_sel(SWITCH_MODEM);
		fsa9480_write(client, REGISTER_MANUALSW1, 0x90); // V_Audio port (Modem USB)
	}
	else
		DEBUG_FSA9480("[FSA9480] Invalid mode...\n");
}
EXPORT_SYMBOL(usb_switch_mode);


/* for sysfs control (/sys/class/sec/switch/usb_sel) */
static ssize_t usb_sel_show(struct device *dev, struct device_attribute *attr, char *buf)
{
//	struct i2c_client *client = fsa9480_i2c_client;
//	u8 i, pData;

	sprintf(buf, "USB Switch : %s\n", usb_path==SWITCH_PDA?"PDA":"MODEM");

//    	for(i = 0; i <= 0x14; i++)
//		fsa9480_read(client, i, &pData);

	return sprintf(buf, "%s\n", buf);
}

void usb_path_store(int sel)
{
	if (bFactoryReset)
		return;

	if (sel == USBSTATUS_ADB)
		usb_menu_path |= USBSTATUS_ADB;
	else if (sel == USBSTATUS_ADB_REMOVE)
		usb_menu_path &= ~USBSTATUS_ADB;
	else {
		usb_menu_path &= (FSA9480_UART | FSA9480_USB | USBSTATUS_ADB);
		usb_menu_path |= sel;
	}

	SEC_SET_PARAM(__SWITCH_SEL, usb_menu_path);
}

static ssize_t usb_sel_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	DEBUG_FSA9480("[FSA9480] USB PATH = %s\n ", buf);

	if(strncmp(buf, "PDA", 3) == 0 || strncmp(buf, "pda", 3) == 0) {
		usb_switch_mode(SWITCH_PDA);
		usb_menu_path |= USB_SEL_MASK;
	}

	if(strncmp(buf, "MODEM", 5) == 0 || strncmp(buf, "modem", 5) == 0) {
		usb_switch_mode(SWITCH_MODEM);
		usb_menu_path &= ~USB_SEL_MASK;
	}

	SEC_SET_PARAM(__SWITCH_SEL, usb_menu_path);

	return size;
}

static DEVICE_ATTR(usb_sel, S_IRUGO |S_IWUGO | S_IRUSR | S_IWUSR, usb_sel_show, usb_sel_store);


/**********************************************************************
*    Name         : usb_state_show()
*    Description : for sysfs control (/sys/class/sec/switch/usb_state)
*                        return usb state using fsa9480's device1 and device2 register
*                        this function is used only when NPS want to check the usb cable's state.
*    Parameter   :
*                       
*                       
*    Return        : USB cable state's string
*                        USB_STATE_CONFIGURED is returned if usb cable is connected
***********************************************************************/
static ssize_t usb_state_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int cable_state;

	cable_state = get_usb_cable_state();

	sprintf(buf, "%s\n", (cable_state & (CRB_JIG_USB<<8 | CRA_USB<<0 ))?"USB_STATE_CONFIGURED":"USB_STATE_NOTCONFIGURED");

	return sprintf(buf, "%s\n", buf);
} 


/**********************************************************************
*    Name         : usb_state_store()
*    Description : for sysfs control (/sys/class/sec/switch/usb_state)
*                        noting to do.
*    Parameter   :
*                       
*                       
*    Return        : None
*
***********************************************************************/
static ssize_t usb_state_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
//	DEBUG_FSA9480("[FSA9480] %s\n ", __func__);
	return 0;
}

/*sysfs for usb cable's state.*/
static DEVICE_ATTR(usb_state, S_IRUGO |S_IWUGO | S_IRUSR | S_IWUSR, usb_state_show, usb_state_store);


static ssize_t DMport_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{	
	return sprintf(buf, "%s[UsbMenuSel test] ready!! \n", buf);	
}

static ssize_t DMport_switch_store(struct device *dev, struct device_attribute *attr,	const char *buf, size_t size)
{	
#if 0
	int fd;
	int i;
	char dm_buf[] = "DM port test sucess\n";
	int old_fs = get_fs();
	set_fs(KERNEL_DS);
	
		if (strncmp(buf, "DMport", 5) == 0 || strncmp(buf, "dmport", 5) == 0) {		
			
				fd = sys_open("/dev/dm", O_RDWR, 0);
				if(fd < 0){
				printk("Cannot open the file");
				return fd;}
				for(i=0;i<5;i++)
				{		
				sys_write(fd,dm_buf,sizeof(dm_buf));
				mdelay(1000);
				}
		sys_close(fd);
		set_fs(old_fs);				
		}

		if ((strncmp(buf, "logusb", 6) == 0)||(log_via_usb == log_usb_enable)) {		
			log_via_usb = log_usb_active;
				printk("denis_test_prink_log_via_usb_1\n");
				mdelay(1000);
				printk(KERN_INFO"%s: 21143 10baseT link beat good.\n", "denis_test");
			set_fs(old_fs);				
			}
		return size;
#else
	if (strncmp(buf, "ENABLE", 6) == 0)
	{
		usb_switch_select(USBSTATUS_DM);
	}

	if (strncmp(buf, "DISABLE", 6) == 0)
	{
		usb_switch_select(USBSTATUS_SAMSUNG_KIES);			
	}
		
	return size;
#endif
}

static DEVICE_ATTR(DMport, S_IRUGO |S_IWUGO | S_IRUSR | S_IWUSR, DMport_switch_show, DMport_switch_store);


static ssize_t DMlog_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{	
	return sprintf(buf, "%s[DMlog test] ready!! \n", buf);	
}

static ssize_t DMlog_switch_store(struct device *dev, struct device_attribute *attr,	const char *buf, size_t size)
{
	if (strncmp(buf, "CPONLY", 6) == 0)
	{
		printk("DMlog selection : CPONLY\n");
	}

	if (strncmp(buf, "APONLY", 6) == 0)
	{
		printk("DMlog selection : APONLY\n");
	}

	if (strncmp(buf, "CPAP", 4) == 0)
	{
		printk("DMlog selection : AP+CP\n");
	}
		
	return size;
}

static DEVICE_ATTR(DMlog, S_IRUGO |S_IWUGO | S_IRUSR | S_IWUSR, DMlog_switch_show, DMlog_switch_store);


static ssize_t UsbMenuSel_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{	
		if (usb_menu_path & USBSTATUS_ADB) 
			return sprintf(buf, "%s[UsbMenuSel] ACM_ADB_UMS\n", buf);	

		else if (usb_menu_path & USBSTATUS_UMS) 
			return sprintf(buf, "%s[UsbMenuSel] UMS\n", buf);	
		
		else if (usb_menu_path & USBSTATUS_SAMSUNG_KIES) 
			return sprintf(buf, "%s[UsbMenuSel] ACM_MTP\n", buf);	
		
		else if (usb_menu_path & USBSTATUS_MTPONLY) 
			return sprintf(buf, "%s[UsbMenuSel] MTP\n", buf);	
		
		else if (usb_menu_path & USBSTATUS_ASKON) 
			return sprintf(buf, "%s[UsbMenuSel] ASK\n", buf);	
		
		else if (usb_menu_path & USBSTATUS_VTP) 
			return sprintf(buf, "%s[UsbMenuSel] VTP\n", buf);	

		return sprintf(buf, "%s[UsbMenuSel] Invaild USB mode!\n", buf);		
}

static ssize_t UsbMenuSel_switch_store(struct device *dev, struct device_attribute *attr,	const char *buf, size_t size)
{		
	
	if (strncmp(buf, "KIES", 4) == 0)
	{
		usb_path_store(USBSTATUS_SAMSUNG_KIES);
		usb_switch_select(USBSTATUS_ACMONLY);
	}

	if (strncmp(buf, "MTP", 3) == 0)
	{
		usb_path_store(USBSTATUS_MTPONLY);
		usb_switch_select(USBSTATUS_MTPONLY);
	}

	if (strncmp(buf, "UMS", 3) == 0)
	{
		usb_path_store(USBSTATUS_UMS);
		usb_switch_select(USBSTATUS_UMS);
	}

	if (strncmp(buf, "VTP", 3) == 0)
	{
		usb_path_store(USBSTATUS_VTP);
		usb_switch_select(USBSTATUS_VTP);
	}

	if (strncmp(buf, "ASKON", 5) == 0)
	{		
		usb_path_store(USBSTATUS_ASKON);
		usb_switch_select(USBSTATUS_ASKON);
		inaskonstatus = 0;
	}


	return size;
}

static DEVICE_ATTR(UsbMenuSel, S_IRUGO |S_IWUGO | S_IRUSR | S_IWUSR, UsbMenuSel_switch_show, UsbMenuSel_switch_store);

static ssize_t AskOnStatus_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if(inaskonstatus)
		return sprintf(buf, "%s\n", "Blocking");
	else
		return sprintf(buf, "%s\n", "NonBlocking");
	
}

static ssize_t AskOnStatus_switch_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	return size;
}

static DEVICE_ATTR(AskOnStatus, S_IRUGO |S_IWUGO | S_IRUSR | S_IWUSR, AskOnStatus_switch_show, AskOnStatus_switch_store);


static ssize_t AskOnMenuSel_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{	
		return sprintf(buf, "%s[AskOnMenuSel] Port test ready!! \n", buf);	
}

static ssize_t AskOnMenuSel_switch_store(struct device *dev, struct device_attribute *attr,	const char *buf, size_t size)
{		
	inaskonstatus = 1;

	if (strncmp(buf, "KIES", 4) == 0)
	{	
		ask_on_kies = 1;
		usb_switch_select(USBSTATUS_ACMONLY);
	}

	if (strncmp(buf, "MTP", 3) == 0)
	{
		mtp_mode_on = 1;
		usb_switch_select(USBSTATUS_MTPONLY);
	}

	if (strncmp(buf, "UMS", 3) == 0)
	{
		usb_switch_select(USBSTATUS_UMS);
	}

	if (strncmp(buf, "VTP", 3) == 0)
	{
		usb_switch_select(USBSTATUS_VTP);
	}

	return size;
}

static DEVICE_ATTR(AskOnMenuSel, S_IRUGO |S_IWUGO | S_IRUSR | S_IWUSR, AskOnMenuSel_switch_show, AskOnMenuSel_switch_store);


static ssize_t Mtp_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{	
		return sprintf(buf, "%s[Mtp] MtpDeviceOn \n", buf);	
}

static ssize_t Mtp_switch_store(struct device *dev, struct device_attribute *attr,	const char *buf, size_t size)
{		
	if (strncmp(buf, "Mtp", 3) == 0)
	{
		/* ACM -> KIES */
		if (usb_menu_path & USBSTATUS_SAMSUNG_KIES)
		{
			usb_switch_select(USBSTATUS_SAMSUNG_KIES);
		}

		/* In Askon Mode, mtp_mode_on : 1 -> MTP Only, mtp_mode_on : 0 -> KIES */
		if (usb_menu_path & USBSTATUS_ASKON)
		{
			if (!mtp_mode_on)	
				usb_switch_select(USBSTATUS_SAMSUNG_KIES);
		}

		/* KIES mtp_mode_on : already power on */
		if(mtp_mode_on)
		{
			printk("[Mtp_switch_store]AP USB power on. \n");
			s3c_udc_power_up();
			mtp_mode_on = 0;
		}
	} else if (strncmp(buf, "OFF", 3) == 0) {
		printk("[Mtp_switch_store] AP USB power off. \n");
		s3c_udc_power_down();
	}

	return size;
}

static DEVICE_ATTR(Mtp, S_IRUGO |S_IWUGO | S_IRUSR | S_IWUSR, Mtp_switch_show, Mtp_switch_store);


static int mtpinitstatus=0;
static ssize_t MtpInitStatusSel_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{	
	if(mtpinitstatus == 2)
		return sprintf(buf, "%s\n", "START");
	else
		return sprintf(buf, "%s\n", "STOP");

}

static ssize_t MtpInitStatusSel_switch_store(struct device *dev, struct device_attribute *attr,	const char *buf, size_t size)
{
	mtpinitstatus = mtpinitstatus + 1;

	return size;
}

static DEVICE_ATTR(MtpInitStatusSel, S_IRUGO |S_IWUGO | S_IRUSR | S_IWUSR, MtpInitStatusSel_switch_show, MtpInitStatusSel_switch_store);


static ssize_t tethering_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (g_tethering)
		return sprintf(buf, "1\n");
	else			
		return sprintf(buf, "0\n");
}

static ssize_t tethering_switch_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int usbstatus;
	usbstatus = get_jig_cable_state();
//	printk("usbstatus = 0x%x, currentusbstatus = 0x%x\n", usbstatus, currentusbstatus);

	if (strncmp(buf, "1", 1) == 0)
	{
		printk("tethering On\n");

		g_tethering = 1;
		oldusbstatus = usb_menu_path;
		usb_menu_path = USBSTATUS_VTP;
		usb_switch_select(USBSTATUS_VTP);
		UsbIndicator(0);
	}
	else if (strncmp(buf, "0", 1) == 0)
	{
		printk("tethering Off\n");

		g_tethering = 0;
		
		usb_menu_path = oldusbstatus;		
		
		if (usb_menu_path & USBSTATUS_ADB)		 		usb_switch_select(USBSTATUS_ADB);
		else if (usb_menu_path & USBSTATUS_UMS) 			usb_switch_select(USBSTATUS_UMS);	
		else if (usb_menu_path & USBSTATUS_SAMSUNG_KIES)	usb_switch_select(USBSTATUS_SAMSUNG_KIES);
		else if (usb_menu_path & USBSTATUS_ASKON) 			usb_switch_select(USBSTATUS_ASKON);
		else if (usb_menu_path & USBSTATUS_MTPONLY) 		usb_switch_select(USBSTATUS_MTPONLY);
		else
		{
			printk("usb menu path is error\n");
			return 0;
		}

//		usb_switch_select(oldusbstatus);
/*		if(usbstatus)
			UsbIndicator(1);
			
			*/
	}

	return size;
}

static DEVICE_ATTR(tethering, S_IRUGO |S_IWUGO | S_IRUSR | S_IWUSR, tethering_switch_show, tethering_switch_store);


static int askinitstatus=0;
static ssize_t AskInitStatusSel_switch_show(struct device *dev, struct device_attribute *attr, char *buf)
{ 
	if(askinitstatus == 2)
		return sprintf(buf, "%s\n", "START");
	else
		return sprintf(buf, "%s\n", "STOP");
}

static ssize_t AskInitStatusSel_switch_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	askinitstatus = askinitstatus + 2;
	return size;
}

static DEVICE_ATTR(AskInitStatusSel, S_IRUGO |S_IWUGO | S_IRUSR | S_IWUSR, AskInitStatusSel_switch_show, AskInitStatusSel_switch_store);


irqreturn_t fsa9480_interrupt(int irq, void *ptr)
{
	DEBUG_FSA9480("[FSA9480] %s\n ", __func__);
	
//	disable_irq_nosync(IRQ_FSA9480_INTB);
	
	set_dvfs_perf_level();
	

	queue_work(fsa9480_workqueue, &fsa9480_work);

	return IRQ_HANDLED; 
}

static void fsa9480_mode_check(struct work_struct *work)
{
	fsa9480_check_usb_connection();	
}

static void fsa9480_setup_irq(struct work_struct *work)
{
	set_irq_type(IRQ_EINT(23), IRQ_TYPE_EDGE_FALLING);
	if (request_irq(IRQ_EINT(23), fsa9480_interrupt, IRQF_DISABLED, "FSA9480 Detected", NULL)) 
	{
		DEBUG_FSA9480("[FSA9480] Fail to register IRQ[%d] for FSA9480 USB Switch \n", IRQ_EINT(23));
	}
	queue_work(fsa9480_workqueue, &fsa9480_work);
}

/* pm, param init check */
void pm_check(void)
{
	if (available_PM_Set()) {
		printk("[FSA9480] %s: pmic init finished\n", __func__);
		pm_check_finished = 1;
	}
	else {
		fsa9480_init_timer.expires = get_jiffies_64() + 50;
		add_timer(&fsa9480_init_timer);
	}
}

void check_param_init(void)
{
	if (pm_check_finished && sec_get_param_value) {
		printk("[FSA9480] %s: param init finished\n", __func__);
		if (!bFactoryReset)
			SEC_GET_PARAM(__SWITCH_SEL, usb_menu_path);

		// Irq request moved to workqueue because of kernel bug report (iks.kim)
		queue_work(fsa9480_workqueue, &fsa9480_irq_work);
	}
	else {
		param_check_timer.expires = get_jiffies_64() + 100;
		add_timer(&param_check_timer);
	}
}

static ssize_t print_switch_name(struct switch_dev *sdev, char *buf)
{
	return sprintf(buf, "%s\n", DRIVER_NAME);
}

static ssize_t print_switch_state(struct switch_dev *sdev, char *buf)
{
	if (usb_state & CRA_USB || (usb_state & CRB_JIG_USB))  {
			if(usb_menu_path & USBSTATUS_VTP)
				return sprintf(buf, "%s\n", "tethering online");
			else
				return sprintf(buf, "%s\n", "ums online");
/*		if((usb_menu_path & USBSTATUS_UMS) || (usb_menu_path & USBSTATUS_ADB))
			return sprintf(buf, "%s\n", "ums online");
//			return sprintf(buf, "%s\n", "online");
//			return sprintf(buf, "%s\n", "InsertOnline");
		else
			return sprintf(buf, "%s\n", "ums online");
//			return sprintf(buf, "%s\n", "online");
//			return sprintf(buf, "%s\n", "InsertOffline");*/
	}
	else 
	{
		if((usb_menu_path & USBSTATUS_UMS) || (usb_menu_path & USBSTATUS_ADB))
			return sprintf(buf, "%s\n", "ums offline");
//			return sprintf(buf, "%s\n", "offline");
//			return sprintf(buf, "%s\n", "RemoveOnline");
		else
			return sprintf(buf, "%s\n", "ums offline");
//			return sprintf(buf, "%s\n", "offline");
//			return sprintf(buf, "%s\n", "RemoveOffline");
	}		
}

static int fsa9480_codec_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct fsa9480_state *state;
	struct device *dev = &client->dev;
	u8 pData;

	DEBUG_FSA9480("[FSA9480] %s\n ", __func__);

	init_waitqueue_head(&usb_detect_waitq); 
	INIT_WORK(&fsa9480_work, fsa9480_mode_check);
	INIT_WORK(&fsa9480_irq_work, fsa9480_setup_irq);  // Added by iks.kim
	fsa9480_workqueue = create_singlethread_workqueue("fsa9480_workqueue");

	if (device_create_file(switch_dev, &dev_attr_usb_sel) < 0)
		pr_err("[FSA9480] Failed to create device file(%s)!\n", dev_attr_usb_sel.attr.name);

	if (device_create_file(switch_dev, &dev_attr_usb_state) < 0)
		pr_err("[FSA9480] Failed to create device file(%s)!\n", dev_attr_usb_state.attr.name);

	if (device_create_file(switch_dev, &dev_attr_DMport) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_DMport.attr.name);

	if (device_create_file(switch_dev, &dev_attr_DMlog) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_DMlog.attr.name);
	
	if (device_create_file(switch_dev, &dev_attr_UsbMenuSel) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_UsbMenuSel.attr.name);
	 
	if (device_create_file(switch_dev, &dev_attr_AskOnMenuSel) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_AskOnMenuSel.attr.name);

	if (device_create_file(switch_dev, &dev_attr_Mtp) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_Mtp.attr.name);

	if (device_create_file(switch_dev, &dev_attr_FactoryResetValue) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_FactoryResetValue.attr.name);

	if (device_create_file(switch_dev, &dev_attr_AskOnStatus) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_AskOnStatus.attr.name);

	if (device_create_file(switch_dev, &dev_attr_MtpInitStatusSel) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_MtpInitStatusSel.attr.name);

	if (device_create_file(switch_dev, &dev_attr_AskInitStatusSel) < 0)
		pr_err("Failed to create device file(%s)!\n", dev_attr_AskInitStatusSel.attr.name);	
	
	if (device_create_file(switch_dev, &dev_attr_tethering) < 0)		
		pr_err("Failed to create device file(%s)!\n", dev_attr_tethering.attr.name);

	/* FSA9480 Interrupt */
	s3c_gpio_cfgpin(GPIO_JACK_nINT, S3C_GPIO_SFN(GPIO_JACK_nINT_STATE));
	s3c_gpio_setpull(GPIO_JACK_nINT, S3C_GPIO_PULL_NONE);

	 state = kzalloc(sizeof(struct fsa9480_state), GFP_KERNEL);
	 if(!state) {
		 dev_err(dev, "%s: failed to create fsa9480_state\n", __func__);
		 return -ENOMEM;
	 }

	fsa9480_init = 1;

	indicator_dev.name = DRIVER_NAME;
	indicator_dev.print_name = print_switch_name;
	indicator_dev.print_state = print_switch_state;
	indicator_dev.state = 0;
	if (switch_dev_register(&indicator_dev) < 0) {
		printk("USB Indicator switch register failed.!!");	
	}

	state->client = client;
	fsa9480_i2c_client = client;

	i2c_set_clientdata(client, state);
	if(!client)
	{
		dev_err(dev, "%s: failed to create fsa9480_i2c_client\n", __func__);
		return -ENODEV;
	}

	/*init wakelock*/
	wake_lock_init(&fsa9480_wake_lock, WAKE_LOCK_SUSPEND, "fsa9480_wakelock");

	/*clear interrupt mask register*/
	fsa9480_read(client, REGISTER_CONTROL, &pData);
	fsa9480_write(client, REGISTER_CONTROL, pData & ~INT_MASK);

	// Check MANUALSW1 register (It was set by SWITCH_SEL parameter)
	fsa9480_read(client, REGISTER_MANUALSW1, &pData);
	DEBUG_FSA9480("[FSA9480] %s : MANUALSW1 reg (0x%x)\n", __func__, pData);

	if (pData == 0x24) // PDA
		usb_path = SWITCH_PDA;	
	else
		usb_path = SWITCH_MODEM;
	usb_sel(usb_path);

	init_timer(&fsa9480_init_timer);
	fsa9480_init_timer.function = (void*) pm_check;

	init_timer(&param_check_timer);
	param_check_timer.function = (void*) check_param_init;

	pm_check();

	check_param_init();

	return 0;
}

static int __devexit fsa9480_remove(struct i2c_client *client)
{
	struct fsa9480_state *state = i2c_get_clientdata(client);
	kfree(state);
	wake_lock_destroy(&fsa9480_wake_lock);
	return 0;
}


struct i2c_driver fsa9480_i2c_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "fsa9480",
	},
	.id_table	= fsa9480_id,
	.probe	= fsa9480_codec_probe,
	.remove	= __devexit_p(fsa9480_remove),
	.command = NULL,
};

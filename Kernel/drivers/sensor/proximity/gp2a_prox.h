#ifndef __GP2A_PROX_H
#define __GP2A_PROX_H

#define PROX_SENSOR_I2C_ADDR	0x44
#define PROX_IRQ				IRQ_EINT(25)		

//#define GP2A_DEBUG

#define error(fmt, arg...) printk("--------" fmt "\n", ##arg)

#ifdef GP2A_DEBUG
#define debug(fmt, arg...) printk("--------" fmt "\n", ##arg)
#else
#define debug(fmt,arg...)
#endif

/*GP2AP002S00F Register Addresses1*/
#define GP2A_REG_PROX 	0x00
#define GP2A_REG_GAIN 	0x01
#define GP2A_REG_HYS 	0x02
#define GP2A_REG_CYCLE 	0x03
#define GP2A_REG_OPMOD 	0x04
#define GP2A_REG_CON 	0x06

/*Proximity value register*/	
#define REG0_PROX_VALUE_MASK   	0x01
	
/*Driver data */
struct gp2a_prox_data {		
	int    irq;
	struct i2c_client* gp2a_prox_i2c_client;
	struct work_struct work_prox;  /* for proximity sensor */ 
	struct input_dev *prox_input_dev;
	u8 reg[7];
};
static struct i2c_driver  gp2a_prox_i2c_driver;
static struct gp2a_prox_data *gp2a_data;
static struct workqueue_struct *gp2a_prox_wq;
static int proximity_enable;
static struct wake_lock prox_wakelock;

static irqreturn_t gp2a_irq_handler( int, void* );
static int gp2a_prox_open(struct inode* , struct file* );
static int gp2a_prox_release(struct inode* , struct file* );
static int gp2a_prox_ioctl(struct inode* , struct file*, 
	                        unsigned int,unsigned long );
static int gp2a_prox_probe(struct i2c_client *,const struct i2c_device_id *);
static int gp2a_prox_suspend(struct i2c_client *, pm_message_t);
static int gp2a_prox_resume(struct i2c_client *);
static int gp2a_prox_mode(int );
static void gp2a_prox_work_func(struct work_struct* );
static void gp2a_chip_init(void);
static int gp2a_i2c_read(u8*);
static int gp2a_i2c_write(u8,u8*);
int get_gp2a_proximity_value(void);
int get_gp2a_enable_state(void);
/*Function call for checking the hardware revision of the target*/
extern int apollo_get_remapped_hw_rev_pin();
extern void gp2a_report_proximity_value(unsigned int); 

	
#endif
/* drivers/leds/leds-s5p6442.c
 *
 * (c) 2011 Sebastien Dadier (s.dadier@gmail.com)
 *
 * S5P6442 - LED driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <plat/gpio-cfg.h>
#include <linux/leds.h>
#include <linux/i2c/pmic.h>
#include <mach/hardware.h>
#include <mach/s5p_leds.h>

/* our context */

struct s5p6442_led {
	struct led_classdev		 cdev;
	struct s5p6442_led_platdata	*pdata;
};

static inline struct s5p6442_led *pdev_to_led(struct platform_device *dev)
{
	return platform_get_drvdata(dev);
}

static inline struct s5p6442_led *to_led(struct led_classdev *led_cdev)
{
	return container_of(led_cdev, struct s5p6442_led, cdev);
}


static void s5p6442_led_set(struct led_classdev *led_cdev,
			    enum led_brightness value)
{

  if ( value == LED_OFF )

	  Set_MAX8998_PM_REG(ELDO17, 0);

	else

		Set_MAX8998_PM_REG(ELDO17, 1);

}

static int s5p6442_led_remove(struct platform_device *dev)
{
  struct s5p6442_led *led = pdev_to_led(dev);

	led_classdev_unregister(&led->cdev);
	kfree(led);

	return 0;
}

static int s5p6442_led_probe(struct platform_device *dev)
{
	struct s5p6442_led_platdata *pdata = dev->dev.platform_data;
	struct s5p6442_led *led;
	int ret;

	led = kzalloc(sizeof(struct s5p6442_led), GFP_KERNEL);
	if (led == NULL) {
		dev_err(&dev->dev, "No memory for device\n");
		return -ENOMEM;
	}

	platform_set_drvdata(dev, led);

	led->cdev.brightness_set = s5p6442_led_set;
	led->cdev.default_trigger = pdata->def_trigger;
	led->cdev.name = pdata->name;
	led->cdev.flags |= LED_CORE_SUSPENDRESUME;

	led->pdata = pdata;

	/* register our new led device */

	ret = led_classdev_register(&dev->dev, &led->cdev);
	if (ret < 0) {
		dev_err(&dev->dev, "led_classdev_register failed\n");
		kfree(led);
		return ret;
	}

  printk("registered s5p6442_led driver\n");

	return 0;
}

static struct platform_driver s5p6442_led_driver = {
	.probe		= s5p6442_led_probe,
	.remove		= s5p6442_led_remove,
	.driver		= {
		.name		= "s5p6442_led",
		.owner		= THIS_MODULE,
	},
};

static int __init s5p6442_led_init(void)
{
	return platform_driver_register(&s5p6442_led_driver);
}

static void __exit s5p6442_led_exit(void)
{
	platform_driver_unregister(&s5p6442_led_driver);
}

module_init(s5p6442_led_init);
module_exit(s5p6442_led_exit);

MODULE_AUTHOR("Sebastien Dadier <s.dadier@gmail.com>");
MODULE_DESCRIPTION("s5p6442 LED driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:s5p6442_led");

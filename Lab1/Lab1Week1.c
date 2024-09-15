#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/err.h>

MODULE_LICENSE("GPL");

const int BLUE_LED_GPIO = 5;
const int GREEN_LED_GPIO = 4;
const int RED_LED_GPIO = 2;
const int YELLOW_LED_GPIO = 3;

static int __init led_blink_init(void) {
    printk(KERN_INFO "Module installed!\n");

   // Check if LED GPIO pins are valid
   if(!gpio_is_valid(BLUE_LED_GPIO) || !gpio_is_valid(GREEN_LED_GPIO) ||
       !gpio_is_valid(RED_LED_GPIO) || !gpio_is_valid(YELLOW_LED_GPIO)) {

	   printk(KERN_INFO "A GPIO Pin is Invalid");
	   return 0;
   }

   // Request GPIO pins, return an error code upon an error
   if(gpio_request(BLUE_LED_GPIO, "Blue LED") || gpio_request(GREEN_LED_GPIO, "Green LED") ||
      gpio_request(RED_LED_GPIO, "Red LED") || gpio_request(YELLOW_LED_GPIO, "Yellow LED")) {

	   printk(KERN_INFO "gpio_request failed");
      return 0;
   }

   // Set GPIO pins to outputs, setting everything to zero. Return an error code upon an error
   if(gpio_direction_output(BLUE_LED_GPIO, 0) || gpio_direction_output(GREEN_LED_GPIO, 0) ||
      gpio_direction_output(RED_LED_GPIO, 0) || gpio_direction_output(YELLOW_LED_GPIO, 0)) {

      printk(KERN_INFO "gpio_directon_output failed");
      return 0;
   }

   // Turn the pins on and off
   for(int i=0; i<5; i++){
	   gpio_set_value(BLUE_LED_GPIO, 1);
        gpio_set_value(GREEN_LED_GPIO, 1);
	   gpio_set_value(YELLOW_LED_GPIO, 1);
	   gpio_set_value(RED_LED_GPIO, 1);

	   msleep(500);

	   gpio_set_value(BLUE_LED_GPIO, 0);
	   gpio_set_value(GREEN_LED_GPIO, 0);
	   gpio_set_value(YELLOW_LED_GPIO, 0);
	   gpio_set_value(RED_LED_GPIO, 0);

	   msleep(500);
   }

    return 0;
}

static void __exit led_blink_exit(void) {
   printk(KERN_INFO "Module removed!\n");

   // Release GPIO pins
   gpio_free(BLUE_LED_GPIO);
   gpio_free(RED_LED_GPIO);
   gpio_free(GREEN_LED_GPIO);
   gpio_free(YELLOW_LED_GPIO);
}

module_init(led_blink_init);
module_exit(led_blink_exit);

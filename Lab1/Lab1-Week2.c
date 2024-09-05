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
#include <linux/jiffies.h>

MODULE_LICENSE("GPL");

#define MSG_SIZE 40
#define CDEV_NAME "Lab1"

int InputPin=16, OutPin=3;
int GPIO_irqNumber;

// Interrupt handler function. Tha name "button_isr" can be different.
// You may use printk statements for debugging purposes.

static char msg[40];
static int major;

static ssize_t device_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset)
{
	printk(KERN_INFO "In device_read function");

	// Whatever is in msg will be placed into buffer, which will be copied into user space
	ssize_t dummy = copy_to_user(buffer, msg, length); // dummy will be 0 if successful
	// msg should be protected (e.g. semaphore). Not implemented here, but you can add it.
	msg[0] = '\0'; // "Clear" the message, in case the device is read again.
	// This way, the same message will not be read twice.
	// Also convenient for checking if there is nothing new, in user space.
	return length;
}

// structure needed when registering the Character Device. Members are the callback
// functions when the device is read from or written to.
static struct file_operations fops = {
	.read = device_read,
};

//Interrupt handler for InputPin. This will be called whenever there is a raising edge detected.
static irqreturn_t gpio_irq_handler(int irq,void *dev_id)
{
  	// Stuff you want the module do when the interrupt handler is called
	msg[0] = irq;
	printk(KERN_INFO "Interrupt on IRQ %i", irq);
	return IRQ_HANDLED;
}



/*
** Module Init function
*/
int init_module()   // Call the default name
{
	// register the Characted Device and obtain the major (assigned by the system)
	major = register_chrdev(0, CDEV_NAME, &fops);
	if (major < 0) {
		printk("Registering the character device failed with %d\n", major);
		return major;
	}
	printk("Lab1_cdev_kmod example, assigned major: %d\n", major);
	printk("Create Char Device (node) with: sudo mknod /dev/%s c %d 0\n", CDEV_NAME, major);

	// Configure the output and the input pins

	// Verify pins
	if(!gpio_is_valid(InputPin) || !gpio_is_valid(OutPin)){
		printk(KERN_INFO "A GPIO Pin is Invalid");
	}

	// Request pins
	if(gpio_request(InputPin, "Input") || gpio_request(OutPin, "Output")){
		printk(KERN_INFO "gpio_request failed");
	}

	// Set direction
	if(gpio_direction_input(InputPin) || gpio_direction_output(OutPin, 0)){
		printk(KERN_INFO "gpio_direction failed");
	}


	//Get the IRQ number for our GPIO
	GPIO_irqNumber = gpio_to_irq(InputPin);

 	// Request the interrupt / attach handler
 	//Enable (Async) Rising Edge detection

	//The Fourth argument string can be different (you give the name)
	//The last argument is a variable needed to identify the handler, but can be set to NULL
	if (request_irq(GPIO_irqNumber,(void *)gpio_irq_handler, IRQF_TRIGGER_RISING,"Button Interrupt", NULL)) {
		pr_err("Cannot register the IRQ ");
		gpio_free(InputPin);
	}

	pr_info("Module Inserted!!!\n");
	return 0;

}

/*
** Module exit function
*/
void cleanup_module()
{

	// ------
	// Remove the interrupt handler; you need to provide the same identifier
	free_irq(GPIO_irqNumber,NULL);

	// Free the Pins
	gpio_free(InputPin);
	gpio_free(OutPin);

	// Once unregistered, the Character Device won't be able to be accessed,even if the file /dev/YourDevName still exists.
	unregister_chrdev(major, CDEV_NAME);
	printk("Char Device /dev/%s unregistered.\n", CDEV_NAME);

	printk("*************Module Removed**********\n");
}


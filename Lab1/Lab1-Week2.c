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

const int BLUE_BUTTON_GPIO = 19;
const int GREEN_BUTTON_GPIO = 18;
const int RED_BUTTON_GPIO = 16;
const int YELLOW_BUTTON_GPIO = 17;
const int RESET_BUTTON_GPIO = 20;

int GPIO_irqNumber;

// Interrupt handler function. Tha name "button_isr" can be different.
// You may use printk statements for debugging purposes.

static char msg[40];
static int major;

static ssize_t device_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset)
{
	// Whatever is in msg will be placed into buffer, which will be copied into user space
	ssize_t dummy = copy_to_user(buffer, msg, length); // dummy will be 0 if successful
	printk(KERN_INFO "Here %i\n", buffer[0]);

	// msg should be protected (e.g. semaphore). Not implemented here, but you can add it.
	//msg[0] = '\0'; // "Clear" the message, in case the device is read again.

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
  	// Send character to character device based on irq
	if(irq == gpio_to_irq(BLUE_BUTTON_GPIO)){
		msg[0] = 'b';
	}
	else if(irq == gpio_to_irq(RED_BUTTON_GPIO)){
		msg[0] = 'm';
	}
	else if(irq == gpio_to_irq(GREEN_BUTTON_GPIO)){
		msg[0] = 'g';
	}
	else if(irq == gpio_to_irq(YELLOW_BUTTON_GPIO)){
		msg[0] = 'y';
	}
	else if(irq == gpio_to_irq(RESET_BUTTON_GPIO)){
		msg[0] = 'r';
	}
	
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
	if(!gpio_is_valid(BLUE_BUTTON_GPIO) || !gpio_is_valid(GREEN_BUTTON_GPIO) ||
       !gpio_is_valid(RED_BUTTON_GPIO) || !gpio_is_valid(YELLOW_BUTTON_GPIO) || !gpio_is_valid(RESET_BUTTON_GPIO)) {

		printk(KERN_INFO "A Button GPIO Pin is Invalid");
	}

	// Request pins
	if(gpio_request(BLUE_BUTTON_GPIO, "Blue Button") || gpio_request(GREEN_BUTTON_GPIO, "Green Button") ||
	   gpio_request(RED_BUTTON_GPIO, "Red Button") || gpio_request(YELLOW_BUTTON_GPIO, "Yellow Button") || gpio_request(RESET_BUTTON_GPIO, "Reset Button")){
		
		printk(KERN_INFO "Button gpio_request failed");
	}

	// Set direction
	if(gpio_direction_input(BLUE_BUTTON_GPIO) || gpio_direction_input(RED_BUTTON_GPIO) || gpio_direction_input(GREEN_BUTTON_GPIO) ||
	   gpio_direction_input(YELLOW_BUTTON_GPIO) || gpio_direction_input(RESET_BUTTON_GPIO) ){
		printk(KERN_INFO "Button gpio_direction failed");
	}

 	// Request the interrupt / attach handler
	//Get the IRQ number for our GPIO
	int blue_GPIO_irqNumber = gpio_to_irq(BLUE_BUTTON_GPIO);
	int red_GPIO_irqNumber = gpio_to_irq(RED_BUTTON_GPIO);
	int green_GPIO_irqNumber = gpio_to_irq(GREEN_BUTTON_GPIO);
	int yellow_GPIO_irqNumber = gpio_to_irq(YELLOW_BUTTON_GPIO);
	int reset_GPIO_irqNumber = gpio_to_irq(RESET_BUTTON_GPIO);

 	//Enable (Async) Rising Edge detection
	if (request_irq(blue_GPIO_irqNumber,(void *)gpio_irq_handler, IRQF_TRIGGER_RISING,"Button Interrupt", NULL)) {
		pr_err("Cannot register the IRQ for Blue Button");
		gpio_free(BLUE_BUTTON_GPIO);
	}
	if (request_irq(red_GPIO_irqNumber,(void *)gpio_irq_handler, IRQF_TRIGGER_RISING,"Button Interrupt", NULL)) {
		pr_err("Cannot register the IRQ for Red Button");
		gpio_free(RED_BUTTON_GPIO);
	}
	if (request_irq(green_GPIO_irqNumber,(void *)gpio_irq_handler, IRQF_TRIGGER_RISING,"Button Interrupt", NULL)) {
		pr_err("Cannot register the IRQ for Green Button");
		gpio_free(GREEN_BUTTON_GPIO);
	}
	if (request_irq(yellow_GPIO_irqNumber,(void *)gpio_irq_handler, IRQF_TRIGGER_RISING,"Button Interrupt", NULL)) {
		pr_err("Cannot register the IRQ for Yellow Button");
		gpio_free(YELLOW_BUTTON_GPIO);
	}
	if (request_irq(reset_GPIO_irqNumber,(void *)gpio_irq_handler, IRQF_TRIGGER_RISING,"Button Interrupt", NULL)) {
		pr_err("Cannot register the IRQ for Reset Button");
		gpio_free(RESET_BUTTON_GPIO);
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
	gpio_free(BLUE_BUTTON_GPIO);
	gpio_free(RED_BUTTON_GPIO);
	gpio_free(GREEN_BUTTON_GPIO);
	gpio_free(YELLOW_BUTTON_GPIO);
	gpio_free(RESET_BUTTON_GPIO);

	// Once unregistered, the Character Device won't be able to be accessed,even if the file /dev/YourDevName still exists.
	unregister_chrdev(major, CDEV_NAME);
	printk("Char Device /dev/%s unregistered.\n", CDEV_NAME);

	printk("*************Module Removed**********\n");
}


/* Based on code found at https://gist.github.com/maggocnx/5946907
   Modified and commented by: Luis Rivera

   Compile using the Makefile
*/

#ifndef MODULE
#define MODULE
#endif
#ifndef __KERNEL__
#define __KERNEL__
#endif

#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/err.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <asm/io.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/unistd.h>

MODULE_LICENSE("GPL");

#define GPIO_OUT6 (6)
#define BUTTON_1 (16)
#define BUTTON_2 (17)
#define BUTTON_3 (18)
#define BUTTON_4 (19)
#define BUTTON_5 (20)

#define MSG_SIZE 40
#define CDEV_NAME "Lab6"	// "YourDevName"

unsigned long timer_interval_ns = 5e5;	// timer interval length (nano sec part)
static struct hrtimer hr_timer;			// timer structure
static int count = 0, dummy = 0;

unsigned long *ptr,*GPSET0, *GPFSEL0, *GPCLR0;

int flag=0;
int state = 0;
 
static int major; 
static char msg_r[MSG_SIZE];
static char msg_w[MSG_SIZE];

static int button_1_irqNumber;
static int button_2_irqNumber;
static int button_3_irqNumber;
static int button_4_irqNumber;
static int button_5_irqNumber;


// Function called when the user space program reads the character device.
// Some arguments not used here.
// buffer: data will be placed there, so it can go to user space
// The global variable msg is used. Whatever is in that string will be sent to userspace.
// Notice that the variable may be changed anywhere in the module...
static ssize_t device_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset)
{
	// Whatever is in msg will be placed into buffer, which will be copied into user space
	ssize_t dummy = copy_to_user(buffer, msg_r, length);	// dummy will be 0 if successful

	// msg should be protected (e.g. semaphore). Not implemented here, but you can add it.
	msg_r[0] = '\0';	// "Clear" the message, in case the device is read again.
					// This way, the same message will not be read twice.
					// Also convenient for checking if there is nothing new, in user space.
	
	return length;
}

// Function called when the user space program writes to the Character Device.
// Some arguments not used here.
// buff: data that was written to the Character Device will be there, so it can be used
//       in Kernel space.
// In this example, the data is placed in the same global variable msg used above.
// That is not needed. You could place the data coming from user space in a different
// string, and use it as needed...
static ssize_t device_write(struct file *filp, const char __user *buff, size_t len, loff_t *off)
{
	ssize_t dummy;
	char note;
	
	if(len > MSG_SIZE)
		return -EINVAL;
	
	// unsigned long copy_from_user(void *to, const void __user *from, unsigned long n);
	dummy = copy_from_user(msg_w, buff, len);	// Transfers the data from user space to kernel space
	if(len == MSG_SIZE)
		msg_w[len-1] = '\0';	// will ignore the last character received.
	else
		msg_w[len] = '\0';
	
	// You may want to remove the following printk in your final version.
	printk("Message from user space: %s\n", msg_w);

	// Set timer period here
	note = msg_w[0]; // Assume first char is the note

	if(note == 'A'){
		timer_interval_ns = 2272727; // A5
	}
	else if(note == 'B'){
		timer_interval_ns = 2024291; // B5
	}
	else if(note == 'C'){
		timer_interval_ns = 1912046; // C5
	}
	else if(note == 'D'){
		timer_interval_ns = 1703577; // D5
	}
	else if(note == 'E'){
		timer_interval_ns = 1517451; // E5
	}
	else{ // Misread the note
		timer_interval_ns = 5405405; //F#3
	}
	
	return len;		// the number of bytes that were written to the Character Device.
}

// structure needed when registering the Character Device. Members are the callback
// functions when the device is read from or written to.
static struct file_operations fops = {
	.read = device_read, 
	.write = device_write,
};

// Timer callback function: this executes when the timer expires
enum hrtimer_restart timer_callback(struct hrtimer *timer_for_restart)
{
  	ktime_t currtime, interval;	// time type, in nanoseconds
	unsigned long overruns = 0;

	// Re-configure the timer parameters (if needed/desired)
  	currtime  = ktime_get();
  	interval = ktime_set(0, timer_interval_ns); // (long sec, long nano_sec)

	// Write a code to alternate the SPKR PIN from HIGH to LOW or vice versa 
	// (based on the previous state to generate the square signal)
	gpio_set_value(GPIO_OUT6, state);
	state = !state;

	// Advancing the expiration time to the next interval. This returns how many
	// intervals have passed. More than 1 may happen if the system load is too high.
  	overruns = hrtimer_forward(timer_for_restart, currtime, interval);


	// The following printk only executes once every 1000 cycles.
	if(dummy == 0){
		printk("Count: %d, overruns: %ld\n", ++count, overruns);
	}
	dummy = (dummy + 1)%1000;


	return HRTIMER_RESTART;	// Return this value to restart the timer.
							// If you don't want/need a recurring timer, return
							// HRTIMER_NORESTART (and don't forward the timer).
}

//Interrupt handler for InputPin. This will be called whenever there is a raising edge detected.
static irqreturn_t gpio_irq_handler(int irq, void *dev_id)
{
  	// Send character to character device based on irq
	if(irq == gpio_to_irq(BUTTON_1)){
		msg_r[0] = 'A';
	}
	else if(irq == gpio_to_irq(BUTTON_2)){
		msg_r[0] = 'B';
	}
	else if(irq == gpio_to_irq(BUTTON_3)){
		msg_r[0] = 'C';
	}
	else if(irq == gpio_to_irq(BUTTON_4)){
		msg_r[0] = 'D';
	}
	else if(irq == gpio_to_irq(BUTTON_5)){
		msg_r[0] = 'E';
	}
	
	printk(KERN_INFO "Interrupt on IRQ %i", irq);
	return IRQ_HANDLED;
}


int timer_init(void)
{
	// register the Characted Device and obtain the major (assigned by the system)
	major = register_chrdev(0, CDEV_NAME, &fops);
	if (major < 0) {
     		printk("Registering the character device failed with %d\n", major);
	     	return major;
	}
	printk("Lab6_cdev_kmod example, assigned major: %d\n", major);
	printk("Create Char Device (node) with: sudo mknod /dev/%s c %d 0\n", CDEV_NAME, major);

	// Configure the SPKR PIN to be output 
	// Verify pin
	if(!gpio_is_valid(GPIO_OUT6)) {

		printk(KERN_INFO "A GPIO Pin 6 is Invalid");
	}

	// Request pins
	if(gpio_request(GPIO_OUT6, "Speaker")) {
		printk(KERN_INFO "gpio_request failed");
	}

	// Set to Output
	if(gpio_direction_output(GPIO_OUT6, 0)){
		printk(KERN_INFO "gpio_direction failed");
	}

	// Set up buttons

	// Verify pins
	if(!gpio_is_valid(BUTTON_1) || !gpio_is_valid(BUTTON_2) ||
       !gpio_is_valid(BUTTON_3) || !gpio_is_valid(BUTTON_4) || !gpio_is_valid(BUTTON_5)) {

		printk(KERN_INFO "A Button GPIO Pin is Invalid");
	}

	// Request pins
	if(gpio_request(BUTTON_1, "Button 1") || gpio_request(BUTTON_2, "Button 2") ||
	   gpio_request(BUTTON_3, "Button 3") || gpio_request(BUTTON_4, "Button 4") || gpio_request(BUTTON_5, "Button 5")){
		
		printk(KERN_INFO "Button gpio_request failed");
	}

	// Set direction
	if(gpio_direction_input(BUTTON_1) || gpio_direction_input(BUTTON_2) || gpio_direction_input(BUTTON_3) ||
	   gpio_direction_input(BUTTON_4) || gpio_direction_input(BUTTON_5) ){
		printk(KERN_INFO "Button gpio_direction failed");
	}

 	// Request the interrupt / attach handler
	//Get the IRQ number for our GPIO
	button_1_irqNumber = gpio_to_irq(BUTTON_1);
	button_2_irqNumber = gpio_to_irq(BUTTON_2);
	button_3_irqNumber = gpio_to_irq(BUTTON_3);
	button_4_irqNumber = gpio_to_irq(BUTTON_4);
	button_5_irqNumber = gpio_to_irq(BUTTON_5);

 	//Enable (Async) Rising Edge detection
	if (request_irq(button_1_irqNumber,(void *)gpio_irq_handler, IRQF_TRIGGER_RISING,"Button Interrupt", NULL)) {
		pr_err("Cannot register the IRQ for Button 1");
		gpio_free(BUTTON_1);
	}
	if (request_irq(button_2_irqNumber,(void *)gpio_irq_handler, IRQF_TRIGGER_RISING,"Button Interrupt", NULL)) {
		pr_err("Cannot register the IRQ for Button 2");
		gpio_free(BUTTON_2);
	}
	if (request_irq(button_3_irqNumber,(void *)gpio_irq_handler, IRQF_TRIGGER_RISING,"Button Interrupt", NULL)) {
		pr_err("Cannot register the IRQ for Button 3");
		gpio_free(BUTTON_3);
	}
	if (request_irq(button_4_irqNumber,(void *)gpio_irq_handler, IRQF_TRIGGER_RISING,"Button Interrupt", NULL)) {
		pr_err("Cannot register the IRQ for Button 4");
		gpio_free(BUTTON_4);
	}
	if (request_irq(button_5_irqNumber,(void *)gpio_irq_handler, IRQF_TRIGGER_RISING,"Button Interrupt", NULL)) {
		pr_err("Cannot register the IRQ for Button 5");
		gpio_free(BUTTON_5);
	}

	printk(KERN_INFO "Interrupts Completed");

   //Timer initialization 

	ktime_t ktime = ktime_set(0, timer_interval_ns); // (long sec, long nano_sec)

	// CLOCK_MONOTONIC: always move forward in time, even if system time changes
	// HRTIMER_MODE_REL: time relative to current time.
	hrtimer_init(&hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
 	return 0;
	// Attach callback function to the timer
	hr_timer.function = &timer_callback;

	// Start the timer
 	hrtimer_start(&hr_timer, ktime, HRTIMER_MODE_REL);

	return 0;
}

void timer_exit(void)
{
	int ret;
  	ret = hrtimer_cancel(&hr_timer);	// cancels the timer.
  	if(ret)
		printk("The timer was still in use...\n");
	else
		printk("The timer was already cancelled...\n");	// if not restarted or
														// cancelled before
	
	//FREE PIN 6
	gpio_free(GPIO_OUT6);

	//FREE Interrupts
	free_irq(button_1_irqNumber, "Button Interrupt");
	free_irq(button_2_irqNumber, "Button Interrupt");
	free_irq(button_3_irqNumber, "Button Interrupt");
	free_irq(button_4_irqNumber, "Button Interrupt");
	free_irq(button_5_irqNumber, "Button Interrupt");

	//FREE Buttons
	gpio_free(BUTTON_1);
	gpio_free(BUTTON_2);
	gpio_free(BUTTON_3);
	gpio_free(BUTTON_4);
	gpio_free(BUTTON_5);

  	printk("HR Timer module uninstalling\n");

	// Once unregistered, the Character Device won't be able to be accessed,
	// even if the file /dev/YourDevName still exists
	unregister_chrdev(major, CDEV_NAME);
	printk("Char Device /dev/%s unregistered.\n", CDEV_NAME);

}

// Notice this alternative way to define your init_module()
// and cleanup_module(). "timer_init" will execute when you install your
// module. "timer_exit" will execute when you remove your module.
// You can give different names to those functions.
module_init(timer_init);
module_exit(timer_exit);


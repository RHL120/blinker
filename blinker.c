#include <linux/fs.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>

MODULE_LICENSE("GPL");

//The default value in blinker_device.sleep_time
unsigned long default_sleep_time = 10;
//The default major number, if it is zero alloc_chrdev_region will be used
int major = 0;
//The gpio pin to which the led is connected
int gpio_pin = 7;
module_param(default_sleep_time, ulong, S_IRUGO);
module_param(major, int, S_IRUGO);
module_param(gpio_pin, int, S_IRUGO);

dev_t dev_num;

//The device's struct
struct blinker_device_struct {
	struct cdev cdev; //The cdev of the char device
	bool led_status; //This should be true if the led is on, false if the led is off
	unsigned long sleep_time; //The amount of time to sleep between each blink
};


int blinker_open(struct inode *inode, struct file *filp)
{
	return 0;
}

ssize_t blinker_write(struct file *filp, const char __user *buf, size_t size,
		loff_t *off)
{
	return 0;
}

long blinker_ioctl (struct file *filp, unsigned int cmd, unsigned long arg)
{
	return 0;
}

struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = blinker_open,
	.write = blinker_write,
};

static __init int blinker_init(void)
{
	return 0;
}

static __exit void blinker_exit(void)
{

}

module_init(blinker_init);
module_exit(blinker_exit);

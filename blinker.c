#include <linux/fs.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include "blinker.h"

#define DRIVER_LABLE "blinker"

MODULE_LICENSE("GPL");

//The default value in blinker_device.sleep_time
unsigned long default_sleep_time = 1000;
//The default major number, if it is zero alloc_chrdev_region will be used
int major = 0;
//The gpio pin to which the led is connected
int gpio_pin = 7;
//What status should the LED be in when the module is loaded
bool default_led_status = false;
module_param(default_sleep_time, ulong, S_IRUGO);
module_param(major, int, S_IRUGO);
module_param(gpio_pin, int, S_IRUGO);
module_param(default_led_status, bool, S_IRUGO);

dev_t dev_num;

//The device's struct
struct blinker_device_struct {
	struct mutex mutex; //The mutex held when the file is being writen to
	struct cdev cdev; //The cdev of the char device
	bool led_status; //This should be true if the led is on, false if the led is off
	unsigned long sleep_time; //The amount of time to sleep between each blink
	int pin; //The pin to which the led is connected
};

struct blinker_device_struct blinker_device;

int blinker_open(struct inode *inode, struct file *filp)
{
	//Get a pointer to the device_struct
	filp->private_data = container_of(inode->i_cdev,
			struct blinker_device_struct, cdev);
	BUG_ON(!filp->private_data);
	return 0;
}

ssize_t blinker_read(struct file *filp, char __user *buf, size_t size, loff_t *off)
{
	struct blinker_device_struct *dev = filp->private_data;
	char res = 0;
	//only output once.
	//this helps with cat
	if (*off > 0) {
		return 0;
	}
	if (mutex_lock_interruptible(&dev->mutex)) {
		return -ERESTARTSYS;
	}
	res = dev->led_status? '1' : '0';
	mutex_unlock(&dev->mutex);
	if (put_user(res, buf)) {
		return -EFAULT;
	}
	(*off)++;
	return 1;
}

ssize_t blinker_write(struct file *filp, const char __user *buf, size_t size,
		loff_t *off)
{
	ssize_t ret = 0;
	struct blinker_device_struct *dev = filp->private_data;
	if (mutex_lock_interruptible(&dev->mutex))
		return -ERESTARTSYS;
	for (size_t i = 0; i < size; i++) {
		char chr = 0;
		if (get_user(chr, buf + i)) {
			ret = -EFAULT;
			goto ret;
		}
		switch (chr) {
		case '0':
			dev->led_status = false;
			break;
		case '1':
			dev->led_status = true;
			break;
		default:
			ret = -EINVAL;
			goto ret;
		}
#ifndef NO_GPIO
		gpio_set_value(dev->pin, dev->led_status);
#endif
		if (i < size - 1)
			mdelay(dev->sleep_time);
		(*off)++;
		ret++;
	}
ret:
	mutex_unlock(&dev->mutex);
	return ret;
}

long blinker_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long ret = 0;
	struct blinker_device_struct *dev = filp->private_data;
	if (mutex_lock_interruptible(&dev->mutex)) {
		return -ERESTARTSYS;
	}
	switch (cmd) {
	case BLINKER_GET_PIN:
		if (put_user(dev->pin, (int *)arg)) {
			ret = -EFAULT;
		}
		goto ret;
	case BLINKER_SET_PIN:
		{
			int pin = 0;
			if (get_user(pin, (int *)arg)) {
				ret = -EFAULT;
				goto ret;
			}
#ifdef NO_GPIO
			dev->pin = pin;
			ret = 0;
			goto ret;
#endif
			if (pin != dev->pin && !(ret = gpio_request(pin,
							DRIVER_LABLE))) {
				if ((ret = gpio_direction_output(pin, 0))) {
					gpio_free(pin);
					goto ret;
				}
				gpio_set_value(pin, dev->led_status);
				dev->pin = pin;
			}
			goto ret;
		}
	case BLINKER_GET_SLEEP:
		if (put_user(dev->sleep_time, (unsigned long *)arg)) {
			ret = -EFAULT;
		}
		goto ret;
	case BLINKER_SET_SLEEP:
		if (get_user(dev->sleep_time, (unsigned long *)arg))
			ret = -EFAULT;
		goto ret;
	default:
		ret = -ENOTTY;
	}
ret:
	mutex_unlock(&dev->mutex);
	return ret;
}

struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = blinker_open,
	.read = blinker_read,
	.write = blinker_write,
	.unlocked_ioctl = blinker_ioctl,
};

static int blinker_device_init(struct blinker_device_struct *dev,
		bool led_status, int gpio_pin, unsigned long sleep_time,
		dev_t dev_num)
{
	int ret = 0;
	mutex_init(&dev->mutex);
	dev->led_status = led_status;
	dev->pin = gpio_pin;
	dev->sleep_time = sleep_time;
#ifndef NO_GPIO
	if ((ret = gpio_request(gpio_pin, DRIVER_LABLE)))
		goto ret;
	if ((ret = gpio_direction_output(gpio_pin, 0)))
		goto gpio_free_ret;
	gpio_set_value(gpio_pin, led_status);
#endif
	cdev_init(&dev->cdev, &fops);
	if(!(ret = cdev_add(&dev->cdev, dev_num, 1)))
		goto ret;
gpio_free_ret: gpio_free(gpio_pin);
ret: return ret;

}

static __init int blinker_init(void)
{
	int ret = 0;
	if (major) {
		dev_num = MKDEV(major, 0);
		ret = register_chrdev_region(dev_num, 1, DRIVER_LABLE);
	} else {
		ret = alloc_chrdev_region(&dev_num, 0, 1, DRIVER_LABLE);
	}
	if (ret)
		goto ret;
	ret = blinker_device_init(&blinker_device, default_led_status, gpio_pin,
			default_sleep_time, dev_num);
	if (ret)
		goto chrdev_unregister_ret;
	printk("The device has been registered, got major: %d, minor: %d\n",
			MAJOR(dev_num), MINOR(dev_num));
	goto ret;
chrdev_unregister_ret: unregister_chrdev_region(dev_num, 1);
ret: return ret;
}

static __exit void blinker_exit(void)
{
	cdev_del(&blinker_device.cdev);
	unregister_chrdev_region(dev_num, 1);
#ifndef NO_GPIO
	gpio_free(gpio_pin);
#endif
}

module_init(blinker_init);
module_exit(blinker_exit);

#include <linux/ioctl.h>

#define BLINKER_IOC_MAGIC 'B'

#define BLINKER_GET_PIN _IOR(BLINKER_IOC_MAGIC, 0, int)
#define BLINKER_SET_PIN _IOW(BLINKER_IOC_MAGIC, 1, int)
#define BLINKER_GET_SLEEP _IOR(BLINKER_IOC_MAGIC, 2, unsigned long)
#define BLINKER_SET_SLEEP _IOR(BLINKER_IOC_MAGIC, 3, unsigned long)

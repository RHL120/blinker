LDDINC=$(PWD)/../include
#uncomment bellow to disable gpio calls
EXTRA_CFLAGS += -std=gnu99 -I$(LDDINC) -DNO_GPIO

ifeq ($(KERNELRELEASE),)

    KERNELDIR ?= /lib/modules/$(shell uname -r)/build
    PWD := $(shell pwd)

all: modules blctl
modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.mod modules.order *.symvers ; rm blctl
blctl: blctl.c
	gcc blctl.c -o blctl

.PHONY: modules modules_install clean

else
    obj-m := blinker.o
endif



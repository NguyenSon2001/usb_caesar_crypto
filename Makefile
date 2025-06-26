obj-m += usb_crypto_driver.o

KERNEL_DIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean

install:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules_install
	depmod -a

load:
	sudo insmod usb_crypto_driver.ko

unload:
	sudo rmmod usb_crypto_driver

reload: unload load

.PHONY: all clean install load unload reload
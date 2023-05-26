PROJ=usbTestDriver
obj-m := $(PROJ).o

KERNEL_LOCATION=/lib/modules/$(shell uname -r)
BUILDDIR=$(KERNEL_LOCATION)/build

all:
	make -C $(BUILDDIR) M=$(shell pwd) modules

install:
	sudo make -C $(BUILDDIR) M=$(shell pwd) modules_install
	# Rebuild the kernel module dependencies for modprobe
	sudo depmod -a
	#sudo cp -f $(PROJ).ko /lib/modules/$(shell uname -r)/kernel/drivers/char/
	sudo modprobe $(PROJ)	

uninstall:
	#sudo rm  /lib/modules/$(shell uname -r)/kernel/drivers/char/$(PROJ).ko
	sudo modprobe -r $(PROJ)
	sudo rm $(KERNEL_LOCATION)/extra/$(PROJ).ko.gz
	# Rebuild the kernel module dependencies for modprobe
	sudo depmod -a
	sudo rmmod $(PROJ)
	
test:
	make install; echo
	ls -l /dev/$(PROJ)*
	make uninstall

clean:
	make -C $(BUILDDIR) M=$(PWD) clean
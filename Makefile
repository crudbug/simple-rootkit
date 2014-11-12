obj-m		:= simple-rootkit.o

KBUILD_DIR	:= /lib/modules/$(shell uname -r)/build

default:
	$(MAKE) -C $(KBUILD_DIR) M=$(shell pwd)
clean:
	$(MAKE) -C $(KBUILD_DIR) M=$(shell pwd) clean

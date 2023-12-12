ifneq ($(KERNELRELEASE),)

   obj-m += ap1302.o

# uncomment to activate debug output
# CFLAGS_ap1302.o = -DDEBUG

else
# normal makefile
SRC := $(shell pwd)


all:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC)

modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(SRC) modules_install

clean:
	rm -f *.o *~ core .depend .*.cmd *.ko *.mod.c
	rm -f Module.markers Module.symvers modules.order
	rm -rf .tmp_versions Modules.s

endif

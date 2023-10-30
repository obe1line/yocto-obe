ifneq ($(KERNELRELEASE),)

   obj-m += ar1335-camera-driver.o

# uncomment to activate debug output
CFLAGS_ar1335-camera-driver.o = -DDEBUG

else
# normal makefile
SRC := $(shell pwd)
KERNEL_SRC := "~/linux-6.4.16/"
ARCH=arm64
CROSS_COMPILE=aarch64-linux-gnu-

CFLAGS=-I~/linux-6.4.26/include

all:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC) M=$(SRC)

modules_install:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_SRC) M=$(SRC) modules_install

clean:
	rm -f *.o *~ core .depend .*.cmd *.ko *.mod.c
	rm -f Module.markers Module.symvers modules.order
	rm -rf .tmp_versions Modules.s

endif

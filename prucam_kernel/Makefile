ifneq ($(KERNELRELEASE),)
	obj-m := pru_camera.o rpmsg_prucam.o
else
	PWD := $(shell pwd)

KERNELDIR=/home/cjiang/workspace/rcn-ee/ti-linux-kernel-dev/KERNEL
ARCH=arm
CROSS_CC=/home/cjiang/opt/gcc-linaro-4.9-2016.02-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-

all:
	$(MAKE) -C $(KERNELDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_CC) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNELDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_CC) M=$(PWD) clean

endif



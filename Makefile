CROSS_COMPILE ?= /home/shahak/arm_linux/buildroot-2021.11/output/host/bin/arm-buildroot-linux-uclibcgnueabi-
ARCH ?= arm

LINUX_DIR='/home/shahak/arm_linux/buildroot-2021.11/output/build/linux-5.15'
obj-m += a.o

all:
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(LINUX_DIR) M=$(PWD) modules
clean:
	make -C $(LINUX_DIR) M=$(PWD) clean

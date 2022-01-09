CROSS_COMPILE ?= /home/shahak/arm_linux/buildroot-2021.11/output/host/bin/arm-buildroot-linux-uclibcgnueabi-
ARCH ?= arm
LINUX_DIR ?= /home/shahak/arm_linux/buildroot-2021.11/output/build/linux-5.15

ccflags-y := -std=gnu99 -Wno-declaration-after-statement

obj-m += a.o
a-objs := main.o network_interface.o push_packet_to_interface.o hook_function.o hook_struct.o

all:
	make CFLAGS=$(CFLAGS) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(LINUX_DIR) M=$(PWD) modules
clean:
	make -C $(LINUX_DIR) M=$(PWD) clean

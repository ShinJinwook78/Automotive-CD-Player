#
# Copyright (C) 2011 Telechips, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

KDIR :=../../../kernel

OBJS += \
	$(LOCAL_DIR)cdif_api.o\
	$(LOCAL_DIR)cdif_drv.o\
	$(LOCAL_DIR)knetlink.o\
	$(LOCAL_DIR)cdif_proc.o\
	$(LOCAL_DIR)SingleBuffer.o\
	$(LOCAL_DIR)cdrom.o \
	$(LOCAL_DIR)cdrom_drv.o


LIBS += \
	$(LOCAL_DIR)libc3decoder/libc3decoder.lo

DEFINES += \
	   -D_LINUX_\
	   -D__KERNEL__\
	   -DMODULE

EXTRA_CFLAGS += $(DEFINES)
EXTRA_CFLAGS += -O2 -fno-builtin -finline -fno-common
KBUILD_EXTRA_SYMBOLS := $(ANDROID_BUILD_TOP)/hardware/telechips/tcc_cdrom_dev/Default.symvers

obj-m := tcc_cdrom.o 

tcc_cdrom-objs := $(OBJS) $(LIBS)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	arm-linux-androideabi-strip -g --strip-unneeded tcc_cdrom.ko

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean


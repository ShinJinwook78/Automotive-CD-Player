LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := tcc_cdrom
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .ko
LOCAL_MODULE_PATH := $(PRODUCT_OUT)/system/lib/modules
LOCAL_SRC_FILES := $(LOCAL_MODULE)$(LOCAL_MODULE_SUFFIX)
OVERRIDE_BUILT_MODULE_PATH := $(TARGET_OUT_INTERMEDIATE_LIBRARIES)
include $(BUILD_PREBUILT)

$(LOCAL_PATH)/tcc_cdrom.ko: tcc_cdrom-modules

tcc_cdrom-modules:
	$(MAKE) -C $(TOP)/kernel -f Makefile M=../hardware/telechips/tcc_cdrom modules
	






LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

TARGET = wlan_loader
LOCAL_MODULE = $(TARGET)

ifeq ($(CLI_DEBUG),y)
  CLI_DEBUGFLAGS = -O0 -g -fno-builtin -DDEBUG -D TI_DBG  # "-O" is needed to expand inlines
else
  CLI_DEBUGFLAGS = -O2
endif

LOCAL_SRC_FILES := wlan_loader.c

LOCAL_SHARED_LIBRARIES += libc libcutils libhardware_legacy

LOCAL_CFLAGS = -Wall -Wstrict-prototypes $(CLI_DEBUGFLAGS) -D__LINUX__ $(DK_DEFINES) -mabi=aapcs-linux

LOCAL_C_INCLUDES = $(INCLUDES)

include $(BUILD_EXECUTABLE)

########################

#LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

#TARGET = wlan_mac
LOCAL_MODULE = wlan_mac

ifeq ($(CLI_DEBUG),y)
  CLI_DEBUGFLAGS = -O0 -g -fno-builtin -DDEBUG -D TI_DBG  # "-O" is needed to expand inlines
else
  CLI_DEBUGFLAGS = -O2
endif

LOCAL_SRC_FILES := wlan_mac.c

LOCAL_SHARED_LIBRARIES += libc libcutils libnetutils
LOCAL_CFLAGS = -Wall -Wstrict-prototypes $(CLI_DEBUGFLAGS) -D__LINUX__ $(DK_DEFINES) -mabi=aapcs-linux
LOCAL_C_INCLUDES = $(INCLUDES)

include $(BUILD_EXECUTABLE)

########################

########################


local_target_dir := $(TARGET_OUT)/lib/modules

include $(CLEAR_VARS)
LOCAL_MODULE := wlan.ko
LOCAL_MODULE_TAGS := user
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(local_target_dir)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

########################

########################

local_target_dir := $(TARGET_OUT)/etc

include $(CLEAR_VARS)
LOCAL_MODULE := firmware/sd8686.bin
LOCAL_MODULE_TAGS := user
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(local_target_dir)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

########################

########################

local_target_dir := $(TARGET_OUT)/etc

include $(CLEAR_VARS)
LOCAL_MODULE := firmware/sd8686_helper.bin
LOCAL_MODULE_TAGS := user
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(local_target_dir)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

########################

########################

local_target_dir := $(TARGET_OUT)/etc

include $(CLEAR_VARS)
LOCAL_MODULE := firmware/athtcmd_ram.bin
LOCAL_MODULE_TAGS := user
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(local_target_dir)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

########################

########################

local_target_dir := $(TARGET_OUT)/etc

include $(CLEAR_VARS)
LOCAL_MODULE := firmware/athwlan.bin.z77
LOCAL_MODULE_TAGS := user
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(local_target_dir)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

########################

########################

local_target_dir := $(TARGET_OUT)/etc

include $(CLEAR_VARS)
LOCAL_MODULE := firmware/calData_ar6102_15dBm.bin
LOCAL_MODULE_TAGS := user
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(local_target_dir)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

########################

########################

local_target_dir := $(TARGET_OUT)/etc

include $(CLEAR_VARS)
LOCAL_MODULE := firmware/data.patch.hw2_0.bin
LOCAL_MODULE_TAGS := user
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(local_target_dir)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

########################

########################

local_target_dir := $(TARGET_OUT)/etc

include $(CLEAR_VARS)
LOCAL_MODULE := firmware/device.bin
LOCAL_MODULE_TAGS := user
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(local_target_dir)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

########################

########################

local_target_dir := $(TARGET_OUT)/etc

include $(CLEAR_VARS)
LOCAL_MODULE := firmware/softmac
LOCAL_MODULE_TAGS := user
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(local_target_dir)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

########################

########################

local_target_dir := $(TARGET_OUT)/bin

include $(CLEAR_VARS)
LOCAL_MODULE := iwconfig
LOCAL_MODULE_TAGS := user
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(local_target_dir)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

########################

########################

local_target_dir := $(TARGET_OUT)/bin

include $(CLEAR_VARS)
LOCAL_MODULE := iwlist
LOCAL_MODULE_TAGS := user
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(local_target_dir)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

########################

########################

local_target_dir := $(TARGET_OUT)/etc

include $(CLEAR_VARS)
LOCAL_MODULE := firmware/fw_bcm4329.bin
LOCAL_MODULE_TAGS := user
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(local_target_dir)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

########################

########################

local_target_dir := $(TARGET_OUT)/etc

include $(CLEAR_VARS)
LOCAL_MODULE := firmware/fw_bcm4329_apsta.bin
LOCAL_MODULE_TAGS := user
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(local_target_dir)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

########################

########################

local_target_dir := $(TARGET_OUT)/etc

include $(CLEAR_VARS)
LOCAL_MODULE := firmware/nvram_B23.txt
LOCAL_MODULE_TAGS := user
LOCAL_MODULE_CLASS := ETC
LOCAL_MODULE_PATH := $(local_target_dir)
LOCAL_SRC_FILES := $(LOCAL_MODULE)
include $(BUILD_PREBUILT)

########################


# Generate the libutdemo.so file
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := libUT
LOCAL_SRC_FILES := ../../../../libut.so
ifneq (,$(wildcard $(LOCAL_PATH)/$(LOCAL_SRC_FILES)))
include $(PREBUILT_SHARED_LIBRARY)
endif



include $(CLEAR_VARS)

#
# Module Settings
#
LOCAL_MODULE := utDemo

#
# Source Files
#

LOCAL_SRC_FILES := \
codec_unittest.cpp
#
# Header Includes
#
LOCAL_C_INCLUDES := \
#
# Compile Flags and Link Libraries
#
LOCAL_CFLAGS := -O3 -DANDROID_NDK

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_ARM_MODE := arm
endif

LOCAL_LDLIBS := -llog
LOCAL_SHARED_LIBRARIES := libUT

include $(BUILD_SHARED_LIBRARY)

# Generate the libwelsdecdemo.so file
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := wels
LOCAL_SRC_FILES := ../../../../../libwels.so
include $(PREBUILT_SHARED_LIBRARY)



include $(CLEAR_VARS)

#
# Module Settings
#
LOCAL_MODULE := welsencdemo

#
# Source Files
#
CODEC_PATH := ../../../../
CONSOLE_ENC_PATH := ../../../../console/enc
LOCAL_SRC_FILES := \
            $(CONSOLE_ENC_PATH)/src/welsenc.cpp \
            $(CONSOLE_ENC_PATH)/src/read_config.cpp \
            $(CODEC_PATH)/common/logging.cpp \
            myjni.cpp

#
# Header Includes
#
LOCAL_C_INCLUDES := \
            $(LOCAL_PATH)/../../../../api/svc \
            $(LOCAL_PATH)/../../../../console/enc/inc \
            $(LOCAL_PATH)/../../../../encoder/core/inc \
            $(LOCAL_PATH)/../../../../processing/interface \
            $(LOCAL_PATH)/../../../../common

  
#
# Compile Flags and Link Libraries
#
LOCAL_CFLAGS := -O3 -DANDROID_NDK

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_ARM_MODE := arm
endif

LOCAL_LDLIBS := -llog
LOCAL_SHARED_LIBRARIES := wels

include $(BUILD_SHARED_LIBRARY)

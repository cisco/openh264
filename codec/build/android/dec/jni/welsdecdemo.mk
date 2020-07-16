# Generate the libwelsdecdemo.so file
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

#
# Module Settings
#
LOCAL_MODULE := welsdecdemo

#
# Source Files
#
CODEC_PATH := ../../../..
CONSOLE_DEC_PATH := $(CODEC_PATH)/console/dec
CONSOLE_COMMON_PATH := $(CODEC_PATH)/console/common
LOCAL_SRC_FILES := \
            $(CONSOLE_DEC_PATH)/src/h264dec.cpp \
            $(CONSOLE_COMMON_PATH)/src/read_config.cpp \
            $(CONSOLE_DEC_PATH)/src/d3d9_utils.cpp \
            myjni.cpp
#
# Header Includes
#
LOCAL_C_INCLUDES := \
            $(LOCAL_PATH)/$(CONSOLE_DEC_PATH)/inc \
            $(LOCAL_PATH)/$(CONSOLE_COMMON_PATH)/inc

LOCAL_LDLIBS := -llog
LOCAL_SHARED_LIBRARIES := wels

include $(BUILD_SHARED_LIBRARY)

include $(LOCAL_PATH)/../../wels.mk

# Generate the libwelsencdemo.so file
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

#
# Module Settings
#
LOCAL_MODULE := welsencdemo

#
# Source Files
#
CODEC_PATH := ../../../..
CONSOLE_ENC_PATH := $(CODEC_PATH)/console/enc
CONSOLE_COMMON_PATH := $(CODEC_PATH)/console/common
LOCAL_SRC_FILES := \
            $(CONSOLE_ENC_PATH)/src/welsenc.cpp \
            $(CONSOLE_COMMON_PATH)/src/read_config.cpp \
            myjni.cpp

#
# Header Includes
#
LOCAL_C_INCLUDES := \
            $(LOCAL_PATH)/$(CONSOLE_ENC_PATH)/inc \
            $(LOCAL_PATH)/$(CODEC_PATH)/console/common/inc \
            $(LOCAL_PATH)/$(CODEC_PATH)/encoder/core/inc \
            $(LOCAL_PATH)/$(CODEC_PATH)/processing/interface \

LOCAL_LDLIBS := -llog
LOCAL_SHARED_LIBRARIES := wels

include $(BUILD_SHARED_LIBRARY)

include $(LOCAL_PATH)/../../wels.mk

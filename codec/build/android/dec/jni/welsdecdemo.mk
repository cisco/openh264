# Generate the libwelsdecdemo.so file
LOCAL_PATH := $(call my-dir)

#include $(CLEAR_VARS)
#LOCAL_MODULE := stlport
#LOCAL_SRC_FILES := $(NDK_ROOT)/sources/cxx-stl/stlport/libs/$(TARGET_ARCH_ABI)/libstlport_static.a
#LOCAL_EXPORT_C_INCLUDES := $(NDK_ROOT)/sources/cxx-stl/stlport/stlport
#include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := wels
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../../../../libwels.so
include $(PREBUILT_SHARED_LIBRARY)



include $(CLEAR_VARS)

#
# Module Settings
#
LOCAL_MODULE := welsdecdemo

#
# Source Files
#
CODEC_PATH := ../../../../
CONSOLE_DEC_PATH := ../../../../console/dec
LOCAL_SRC_FILES := \
            $(CONSOLE_DEC_PATH)/src/h264dec.cpp \
            $(CONSOLE_DEC_PATH)/src/read_config.cpp \
            $(CONSOLE_DEC_PATH)/src/d3d9_utils.cpp \
            $(CODEC_PATH)/common/logging.cpp \
            myjni.cpp
#
# Header Includes
#
LOCAL_C_INCLUDES := \
            $(LOCAL_PATH)/../../../../api/svc \
            $(LOCAL_PATH)/../../../../console/dec/inc \
            $(LOCAL_PATH)/../../../../common
#
# Compile Flags and Link Libraries
#
LOCAL_CFLAGS := -O3 -DANDROID_NDK

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
LOCAL_ARM_MODE := arm
endif

#LOCAL_STATIC_LIBRARIES += stlport
LOCAL_LDLIBS := -llog
#LOCAL_LDLIBS += -L../../../ -lwels
LOCAL_SHARED_LIBRARIES := wels

include $(BUILD_SHARED_LIBRARY)

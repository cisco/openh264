# a common .mk that includes libopenh264.so in Android project
LOCAL_PATH := $(call my-dir)/../../..

include $(CLEAR_VARS)
LOCAL_MODULE    := wels
LOCAL_SRC_FILES := libopenh264.so
LOCAL_EXPORT_SHARED_LIBRARIES := cpu-features
LOCAL_EXPORT_CFLAGS := -DANDROID_NDK
LOCAL_EXPORT_C_INCLUDES := \
    $(LOCAL_PATH)/codec/api/svc \
    $(LOCAL_PATH)/codec/common/inc

ifneq (,$(wildcard $(LOCAL_PATH)/$(LOCAL_SRC_FILES)))
include $(PREBUILT_SHARED_LIBRARY)
endif

include $(CLEAR_VARS)
LOCAL_MODULE    := cpu-features
LOCAL_SRC_FILES := cpu_features/android-$(APP_ABI)/ndk_compat/libndk_compat.so
ifneq (,$(wildcard $(LOCAL_PATH)/$(LOCAL_SRC_FILES)))
include $(PREBUILT_SHARED_LIBRARY)
endif
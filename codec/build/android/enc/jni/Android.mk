## yongzxu: refine mk files for android platform

#To fix the bug that Intel NDK can't creat directory
$(shell mkdir -p $(TARGET_OBJS)/cpufeatures)
$(shell mkdir -p $(TARGET_OBJS)/welsdecdemo)

LOCAL_PATH := $(call my-dir)
MY_LOCAL_PATH := $(LOCAL_PATH)

# Step3
#Generate the libwelsdecdemo.so file
include $(LOCAL_PATH)/welsencdemo.mk
LOCAL_PATH := $(MY_LOCAL_PATH)


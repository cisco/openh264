ARCH = arm
include $(SRC_PATH)build/platform-msvc-common.mk
CFLAGS_OPT += -MD
CFLAGS_DEBUG += -MDd
CFLAGS += -DWINAPI_FAMILY=WINAPI_FAMILY_PHONE_APP
CXXFLAGS +=
LDFLAGS +=

codec/common/src/WelsThreadLib.$(OBJ): CXXFLAGS += -ZW


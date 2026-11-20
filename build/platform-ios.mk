ARCH = armv7
include $(SRC_PATH)build/platform-darwin.mk
CXX = clang++
CC = clang
ifneq ($(filter %86 x86_64, $(ARCH)),)
SDKTYPE = iPhoneSimulator
else
SDKTYPE = iPhoneOS
endif
SDK_MIN = 5.1

SDKROOT := $(shell xcrun --sdk $(shell echo $(SDKTYPE) | tr A-Z a-z) --show-sdk-path)
CFLAGS += -arch $(ARCH) -isysroot "$(SDKROOT)" -miphoneos-version-min=$(SDK_MIN) -DAPPLE_IOS -fembed-bitcode
LDFLAGS += -arch $(ARCH) -isysroot "$(SDKROOT)" -miphoneos-version-min=$(SDK_MIN)


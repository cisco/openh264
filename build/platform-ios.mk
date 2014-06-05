#ifeq ($(filter %86 armv7 armv7s arm64, $(ARCH)),)
#$(error Default ARCH is the build platform not ios devices platform,should pass ARCH to make,only support ARCH of %86 armv7 armv7s arm64)
#endif
include build/platform-darwin.mk
CXX = clang++
CC = clang
ifneq ($(filter %86 x86_64, $(ARCH)),)
SDKTYPE = iPhoneSimulator
else
SDKTYPE = iPhoneOS
endif
SDK = $(shell xcrun --sdk $(shell echo $(SDKTYPE) | tr A-Z a-z) --show-sdk-version)
SDK_MIN = 5.1

XCODE=$(shell xcode-select -p)
SDKROOT = $(XCODE)/Platforms/$(SDKTYPE).platform/Developer/SDKs/$(SDKTYPE)$(SDK).sdk
CFLAGS += -arch $(ARCH) -isysroot $(SDKROOT) -miphoneos-version-min=$(SDK_MIN) -DAPPLE_IOS
LDFLAGS += -arch $(ARCH) -isysroot $(SDKROOT) -miphoneos-version-min=$(SDK_MIN)


include build/platform-darwin.mk
CXX = clang++
CC = clang
SDK = 7.0
ifneq ($(filter %86 x86_64, $(ARCH)),)
SDKTYPE = iPhoneSimulator
else
SDKTYPE = iPhoneOS
endif
SDK_MIN = 5.1

SDKROOT = /Applications/Xcode.app/Contents/Developer/Platforms/$(SDKTYPE).platform/Developer/SDKs/$(SDKTYPE)$(SDK).sdk
CFLAGS += -arch $(ARCH) -isysroot $(SDKROOT) -miphoneos-version-min=$(SDK_MIN) -DAPPLE_IOS
LDFLAGS += -arch $(ARCH) -isysroot $(SDKROOT) -miphoneos-version-min=$(SDK_MIN)


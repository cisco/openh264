USE_ASM = No
ARCH = arm
SHAREDLIBSUFFIX = so
GCCVERSION = 4.8
NDKLEVEL = 12
HOSTOS = $(shell uname | tr A-Z a-z | tr -d \\-[:digit:].)
HOSTARCH = $(shell uname -m)
ifeq ($(ARCH), arm)
    GCCPATHPREFIX = arm-linux-androideabi
    GCCPREFIX = arm-linux-androideabi
    CFLAGS += -march=armv7-a -mfloat-abi=softfp
ifeq (Yes, $(HAVE_NEON))
    CFLAGS += -mfpu=neon
endif
    LDFLAGS += -march=armv7-a -Wl,--fix-cortex-a8
    APP_ABI = armeabi-v7a
else
    GCCPATHPREFIX = x86
    GCCPREFIX = i686-linux-android
    APP_ABI = x86
ifeq (Yes, $(USE_ASM))
    ASM = nasm
    CFLAGS += -DX86_ASM
    ASMFLAGS += -DNOPREFIX -f elf32 -DX86_32
endif
endif

ifndef NDKROOT
$(error NDKROOT is not set)
endif
ifndef TARGET
$(error TARGET is not set)
endif

SYSROOT = $(NDKROOT)/platforms/android-$(NDKLEVEL)/arch-$(ARCH)
CXX = $(NDKROOT)/toolchains/$(GCCPATHPREFIX)-$(GCCVERSION)/prebuilt/$(HOSTOS)-$(HOSTARCH)/bin/$(GCCPREFIX)-g++
CFLAGS += -DLINUX -fpic --sysroot=$(SYSROOT) -fno-rtti -fno-exceptions
LDFLAGS += --sysroot=$(SYSROOT) -Wl,--no-undefined -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now -Wl,-soname,libwels.so


binaries : decdemo encdemo

decdemo: libraries
	sh -c 'cd ./codec/build/android/dec/jni && $(NDKROOT)/ndk-build -B APP_ABI=$(APP_ABI) && cd .. && android update project -t $(TARGET) -p . && ant debug && cd ../../../..'

encdemo: libraries
	sh -c 'cd ./codec/build/android/enc/jni && $(NDKROOT)/ndk-build -B APP_ABI=$(APP_ABI) && cd .. && android update project -t $(TARGET) -p . && ant debug && cd ../../../..'

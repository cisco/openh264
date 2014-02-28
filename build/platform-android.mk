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
  else
    CFLAGS += -mfpu=vfpv3-d16
  endif
    LDFLAGS += -march=armv7-a -Wl,--fix-cortex-a8
    APP_ABI = armeabi-v7a
else
    GCCPATHPREFIX = x86
    GCCPREFIX = i686-linux-android
    APP_ABI = x86
  ifeq (Yes, $(USE_ASM))
    ASM_ARCH = x86
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
CC = $(NDKROOT)/toolchains/$(GCCPATHPREFIX)-$(GCCVERSION)/prebuilt/$(HOSTOS)-$(HOSTARCH)/bin/$(GCCPREFIX)-gcc
AR = $(NDKROOT)/toolchains/$(GCCPATHPREFIX)-$(GCCVERSION)/prebuilt/$(HOSTOS)-$(HOSTARCH)/bin/$(GCCPREFIX)-ar
CFLAGS += -DLINUX -fpic --sysroot=$(SYSROOT)
CXXFLAGS += -fno-rtti -fno-exceptions
LDFLAGS += --sysroot=$(SYSROOT)
SHLDFLAGS = -Wl,--no-undefined -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now -Wl,-soname,libwels.so

STL_INCLUDES = \
    -I$(NDKROOT)/sources/cxx-stl/gnu-libstdc++/$(GCCVERSION)/include \
    -I$(NDKROOT)/sources/cxx-stl/gnu-libstdc++/$(GCCVERSION)/libs/$(APP_ABI)/include

GTEST_INCLUDES = $(STL_INCLUDES)
CODEC_UNITTEST_INCLUDES = $(STL_INCLUDES)
CODEC_UNITTEST_LDFLAGS_SUFFIX = \
    $(NDKROOT)/sources/cxx-stl/gnu-libstdc++/$(GCCVERSION)/libs/$(APP_ABI)/libgnustl_static.a

binaries : decdemo encdemo

decdemo: libraries
	cd ./codec/build/android/dec && $(NDKROOT)/ndk-build -B APP_ABI=$(APP_ABI) && android update project -t $(TARGET) -p . && ant debug

encdemo: libraries
	cd ./codec/build/android/enc && $(NDKROOT)/ndk-build -B APP_ABI=$(APP_ABI) && android update project -t $(TARGET) -p . && ant debug

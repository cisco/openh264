ARCH = arm
include build/platform-arch.mk
SHAREDLIBSUFFIX = so
NDKLEVEL = 12
ifeq ($(ARCH), arm)
  ifneq ($(APP_ABI), armeabi)
    CFLAGS += -march=armv7-a -mfloat-abi=softfp
    CFLAGS += -mfpu=vfpv3-d16
    LDFLAGS += -march=armv7-a -Wl,--fix-cortex-a8
    APP_ABI = armeabi-v7a
  endif
    TOOLCHAINPREFIX = $(shell NDK_PROJECT_PATH=./codec/build/android/dec make --no-print-dir -f $(NDKROOT)/build/core/build-local.mk DUMP_TOOLCHAIN_PREFIX APP_ABI=armeabi)
  ifeq (Yes, $(USE_ASM))
    ASMFLAGS += -march=armv7-a -mfpu=neon
  endif
else ifeq ($(ARCH), x86)
    APP_ABI = x86
    TOOLCHAINPREFIX = $(shell NDK_PROJECT_PATH=./codec/build/android/dec make --no-print-dir -f $(NDKROOT)/build/core/build-local.mk DUMP_TOOLCHAIN_PREFIX APP_ABI=x86)
  ifeq (Yes, $(USE_ASM))
    ASMFLAGS += -f elf32
  endif
else
    APP_ABI = $(ARCH)
    TOOLCHAINPREFIX = $(shell NDK_PROJECT_PATH=./codec/build/android/dec make --no-print-dir -f $(NDKROOT)/build/core/build-local.mk DUMP_TOOLCHAIN_PREFIX APP_ABI=$(APP_ABI))
endif

ifndef NDKROOT
$(error NDKROOT is not set)
endif
ifndef TARGET
$(error TARGET is not set)
endif

SYSROOT = $(NDKROOT)/platforms/android-$(NDKLEVEL)/arch-$(ARCH)
CXX = $(TOOLCHAINPREFIX)g++
CC = $(TOOLCHAINPREFIX)gcc
AR = $(TOOLCHAINPREFIX)ar
CFLAGS += -DANDROID_NDK -fpic --sysroot=$(SYSROOT)
CXXFLAGS += -fno-rtti -fno-exceptions
LDFLAGS += --sysroot=$(SYSROOT)
SHLDFLAGS = -Wl,--no-undefined -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now -Wl,-soname,lib$(PROJECT_NAME).so

STL_INCLUDES = \
    -I$(NDKROOT)/sources/cxx-stl/stlport/stlport

GTEST_INCLUDES = $(STL_INCLUDES)
CODEC_UNITTEST_INCLUDES = $(STL_INCLUDES)
CODEC_UNITTEST_LDFLAGS_SUFFIX = \
    $(NDKROOT)/sources/cxx-stl/stlport/libs/$(APP_ABI)/libstlport_static.a

binaries : decdemo encdemo

decdemo: libraries
	cd ./codec/build/android/dec && $(NDKROOT)/ndk-build -B APP_ABI=$(APP_ABI) && android update project -t $(TARGET) -p . && ant debug

encdemo: libraries
	cd ./codec/build/android/enc && $(NDKROOT)/ndk-build -B APP_ABI=$(APP_ABI) && android update project -t $(TARGET) -p . && ant debug

COMMON_INCLUDES += -I$(NDKROOT)/sources/android/cpufeatures
COMMON_OBJS += $(COMMON_SRCDIR)/cpu-features.$(OBJ)

COMMON_CFLAGS += \
	-Dandroid_getCpuIdArm=wels_getCpuIdArm -Dandroid_setCpuArm=wels_setCpuArm \
	-Dandroid_getCpuCount=wels_getCpuCount -Dandroid_getCpuFamily=wels_getCpuFamily \
	-Dandroid_getCpuFeatures=wels_getCpuFeatures -Dandroid_setCpu=wels_setCpu \

codec/common/cpu-features.$(OBJ): $(NDKROOT)/sources/android/cpufeatures/cpu-features.c
	$(QUIET_CC)$(CC) $(CFLAGS) $(INCLUDES) $(COMMON_CFLAGS) $(COMMON_INCLUDES) -c $(CXX_O) $<

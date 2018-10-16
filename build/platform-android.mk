ARCH = arm
include $(SRC_PATH)build/arch.mk
SHAREDLIBSUFFIX = so
# Android APK/JARs expect libraries to be unversioned
SHAREDLIBSUFFIXFULLVER=$(SHAREDLIBSUFFIX)
SHAREDLIBSUFFIXMAJORVER=$(SHAREDLIBSUFFIX)
SHLDFLAGS =
NDKLEVEL = 12
ifeq ($(ARCH), arm)
  ifneq ($(APP_ABI), armeabi)
    CFLAGS += -march=armv7-a -mfloat-abi=softfp
    CFLAGS += -mfpu=vfpv3-d16
    LDFLAGS += -march=armv7-a -Wl,--fix-cortex-a8
    APP_ABI = armeabi-v7a
  endif
else ifeq ($(ARCH), arm64)
  APP_ABI = arm64-v8a
else ifeq ($(ARCH), x86)
  APP_ABI = x86
  ifeq (Yes, $(USE_ASM))
    ASMFLAGS += -f elf
  endif
else ifeq ($(ARCH), x86_64)
  APP_ABI = x86_64
  ifeq (Yes, $(USE_ASM))
    ASMFLAGS += -f elf64
  endif
else
  APP_ABI = $(ARCH)
endif

ifndef NDKROOT
$(error NDKROOT is not set)
endif
ifndef TARGET
$(error TARGET is not set)
endif

TOOLCHAINPREFIX = $(shell NDK_PROJECT_PATH=$(SRC_PATH)/codec/build/android/dec make --no-print-dir -f $(NDKROOT)/build/core/build-local.mk DUMP_TOOLCHAIN_PREFIX APP_ABI=$(APP_ABI))
TOOLCHAIN_NAME = $(shell NDK_TOOLCHAIN_VERSION= NDK_PROJECT_PATH=$(SRC_PATH)/codec/build/android/dec make --no-print-dir -f $(NDKROOT)/build/core/build-local.mk DUMP_TOOLCHAIN_NAME APP_ABI=$(APP_ABI))
GCC_TOOLCHAIN_PATH = $(shell dirname $(TOOLCHAINPREFIX) | xargs dirname )

SYSROOT = $(NDKROOT)/platforms/android-$(NDKLEVEL)/arch-$(ARCH)
CXX = $(TOOLCHAINPREFIX)g++
CC = $(TOOLCHAINPREFIX)gcc
AR = $(TOOLCHAINPREFIX)ar
CFLAGS += -DANDROID_NDK -fpic --sysroot=$(SYSROOT) -MMD -MP
CFLAGS += -isystem $(NDKROOT)/sysroot/usr/include -isystem $(NDKROOT)/sysroot/usr/include/$(TOOLCHAIN_NAME) -D__ANDROID_API__=$(NDKLEVEL)
CXXFLAGS += -fno-rtti -fno-exceptions
LDFLAGS += --sysroot=$(SYSROOT)
SHLDFLAGS = -Wl,--no-undefined -Wl,-z,relro -Wl,-z,now -Wl,-soname,lib$(PROJECT_NAME).so

ifeq ($(NDK_TOOLCHAIN_VERSION), clang)
  HOST_OS = $(shell uname -s | tr [A-Z] [a-z])
  LLVM_INSTALL_DIR = $(NDKROOT)/toolchains/llvm/prebuilt/$(HOST_OS)-x86_64/bin
  CC = $(LLVM_INSTALL_DIR)/clang
  CXX = $(LLVM_INSTALL_DIR)/clang++

  ifeq ($(ARCH), arm)
    TARGET_NAME = armv7-none-linux-androideabi
  else ifeq ($(ARCH), arm64)
    TARGET_NAME = aarch64-none-linux-android
  else ifeq ($(ARCH), x86)
    TARGET_NAME = i686-none-linux-android
  else ifeq ($(ARCH), x86_64)
    TARGET_NAME = x86_64-none-linux-android
  else
    $(error "does not support this arch now!")
  endif

  CFLAGS += -target $(TARGET_NAME)
  LDFLAGS += -target $(TARGET_NAME) -gcc-toolchain $(GCC_TOOLCHAIN_PATH)
  LDFLAGS += -Wl,--exclude-libs,libgcc.a -Wl,--exclude-libs,libunwind.a
endif

ifneq ($(CXX),$(wildcard $(CXX)))
ifneq ($(CXX).exe,$(wildcard $(CXX).exe))
$(error Compiler not found, bad NDKROOT or ARCH?)
endif
endif

STL_INCLUDES = \
    -I$(NDKROOT)/sources/cxx-stl/stlport/stlport
STL_LIB = \
    $(NDKROOT)/sources/cxx-stl/stlport/libs/$(APP_ABI)/libstlport_static.a

GTEST_INCLUDES = $(STL_INCLUDES)
CODEC_UNITTEST_INCLUDES = $(STL_INCLUDES)
CODEC_UNITTEST_LDFLAGS_SUFFIX = $(STL_LIB)
MODULE_INCLUDES = $(STL_INCLUDES)
MODULE_LDFLAGS = $(STL_LIB)

ifeq (./,$(SRC_PATH))
binaries: decdemo encdemo

decdemo: libraries
	cd ./codec/build/android/dec && $(NDKROOT)/ndk-build -B NDK_TOOLCHAIN_VERSION=$(NDK_TOOLCHAIN_VERSION) APP_ABI=$(APP_ABI) APP_PLATFORM=$(TARGET) && android update project -t $(TARGET) -p . && ant debug

encdemo: libraries
	cd ./codec/build/android/enc && $(NDKROOT)/ndk-build -B NDK_TOOLCHAIN_VERSION=$(NDK_TOOLCHAIN_VERSION) APP_ABI=$(APP_ABI) APP_PLATFORM=$(TARGET) && android update project -t $(TARGET) -p . && ant debug

clean_Android: clean_Android_dec clean_Android_enc

clean_Android_dec:
	-cd ./codec/build/android/dec && $(NDKROOT)/ndk-build APP_ABI=$(APP_ABI) clean && ant clean
clean_Android_enc:
	-cd ./codec/build/android/enc && $(NDKROOT)/ndk-build APP_ABI=$(APP_ABI) clean && ant clean
else
clean_Android:
	@:
endif

COMMON_INCLUDES += -I$(NDKROOT)/sources/android/cpufeatures
COMMON_OBJS += $(COMMON_SRCDIR)/src/cpu-features.$(OBJ)

COMMON_CFLAGS += \
	-Dandroid_getCpuIdArm=wels_getCpuIdArm -Dandroid_setCpuArm=wels_setCpuArm \
	-Dandroid_getCpuCount=wels_getCpuCount -Dandroid_getCpuFamily=wels_getCpuFamily \
	-Dandroid_getCpuFeatures=wels_getCpuFeatures -Dandroid_setCpu=wels_setCpu \

codec/common/src/cpu-features.$(OBJ): $(NDKROOT)/sources/android/cpufeatures/cpu-features.c
	$(QUIET_CC)$(CC) $(CFLAGS) $(INCLUDES) $(COMMON_CFLAGS) $(COMMON_INCLUDES) -c $(CXX_O) $<

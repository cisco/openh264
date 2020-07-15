ARCH = arm
include $(SRC_PATH)build/arch.mk
SHAREDLIBSUFFIX = so
# Android APK/JARs expect libraries to be unversioned
SHAREDLIBSUFFIXFULLVER=$(SHAREDLIBSUFFIX)
SHAREDLIBSUFFIXMAJORVER=$(SHAREDLIBSUFFIX)
SHLDFLAGS =
ifdef ANDROID_NDK_ROOT
  NDKROOT = $(ANDROID_NDK_ROOT)
endif
ANDROID_SDK_TOOLS = $(ANDROID_HOME)/tools/
NDK_TOOLCHAIN_VERSION = clang
NDKLEVEL = $(TARGET:android-%=%)
SUPPORTED_NDK_RELEASES = 20 21
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
include $(NDKROOT)/source.properties
NDK_RELEASE = $(basename $(basename $(Pkg.Revision)))
ifeq (, $(filter $(NDK_RELEASE), $(SUPPORTED_NDK_RELEASES)))
$(error unsupported NDK release $(Pkg.Revision))
endif
ifndef TARGET
$(error TARGET is not set)
endif

CFLAGS += -DANDROID_NDK -fpic --sysroot=$(SYSROOT) -MMD -MP -fstack-protector-all
CXXFLAGS += -fno-rtti -fno-exceptions
LDFLAGS += --sysroot=$(SYSROOT)
SHLDFLAGS = -Wl,--no-undefined -Wl,-z,relro -Wl,-z,now -Wl,-soname,lib$(PROJECT_NAME).so
UTSHLDFLAGS = -Wl,-soname,libut.so

ifeq ($(NDK_TOOLCHAIN_VERSION), clang)
  LLVM_INSTALL_DIR = $(wildcard $(NDKROOT)/toolchains/llvm/prebuilt/*/bin)

  ifeq ($(ARCH), arm)
    TARGET_NAME = armv7a-linux-androideabi
  else ifeq ($(ARCH), arm64)
    TARGET_NAME = aarch64-linux-android
  else ifeq ($(ARCH), x86)
    TARGET_NAME = i686-linux-android
  else ifeq ($(ARCH), x86_64)
    TARGET_NAME = x86_64-linux-android
  else
    $(error "does not support this arch now!")
  endif

  CC = $(LLVM_INSTALL_DIR)/$(TARGET_NAME)$(NDKLEVEL)-clang
  CXX = $(LLVM_INSTALL_DIR)/$(TARGET_NAME)$(NDKLEVEL)-clang++
  AR = $(LLVM_INSTALL_DIR)/llvm-ar
  SYSROOT = $(LLVM_INSTALL_DIR)/../sysroot
endif

# background reading: https://android.googlesource.com/platform/ndk/+/master/docs/BuildSystemMaintainers.md#unwinding
LDFLAGS += -Wl,--exclude-libs,libgcc.a -Wl,--exclude-libs,libunwind.a

ifneq ($(findstring /,$(CXX)),$(findstring \,$(CXX)))
ifneq ($(CXX),$(wildcard $(CXX)))
ifneq ($(CXX).exe,$(wildcard $(CXX).exe))
$(error Compiler not found, bad NDKROOT or ARCH? $(CXX))
endif
endif
endif

GTEST_INCLUDES = $(STL_INCLUDES)
CODEC_UNITTEST_INCLUDES = $(STL_INCLUDES)
CODEC_UNITTEST_LDFLAGS_SUFFIX = $(STL_LIB)
MODULE_INCLUDES = $(STL_INCLUDES)
MODULE_LDFLAGS = $(STL_LIB)

ifeq (./,$(SRC_PATH))
binaries: decdemo encdemo

NDK_BUILD = $(NDKROOT)/ndk-build APP_ABI=$(APP_ABI) APP_PLATFORM=$(TARGET) NDK_TOOLCHAIN_VERSION=$(NDK_TOOLCHAIN_VERSION) V=$(V:Yes=1)

decdemo: libraries
	$(NDK_BUILD) -C codec/build/android/dec -B
	./gradlew test-dec:assembleDebug

encdemo: libraries
	$(NDK_BUILD) -C codec/build/android/enc -B
	./gradlew test-enc:assembleDebug

clean_Android: clean_Android_dec clean_Android_enc

clean_Android_dec:
	-$(NDK_BUILD) -C codec/build/android/dec clean
	-./gradlew test-dec:clean

clean_Android_enc:
	-$(NDK_BUILD) -C codec/build/android/enc clean
	-./gradlew test-enc:clean

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

include $(SRC_PATH)build/arch.mk
SHAREDLIBSUFFIX = so
SHAREDLIBSUFFIXFULLVER=$(SHAREDLIBSUFFIX).$(FULL_VERSION)
SHAREDLIBSUFFIXMAJORVER=$(SHAREDLIBSUFFIX).$(SHAREDLIB_MAJORVERSION)
SHLDFLAGS = -Wl,-soname,$(LIBPREFIX)$(PROJECT_NAME).$(SHAREDLIBSUFFIXMAJORVER)
CFLAGS += -Wall -fno-strict-aliasing -fPIC -MMD -MP
ifeq ($(USE_STACK_PROTECTOR), Yes)
CFLAGS += -fstack-protector-all
endif
LDFLAGS += -lpthread
STATIC_LDFLAGS += -lpthread -lm
AR_OPTS = crD $@
ifeq ($(ASM_ARCH), x86)
ifeq ($(ARCH), x86_64)
ASMFLAGS += -f elf64
else ifeq ($(ARCH), x32)
ASMFLAGS += -f elfx32
else
ASMFLAGS += -f elf
endif
endif
ifeq ($(ASM_ARCH), arm)
ASMFLAGS += -march=armv7-a -mfpu=neon
endif

ifeq ($(ASM_ARCH), arm64)
CFLAGS += -march=armv8-a
ASMFLAGS += -march=armv8-a
endif

ifneq ($(filter %clang++,$(CXX)),)
CXXFLAGS += -Wc++11-compat-reserved-user-defined-literal
endif

ifneq ($(filter %g++,$(CXX)),)
ifeq ($(filter %clang++,$(CXX)),)
GCCVER_GTEQ8 = $(shell echo $$(($$($(CXX) -dumpversion | awk -F "." '{print $$1}') >= 8)))
ifeq ($(GCCVER_GTEQ8), 1)
CXXFLAGS += -Wno-class-memaccess
endif
endif
endif

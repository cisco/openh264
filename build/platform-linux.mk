include $(SRC_PATH)build/arch.mk
SHAREDLIBSUFFIX = so
SHAREDLIBSUFFIXFULLVER=$(SHAREDLIBSUFFIX).$(FULL_VERSION)
SHAREDLIBSUFFIXMAJORVER=$(SHAREDLIBSUFFIX).$(SHAREDLIB_MAJORVERSION)
SHLDFLAGS = -Wl,-soname,$(LIBPREFIX)$(PROJECT_NAME).$(SHAREDLIBSUFFIXMAJORVER)
CFLAGS += -Wall -fno-strict-aliasing -fPIC -MMD -MP
LDFLAGS += -lpthread
AR_OPTS = crD $@
ifeq ($(ASM_ARCH), x86)
ifeq ($(ARCH), x86_64)
ASMFLAGS += -f elf64
else
ASMFLAGS += -f elf
endif
endif
ifeq ($(ASM_ARCH), arm)
ASMFLAGS += -march=armv7-a -mfpu=neon
endif

ifeq ($(CXX), clang++)
CXXFLAGS += -Wc++11-compat-reserved-user-defined-literal
endif


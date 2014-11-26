include $(SRC_PATH)build/arch.mk
SHAREDLIB_DIR = $(PREFIX)/lib
SHAREDLIBSUFFIX = so
SHAREDLIBSUFFIXVER=$(SHAREDLIBSUFFIX).$(SHAREDLIBVERSION)
SHLDFLAGS = -Wl,-soname,$(LIBPREFIX)$(PROJECT_NAME).$(SHAREDLIBSUFFIXVER)
CFLAGS += -Wall -fno-strict-aliasing -fPIC -MMD -MP
LDFLAGS += -lpthread
ifeq ($(ASM_ARCH), x86)
ifeq ($(ENABLE64BIT), Yes)
ASMFLAGS += -f elf64
else
ASMFLAGS += -f elf
endif
endif
ifeq ($(ASM_ARCH), arm)
ASMFLAGS += -march=armv7-a -mfpu=neon
endif


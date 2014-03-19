include build/platform-arch.mk
SHAREDLIBSUFFIX = so
CFLAGS += -Wall -fno-strict-aliasing -Wno-sign-compare -Wno-missing-braces -fPIC -DLINUX -DMT_ENABLED -MMD -MP
LDFLAGS += -lpthread
ifeq ($(ASM_ARCH), x86)
ifeq ($(ENABLE64BIT), Yes)
ASMFLAGS += -f elf64
else
ASMFLAGS += -f elf32
endif
endif
ifeq ($(ASM_ARCH), arm)
ASMFLAGS += -march=armv7-a -mfpu=neon
endif


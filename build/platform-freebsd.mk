include build/platform-x86-common.mk
ASM = nasm
SHAREDLIBSUFFIX = so
CFLAGS += -fPIC -DMT_ENABLED
LDFLAGS += -lpthread
ASMFLAGS += -DNOPREFIX
ifeq ($(ENABLE64BIT), Yes)
ASMFLAGS += -f elf64
else
ASMFLAGS += -f elf
endif


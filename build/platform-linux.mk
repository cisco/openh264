include build/platform-arch.mk
ASM = nasm
SHAREDLIBSUFFIX = so
CFLAGS += -Werror -fPIC -DLINUX -DMT_ENABLED -MMD -MP
LDFLAGS += -lpthread
ASMFLAGS += -DNOPREFIX
ifeq ($(ENABLE64BIT), Yes)
ASMFLAGS += -f elf64
else
ASMFLAGS += -f elf32
endif


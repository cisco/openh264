ASM = nasm
CFLAGS += -Werror -fPIC -DLINUX -DMT_ENABLED
LDFLAGS += -lpthread
ASMFLAGS += -DNOPREFIX
ifeq ($(ENABLE64BIT), Yes)
ASMFLAGS += -f elf64
else
ASMFLAGS += -f elf32
endif


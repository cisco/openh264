
ASM = nasm
CFLAGS += -Werror -fPIC
LDFLAGS += -lpthread
ASMFLAGS += --prefix _ -DNOPREFIX
ifeq ($(ENABLE64BIT), Yes)
ASMFLAGS += -f macho64
else
ASMFLAGS += -f macho
endif


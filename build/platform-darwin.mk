
ASM = nasm
CFLAGS += -Werror -fPIC
LDFLAGS += -lpthread
<<<<<<< HEAD
ASMFLAGS += --prefix _
=======
ASMFLAGS += --prefix _ -DNOPREFIX
>>>>>>> 64bits_Support
ifeq ($(ENABLE64BIT), Yes)
ASMFLAGS += -f macho64
else
ASMFLAGS += -f macho
endif


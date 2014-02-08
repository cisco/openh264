
ASM = nasm
CFLAGS += -Wno-deprecated-declarations -Werror -fPIC -DMACOS -DMT_ENABLED -MMD -MP
LDFLAGS += -lpthread
ASMFLAGS += --prefix _ -DNOPREFIX
ifeq ($(ENABLE64BIT), Yes)
ASMFLAGS += -f macho64
else
ASMFLAGS += -f macho
endif


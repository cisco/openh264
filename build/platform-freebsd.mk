include $(SRC_PATH)build/platform-arch.mk
SHAREDLIBSUFFIX = so
CFLAGS += -fPIC
LDFLAGS += -lpthread
ifeq ($(ASM_ARCH), x86)
ifeq ($(ENABLE64BIT), Yes)
ASMFLAGS += -f elf64
else
ASMFLAGS += -f elf
endif
endif


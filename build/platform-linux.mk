ASM = nasm
CFLAGS += -m32 -fPIC -DLINUX -D__NO_CTYPE
LDFLAGS += -m32 -ldl -lpthread
ASMFLAGS += -f elf


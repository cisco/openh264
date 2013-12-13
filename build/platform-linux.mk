ASM = nasm
CFLAGS +=  -fPIC -DLINUX -D__NO_CTYPE
LDFLAGS +=  -ldl -lpthread
ASMFLAGS += -f elf


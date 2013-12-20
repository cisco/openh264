ASM = nasm
CFLAGS += -Werror -fPIC -DLINUX -D__NO_CTYPE
LDFLAGS += -lpthread
ASMFLAGS += -f elf


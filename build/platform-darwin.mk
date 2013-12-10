USE_ASM = No  # We don't have ASM working on Mac yet
ASM = nasm
CFLAGS += -arch i386 -fPIC
LDFLAGS += -arch i386 -ldl -lpthread
ASMFLAGS += -f macho --prefix _

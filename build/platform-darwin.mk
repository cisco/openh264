USE_ASM = No  # We don't have ASM working on Mac yet
ASM = nasm
CFLAGS += -fPIC
LDFLAGS += -lpthread
ASMFLAGS += -f macho --prefix _

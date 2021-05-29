include $(SRC_PATH)build/arch.mk
SHAREDLIB_DIR = $(PREFIX)/bin
SHAREDLIBSUFFIX = dll
SHAREDLIBSUFFIXFULLVER=$(SHAREDLIBSUFFIX)
SHAREDLIBSUFFIXMAJORVER=$(SHAREDLIBSUFFIX)
EXTRA_LIBRARY=$(LIBPREFIX)$(PROJECT_NAME).dll.a
SHLDFLAGS = -Wl,--out-implib,$(EXTRA_LIBRARY)
CFLAGS += -MMD -MP
LDFLAGS +=
ifeq ($(ASM_ARCH), x86)
ifeq ($(ARCH), x86_64)
ASMFLAGS += -f win64
ASMFLAGS_PLATFORM = -DWIN64
CC = x86_64-w64-mingw32-gcc
CXX = x86_64-w64-mingw32-g++
AR = x86_64-w64-mingw32-ar
else
ASMFLAGS += -f win32 -DPREFIX
endif
endif
ifeq ($(ASM_ARCH), arm)
CCAS = gas-preprocessor.pl -as-type clang -force-thumb -- $(CC)
CCASFLAGS = -DHAVE_NEON -mimplicit-it=always
endif
EXEEXT = .exe


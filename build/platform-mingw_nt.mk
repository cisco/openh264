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
else
ASMFLAGS += -f win32 -DPREFIX
endif
endif
ifeq ($(ASM_ARCH), arm)
CCAS = gas-preprocessor.pl -as-type clang -force-thumb -- $(CC)
CCASFLAGS = -mimplicit-it=always
ifeq ($(USE_NEON), Yes)
CCASFLAGS += -DHAVE_NEON
endif
endif
EXEEXT = .exe


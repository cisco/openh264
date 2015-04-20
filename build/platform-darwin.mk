include $(SRC_PATH)build/arch.mk
SHAREDLIB_DIR = $(PREFIX)/lib
SHAREDLIBSUFFIX = dylib
SHAREDLIBSUFFIXVER=$(SHAREDLIBVERSION).$(SHAREDLIBSUFFIX)
SHLDFLAGS = -dynamiclib -twolevel_namespace -undefined dynamic_lookup \
	-fno-common -headerpad_max_install_names -install_name \
	$(SHAREDLIB_DIR)/$(LIBPREFIX)$(PROJECT_NAME).$(SHAREDLIBSUFFIXVER)
SHARED = -dynamiclib
CFLAGS += -Wall -fPIC -MMD -MP
LDFLAGS += -lpthread
ifeq ($(ASM_ARCH), x86)
ASMFLAGS += -DPREFIX
ifeq ($(ARCH), x86_64)
ASMFLAGS += -f macho64
else
ASMFLAGS += -f macho
LDFLAGS += -read_only_relocs suppress
endif
endif


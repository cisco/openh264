include $(SRC_PATH)build/msvc-common.mk
LDFLAGS += user32.lib
CFLAGS_OPT += -MT
CFLAGS_DEBUG += -MTd -Gm

# Make sure a plain "make" without a target still builds "all"
all:

$(LIBPREFIX)$(PROJECT_NAME).$(SHAREDLIBSUFFIXVER): openh264.res

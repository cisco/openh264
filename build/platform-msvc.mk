include $(SRC_PATH)build/msvc-common.mk
LDFLAGS += user32.lib
CFLAGS_OPT += -MT
CFLAGS_DEBUG += -MTd -Gm

$(LIBPREFIX)$(PROJECT_NAME).$(SHAREDLIBSUFFIXFULLVER): openh264.res

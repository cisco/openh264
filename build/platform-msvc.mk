include build/platform-msvc-common.mk
LDFLAGS += user32.lib
CFLAGS_OPT += -MT
CFLAGS_DEBUG += -MTd -Gm


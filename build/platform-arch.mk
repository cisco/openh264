ifneq ($(filter %86 x86_64, $(ARCH)),)
include build/platform-x86-common.mk
endif

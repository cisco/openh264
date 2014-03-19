ifneq ($(filter %86 x86_64, $(ARCH)),)
include build/platform-x86-common.mk
endif
ifneq ($(filter-out arm64, $(filter arm%, $(ARCH))),)
ifeq ($(USE_ASM), Yes)
ASM_ARCH = arm
ASMFLAGS += -Icodec/common/arm/
CFLAGS += -DHAVE_NEON
endif
endif

ifneq ($(filter %86 x86_64, $(ARCH)),)
include $(SRC_PATH)build/x86-common.mk
endif
ifneq ($(filter-out arm64, $(filter arm%, $(ARCH))),)
ifeq ($(USE_ASM), Yes)
ASM_ARCH = arm
ASMFLAGS += -I$(SRC_PATH)codec/common/arm/
CFLAGS += -DHAVE_NEON
endif
endif
ifneq ($(filter arm64 aarch64, $(ARCH)),)
ifeq ($(USE_ASM), Yes)
ASM_ARCH = arm64
ASMFLAGS += -I$(SRC_PATH)codec/common/arm64/
CFLAGS += -DHAVE_NEON_AARCH64
endif
endif

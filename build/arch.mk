#for x86
HAVE_AVX2 := true

ifneq ($(filter %86 x86_64, $(ARCH)),)
include $(SRC_PATH)build/x86-common.mk
ifeq ($(USE_ASM), Yes)
ifeq ($(HAVE_AVX2), true)
CFLAGS += -DHAVE_AVX2
CXXFLAGS += -DHAVE_AVX2
ASMFLAGS += -DHAVE_AVX2
endif
endif
endif

#for arm
ifneq ($(filter-out arm64, $(filter arm%, $(ARCH))),)
ifeq ($(USE_ASM), Yes)
ASM_ARCH = arm
ASMFLAGS += -I$(SRC_PATH)codec/common/arm/
CFLAGS += -DHAVE_NEON
endif
endif

#for arm64
ifneq ($(filter arm64 aarch64, $(ARCH)),)
ifeq ($(USE_ASM), Yes)
ASM_ARCH = arm64
ASMFLAGS += -I$(SRC_PATH)codec/common/arm64/
CFLAGS += -DHAVE_NEON_AARCH64
endif
endif

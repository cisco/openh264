UNAME=$(shell uname | tr A-Z a-z | tr -d \\-[:digit:].)
ARCH=$(shell uname -m)
LIBPREFIX=lib
LIBSUFFIX=a
CXX_O=-o $@
CXX_LINK_O=-o $@
AR_OPTS=cr $@
LINK_LIB=-l$(1)
CFLAGS_OPT=-O3
CFLAGS_DEBUG=-g
CFLAGS_M32=-m32
CFLAGS_M64=-m64
BUILDTYPE=Release

ifeq (, $(ENABLE64BIT))
ifeq ($(ARCH), x86_64)
ENABLE64BIT=Yes
endif
endif

ifeq (,$(wildcard ./gtest))
HAVE_GTEST=No
else
HAVE_GTEST=Yes
endif

# Configurations
ifeq ($(BUILDTYPE), Release)
CFLAGS += $(CFLAGS_OPT)
USE_ASM = Yes
else
CFLAGS = $(CFLAGS_DEBUG)
USE_ASM = No
endif

ifeq ($(USE_ASAN), Yes)
CFLAGS += -fsanitize=address
LDFLAGS += -fsanitize=address
endif

ifeq ($(ENABLE64BIT), Yes)
CFLAGS += $(CFLAGS_M64)
LDFLAGS += $(CFLAGS_M64)
ASMFLAGS_PLATFORM = -DUNIX64
else
CFLAGS += $(CFLAGS_M32)
LDFLAGS += $(CFLAGS_M32)
ASMFLAGS_PLATFORM = -DX86_32
endif

include build/platform-$(UNAME).mk

ifeq ($(USE_ASM),Yes)
CFLAGS += -DX86_ASM
endif

CFLAGS += -DNO_DYNAMIC_VP
LDFLAGS +=
ASMFLAGS += $(ASMFLAGS_PLATFORM) -DNO_DYNAMIC_VP


#### No user-serviceable parts below this line
INCLUDES = -Icodec/api/svc -Icodec/common
#ASM_INCLUDES = -Iprocessing/src/asm/
ASM_INCLUDES = -Icodec/common/

DECODER_INCLUDES = \
    -Icodec/decoder/core/inc \
    -Icodec/decoder/plus/inc

ENCODER_INCLUDES = \
    -Icodec/encoder/core/inc \
    -Icodec/encoder/plus/inc

PROCESSING_INCLUDES = \
    -Icodec/encoder/core/inc \
    -Icodec/encoder/plus/inc

GTEST_INCLUDES = \
    -Igtest \
    -Igtest/include

CODEC_UNITTEST_INCLUDES = \
    -Igtest/include

H264DEC_INCLUDES = $(DECODER_INCLUDES) -Icodec/console/dec/inc
H264DEC_LDFLAGS = -L. $(call LINK_LIB,decoder) $(call LINK_LIB,common)
H264DEC_DEPS = $(LIBPREFIX)decoder.$(LIBSUFFIX) $(LIBPREFIX)common.$(LIBSUFFIX)

H264ENC_INCLUDES = $(ENCODER_INCLUDES) -Icodec/console/enc/inc
H264ENC_LDFLAGS = -L. $(call LINK_LIB,encoder) $(call LINK_LIB,processing) $(call LINK_LIB,common)
H264ENC_DEPS = $(LIBPREFIX)encoder.$(LIBSUFFIX) $(LIBPREFIX)processing.$(LIBSUFFIX) $(LIBPREFIX)common.$(LIBSUFFIX)

CODEC_UNITTEST_LDFLAGS = -L. -lgtest -ldecoder -lcrypto -lencoder -lprocessing -lcommon
CODEC_UNITTEST_DEPS = $(LIBPREFIX)gtest.$(LIBSUFFIX) $(LIBPREFIX)decoder.$(LIBSUFFIX) $(LIBPREFIX)encoder.$(LIBSUFFIX) $(LIBPREFIX)processing.$(LIBSUFFIX) $(LIBPREFIX)common.$(LIBSUFFIX)

.PHONY: test gtest-bootstrap clean

all:	libraries binaries

clean:
	rm -f $(OBJS) $(OBJS:.o=.d) $(LIBRARIES) $(BINARIES)

gtest-bootstrap:
	svn co https://googletest.googlecode.com/svn/trunk/ gtest

ifeq ($(HAVE_GTEST),Yes)
test: codec_unittest$(EXEEXT)
	./codec_unittest
else
test:
	@echo "./gtest : No such file or directory."
	@echo "You do not have gtest. Run make gtest-bootstrap to get gtest"
endif

include codec/common/targets.mk
include codec/decoder/targets.mk
include codec/encoder/targets.mk
include codec/processing/targets.mk
include codec/console/dec/targets.mk
include codec/console/enc/targets.mk

ifeq ($(HAVE_GTEST),Yes)
include build/gtest-targets.mk
include test/targets.mk
endif

-include $(OBJS:.o=.d)

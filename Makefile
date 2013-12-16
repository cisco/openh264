UNAME=$(shell uname | tr A-Z a-z)
LIBPREFIX=lib
LIBSUFFIX=a
CP=cp
ROOTDIR=$(PWD)

ifeq (,$(wildcard ./gtest))
HAVE_GTEST=No
else
HAVE_GTEST=Yes
endif

# Configurations
ifeq ($(BUILDTYPE), Release)
CFLAGS += -O3
ifneq ($(ENABLE64BIT), Yes)
USE_ASM = Yes
endif
else
CFLAGS = -g
USE_ASM = No
endif
ifeq ($(ENABLE64BIT), Yes)
CFLAGS += -m64
LDFLAGS += -m64
else
CFLAGS += -m32
LDFLAGS += -m32
endif
include build/platform-$(UNAME).mk

ifeq ($(USE_ASM),Yes)
  CFLAGS += -DX86_ASM
endif

CFLAGS += -DNO_DYNAMIC_VP -DHAVE_CACHE_LINE_ALIGN
LDFLAGS +=
ASMFLAGS += -DNO_DYNAMIC_VP -DNOPREFIX


#### No user-serviceable parts below this line
INCLUDES = -Icodec/api/svc  -Icodec/common -Igtest/include
ASM_INCLUDES = -Iprocessing/src/asm/

COMMON_INCLUDES = \
    -Icodec/decoder/core/inc

DECODER_INCLUDES = \
    -Icodec/decoder/core/inc \
    -Icodec/decoder/plus/inc

ENCODER_INCLUDES = \
    -Icodec/encoder/core/inc \
    -Icodec/encoder/plus/inc \
    -Icodec/WelsThreadLib/api

PROCESSING_INCLUDES = \
    -Icodec/encoder/core/inc \
    -Icodec/encoder/plus/inc

H264DEC_INCLUDES = $(DECODER_INCLUDES) -Icodec/console/dec/inc
H264DEC_LDFLAGS = -L. -ldecoder -lcommon

H264ENC_INCLUDES = $(ENCODER_INCLUDES) -Icodec/console/enc/inc
H264ENC_LDFLAGS = -L. -lencoder -lprocessing -lcommon

CODEC_UNITTEST_LDFLAGS = -L. -lgtest -ldecoder -lcommon

.PHONY: test

all:	libraries binaries

clean:
	rm -f $(OBJS) $(LIBRARIES) $(BINARIES)
	echo $(HAVE_GTEST)

gtest-bootstrap:
	svn co https://googletest.googlecode.com/svn/trunk/ gtest

test:
	./codec_unittest

include codec/common/targets.mk
include codec/decoder/targets.mk
include codec/encoder/targets.mk
include processing/targets.mk
include codec/console/dec/targets.mk
include codec/console/enc/targets.mk

ifeq ($(HAVE_GTEST),Yes)
include build/gtest-targets.mk
include test/targets.mk
endif







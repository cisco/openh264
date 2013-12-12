UNAME=$(shell uname | tr A-Z a-z)
LIBPREFIX=lib
LIBSUFFIX=a
ROOTDIR=$(PWD)
HAVE_GTEST=Yes

# Configurations
ifeq ($(BUILDTYPE), Release)
CFLAGS += -O3
USE_ASM = Yes
else
CFLAGS = -g
USE_ASM = No
endif

ifeq ($(USE_ASM),Yes)
  CFLAGS += -DX86_ASM
endif

include build/platform-$(UNAME).mk

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

CODEC_UNITTEST_LDFLAGS = -L. -lgtest

all:	libraries binaries

clean:
	rm -f $(OBJS) $(LIBRARIES) $(BINARIES)


include codec/common/targets.mk
include codec/decoder/targets.mk
include codec/encoder/targets.mk
include processing/targets.mk
include codec/console/dec/targets.mk
include codec/console/enc/targets.mk

ifdef HAVE_GTEST
include gtest/targets.mk
include test/targets.mk
endif






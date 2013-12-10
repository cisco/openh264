UNAME=$(shell uname | tr A-Z a-z)
LIBPREFIX=lib
LIBSUFFIX=a
ROOTDIR=$(PWD)

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
INCLUDES = -Icodec/api/svc
ASM_INCLUDES = -Iprocessing/src/asm/

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
H264DEC_LDFLAGS = -L. -ldecoder

H264ENC_INCLUDES = $(ENCODER_INCLUDES) -Icodec/console/enc/inc
H264ENC_LDFLAGS = -L. -lencoder -lprocessing

all:	libraries binaries

clean:
	rm -f $(OBJS) $(LIBRARIES) $(BINARIES)


include codec/decoder/targets.mk
include codec/encoder/targets.mk
include processing/targets.mk
include codec/console/dec/targets.mk
include codec/console/enc/targets.mk




LIBPREFIX=lib
LIBSUFFIX=a
ROOTDIR=$(PWD)

CFLAGS = -g -arch i386 -fPIC -DNO_DYNAMIC_VP
LDFLAGS = -arch i386 -ldl -lpthread 

INCLUDES = -Icodec/api/svc
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




OS=$(shell uname | tr A-Z a-z | tr -d \\-[:digit:].)
ARCH=$(shell uname -m)
LIBPREFIX=lib
LIBSUFFIX=a
CXX_O=-o $@
CXX_LINK_O=-o $@
AR_OPTS=cr $@
LINK_LIB=-l$(1)
CFLAGS_OPT=-O3
CFLAGS_DEBUG=-g
BUILDTYPE=Release
V=Yes
PREFIX=/usr/local
SHARED=-shared

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

include build/platform-$(OS).mk


CFLAGS += -DNO_DYNAMIC_VP
LDFLAGS +=


#### No user-serviceable parts below this line
ifneq ($(V),Yes)
    QUIET_CXX = @printf "CXX\t$@\n";
    QUIET_CC  = @printf "CC\t$@\n";
    QUIET_ASM = @printf "ASM\t$@\n";
    QUIET_AR  = @printf "AR\t$@\n";
    QUIET     = @
endif


INCLUDES = -Icodec/api/svc -Icodec/common
#ASM_INCLUDES = -Iprocessing/src/asm/
ASM_INCLUDES = -Icodec/common/

DECODER_INCLUDES = \
    -Icodec/decoder/core/inc \
    -Icodec/decoder/plus/inc

ENCODER_INCLUDES = \
    -Icodec/encoder/core/inc \
    -Icodec/encoder/plus/inc \
    -Icodec/processing/interface

PROCESSING_INCLUDES = \
    -Icodec/processing/interface \
    -Icodec/processing/src/common

GTEST_INCLUDES += \
    -Igtest \
    -Igtest/include

CODEC_UNITTEST_INCLUDES += \
    -Igtest/include

H264DEC_INCLUDES = $(DECODER_INCLUDES) -Icodec/console/dec/inc
H264DEC_LDFLAGS = -L. $(call LINK_LIB,decoder) $(call LINK_LIB,common)
H264DEC_DEPS = $(LIBPREFIX)decoder.$(LIBSUFFIX) $(LIBPREFIX)common.$(LIBSUFFIX)

H264ENC_INCLUDES = $(ENCODER_INCLUDES) -Icodec/console/enc/inc
H264ENC_LDFLAGS = -L. $(call LINK_LIB,encoder) $(call LINK_LIB,processing) $(call LINK_LIB,common)
H264ENC_DEPS = $(LIBPREFIX)encoder.$(LIBSUFFIX) $(LIBPREFIX)processing.$(LIBSUFFIX) $(LIBPREFIX)common.$(LIBSUFFIX)

CODEC_UNITTEST_LDFLAGS = -L. $(call LINK_LIB,gtest) $(call LINK_LIB,decoder) $(call LINK_LIB,encoder) $(call LINK_LIB,processing) $(call LINK_LIB,common) $(CODEC_UNITTEST_LDFLAGS_SUFFIX)
CODEC_UNITTEST_DEPS = $(LIBPREFIX)gtest.$(LIBSUFFIX) $(LIBPREFIX)decoder.$(LIBSUFFIX) $(LIBPREFIX)encoder.$(LIBSUFFIX) $(LIBPREFIX)processing.$(LIBSUFFIX) $(LIBPREFIX)common.$(LIBSUFFIX)

.PHONY: test gtest-bootstrap clean

all:	libraries binaries

clean:
	$(QUIET)rm -f $(OBJS) $(OBJS:.o=.d) $(LIBRARIES) $(BINARIES)

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

ifneq (android, $(OS))
include codec/console/dec/targets.mk
include codec/console/enc/targets.mk
endif

libraries: $(LIBPREFIX)wels.$(LIBSUFFIX) $(LIBPREFIX)wels.$(SHAREDLIBSUFFIX)
LIBRARIES += $(LIBPREFIX)wels.$(LIBSUFFIX) $(LIBPREFIX)wels.$(SHAREDLIBSUFFIX)

$(LIBPREFIX)wels.$(LIBSUFFIX): $(ENCODER_OBJS) $(DECODER_OBJS) $(PROCESSING_OBJS) $(COMMON_OBJS)
	$(QUIET)rm -f $@
	$(QUIET_AR)$(AR) $(AR_OPTS) $+

$(LIBPREFIX)wels.$(SHAREDLIBSUFFIX): $(ENCODER_OBJS) $(DECODER_OBJS) $(PROCESSING_OBJS) $(COMMON_OBJS)
	$(QUIET)rm -f $@
	$(QUIET_CXX)$(CXX) $(SHARED) $(LDFLAGS) $(CXX_LINK_O) $+ $(SHLDFLAGS)

install: $(LIBPREFIX)wels.$(LIBSUFFIX) $(LIBPREFIX)wels.$(SHAREDLIBSUFFIX)
	mkdir -p $(PREFIX)/lib
	mkdir -p $(PREFIX)/include/wels
	install -m 644 $(LIBPREFIX)wels.$(LIBSUFFIX) $(PREFIX)/lib
	install -m 755 $(LIBPREFIX)wels.$(SHAREDLIBSUFFIX) $(PREFIX)/lib
ifneq ($(EXTRA_LIBRARY),)
	install -m 644 $(EXTRA_LIBRARY) $(PREFIX)/lib
endif
	install -m 644 codec/api/svc/codec*.h $(PREFIX)/include/wels

ifeq ($(HAVE_GTEST),Yes)
include build/gtest-targets.mk
include test/targets.mk
endif

-include $(OBJS:.o=.d)

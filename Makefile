OS=$(shell uname | tr A-Z a-z | tr -d \\-[:digit:].)
ARCH=$(shell uname -m)
LIBPREFIX=lib
LIBSUFFIX=a
CCAS=$(CC)
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
OBJ=o
PROJECT_NAME=openh264
MODULE_NAME=gmpopenh264
CCASFLAGS=$(CFLAGS)

ifeq (,$(wildcard ./gmp-api))
HAVE_GMP_API=No
else
HAVE_GMP_API=Yes
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

include build/platform-$(OS).mk


CFLAGS +=
LDFLAGS +=

ifeq (Yes, $(GCOV))
CFLAGS += -fprofile-arcs -ftest-coverage
LDFLAGS += -lgcov
endif

#### No user-serviceable parts below this line
ifneq ($(V),Yes)
    QUIET_CXX = @printf "CXX\t$@\n";
    QUIET_CC  = @printf "CC\t$@\n";
    QUIET_CCAS = @printf "CCAS\t$@\n";
    QUIET_ASM = @printf "ASM\t$@\n";
    QUIET_AR  = @printf "AR\t$@\n";
    QUIET     = @
endif


INCLUDES = -Icodec/api/svc -Icodec/common/inc

DECODER_INCLUDES = \
    -Icodec/decoder/core/inc \
    -Icodec/decoder/plus/inc

ENCODER_INCLUDES = \
    -Icodec/encoder/core/inc \
    -Icodec/encoder/plus/inc \
    -Icodec/processing/interface

PROCESSING_INCLUDES = \
    -Icodec/processing/interface \
    -Icodec/processing/src/common \
    -Icodec/processing/src/scrolldetection

GTEST_INCLUDES += \
    -Igtest \
    -Igtest/include

CODEC_UNITTEST_INCLUDES += \
    -Igtest/include \
    -Icodec/common/inc \

H264DEC_INCLUDES = $(DECODER_INCLUDES) -Icodec/console/dec/inc
H264DEC_LDFLAGS = -L. $(call LINK_LIB,decoder) $(call LINK_LIB,common)
H264DEC_DEPS = $(LIBPREFIX)decoder.$(LIBSUFFIX) $(LIBPREFIX)common.$(LIBSUFFIX)

H264ENC_INCLUDES = $(ENCODER_INCLUDES) -Icodec/console/enc/inc
H264ENC_LDFLAGS = -L. $(call LINK_LIB,encoder) $(call LINK_LIB,processing) $(call LINK_LIB,common)
H264ENC_DEPS = $(LIBPREFIX)encoder.$(LIBSUFFIX) $(LIBPREFIX)processing.$(LIBSUFFIX) $(LIBPREFIX)common.$(LIBSUFFIX)

CODEC_UNITTEST_LDFLAGS = -L. $(call LINK_LIB,gtest) $(call LINK_LIB,decoder) $(call LINK_LIB,encoder) $(call LINK_LIB,processing) $(call LINK_LIB,common) $(CODEC_UNITTEST_LDFLAGS_SUFFIX)
CODEC_UNITTEST_DEPS = $(LIBPREFIX)gtest.$(LIBSUFFIX) $(LIBPREFIX)decoder.$(LIBSUFFIX) $(LIBPREFIX)encoder.$(LIBSUFFIX) $(LIBPREFIX)processing.$(LIBSUFFIX) $(LIBPREFIX)common.$(LIBSUFFIX)
DECODER_UNITTEST_INCLUDES = $(CODEC_UNITTEST_INCLUDES) $(DECODER_INCLUDES) -Itest -Itest/decoder
ENCODER_UNITTEST_INCLUDES = $(CODEC_UNITTEST_INCLUDES) $(ENCODER_INCLUDES) -Itest -Itest/encoder
PROCESSING_UNITTEST_INCLUDES = $(CODEC_UNITTEST_INCLUDES) $(PROCESSING_INCLUDES) -Itest -Itest/processing
API_TEST_INCLUDES = $(CODEC_UNITTEST_INCLUDES) -Itest -Itest/api
COMMON_UNITTEST_INCLUDES = $(CODEC_UNITTEST_INCLUDES) $(DECODER_INCLUDES) -Itest -Itest/common
MODULE_INCLUDES += -Igmp-api

.PHONY: test gtest-bootstrap clean

all:	libraries binaries

clean:
ifeq (android,$(OS))
clean: clean_Android
endif
	$(QUIET)rm -f $(OBJS) $(OBJS:.$(OBJ)=.d) $(LIBRARIES) $(BINARIES)

gmp-bootstrap:
	git clone https://github.com/mozilla/gmp-api gmp-api

gtest-bootstrap:
	svn co https://googletest.googlecode.com/svn/trunk/ gtest

ifeq ($(HAVE_GTEST),Yes)

test: codec_unittest$(EXEEXT)
ifneq (android,$(OS))
ifneq (ios,$(OS))
	./codec_unittest
endif
endif

else
test:
	@echo "./gtest : No such file or directory."
	@echo "You do not have gtest. Run make gtest-bootstrap to get gtest"
endif

include codec/common/targets.mk
include codec/decoder/targets.mk
include codec/encoder/targets.mk
include codec/processing/targets.mk

ifeq ($(HAVE_GMP_API),Yes)
include module/targets.mk
endif

ifneq (android, $(OS))
ifneq (ios, $(OS))
include codec/console/dec/targets.mk
include codec/console/enc/targets.mk
endif
endif

ifneq (ios, $(OS))
libraries: $(LIBPREFIX)$(PROJECT_NAME).$(LIBSUFFIX) $(LIBPREFIX)$(PROJECT_NAME).$(SHAREDLIBSUFFIX)
else
libraries: $(LIBPREFIX)$(PROJECT_NAME).$(LIBSUFFIX)
endif

LIBRARIES += $(LIBPREFIX)$(PROJECT_NAME).$(LIBSUFFIX) $(LIBPREFIX)$(PROJECT_NAME).$(SHAREDLIBSUFFIX)

$(LIBPREFIX)$(PROJECT_NAME).$(LIBSUFFIX): $(ENCODER_OBJS) $(DECODER_OBJS) $(PROCESSING_OBJS) $(COMMON_OBJS)
	$(QUIET)rm -f $@
	$(QUIET_AR)$(AR) $(AR_OPTS) $+

$(LIBPREFIX)$(PROJECT_NAME).$(SHAREDLIBSUFFIX): $(ENCODER_OBJS) $(DECODER_OBJS) $(PROCESSING_OBJS) $(COMMON_OBJS)
	$(QUIET)rm -f $@
	$(QUIET_CXX)$(CXX) $(SHARED) $(LDFLAGS) $(CXX_LINK_O) $+ $(SHLDFLAGS)

ifeq ($(HAVE_GMP_API),Yes)
plugin: $(LIBPREFIX)$(MODULE_NAME).$(SHAREDLIBSUFFIX)
LIBRARIES += $(LIBPREFIX)$(MODULE_NAME).$(SHAREDLIBSUFFIX)
else
plugin:
	@echo "./gmp-api : No such file or directory."
	@echo "You do not have gmp-api.  Run make gmp-bootstrap to get the gmp-api headers."
endif

$(LIBPREFIX)$(MODULE_NAME).$(SHAREDLIBSUFFIX): $(MODULE_OBJS) $(ENCODER_OBJS) $(DECODER_OBJS) $(PROCESSING_OBJS) $(COMMON_OBJS)
	$(QUIET)rm -f $@
	$(QUIET_CXX)$(CXX) $(SHARED) $(LDFLAGS) $(CXX_LINK_O) $+ $(SHLDFLAGS) $(MODULE_LDFLAGS)

install-headers:
	mkdir -p $(PREFIX)/include/wels
	install -m 644 codec/api/svc/codec*.h $(PREFIX)/include/wels

install-static: $(LIBPREFIX)$(PROJECT_NAME).$(LIBSUFFIX) install-headers
	mkdir -p $(PREFIX)/lib
	install -m 644 $(LIBPREFIX)$(PROJECT_NAME).$(LIBSUFFIX) $(PREFIX)/lib

install-shared: $(LIBPREFIX)$(PROJECT_NAME).$(SHAREDLIBSUFFIX) install-headers
	mkdir -p $(PREFIX)/lib
	install -m 755 $(LIBPREFIX)$(PROJECT_NAME).$(SHAREDLIBSUFFIX) $(PREFIX)/lib
ifneq ($(EXTRA_LIBRARY),)
	install -m 644 $(EXTRA_LIBRARY) $(PREFIX)/lib
endif

install: install-static install-shared
	@:

ifeq ($(HAVE_GTEST),Yes)
include build/gtest-targets.mk
include test/api/targets.mk
include test/decoder/targets.mk
include test/encoder/targets.mk
include test/processing/targets.mk
include test/common/targets.mk

LIBRARIES += $(LIBPREFIX)ut.$(LIBSUFFIX)
$(LIBPREFIX)ut.$(LIBSUFFIX): $(DECODER_UNITTEST_OBJS) $(ENCODER_UNITTEST_OBJS) $(PROCESSING_UNITTEST_OBJS) $(COMMON_UNITTEST_OBJS) $(API_TEST_OBJS)
	$(QUIET)rm -f $@
	$(QUIET_AR)$(AR) $(AR_OPTS) $+


LIBRARIES +=$(LIBPREFIX)ut.$(SHAREDLIBSUFFIX)
$(LIBPREFIX)ut.$(SHAREDLIBSUFFIX): $(DECODER_UNITTEST_OBJS) $(ENCODER_UNITTEST_OBJS) $(PROCESSING_UNITTEST_OBJS) $(API_TEST_OBJS) $(COMMON_UNITTEST_OBJS)  $(CODEC_UNITTEST_DEPS)
	$(QUIET)rm -f $@
	$(QUIET_CXX)$(CXX) $(SHARED) $(LDFLAGS) $(CXX_LINK_O) $+ $(CODEC_UNITTEST_LDFLAGS)

binaries: codec_unittest$(EXEEXT)
BINARIES += codec_unittest$(EXEEXT)

ifeq (ios,$(OS))
codec_unittest$(EXEEXT): $(LIBPREFIX)ut.$(LIBSUFFIX) $(LIBPREFIX)gtest.$(LIBSUFFIX) $(LIBPREFIX)$(PROJECT_NAME).$(LIBSUFFIX)

else
ifeq (android,$(OS))
codec_unittest$(EXEEXT): $(LIBPREFIX)ut.$(SHAREDLIBSUFFIX)
	cd ./test/build/android && $(NDKROOT)/ndk-build -B APP_ABI=$(APP_ABI) && android update project -t $(TARGET) -p . && ant debug

clean_Android: clean_Android_ut
clean_Android_ut:
	cd ./test/build/android && $(NDKROOT)/ndk-build APP_ABI=$(APP_ABI) clean && ant clean

else
codec_unittest$(EXEEXT): $(DECODER_UNITTEST_OBJS) $(ENCODER_UNITTEST_OBJS) $(PROCESSING_UNITTEST_OBJS) $(API_TEST_OBJS) $(COMMON_UNITTEST_OBJS) $(CODEC_UNITTEST_DEPS)
	$(QUIET)rm -f $@
	$(QUIET_CXX)$(CXX) $(CXX_LINK_O) $+ $(CODEC_UNITTEST_LDFLAGS) $(LDFLAGS)

endif
endif

else
binaries:
	@:
endif

-include $(OBJS:.$(OBJ)=.d)

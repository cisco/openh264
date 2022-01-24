include $(SRC_PATH)build/arch.mk
ifeq ($(ASM_ARCH), x86)
ifeq ($(ARCH), x86_64)
ASMFLAGS += -f win64
ASMFLAGS_PLATFORM = -DWIN64
else
ASMFLAGS += -f win32 -DPREFIX
endif
else
endif
ifeq ($(ASM_ARCH), arm)
CCAS = gas-preprocessor.pl -as-type armasm -force-thumb -- armasm
CCASFLAGS = -nologo -DHAVE_NEON -ignore 4509
endif

CC=cl
CXX=cl
AR=lib
CXX_O=-Fo$@

ifeq ($(ASM_ARCH), arm64)
CCAS = clang-cl
CCASFLAGS = -nologo -DHAVE_NEON_AARCH64 --target=arm64-windows
endif


# -D_VARIADIC_MAX=10 is required to fix building gtest on MSVC 2012, but
# since we don't (easily) know which version of MSVC we use here, we add
# it unconditionally. The same issue can also be worked around by adding
# -DGTEST_HAS_TR1_TUPLE=0 instead, but we prefer this version since it
# matches what gtest itself does.
CFLAGS += -nologo -W3 -EHsc -fp:precise -Zc:wchar_t -Zc:forScope -D_VARIADIC_MAX=10
CXX_LINK_O=-nologo -Fe$@
AR_OPTS=-nologo -out:$@
CFLAGS_OPT=-O2 -Ob1 -Oy- -Zi -GF -GS -Gy -DNDEBUG
CFLAGS_DEBUG=-Od -Oy- -Zi -RTC1 -D_DEBUG
CFLAGS_M32=
CFLAGS_M64=
LINK_LOCAL_DIR=
LINK_LIB=$(1).lib
LIBSUFFIX=lib
LIBPREFIX=
EXEEXT=.exe
OBJ=obj
SHAREDLIB_DIR = $(PREFIX)/bin
SHAREDLIBSUFFIX=dll
SHAREDLIBSUFFIXFULLVER=$(SHAREDLIBSUFFIX)
SHAREDLIBSUFFIXMAJORVER=$(SHAREDLIBSUFFIX)
SHARED=-LD
EXTRA_LIBRARY=$(PROJECT_NAME)_dll.lib
LDFLAGS += -link
SHLDFLAGS=-debug -map -opt:ref -opt:icf -def:$(SRC_PATH)openh264.def -implib:$(EXTRA_LIBRARY)
STATIC_LDFLAGS=
CODEC_UNITTEST_CFLAGS+=-D_CRT_SECURE_NO_WARNINGS

ifneq ($(filter %86 x86_64, $(ARCH)),)
LDFLAGS += -cetcompat
endif

%.res: %.rc
	$(QUIET_RC)rc -fo $@ $<

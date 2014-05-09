include build/platform-arch.mk
ifeq ($(ASM_ARCH), x86)
ifeq ($(ENABLE64BIT), Yes)
ASMFLAGS += -f win64
ASMFLAGS_PLATFORM = -DWIN64
CFLAGS += -DWIN64
else
ASMFLAGS += -f win32 -DPREFIX
CFLAGS += -DWIN32
endif
else
CFLAGS += -DWIN32
endif
ifeq ($(ASM_ARCH), arm)
CCAS = gas-preprocessor.pl -as-type armasm -force-thumb -- armasm
endif

CC=cl
CXX=cl
AR=lib
CXX_O=-Fo$@
# -DGTEST_HAS_TR1_TUPLE=0 is temporarily broken in gtest,
# using _VARIADIC_MAX=10 to fix building on MSVC 2012 meanwhile.
# Once gtest works with the former again, it should be preferred.
CFLAGS += -nologo -W3 -EHsc -fp:precise -Zc:wchar_t -Zc:forScope -D_VARIADIC_MAX=10
CXX_LINK_O=-nologo -Fe$@
AR_OPTS=-nologo -out:$@
CFLAGS_OPT=-O2 -Ob1 -Oy- -Zi -GF -Gm- -GS -Gy -DNDEBUG
CFLAGS_DEBUG=-Od -Oy- -ZI -RTC1 -D_DEBUG
CFLAGS_M32=
CFLAGS_M64=
LINK_LIB=$(1).lib
LIBSUFFIX=lib
LIBPREFIX=
EXEEXT=.exe
OBJ=obj
SHAREDLIBSUFFIX=dll
SHARED=-LD
SHLDFLAGS=-link -def:openh264.def -implib:$(PROJECT_NAME)_dll.lib
EXTRA_LIBRARY=$(PROJECT_NAME)_dll.lib

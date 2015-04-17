ARCH = arm
include $(SRC_PATH)build/msvc-common.mk
CFLAGS_OPT += -MD
CFLAGS_DEBUG += -MDd
CFLAGS += -DWINAPI_FAMILY=WINAPI_FAMILY_PHONE_APP -DWINDOWS_PHONE
# Ignore warnings about the main function prototype when building with -ZW
CXXFLAGS += -ZW -wd4447
LDFLAGS += -nodefaultlib:kernel32.lib -nodefaultlib:ole32.lib WindowsPhoneCore.lib
UTSHLDFLAGS=-def:ut.def
# Ignore warnings about code built with -ZW in .lib files
AR_OPTS += -ignore:4264

# WelsThreadLib requires building with -ZW, since it uses new Windows Runtime
# classes for creating threads.
# If linking an .exe that contains Windows Runtime code, the first object
# file linked into the exe also needs to be built with -ZW (otherwise the build
# fails with "vccorlib_lib_should_be_specified_before_msvcrt_lib_to_linker",
# so set it on all files.

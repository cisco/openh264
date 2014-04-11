include build/platform-msvc-common.mk
CFLAGS_OPT += -MD
CFLAGS_DEBUG += -MDd
ARCH=arm
include build/platform-arch.mk
CFLAGS += -DWINAPI_FAMILY=WINAPI_FAMILY_PHONE_APP -MD -DWIN32
CXXFLAGS += -ZW
LDFLAGS +=
CCAS = gas-preprocessor.pl -as-type armasm -force-thumb -- armasm


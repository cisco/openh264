include build/platform-msvc-common.mk
ARCH=arm
include build/platform-arch.mk
CFLAGS += -DWINAPI_FAMILY=WINAPI_FAMILY_PHONE_APP -MD -DWIN32
LDFLAGS +=
CCAS = gas-preprocessor.pl -as-type armasm -force-thumb -- armasm


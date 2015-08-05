ARCH = arm
include $(SRC_PATH)build/msvc-app.mk
CFLAGS += -DWINAPI_FAMILY=WINAPI_FAMILY_PHONE_APP -DWINDOWS_PHONE
LDFLAGS += -nodefaultlib:kernel32.lib -nodefaultlib:ole32.lib WindowsPhoneCore.lib
UTSHLDFLAGS = -def:$(SRC_PATH)ut.def


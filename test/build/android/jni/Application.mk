# debug/release, default is release
ifeq ($(OPTIM_debug),true)
APP_OPTIM := debug
else
APP_OPTIM := release
endif

# x86/armeabi-v7a/armeabi, default is armeabi-v7a
ifeq ($(ABI_x86),true)
APP_ABI := x86
else
ifeq ($(ABI_armeabi),true)
APP_ABI := armeabi
else
APP_ABI := armeabi-v7a
endif
endif

APP_STL := stlport_shared
APP_PLATFORM := android-12

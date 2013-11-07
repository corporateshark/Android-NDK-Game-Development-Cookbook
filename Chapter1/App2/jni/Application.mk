#
APP_OPTIM := release
APP_PLATFORM := android-7
APP_STL := gnustl_static
APP_CPPFLAGS += -frtti 
APP_CPPFLAGS += -fexceptions
APP_CPPFLAGS += -DANDROID
APP_ABI := armeabi-v7a
APP_MODULES := App2
NDK_TOOLCHAIN_VERSION := clang

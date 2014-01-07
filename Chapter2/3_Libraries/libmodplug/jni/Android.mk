#
# Part of the Linderdaum Engine

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

GLOBAL_C_INCLUDES := $(LOCAL_PATH)

LOCAL_SRC_FILES = ../loaders.cpp ../sndfile.cpp ../mixer.cpp ../modplug.cpp

LOCAL_MODULE := ModPlug

GLOBAL_CFLAGS   := -O3 -Werror -DHAVE_CONFIG_H=1

ifeq ($(TARGET_ARCH),x86)
	LOCAL_CFLAGS   := $(GLOBAL_CFLAGS)
else
	LOCAL_CFLAGS   := -mfpu=vfp -mfloat-abi=softfp -fno-short-enums $(GLOBAL_CFLAGS)
endif

LOCAL_C_INCLUDES := $(GLOBAL_C_INCLUDES)

include $(BUILD_STATIC_LIBRARY)

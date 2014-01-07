#

TARGET_PLATFORM := android-7

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
LOCAL_MODULE     := App13

LOCAL_C_INCLUDES += \
	-I $(LOCAL_PATH)/../src \
	-I $(LOCAL_PATH)/../src/GL \
	-I $(LOCAL_PATH)/../src/LGL \

LOCAL_SRC_FILES += Wrappers.cpp WrappersJNI.c ../main.cpp ../src/LGL/LGL.cpp

LOCAL_ARM_MODE := arm
COMMON_CFLAGS := -Werror -DANDROID -DDISABLE_IMPORTGL

ifeq ($(TARGET_ARCH),x86)
	LOCAL_CFLAGS   := $(COMMON_CFLAGS)
else
	LOCAL_CFLAGS   := -mfpu=vfp -mfloat-abi=softfp -fno-short-enums $(COMMON_CFLAGS)
endif

LOCAL_LDLIBS     := -llog -lGLESv2 -Wl,-s

LOCAL_CPPFLAGS += -std=gnu++0x

include $(BUILD_SHARED_LIBRARY)

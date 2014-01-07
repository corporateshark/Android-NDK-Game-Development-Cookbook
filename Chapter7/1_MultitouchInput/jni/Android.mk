#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
LOCAL_MODULE := App12

# using Engine include folders
LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/../include \
	$(LOCAL_PATH)/../src \
	$(LOCAL_PATH)/../src/core \
	$(LOCAL_PATH)/../src/threading \

LOCAL_SRC_FILES += Wrappers.cpp WrappersJNI.c \
	../src/threading/Event.cpp \
	../src/threading/Thread.cpp \
	../src/threading/tinythread.cpp \
	../src/Multitouch.cpp \
	../src/Rendering.cpp \
	../src/Gestures.cpp \
	../src/core/VecMath.cpp \
	../main.cpp

LOCAL_ARM_MODE := arm
PROJ_CFLAGS := -Wno-unused-value -Wno-format-extra-args -Wno-format-security -O3 -Werror -DANDROID -DDISABLE_IMPORTGL -D_DISABLE_TUNNELLERS_ -D_DISABLE_METHODS_ -DDISABLE_GUARD_UNGUARD_MECHANISM

ifeq ($(TARGET_ARCH),x86)
	GLOBAL_CFLAGS  := $(PROJ_CFLAGS)
else
	GLOBAL_CFLAGS  := -mfpu=vfp -mfloat-abi=softfp -fno-short-enums $(PROJ_CFLAGS)
endif

LOCAL_CFLAGS := $(GLOBAL_CFLAGS)

LOCAL_CPPFLAGS += -std=gnu++0x

LOCAL_LDLIBS    += -llog -lGLESv2

include $(BUILD_SHARED_LIBRARY)

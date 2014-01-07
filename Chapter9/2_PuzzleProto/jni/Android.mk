#

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := libFreeImage
LOCAL_SRC_FILES := ../../../Libs.Android/libFreeImage.$(TARGET_ARCH_ABI).a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libFreeType
LOCAL_SRC_FILES := ../../../Libs.Android/libFreeType.$(TARGET_ARCH_ABI).a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libOpenAL
LOCAL_SRC_FILES := ../../../Libs.Android/libOpenAL.$(TARGET_ARCH_ABI).a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libOGG
LOCAL_SRC_FILES := ../../../Libs.Android/libOGG.$(TARGET_ARCH_ABI).a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libVorbis
LOCAL_SRC_FILES := ../../../Libs.Android/libVorbis.$(TARGET_ARCH_ABI).a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libModPlug
LOCAL_SRC_FILES := ../../../Libs.Android/libModPlug.$(TARGET_ARCH_ABI).a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

TARGET_PLATFORM := android-7

LOCAL_ARM_MODE := arm
LOCAL_MODULE     := Game1

LOCAL_C_INCLUDES += \
	-I $(LOCAL_PATH)/../src \
	-I $(LOCAL_PATH)/../src/game \
	-I $(LOCAL_PATH)/../../Engine \
	-I $(LOCAL_PATH)/../../Engine/GL \
	-I $(LOCAL_PATH)/../../Engine/LGL \
	-I $(LOCAL_PATH)/../../Engine/core \
	-I $(LOCAL_PATH)/../../Engine/graphics \
	-I $(LOCAL_PATH)/../../Engine/fs \
	-I $(LOCAL_PATH)/../../Engine/sound \
	-I $(LOCAL_PATH)/../../Engine/network \
	-I $(LOCAL_PATH)/../../Engine/threading \
	-I $(LOCAL_PATH)/../../Engine/include \
	-I $(LOCAL_PATH)/../../Engine/include/vorbis \
	-I $(LOCAL_PATH)/../../Engine/include/modplug \

LOCAL_SRC_FILES += Wrappers.cpp WrappersJNI.c 
LOCAL_SRC_FILES += ../main.cpp
LOCAL_SRC_FILES += ../../Engine/Engine.cpp
LOCAL_SRC_FILES += ../../Engine/LGL/LGL.cpp ../../Engine/LGL/GLClasses.cpp
LOCAL_SRC_FILES += ../../Engine/core/iIntrusivePtr.cpp ../../Engine/core/VecMath.cpp
LOCAL_SRC_FILES += ../../Engine/fs/FileSystem.cpp ../../Engine/fs/libcompress.c ../../Engine/fs/Archive.cpp
LOCAL_SRC_FILES += ../../Engine/graphics/Geometry.cpp  ../../Engine/graphics/Canvas.cpp ../../Engine/graphics/Gestures.cpp ../../Engine/graphics/Multitouch.cpp ../../Engine/graphics/TextRenderer.cpp ../../Engine/graphics/ft_load.cpp ../../Engine/graphics/Bitmap.cpp ../../Engine/graphics/FI_Utils.cpp ../../Engine/graphics/GUI.cpp
LOCAL_SRC_FILES += ../../Engine/sound/Decoders.cpp ../../Engine/sound/LAL.cpp ../../Engine/sound/Audio.cpp
LOCAL_SRC_FILES += ../../Engine/threading/Event.cpp ../../Engine/threading/Thread.cpp ../../Engine/threading/tinythread.cpp ../../Engine/threading/WorkerThread.cpp
LOCAL_SRC_FILES += ../src/game/Game.cpp

LOCAL_ARM_MODE := arm
COMMON_CFLAGS := -Werror -DANDROID -DDISABLE_IMPORTGL

ifeq ($(TARGET_ARCH),x86)
	LOCAL_CFLAGS   := $(COMMON_CFLAGS)
else
	LOCAL_CFLAGS   := -mfpu=vfp -mfloat-abi=softfp -fno-short-enums $(COMMON_CFLAGS)
endif

LOCAL_LDLIBS     := -llog -lGLESv2 -Wl,-s

LOCAL_CPPFLAGS += -std=gnu++11

LOCAL_STATIC_LIBRARIES += FreeImage
LOCAL_STATIC_LIBRARIES += FreeType
LOCAL_STATIC_LIBRARIES += OpenAL
LOCAL_STATIC_LIBRARIES += Vorbis
LOCAL_STATIC_LIBRARIES += OGG
LOCAL_STATIC_LIBRARIES += ModPlug

include $(BUILD_SHARED_LIBRARY)

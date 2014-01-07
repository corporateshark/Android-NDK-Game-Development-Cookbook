#

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE    := libFreeImage
LOCAL_SRC_FILES :=../../../Libs.Android/libFreeImage.$(TARGET_ARCH_ABI).a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libFreeType
LOCAL_SRC_FILES :=../../../Libs.Android/libFreeType.$(TARGET_ARCH_ABI).a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libOpenAL
LOCAL_SRC_FILES :=../../../Libs.Android/libOpenAL.$(TARGET_ARCH_ABI).a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libOGG
LOCAL_SRC_FILES :=../../../Libs.Android/libOGG.$(TARGET_ARCH_ABI).a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libVorbis
LOCAL_SRC_FILES :=../../../Libs.Android/libVorbis.$(TARGET_ARCH_ABI).a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)
LOCAL_MODULE    := libModPlug
LOCAL_SRC_FILES :=../../../Libs.Android/libModPlug.$(TARGET_ARCH_ABI).a
include $(PREBUILT_STATIC_LIBRARY)

include $(CLEAR_VARS)

TARGET_PLATFORM := android-7

LOCAL_ARM_MODE := arm
LOCAL_MODULE     := Game1

LOCAL_C_INCLUDES += \
	-I $(LOCAL_PATH)/../src \
	-I $(LOCAL_PATH)/../src/GL \
	-I $(LOCAL_PATH)/../src/LGL \
	-I $(LOCAL_PATH)/../src/core \
	-I $(LOCAL_PATH)/../src/game \
	-I $(LOCAL_PATH)/../src/graphics \
	-I $(LOCAL_PATH)/../src/fs \
	-I $(LOCAL_PATH)/../src/sound \
	-I $(LOCAL_PATH)/../src/threading \
	-I $(LOCAL_PATH)/../src/include \
	-I $(LOCAL_PATH)/../src/include/vorbis \
	-I $(LOCAL_PATH)/../src/include/modplug \


LOCAL_SRC_FILES += Wrappers.cpp WrappersJNI.c 
LOCAL_SRC_FILES += ../main.cpp ../src/Engine.cpp
LOCAL_SRC_FILES += ../src/LGL/LGL.cpp ../src/LGL/GLClasses.cpp
LOCAL_SRC_FILES += ../src/core/iIntrusivePtr.cpp ../src/core/VecMath.cpp
LOCAL_SRC_FILES += ../src/fs/FileSystem.cpp ../src/fs/libcompress.c ../src/fs/Archive.cpp
LOCAL_SRC_FILES += ../src/graphics/Geometry.cpp  ../src/graphics/Canvas.cpp ../src/graphics/Gestures.cpp ../src/graphics/Multitouch.cpp ../src/graphics/TextRenderer.cpp ../src/graphics/ft_load.cpp ../src/graphics/Bitmap.cpp ../src/graphics/FI_Utils.cpp
LOCAL_SRC_FILES += ../src/sound/Decoders.cpp ../src/sound/LAL.cpp ../src/sound/Audio.cpp
LOCAL_SRC_FILES += ../src/threading/Event.cpp ../src/threading/Thread.cpp ../src/threading/tinythread.cpp ../src/threading/WorkerThread.cpp
LOCAL_SRC_FILES += ../src/game/Globals.cpp ../src/game/Pentomino.cpp

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

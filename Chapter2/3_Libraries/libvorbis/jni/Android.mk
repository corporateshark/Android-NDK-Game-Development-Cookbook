#
# Part of the Linderdaum Engine

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

GLOBAL_C_INCLUDES := \
	$(LOCAL_PATH)/../include \
	$(LOCAL_PATH)/../lib

LOCAL_SRC_FILES = ../lib/mdct.c ../lib/smallft.c ../lib/block.c ../lib/envelope.c ../lib/window.c ../lib/lsp.c \
			../lib/lpc.c ../lib/analysis.c ../lib/synthesis.c ../lib/psy.c ../lib/info.c \
			../lib/floor1.c ../lib/floor0.c \
			../lib/res0.c ../lib/mapping0.c ../lib/registry.c ../lib/codebook.c ../lib/sharedbook.c \
			../lib/lookup.c ../lib/bitrate.c ../lib/vorbisfile.c

LOCAL_MODULE := Vorbis

GLOBAL_CFLAGS   := -O3 -Werror -DHAVE_CONFIG_H=1

ifeq ($(TARGET_ARCH),x86)
	LOCAL_CFLAGS   := $(GLOBAL_CFLAGS)
else
	LOCAL_CFLAGS   := -mfpu=vfp -mfloat-abi=softfp -fno-short-enums $(GLOBAL_CFLAGS)
endif

LOCAL_C_INCLUDES := $(GLOBAL_C_INCLUDES)

include $(BUILD_STATIC_LIBRARY)

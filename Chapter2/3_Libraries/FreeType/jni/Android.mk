#
# Part of the Linderdaum Engine

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

GLOBAL_C_INCLUDES := \
	$(LOCAL_PATH)/../include \
	$(LOCAL_PATH)/../devel

LOCAL_SRC_FILES := \
	../src/pfr/pfr.c\
	../src/bdf/bdf.c\
	../src/winfonts/winfnt.c\
	../src/gzip/ftgzip.c\
	../src/lzw/ftlzw.c\
	../src/psaux/psaux.c\
	../src/psnames/psnames.c\
	../src/type1/type1.c\
	../src/cid/type1cid.c\
	../src/base/ftbase.c\
	../src/base/ftbbox.c\
	../src/base/ftbdf.c\
	../src/base/ftbitmap.c\
	../src/base/ftcid.c\
	../src/base/ftdebug.c\
	../src/base/ftfstype.c\
	../src/base/ftgasp.c\
	../src/base/ftglyph.c\
	../src/base/ftgxval.c\
	../src/base/ftinit.c\
	../src/base/ftlcdfil.c\
	../src/base/ftmm.c\
	../src/base/ftotval.c\
	../src/base/ftpatent.c\
	../src/base/ftpfr.c\
	../src/base/ftstroke.c\
	../src/base/ftsynth.c\
	../src/base/ftsystem.c\
	../src/base/fttype1.c\
	../src/base/ftwinfnt.c\
	../src/base/ftxf86.c\
	../src/raster/raster.c\
	../src/smooth/smooth.c\
	../src/autofit/autofit.c\
	../src/cache/ftcache.c\
	../src/sfnt/sfnt.c\
	../src/truetype/truetype.c\

LOCAL_MODULE := FreeType

GLOBAL_CFLAGS   := -Werror -DHAVE_CONFIG_H=1 -DFT2_BUILD_LIBRARY

ifeq ($(TARGET_ARCH),x86)
	LOCAL_CFLAGS   := $(GLOBAL_CFLAGS)
else
	LOCAL_CFLAGS   := -mfpu=vfp -mfloat-abi=softfp -fno-short-enums $(GLOBAL_CFLAGS)
endif

LOCAL_C_INCLUDES := $(GLOBAL_C_INCLUDES)

include $(BUILD_STATIC_LIBRARY)

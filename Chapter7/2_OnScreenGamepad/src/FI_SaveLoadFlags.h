/*
 * Copyright (C) 2013 Sergey Kosarevsky (sk@linderdaum.com)
 * Copyright (C) 2013 Viktor Latypov (vl@linderdaum.com)
 * Based on Linderdaum Engine http://www.linderdaum.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must display the names 'Sergey Kosarevsky' and
 *    'Viktor Latypov'in the credits of the application, if such credits exist.
 *    The authors of this work must be notified via email (sk@linderdaum.com) in
 *    this case of redistribution.
 *
 * 3. Neither the name of copyright holders nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _FI_SaveLoadFlags_h_
#define _FI_SaveLoadFlags_h_

#if defined( OS_ANDROID ) || defined( OS_MACOSX )
// Android uses statically linked FreeImage
#define FREEIMAGE_LIB
//	#include "ExtLibs/FreeImage.h"
#endif

//#else // OS_ANDROID


// extracted from FreeImage.h
#if defined(FREEIMAGE_LIB)
#define DLL_API
#define DLL_CALLCONV
#else
#if defined(_WIN32) || defined(__WIN32__)
#define DLL_CALLCONV __stdcall
// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the FREEIMAGE_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// DLL_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef FREEIMAGE_EXPORTS
#define DLL_API __declspec(dllexport)
#else
#define DLL_API __declspec(dllimport)
#endif // FREEIMAGE_EXPORTS
#else
// try the gcc visibility support (see http://gcc.gnu.org/wiki/Visibility)
#if defined(__GNUC__) && ((__GNUC__ >= 4) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4))
#ifndef GCC_HASCLASSVISIBILITY
#define GCC_HASCLASSVISIBILITY
#endif
#endif // __GNUC__
#define DLL_CALLCONV
#if defined(GCC_HASCLASSVISIBILITY)
#define DLL_API __attribute__ ((visibility("default")))
#else
#define DLL_API
#endif
#endif // WIN32 / !WIN32
#endif // FREEIMAGE_LIB

#define BMP_DEFAULT         0
#define BMP_SAVE_RLE        1
#define CUT_DEFAULT         0
#define DDS_DEFAULT        0
#define EXR_DEFAULT        0     // save data as half with piz-based wavelet compression
#define EXR_FLOAT       0x0001   // save data as float instead of as half (not recommended)
#define EXR_NONE        0x0002   // save with no compression
#define EXR_ZIP            0x0004   // save with zlib compression, in blocks of 16 scan lines
#define EXR_PIZ            0x0008   // save with piz-based wavelet compression
#define EXR_PXR24       0x0010   // save with lossy 24-bit float compression
#define EXR_B44            0x0020   // save with lossy 44% float compression - goes to 22% when combined with EXR_LC
#define EXR_LC          0x0040   // save images with one luminance and two chroma channels, rather than as RGB (lossy compression)
#define FAXG3_DEFAULT      0
#define GIF_DEFAULT        0
#define GIF_LOAD256        1     // Load the image as a 256 color image with ununsed palette entries, if it's 16 or 2 color
#define GIF_PLAYBACK    2     // 'Play' the GIF to generate each frame (as 32bpp) instead of returning raw frame data when loading
#define HDR_DEFAULT        0
#define ICO_DEFAULT         0
#define ICO_MAKEALPHA      1     // convert to 32bpp and create an alpha channel from the AND-mask when loading
#define IFF_DEFAULT         0
#define J2K_DEFAULT        0     // save with a 16:1 rate
#define JP2_DEFAULT        0     // save with a 16:1 rate
#define JPEG_DEFAULT        0    // loading (see JPEG_FAST); saving (see JPEG_QUALITYGOOD|JPEG_SUBSAMPLING_420)
#define JPEG_FAST           0x0001  // load the file as fast as possible, sacrificing some quality
#define JPEG_ACCURATE       0x0002  // load the file with the best quality, sacrificing some speed
#define JPEG_CMYK       0x0004   // load separated CMYK "as is" (use | to combine with other load flags)
#define JPEG_EXIFROTATE    0x0008   // load and rotate according to Exif 'Orientation' tag if available
#define JPEG_QUALITYSUPERB  0x80 // save with superb quality (100:1)
#define JPEG_QUALITYGOOD    0x0100  // save with good quality (75:1)
#define JPEG_QUALITYNORMAL  0x0200  // save with normal quality (50:1)
#define JPEG_QUALITYAVERAGE 0x0400  // save with average quality (25:1)
#define JPEG_QUALITYBAD     0x0800  // save with bad quality (10:1)
#define JPEG_PROGRESSIVE   0x2000   // save as a progressive-JPEG (use | to combine with other save flags)
#define JPEG_SUBSAMPLING_411 0x1000    // save with high 4x1 chroma subsampling (4:1:1) 
#define JPEG_SUBSAMPLING_420 0x4000    // save with medium 2x2 medium chroma subsampling (4:2:0) - default value
#define JPEG_SUBSAMPLING_422 0x8000    // save with low 2x1 chroma subsampling (4:2:2) 
#define JPEG_SUBSAMPLING_444 0x10000   // save with no chroma subsampling (4:4:4)
#define JPEG_OPTIMIZE      0x20000     // on saving, compute optimal Huffman coding tables (can reduce a few percent of file size)
#define JPEG_BASELINE      0x40000     // save basic JPEG, without metadata or any markers
#define KOALA_DEFAULT       0
#define LBM_DEFAULT         0
#define MNG_DEFAULT         0
#define PCD_DEFAULT         0
#define PCD_BASE            1    // load the bitmap sized 768 x 512
#define PCD_BASEDIV4        2    // load the bitmap sized 384 x 256
#define PCD_BASEDIV16       3    // load the bitmap sized 192 x 128
#define PCX_DEFAULT         0
#define PFM_DEFAULT         0
#define PICT_DEFAULT        0
#define PNG_DEFAULT         0
#define PNG_IGNOREGAMMA    1     // loading: avoid gamma correction
#define PNG_Z_BEST_SPEED         0x0001   // save using ZLib level 1 compression flag (default value is 6)
#define PNG_Z_DEFAULT_COMPRESSION   0x0006   // save using ZLib level 6 compression flag (default recommended value)
#define PNG_Z_BEST_COMPRESSION      0x0009   // save using ZLib level 9 compression flag (default value is 6)
#define PNG_Z_NO_COMPRESSION     0x0100   // save without ZLib compression
#define PNG_INTERLACED           0x0200   // save using Adam7 interlacing (use | to combine with other save flags)
#define PNM_DEFAULT         0
#define PNM_SAVE_RAW        0       // If set the writer saves in RAW format (i.e. P4, P5 or P6)
#define PNM_SAVE_ASCII      1       // If set the writer saves in ASCII format (i.e. P1, P2 or P3)
#define PSD_DEFAULT         0
#define PSD_CMYK        1     // reads tags for separated CMYK (default is conversion to RGB)
#define PSD_LAB            2     // reads tags for CIELab (default is conversion to RGB)
#define RAS_DEFAULT         0
#define RAW_DEFAULT         0    // load the file as linear RGB 48-bit
#define RAW_PREVIEW        1     // try to load the embedded JPEG preview with included Exif Data or default to RGB 24-bit
#define RAW_DISPLAY        2     // load the file as RGB 24-bit
#define RAW_HALFSIZE    4     // output a half-size color image
#define SGI_DEFAULT        0
#define TARGA_DEFAULT       0
#define TARGA_LOAD_RGB888   1       // If set the loader converts RGB555 and ARGB8888 -> RGB888.
#define TARGA_SAVE_RLE     2     // If set, the writer saves with RLE compression
#define TIFF_DEFAULT        0
#define TIFF_CMYK       0x0001   // reads/stores tags for separated CMYK (use | to combine with compression flags)
#define TIFF_PACKBITS       0x0100  // save using PACKBITS compression
#define TIFF_DEFLATE        0x0200  // save using DEFLATE compression (a.k.a. ZLIB compression)
#define TIFF_ADOBE_DEFLATE  0x0400  // save using ADOBE DEFLATE compression
#define TIFF_NONE           0x0800  // save without any compression
#define TIFF_CCITTFAX3     0x1000  // save using CCITT Group 3 fax encoding
#define TIFF_CCITTFAX4     0x2000  // save using CCITT Group 4 fax encoding
#define TIFF_LZW        0x4000   // save using LZW compression
#define TIFF_JPEG       0x8000   // save using JPEG compression
#define TIFF_LOGLUV        0x10000  // save using LogLuv compression
#define WBMP_DEFAULT        0
#define XBM_DEFAULT        0
#define XPM_DEFAULT        0


enum FREE_IMAGE_FORMAT
{
   FIF_UNKNOWN = -1,
   FIF_BMP     = 0,
   FIF_ICO     = 1,
   FIF_JPEG = 2,
   FIF_JNG     = 3,
   FIF_KOALA   = 4,
   FIF_LBM     = 5,
   FIF_IFF = FIF_LBM,
   FIF_MNG     = 6,
   FIF_PBM     = 7,
   FIF_PBMRAW  = 8,
   FIF_PCD     = 9,
   FIF_PCX     = 10,
   FIF_PGM     = 11,
   FIF_PGMRAW  = 12,
   FIF_PNG     = 13,
   FIF_PPM     = 14,
   FIF_PPMRAW  = 15,
   FIF_RAS     = 16,
   FIF_TARGA   = 17,
   FIF_TIFF = 18,
   FIF_WBMP = 19,
   FIF_PSD     = 20,
   FIF_CUT     = 21,
   FIF_XBM     = 22,
   FIF_XPM     = 23,
   FIF_DDS     = 24,
   FIF_GIF     = 25,
   FIF_HDR     = 26,
   FIF_FAXG3   = 27,
   FIF_SGI     = 28,
   FIF_EXR     = 29,
   FIF_J2K     = 30,
   FIF_JP2     = 31
};

enum FREE_IMAGE_FILTER
{
   FILTER_BOX       = 0,   // Box, pulse, Fourier window, 1st order (constant) b-spline
   FILTER_BICUBIC   = 1,   // Mitchell & Netravali's two-param cubic filter
   FILTER_BILINEAR   = 2,  // Bilinear filter
   FILTER_BSPLINE   = 3,   // 4th order (cubic) b-spline
   FILTER_CATMULLROM = 4,  // Catmull-Rom spline, Overhauser spline
   FILTER_LANCZOS3     = 5 // Lanczos3 filter
};

typedef struct
{
	void* data;
} FIBITMAP;

typedef struct
{
	void* data;
} FIMEMORY;

// #endif // OS_ANDROID

typedef FIMEMORY* ( DLL_CALLCONV* PFNFreeImage_OpenMemory ) ( void*, Luint );
typedef void      ( DLL_CALLCONV* PFNFreeImage_CloseMemory ) ( FIMEMORY* );
typedef FIBITMAP* ( DLL_CALLCONV* PFNFreeImage_LoadFromMemory ) ( FREE_IMAGE_FORMAT, FIMEMORY*, int );
typedef bool      ( DLL_CALLCONV* PFNFreeImage_AcquireMemory ) ( FIMEMORY* stream, Lubyte** data, Luint32* size_in_bytes );
typedef FREE_IMAGE_FORMAT ( DLL_CALLCONV* PFNFreeImage_GetFileTypeFromMemory ) ( FIMEMORY*, int );
typedef unsigned  ( DLL_CALLCONV* PFNFreeImage_GetBPP ) ( FIBITMAP* );
typedef unsigned  ( DLL_CALLCONV* PFNFreeImage_GetWidth )( FIBITMAP* );
typedef unsigned  ( DLL_CALLCONV* PFNFreeImage_GetHeight )( FIBITMAP* );
typedef unsigned  ( DLL_CALLCONV* PFNFreeImage_GetPitch )( FIBITMAP* );
typedef FIBITMAP* ( DLL_CALLCONV* PFNFreeImage_ConvertToRGBF )( FIBITMAP* );
typedef FIBITMAP* ( DLL_CALLCONV* PFNFreeImage_ConvertTo24Bits )( FIBITMAP* );
typedef FIBITMAP* ( DLL_CALLCONV* PFNFreeImage_ConvertTo32Bits )( FIBITMAP* );
typedef void      ( DLL_CALLCONV* PFNFreeImage_Unload )( FIBITMAP* );
typedef int       ( DLL_CALLCONV* PFNFreeImage_IsTransparent )( FIBITMAP* );
typedef int       ( DLL_CALLCONV* PFNFreeImage_FlipVertical )( FIBITMAP* );
typedef Lubyte*   ( DLL_CALLCONV* PFNFreeImage_GetBits )( FIBITMAP* );
typedef FIBITMAP* ( DLL_CALLCONV* PFNFreeImage_Allocate )( int, int, int, unsigned, unsigned, unsigned );
typedef bool      ( DLL_CALLCONV* PFNFreeImage_Save )( FREE_IMAGE_FORMAT, FIBITMAP*, const char*, int );
typedef bool      ( DLL_CALLCONV* PFNFreeImage_SaveToMemory )( FREE_IMAGE_FORMAT fif, FIBITMAP*, FIMEMORY*, int );
typedef void      ( DLL_CALLCONV* PFNFreeImage_Initialise )( int );
typedef void      ( DLL_CALLCONV* PFNFreeImage_DeInitialise )( void );

typedef FIBITMAP* ( DLL_CALLCONV* PFNFreeImage_ConvertFromRawBits )( void*, int , int , int , unsigned , unsigned , unsigned , unsigned , int );
typedef void      ( DLL_CALLCONV* PFNFreeImage_ConvertToRawBits )( void*, FIBITMAP*, int , unsigned , unsigned , unsigned , unsigned , int );

typedef Lubyte*   ( DLL_CALLCONV* PFNFreeImage_GetScanLine )( FIBITMAP* dib, int scanline );
typedef FIBITMAP* ( DLL_CALLCONV* PFNFreeImage_Rescale )( FIBITMAP* dib, int dst_width, int dst_height, FREE_IMAGE_FILTER filter );

#endif

/*
 * 24/03/2010
     It's here
*/

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

#include "Platform.h"
#include "fs/Streams.h"

#include "Bitmap.h"
#include "FI_Utils.h"
#include "FI_SaveLoadFlags.h"

#include <memory.h>

// init counter
static int FI_NumInitialized = 0;

#if !defined( ANDROID )
#if !defined( _WIN64 )
const char* FreeImageDLL = "freeimage32.dll";
#else
const char* FreeImageDLL = "freeimage64.dll";
#endif

HMODULE Lib = NULL;
#else
// Android uses statically linked FreeImage.
// LV: But this doesn't mean we use some of its headers directly
extern "C"
{
	DLL_API void DLL_CALLCONV FreeImage_Initialise( bool load_local_plugins_only );
	DLL_API void DLL_CALLCONV FreeImage_DeInitialise( void );

// Allocate / Clone / Unload routines ---------------------------------------

	DLL_API FIBITMAP* DLL_CALLCONV FreeImage_Allocate( int width, int height, int bpp, unsigned red_mask, unsigned green_mask, unsigned blue_mask );
	DLL_API void DLL_CALLCONV FreeImage_Unload( FIBITMAP* dib );

// Load / Save routines -----------------------------------------------------

	DLL_API bool DLL_CALLCONV FreeImage_Save( FREE_IMAGE_FORMAT fif, FIBITMAP* dib, const char* filename, int flags );

// Memory I/O stream routines -----------------------------------------------

	DLL_API FIMEMORY* DLL_CALLCONV FreeImage_OpenMemory( Lubyte data, Luint32 size_in_bytes );
	DLL_API bool      DLL_CALLCONV FreeImage_AcquireMemory( FIMEMORY* stream, Lubyte** data, Luint32* size_in_bytes );
	DLL_API void DLL_CALLCONV FreeImage_CloseMemory( FIMEMORY* stream );

	DLL_API FIBITMAP* DLL_CALLCONV FreeImage_LoadFromMemory( FREE_IMAGE_FORMAT fif, FIMEMORY* stream, int flags );
	DLL_API bool DLL_CALLCONV FreeImage_SaveToMemory( FREE_IMAGE_FORMAT fif, FIBITMAP* dib, FIMEMORY* stream, int flags );
	DLL_API FREE_IMAGE_FORMAT DLL_CALLCONV FreeImage_GetFileTypeFromMemory( FIMEMORY* stream, int size );

// Pixel access routines ----------------------------------------------------
	DLL_API Lubyte* DLL_CALLCONV FreeImage_GetBits( FIBITMAP* dib );

	DLL_API unsigned DLL_CALLCONV FreeImage_GetPitch( FIBITMAP* dib );

	DLL_API FIBITMAP* DLL_CALLCONV FreeImage_ConvertFromRawBits ( void*, int , int , int , unsigned , unsigned , unsigned , unsigned , int );
	DLL_API void      DLL_CALLCONV FreeImage_ConvertToRawBits ( void*, FIBITMAP*, int , unsigned , unsigned , unsigned , unsigned , int );

	DLL_API Lubyte*   DLL_CALLCONV FreeImage_GetScanLine( FIBITMAP* dib, int scanline );

	DLL_API FIBITMAP* DLL_CALLCONV FreeImage_Rescale( FIBITMAP* dib, int dst_width, int dst_height, FREE_IMAGE_FILTER filter );

// DIB info routines --------------------------------------------------------

	DLL_API unsigned DLL_CALLCONV FreeImage_GetBPP( FIBITMAP* dib );
	DLL_API unsigned DLL_CALLCONV FreeImage_GetWidth( FIBITMAP* dib );
	DLL_API unsigned DLL_CALLCONV FreeImage_GetHeight( FIBITMAP* dib );

// Smart conversion routines ------------------------------------------------
	DLL_API FIBITMAP* DLL_CALLCONV FreeImage_ConvertTo24Bits( FIBITMAP* dib );
	DLL_API FIBITMAP* DLL_CALLCONV FreeImage_ConvertTo32Bits( FIBITMAP* dib );
	DLL_API FIBITMAP* DLL_CALLCONV FreeImage_ConvertToRGBF( FIBITMAP* dib );
	DLL_API bool DLL_CALLCONV FreeImage_IsTransparent( FIBITMAP* dib );
	DLL_API bool DLL_CALLCONV FreeImage_FlipHorizontal( FIBITMAP* dib );
	DLL_API bool DLL_CALLCONV FreeImage_FlipVertical( FIBITMAP* dib );
}

#endif // OS_ANDROID

PFNFreeImage_Initialise            FI_Initialise = NULL;
PFNFreeImage_DeInitialise          FI_DeInitialise = NULL;
PFNFreeImage_OpenMemory            FI_OpenMemory = NULL;
PFNFreeImage_CloseMemory           FI_CloseMemory = NULL;
PFNFreeImage_LoadFromMemory        FI_LoadFromMemory = NULL;
PFNFreeImage_AcquireMemory         FI_AcquireMemory = NULL;
PFNFreeImage_GetFileTypeFromMemory FI_GetFileTypeFromMemory = NULL;
PFNFreeImage_GetBPP                FI_GetBPP = NULL;
PFNFreeImage_GetWidth              FI_GetWidth = NULL;
PFNFreeImage_GetHeight             FI_GetHeight = NULL;
PFNFreeImage_ConvertToRGBF         FI_ConvertToRGBF = NULL;
PFNFreeImage_ConvertTo24Bits       FI_ConvertTo24Bits = NULL;
PFNFreeImage_ConvertTo32Bits       FI_ConvertTo32Bits = NULL;
PFNFreeImage_Unload                FI_Unload = NULL;
PFNFreeImage_IsTransparent         FI_IsTransparent = NULL;
PFNFreeImage_FlipVertical          FI_FlipVertical = NULL;
PFNFreeImage_GetBits               FI_GetBits = NULL;
PFNFreeImage_Allocate              FI_Allocate = NULL;
PFNFreeImage_Save                  FI_Save = NULL;
PFNFreeImage_SaveToMemory          FI_SaveToMemory = NULL;
PFNFreeImage_GetPitch              FI_GetPitch = NULL;
PFNFreeImage_ConvertFromRawBits    FI_ConvertFromRawBits = NULL;
PFNFreeImage_ConvertToRawBits      FI_ConvertToRawBits = NULL;
PFNFreeImage_GetScanLine           FI_GetScanLine = NULL;
PFNFreeImage_Rescale               FI_Rescale = NULL;

void FreeImage_Unload()
{
#if !defined( ANDROID )

	if ( FI_NumInitialized != 1 ) { if ( FI_NumInitialized > 0 ) { FI_NumInitialized--; } return; }

	FI_NumInitialized--;

	CloseHandle( Lib );

	FI_OpenMemory = NULL;
	Lib = NULL;
#endif // OS_ANDROID
}

bool FreeImage_Init()
{
	if ( FI_NumInitialized > 0 ) { FI_NumInitialized++; return true; }

#if defined( ANDROID )
	FI_Initialise = ( PFNFreeImage_Initialise )&FreeImage_Initialise;
	FI_DeInitialise = ( PFNFreeImage_DeInitialise )&FreeImage_DeInitialise;
	FI_OpenMemory = ( PFNFreeImage_OpenMemory )&FreeImage_OpenMemory;
	FI_CloseMemory = ( PFNFreeImage_CloseMemory )&FreeImage_CloseMemory;
	FI_LoadFromMemory = ( PFNFreeImage_LoadFromMemory )&FreeImage_LoadFromMemory;
	FI_AcquireMemory = ( PFNFreeImage_AcquireMemory )&FreeImage_AcquireMemory;
	FI_GetFileTypeFromMemory = ( PFNFreeImage_GetFileTypeFromMemory )&FreeImage_GetFileTypeFromMemory;
	FI_GetBPP = ( PFNFreeImage_GetBPP )&FreeImage_GetBPP;
	FI_GetWidth = ( PFNFreeImage_GetWidth )&FreeImage_GetWidth;
	FI_GetHeight = ( PFNFreeImage_GetHeight )&FreeImage_GetHeight;
	FI_GetPitch = ( PFNFreeImage_GetPitch )&FreeImage_GetPitch;
	FI_ConvertToRGBF = ( PFNFreeImage_ConvertToRGBF )&FreeImage_ConvertToRGBF;
	FI_ConvertTo24Bits = ( PFNFreeImage_ConvertTo24Bits )&FreeImage_ConvertTo24Bits;
	FI_ConvertTo32Bits = ( PFNFreeImage_ConvertTo32Bits )&FreeImage_ConvertTo32Bits;
	FI_Unload = ( PFNFreeImage_Unload )&FreeImage_Unload;
	FI_IsTransparent = ( PFNFreeImage_IsTransparent )&FreeImage_IsTransparent;
	FI_FlipVertical = ( PFNFreeImage_FlipVertical )&FreeImage_FlipVertical;
	FI_GetBits = ( PFNFreeImage_GetBits )&FreeImage_GetBits;
	FI_Allocate = ( PFNFreeImage_Allocate )&FreeImage_Allocate;
	FI_Save = ( PFNFreeImage_Save )&FreeImage_Save;
	FI_SaveToMemory = ( PFNFreeImage_SaveToMemory )&FreeImage_SaveToMemory;

	FI_ConvertFromRawBits = ( PFNFreeImage_ConvertFromRawBits )&FreeImage_ConvertFromRawBits;
	FI_ConvertToRawBits = ( PFNFreeImage_ConvertToRawBits )&FreeImage_ConvertToRawBits;

	FI_GetScanLine = ( PFNFreeImage_GetScanLine )&FreeImage_GetScanLine;

	FI_Rescale = ( PFNFreeImage_Rescale )&FreeImage_Rescale;

#else // OS_ANDROID
	Lib = LoadLibrary( FreeImageDLL );

	FI_Initialise = ( PFNFreeImage_Initialise )GetProcAddress( Lib, "FreeImage_Initialise" );
	FI_DeInitialise = ( PFNFreeImage_DeInitialise )GetProcAddress( Lib, "FreeImage_DeInitialise" );
	FI_OpenMemory = ( PFNFreeImage_OpenMemory )GetProcAddress( Lib, "FreeImage_OpenMemory" );
	FI_CloseMemory = ( PFNFreeImage_CloseMemory )GetProcAddress( Lib, "FreeImage_CloseMemory" );
	FI_LoadFromMemory = ( PFNFreeImage_LoadFromMemory )GetProcAddress( Lib, "FreeImage_LoadFromMemory" );
	FI_AcquireMemory = ( PFNFreeImage_AcquireMemory )GetProcAddress( Lib, "FreeImage_AcquireMemory" );
	FI_GetFileTypeFromMemory = ( PFNFreeImage_GetFileTypeFromMemory )GetProcAddress( Lib, "FreeImage_GetFileTypeFromMemory" );
	FI_GetBPP = ( PFNFreeImage_GetBPP )GetProcAddress( Lib, "FreeImage_GetBPP" );
	FI_GetWidth = ( PFNFreeImage_GetWidth )GetProcAddress( Lib, "FreeImage_GetWidth" );
	FI_GetHeight = ( PFNFreeImage_GetHeight )GetProcAddress( Lib, "FreeImage_GetHeight" );
	FI_GetPitch = ( PFNFreeImage_GetPitch )GetProcAddress( Lib, "FreeImage_GetPitch" );
	FI_ConvertToRGBF = ( PFNFreeImage_ConvertToRGBF )GetProcAddress( Lib, "FreeImage_ConvertToRGBF" );
	FI_ConvertTo24Bits = ( PFNFreeImage_ConvertTo24Bits )GetProcAddress( Lib, "FreeImage_ConvertTo24Bits" );
	FI_ConvertTo32Bits = ( PFNFreeImage_ConvertTo32Bits )GetProcAddress( Lib, "FreeImage_ConvertTo32Bits" );
	FI_Unload = ( PFNFreeImage_Unload )GetProcAddress( Lib, "FreeImage_Unload" );
	FI_IsTransparent = ( PFNFreeImage_IsTransparent )GetProcAddress( Lib, "FreeImage_IsTransparent" );
	FI_FlipVertical = ( PFNFreeImage_FlipVertical )GetProcAddress( Lib, "FreeImage_FlipVertical" );
	FI_GetBits = ( PFNFreeImage_GetBits )GetProcAddress( Lib, "FreeImage_GetBits" );
	FI_Allocate = ( PFNFreeImage_Allocate )GetProcAddress( Lib, "FreeImage_Allocate" );
	FI_Save = ( PFNFreeImage_Save )GetProcAddress( Lib, "FreeImage_Save" );
	FI_SaveToMemory = ( PFNFreeImage_SaveToMemory )GetProcAddress( Lib, "FreeImage_SaveToMemory" );

	FI_ConvertFromRawBits = ( PFNFreeImage_ConvertFromRawBits )GetProcAddress( Lib, "FreeImage_ConvertFromRawBits" );
	FI_ConvertToRawBits = ( PFNFreeImage_ConvertToRawBits )GetProcAddress( Lib, "FreeImage_ConvertToRawBits" );

	FI_GetScanLine = ( PFNFreeImage_GetScanLine )GetProcAddress( Lib, "FreeImage_GetScanLine" );
	FI_Rescale = ( PFNFreeImage_Rescale )GetProcAddress( Lib, "FreeImage_Rescale" );

	if ( !FI_Initialise )
	{
		// some Win32 / C++ name mangling black magic
		FI_Initialise = ( PFNFreeImage_Initialise )GetProcAddress( Lib, "_FreeImage_Initialise@4" );
		FI_DeInitialise = ( PFNFreeImage_DeInitialise )GetProcAddress( Lib, "_FreeImage_DeInitialise@0" );
		FI_OpenMemory = ( PFNFreeImage_OpenMemory )GetProcAddress( Lib, "_FreeImage_OpenMemory@8" );
		FI_CloseMemory = ( PFNFreeImage_CloseMemory )GetProcAddress( Lib, "_FreeImage_CloseMemory@4" );
		FI_LoadFromMemory = ( PFNFreeImage_LoadFromMemory )GetProcAddress( Lib, "_FreeImage_LoadFromMemory@12" );
		FI_AcquireMemory = ( PFNFreeImage_AcquireMemory )GetProcAddress( Lib, "_FreeImage_AcquireMemory@12" );
		FI_GetFileTypeFromMemory = ( PFNFreeImage_GetFileTypeFromMemory )GetProcAddress( Lib, "_FreeImage_GetFileTypeFromMemory@8" );
		FI_GetBPP = ( PFNFreeImage_GetBPP )GetProcAddress( Lib, "_FreeImage_GetBPP@4" );
		FI_GetWidth = ( PFNFreeImage_GetWidth )GetProcAddress( Lib, "_FreeImage_GetWidth@4" );
		FI_GetHeight = ( PFNFreeImage_GetHeight )GetProcAddress( Lib, "_FreeImage_GetHeight@4" );
		FI_GetPitch = ( PFNFreeImage_GetPitch )GetProcAddress( Lib, "_FreeImage_GetPitch@4" );
		FI_ConvertToRGBF = ( PFNFreeImage_ConvertToRGBF )GetProcAddress( Lib, "_FreeImage_ConvertToRGBF@4" );
		FI_ConvertTo24Bits = ( PFNFreeImage_ConvertTo24Bits )GetProcAddress( Lib, "_FreeImage_ConvertTo24Bits@4" );
		FI_ConvertTo32Bits = ( PFNFreeImage_ConvertTo32Bits )GetProcAddress( Lib, "_FreeImage_ConvertTo32Bits@4" );
		FI_Unload = ( PFNFreeImage_Unload )GetProcAddress( Lib, "_FreeImage_Unload@4" );
		FI_IsTransparent = ( PFNFreeImage_IsTransparent )GetProcAddress( Lib, "_FreeImage_IsTransparent@4" );
		FI_FlipVertical = ( PFNFreeImage_FlipVertical )GetProcAddress( Lib, "_FreeImage_FlipVertical@4" );
		FI_GetBits = ( PFNFreeImage_GetBits )GetProcAddress( Lib, "_FreeImage_GetBits@4" );
		FI_Allocate = ( PFNFreeImage_Allocate )GetProcAddress( Lib, "_FreeImage_Allocate@24" );
		FI_Save = ( PFNFreeImage_Save )GetProcAddress( Lib, "_FreeImage_Save@16" );
		FI_SaveToMemory = ( PFNFreeImage_SaveToMemory )GetProcAddress( Lib, "_FreeImage_SaveToMemory@16" );
		FI_ConvertFromRawBits = ( PFNFreeImage_ConvertFromRawBits )GetProcAddress( Lib, "_FreeImage_ConvertFromRawBits@36" );
		FI_ConvertToRawBits = ( PFNFreeImage_ConvertToRawBits )GetProcAddress( Lib, "_FreeImage_ConvertToRawBits@32" );

		FI_GetScanLine = ( PFNFreeImage_GetScanLine )GetProcAddress( Lib, "_FreeImage_GetScanLine@8" );
		FI_Rescale = ( PFNFreeImage_Rescale )GetProcAddress( Lib, "_FreeImage_Rescale@16" );
	}

#endif // OS_ANDROID

	FI_Initialise( false );
	FI_NumInitialized = 1;

	return true;
}

bool FreeImage_LoadFromStream( clPtr<iIStream> IStream, const clPtr<clBitmap>& OutBitmap, bool DoFlipV )
{
	if ( !FI_OpenMemory ) { FreeImage_Init(); }

	FIMEMORY* Mem = FI_OpenMemory( const_cast<ubyte*>( IStream->MapStreamFromCurrentPos() ), static_cast<Luint>( IStream->GetSize() ) );

	FREE_IMAGE_FORMAT FIF = FI_GetFileTypeFromMemory( Mem, 0 );

	FIBITMAP* Bitmap = FI_LoadFromMemory( FIF, Mem, 0 );
	FIBITMAP* ConvBitmap;

	FI_CloseMemory( Mem );

	ConvBitmap = FI_IsTransparent( Bitmap ) ? FI_ConvertTo32Bits( Bitmap ) : FI_ConvertTo24Bits( Bitmap );

	FI_Unload( Bitmap );

	Bitmap = ConvBitmap;

	if ( DoFlipV ) { FI_FlipVertical( Bitmap ); }

	LBitmapFormat BitmapFormat = FI_GetBPP( Bitmap ) == 24 ? L_BITMAP_BGR8 : L_BITMAP_BGRA8;

	int Width  = FI_GetWidth( Bitmap );
	int Height = FI_GetHeight( Bitmap );
	int Pitch  = FI_GetPitch( Bitmap );

	sBitmapParams BMPRec = sBitmapParams( Width, Height, BitmapFormat );

	OutBitmap->ReallocImageData( &BMPRec );

	FI_ConvertToRawBits( OutBitmap->FBitmapData, Bitmap, OutBitmap->FBitmapParams.FWidth * OutBitmap->FBitmapParams.GetBytesPerPixel(), OutBitmap->FBitmapParams.GetBitsPerPixel(), 2, 1, 0, false );

	FI_Unload( Bitmap );

	return true;
}

void FreeImage_Rescale( const clPtr<clBitmap>& Bmp, int NewWidth, int NewHeight )
{
	FIBITMAP* Bitmap = FI_ConvertFromRawBits( Bmp->FBitmapData,
	                                          Bmp->FBitmapParams.FWidth,
	                                          Bmp->FBitmapParams.FHeight,
	                                          Bmp->FBitmapParams.FWidth * Bmp->FBitmapParams.GetBytesPerPixel(),
	                                          Bmp->FBitmapParams.GetBitsPerPixel(), 0, 1, 2, false );

	FIBITMAP* ConvBitmap = FI_Rescale( Bitmap, NewWidth, NewHeight, FILTER_BSPLINE );

	sBitmapParams BMPRec = Bmp->FBitmapParams;
	BMPRec.FWidth = NewWidth;
	BMPRec.FHeight = NewHeight;

	Bmp->ReallocImageData( &BMPRec );

	FI_ConvertToRawBits( Bmp->FBitmapData,
	                     ConvBitmap,
	                     Bmp->FBitmapParams.FWidth * Bmp->FBitmapParams.GetBytesPerPixel(),
	                     Bmp->FBitmapParams.GetBitsPerPixel(), 0, 1, 2, false );

	FI_Unload( ConvBitmap );
	FI_Unload( Bitmap );
}

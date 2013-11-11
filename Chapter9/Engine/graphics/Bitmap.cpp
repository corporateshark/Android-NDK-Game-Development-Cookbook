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

#include "Bitmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#include "FI_Utils.h"

LBitmapFormat sBitmapParams::SuggestBitmapFormat( int BitsPerPixel )
{
	if ( BitsPerPixel == 24 ) { return L_BITMAP_BGR8; }

	if ( BitsPerPixel == 32 ) { return L_BITMAP_BGRA8; }

	return L_BITMAP_INVALID_FORMAT;
}

int sBitmapParams::GetStorageSize() const
{
	return FWidth * FHeight * GetBytesPerPixel();
}

int sBitmapParams::GetBitsPerPixel() const
{
	if ( FBitmapFormat == L_BITMAP_BGR8 ) { return 24; }

	if ( FBitmapFormat == L_BITMAP_BGRA8 ) { return 32; }

	return 0;
}

int sBitmapParams::GetBytesPerPixel() const
{
	if ( FBitmapFormat == L_BITMAP_BGR8 ) { return 3; }

	if ( FBitmapFormat == L_BITMAP_BGRA8 ) { return 4; }

	return 0;
}

sBitmapParams::sBitmapParams( const int Width, const int Height, const LBitmapFormat BitmapFormat )
{
	FWidth = Width;
	FHeight = Height;
	FBitmapFormat = BitmapFormat;
}

void clBitmap::Load2DImage( const clPtr<iIStream>& Stream, bool DoFlipV )
{
	FreeImage_LoadFromStream( Stream, this, DoFlipV );
#if defined( ANDROID )
	ConvertRGBtoBGR();
#endif
}

clPtr<clBitmap> clBitmap::LoadImg( const clPtr<iIStream>& Stream )
{
	clPtr<clBitmap> Bmp = new clBitmap();

	Bmp->Load2DImage( Stream, true );

	return Bmp;
}

clPtr<clBitmap> clBitmap::LoadImg( const clPtr<iIStream>& Stream, bool DoFlipV )
{
	clPtr<clBitmap> Bmp = new clBitmap();

	Bmp->Load2DImage( Stream, DoFlipV );

	return Bmp;
}

struct
#if defined(__GCC__)
__attribute__( ( packed, aligned( 1 ) ) )
#endif
sRGB
{
   char R, G, B;
};

struct
#if defined(__GCC__)
__attribute__( ( packed, aligned( 1 ) ) )
#endif
sRGBA
{
   char R, G, B, A;
};

void clBitmap::ConvertRGBtoBGR()
{
	if ( !FBitmapData ) { return; }

	if ( FBitmapParams.FBitmapFormat == L_BITMAP_BGR8 )
	{
		for ( int y = 0; y != FBitmapParams.FHeight; y++ )
		{
			for ( int x = 0; x != FBitmapParams.FWidth; x++ )
			{
				sRGB* C = ( sRGB* )( &FBitmapData[ 3 * ( y * FBitmapParams.FWidth + x ) ] );
				std::swap( C->R, C->B );
			}
		}
	}

	if ( FBitmapParams.FBitmapFormat == L_BITMAP_BGRA8 )
	{
		for ( int y = 0; y != FBitmapParams.FHeight; y++ )
		{
			for ( int x = 0; x != FBitmapParams.FWidth; x++ )
			{
				sRGBA* C = ( sRGBA* )( &FBitmapData[ 4 * ( y * FBitmapParams.FWidth + x ) ] );
				std::swap( C->R, C->B );
			}
		}
	}
}

void clBitmap::SetPixel( int X, int Y, const LVector4i& Color )
{
	if ( X < 0 || X >= FBitmapParams.FWidth || Y < 0 || Y >= FBitmapParams.FHeight ) { return; }

	if ( FBitmapParams.FBitmapFormat == L_BITMAP_BGR8 )
	{
		int Offset = ( Y * FBitmapParams.FWidth + X ) * 3;

		sRGB* C = ( sRGB* )( &FBitmapData[ Offset ] );
		C->R = Color.x;
		C->G = Color.y;
		C->B = Color.z;
	}

	if ( FBitmapParams.FBitmapFormat == L_BITMAP_BGRA8 )
	{
		int Offset = ( Y * FBitmapParams.FWidth + X ) * 4;

		sRGBA* C = ( sRGBA* )( &FBitmapData[ Offset ] );
		C->R = Color.x;
		C->G = Color.y;
		C->B = Color.z;
		C->A = Color.w;
	}
}

void clBitmap::Clear()
{
	if ( !FBitmapData ) { return; }

	memset( FBitmapData, 0, FBitmapParams.GetStorageSize() );
}

void clBitmap::Rescale( int NewW, int NewH )
{
	FreeImage_Rescale( clPtr<clBitmap>( this ), NewW, NewH );
}

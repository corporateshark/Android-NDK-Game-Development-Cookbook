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

struct __attribute__( ( packed, aligned( 1 ) ) ) sRGB
{
   char R, G, B;
};

struct __attribute__( ( packed, aligned( 1 ) ) ) sRGBA
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

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

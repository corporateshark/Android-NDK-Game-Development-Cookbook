#include "Bitmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

unsigned char* read_bmp_mem( const unsigned char* data, int* w, int* h )
{
	const unsigned char* head = data;
	*w = head[18] + ( ( ( int )head[19] ) << 8 ) + ( ( ( int )head[20] ) << 16 ) + ( ( ( int )head[21] ) << 24 );
	*h = head[22] + ( ( ( int )head[23] ) << 8 ) + ( ( ( int )head[24] ) << 16 ) + ( ( ( int )head[25] ) << 24 );
	const int fileSize = ( ( *w ) * 3 + ( ( *w ) % 4 ) ) * ( *h );
	unsigned char* img = ( unsigned char* )malloc( ( *w ) * ( *h ) * 3 );
	data += 54;
	int i, j, k, rev_j;

	for ( j = 0, rev_j = ( *h ) - 1; j < ( *h ) ; j++, rev_j-- )
	{
		for ( i = 0 ; i < ( *w ) ; i++ )
		{
			int fpos = j * ( ( *w ) * 3 + ( *h ) % 4 ) + i * 3, pos = rev_j * ( *w ) * 3 + i * 3;

			for ( k = 0 ; k < 3 ; k++ )
			{
				// RGB -> BGR
				img[pos + k] =
#if defined( ANDROID )
				   data[fpos + ( 2 - k )];
#else
				   data[fpos + k];
#endif
			}
		}
	}

	return img;
}

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

void clBitmap::Load2DImage( const clPtr<iIStream>& Stream )
{
	free( FBitmapData );

	FBitmapData = read_bmp_mem( Stream->MapStream(), &FBitmapParams.FWidth, &FBitmapParams.FHeight );
}

clPtr<clBitmap> clBitmap::LoadImg( const clPtr<iIStream>& Stream )
{
	clPtr<clBitmap> Bmp = new clBitmap();

	Bmp->Load2DImage( Stream );

	return Bmp;
}

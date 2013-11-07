#pragma once

#include "Streams.h"
#include "bmp.h"

class Bitmap: public iObject
{
public:
	Bitmap( const int W, const int H )
	{
		FWidth  = W;
		FHeight = H;

		size_t Size = FWidth * FHeight * 3;

		if ( Size == 0 ) { return; }

		FBitmapData = ( ubyte* )malloc( Size );
		memset( FBitmapData, 0xFF, Size );
	}
	virtual ~Bitmap() { free( FBitmapData ); }
	void Load2DImage( clPtr<iIStream> Stream )
	{
		free( FBitmapData );
		FBitmapData = read_bmp_mem( Stream->MapStream(), &FWidth, &FHeight );
	}
public:
	/// Dimensions
	int FWidth, FHeight;
	/// Image data
	ubyte*          FBitmapData;
};

#pragma once

#include "iObject.h"

class Bitmap: public iObject
{
public:
	Bitmap( int W, int H )
	{
		FWidth  = W;
		FHeight = H;

		size_t Size = FWidth * FHeight * 4;

		if ( Size == 0 ) { return; }

		FBitmapData = ( ubyte* )malloc( Size );
		memset( FBitmapData, 0x0, Size );
	}
	virtual ~Bitmap() { free( FBitmapData ); }
public:
	/// Dimensions
	int FWidth, FHeight;
	/// Image data
	ubyte*          FBitmapData;
};

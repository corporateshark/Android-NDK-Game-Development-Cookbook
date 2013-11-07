#pragma once

#include "Platform.h"
#include "Streams.h"
#include "iIntrusivePtr.h"

enum LBitmapFormat
{
   L_BITMAP_INVALID_FORMAT = -1,
   L_BITMAP_BGR8           =  0,
   L_BITMAP_BGRA8          =  1,
};

struct sBitmapParams
{
public:
	sBitmapParams() : FWidth( 0 ), FHeight( 0 ), FBitmapFormat( L_BITMAP_BGR8 ) {}

	sBitmapParams( const int Width, const int Height, const LBitmapFormat BitmapFormat );

	/// Returns how many bytes of memory the texture image will take
	int        GetStorageSize() const;
	int        GetBitsPerPixel() const;
	int        GetBytesPerPixel() const;

	static LBitmapFormat SuggestBitmapFormat( int BitsPerPixel );
public:
	int           FWidth;
	int           FHeight;
	LBitmapFormat FBitmapFormat;
};

class clBitmap: public iObject
{
public:
	clBitmap()
		: FBitmapParams( 0, 0, L_BITMAP_BGR8 )
		, FBitmapData( NULL )
	{};
	clBitmap( const int W, const int H )
		: FBitmapParams( W, H, L_BITMAP_BGR8 )
	{
		size_t Size = FBitmapParams.GetStorageSize();

		FBitmapData = ( ubyte* )malloc( Size );
		memset( FBitmapData, 0xFF, Size );
	}
	virtual ~clBitmap() { free( FBitmapData ); }

	int GetWidth() const { return FBitmapParams.FWidth; }
	int GetHeight() const { return FBitmapParams.FHeight; }

	void Load2DImage( const clPtr<iIStream>& Stream );

	static clPtr<clBitmap> LoadImg( const clPtr<iIStream>& Stream );

public:
	sBitmapParams FBitmapParams;
	ubyte*        FBitmapData;
};

#pragma once

#include "Platform.h"
#include "Streams.h"
#include "iIntrusivePtr.h"
#include "VecMath.h"

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
		, FBitmapData( NULL )
	{
		ReallocImageData();
	}
	clBitmap( const int W, const int H, LBitmapFormat Format )
		: FBitmapParams( W, H, Format )
		, FBitmapData( NULL )
	{
		ReallocImageData();
	}
	virtual ~clBitmap() { free( FBitmapData ); }

	void ReallocImageData( const sBitmapParams* BitmapParams )
	{
		free( FBitmapData );

		FBitmapData = NULL;

		if ( !BitmapParams ) { return; }

		FBitmapParams = *BitmapParams;
		ReallocImageData();
	}

	void ReallocImageData()
	{
		size_t Size = FBitmapParams.GetStorageSize();

		FBitmapData = ( ubyte* )malloc( Size );
		memset( FBitmapData, 0xFF, Size );
	}

	void ConvertRGBtoBGR();

	int GetWidth() const { return FBitmapParams.FWidth; }
	int GetHeight() const { return FBitmapParams.FHeight; }

	void SetPixel( int X, int Y, const LVector4i& Color );
	void Clear();
	void Rescale( int NewW, int NewH );

	void Load2DImage( const clPtr<iIStream>& Stream, bool DoFlipV );

	static clPtr<clBitmap> LoadImg( const clPtr<iIStream>& Stream );
	static clPtr<clBitmap> LoadImg( const clPtr<iIStream>& Stream, bool DoFlipV );

public:
	sBitmapParams FBitmapParams;
	ubyte*        FBitmapData;
};

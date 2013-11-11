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

	void Load2DImage( const clPtr<iIStream>& Stream, bool DoFlipV );

	static clPtr<clBitmap> LoadImg( const clPtr<iIStream>& Stream );
	static clPtr<clBitmap> LoadImg( const clPtr<iIStream>& Stream, bool DoFlipV );

public:
	sBitmapParams FBitmapParams;
	ubyte*        FBitmapData;
};

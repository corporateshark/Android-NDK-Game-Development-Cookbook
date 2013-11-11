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

#include "TextRenderer.h"

#include "Bitmap.h"
#include <stdio.h>

#include "Blob.h"
#include "FileSystem.h"

#include "ft_load.h"

extern clPtr<clFileSystem> g_FS;

clTextRenderer::clTextRenderer()
	: FInitialized( false ),
	  FLibrary( NULL ),
	  FManager( NULL ),
	  FImageCache( NULL ),
	  FCMapCache( NULL )
{
	InitFreeType();
}

bool clTextRenderer::DecodeUTF8( const char* InStr )
{
	FIndex = 0;
	FBuffer = InStr;
	FLength = ( int )strlen( InStr );

	FString.clear();

	int R = DecodeNextUTF8Char();

	while ( ( R != UTF8_LINE_END ) && ( R != UTF8_DECODE_ERROR ) )
	{
		sFTChar Ch;
		Ch.FGlyph     = NULL;
		Ch.FCacheNode = NULL;
		Ch.FIndex   = ( FT_UInt )( -1 );
		Ch.FAdvance = 0;
		Ch.FWidth   = 0;
		Ch.FChar    = R;

		FString.push_back( Ch );

		R = DecodeNextUTF8Char();
	}

	return ( R != UTF8_DECODE_ERROR );
}

/*
    strict UTF-8 decoder for continuations not more than 4 bytes

    Code     | Cont |  Min  |    Max
    ==================================
    0xxxxxxx |   0  |     0 |     127
    110xxxxx |   1  |   128 |    2047
    1110xxxx |   2  |  2048 |   65535 \ {55296..57343}
    11110xxx |   3  | 65536 | 1114111
*/

int clTextRenderer::DecodeNextUTF8Char()
{
	// the first byte of the character and the result
	int c, r;

	if ( FIndex >= FLength ) { return FIndex == FLength ? UTF8_LINE_END : UTF8_DECODE_ERROR; }

	c = NextUTF8();

	// Zero continuation (0 to 127)
	if ( ( c & 0x80 ) == 0 ) { return c; }

	// First contination (128 to 2047)
	if ( ( c & 0xE0 ) == 0xC0 )
	{
		int c1 = ContUTF8();

		if ( c1 < 0 ) { return UTF8_DECODE_ERROR; }

		r = ( ( c & 0x1F ) << 6 ) | c1;
		return r >= 128 ? r : UTF8_DECODE_ERROR;
	}

	// Second continuation (2048 to 55295 and 57344 to 65535)
	if ( ( c & 0xF0 ) == 0xE0 )
	{
		int c1 = ContUTF8();
		int c2 = ContUTF8();

		if ( c1 < 0 || c2 < 0 ) { return UTF8_DECODE_ERROR; }

		r = ( ( c & 0x0F ) << 12 ) | ( c1 << 6 ) | c2;
		return r >= 2048 && ( r < 55296 || r > 57343 ) ? r : UTF8_DECODE_ERROR;
	}

	// Third continuation (65536 to 1114111)
	if ( ( c & 0xF8 ) == 0xF0 )
	{
		int c1 = ContUTF8();
		int c2 = ContUTF8();
		int c3 = ContUTF8();

		if ( c1 < 0 || c2 < 0 || c3 < 0 ) { return UTF8_DECODE_ERROR; }

		r = ( ( c & 0x0F ) << 18 ) | ( c1 << 12 ) | ( c2 << 6 ) | c3;
		return r >= 65536 && r <= 1114111 ? r : UTF8_DECODE_ERROR;
	}

	return UTF8_DECODE_ERROR;
}

void clTextRenderer::InitFreeType()
{
	FInitialized = LoadFT();

	if ( FInitialized )
	{
		FInitialized = false;

		if ( FT_Init_FreeTypePTR( &FLibrary ) != 0 ) { return; }

		if ( FTC_Manager_NewPTR( FLibrary, 0, 0, 0, FreeType_Face_Requester, this, &FManager ) != 0 ) { return; }

		if ( FTC_ImageCache_NewPTR( FManager, &FImageCache ) != 0 ) { return; }

		if ( FTC_CMapCache_NewPTR( FManager, &FCMapCache ) != 0 ) { return; }

		FInitialized = true;
	}
}

void clTextRenderer::StopFreeType()
{
	FreeString();

	// deallocate font handles and release font buffers
	for ( std::map<std::string, void*>::iterator p = FAllocatedFonts.begin(); p != FAllocatedFonts.end() ; p++ )
	{
		// ? FT_FreeFace (FFontFaceHandles[p->first]);
		delete[] ( char* )( p->second );
	}

	FFontFaces.clear();

	if ( FManager ) { FTC_Manager_DonePTR( FManager ); }

	if ( FLibrary ) { FT_Done_FreeTypePTR( FLibrary ); }
}

#pragma endregion
// end of init code

FT_Error clTextRenderer::LoadFontFile( const std::string& FileName )
{
	if ( !FInitialized ) { return -1; }

	if ( FAllocatedFonts.count( FileName ) > 0 ) { return 0; }

	clPtr<clBlob> DataBlob = g_FS->LoadFileAsBlob( FileName );
	int DataSize = DataBlob->GetSize();

	char* Data = new char[DataSize];
	memcpy( Data, DataBlob->GetData(), DataSize );

	FT_Face TheFace;

	// 0 is the face index
	FT_Error Result = FT_New_Memory_FacePTR( FLibrary, ( FT_Byte* )Data, ( FT_Long )DataSize, 0, &TheFace );

	if ( Result == 0 )
	{
		FFontFaceHandles[FileName] = TheFace;

		FAllocatedFonts[FileName] = ( void* )Data;

		FFontFaces.push_back( FileName );
	}

	return Result;
}

FT_Error clTextRenderer::FreeType_Face_Requester( FTC_FaceID FaceID, FT_Library Library, FT_Pointer RequestData, FT_Face* TheFace )
{
#ifdef _WIN64
	long long int Idx = ( long long int )FaceID;
	int FaceIdx = ( int )( Idx & 0xFF );
#else
	int FaceIdx = reinterpret_cast< int >( FaceID );
#endif

	if ( FaceIdx < 0 ) { return 1; }

	clTextRenderer* This = ( clTextRenderer* )RequestData;

	std::string FileName = This->FFontFaces[FaceIdx];

	FT_Error LoadResult = This->LoadFontFile( FileName );

	*TheFace = ( LoadResult == 0 ) ? This->FFontFaceHandles[FileName] : NULL;

	return LoadResult;
}

void clTextRenderer::FreeString()
{
	for ( size_t i = 0 ; i < FString.size() ; i++ )
	{
		if ( FString[i].FCacheNode != NULL )
		{
			FTC_Node_UnrefPTR( FString[i].FCacheNode, FManager );
			FString[i].FCacheNode = NULL;
		}
	}

	FString.clear();
}

inline void* IntToID( int FontID )
{
#ifdef _WIN64
	long long int Idx = FontID;
#else
	int Idx = FontID;
#endif
	FTC_FaceID ID = reinterpret_cast<void*>( Idx );

	return ID;
}

FT_UInt clTextRenderer::GetCharIndex( int FontID, FT_UInt Char )
{
	/// Default charmap causes severe problems on Mac OS. There is another default encoding
	return FTC_CMapCache_LookupPTR( FCMapCache, IntToID( FontID ), -1 /* use default cmap */, Char );
}

FT_Glyph clTextRenderer::GetGlyph( int FontID, int Height, FT_UInt Char, FT_UInt LoadFlags, FTC_Node* CNode )
{
	FT_UInt Index = GetCharIndex( FontID, Char );

	FTC_ImageTypeRec ImageType;

	ImageType.face_id = IntToID( FontID );
	ImageType.height = Height;
	ImageType.width = 0;
	ImageType.flags = LoadFlags;

	FT_Glyph Glyph;

	if ( FTC_ImageCache_LookupPTR( FImageCache, &ImageType, Index, &Glyph, CNode ) != 0 ) { return NULL; }

	return Glyph;
}

void clTextRenderer::SetAdvance( sFTChar& Char )
{
	Char.FAdvance = Char.FWidth = 0;

	if ( !Char.FGlyph ) { return; }

	Char.FAdvance = Char.FGlyph->advance.x >> 10;

	FT_BBox bbox;
	FT_Glyph_Get_CBoxPTR( Char.FGlyph, FT_GLYPH_BBOX_GRIDFIT, &bbox );
	Char.FWidth = bbox.xMax;

	// use advance as the width for empty glyphs (like space)
	if ( Char.FWidth == 0 && Char.FAdvance != 0 ) { Char.FWidth = Char.FAdvance; }
}

void clTextRenderer::Kern( sFTChar& Left, const sFTChar& Right )
{
	if ( Left.FIndex == -1 || Right.FIndex == -1 ) { return; }

	FT_Vector Delta;
	FT_Get_KerningPTR( FFace, Left.FIndex, Right.FIndex, FT_KERNING_DEFAULT, &Delta );

	Left.FAdvance += Delta.x;
}

FT_Face clTextRenderer::GetSizedFace( int FontID, int Height )
{
	FTC_ScalerRec Scaler;

	Scaler.face_id = IntToID( FontID );
	Scaler.height = Height;
	Scaler.width = 0;
	Scaler.pixel = 1;

	FT_Size SizedFont;

	if ( FTC_Manager_LookupSizePTR( FManager, &Scaler, &SizedFont ) != 0 ) { return NULL; }

	if ( FT_Activate_SizePTR( SizedFont ) != 0 ) { return NULL; }

	return SizedFont->face;
}

int clTextRenderer::GetFontHandle( const std::string& FileName )
{
	if ( LoadFontFile( FileName ) != 0 )
	{
		return -1;
	}

	for ( size_t i = 0 ; i != FFontFaces.size() ; i++ )
	{
		if ( FFontFaces[i] == FileName ) { return ( int )i; }
	}

	return -1;
}

bool clTextRenderer::LoadStringWithFont( const std::string& S, int ID, int Height )
{
	if ( ID < 0 ) { return false; }

	// 1. Get the font face
	FFace = GetSizedFace( ID, Height );

	if ( FFace == NULL ) { return false; }

	bool UseKerning = ( FT_HAS_KERNING( FFace ) > 0 );

	// 2. Decode utf8 string
	DecodeUTF8( S.c_str() );

	// 3. Calculate character sizes
	for ( size_t i = 0, count = FString.size(); i != count; i++ )
	{
		sFTChar& Char = FString[i];
		FT_UInt ch = Char.FChar;

		Char.FIndex = ( ch != '\r' && ch != '\n' ) ? GetCharIndex( ID, ch ) : -1;
		Char.FGlyph = ( Char.FIndex != -1 ) ? GetGlyph( ID, Height, ch, FT_LOAD_RENDER, &Char.FCacheNode ) : NULL;

		if ( !Char.FGlyph || Char.FIndex == -1 ) { continue; }

		SetAdvance( Char );

		if ( i > 0 && UseKerning ) { Kern( FString[i - 1], Char ); }
	}

	return true;
}

void clTextRenderer::CalculateLineParameters( int* Width, int* MinY, int* MaxY, int* BaseLine ) const
{
	int StrMinY = -1000;
	int StrMaxY = -1000;

	if ( FString.empty() )
	{
		StrMinY = 0;
		StrMaxY = 0;
	}

	int SizeX = 0;

	for ( size_t i = 0 ; i != FString.size(); i++ )
	{
		if ( FString[i].FGlyph == NULL ) { continue; }

		FT_BitmapGlyph BmpGlyph = ( FT_BitmapGlyph )FString[i].FGlyph;

		SizeX += FString[i].FAdvance;

		int Y = BmpGlyph->top;
		int H = BmpGlyph->bitmap.rows;

		if ( Y     > StrMinY ) { StrMinY = Y; }

		if ( H - Y > StrMaxY ) { StrMaxY = H - Y; }
	}

	if ( Width    ) { *Width = ( SizeX >> 6 ); }

	if ( BaseLine ) { *BaseLine = StrMaxY; }

	if ( MinY     ) { *MinY = StrMinY; }

	if ( MaxY     ) { *MaxY = StrMaxY; }
}

void clTextRenderer::RenderLineOnBitmap( const std::string& TextString, int FontID, int FontHeight, int StartX, int Y, unsigned int Color, bool LeftToRight, const clPtr<clBitmap>& Out )
{
	LoadStringWithFont( TextString, FontID, FontHeight );

	int x = StartX << 6;

	for ( size_t j = 0 ; j != FString.size(); j++ )
	{
		if ( FString[j].FGlyph != 0 )
		{
			FT_BitmapGlyph BmpGlyph = ( FT_BitmapGlyph ) FString[j].FGlyph;

			int in_x = ( x >> 6 ) + ( ( LeftToRight ? 1 : -1 ) * BmpGlyph->left );

			if ( !LeftToRight )
			{
				in_x += BmpGlyph->bitmap.width;
				in_x = StartX + ( StartX - in_x );
			}

			DrawGlyphOnBitmap( Out, &BmpGlyph->bitmap, in_x, Y - BmpGlyph->top /*- 5*/, Color );
		}

		x += FString[j].FAdvance;
	}
}

/// Create appropriate bitmap and render text line using specified font
clPtr<clBitmap> clTextRenderer::RenderTextWithFont( const std::string& TextString, int FontID, int FontHeight, unsigned int Color, bool LeftToRight )
{
	if ( !LoadStringWithFont( TextString, FontID, FontHeight ) ) { return NULL; }

	int W, Y;
	int MinY, MaxY;
	CalculateLineParameters( &W, &MinY, &MaxY, &Y );
	int H2 = MaxY + MinY;

	clPtr<clBitmap> Result = new clBitmap( W, H2, L_BITMAP_BGRA8 );
	Result->Clear();

	RenderLineOnBitmap( TextString, FontID, FontHeight, LeftToRight ? 0 : W - 1, MinY, Color, LeftToRight, Result );

	Result->Rescale( Linderdaum::Math::GetNextPowerOf2( W ), Linderdaum::Math::GetNextPowerOf2( H2 ) );

	return Result;
}

/// Multiply each component of the int-encoded color by Mult
inline unsigned int MultColor( unsigned int Color, unsigned int Mult )
{
	return ( ( unsigned int )Mult << 24 ) | Color;
}

void clTextRenderer::DrawGlyphOnBitmap( const clPtr<clBitmap>& Out, FT_Bitmap* Bitmap, int X0, int Y0, unsigned int Color ) const
{
	int W = Out->GetWidth();
	int Width = W - X0;

	if ( Width > Bitmap->width ) { Width = Bitmap->width; }

	for ( int Y = Y0 ; Y < Y0 + Bitmap->rows ; ++Y )
	{
		unsigned char* Src = Bitmap->buffer + ( Y - Y0 ) * Bitmap->pitch;

		for ( int X = X0 + 0 ; X < X0 + Width ; X++ )
		{
			unsigned char OutMaskCol = *Src++;

			Out->SetPixel( X, Y, LVector4i( OutMaskCol ) );
		}
	}
}

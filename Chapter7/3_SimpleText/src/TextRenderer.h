#ifndef __TextRenderer__h__included__
#define __TextRenderer__h__included__

#include "iObject.h"

class Bitmap;

#include <map>
#include <cstring>
#include <string>
#include <vector>

#include "ft.h"

class TextRenderer
{
public:
	TextRenderer();
	virtual ~TextRenderer() { StopFreeType(); }

	/// Check if the FreeType library is initialized
	bool IsInitialized() const { return FInitialized; }

	/// Loads required files and changes intenal buffers
	int GetFontHandle( const std::string& FileName );

	/// Load a single line of text to internal buffers and calculate individual character sizes/positions
	bool LoadStringWithFont( const std::string& TextString, int FontID, int FontHeight );

	/// Calculate bounding box and the base line for the loaded text string (in pixels)
	void CalculateLineParameters( int* Width, int* MinY, int* MaxY, int* BaseLine ) const;

	/**
	   \brief Render a single line of text using the specified font

	   \param StartX     - starting pixel for the text line
	   \param Y          - base vertical position for the text
	   \param Color      - RGBA color for symbols
	   \param Out        - output bitmap
	*/
	void RenderLineOnBitmap( const std::string& TextString, int FontID, int FontHeight, int StartX, int Y, unsigned int Color, bool LeftToRight, const clPtr<Bitmap>& Out );

	/// Create appropriate bitmap and render text line using specified font
	clPtr<Bitmap> RenderTextWithFont( const std::string& TextString, int FontID, int FontHeight, unsigned int Color, bool LeftToRight );

	/// Render alpha-blended pixels or only the alpha channel
	bool FMaskMode;

private:
	/// List of available font faces
	std::vector<std::string> FFontFaces;

	#pragma region FreeType internals

	/// Internal flag to check the FreeType state
	bool FInitialized;

	/// Local instance of the library (for thread-safe execution)
	FT_Library FLibrary;

	/// Cache manager
	FTC_Manager FManager;

	/// Glyph cache
	FTC_ImageCache FImageCache;

	/// Character map cache
	FTC_CMapCache FCMapCache;

	/// List of buffers with loaded font files. Map is used to prevent multiple file reads
	std::map<std::string, void*> FAllocatedFonts;

	/// List of initialized font face handles
	std::map<std::string, FT_Face> FFontFaceHandles;

	/// Load FreeType dll and initialize local library instance with caches
	void InitFreeType();

	/// Decrement reference count to FreeType dll and destroy local library instance
	void StopFreeType();

	/// Handle for the font face
	FT_Face FFace;

	/**
	   \brief Internal representation of a single freetype character

	   Copied from UTF-8/ASCII strings by setting the 'FChar' field
	*/
	struct sFTChar
	{
		/// UCS2(4) character, suitable for FreeType
		FT_UInt FChar;

		/// Internal character index
		FT_UInt FIndex;

		/// Handle for the rendered glyph
		FT_Glyph FGlyph;

		/// Fixed-point character advance and character size
		FT_F26Dot6 FAdvance, FWidth;

		/// Cache node for this glyph
		FTC_Node FCacheNode;

		/// Default parameters
		sFTChar() : FChar( 0 ), FIndex( ( FT_UInt )( -1 ) ), FGlyph( NULL ), FAdvance( 0 ), FWidth( 0 ), FCacheNode( NULL ) { }
	};

	/// Adjust current glyph's advance using information about adjacent symbols
	void Kern( sFTChar& Left, const sFTChar& Right );

	/// Get the width and advance information for the char
	void SetAdvance( sFTChar& Char );

	/// Release cache instances for string characters
	void FreeString();

	/// Internal face requester for FreeType (loads fonts from memory blocks)
	static FT_Error FreeType_Face_Requester( FTC_FaceID FaceID, FT_Library Library, FT_Pointer RequestData, FT_Face* TheFace );

	/// Get the char index in selected font. TODO: handle non-default charmaps for MacOS
	FT_UInt  GetCharIndex( int FontID, FT_UInt Char );

	/// Get the rendered glyph for Char using specified font and font height
	FT_Glyph GetGlyph( int FontID, int Height, FT_UInt Char, FT_UInt LoadFlags, FTC_Node* CNode );

	/// Construct a new face with specified dimension
	FT_Face  GetSizedFace( int FontID, int Height );

	/// Load font file and store it in internal list for later use
	FT_Error LoadFontFile( const std::string& FileName );

	/// Draw the single character on the image
	void DrawGlyphOnBitmap( const clPtr<Bitmap>& Out, FT_Bitmap* Bitmap, int X0, int Y0, unsigned int Color ) const;

	#pragma endregion

	#pragma region UTF8 decoding

	static const int UTF8_LINE_END = 0;
	static const int UTF8_DECODE_ERROR = -1;

	/// Buffer with the current string
	std::vector<sFTChar> FString;

	/// Fill the FString buffer (decode utf8 to FT_Uints)
	bool DecodeUTF8( const char* InStr );

	/// Current character index
	int  FIndex;

	/// Source buffer length
	int  FLength;

	/// Source buffer pointer
	const char* FBuffer;

	/// Current byte
	int  FByte;

	/// Gets the next byte. Returns UTF8_LINE_END if there are no more bytes.
	inline int NextUTF8() { return ( FIndex >= FLength ) ? UTF8_LINE_END : ( FBuffer[FIndex++] & 0xFF ); }

	/// Gets the low 6 bits of the next continuation byte. Returns UTF8_DECODE_ERROR if it is not a contination byte.
	inline int ContUTF8() { int c = NextUTF8(); return ( ( c & 0xC0 ) == 0x80 ) ? ( c & 0x3F ) : UTF8_DECODE_ERROR; }

	/// Extract the next character (between 0 and 1114111) or return UTF8_END/UTF8_ERROR
	int DecodeNextUTF8Char();

	#pragma endregion
};

#endif

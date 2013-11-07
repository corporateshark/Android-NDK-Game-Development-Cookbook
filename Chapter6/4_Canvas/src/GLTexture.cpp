/**
 * \file GLTexture.cpp
 * \brief OpenGL textures
 * \version 0.5.91
 * \date 03/07/2010
 * \author Sergey Kosarevsky, 2005-2010
 * \author support@linderdaum.com http://www.linderdaum.com
 */

#include "Engine.h"

#include "GLTexture.h"

#include "LGL/LGL.h"

extern sLGLAPI* LGL3;

bool ChooseInternalFormat( const sBitmapParams& BMPRec, Lenum* Format, Lenum* InternalFormat )
{
	if ( BMPRec.FBitmapFormat == L_BITMAP_BGR8 )
	{
#if defined( ANDROID )
		*InternalFormat = GL_RGB;
		*Format = GL_RGB;
#else
		*InternalFormat = GL_RGB8;
		*Format = GL_BGR;
#endif
	}

	if ( BMPRec.FBitmapFormat == L_BITMAP_BGRA8 )
	{
#if defined( ANDROID )
		*InternalFormat = GL_RGBA;
		*Format = GL_RGBA;
#else
		*InternalFormat = GL_RGBA8;
		*Format = GL_BGRA;
#endif
	}

	// unknown format
	return false;
}

clGLTexture::clGLTexture()
	: FTexID( 0 )
	, FInternalFormat( 0 )
	, FFormat( 0 )
{
}

clGLTexture::~clGLTexture()
{
	if ( FTexID ) { LGL3->glDeleteTextures( 1, &FTexID ); }
}

void clGLTexture::Bind( int TextureUnit ) const
{
	LGL3->glActiveTexture( GL_TEXTURE0 + TextureUnit );
	LGL3->glBindTexture( GL_TEXTURE_2D, FTexID );
}

void clGLTexture::LoadFromBitmap( const clPtr<clBitmap>& B )
{
	if ( !FTexID )
	{
		LGL3->glGenTextures( 1, &FTexID );
	}

	ChooseInternalFormat( B->FBitmapParams, &FFormat, &FInternalFormat );

	Bind( 0 );

	LGL3->glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	LGL3->glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

#if defined( ANDROID )
	LGL3->glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	LGL3->glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
#else
	LGL3->glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );
	LGL3->glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
	LGL3->glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER );
#endif

	LGL3->glTexImage2D( GL_TEXTURE_2D, 0, FInternalFormat, B->GetWidth(), B->GetHeight(), 0, FFormat, GL_UNSIGNED_BYTE, B->FBitmapData );
}

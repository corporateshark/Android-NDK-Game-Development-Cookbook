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

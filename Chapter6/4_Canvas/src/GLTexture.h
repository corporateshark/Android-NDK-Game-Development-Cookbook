/**
 * \file GLTexture.h
 * \brief OpenGL textures
 * \version 0.5.90
 * \date 04/02/2010
 * \author Sergey Kosarevsky, 2005-2010
 * \author support@linderdaum.com http://www.linderdaum.com
 */

#pragma once

#include "Platform.h"

#include "Bitmap.h"
#include "iIntrusivePtr.h"

class clGLTexture
{
public:
	clGLTexture();
	virtual ~clGLTexture();

	void    Bind( int TextureUnit ) const;
	void    LoadFromBitmap( const clPtr<clBitmap>& Bitmap );

private:
	Luint         FTexID;
	///
	Lenum         FInternalFormat;
	Lenum         FFormat;
};

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

#include "iObject.h"
#include <string>
#include "Blob.h"

#include "GLClasses.h"

enum LPhotoSize
{
   L_PHOTO_SIZE_128     = 0,
   L_PHOTO_SIZE_256     = 1,
   L_PHOTO_SIZE_512     = 2,
   L_PHOTO_SIZE_1024    = 3,
   L_PHOTO_SIZE_ORIGINAL = 4
};

/// States of the thumbnail/LODImage component
enum LImageState
{
   L_NOTSTARTED,
   L_LOADING,
   L_LOADED,
   L_ERROR
};

class iImageCreator;
class clImage;

/// State of the image
class sImageDescriptor: public iObject
{
public:
	/// Global ID
	size_t               FID;

	std::string          FURL; // or filename here

	/// Size code of the image
	LPhotoSize           FSize;

	/// State of the descriptor (L_NOTSTARTED at the beginning)
	LImageState          FState;

	clPtr<clGLTexture>   FTexture;

	clPtr<clBitmap> FNewBitmap;

	sImageDescriptor():
		FState( L_NOTSTARTED ),
		FSize( L_PHOTO_SIZE_128 )
	{
		FTexture = new clGLTexture();
	}

	void StartDownload( bool AsFullSize );

	void ImageDownloaded( clPtr<clBlob> Blob );

	void UpdateTexture();
};

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

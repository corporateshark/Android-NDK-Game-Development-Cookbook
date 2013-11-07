#pragma once

#include "iObject.h"
#include "ImageTypes.h"
#include "Picasa.h"

class Blob;

class clGallery: public iObject
{
public:
	clGallery(): FNoImagesList( true ) {}

	/// Get fullsize URL for this image
	std::string GetFullSizeURL( int Idx ) const
	{
		return ( Idx < ( int )FURLs.size() ) ? Picasa_GetDirectImageURL( FURLs[Idx], L_PHOTO_SIZE_ORIGINAL ) : std::string();
	}

	size_t            GetTotalImages() const { return FImages.size(); }
	clPtr<sImageDescriptor> GetImage( size_t Idx ) const { return ( Idx < FImages.size() ) ? FImages[Idx] : NULL; };

	void CancellAllDownloads();
	// restart downloading of all images that are not loaded
	void ResetAllDownloads();
	bool StartListDownload();

	/// Callback for start list download
	void ListDownloaded( clPtr<clBlob> Blob );
private:
	/// Internal state: loaded list of images
	bool FNoImagesList;

	/// Base URLs of the images
	std::vector<std::string> FURLs;

	/// Complete list of the images being loaded
	std::vector< clPtr<sImageDescriptor> > FImages;
};

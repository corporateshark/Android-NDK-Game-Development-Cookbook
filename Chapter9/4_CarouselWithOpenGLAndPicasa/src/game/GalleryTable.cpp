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

#include "GalleryTable.h"
#include "ImageTypes.h"

#include "Globals.h"

#include "Downloader.h"
#include "Blob.h"

class clListDownloadedCallback: public clDownloadCompleteCallback
{
public:
	explicit clListDownloadedCallback( const clPtr<clGallery>& G ): FGallery( G ) {}

	virtual void Invoke()
	{
		FGallery->ListDownloaded( FResult );
	}

	clPtr<clGallery> FGallery;
};

void clGallery::ListDownloaded( clPtr<clBlob> B )
{
	if ( B )
	{
		/// Parse the blob
		FURLs.clear();

		void*  Data     = B->GetData();
		size_t DataSize = B->GetSize();

		Picasa_ParseXMLResponse( std::string( ( char* )Data, DataSize ), FURLs );
	}
	else
	{
		FNoImagesList = true;
		return;
	}

	for ( size_t j = 0 ; j != FURLs.size() ; j++ )
	{
		printf( "URL[%d] = %s\n", ( int )j, FURLs[j].c_str() );
	}

	fflush( stdout );

	// теперь что-то создать в FImages ?
	FImages.clear(); // resize(FURLs.size());

	for ( size_t j = 0 ; j != FURLs.size() ; j++ )
	{
		LPhotoSize Size = L_PHOTO_SIZE_128;
		std::string ImgUrl = Picasa_GetDirectImageURL( FURLs[j], Size );

		clPtr<sImageDescriptor> Desc = new sImageDescriptor();
		Desc->FSize    = Size;
		Desc->FURL     = ImgUrl;
		Desc->FID      = j;

		FImages.push_back( Desc );

		// и запустить загрузку
		Desc->StartDownload( true );
	}

	FNoImagesList = false;
}

bool clGallery::StartListDownload()
{
	static const size_t ListID = 1234567890;

	g_Downloader->CancelLoad( ListID );

	// start url download
	clPtr<iTask> T = g_Downloader->DownloadURL( Picasa_ListURL, ListID, new clListDownloadedCallback( this ) );

	return false;
}

void clGallery::CancellAllDownloads()
{
	for ( size_t i = 0 ; i != FImages.size(); i++ )
	{
		sImageDescriptor* D = FImages[i].GetInternalPtr();

		if ( D->FState != L_LOADED )
		{
			g_Downloader->CancelLoad( ( size_t )D );
		}
	}
}

void clGallery::ResetAllDownloads()
{
	for ( size_t i = 0 ; i != FImages.size(); i++ )
	{
		sImageDescriptor* D = FImages[i].GetInternalPtr();

		if ( D->FState != L_LOADED ) { D->FState = L_NOTSTARTED; }
	}
}

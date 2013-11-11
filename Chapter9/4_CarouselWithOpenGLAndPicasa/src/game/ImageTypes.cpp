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

#include "ImageTypes.h"
#include "Globals.h"
#include "ImageLoadTask.h"

class clImageDownloadedCallback: public clDownloadCompleteCallback
{
public:
	explicit clImageDownloadedCallback( const clPtr<sImageDescriptor>& D ): FDesc( D ) {}

	virtual void Invoke()
	{
		FDesc->ImageDownloaded( FResult );

		FDesc = NULL;
	}

	clPtr<sImageDescriptor> FDesc;
};

class clImageLoadingComplete: public clImageLoadingCompleteCallback
{
public:
	explicit clImageLoadingComplete( const clPtr<sImageDescriptor>& d ): FDesc( d ) {}

	virtual void Invoke()
	{
		FDesc->FNewBitmap = FResult;

		// update texture
		FDesc->UpdateTexture();

		FDesc = NULL;
	}

	clPtr<sImageDescriptor> FDesc;
};

void sImageDescriptor::StartDownload( bool AsFullSize )
{
	if ( FState == L_LOADING || FState == L_LOADED ) { return; }

	FState = L_LOADING;

	clPtr<iIStream> In = g_FS->CreateReader( "NoImageAvailable.png" );
	clPtr<clImageLoadingCompleteCallback> CB = new clImageLoadingComplete ( this );

	clPtr<iTask> LoadTask = new clImageLoadTask( In, 0, CB, g_Events.GetInternalPtr() );

	g_Loader->AddTask( LoadTask );

	// task ID should be unique
	g_Downloader->CancelLoad( ( size_t )this );
	g_Downloader->DownloadURL( FURL, ( size_t )this, new clImageDownloadedCallback( this ) );
}

void sImageDescriptor::ImageDownloaded( clPtr<clBlob> B )
{
	if ( !B )
	{
		FState = L_ERROR;
		return;
	}

	// заменить на ещё один таск с колбеком, в котором делается Desc->FState = loaded и UpdateTexture()
	clPtr<clImageLoadingCompleteCallback> CB = new clImageLoadingComplete( this );
	clPtr<clImageLoadTask> LoadTask = new clImageLoadTask( B, 0, CB, g_Events.GetInternalPtr() );

	g_Loader->AddTask( LoadTask );
}

void sImageDescriptor::UpdateTexture()
{
	this->FState = L_LOADED;

	// update GLTexture instance
	FTexture->LoadFromBitmap( FNewBitmap );
}

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

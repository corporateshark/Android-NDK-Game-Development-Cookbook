#include "Globals.h"

#include "GalleryTable.h"
#include "ImageTypes.h"

#include "Downloader.h"
#include "Blob.h"

class clListDownloadedCallback: public clDownloadCompleteCallback
{
public:
	clListDownloadedCallback( const clPtr<clGallery>& G ): FGallery( G ) {}

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

		printf( "Data: %s\n", ( char* )Data );
	}
	else
	{
		FNoImagesList = true;
		return;
	}

	printf( "Done list fetch in GalleryTable::ListDownloadedCallback()\n" );

	for ( size_t j = 0 ; j != FURLs.size() ; j++ )
	{
		printf( "URL[%d] = %s\n", ( int )j, FURLs[j].c_str() );
	}

	fflush( stdout );

	// теперь что-то создать в FImages ?
	FImages.clear(); // resize(FURLs.size());

	printf( "creating descriptors\n" );
	fflush( stdout );

	for ( size_t j = 0 ; j != FURLs.size() ; j++ )
	{
		LPhotoSize Size = L_PHOTO_SIZE_256;
		std::string ImgUrl = Picasa_GetDirectImageURL( FURLs[j], Size );

		clPtr<sImageDescriptor> Desc = new sImageDescriptor();
		Desc->FSize    = Size;
		Desc->FURL     = ImgUrl;
		Desc->FID      = j;

		FImages.push_back( Desc );

		// и запустить загрузку
		Desc->StartDownload( true );
	}

	printf( "number of images: %d\n", ( int )FImages.size() );
	fflush( stdout );

	FNoImagesList = false;

	printf( "OK\n" );
	fflush( stdout );
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

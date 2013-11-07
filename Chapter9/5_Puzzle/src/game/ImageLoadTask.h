#pragma once

#include "iObject.h"
#include "WorkerThread.h"
#include "Event.h"
#include "Blob.h"
#include "Bitmap.h"
#include "Picasa.h"
#include "FileSystem.h"
#include "Globals.h"

class clImageLoadTask;

// TaskID, DownloadedDataBlob, ResponseCode
class clImageLoadingCompleteCallback: public iAsyncCapsule
{
public:
	virtual void Invoke() {}

	size_t FTaskID;
	clPtr<clBitmap> FResult;
	clPtr<clImageLoadTask> FTask;
};

class clImageLoadTask: public iTask
{
public:
	clImageLoadTask( const clPtr<clBlob>& B, size_t TaskID, const clPtr<clImageLoadingCompleteCallback>& CB, iAsyncQueue* CallbackQueue )
		: FSource( B ), FSourceStream( NULL ), FCallback( CB ), FCallbackQueue( CallbackQueue )
	{ SetTaskID( TaskID ); }

	clImageLoadTask( const clPtr<iIStream>& S, size_t TaskID, const clPtr<clImageLoadingCompleteCallback>& CB, iAsyncQueue* CallbackQueue )
		: FSource( NULL ), FSourceStream( S ), FCallback( CB ), FCallbackQueue( CallbackQueue )
	{ SetTaskID( TaskID ); }

	virtual void Run()
	{
		clPtr<clImageLoadTask> Guard( this );
		clPtr<iIStream> In = ( FSourceStream == NULL ) ? g_FS->ReaderFromBlob( FSource ) : FSourceStream;

		FResult = new clBitmap();
		FResult->Load2DImage( In, true );

		if ( FCallback )
		{
			FCallback->FTaskID = GetTaskID();
			FCallback->FResult = FResult;
			FCallback->FTask = this;
			FCallbackQueue->EnqueueCapsule( FCallback );
			FCallback = NULL;
		}
	}

	virtual clPtr<clBitmap> GetResult() const { return FResult; }
private:
	clPtr<iIStream> FSourceStream;
	clPtr<clBlob>   FSource;
	clPtr<clImageLoadingCompleteCallback> FCallback;
	iAsyncQueue* FCallbackQueue;
	clPtr<clBitmap> FResult;
};

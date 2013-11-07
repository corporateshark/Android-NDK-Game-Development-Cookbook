#include "Downloader.h"

#include "Blob.h"
#include "WorkerThread.h"
#include "Event.h"

static const size_t DownloadSizeLimit = 2 * 1024 * 1024;

clDownloader::clDownloader()
	: FTasksMutex()
{
	FDownloadThread = new clWorkerThread();
	FDownloadThread->Start( iThread::Priority_Normal );
}

clDownloader::~clDownloader()
{
	FDownloadThread->CancelAll();
	FDownloadThread->Exit( true );

	LMutex Lock( &FTasksMutex );

	FLoadedTasks.clear();
}

clPtr<clDownloadTask> clDownloader::DownloadURL( const std::string& URL, size_t TaskID, clDownloadCompleteCallback* CB )
{
	if ( !TaskID || !CB ) { return clPtr<clDownloadTask>(); }

	// DEBUG: "Downloading file: %s (%p)", URL.c_str(), TaskID

	clPtr<clDownloadTask> Task = new clDownloadTask( URL, TaskID, CB, this );

	FDownloadThread->AddTask( Task );

	return Task;
}

void clDownloader::CancelAll()
{
	FDownloadThread->CancelAll();

	LMutex Lock( &FTasksMutex );

	// clean already loaded tasks
	for ( int i = ( int )FLoadedTasks.size() - 1; i >= 0; --i )
	{
		FLoadedTasks[i]->Exit();
	}

	FLoadedTasks.clear();
}

bool clDownloader::CancelLoad( size_t TaskID )
{
	FDownloadThread->CancelTask( TaskID );

	LMutex Lock( &FTasksMutex );

	// clean already loaded tasks
	for ( size_t i = 0; i != FLoadedTasks.size(); i++ )
	{
		if ( FLoadedTasks[i]->GetTaskID() == TaskID )
		{
			FLoadedTasks[i]->Exit();

			if ( FLoadedTasks.size() ) { FLoadedTasks[i] = FLoadedTasks.back(); }

			FLoadedTasks.pop_back();

			return true;
		}
	}

	return false;
}

size_t clDownloader::GetNumDownloads() const
{
	return FDownloadThread->GetQueueSize();
}

class clPendingCallbacksProcessor: public iAsyncCapsule
{
public:
	explicit clPendingCallbacksProcessor( clDownloader* dwn )
	{
		FDownloader = dwn;
	}

	virtual void Invoke()
	{
		FDownloader->ProcessPendingCallbacks();
	}

private:
	clDownloader* FDownloader;
};

void clDownloader::CompleteTask( clPtr<clDownloadTask> Task )
{
	if ( !Task->IsPendingExit() )
	{
		{
			LMutex Lock( &FTasksMutex );

			FLoadedTasks.push_back( Task );
		}

		if ( FEventQueue ) { FEventQueue->EnqueueCapsule( new clPendingCallbacksProcessor( this ) ); }
	}
}

bool clDownloader::ProcessPendingCallbacks()
{
	while ( !FLoadedTasks.empty() )
	{
		// prevent deadlock
		clPtr<clDownloadTask> Task;
		{
			// we need this mutex to ensure block of Cancel while we are inside a callback
			LMutex Lock( &FTasksMutex );

			if ( !FLoadedTasks.empty() )
			{
				Task = FLoadedTasks.back();

				FLoadedTasks.pop_back();
			}
		}

		if ( Task ) { Task->InvokeCallback(); }
	}

	return true;
}

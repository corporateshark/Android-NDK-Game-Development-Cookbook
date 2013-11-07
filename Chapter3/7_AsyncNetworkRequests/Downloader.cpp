#include "Downloader.h"

#include "Blob.h"
#include "WorkerThread.h"
#include "Event.h"

static const size_t DownloadSizeLimit = 2 * 1024 * 1024;

Downloader::Downloader()
	: FTasksMutex()
{
	FDownloadThread = new WorkerThread();
	FDownloadThread->Start( iThread::Priority_Normal );
}

Downloader::~Downloader()
{
	FDownloadThread->CancelAll();
	FDownloadThread->Exit( true );

	LMutex Lock( &FTasksMutex );

	FLoadedTasks.clear();
}

clPtr<DownloadTask> Downloader::DownloadURL( const std::string& URL, size_t TaskID, DownloadCompleteCallback* CB )
{
	if ( !TaskID || !CB ) { return clPtr<DownloadTask>(); }

	// DEBUG: "Downloading file: %s (%p)", URL.c_str(), TaskID

	clPtr<DownloadTask> Task = new DownloadTask( URL, TaskID, CB, this );

	FDownloadThread->AddTask( Task );

	return Task;
}

void Downloader::CancelAll()
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

bool Downloader::CancelLoad( size_t TaskID )
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

size_t Downloader::GetNumDownloads() const
{
	return FDownloadThread->GetQueueSize();
}

class PendingCallbacksProcessor: public iAsyncCapsule
{
public:
	PendingCallbacksProcessor( Downloader* dwn )
	{
		FDownloader = dwn;
	}

	virtual void Invoke()
	{
		FDownloader->ProcessPendingCallbacks();
	}

	Downloader* FDownloader;
};

void Downloader::CompleteTask( clPtr<DownloadTask> Task )
{
	if ( !Task->IsPendingExit() )
	{
		{
			LMutex Lock( &FTasksMutex );

			FLoadedTasks.push_back( Task );
		}

		if ( FEventQueue ) { FEventQueue->EnqueueCapsule( new PendingCallbacksProcessor( this ) ); }
	}
}

bool Downloader::ProcessPendingCallbacks()
{
	while ( !FLoadedTasks.empty() )
	{
		// prevent deadlock
		clPtr<DownloadTask> Task;
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

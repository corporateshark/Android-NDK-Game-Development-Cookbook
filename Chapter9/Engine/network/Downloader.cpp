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

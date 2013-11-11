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

#include "DownloadTask.h"
#include "Downloader.h"

#include "Blob.h"

#include "CurlWrap.h"

static const size_t DownloadSizeLimit = 2 * 1024 * 1024;

clDownloadTask::clDownloadTask( const std::string& URL, size_t TaskID, clDownloadCompleteCallback* CB, clDownloader* Downloader )
	: FURL( URL ),
	  FCallback( CB ),
	  FCurlCode( 0 ),
	  FRespCode( 0 ),
	  FDownloader( Downloader )
{
	SetTaskID( TaskID );
	FResult = new clBlob();
}

size_t clDownloadTask::MemoryCallback( void* P, size_t Size, size_t NumMemBlocks, void* Data )
{
	clDownloadTask* T = ( clDownloadTask* )Data;

	if ( T->IsPendingExit() )
	{
		return 0;
	}

	const size_t DataSize = Size * NumMemBlocks;

	if ( T->FResult->GetSize() + DataSize > DownloadSizeLimit ) { return 0; }

	if ( !T->FResult->AppendBytes( P, DataSize ) )
	{
		return 0;
	}

	return DataSize;
}


int clDownloadTask::ProgressCallback( void* P, double TotalToDownload, double NowDownloaded, double TotalToUpload, double NowUploaded )
{
	clDownloadTask* T = ( clDownloadTask* )P;

	return ( T && T->IsPendingExit() );
}

void clDownloadTask::Run()
{
	clPtr<clDownloadTask> Guard( this );

	CURL* Curl = curl_easy_init_P();

	curl_easy_setopt_P( Curl, CURLOPT_URL, FURL.c_str() );
	curl_easy_setopt_P( Curl, CURLOPT_FOLLOWLOCATION, 1 );
	curl_easy_setopt_P( Curl, CURLOPT_NOPROGRESS, false );
	curl_easy_setopt_P( Curl, CURLOPT_FAILONERROR, true );
	curl_easy_setopt_P( Curl, CURLOPT_MAXCONNECTS, 10 );
	curl_easy_setopt_P( Curl, CURLOPT_MAXFILESIZE, DownloadSizeLimit );
	curl_easy_setopt_P( Curl, CURLOPT_WRITEFUNCTION, &MemoryCallback );
	curl_easy_setopt_P( Curl, CURLOPT_WRITEDATA, this );
	curl_easy_setopt_P( Curl, CURLOPT_PROGRESSFUNCTION, &ProgressCallback );
	curl_easy_setopt_P( Curl, CURLOPT_PROGRESSDATA, this );

	// Number of seconds to wait while trying to connect. Use 0 to wait indefinitely
	curl_easy_setopt_P( Curl, CURLOPT_CONNECTTIMEOUT, 30 );

	// Maximal number of seconds to allow CURL functions to execute
	curl_easy_setopt_P( Curl, CURLOPT_TIMEOUT, 60 );

	// SSL
	curl_easy_setopt_P( Curl, CURLOPT_SSL_VERIFYPEER, 0 );
	curl_easy_setopt_P( Curl, CURLOPT_SSL_VERIFYHOST, 0 );

	curl_easy_setopt_P( Curl, CURLOPT_HTTPGET, 1 );

	// start Curl
	FCurlCode = curl_easy_perform_P( Curl );

	curl_easy_getinfo_P( Curl, CURLINFO_RESPONSE_CODE, &FRespCode );

	curl_easy_cleanup_P( Curl );

	// DEBUG: "CurlCore = %i, RespCode = %i", FCurlCode, FRespCode

	if ( FDownloader ) { FDownloader->CompleteTask( this ); }
}

void clDownloadTask::InvokeCallback()
{
	LMutex Lock( &FExitingMutex );

	if ( !IsPendingExit() )
	{
		if ( FCurlCode != 0 )
		{
			FResult = NULL;
		}

		if ( FCallback )
		{
			FCallback->FTaskID = GetTaskID();
			FCallback->FResult = FResult;
			FCallback->FTask = clPtr<clDownloadTask>( this );
			FCallback->Invoke();
		}
	}
}

void clDownloadTask::Exit()
{
	LMutex Lock( &FExitingMutex );

	FCallback = NULL;

	iTask::Exit();
}

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

#ifndef _DownloadTask
#define _DownloadTask

#include "iObject.h"
#include "WorkerThread.h"
#include "Event.h"

class clBlob;
class clDownloader;
class clWorkerThread;
class clDownloadTask;

// TaskID, DownloadedDataBlob, ResponseCode
class clDownloadCompleteCallback: public iAsyncCapsule
{
public:
	virtual void Invoke() {}

	size_t FTaskID;
	clPtr<clBlob> FResult;
	clPtr<clDownloadTask> FTask;
};

class clDownloadTask: public iTask
{
public:
	// URL - URL of the resource to download
	// TaskID - unique task ID
	// Stream - optional
	clDownloadTask( const std::string& URL, size_t TaskID, clDownloadCompleteCallback* CB, clDownloader* Downloader );

	virtual void Run();
	virtual void Exit();

	virtual int           GetErrorCode() const { return FCurlCode; }
	virtual clPtr<clBlob> GetResult() const { return FResult; }

private:
	void InvokeCallback();

	friend class clDownloader;

	static size_t MemoryCallback( void* P, size_t Size, size_t NumMemBlocks, void* Data );
	static int    ProgressCallback( void* P, double TotalToDownload, double NowDownloaded, double TotalToUpload, double NowUploaded );

private:
	clMutex    FExitingMutex;
	// initialization
	std::string                 FURL;
	clDownloadCompleteCallback* FCallback;
	clDownloader*               FDownloader;
	// result
	clPtr<clBlob>   FResult;
	long            FCurlCode;
	long            FRespCode;
};

#endif

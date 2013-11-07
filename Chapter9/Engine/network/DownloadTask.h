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

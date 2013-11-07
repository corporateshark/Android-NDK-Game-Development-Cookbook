#ifndef _DownloadTask
#define _DownloadTask

#include "iObject.h"
#include "WorkerThread.h"
#include "Event.h"

class Blob;
class Downloader;
class WorkerThread;
class DownloadTask;

// TaskID, DownloadedDataBlob, ResponseCode
class DownloadCompleteCallback: public iAsyncCapsule
{
public:
	virtual void Invoke() {}

	size_t FTaskID;
	clPtr<Blob> FResult;
	clPtr<DownloadTask> FTask;
};

class DownloadTask: public iTask
{
public:
	// URL - URL of the resource to download
	// TaskID - unique task ID
	// Stream - optional
	DownloadTask( const std::string& URL, size_t TaskID, DownloadCompleteCallback* CB, Downloader* Downloader );

	virtual void Run();
	virtual void Exit();

	virtual int           GetErrorCode() const { return FCurlCode; }
	virtual clPtr<Blob> GetResult() const { return FResult; }

private:
	void InvokeCallback();

	friend class Downloader;

	static size_t MemoryCallback( void* P, size_t Size, size_t NumMemBlocks, void* Data );
	static int    ProgressCallback( void* P, double TotalToDownload, double NowDownloaded, double TotalToUpload, double NowUploaded );

private:
	Mutex    FExitingMutex;
	// initialization
	std::string               FURL;
	DownloadCompleteCallback* FCallback;
	Downloader*     FDownloader;
	// result
	clPtr<Blob>   FResult;
	long            FCurlCode;
	long            FRespCode;
};

#endif

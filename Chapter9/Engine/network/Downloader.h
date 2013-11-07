#ifndef _Downloader
#define _Downloader

#include "iObject.h"
#include "DownloadTask.h"
#include "WorkerThread.h"
#include "Mutex.h"

class iAsyncQueue;
class clPendingCallbacksProcessor;

class clDownloader: public iObject
{
public:
	clDownloader();
	virtual ~clDownloader();

	virtual clPtr<clDownloadTask> DownloadURL( const std::string& URL, size_t TaskID, clDownloadCompleteCallback* CB );

	virtual bool          CancelLoad( size_t TaskID );
	virtual void          CancelAll();
	virtual size_t        GetNumDownloads() const;

	// external event queue
	iAsyncQueue* FEventQueue;
private:
	void CompleteTask( clPtr<clDownloadTask> Task );
	virtual bool ProcessPendingCallbacks();

	friend class clDownloadTask;
	friend class clPendingCallbacksProcessor;
private:
	clMutex                              FTasksMutex;
	std::vector< clPtr<clDownloadTask> > FLoadedTasks;
	clPtr<clWorkerThread>                FDownloadThread;
};

#endif

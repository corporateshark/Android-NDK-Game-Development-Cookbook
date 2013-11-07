#ifndef _Downloader
#define _Downloader

#include "iObject.h"
#include "DownloadTask.h"
#include "WorkerThread.h"
#include "Mutex.h"

class iAsyncQueue;
class PendingCallbacksProcessor;

class Downloader: public iObject
{
public:
	Downloader();
	virtual ~Downloader();

	virtual clPtr<DownloadTask> DownloadURL( const std::string& URL, size_t TaskID, DownloadCompleteCallback* CB );

	virtual bool          CancelLoad( size_t TaskID );
	virtual void          CancelAll();
	virtual size_t        GetNumDownloads() const;

	// external event queue
	clPtr<iAsyncQueue> FEventQueue;
private:
	void CompleteTask( clPtr<DownloadTask> Task );
	virtual bool ProcessPendingCallbacks();

	friend class DownloadTask;
	friend class PendingCallbacksProcessor;
private:
	Mutex                              FTasksMutex;
	std::vector< clPtr<DownloadTask> > FLoadedTasks;
	clPtr<WorkerThread>                FDownloadThread;
};

#endif

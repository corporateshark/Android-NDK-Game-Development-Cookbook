#ifndef _WorkerThread_
#define _WorkerThread_

#include "iObject.h"
#include "Thread.h"
#include "tinythread.h"

#include "iIntrusivePtr.h"

#include <list>
#include <string>

class iTask: public iObject
{
public:
	iTask()
		: FIsPendingExit( false ),
		  FTaskID( 0 ),
		  FPriority( 0 ) {};

	virtual void Run() = 0;
	virtual void Exit() { FIsPendingExit = true; }
	virtual bool IsPendingExit() const volatile { return FIsPendingExit; }

	virtual void   SetTaskID( size_t ID ) { FTaskID = ID; };
	virtual size_t GetTaskID() const { return FTaskID; };

	virtual void   SetPriority( int Priority ) { FPriority = Priority; };
	virtual int    GetPriority() const { return FPriority; };

private:
	volatile bool           FIsPendingExit;
	size_t                  FTaskID;
	int                     FPriority;
};

class WorkerThread: public iThread
{
public:
	virtual void   AddTask( const clPtr<iTask>& Task );
	virtual bool   CancelTask( size_t ID );
	virtual void   CancelAll();
	virtual size_t GetQueueSize() const;

protected:
	virtual void Run();
	virtual void NotifyExit();

private:
	clPtr<iTask> ExtractTask();
	clPtr<iTask> FCurrentTask;

private:
	std::list< clPtr<iTask> >   FPendingTasks;
	tthread::mutex              FTasksMutex;
	tthread::condition_variable FCondition;
};

#endif

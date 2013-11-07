#include "WorkerThread.h"

#include <algorithm>

void clWorkerThread::NotifyExit()
{
	FCondition.notify_all();
}

void clWorkerThread::AddTask( const clPtr<iTask>& Task )
{
	tthread::lock_guard<tthread::mutex> Lock( FTasksMutex );

	// non-zero IDs should be unique
	if ( size_t ID = Task->GetTaskID() )
	{
		for ( std::list< clPtr<iTask> >::iterator i = FPendingTasks.begin(); i != FPendingTasks.end(); ++i )
		{
			// LASSERT( (*i)->GetTaskID() != ID );
		}
	}

	FPendingTasks.push_back( Task );

	FCondition.notify_all();
}

void clWorkerThread::CancelAll()
{
	// we have to ensure no callbacks will be invoked after this call
	tthread::lock_guard<tthread::mutex> Lock( FTasksMutex );

	if ( FCurrentTask ) { FCurrentTask->Exit(); }

	// Clear pending tasks
	for ( std::list< clPtr<iTask> > ::iterator i = FPendingTasks.begin() ; i != FPendingTasks.end() ; i++ )
	{
		( *i )->Exit();
	}

	FPendingTasks.clear();

	FCondition.notify_all();
}

bool ShouldRemove( const clPtr<iTask> T, size_t ID )
{
	if ( T->GetTaskID() == ID )
	{
		T->Exit();
		return true;
	}

	return false;
}

bool clWorkerThread::CancelTask( size_t ID )
{
	if ( !ID ) { return false; }

	tthread::lock_guard<tthread::mutex> Lock( FTasksMutex );

	if ( FCurrentTask && FCurrentTask->GetTaskID() == ID ) { FCurrentTask->Exit(); }

	std::list< clPtr<iTask> >::iterator first = FPendingTasks.begin();
	std::list< clPtr<iTask> >::iterator result = first;

	for ( ; first != FPendingTasks.end() ; ++first )
		if ( !ShouldRemove( *first, ID ) ) { *result++ = *first; }

	FPendingTasks.erase ( result, FPendingTasks.end( ) );

	FCondition.notify_all();

	return true;
}

clPtr<iTask> clWorkerThread::ExtractTask()
{
	tthread::lock_guard<tthread::mutex> Lock( FTasksMutex );

	while ( FPendingTasks.empty() && !IsPendingExit() )
	{
		FCondition.wait( FTasksMutex );
	}

	if ( FPendingTasks.empty() ) { return clPtr<iTask>(); }

	std::list< clPtr<iTask> >::iterator Best = FPendingTasks.begin();

	for ( std::list< clPtr<iTask> >::iterator i = FPendingTasks.begin(); i != FPendingTasks.end(); ++i )
	{
		if ( ( *i )->GetPriority() > ( *Best )->GetPriority() ) { Best = i; }
	}

	clPtr<iTask> T = *Best;

	FPendingTasks.erase( Best );

	return T;
}

size_t clWorkerThread::GetQueueSize() const
{
	return FPendingTasks.size() + ( FCurrentTask ? 1 : 0 );
}

void clWorkerThread::Run()
{
	FPendingExit = false;

	while ( !IsPendingExit() )
	{
		FCurrentTask = ExtractTask();

		if ( FCurrentTask && !FCurrentTask->IsPendingExit() )
		{
			FCurrentTask->Run();
		}

		// we need to reset current task since ExtractTask() is blocking operation and could take some time
		FCurrentTask = NULL;
	}
}

/*
 * 05/12/2012
     It's here
*/

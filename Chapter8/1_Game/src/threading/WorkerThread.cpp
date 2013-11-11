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

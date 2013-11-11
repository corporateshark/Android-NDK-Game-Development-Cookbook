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

#include "Thread.h"

#ifdef _WIN32
#  include <process.h>
#else
#  include <sched.h>
#  include <unistd.h>
#endif

iThread::iThread()
	: FThreadHandle( 0 ),
	  FPendingExit( false )
{
}

iThread::~iThread()
{
}

THREAD_CALL iThread::ThreadStaticEntryPoint( void* Ptr )
{
	iThread* Thread = reinterpret_cast<iThread*>( Ptr );

	if ( Thread )
	{
		Thread->Run();
	}

#ifdef _WIN32
	_endthreadex( 0 );
	return 0;
#else
	pthread_exit( 0 );
	return NULL;
#endif
}

void iThread::Start( LPriority Priority )
{
	void* ThreadParam = reinterpret_cast<void*>( this );

#ifdef _WIN32
	unsigned int ThreadID = 0;
	FThreadHandle = ( uintptr_t )_beginthreadex( NULL, 0, &ThreadStaticEntryPoint, ThreadParam, 0, &ThreadID );

	int P = THREAD_PRIORITY_IDLE;

	if ( Priority == Priority_Lowest      ) { P = THREAD_PRIORITY_LOWEST; }

	if ( Priority == Priority_Low         ) { P = THREAD_PRIORITY_BELOW_NORMAL; }

	if ( Priority == Priority_Normal      ) { P = THREAD_PRIORITY_NORMAL; }

	if ( Priority == Priority_High        ) { P = THREAD_PRIORITY_ABOVE_NORMAL; }

	if ( Priority == Priority_Highest     ) { P = THREAD_PRIORITY_HIGHEST; }

	if ( Priority == Priority_TimeCritical ) { P = THREAD_PRIORITY_TIME_CRITICAL; }

	SetThreadPriority( ( HANDLE )FThreadHandle, P );
#else
	pthread_create( &FThreadHandle, NULL, ThreadStaticEntryPoint, ThreadParam );
	pthread_detach( FThreadHandle );

	int SchedPolicy = SCHED_OTHER;

	int MaxP = sched_get_priority_max( SchedPolicy );
	int MinP = sched_get_priority_min( SchedPolicy );

	sched_param SchedParam;
	SchedParam.sched_priority = MinP + ( MaxP - MinP ) / ( Priority_TimeCritical - Priority + 1 );

	pthread_setschedparam( FThreadHandle, SchedPolicy, &SchedParam );
#endif
}

void iThread::Exit( bool Wait )
{
	FPendingExit = true;

	NotifyExit();

	if ( !Wait ) { return; }

	if ( GetCurrentThread() != FThreadHandle )
	{
#ifdef _WIN32
		WaitForSingleObject( ( HANDLE )FThreadHandle, INFINITE );
		CloseHandle( ( HANDLE )FThreadHandle );
#else
		pthread_join( FThreadHandle, NULL );
#endif
	}
}

native_thread_handle_t iThread::GetCurrentThread()
{
#if defined( _WIN32)
	return GetCurrentThreadId();
#elif defined( ANDROID )
	return gettid();
#else
	return pthread_self();
#endif
}

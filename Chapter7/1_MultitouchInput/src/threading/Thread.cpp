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

/*
 * 06/04/2009
     Initial implementation
*/

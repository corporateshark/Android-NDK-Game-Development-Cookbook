#include "Event.h"
#include <stdio.h>

iAsyncQueue::iAsyncQueue()
	: FDemultiplexerMutex()
	, FCurrentQueue( 0 )
	, FAsyncQueues( 2 )
	, FAsyncQueue( &FAsyncQueues[0] )
{
}

void iAsyncQueue::EnqueueCapsule( const clPtr<iAsyncCapsule>& Capsule )
{
	LMutex Mutex( &FDemultiplexerMutex );
	FAsyncQueue->push_back( Capsule );

	printf( "added capsule\n" );
}

void iAsyncQueue::DemultiplexEvents()
{
	CallQueue* LocalQueue = &FAsyncQueues[ FCurrentQueue ];

	// switch current queue
	{
		LMutex Lock( &FDemultiplexerMutex );

		FCurrentQueue = ( FCurrentQueue + 1 ) % 2;
		FAsyncQueue = &FAsyncQueues[ FCurrentQueue ];
	}

	// invoke callbacks
	for ( CallQueue::iterator i = LocalQueue->begin(); i != LocalQueue->end(); ++i )
	{
		( *i )->Invoke();
	}

	LocalQueue->clear();
}

#ifndef _Event__h_
#define _Event__h_

#include "Mutex.h"
#include "iObject.h"
#include "iIntrusivePtr.h"
#include <vector>

class iAsyncCapsule: public iObject
{
public:
	/// Run the method
	virtual void Invoke() = 0;
};

class AsyncQueue
{
public:
	AsyncQueue(): FDemultiplexerMutex()
		, FCurrentQueue( 0 )
		, FAsyncQueues( 2 )
		, FAsyncQueue( &FAsyncQueues[0] )
	{ }

	/// Put the event into the events queue
	virtual void    EnqueueCapsule( const clPtr<iAsyncCapsule>& Capsule )
	{
		LMutex Mutex( &FDemultiplexerMutex );
		FAsyncQueue->push_back( Capsule );
	}

	/// Events demultiplexer as described in Reactor pattern
	virtual void DemultiplexEvents()
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

private:
	size_t          FCurrentQueue;

	typedef std::vector< clPtr<iAsyncCapsule> > CallQueue;
	std::vector<CallQueue> FAsyncQueues;

	/// switched for shared non-locked access
	CallQueue*   FAsyncQueue;
	Mutex         FDemultiplexerMutex;
};

#endif

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

class iAsyncQueue
{
public:
	iAsyncQueue();

	/// Put the event into the events queue
	virtual void    EnqueueCapsule( const clPtr<iAsyncCapsule>& Capsule );

	/// Events demultiplexer as described in Reactor pattern
	virtual void    DemultiplexEvents();

private:
	size_t          FCurrentQueue;

	typedef std::vector< clPtr<iAsyncCapsule> > CallQueue;
	std::vector<CallQueue> FAsyncQueues;

	/// switched for shared non-locked access
	CallQueue*   FAsyncQueue;
	Mutex         FDemultiplexerMutex;
};

#endif

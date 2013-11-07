#include "Thread.h"
#include "Event.h"
#include <stdio.h>

class ResponseThread: public iThread, public AsyncQueue
{
public:
	virtual void Run() { while ( 1 ) { DemultiplexEvents(); } }
};

ResponseThread* Responder;

class TestCall: public iAsyncCapsule
{
public:
	virtual void Invoke() { printf( "Test\n" ); }
};

class RequestThread: public iThread
{
public:
	virtual void Run()
	{
		while ( 1 )
		{
			Responder->EnqueueCapsule( new TestCall() );
			Sleep( 1000 );
		}
	}
};

int main()
{
	( Responder = new ResponseThread() )->Start( iThread::Priority_Normal );
	( new RequestThread() )->Start( iThread::Priority_Normal );

	while ( 1 ) {}

	return 0;
}

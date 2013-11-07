#include "WorkerThread.h"

#include <stdio.h>

class TestTask: public iTask
{
public:
	virtual void Run()
	{
		printf( "Test\n" );
	}
};

int main()
{
	WorkerThread* wt = new WorkerThread();

	wt->Start( iThread::Priority_Normal );

	wt->AddTask( new TestTask() );
	wt->AddTask( new TestTask() );
	wt->AddTask( new TestTask() );

	// Wait for task completion
	while ( wt->GetQueueSize() > 0 ) {}

	fflush( stdout );

	return 0;
}

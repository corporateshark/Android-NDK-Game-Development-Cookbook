#ifndef __Mutex__h__included__
#define __Mutex__h__included__

#if !defined(_WIN32)
#  include <pthread.h>
#else
#  include <windows.h>
#endif

class clMutex
{
public:
	clMutex()
	{
#if defined( _WIN32 )
		InitializeCriticalSection( &TheCS );
#else
		pthread_mutex_init( &TheMutex, NULL );
#endif
	}

	~clMutex()
	{
#if defined( _WIN32)
		DeleteCriticalSection( &TheCS );
#else
		pthread_mutex_destroy( &TheMutex );
#endif
	}

	void Lock() const
	{
#if defined( _WIN32 )
		EnterCriticalSection( ( CRITICAL_SECTION* )&TheCS );
#else
		pthread_mutex_lock( &TheMutex );
#endif
	}

	void Unlock() const
	{
#if defined( _WIN32 )
		LeaveCriticalSection( ( CRITICAL_SECTION* )&TheCS );
#else
		pthread_mutex_unlock( &TheMutex );
#endif
	}

#if defined( _WIN32 )
	CRITICAL_SECTION TheCS;
#else
	mutable pthread_mutex_t TheMutex;
#endif
};

class LMutex
{
public:
	explicit LMutex( const clMutex* Mutex ) : FMutex( Mutex ) { FMutex->Lock(); };
	~LMutex() { FMutex->Unlock(); };
private:
	const clMutex* FMutex;
};

#endif

/**
 * \file Thread.h
 * \brief POSIX thread
 * \version 0.6.24
 * \date 05/12/2012
 * \author Viktor Latypov, 2009-2010
 * \author Sergey Kosarevsky, 2010-2012
 * \author support@linderdaum.com http://www.linderdaum.com
 */

#ifndef __iThread__h__included__
#define __iThread__h__included__

#ifndef _WIN32
#include <pthread.h>
typedef pthread_t thread_handle_t;
typedef pthread_t native_thread_handle_t;
#else
#include <windows.h>
typedef uintptr_t thread_handle_t;
typedef uintptr_t native_thread_handle_t;
#endif

/**
   Posix-based thread

   Linux implementation uses pthreads and win32/64 uses WinAPI to avoid external dependancies
*/
class iThread
{
public:
	enum LPriority
	{
	   Priority_Idle         = 0,
	   Priority_Lowest       = 1,
	   Priority_Low          = 2,
	   Priority_Normal       = 3,
	   Priority_High         = 4,
	   Priority_Highest      = 5,
	   Priority_TimeCritical = 6
	};
public:
	iThread();
	virtual ~iThread();

	/// start a thread
	void Start( LPriority Priority );
	void Exit( bool Wait );

	bool IsPendingExit() const { return FPendingExit; };

	static native_thread_handle_t GetCurrentThread();

protected:
	/// Worker routine
	virtual void Run() = 0;
	virtual void NotifyExit() {};

	thread_handle_t GetHandle() { return FThreadHandle; }

private:
#ifdef _WIN32
#  define THREAD_CALL unsigned int __stdcall
#else
#  define THREAD_CALL void*
#endif
	static THREAD_CALL ThreadStaticEntryPoint( void* Ptr );

private:
	volatile bool FPendingExit;
	thread_handle_t FThreadHandle;
};

#endif

/*
 * 16/06/2010
     Linux port
 * 06/04/2009
     Initial implementation
*/

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

protected:
	volatile bool FPendingExit;
	thread_handle_t FThreadHandle;
};

#endif

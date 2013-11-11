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

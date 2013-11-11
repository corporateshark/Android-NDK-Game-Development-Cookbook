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

#pragma once

#if defined(_WIN32)
#  include <windows.h>
#endif

namespace Atomic
{
	template <class T> inline T Inc( T* Value )
	{
#ifdef _WIN32
		return InterlockedIncrement( Value );
#else
		return __sync_fetch_and_add( Value, 1 );
#endif
	}

	template <class T> inline T Dec( T* Value )
	{
#ifdef _WIN32
		return InterlockedDecrement( Value );
#else
		return __sync_sub_and_fetch( Value, 1 );
#endif
	}

} // namespace Atomic

typedef unsigned char ubyte;

#ifdef _MSC_VER
typedef __int64          int64;
typedef unsigned __int64 uint64;
#else
#include <stdint.h>
typedef int64_t       int64;
typedef uint64_t      uint64;
#endif

/// Intrusive reference-countable object for garbage collection
class iObject
{
public:
	iObject(): FRefCounter( 0 ) {}
	virtual ~iObject() {}

	void    IncRefCount() { Atomic::Inc( &FRefCounter ); }

	void    DecRefCount() { if ( Atomic::Dec( &FRefCounter ) == 0 ) { delete this; } }

	long    GetReferenceCounter() const volatile { return FRefCounter; }

private:
	volatile long    FRefCounter;
};

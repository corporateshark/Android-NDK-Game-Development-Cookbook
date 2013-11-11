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

#ifndef NULL
#  define NULL 0
#endif

namespace LPtr
{
	void IncRef( void* p );
	void DecRef( void* p );
};

/// Intrusive smart pointer
template <class T> class clPtr
{
private:
	class clProtector
	{
	private:
		void operator delete( void* );
	};
public:
	/// default constructor
	clPtr(): FObject( 0 )
	{
	}
	/// copy constructor
	clPtr( const clPtr& Ptr ): FObject( Ptr.FObject )
	{
		LPtr::IncRef( FObject );
	}
	template <typename U> clPtr( const clPtr<U>& Ptr ): FObject( Ptr.GetInternalPtr() )
	{
		LPtr::IncRef( FObject );
	}
	/// constructor from T*
	clPtr( T* const Object ): FObject( Object )
	{
		LPtr::IncRef( FObject );
	}
	/// destructor
	~clPtr()
	{
		LPtr::DecRef( FObject );
	}
	/// check consistency
	inline bool IsValid() const
	{
		return FObject != 0;
	}
	/// assignment of clPtr
	clPtr& operator = ( const clPtr& Ptr )
	{
		T* Temp = FObject;
		FObject = Ptr.FObject;

		LPtr::IncRef( Ptr.FObject );
		LPtr::DecRef( Temp );

		return *this;
	}
	/// -> operator
	inline T* operator -> () const
	{
		return FObject;
	}
	/// allow "if ( clPtr )" construction
	inline operator clProtector* () const
	{
		if ( !FObject )
		{
			return NULL;
		}

		static clProtector Protector;

		return &Protector;
	}
	/// cast
	template <typename U> inline clPtr<U> DynamicCast() const
	{
		return clPtr<U>( dynamic_cast<U*>( FObject ) );
	}
	/// compare
	template <typename U> inline bool operator == ( const clPtr<U>& Ptr1 ) const
	{
		return FObject == Ptr1.GetInternalPtr();
	}
	template <typename U> inline bool operator == ( const U* Ptr1 ) const
	{
		return FObject == Ptr1;
	}
	template <typename U> inline bool operator != ( const clPtr<U>& Ptr1 ) const
	{
		return FObject != Ptr1.GetInternalPtr();
	}
	/// helper
	inline T* GetInternalPtr() const
	{
		return FObject;
	}
private:
	T*    FObject;
};

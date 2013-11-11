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
#include <vector>

/// Division remainder (wrap around for negative values)
inline int ModInt( int a, int b )
{
	int r = a % b;
	return ( r < 0 ) ? r + b : r;
}

// typesafe circular buffer
template <typename T> class RingBuffer
{
public:
	explicit RingBuffer( size_t Capacity ): FBuffer( Capacity )
	{ clear(); }

	inline void    push_back( const T& Value )
	{
		if ( FCount < FBuffer.size() ) { FCount++; }

		// fill linear buffer
		FBuffer[ FHead++ ] = Value;

		// wrap around
		if ( FHead == FBuffer.size() ) { FHead = 0; }
	}

	inline void clear() { FCount = FHead  = 0; }

	inline T* prev( size_t i )
	{
		return ( i >= FCount ) ? NULL : &FBuffer[ AdjustIndex( i ) ];
	}

	inline size_t AdjustIndex( size_t i ) const
	{
		return ( size_t )ModInt( ( int )FHead - ( int )i - 1, ( int )FBuffer.size() );
	}
private:
	std::vector<T> FBuffer;
	size_t     FCount, FHead;
};

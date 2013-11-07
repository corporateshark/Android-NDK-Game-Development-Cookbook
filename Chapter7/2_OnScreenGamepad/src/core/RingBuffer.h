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

#pragma once

#if defined(_WIN32)
#  include <windows.h>
#endif

/// Intrusive reference-countable object for garbage collection
class iObject
{
public:
	iObject(): FRefCounter( 0 ) {}
	virtual ~iObject() {}

	void    IncRefCount()
	{
#ifdef _WIN32
		InterlockedIncrement( &FRefCounter );
#else
		__sync_fetch_and_add( &FRefCounter, 1 );
#endif
	}

	void    DecRefCount()
	{
#ifdef _WIN32

		if ( InterlockedDecrement( &FRefCounter ) == 0 )
#else
		if ( __sync_sub_and_fetch( Value, 1 ) == 0 )
#endif
		{ delete this; }
	}

private:
	volatile long    FRefCounter;
};

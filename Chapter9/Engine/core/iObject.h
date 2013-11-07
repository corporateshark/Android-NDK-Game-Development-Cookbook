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

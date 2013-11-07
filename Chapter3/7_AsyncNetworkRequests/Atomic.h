#ifndef _Atomic_h_
#define _Atomic_h_

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

#endif

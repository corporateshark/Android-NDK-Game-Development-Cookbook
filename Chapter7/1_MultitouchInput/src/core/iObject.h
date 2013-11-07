#ifndef _iObject_
#define _iObject_

#if defined(_WIN32)
#  include <windows.h>
#endif

#define scriptmethod
#define noexport
#define netexportable
#define scriptfinal
#define TODO(X)
#define ERROR_MSG(X)

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

typedef int int32;
typedef unsigned int uint32;

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

namespace LPtr
{
	inline void IncRef( void* Obj )
	{
		if ( Obj ) { reinterpret_cast<iObject*>( Obj )->IncRefCount(); }
	}

	inline void DecRef( void* Obj )
	{
		if ( Obj ) { reinterpret_cast<iObject*>( Obj )->DecRefCount(); }
	}
};

#endif

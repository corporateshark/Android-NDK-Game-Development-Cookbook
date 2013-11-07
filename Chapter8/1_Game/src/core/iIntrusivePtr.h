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

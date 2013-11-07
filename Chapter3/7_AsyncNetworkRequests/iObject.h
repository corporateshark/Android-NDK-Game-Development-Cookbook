#ifndef _iObject_
#define _iObject_

#include "Atomic.h"

/// Intrusive reference-countable object for garbage collection
class iObject
{
public:
	iObject(): FRefCounter( 0 ) {}
	virtual ~iObject() {}

	void    IncRefCount()
	{
		Atomic::Inc( &FRefCounter );
	}

	void    DecRefCount()
	{
		// ASSERT( FRefCounter > 0 );
		if ( Atomic::Dec( &FRefCounter ) == 0 ) { delete this; }
	}

	long    GetReferenceCounter() const
	{
		return FRefCounter;
	}

	void    SetReferenceCounter( long C )
	{
		FRefCounter = C;
	}

private:
	volatile long    FRefCounter;
};

#endif

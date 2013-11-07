#include "iObject.h"

namespace LPtr
{
	void IncRef( void* Obj )
	{
		if ( Obj ) { reinterpret_cast<iObject*>( Obj )->IncRefCount(); }
	}

	void DecRef( void* Obj )
	{
		if ( Obj ) { reinterpret_cast<iObject*>( Obj )->DecRefCount(); }
	}
};

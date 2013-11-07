/**
 * \file iIntrusivePtr.cpp
 * \brief Intrusive smart pointer
 * \version 0.6.14
 * \date 26/08/2012
 * \author Sergey Kosarevsky, 2012
 * \author support@linderdaum.com http://www.linderdaum.com
 */

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

/*
 * 26/08/2012
     It's here
*/

#ifndef StrUtils_h_
#define StrUtils_h_

#include <string>

inline void Str_AddTrailingChar( std::string* Str, char Ch )
{
	if ( ( !Str->empty() ) && ( Str->data()[Str->length() - 1] == Ch ) ) { return; }

	Str->push_back( Ch );
}

#endif

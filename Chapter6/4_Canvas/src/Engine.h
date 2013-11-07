#include "Platform.h"
#include "Wrapper_Callbacks.h"

#if defined( _WIN32 )
#  include "Wrapper_Windows.h"
#else
#  include "Wrapper_Android.h"
#endif

#include "LGL.h"
#include "LGLAPI.h"

#include "GeomServ.h"
#include "GLSL.h"
#include "GLVertexArray.h"
#include "GLTexture.h"
#include "VertexAttribs.h"
#include "Canvas.h"
#include "FileSystem.h"

#include <string>

inline void Str_AddTrailingChar( std::string* Str, char Ch )
{
	if ( ( !Str->empty() ) && ( Str->data()[Str->length() - 1] == Ch ) ) { return; }

	Str->push_back( Ch );
}

inline std::string Str_ReplaceAllSubStr( const std::string& Str, const std::string& OldSubStr, const std::string& NewSubStr )
{
	std::string Result = Str;

	for ( size_t Pos = Result.find( OldSubStr ); Pos != std::string::npos; Pos = Result.find( OldSubStr ) )
	{
		Result.replace( Pos, OldSubStr.length(), NewSubStr );
	}

	return Result;
}

/*
 * Copyright (C) 2013 Sergey Kosarevsky (sk@linderdaum.com)
 * Copyright (C) 2013 Viktor Latypov (vl@linderdaum.com)
 * Based on Linderdaum Engine http://www.linderdaum.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must display the names 'Sergey Kosarevsky' and
 *    'Viktor Latypov'in the credits of the application, if such credits exist.
 *    The authors of this work must be notified via email (sk@linderdaum.com) in
 *    this case of redistribution.
 *
 * 3. Neither the name of copyright holders nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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
	bool HasLastChar = ( !Str->empty() ) && ( Str->data()[Str->length() - 1] == Ch );

	if ( !HasLastChar ) { Str->push_back( Ch ); }
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

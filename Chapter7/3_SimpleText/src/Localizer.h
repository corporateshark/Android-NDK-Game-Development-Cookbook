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

#include "Core/iObject.h"
#include <map>

extern clPtr<FileSystem> g_FS;

const std::string g_LocalePath = "Localizer";

class Localizer: public iObject
{
public:
	Localizer() {};
	virtual ~Localizer() {}

	void ::LoadLocale()
	{
		FTranslations.clear();
		const std::string FileName( g_LocalePath + "/Localizer-" + g_LocaleName + ".txt" );

		if ( g_FS->FileExists( FileName ) )
		{
			clPtr<iIStream> Stream = g_FS->CreateReader( FileName );

			while ( !Stream->Eof() )
			{
				std::string Line = Stream->ReadLine();
				size_t SepPos = Line.find( "~" );

				if ( SepPos == std::string::npos ) { continue; } // Invalid locale line - missing ~

				std::string Text( Line.substr( 0, SepPos ) );
				std::string Translation( Line.substr( SepPos + 1, Line.length() - SepPos - 1 ) );
				FTranslations[ Text ] = Translation;
			}
		}
	}

	std::string LocalizeString( const std::string& Str ) const
	{
		std::map<std::string, std::string>::const_iterator i = FTranslations.find( Str );
		return ( i != FTranslations.end() ) ? return i->second : Str;
	}
private:
	std::map<std::string, std::string> FTranslations;
};

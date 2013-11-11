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

#include "Picasa.h"
#include "ImageTypes.h"

std::string CleanPhotoAPI_ExtractURLAttribute( const std::string& InStr, const std::string& AttrName, char Delim )
{
	size_t AttrLen = AttrName.length();
	size_t pos = InStr.find( AttrName );
	size_t j;

	if ( pos != std::string::npos )
	{
		for ( j = pos + AttrLen ; j < InStr.size() ; j++ )
		{
			if ( InStr[j] == Delim ) { break; }
		}

		return InStr.substr( pos + AttrLen, j - pos - AttrLen );
	}

	return "";
}

void Picasa_ParseXMLResponse( const std::string& Response, std::vector<std::string>& URLs )
{
	size_t cur = 0;

	size_t sz = Response.length();

	while ( cur < sz )
	{
		size_t s_begin = Response.find( "<media:content ", cur );

		if ( s_begin == std::string::npos ) { break; }

		size_t s_end = Response.find( "/>", s_begin );

		if ( s_end == std::string::npos ) { break; }

		std::string new_s = Response.substr( s_begin, s_end - s_begin + 2 );

		URLs.push_back( CleanPhotoAPI_ExtractURLAttribute( new_s, "url=\'", '\'' ) );

		cur = s_end + 2;
	}
}

std::string Picasa_GetDirectImageURL( const std::string& InURL, int ImgSizeType )
{
	std::string Fmt = "";

	if ( ImgSizeType == L_PHOTO_SIZE_128     ) { Fmt = "/s128/"; }
	else if ( ImgSizeType == L_PHOTO_SIZE_256     ) { Fmt = "/s256/"; }
	else if ( ImgSizeType == L_PHOTO_SIZE_512     ) { Fmt = "/s512/"; }
	else if ( ImgSizeType == L_PHOTO_SIZE_1024    ) { Fmt = "/s1024/"; }
	else if ( ImgSizeType == L_PHOTO_SIZE_ORIGINAL ) { Fmt = "/s1600/"; };

	size_t spos = InURL.find( "/s1600/" );

	if ( spos == std::string::npos ) { return ""; }

	return InURL.substr( 0, spos ) + Fmt + InURL.substr( spos + 7, InURL.length() - spos - 7 );
}

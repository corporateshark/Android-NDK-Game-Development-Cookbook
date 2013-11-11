/*
 * Copyright (C) 2013 Sergey Kosarevsky (sk@linderdaum.com)
 * Copyright (C) 2013 Viktor Latypov (vl@linderdaum.com)
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

#if _MSC_VER >= 1400
#  define Lvsnprintf _vsnprintf_s
#  define Lsnprintf  _snprintf_s
#else
#  define Lvsnprintf vsnprintf
#  define Lsnprintf  snprintf
#endif

#include <string>
#include <vector>
#include <stdio.h>

enum PhotoSize
{
   PHOTO_SIZE_128     = 0,
   PHOTO_SIZE_256     = 1,
   PHOTO_SIZE_512     = 2,
   PHOTO_SIZE_1024    = 3,
   PHOTO_SIZE_ORIGINAL = 4
};

std::string IntToStr( int FromUInt32 )
{
	char buf[64];

	Lsnprintf( buf, 63, "%i", FromUInt32 );

	return std::string( buf );
}

static const std::string AppKey = "YourAppKeyHere";
static const std::string FlickrAPIBase = "http://api.flickr.com/services/rest/?method=";

static const std::string FlickrFavoritesURL = FlickrAPIBase + "flickr.interestingness.getList";
static const std::string FlickrRecentURL    = FlickrAPIBase + "flickr.photos.getRecent";

static const std::string PicasaAPIBase = "http://picasaweb.google.com/data/feed/api/";

static const std::string PicasaFavoritesURL = PicasaAPIBase + "featured/?";
static const std::string PicasaRecentURL    = PicasaAPIBase + "all/?";

std::string ExtractURLAttribute( const std::string& InStr, const std::string& AttrName, char Delim )
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

std::string Flickr_GetListURL( const std::string& BaseURL, int MaxResults, int PageIndex, const std::string& SearchQuery )
{
	std::string Result = BaseURL + std::string( "&api_key=" );

	Result += AppKey;

	if ( !SearchQuery.empty() ) { Result += std::string( "&q=\"" ) + SearchQuery + std::string( "\"" ); }

	Result += std::string( "&per_page=" );

	Result += IntToStr( MaxResults );

	if ( PageIndex > -1 ) { Result += std::string( "&page=" ) + IntToStr( PageIndex + 1 ); }

	return Result;
}

void Flickr_ParseXMLResponse( const std::string& Response, std::vector<std::string>& URLs )
{
	size_t begin = Response.find( "<photos" );

	if ( begin == std::string::npos ) { return; }

	begin = Response.find_first_of( '>', begin );

	if ( begin == std::string::npos ) { return; }

	size_t end = Response.find( "/photos>" );

	if ( end == std::string::npos ) { return; }

	size_t cur = begin;

	size_t ResLen = Response.length();

	while ( cur < ResLen )
	{
		size_t s_begin = Response.find( "<photo", cur );

		if ( s_begin == std::string::npos ) { break; }

		size_t s_end = Response.find( "/>", s_begin );

		if ( s_end == std::string::npos ) { break; }

		std::string Part = Response.substr( s_begin, s_end - s_begin + 2 );

		URLs.push_back( Part );

		cur = s_end + 2;
	}
}

std::string Flickr_GetDirectImageURL( const std::string& InURL, int ImgSizeType )
{
	std::string id     = ExtractURLAttribute( InURL, "id=\"", '"' );
	std::string secret = ExtractURLAttribute( InURL, "secret=\"", '"' );
	std::string server = ExtractURLAttribute( InURL, "server=\"", '"' );
	std::string farm   = ExtractURLAttribute( InURL, "farm=\"", '"' );

	std::string Res = std::string( "http://farm" ) + farm + std::string( ".staticflickr.com/" ) + server + std::string( "/" ) + id + std::string( "_" ) + secret;

	std::string Fmt = "";

	if ( ImgSizeType == PHOTO_SIZE_128     ) { Fmt = "t"; }
	else if ( ImgSizeType == PHOTO_SIZE_256     ) { Fmt = "m"; }
	else if ( ImgSizeType == PHOTO_SIZE_512     ) { Fmt = "-"; }
	else if ( ImgSizeType == PHOTO_SIZE_1024    ) { Fmt = "b"; }
	// hack
	else if ( ImgSizeType == PHOTO_SIZE_ORIGINAL ) { Fmt = "b"; };

	return Res + std::string( "_" ) + Fmt + std::string( ".jpg" );
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

		URLs.push_back( ExtractURLAttribute( new_s, "url=\'", '\'' ) );

		cur = s_end + 2;
	}
}

std::string Picasa_GetListURL( const std::string& BaseURL, int MaxResults, int PageIndex, const std::string& SearchQuery )
{
	std::string Result = BaseURL;

	Result +=  std::string( "kind=photo&imgmax=1600" );

	if ( !SearchQuery.empty() ) { Result += std::string( "&q=\"" ) + SearchQuery + std::string( "\"" ); }

	Result += std::string( "&max-results=" );
	Result += IntToStr( MaxResults );

	if ( PageIndex > 0 ) { Result += std::string( "&start-index=" ) + IntToStr( ( int )( 1 + PageIndex * MaxResults ) ); }

	return Result;
}

std::string Picasa_GetDirectImageURL( const std::string& InURL, int ImgSizeType )
{
	std::string Fmt = "";

	if ( ImgSizeType == PHOTO_SIZE_128     ) { Fmt = "/s128/"; }
	else if ( ImgSizeType == PHOTO_SIZE_256     ) { Fmt = "/s256/"; }
	else if ( ImgSizeType == PHOTO_SIZE_512     ) { Fmt = "/s512/"; }
	else if ( ImgSizeType == PHOTO_SIZE_1024    ) { Fmt = "/s1024/"; }
	else if ( ImgSizeType == PHOTO_SIZE_ORIGINAL ) { Fmt = "/s1600/"; };

	size_t spos = InURL.find( "/s1600/" );

	if ( spos == std::string::npos ) { return ""; }

	return InURL.substr( 0, spos /*-1*/ ) + Fmt + InURL.substr( spos + 7, InURL.length() - spos - 7 );
}

#include <iostream>

int main()
{
	// 1. declare an URL list
	std::vector<std::string> URLs;

	// 2. test data from Flickr
	const char* contents =
	   "<?xml version=\"1.0\" encoding=\"utf-8\" ?>"
	   "<rsp stat=\"ok\">"
	   "<photos page=\"1\" pages=\"67\" perpage=\"15\" total=\"1000\">"
	   "	<photo id=\"7304601612\" owner=\"18908739@N08\" secret=\"47376af466\" server=\"7231\" farm=\"8\" title=\"IMG_5041\" ispublic=\"1\" isfriend=\"0\" isfamily=\"0\" />"
	   "	<photo id=\"7304601676\" owner=\"63959892@N00\" secret=\"a44a1ab917\" server=\"7089\" farm=\"8\" title=\"IMG_20100509_0134\" ispublic=\"1\" isfriend=\"0\" isfamily=\"0\" />"
	   "</photos>"
	   "</rsp>";

	Flickr_ParseXMLResponse( std::string( contents ), URLs );

	for ( size_t j = 0 ; j != URLs.size() ; j++ ) { std::cout << "URL[" << j << "] = " << URLs[j] << std::endl; }

	return 0;
}

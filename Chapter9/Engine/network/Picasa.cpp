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

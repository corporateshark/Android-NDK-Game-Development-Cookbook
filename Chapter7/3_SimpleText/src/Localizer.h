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

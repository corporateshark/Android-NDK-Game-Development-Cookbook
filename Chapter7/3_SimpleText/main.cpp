#include "TextRenderer.h"
#include "Bitmap.h"
#include "bmp.c"

#include "Rendering.h"
#include "FileSystem.h"

#include <stdio.h>

#include <map>

const std::string g_LocalePath = "Localizer";
std::string g_LocaleName;
std::string g_ExternalStorage;

clPtr<FileSystem> g_FS;
clPtr<TextRenderer > g_TextRenderer;

std::map<std::string, std::string> g_Translations;

void LoadLocale()
{
	g_Translations.clear();
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
			g_Translations[ Text ] = Line.substr( SepPos + 1 );
		}
	}
}

std::string LocalizeString( const std::string& Str )
{
	auto i = g_Translations.find( Str );
	return ( i != g_Translations.end() ) ? i->second : Str;
}

void OnStart( const std::string& RootPath )
{
	g_FrameBuffer = ( unsigned char* )malloc( ImageWidth * ImageHeight * 4 );
	memset( g_FrameBuffer, 0xFF, ImageWidth * ImageHeight * 4 );

	g_FS = new FileSystem();
	g_FS->Mount( "." );
#if defined(ANDROID)
	g_FS->Mount( RootPath );
	g_FS->AddAliasMountPoint( RootPath, "assets" );
#endif

	g_TextRenderer = new TextRenderer();
}

void OnDrawFrame()
{
	int fnt = g_TextRenderer->GetFontHandle( "default.ttf" );

	g_TextRenderer->FMaskMode = true;

	unsigned char tt1[] = {'D', 'i', 'r', 'e', 0xC3, 0xA7, 0xC3, 0xA3, 'o', 0 };

	char tt2[] = "Русский текст";

	auto bmp = g_TextRenderer->RenderTextWithFont( ( const char* )tt2, fnt, 24, 0xFFFFFFFF, true );

	if ( bmp )
	{
		for ( int y = 0 ; y < bmp->FHeight ; y++ )
		{
			memcpy( g_FrameBuffer + 4 * ( 100 - y ) * ImageWidth, bmp->FBitmapData + y * bmp->FWidth * 4, bmp->FWidth * 4 );
		}
	}
}

void OnKeyUp( int w ) {}

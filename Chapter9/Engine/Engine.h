#pragma once

#include "Platform.h"
#include "Wrapper_Callbacks.h"

#if defined( _WIN32 )
#  include "Wrapper_Windows.h"
#else
#  include "Wrapper_Android.h"
#endif

#include "LGL.h"
#include "LGLAPI.h"

#include "Geometry.h"
#include "GLClasses.h"
#include "Canvas.h"
#include "FileSystem.h"
#include "Bitmap.h"
#include "Audio.h"
#include "OGG.h"
#include "MOD.h"
#include "Gestures.h"
#include "TextRenderer.h"
#include "GUI.h"
#include "iIntrusivePtr.h"

#include <string>

double GetSeconds();
void GenerateTicks();

void Str_AddTrailingChar( std::string* Str, char Ch );

std::string Str_ReplaceAllSubStr( const std::string& Str, const std::string& OldSubStr, const std::string& NewSubStr );
void Str_PadLeft( std::string* Str, size_t Len, char Pad );
std::string Str_GetPadLeft( const std::string& Str, size_t Len, char Pad );
std::string Str_GetFormatted( const char* Pattern, ... );
std::string Str_ToStr( int i );

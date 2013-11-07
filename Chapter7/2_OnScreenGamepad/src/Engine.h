#pragma once

#include "Wrapper_Callbacks.h"

#if defined( _WIN32 )
#  include "Wrapper_Windows.h"
#else
#  include "Wrapper_Android.h"
#endif

#include <string>

double GetSeconds();
void GenerateTicks();

void Str_AddTrailingChar( std::string* Str, char Ch );

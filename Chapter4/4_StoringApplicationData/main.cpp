#include <stdlib.h>

#if defined( ANDROID )
#  include "Wrapper_Android.h"
#else
#  include "Wrapper_Windows.h"
#endif

#include <string>

std::string g_ExternalStorage;

void OnStart()
{
	LOGI( "External storage path: %s", g_ExternalStorage.c_str() );
}

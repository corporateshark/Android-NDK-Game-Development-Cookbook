#include "Wrapper_Windows.h"

#include <string>
#include <shlobj.h>

extern std::string g_ExternalStorage;

int main()
{
	CHAR MyDocumentsPath[MAX_PATH];
	HRESULT result = SHGetFolderPath( NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, MyDocumentsPath );

	g_ExternalStorage = std::string( MyDocumentsPath ) + "\\ndkcookbook\\App6";

	OnStart();

	return 0;
}

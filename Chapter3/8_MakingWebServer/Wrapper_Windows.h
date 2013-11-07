#ifndef _Wrapper_Windows_h_
#define _Wrapper_Windows_h_

#include <windows.h>

#include "Wrapper_Callbacks.h"

#define LOGI(...)

LRESULT CALLBACK MyFunc( HWND h, UINT msg, WPARAM w, LPARAM p );

#endif

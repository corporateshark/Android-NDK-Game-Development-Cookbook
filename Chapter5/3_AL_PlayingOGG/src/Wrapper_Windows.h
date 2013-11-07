#ifndef _Wrapper_Windows_h_
#define _Wrapper_Windows_h_

#include <windows.h>

#include "Wrapper_Callbacks.h"

#define LOGI(...) printf(__VA_ARGS__)

LRESULT CALLBACK MyFunc( HWND h, UINT msg, WPARAM w, LPARAM p );

#endif

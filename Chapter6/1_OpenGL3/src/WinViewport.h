#pragma once

#include <windows.h>

class WinViewport
{
public:

	WinViewport( int W, int H, const char* Title, const char* WndClassName, WNDPROC WndProc, bool Show ) : Width( W ), Height( H )
	{
		WNDCLASS wcl;
		memset( &wcl, 0, sizeof( WNDCLASS ) );
		wcl.lpszClassName = WndClassName;
		wcl.lpfnWndProc = WndProc;
		wcl.hCursor = LoadCursor( NULL, IDC_ARROW );

		RegisterClass( &wcl );

		hWnd = CreateWindowA( WndClassName, Title, WS_OVERLAPPEDWINDOW, 100, 100, W, H, 0, NULL, NULL, NULL );
		DeviceContext = GetDC( hWnd );

		if ( Show )
		{
			ShowWindow( hWnd, SW_SHOW );
			UpdateWindow( hWnd );
		}
	}

	virtual ~WinViewport()
	{
		ReleaseDC( hWnd, DeviceContext );
		DestroyWindow( hWnd );
	}

	virtual HDC GetDeviceContext() const
	{
		return DeviceContext;
	}

	virtual int GetWidth() const
	{
		return Width;
	}

	virtual int GetHeight() const
	{
		return Height;
	}

private:
	int Width;
	int Height;
	HWND hWnd;
	HDC DeviceContext;
};

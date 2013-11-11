/*
 * Copyright (C) 2013 Sergey Kosarevsky (sk@linderdaum.com)
 * Copyright (C) 2013 Viktor Latypov (vl@linderdaum.com)
 * Based on Linderdaum Engine http://www.linderdaum.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must display the names 'Sergey Kosarevsky' and
 *    'Viktor Latypov'in the credits of the application, if such credits exist.
 *    The authors of this work must be notified via email (sk@linderdaum.com) in
 *    this case of redistribution.
 *
 * 3. Neither the name of copyright holders nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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

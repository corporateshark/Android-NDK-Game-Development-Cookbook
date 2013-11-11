/*
 * Copyright (C) 2013 Sergey Kosarevsky (sk@linderdaum.com)
 * Copyright (C) 2013 Viktor Latypov (vl@linderdaum.com)
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

#include <windows.h>

LRESULT CALLBACK MyFunc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	if ( message == WM_DESTROY ) { PostQuitMessage( 0 ); }

	return DefWindowProc( hwnd, message, wParam, lParam );
}

char WinName[] = "MyWin";

// entry point, different from Android
int main()
{
	// 1. Register window class
	WNDCLASS wcl;
	memset( &wcl, 0, sizeof( WNDCLASS ) );
	wcl.lpszClassName = WinName;
	// setup window procedure
	wcl.lpfnWndProc = MyFunc;
	// default cursor
	wcl.hCursor = LoadCursor( NULL, IDC_ARROW );

	if ( !RegisterClass( &wcl ) ) { return 0; }

	// 2. Create the standard dialog window
	HWND hWnd = CreateWindowA( WinName, "Min_Win1",
	                           WS_OVERLAPPEDWINDOW, 100, 100, 500, 500,
	                           0, NULL, NULL, NULL );

	ShowWindow( hWnd, SW_SHOW );
	UpdateWindow( hWnd );

	// 3. start the message loop - hidden in Android
	MSG msg;

	while ( GetMessage( &msg, NULL, 0, 0 ) )
	{
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

	// 4. exit the program with return code
	return msg.wParam;
}

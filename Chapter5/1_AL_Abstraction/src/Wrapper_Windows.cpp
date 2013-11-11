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

#include "Wrapper_Windows.h"

HDC hMemDC;
HBITMAP hTmpBmp;
BITMAPINFO BitmapInfo;

LRESULT CALLBACK MyFunc( HWND h, UINT msg, WPARAM w, LPARAM p )
{
	HDC dc;
	PAINTSTRUCT ps;
	int x = ( ( int )( short )LOWORD( p ) ), y = ( ( int )( short )HIWORD( p ) );

	switch ( msg )
	{
		case WM_KEYUP:
			if ( w == 27 ) { SendMessage( h, WM_DESTROY, 0, 0 ); } // OnKeyUp( w );

			break;

		case WM_KEYDOWN:
			OnKeyDown( w );
			break;

		case WM_LBUTTONDOWN:
			SetCapture( h );
			OnMouseDown( 0, x, y );
			break;

		case WM_MOUSEMOVE:
			OnMouseMove( x, y );
			break;

		case WM_LBUTTONUP:
			OnMouseUp( 0, x, y );
			ReleaseCapture();
			break;

		case WM_DESTROY:
			PostQuitMessage( 0 );
			break;

		case WM_PAINT:
			OnDrawFrame();
			dc = BeginPaint( h, &ps );
			// transfer the g_FrameBuffer to the hTmpBmp
			SetDIBits( hMemDC, hTmpBmp, 0, ImageHeight, g_FrameBuffer, &BitmapInfo, DIB_RGB_COLORS );
			SelectObject( hMemDC, hTmpBmp );
			// Copying the offscreen buffer to the window surface
			BitBlt( dc, 0, 0, ImageWidth, ImageHeight, hMemDC, 0, 0, SRCCOPY );
			EndPaint( h, &ps );
			break;

		case WM_TIMER:
			InvalidateRect( h, NULL, 1 );
			break;
	}

	return DefWindowProc( h, msg, w, p );
}

int main()
{
	OnStart( "." );

	const char WinName[] = "MyWin";

	WNDCLASS wcl;
	memset( &wcl, 0, sizeof( WNDCLASS ) );
	wcl.lpszClassName = WinName;
	wcl.lpfnWndProc = MyFunc;
	wcl.hCursor = LoadCursor( NULL, IDC_ARROW );

	if ( !RegisterClass( &wcl ) ) { return 0; }

	RECT Rect;

	Rect.left = 0;
	Rect.top = 0;
	Rect.right  = ImageWidth;
	Rect.bottom = ImageHeight;

	DWORD dwStyle = WS_OVERLAPPEDWINDOW;

	AdjustWindowRect( &Rect, dwStyle, false );

	int WinWidth  = Rect.right  - Rect.left;
	int WinHeight = Rect.bottom - Rect.top;

	HWND hWnd = CreateWindowA( WinName, "App9", dwStyle, 100, 100, WinWidth, WinHeight, 0, NULL, NULL, NULL );

	ShowWindow( hWnd, SW_SHOW );

	HDC dc = GetDC( hWnd );

	// Create the offscreen device context and buffer
	hMemDC = CreateCompatibleDC( dc );
	hTmpBmp = CreateCompatibleBitmap( dc, ImageWidth, ImageHeight );

	// filling the RGB555 bitmap header
	memset( &BitmapInfo.bmiHeader, 0, sizeof( BITMAPINFOHEADER ) );
	BitmapInfo.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
	BitmapInfo.bmiHeader.biWidth = ImageWidth;
	BitmapInfo.bmiHeader.biHeight = ImageHeight;
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 32;
	BitmapInfo.bmiHeader.biSizeImage = ImageWidth * ImageHeight * 4;

	UpdateWindow( hWnd );

	MSG msg;

	SetTimer( hWnd, 1, 10, NULL );

	while ( GetMessage( &msg, NULL, 0, 0 ) )
	{
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

	DeleteDC( hMemDC );
	DeleteObject( hTmpBmp );
	free( g_FrameBuffer );

	return msg.wParam;
}

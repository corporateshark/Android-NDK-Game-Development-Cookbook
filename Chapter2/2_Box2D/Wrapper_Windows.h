#ifndef _Wrapper_Windows_h_
#define _Wrapper_Windows_h_

#include <windows.h>

#include "Wrapper_Callbacks.h"

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
			OnKeyUp( w );
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
	OnStart();

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

	HWND hWnd = CreateWindowA( WinName, "App4", dwStyle, 100, 100, WinWidth, WinHeight, 0, NULL, NULL, NULL );

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

#endif

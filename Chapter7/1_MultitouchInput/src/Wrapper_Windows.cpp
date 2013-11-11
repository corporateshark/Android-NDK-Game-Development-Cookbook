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

#include "Multitouch.h"
#include <stdio.h>

#include "windowsx.h"

#include "VecMath.h"

#include "Gestures.h"

/// Multitouch initialization

#if !defined(_MSC_VER)

#define SM_DIGITIZER            94
#define SM_MAXIMUMTOUCHES       95

#define TOUCHEVENTF_DOWN        0x0001
#define TOUCHEVENTF_MOVE        0x0002
#define TOUCHEVENTF_UP          0x0004
#define TOUCHEVENTF_PRIMARY     0x0010

#define WM_TOUCH                        0x0240

#define TOUCH_COORD_TO_PIXEL(l) ((l) / 100)

typedef struct _TOUCHINPUT
{
	LONG x;
	LONG y;
	HANDLE hSource;
	DWORD dwID;
	DWORD dwFlags;
	DWORD wMask;
	DWORD dwTime;
	ULONG_PTR dwExtraInfo;
	DWORD cxContact;
	DWORD cyContact;
} TOUCHINPUT, *PTOUCHINPUT;

#endif

typedef BOOL ( WINAPI* CloseTouchInputHandle_func )( HANDLE hTouchInput );
typedef BOOL ( WINAPI* GetTouchInputInfo_func )( HANDLE hTouchInput, UINT cInputs, PTOUCHINPUT pInputs, int cbSize );
typedef BOOL ( WINAPI* RegisterTouchWindow_func )( HWND hWnd, ULONG ulFlags );
typedef BOOL ( WINAPI* UnregisterTouchWindow_func )( HWND hWnd );

static CloseTouchInputHandle_func CloseTouchInputHandle_Ptr = NULL;
static GetTouchInputInfo_func GetTouchInputInfo_Ptr = NULL;
static RegisterTouchWindow_func RegisterTouchWindow_Ptr = NULL;
static UnregisterTouchWindow_func UnregisterTouchWindow_Ptr = NULL;

// MinGW does not support TouchInput, we have to load it manually
static bool LoadTouchFuncs()
{
	if ( !CloseTouchInputHandle_Ptr )
	{
		HMODULE hUser = LoadLibraryA( "user32.dll" );

		CloseTouchInputHandle_Ptr = ( CloseTouchInputHandle_func )GetProcAddress( hUser, "CloseTouchInputHandle" );
		GetTouchInputInfo_Ptr = ( GetTouchInputInfo_func )GetProcAddress( hUser, "GetTouchInputInfo" );
		RegisterTouchWindow_Ptr = ( RegisterTouchWindow_func )GetProcAddress( hUser, "RegisterTouchWindow" );
		UnregisterTouchWindow_Ptr = ( UnregisterTouchWindow_func )GetProcAddress( hUser, "UnregisterTouchWindow" );
	}

	return ( RegisterTouchWindow_Ptr != NULL );
}

static POINT GetTouchPoint( HWND hWnd, const TOUCHINPUT& ti )
{
	POINT pt;
	pt.x = TOUCH_COORD_TO_PIXEL( ti.x );
	pt.y = TOUCH_COORD_TO_PIXEL( ti.y );
	ScreenToClient( hWnd, &pt );
	return pt;
}

/// End of Multitouch initialization

HDC hMemDC;
HBITMAP hTmpBmp;
BITMAPINFO BitmapInfo;

static bool g_TouchEnabled = false;

LRESULT CALLBACK MyFunc( HWND h, UINT msg, WPARAM wParam, LPARAM lParam )
{
	HDC dc;
	PAINTSTRUCT ps;
	int x = ( ( int )( short )LOWORD( lParam ) ), y = ( ( int )( short )HIWORD( lParam ) );

	switch ( msg )
	{
		case WM_CREATE:
		{
			g_TouchEnabled = false;
			BYTE DigitizerStatus = ( BYTE )GetSystemMetrics( SM_DIGITIZER );

			if ( ( DigitizerStatus & ( 0x80 + 0x40 ) ) != 0 ) //Stack Ready + MultiTouch
			{
				BYTE nInputs = ( BYTE )GetSystemMetrics( SM_MAXIMUMTOUCHES );

				if ( LoadTouchFuncs() )
				{
					if ( !RegisterTouchWindow_Ptr( h, 0 ) )
					{
						LOGI( "Enabled multitouch, num points: %d\n", ( int )nInputs );
						g_TouchEnabled = true;
						break;
					}
				}
			}

			LOGI( "Unable to use multitouch\n" );
			break;
		}

		case WM_TOUCH:
		{
			if ( !GetTouchInputInfo_Ptr || !CloseTouchInputHandle_Ptr ) { break; }

			// A WM_TOUCH Msg contains several messages from different contacts packed together.
			unsigned int NumInputs = ( int ) wParam; //Number of actual contact messages

			if ( NumInputs < 1 ) { break; }

			// Allocate the storage for the parameters of the per-contact messages
			TOUCHINPUT* ti = new TOUCHINPUT[NumInputs];

			// Unpack message parameters into the array of TOUCHINPUT structures, each
			// representing a message for one single contact.
			DWORD Res = GetTouchInputInfo_Ptr( ( HANDLE )lParam, NumInputs, ti, sizeof( TOUCHINPUT ) );

			double EventTime = Env_GetSeconds();

			if ( !Res ) { break; }

			// For each contact, update its status in internal array
			for ( unsigned int i = 0; i < NumInputs ; ++i )
			{
				POINT touch_pt = GetTouchPoint( h, ti[i] );
				vec2 Coord( ( float )touch_pt.x / ( float )ImageWidth, ( float )touch_pt.y / ( float )ImageHeight );
				sTouchPoint pt(
				   ti[i].dwID,
				   Coord,
				   L_MOTION_MOVE, EventTime );

				if ( ti[i].dwFlags & TOUCHEVENTF_DOWN ) { pt.FFlag = L_MOTION_DOWN; }

				if ( ti[i].dwFlags & TOUCHEVENTF_MOVE ) { pt.FFlag = L_MOTION_MOVE; }

				if ( ti[i].dwFlags & TOUCHEVENTF_UP )   { pt.FFlag = L_MOTION_UP;   }

				Viewport_UpdateTouchPoint( pt );
			}

			CloseTouchInputHandle_Ptr( ( HANDLE )lParam );
			delete[] ti;

			Viewport_ClearReleasedPoints();
			// then send the whole batch to event system
			Viewport_UpdateCurrentGesture();
			break;
		}

		case WM_KEYUP:
			if ( wParam == 27 ) { SendMessage( h, WM_DESTROY, 0, 0 ); }

			OnKeyUp( wParam );
			break;

		case WM_KEYDOWN:
			OnKeyDown( wParam );
			break;

		case WM_LBUTTONDOWN:
			Viewport_SendKey( LK_LBUTTON, true );
			SetCapture( h );
			OnMouseDown( 0, x, y );
			break;

		case WM_MOUSEMOVE:
		{
			int X = GET_X_LPARAM( lParam );
			int Y = GET_Y_LPARAM( lParam );

			if ( !g_TouchEnabled )
			{
				Viewport_MoveMouse( X, Y );

				LVector2 Pos( ( float )X / ( float )ImageWidth, ( float )Y / ( float )ImageHeight );

				// make the gestures' handlers happy
				GestureHandler_SendMotion( L_MOTION_START, L_MOTION_MOVE, LVector2(), false );
				GestureHandler_SendMotion( 0,            L_MOTION_MOVE, Pos, Viewport_IsKeyPressed( LK_LBUTTON ) );
				GestureHandler_SendMotion( L_MOTION_END,   L_MOTION_MOVE, LVector2(), false );
			}
		}
		break;

		case WM_LBUTTONUP:
			Viewport_SendKey( LK_LBUTTON, false );
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

	return DefWindowProc( h, msg, wParam, lParam );
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

	HWND hWnd = CreateWindowA( WinName, "App12", dwStyle, 100, 100, ImageWidth, ImageHeight, 0, NULL, NULL, NULL );

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

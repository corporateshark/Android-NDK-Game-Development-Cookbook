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

#include "Engine.h"

#include <stdio.h>

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

	virtual float GetAspectRatio() const
	{
		return static_cast<float>( Width ) / static_cast<float>( Height );
	};

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

LRESULT CALLBACK MyFunc( HWND h, UINT msg, WPARAM w, LPARAM p )
{
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
	}

	return DefWindowProc( h, msg, w, p );
}

extern sLGLAPI* LGL3;

int main()
{
	WinViewport* MainWindow = new WinViewport( 1280, 720, "OpenGL3", "MyWindowClass", &MyFunc, true );

	LGL3 = new sLGLAPI;

	LGL::clGLExtRetriever* OpenGL;
	OpenGL = new LGL::clGLExtRetriever;
	OpenGL->Reload( LGL3 );

	const int GLVerMajor = 3;
	const int GLVerMinor = 2;

	WinViewport* TempWindow = new WinViewport( 0, 0, "TempWindow", "TempWindowClass", &DefWindowProc, false );

	// create dummy context to get pointers to GL extensions functions
	GL_CONTEXT_TYPE TempContext  = OpenGL->CreateContextFull( LGL3, TempWindow->GetDeviceContext(), 32, 24, 0, 0, GLVerMajor, GLVerMinor );

	if ( !TempContext )
	{
		printf( "Cannot create TempContext\n" );
		return 0;
	}

	// create the main context
	GL_CONTEXT_TYPE Context  = OpenGL->CreateContextFull( LGL3, MainWindow->GetDeviceContext(), 32, 24, 0, 0, GLVerMajor, GLVerMinor );

	if ( !Context )
	{
		printf( "Cannot create GL context\n" );
		return 0;
	}

	// cleanup temp stuff
	OpenGL->DeleteContext( LGL3, TempWindow->GetDeviceContext(), TempContext );
	delete( TempWindow );
	OpenGL->MakeCurrent( LGL3, MainWindow->GetDeviceContext(), Context );
	OpenGL->Reload( LGL3 );

	delete( OpenGL );

	printf( "OpenGL version: %s\n", LGL3->glGetString( GL_VERSION ) );
	printf( "OpenGL renderer: %s\n", LGL3->glGetString( GL_RENDERER ) );
	printf( "OpenGL vendor: %s\n", LGL3->glGetString( GL_VENDOR ) );

	OnStart( "." );

	MSG msg;

	bool PendingExit = false;

	while ( !PendingExit )
	{
		while ( PeekMessage( &msg, 0, 0, 0, PM_NOREMOVE ) )
		{
			if ( !GetMessage( &msg, 0, 0, 0 ) ) { PendingExit = true; }

			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}

		OnDrawFrame();

		SwapBuffers( MainWindow->GetDeviceContext() );
	}

	delete( MainWindow );

	return msg.wParam;
}

#include "Engine.h"

#include <stdio.h>

const int WindowW = 960;
const int WindowH = 540;

volatile bool g_PendingExit = false;

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

		RECT Rect;

		Rect.left = 0;
		Rect.top = 0;
		Rect.right  = W;
		Rect.bottom = H;

		AdjustWindowRect( &Rect, WS_OVERLAPPEDWINDOW, false );

		const int WinWidth  = Rect.right  - Rect.left;
		const int WinHeight = Rect.bottom - Rect.top;

		hWnd = CreateWindowA( WndClassName, Title, WS_OVERLAPPEDWINDOW, 100, 100, WinWidth, WinHeight, 0, NULL, NULL, NULL );
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

	float Xf = ( float )x / ( float )WindowW;
	float Yf = ( float )y / ( float )WindowH;

	switch ( msg )
	{
		case WM_KEYUP:
			OnKey( w, false );
			break;

		case WM_KEYDOWN:
			OnKey( w, true );
			break;

		case WM_LBUTTONDOWN:
			SetCapture( h );
			OnMouseDown( 0, LVector2( Xf, Yf ) );
			break;

		case WM_MOUSEMOVE:
			OnMouseMove( LVector2( Xf, Yf ) );
			break;

		case WM_LBUTTONUP:
			OnMouseUp( 0, LVector2( Xf, Yf ) );
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
	WinViewport* MainWindow = new WinViewport( WindowW, WindowH, "OpenGL3", "MyWindowClass", &MyFunc, true );

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

	while ( !g_PendingExit )
	{
		while ( PeekMessage( &msg, 0, 0, 0, PM_NOREMOVE ) )
		{
			if ( !GetMessage( &msg, 0, 0, 0 ) ) { g_PendingExit = true; }

			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}

		GenerateTicks();

		SwapBuffers( MainWindow->GetDeviceContext() );
	}

	OnStop();

	delete( MainWindow );

	return msg.wParam;
}

void ExitApp()
{
	g_PendingExit = true;
}

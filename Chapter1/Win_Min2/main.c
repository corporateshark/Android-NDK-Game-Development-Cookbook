#include <windows.h>

HDC hMemDC;
HBITMAP hTmpBmp;
BITMAPINFO BitmapInfo;

const int ImageWidth = 128, ImageHeight = 128;

// rgba1555
unsigned char* g_FrameBuffer;

void DrawFrame()
{
	// make a simple "XOR" pattern
	int x, y;

	for ( y = 0 ; y < ImageHeight ; y++ )
		for ( x = 0 ; x < ImageWidth ; x++ )
		{
			int c = ( x ^ y ) & 0xFF;
			( ( unsigned short* )g_FrameBuffer )[ y * ImageWidth + x ] =
			   ( ( c >> 3 ) << 10 ) | ( ( c >> 3 ) << 5 ) | ( c >> 3 );
		}
}

void OnKeyUp( int code ) {}
void OnKeyDown( int code ) {}
void OnMouseDown( int btn, int x, int y ) {}
void OnMouseMove( int x, int y ) {}
void OnMouseUp( int btn, int x, int y ) {}

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
			DrawFrame();
			dc = BeginPaint( h, &ps );
			// transfer the g_FrameBuffer to the hTmpBmp
			SetDIBits( hMemDC, hTmpBmp, 0, ImageHeight,
			           g_FrameBuffer, &BitmapInfo, DIB_RGB_COLORS );
			SelectObject( hMemDC, hTmpBmp );
			// Copying the offscreen buffer to the window surface
			BitBlt( dc, 0, 0, ImageWidth, ImageHeight,
			        hMemDC, 0, 0, SRCCOPY );
			EndPaint( h, &ps );
	}

	return DefWindowProc( h, msg, w, p );
}

char WinName[] = "MyWin";

int main()
{
	WNDCLASS wcl;
	memset( &wcl, 0, sizeof( WNDCLASS ) );
	wcl.lpszClassName = WinName;
	wcl.lpfnWndProc = MyFunc;
	wcl.hCursor = LoadCursor( NULL, IDC_ARROW );

	if ( !RegisterClass( &wcl ) ) { return 0; }

	HWND hWnd = CreateWindowA( WinName, "Win_Min2",
	                           WS_OVERLAPPEDWINDOW, 100, 100, 200, 170,
	                           0, NULL, NULL, NULL );

	ShowWindow( hWnd, SW_SHOW );

	HDC dc = GetDC( hWnd );

	// Create the offscreen device context and buffer
	hMemDC = CreateCompatibleDC( dc );
	hTmpBmp = CreateCompatibleBitmap( dc, ImageWidth, ImageHeight );
	g_FrameBuffer = malloc( ImageWidth * ImageHeight * 2 );

	// filling the RGB555 bitmap header
	memset( &BitmapInfo.bmiHeader, 0, sizeof( BITMAPINFOHEADER ) );
	BitmapInfo.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
	BitmapInfo.bmiHeader.biWidth = ImageWidth;
	BitmapInfo.bmiHeader.biHeight = ImageHeight;
	BitmapInfo.bmiHeader.biPlanes = 1;
	BitmapInfo.bmiHeader.biBitCount = 16;
	BitmapInfo.bmiHeader.biSizeImage = ImageWidth * ImageHeight * 16;

	UpdateWindow( hWnd );

	MSG msg;

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

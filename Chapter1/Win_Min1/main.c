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

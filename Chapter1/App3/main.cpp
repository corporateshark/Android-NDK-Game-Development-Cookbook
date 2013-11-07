#include <stdlib.h>

#include "Wrapper_Callbacks.h"

#if defined( ANDROID )
#  include "Wrapper_Android.h"
#else
#  include "Wrapper_Windows.h"
#endif

// rgba1555
unsigned char* g_FrameBuffer;

void OnStart()
{
	g_FrameBuffer = ( unsigned char* )malloc( ImageWidth * ImageHeight * 4 );
}

void OnDrawFrame()
{
	// make a simple "XOR" pattern
	int x, y;

	for ( y = 0 ; y < ImageHeight ; y++ )
	{
		for ( x = 0 ; x < ImageWidth ; x++ )
		{
			int c = ( x ^ y ) & 0xFF;
			( ( unsigned int* )g_FrameBuffer )[ y * ImageWidth + x ] = ( c << 16 ) | ( c << 8 ) | ( c << 0 ) | 0xFF000000;
		}
	}
}

void OnKeyUp( int code )
{
}

void OnKeyDown( int code )
{
}

void OnMouseDown( int btn, int x, int y )
{
}

void OnMouseMove( int x, int y )
{
}

void OnMouseUp( int btn, int x, int y )
{
}

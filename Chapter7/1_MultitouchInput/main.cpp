#include <stdlib.h>
#include "Rendering.h"

#if defined( ANDROID )
#  include "Wrapper_Android.h"
#else
#  include "Wrapper_Windows.h"
#endif

#include "Multitouch.h"
#include "Gestures.h"
#include "Thread.h"

std::string g_ExternalStorage;

#include <time.h>
#include <unistd.h>

double Env_GetSeconds()
{
	return ( double )clock() / 1000.0;
}

void Env_Sleep( int Milliseconds )
{
#if defined _WIN32
	Sleep( Milliseconds );
#else
	// mu-sleep supports microsecond-precision
	usleep( static_cast<useconds_t>( Milliseconds ) * 1000 );
#endif
}

volatile bool g_Exit;

volatile bool IsPressed = false;

class GestureResponder: public iGestureResponder
{
public:
	virtual void Event_UpdateGesture( const sMotionData& M )
	{
		if ( !M.GetNumTouchPoints() )
		{
			memset( g_FrameBuffer, 0x00, ImageWidth * ImageHeight * 4 );
		}
		else
		{
			memset( g_FrameBuffer, 0xFF, ImageWidth * ImageHeight * 4 );
		}

		for ( size_t i = 0; i != M.GetNumTouchPoints(); i++ )
		{
			LVector2 P = M.GetTouchPointPos( i );

			P *= LVector2( ImageWidth, ImageHeight );

			Line( P.x - 30, P.y - 30, P.x + 30, P.y - 30, 0xFF0000 );
			Line( P.x - 30, P.y + 30, P.x + 30, P.y + 30, 0xFF0000 );
			Line( P.x - 30, P.y + 30, P.x - 30, P.y - 30, 0xFF0000 );
			Line( P.x + 30, P.y + 30, P.x + 30, P.y - 30, 0xFF0000 );
		}
	}
	virtual void Event_PointerMoved( int PointerID, const LVector2& Pnt )
	{
		LOGI( "Pointer(%d) move %f, %f\n", PointerID, Pnt.x, Pnt.y );
	};
} Responder;

void OnStart( const std::string& APKName )
{
	g_Exit = false;

	g_FrameBuffer = ( unsigned char* )malloc( ImageWidth * ImageHeight * 4 );
	memset( g_FrameBuffer, 0xFF, ImageWidth * ImageHeight * 4 );

	g_Responder = &Responder;
}

void OnDrawFrame()
{
}

void OnTimer( float Delta )
{
}

void OnKeyUp( int code )
{
	if ( code == 27 ) { g_Exit = true; }
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

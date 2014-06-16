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
	return ( double )clock() / CLOCKS_PER_SEC;
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

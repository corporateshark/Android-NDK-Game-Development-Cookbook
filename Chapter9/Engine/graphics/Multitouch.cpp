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

#include "Multitouch.h"
#include "Gestures.h"

#include <algorithm>

#if defined( ANDROID )
#  include "Wrapper_Android.h"
#else
#  include "Wrapper_Windows.h"
#endif

std::list<sTouchPoint> FTouchPoints;

int g_Mouse_X, g_Mouse_Y;

bool KeyPressed[256];

bool Viewport_IsKeyPressed( int Key )
{
	return KeyPressed[Key];
}

void Viewport_SendKey( int Key, bool Pressed )
{
	KeyPressed[ Key ] = Pressed;
}

void Viewport_MoveMouse( int X, int Y )
{
	g_Mouse_X = X;
	g_Mouse_Y = Y;
}

void Viewport_UpdateTouchPoint( const sTouchPoint& pt )
{
	std::list<sTouchPoint>::iterator i = FTouchPoints.end();

	// find point
	for ( std::list<sTouchPoint>::iterator j = FTouchPoints.begin(); j != FTouchPoints.end(); ++j )
	{
		if ( j->FID == pt.FID )
		{
			i = j;
			break;
		}
	}

	switch ( pt.FFlag )
	{
		case MOTION_DOWN:
			if ( i == FTouchPoints.end() ) { FTouchPoints.push_back( pt ); }

		case MOTION_UP:
		case MOTION_MOVE:
			if ( i != FTouchPoints.end() )
			{
				*i = pt;
			}

			break;
	}
}

void Viewport_ClearReleasedPoints()
{
	/// Leave only the moving/pressed points in TouchPoints list
	auto first = FTouchPoints.begin();
	auto result = first;

	for ( ; first != FTouchPoints.end() ; ++first )
		if ( ( *first ).FFlag != MOTION_UP ) { *result++ = *first; }

	FTouchPoints.erase ( result, FTouchPoints.end( ) );
}

void Viewport_ProcessMotion( int PointerID, const vec2& pt, bool pressed, int Flag )
{
	GestureHandler_SendMotion( PointerID, ( eMotionFlag )Flag, pt, pressed );
}

// send events packet for all currently touched points
void Viewport_UpdateCurrentGesture()
{
	// send the whole packet
	Viewport_ProcessMotion( MOTION_START, vec2(), false, MOTION_MOVE );

	for ( std::list<sTouchPoint>::iterator j = FTouchPoints.begin() ; j != FTouchPoints.end(); ++j )
	{
		Viewport_ProcessMotion( j->FID, j->FPoint, j->IsPressed(), j->FFlag );
	}

	Viewport_ProcessMotion( MOTION_END, vec2(), false, MOTION_MOVE );
}

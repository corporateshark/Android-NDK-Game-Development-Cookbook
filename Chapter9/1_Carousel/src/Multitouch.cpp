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
		case L_MOTION_DOWN:
			if ( i == FTouchPoints.end() ) { FTouchPoints.push_back( pt ); }

		case L_MOTION_UP:
		case L_MOTION_MOVE:
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
		if ( ( *first ).FFlag != L_MOTION_UP ) { *result++ = *first; }

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
	Viewport_ProcessMotion( L_MOTION_START, vec2(), false, L_MOTION_MOVE );

	for ( std::list<sTouchPoint>::iterator j = FTouchPoints.begin() ; j != FTouchPoints.end(); ++j )
	{
		Viewport_ProcessMotion( j->FID, j->FPoint, j->IsPressed(), j->FFlag );
	}

	Viewport_ProcessMotion( L_MOTION_END, vec2(), false, L_MOTION_MOVE );
}

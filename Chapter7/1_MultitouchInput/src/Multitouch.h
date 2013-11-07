#pragma once

#include <list>
#include <vector>
#include "Mutex.h"
#include "VecMath.h"

enum eMotionFlag
{
   L_MOTION_MOVE = 0,
   L_MOTION_DOWN = 1,
   L_MOTION_UP   = 2,
};

const int L_MOTION_START = -1;
const int L_MOTION_END   = -2;

/// Touch point in normalized coordinates
struct sTouchPoint
{
	sTouchPoint(): FID( -1 ), FPoint( 0.0f ), FTimeStamp( 0.0 ) {}
	sTouchPoint( int TouchID, const LVector2& Point, int Flag, double TimeStamp ): FID( TouchID ), FFlag( ( eMotionFlag )Flag ), FPoint( Point ), FTimeStamp( TimeStamp ) {}

	/// contact ID within a gesture
	int      FID;

	/// x,y position in normalized coordinates
	LVector2 FPoint;

	/// Up, Down, Moving
	eMotionFlag FFlag;

	/// the time this point was updated
	double   FTimeStamp;

	/// Down or Moving
	inline bool IsPressed() const { return ( FFlag == L_MOTION_MOVE ) || ( FFlag == L_MOTION_DOWN ); }
};

extern int g_Mouse_X, g_Mouse_Y;

#pragma region Multitouch stuff

/// List of current contact points
extern std::list<sTouchPoint> FTouchPoints;

void Viewport_UpdateCurrentGesture();
void Viewport_ClearReleasedPoints();
void Viewport_SendKey( int Key, bool Pressed );
bool Viewport_IsKeyPressed( int Key );
void Viewport_MoveMouse( int X, int Y );
void Viewport_ProcessMotion( int PointerID, int x, int y, bool pressed, int Flag );
void Viewport_UpdateTouchPoint( const sTouchPoint& pt );

#pragma endregion

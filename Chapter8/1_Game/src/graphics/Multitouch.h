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

#pragma once

#include <list>
#include <vector>
#include "Mutex.h"
#include "VecMath.h"

enum eMotionFlag
{
   MOTION_MOVE = 0,
   MOTION_UP   = 1,
   MOTION_DOWN = 2,
};

const int MOTION_START = -1;
const int MOTION_END   = -2;

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
	inline bool IsPressed() const { return ( FFlag == MOTION_MOVE ) || ( FFlag == MOTION_DOWN ); }
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

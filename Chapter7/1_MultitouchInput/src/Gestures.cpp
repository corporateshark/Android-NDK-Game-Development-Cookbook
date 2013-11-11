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

#include "iObject.h"
#include "Gestures.h"
#include "Wrapper_Callbacks.h"

#include <algorithm>

#if defined( ANDROID )
#  include "Wrapper_Android.h"
#else
#  include "Wrapper_Windows.h"
#endif

iGestureResponder* g_Responder;

void    UpdateGesture();

sMotionData                  FMotionData;
RingBuffer<sMotionData> FPrevMotionData( 5 );

bool        FMotionDataValid = false;
bool        FMoving = false;
sTouchPoint FInitialPoint( 0, LVector2(), L_MOTION_MOVE, 0.0 );
/// get the position of the current touch point in the current gesture, that means the current position of the last point touched
sTouchPoint FCurrentPoint( 0, LVector2(), L_MOTION_MOVE, 0.0 );

#pragma region Fling
/// pointer movements below this value are ignored
float FlingThresholdSensitivity = 0.1f;

/// pointer movements below this value will not generate a fling
float FlingStartSensitivity = 0.2f;
bool FFlingWasValid = false;
#pragma endregion

#pragma region Pinch-zoom
bool     FPinchZoomValid = false;
bool     FPinchZoomWasValid = false;
float    FZoomFactor = 1.0f;
float    FInitialDistance = 1.0f;
sTouchPoint FInitialPoint1;
sTouchPoint FInitialPoint2;
sTouchPoint FCurrentPoint1;
sTouchPoint FCurrentPoint2;
LVector2 FInitialCenter;
LVector2 FCurrentCenter;
#pragma endregion

static bool IsGestureValid()
{
	return FMotionDataValid;
}

static LVector2 GetPositionDelta()
{
	return FCurrentPoint.FPoint - FInitialPoint.FPoint;
}

void GestureHandler_SendMotion( int ContactID, eMotionFlag Flag, LVector2 Pos, bool Pressed )
{
	// reset the current gesture
	if ( ContactID == L_MOTION_START )
	{
		FMotionDataValid = false;
		FMotionData.Clear();

		return;
	}
	else
	{
		// complete the current gesture
		if ( ContactID == L_MOTION_END )
		{
			FMotionDataValid = true;

			UpdateGesture();

			g_Responder->Event_UpdateGesture( FMotionData );

			if ( sMotionData* P = FPrevMotionData.prev( 0 ) )
			{
				if ( P->GetNumTouchPoints() != FMotionData.GetNumTouchPoints() )
				{
					FPrevMotionData.push_back( FMotionData );
				}
			}
			else
			{
				FPrevMotionData.push_back( FMotionData );
			}
		}
		// add a point to the current gesture
		else
		{
			if ( Pressed )
			{
				FMotionData.AddTouchPoint( sTouchPoint( ContactID, Pos, L_MOTION_DOWN, Env_GetSeconds() ) );
			}

			switch ( Flag )
			{
				case L_MOTION_MOVE:
					g_Responder->Event_PointerMoved( ContactID, Pos );
					break;

				case L_MOTION_UP:
				case L_MOTION_DOWN:
					g_Responder->Event_PointerChanged( ContactID, Pos, Flag == L_MOTION_DOWN );
					break;
			}
		}
	}
}

static bool IsDraggingValid()
{
	// single-point dragging
	if ( IsGestureValid() && FMotionData.GetNumTouchPoints() == 1 && FMotionData.GetTouchPointID( 0 ) == 0 )
	{
		if ( FMoving )
		{
			FCurrentPoint = FMotionData.GetTouchPoint( 0 );
		}
		else
		{
			FMoving       = true;
			FInitialPoint = FMotionData.GetTouchPoint( 0 );
			// to prevent glitch, since no current point is set
			return false;
		}
	}
	else
	{
		FMoving = false;
	}

	return FMoving;
}

static bool IsPinchZoomValid()
{
	if ( IsGestureValid() && FMotionData.GetNumTouchPoints() == 2 )
	{
		const sTouchPoint& Pt1 = ( FMotionData.GetTouchPoint( 0 ) );
		const sTouchPoint& Pt2 = ( FMotionData.GetTouchPoint( 1 ) );
		const LVector2& Pos1( FMotionData.GetTouchPointPos( 0 ) );
		const LVector2& Pos2( FMotionData.GetTouchPointPos( 1 ) );

		float NewDistance = ( Pos1 - Pos2 ).Length();

		if ( FPinchZoomValid )
		{
			// do pinch zoom
			FZoomFactor    = NewDistance / FInitialDistance;
			FCurrentPoint1 = Pt1;
			FCurrentPoint2 = Pt2;
			FCurrentCenter = ( Pos1 + Pos2 ) * 0.5f;
		}
		else
		{
			// start new pinch zoom
			FInitialDistance = NewDistance;
			FPinchZoomValid  = true;
			FZoomFactor      = 1.0f;
			FInitialPoint1   = Pt1;
			FInitialPoint2   = Pt2;
			FInitialCenter = ( Pos1 + Pos2 ) * 0.5f;
			// to prevent glitch, since no current point is set
			return false;
		}
	}
	else
	{
		// stop pinch zoom
		FPinchZoomValid = false;
		FZoomFactor     = 1.0f;
	}

	return FPinchZoomValid;
}

void UpdateGesture()
{
	const sTouchPoint& Pt1 = FInitialPoint;
	const sTouchPoint& Pt2 = FCurrentPoint;

	sMotionData* Prev0 = FPrevMotionData.prev( 0 );
	sMotionData* Prev1 = FPrevMotionData.prev( 1 );
	sMotionData* Prev2 = FPrevMotionData.prev( 2 );

	g_Responder->Event_UpdateGesture( FMotionData );

	// check double tap
	if ( Prev0 && Prev1 && Prev2 )
	{
		size_t NumPts0 = Prev0->GetNumTouchPoints();
		size_t NumPts1 = Prev1->GetNumTouchPoints();
		size_t NumPts2 = Prev2->GetNumTouchPoints();

		if ( NumPts2 == 0 && NumPts1 == 1 && NumPts0 == 0 && FMotionData.GetNumTouchPoints() == 1 )
		{
			float TimeSpan = float( FMotionData.GetTouchPoint( 0 ).FTimeStamp - Prev1->GetTouchPoint( 0 ).FTimeStamp );
			float Offset = ( FMotionData.GetTouchPointPos( 0 ) - Prev1->GetTouchPointPos( 0 ) ).Length();

			// it looks like a double tap
			if ( Offset <= DefaultDoubleTapOffset )
			{
				FPrevMotionData.clear();

				if ( g_Responder->GetDoubleTapTimeout() > TimeSpan )
				{
					g_Responder->Event_DoubleTap( FMotionData.GetTouchPoint( 0 ).FPoint );
				}
			}
		}
	}

	if ( IsDraggingValid() )
	{
		// react on single point dragging
		if ( GetPositionDelta().Length() > FlingThresholdSensitivity )
		{
			g_Responder->Event_Drag( Pt1, Pt2 );
			FFlingWasValid = true;
		}
	}
	else
	{
		// finish gesture
		if ( FFlingWasValid )
		{
			if ( GetPositionDelta().Length() > FlingStartSensitivity )
			{
				// will be only 1 event
				g_Responder->Event_Fling( Pt1, Pt2 );
			}
			else
			{
				g_Responder->Event_Drag( Pt1, Pt2 );
			}

			FFlingWasValid = false;
		}
	}

	if ( IsPinchZoomValid() )
	{
		if ( FPinchZoomWasValid )
		{
			g_Responder->Event_Pinch( FInitialPoint1, FInitialPoint2, FCurrentPoint1, FCurrentPoint2 );
		}
		else
		{
			g_Responder->Event_PinchStart( FInitialPoint1, FInitialPoint2 );
		}

		FPinchZoomWasValid = true;
	}
	else if ( FPinchZoomWasValid )
	{
		FPinchZoomWasValid = false;
		g_Responder->Event_PinchStop( FInitialPoint1, FInitialPoint2, FCurrentPoint1, FCurrentPoint2 );
	}
}

const sMotionData& GestureHandler_GetMotionData()
{
	return FMotionData;
}

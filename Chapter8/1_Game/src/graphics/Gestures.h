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

#include "VecMath.h"
#include "RingBuffer.h"

#include "Multitouch.h"

const float DefaultDoubleTapTimeout = 1.50f; // seconds
const float DefaultDoubleTapOffset  = 0.01f;

struct sMotionData;

class iGestureResponder
{
public:
	/// low-level gesture handling
	virtual void Event_UpdateGesture( const sMotionData& Data ) {};
	/// send after pointer is up or down
	virtual void Event_PointerChanged( int PointerID, const LVector2& Pnt, bool Pressed ) {};
	/// send after pointer is moved
	virtual void Event_PointerMoved( int PointerID, const LVector2& Pnt ) {};
	/// send after double tap has happend
	virtual void Event_DoubleTap( const LVector2& Pnt ) {};
	/// tell how many seconds between 2 taps should pass to consider them a double tap
	virtual float GetDoubleTapTimeout() const { return DefaultDoubleTapTimeout; };
	/// send after pointer is up
	virtual void Event_Fling( const sTouchPoint& Down, const sTouchPoint& Up ) {};
	/// send while pointer is moving
	virtual void Event_Drag( const sTouchPoint& Down, const sTouchPoint& Current ) {};
	/// send during pinch zoom with Pressed=true and once after it with Pressed=false
	virtual void Event_PinchStart( const sTouchPoint& Initial1, const sTouchPoint& Initial2 ) {};
	virtual void Event_Pinch( const sTouchPoint& Initial1, const sTouchPoint& Initial2, const sTouchPoint& Current1, const sTouchPoint& Current2 ) {};
	virtual void Event_PinchStop( const sTouchPoint& Initial1, const sTouchPoint& Initial2, const sTouchPoint& Current1, const sTouchPoint& Current2 ) {};
};

struct sMotionData
{
public:
	sMotionData(): FTouchPoints() {};
	void Clear() { FTouchPoints.clear(); };

	size_t             GetNumTouchPoints() const { return FTouchPoints.size(); }
	const sTouchPoint& GetTouchPoint( size_t Idx )    const { return FTouchPoints[Idx]; }
	const LVector2&    GetTouchPointPos( size_t Idx ) const { return FTouchPoints[Idx].FPoint; }
	int                GetTouchPointID( size_t Idx )  const { return FTouchPoints[Idx].FID; }
	const std::vector<sTouchPoint>& GetTouchPoints() const { return FTouchPoints; }

	void AddTouchPoint( const sTouchPoint& TouchPoint )
	{
		for ( size_t i = 0; i != FTouchPoints.size(); i++ )
		{
			if ( FTouchPoints[i].FID == TouchPoint.FID )
			{
				FTouchPoints[i] = TouchPoint;
				return;
			}
		}

		FTouchPoints.push_back( TouchPoint );
	}
private:
	std::vector<sTouchPoint> FTouchPoints;
};

extern iGestureResponder* g_Responder;

void GestureHandler_SendMotion( int ContactID, eMotionFlag Flag, LVector2 Pos, bool Pressed );

const sMotionData& GestureHandler_GetMotionData();

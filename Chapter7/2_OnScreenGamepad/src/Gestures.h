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

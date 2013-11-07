#pragma once

#include "iIntrusivePtr.h"
#include "iObject.h"
#include "VecMath.h"

const float c_AccelCoef = 15.0f;
/// PID coefficients
const float c_ValueGain =  0.1f;
const float c_IntGain   = 0.1f;
const float c_DiffGain  = 0.1f;
/// Velocity damping factor (  Velocity *= FDamping; )
const float c_Damping   = 0.7f;

///   Fling/Swipe gesture handler for the flow selector
///   HandleSelection() is overriden in GUIFlow to provide custom selection handling
class clFlowFlinger: public iObject
{
public:
	clFlowFlinger(): FPressed( false ), FValue( 0.0f ), FVelocity( 0.0f ) {}
	virtual ~clFlowFlinger() {}
	/// What to do on selection - return True on select complete
	virtual bool HandleSelection( float mx, float my ) { return false; }
	/// Function to update the Fling state
	void Update( float DeltaTime );
	/// Handle the key press
	void OnTouch( bool KeyState );
public:
	/// Was the mouse pressed
	bool FPressed;
	/// Last and first time/point when/where the mouse was clicked
	double FLastTime,  FClickedTime;
	vec2   FLastPoint, FClickPoint;
	/// Current value
	float FValue;
	/// The value where the fling started
	float FInitVal;
	/// Minimum/maximum values
	float FMinValue, FMaxValue;
	/// Current update velocity
	float FVelocity;
};

#include "FlowUI.h"

vec2 Env_GetMouse();
double Env_GetMouseTime();

/// We handle only left mouse button or touch
void clFlowFlinger::OnTouch( bool KeyState )
{
	int CurImg = ( int )ceilf( FValue / OneImageSize );
	vec2 MousePt = Env_GetMouse(); // (x, y);
	double MouseTime = Env_GetMouseTime();
	FPressed = KeyState;

	if ( KeyState )
	{
		FClickPoint = FLastPoint = MousePt;
		FClickedTime = FLastTime  = MouseTime;
		FInitVal = FValue;
		FVelocity = 0;
	}
	else
	{
		double Time = MouseTime - FClickedTime;

		if ( ( FClickPoint - MousePt ).Length() < 0.01f && Time < 0.15 )
		{
			/// Handle user clicks
			HandleSelection( MousePt.x, MousePt.y );
			FVelocity = 0;
			return;
		}

		float dT = ( float )( MouseTime - FLastTime );
		float dSx = MousePt.x - FLastPoint.x;
		FVelocity = ( dT < 0.3 ) ? -c_AccelCoef * dSx / dT : 0;
	}
}

/// Function to update the Fling state. Increments values and optionally send events
void clFlowFlinger::Update( float DeltaTime )
{
	float NewVal;

	if ( FPressed )
	{
		vec2 CurPoint = Env_GetMouse();
		NewVal = FInitVal - c_AccelCoef * ( CurPoint.x - FLastPoint.x );
	}
	else
	{
		NewVal = FValue + FVelocity * DeltaTime;
		FVelocity -= FVelocity * c_Damping * DeltaTime;
		/// clamp min/max
		const float Damper = 4.5f * DeltaTime;

		if ( NewVal > FMaxValue )
		{
			FVelocity = 0;
			NewVal = FMaxValue * Damper + NewVal * ( 1.0f - Damper );
		}
		else if ( NewVal < FMinValue )
		{
			FVelocity = 0;
			NewVal = FMinValue * Damper + NewVal * ( 1.0f - Damper );
		}
	}

	FValue = NewVal;
}

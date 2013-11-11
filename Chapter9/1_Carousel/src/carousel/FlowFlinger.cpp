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

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

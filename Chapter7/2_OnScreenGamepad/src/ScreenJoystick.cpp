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

#include "ScreenJoystick.h"

void ScreenJoystick::HandleTouch( int ContactID, const vec2& Pos, bool Pressed, eMotionFlag Flag )
{
	if ( ContactID == L_MOTION_START )
	{
		for ( size_t i = 0; i != MAX_TOUCH_CONTACTS; i++ )
		{
			if ( FPushedButtons[i] )
			{
				this->SetKeyState( FPushedButtons[i]->FIndex, false );
				FPushedButtons[i] = NULL;
			}

			if ( FPushedAxis[i] )
			{
				this->SetAxisValue( FPushedAxis[i]->FAxis1, 0.0f );
				this->SetAxisValue( FPushedAxis[i]->FAxis2, 0.0f );
				FPushedAxis[i] = NULL;
			}
		}

		return;
	}

	if ( ContactID == L_MOTION_END ) { return; }

	if ( ContactID < 0 || ContactID >= MAX_TOUCH_CONTACTS ) { return; }

	/// clear all previous presses/axis slides
	if ( Flag == L_MOTION_DOWN || Flag == L_MOTION_MOVE )
	{
		vec4 Colour = GetColourAtPoint( Pos );
		sBitmapButton* Button = GetButtonForColour( Colour );
		sBitmapAxis*     Axis = GetAxisForColour( Colour );

		if ( Button && Pressed )
		{
			// touchdown, set the key
			int Idx = Button->FIndex;

			this->SetKeyState( Idx, true );

			// and store the initial button/color to track movement later
			FPushedButtons[ContactID] = Button;
		}

		if ( Axis && Pressed )
		{
			this->ReadAxis( Axis,  Pos );
			FPushedAxis[ContactID] = Axis;
		}
	}
}

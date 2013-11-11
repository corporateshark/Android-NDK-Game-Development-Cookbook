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

#include "iObject.h"
#include "Multitouch.h"

const size_t MAX_TOUCH_CONTACTS = 10;

/// The description of a button for the on-screen joystick
struct sBitmapButton
{
	/// Colour in the mask for this button
	vec4 FColour;
	/// Button index in the virtual on-screen device
	int FIndex;
};

/// The description of an axis for the on-screen joystick
struct sBitmapAxis
{
	/// Radius of the UI for these axes controller
	float FRadius;
	/// The center of the element
	vec2 FPosition;
	/// Indices of the axes
	int FAxis1, FAxis2;
	/// Colour of the axis in the mask
	vec4 FColour;
};

class ScreenJoystick
{
public:
	ScreenJoystick(): FMaskBitmap( NULL ) {}

	/// Deallocate axis/button info
	virtual ~ScreenJoystick()
	{
	}

	/// Allocate button and axis state arrays
	void InitKeys()
	{
		FKeyValue.resize( FButtonDesc.size() );

		if ( FKeyValue.size() > 0 )
			for ( size_t j = 0 ; j < FKeyValue.size() ; j++ ) { FKeyValue[j] = false; }

		FAxisValue.resize( FAxisDesc.size() * 2 );

		if ( FAxisValue.size() > 0 )
		{
			memset( &FAxisValue[0], 0, FAxisValue.size() * sizeof( float ) );
		}
	}

	void Restart()
	{
		memset( &FPushedAxis[0], 0, sizeof( sBitmapAxis* ) * MAX_TOUCH_CONTACTS );
		memset( &FPushedButtons[0], 0, sizeof( sBitmapButton* ) * MAX_TOUCH_CONTACTS );
	}

	/// Check if the key/button is pressed
	bool IsPressed( int KeyIdx ) const
	{
		return ( KeyIdx < 0 || KeyIdx >= ( int )FKeyValue.size() ) ? false : FKeyValue[KeyIdx];
	}

	/// get current axis value
	float GetAxisValue( int AxisIdx ) const
	{
		return ( ( AxisIdx < 0 ) || AxisIdx >= ( int )FAxisValue.size() ) ? 0.0f : FAxisValue[AxisIdx];
	}

	void SetKeyState( int KeyIdx, bool Pressed )
	{
		if ( KeyIdx < 0 || KeyIdx >= ( int )FKeyValue.size() ) { return; }

		FKeyValue[KeyIdx] = Pressed;
	}

	void SetAxisValue( int AxisIdx, float Val )
	{
		if ( ( AxisIdx < 0 ) || AxisIdx >= ( int )FAxisValue.size() ) { return; }

		FAxisValue[AxisIdx] = Val;
	}

	/// Event handler for the taps
	void HandleTouch( int ContactID, const vec2& Pos, bool Pressed, eMotionFlag Flag );

	sBitmapButton* GetButtonForColour( const vec4& Colour )
	{
		for ( size_t k = 0 ; k < FButtonDesc.size(); k++ )
			if ( ( FButtonDesc[k].FColour - Colour ).Length() < 0.1f )
			{
				return &FButtonDesc[k];
			}

		return NULL;
	}

	sBitmapAxis* GetAxisForColour( const vec4& Colour )
	{
		for ( size_t k = 0 ; k < FAxisDesc.size(); k++ )
			if ( ( FAxisDesc[k].FColour - Colour ).Length() < 0.1f )
			{
				return &FAxisDesc[k];
			}

		return NULL;
	}

public:
	/// Currently pushed buttons and axes being sled
	sBitmapButton* FPushedButtons[MAX_TOUCH_CONTACTS];
	sBitmapAxis*   FPushedAxis[MAX_TOUCH_CONTACTS];

	void ReadAxis( sBitmapAxis* Axis, const vec2& Pos )
	{
		if ( !Axis ) { return; }

		// read axis value based on center and touch point
		float v1 = (  ( Axis->FPosition - Pos ).x / Axis->FRadius );
		float v2 = ( -( Axis->FPosition - Pos ).y / Axis->FRadius );

		this->SetAxisValue( Axis->FAxis1, v1 );
		this->SetAxisValue( Axis->FAxis2, v2 );
	}

	/// Buttons and axes
	std::vector<sBitmapButton> FButtonDesc;
	std::vector<sBitmapAxis> FAxisDesc;

	std::vector<float> FAxisValue;
	std::vector<bool> FKeyValue;

	unsigned char* FMaskBitmap;
private:
	vec4 GetColourAtPoint( const vec2& Pt ) const
	{
		if ( !FMaskBitmap ) { return vec4( -1 ); }

		int x = ( int )( Pt.x * 512.0f );
		int y = ( int )( Pt.y * 512.0f );
		int Ofs = ( y * 512 + x ) * 3;

		float r = ( float )FMaskBitmap[Ofs + 0] / 255.0f;
		float g = ( float )FMaskBitmap[Ofs + 1] / 255.0f;
		float b = ( float )FMaskBitmap[Ofs + 2] / 255.0f;

		return vec4( r, g, b, 0.0f );
	}
};

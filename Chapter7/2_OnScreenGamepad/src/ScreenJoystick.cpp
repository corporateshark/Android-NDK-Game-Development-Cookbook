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

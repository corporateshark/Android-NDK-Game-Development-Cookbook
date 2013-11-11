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

#include "GUI.h"
#include "Canvas.h"
#include "Wrapper_Callbacks.h"

extern clPtr<clCanvas> g_Canvas;
extern clPtr<clTextRenderer> g_TextRenderer;
extern int g_Font;

bool clGUIPage::OnKey( int Key, bool KeyState )
{
	if ( !KeyState && Key == LK_ESCAPE )
	{
		/// Call custom handler to allow some actions like haptic feedback
		if ( FFallbackPage.GetInternalPtr() != NULL )
		{
			FGUI->SetActivePage( FFallbackPage );
			return true;
		}
	}

	return false;
}

void clGUIPage::SetActive()
{
	FGUI->SetActivePage( this );
}

void clGUIButton::Render()
{
	g_Canvas->Rect2D( FRect.X1(), FRect.Y1(), FRect.X2(), FRect.Y2(), FPressed ?  LVector4( 0.0f, 1.0f, 0.0f, 0.5f ) :
	                  LVector4( 1.0f, 0.0f, 0.0f, 0.5f ) );

	const float Ox = 0.10f;
	const float Oy = 0.05f;

	g_Canvas->TextStr( FRect.X1() + Ox, FRect.Y1() + Oy, FRect.X2() - Ox, FRect.Y2() - Oy, FTitle, 64, LVector4( 1.0f ), g_TextRenderer, g_Font );
}

void clGUIButton::OnTouch( const LVector2& Pos, bool TouchState )
{
	FPressed = TouchState;

	if ( FRect.ContainsPoint( Pos ) && !TouchState )
	{
		if ( FFallbackPage )
		{
			FFallbackPage->SetActive();
		}
		else
		{
			ExitApp();
		}
	}
}

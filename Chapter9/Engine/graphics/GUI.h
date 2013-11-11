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
#include "VecMath.h"
#include "iIntrusivePtr.h"

#include <vector>
#include <string>

const int LK_ESCAPE = 27;

class clGUI;
class clGUIPage;

class clGUIButton: public iObject
{
public:
	clGUIButton( const LRect& R, const std::string Title, clPtr<clGUIPage> Page ): FRect( R ), FTitle( Title ), FPressed( false ), FFallbackPage( Page ) {}

	virtual void Render();
	virtual void OnTouch( const LVector2& Pos, bool TouchState );
	virtual bool Contains( const LVector2& Pos )
	{
		return FRect.ContainsPoint( Pos );
	}

public:
	LRect       FRect;
	std::string FTitle;
	bool        FPressed;

	clPtr<clGUIPage> FFallbackPage;
};

/// GUI page - similar to GUI window but always occupies the whole screen and only one page can be active at a time
class clGUIPage: public iObject
{
public:
	clGUIPage(): FFallbackPage( NULL ) {}
	virtual ~clGUIPage() {}

	virtual void Update( float DeltaTime ) {}
	virtual void Render()
	{
		for ( auto i = FButtons.begin() ; i != FButtons.end() ; i++ )
		{
			( *i )->Render();
		}
	}
	virtual void AddButton( clPtr<clGUIButton> B )
	{
		FButtons.push_back( B );
	}
	virtual void SetActive();

	virtual void OnActivation() {}
	virtual void OnDeactivation() {}
public:
	// GUI events (standart Esc press behaviour)
	virtual bool OnKey( int Key, bool KeyState );
	virtual void OnTouch( const LVector2& Pos, bool TouchState )
	{
		if ( !TouchState && FCapture )
		{
			FCapture->OnTouch( Pos, TouchState );
			FCapture = NULL;
			return;
		}

		for ( auto i = FButtons.begin() ; i != FButtons.end() ; i++ )
		{
			if ( ( *i )->Contains( Pos ) )
			{
				( *i )->OnTouch( Pos, TouchState );
				FCapture = ( *i );
				return;
			}
		}
	}

public:
	/// The page we return to when the Esc press occurs
	clPtr<clGUIPage> FFallbackPage;

	std::vector< clPtr<clGUIButton> > FButtons;
	clPtr<clGUIButton> FCapture;

	/// Link to the GUI
	clGUI* FGUI;
};

class clGUI: public iObject
{
public:
	clGUI(): FActivePage( NULL ), FPages() {}
	virtual ~clGUI() {}
	void AddPage( const clPtr<clGUIPage>& P )
	{
		P->FGUI = this;
		FPages.push_back( P );
	}
	void SetActivePage( const clPtr<clGUIPage>& Page )
	{
		if ( Page == FActivePage ) { return; }

		if ( FActivePage )
		{
			FActivePage->OnDeactivation();
		}

		if ( Page )
		{
			Page->OnActivation();
		}

		FActivePage = Page;
	}
	void Update( float DeltaTime )
	{
		if ( FActivePage )
		{
			FActivePage->Update( DeltaTime );
		}
	}
	void Render()
	{
		if ( FActivePage )
		{
			FActivePage->Render();
		}
	}
	void OnKey( vec2 MousePos, int Key, bool KeyState )
	{
		FMousePosition = MousePos;

		if ( FActivePage )
		{
			FActivePage->OnKey( Key, KeyState );
		}
	}
	void OnTouch( const LVector2& Pos, bool TouchState )
	{
		if ( FActivePage )
		{
			FActivePage->OnTouch( Pos, TouchState );
		}
	}
private:
	vec2 FMousePosition;
	clPtr<clGUIPage> FActivePage;
	std::vector< clPtr<clGUIPage> > FPages;

};

#pragma once

#include "GUI.h"

class clPage_MainMenu: public clGUIPage
{
public:
	virtual void Render()
	{
		LGL3->glClearColor( 1.0f, 0.0f, 1.0f, 0.0f );
		LGL3->glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		clGUIPage::Render();
	}
	virtual bool OnKey( int Key, bool KeyState )
	{
		if ( Key == LK_ESCAPE ) { ExitApp(); }

		return true;
	}
};

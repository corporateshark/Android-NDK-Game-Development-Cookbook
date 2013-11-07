#pragma once

#include "GUI.h"

class clPage_MainMenu: public clGUIPage
{
public:
	clPage_MainMenu();
	virtual void Render();
	virtual bool OnKey( int Key, bool KeyState )
	{
		if ( Key == LK_ESCAPE ) { ExitApp(); }

		return true;
	}
private:
	clPtr<clGLTexture> FTexture;
};

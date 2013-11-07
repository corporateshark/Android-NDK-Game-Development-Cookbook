#pragma once

#include "GUI.h"

#include "Engine.h"

extern sLGLAPI* LGL3;

extern clPtr<clCanvas> g_Canvas;
extern clPtr<clTextRenderer> g_TextRenderer;
extern int g_Font;

class clPage_About: public clGUIPage
{
public:
	virtual void Render()
	{
		LGL3->glClearColor( 1.0f, 0.0f, 0.0f, 0.0f );
		LGL3->glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

		g_Canvas->TextStr( 0.3f, 0.4f, 0.7f, 0.6f, "3_UIPrototype", 32, LVector4( 1.0f ), g_TextRenderer, g_Font );

		clGUIPage::Render();
	}
};

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
	clPage_About();
	virtual void Render();
private:
	clPtr<clGLTexture> FTexture;
};

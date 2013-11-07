#pragma once

#include "GUI.h"

#include "Game.h"

extern sLGLAPI* LGL3;
extern clPuzzle g_Game;

void RenderGame( clPuzzle* g );

class clPage_Game: public clGUIPage
{
public:
	virtual void Render()
	{
		LGL3->glDisable( GL_DEPTH_TEST );

		RenderGame( &g_Game );

		clGUIPage::Render();
	}
};

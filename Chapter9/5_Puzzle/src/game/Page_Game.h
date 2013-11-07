#pragma once

#include "GUI.h"

#include "Game.h"

extern sLGLAPI* LGL3;
extern clPuzzle g_Game;

void RenderGame( clPuzzle* g );

class clPage_Game: public clGUIPage
{
public:
	virtual void OnTouch( const LVector2& Pos, bool TouchState )
	{
		g_Game.OnKey( Pos.x, Pos.y, TouchState );

		clGUIPage::OnTouch( Pos, TouchState );
	}

	virtual void Update( float DT )
	{
		g_Game.Timer( DT );
	}

	virtual void Render()
	{
		LGL3->glDisable( GL_DEPTH_TEST );

		RenderGame( &g_Game );

		clGUIPage::Render();
	}
};

#include "Engine.h"

#include "Globals.h"
#include "Page_MainMenu.h"
#include "Page_About.h"

clPage_About::clPage_About()
{
	FTexture = LoadTexture( "AboutBackground.jpg" );
}

void clPage_About::Render()
{
	g_Canvas->TexturedRect2D( 0, 0, 1, 1, LVector4( 1.0f ), FTexture );

	g_Canvas->TextStr( 0.3f, 0.4f, 0.7f, 0.6f, "Puzzle Game", 32, LVector4( 1.0f ), g_TextRenderer, g_Font );

	clGUIPage::Render();
}

clPage_MainMenu::clPage_MainMenu()
{
	FTexture = LoadTexture( "MainMenuBackground.jpg" );
}

void clPage_MainMenu::Render()
{
	g_Canvas->TexturedRect2D( 0, 0, 1, 1, LVector4( 1.0f ), FTexture );

	clGUIPage::Render();
}

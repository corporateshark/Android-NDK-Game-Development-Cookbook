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

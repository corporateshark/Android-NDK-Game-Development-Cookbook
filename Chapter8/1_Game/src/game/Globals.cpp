#include "Globals.h"
#include "Shape.h"

sGameState g_GS;

void sGameState::Reset()
{
	FGameOver      = false;
	FGameTime      = 0.0f;
	FGameTimeCount = 0.0f;
	FUpdateSpeed   = 0.75f; // seconds
	FScore         = 0;
	FLevel         = 0;
	FCurrentColor  = Linderdaum::Math::Random( NUM_COLORS );
	FNextColor     = SelectDifferentColor( FCurrentColor );

	FCurX = 3;
	FCurY = 0;
}

void sGameState::SwitchColor()
{
	FCurrentColor = FNextColor;
	FNextColor = SelectDifferentColor( FNextColor );
}

int sGameState::SelectDifferentColor( int OldColor )
{
	int NewColor = 0;

	do
	{
		NewColor = Linderdaum::Math::Random( NUM_COLORS );
	}
	while ( OldColor == NewColor );

	return NewColor;
}

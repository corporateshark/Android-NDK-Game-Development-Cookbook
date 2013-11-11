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

#include "Game.h"

void clPuzzle::Retoss( int W, int H )
{
	FColumns = W;
	FRows    = H;
	FTiles.resize( FColumns * FRows );

	// init tiles
	for ( int i = 0; i != FColumns; i++ )
		for ( int j = 0; j != FRows; j++ )
		{
			FTiles[j * FColumns + i] = clTile( i, FRows - j - 1, FColumns, FRows );
		}

	// toss the image
	for ( int i = 0; i != FColumns; i++ )
		for ( int j = 0; j != FRows; j++ )
		{
			int NewI = Linderdaum::Math::RandomInRange( i, FColumns - 1 );
			int NewJ = Linderdaum::Math::RandomInRange( j, FRows - 1    );
			SwapTiles( i, j, NewI, NewJ );
		}

	FClickedI = FClickedJ = -1;
}

void clPuzzle::Timer( float DeltaSeconds )
{
	for ( int i = 0; i != FColumns; i++ )
		for ( int j = 0; j != FRows; j++ )
		{
			GetTile( i, j )->Update( DeltaSeconds );
		}
}

bool clPuzzle::IsComplete() const
{
	for ( int i = 0; i != FColumns; i++ )
		for ( int j = 0; j != FRows; j++ )
		{
			clTile* T = GetTile( i, j );

			if ( T->FOriginX != i || T->FOriginY != j )
			{
				return false;
			}
		}

	return true;
}

void clPuzzle::OnKey( float mx, float my, bool KeyState )
{
	int i = ( int )floor( mx * FColumns );
	int j = ( int )floor( my * FRows );
	int MouseI = ( i >= 0 && i < FColumns ) ? i : -1;
	int MouseJ = ( j >= 0 && j < FRows ) ? j : -1;

	FMovingImage = KeyState;

	if ( FMovingImage )
	{
		FClickedI = MouseI;
		FClickedJ = MouseJ;

		if ( FClickedI >= 0 && FClickedJ >= 0 && FClickedI < FColumns && FClickedJ < FRows )
		{
			FOfsX = ( ( float )FClickedI / FColumns - mx );
			FOfsY = ( ( float )FClickedJ / FRows    - my );
		}
		else
		{
			FClickedI = FClickedJ = -1;
		}
	}
	else
	{
		bool NewPosition   = ( MouseI != FClickedI || MouseJ != FClickedJ );
		bool ValidPosition1 = ( FClickedI >= 0 && FClickedJ >= 0 && FClickedI < FColumns && FClickedJ < FRows );
		bool ValidPosition2 = ( MouseI >= 0 && MouseJ >= 0 && MouseI < FColumns && MouseJ < FRows );

		if ( NewPosition && ValidPosition1 && ValidPosition2 )
		{
			int dX = MouseI - FClickedI, dY = MouseJ - FClickedJ;
			SwapTiles( FClickedI, FClickedJ, MouseI, MouseJ );
		}

		if ( IsComplete() ) { /* invoke event handler */ }

		FClickedI = FClickedJ = -1;
	}
}

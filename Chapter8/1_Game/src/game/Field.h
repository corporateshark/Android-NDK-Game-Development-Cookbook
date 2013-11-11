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

#pragma once

#include "Engine.h"
#include "Shape.h"

class clBricksField
{
public:
	static const int FWidth = 11;
	static const int FHeight = 22;
public:
	void ClearField()
	{
		for ( int i = 0 ; i < FWidth ; i++ )
		{
			for ( int j = 0 ; j < FHeight ; j++ ) { FField[i][j] = -1; }
		}
	}

	bool FigureFits( int x, int y, const clBricksShape& fig )
	{
		for ( int i = 0 ; i < fig.FWidth ; i++ )
		{
			for ( int j = 0 ; j < fig.FHeight ; j++ )
			{
				if ( fig.GetMask( i, j ) != ( int )( -1 ) )
				{
					if ( x + i >= FWidth ) { return false; }

					if ( y + j >= FHeight ) { return false; }

					if ( x + i < ( int )( 0 ) ) { return false; }

					if ( y + j < ( int )( 0 ) ) { return false; }

					if ( FField[x + i][y + j] != ( int )( -1 ) ) { return false; }
				}
			}
		}

		return true;
	}

	bool FigureWillHitNextTurn( int x, int y, const clBricksShape& fig )
	{
		return FigureFits( x, y + 1, fig );
	}

	void AddFigure( int x, int y, const clBricksShape& fig )
	{
		for ( int i = 0 ; i < fig.FWidth ; i++ )
		{
			for ( int j = 0 ; j < fig.FHeight ; j++ )
			{
				if ( fig.GetMask( i, j ) != ( int )( -1 ) )
				{
					FField[x + i][y + j] = fig.GetMask( i, j );
				}
			}
		}
	}

	int deleteLines()
	{
		int linesToDelete = 0;
		int deleteLine = 1;

		for ( int j = 0 ; j < FHeight ; j++ )
		{
			deleteLine = 1;

			for ( int i = 0 ; i < FWidth ; i++ )
			{
				if ( FField[i][j] == ( int )( -1 ) )
				{
					deleteLine = 0;
					break;
				}
			}

			if ( deleteLine > 0 )
			{
				// increase stats
				linesToDelete++;

				// make this line empty
				for ( int i = 0 ; i < FWidth ; i++ ) { FField[i][j] = -1; }
			}
		}

		if ( linesToDelete > 0 )
		{
			CollapseField(); // check for empty lines and remove them
		}

		return linesToDelete;
	}

	// recursive flood fill
	int CalcNeighbours( int i, int j, int Col )
	{
		if ( i < 0 || j < 0 || i >= FWidth || j >= FHeight || FField[i][j] != Col ) { return 0; }

		FField[i][j] = -1;

		int Result =  1 + CalcNeighbours( i + 1, j + 0, Col ) + CalcNeighbours( i - 1, j + 0, Col ) +
		              CalcNeighbours( i + 0, j + 1, Col ) + CalcNeighbours( i + 0, j - 1, Col );

		FField[i][j] = Col;

		return Result;
	}

	// recursive flood fill
	void FillNeighbours( int i, int j, int Col )
	{
		if ( i < 0 || j < 0 || i >= FWidth || j >= FHeight || FField[i][j] != Col ) { return; }

		FField[i][j] = -1;

		FillNeighbours( i + 1, j + 0, Col );
		FillNeighbours( i - 1, j + 0, Col );
		FillNeighbours( i + 0, j + 1, Col );
		FillNeighbours( i + 0, j - 1, Col );
	}

	int DeleteRegions( int NumRegionsToDelete )
	{
		int NumRegions = 0;

		for ( int j = 0; j != FHeight; j++ )
		{
			for ( int i = 0 ; i != FWidth ; i++ )
			{
				if ( FField[i][j] != -1 )
				{
					int Neighbours = CalcNeighbours( i, j, FField[i][j] );

					if ( Neighbours >= NumRegionsToDelete )
					{
						FillNeighbours( i, j, FField[i][j] );
						NumRegions += Neighbours;
					}
				}
			}
		}

		// check for empty lines and remove them
		CollapseField();

		return NumRegions;
	}

	void CollapseField()
	{
		for ( int i = 0 ; i != FWidth ; i++ )
		{
			int Offset = 0;

			for ( int j = FHeight - 1 ; j >= 0 ; j-- )
			{
				FField[i][j + Offset] = FField[i][j];

				if ( FField[i][j] == -1 ) { Offset++; }
			}

			for ( int j = 0; j != Offset && j != FHeight; j++ )
			{
				FField[i][j] = -1;
			}
		}
	}
public:
	int FField[ FWidth ][ FHeight ];
};

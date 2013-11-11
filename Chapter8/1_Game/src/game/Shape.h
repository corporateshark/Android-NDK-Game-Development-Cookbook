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
#include "Pentomino.h"

const int NUM_COLORS = 6;

class clBricksShape
{
public:
	static const int FWidth  = SHAPES_X;
	static const int FHeight = SHAPES_Y;
private:
	int FColor[NUM_COLORS];
	int FFigureIndex;
	int FRotationIndex;
public:
	inline int GetMask( int i, int j ) const
	{
		if ( i < 0 || j < 0 ) { return -1; }

		if ( i >= FWidth || j >= FHeight ) { return -1; }

		int ColorIdx = Shapes[FFigureIndex][FRotationIndex][i][j];
		return ColorIdx ? FColor[ColorIdx] : -1;
	}

	inline void Rotate( bool CW )
	{
		FRotationIndex = CW ?
		                 ( FRotationIndex ? FRotationIndex - 1 : ROTATIONS - 1 ) :
			                 ( FRotationIndex + 1 ) % ROTATIONS;
	}

	inline void GenFigure( int figIdx, int Col )
{
		for ( int i = 0; i != NUM_COLORS; i++ ) { FColor[i] = Linderdaum::Math::Random( NUM_COLORS ); }

		FFigureIndex = figIdx;
		FRotationIndex = 0;
	}

	inline void GetTopLeftCorner( int* x, int* y ) const
	{
		*x = SHAPES_X - 1;
		*y = SHAPES_Y - 1;

		for ( int i = 0; i != FWidth; i++ )
			for ( int j = 0; j != FHeight; j++ )
			{
				if ( GetMask( i, j ) != -1 )
				{
					if ( i < *x ) { *x = i; }

					if ( j < *y ) { *y = j; }
				}
			}
	}

	inline void GetBottomRightCorner( int* x, int* y ) const
	{
		*x = 0;
		*y = 0;

		for ( int i = 0; i != FWidth; i++ )
			for ( int j = 0; j != FHeight; j++ )
			{
				if ( GetMask( i, j ) != -1 )
				{
					if ( i > *x ) { *x = i; }

					if ( j > *y ) { *y = j; }
				}
			}
	}

	inline LRect GetSize() const
	{
		int X1, Y1, X2, Y2;

		GetTopLeftCorner( &X1, &Y1 );
		GetBottomRightCorner( &X2, &Y2 );

		return LRect( ( float )X1, ( float )Y1, ( float )X2, ( float )Y2 );
	}
};

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

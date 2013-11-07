#pragma once
#include "Tile.h"
#include <vector>

class Puzzle
{
public:
	Puzzle(): FMovingImage( false ), FClickedI( -1 ), FClickedJ( -1 ), FOfsX( 0.0f ), FOfsY( 0.0f )
	{
		Retoss( 4, 4 );
	}

	void OnKey( float mx, float my, bool KeyState );
	void Timer( float DeltaSeconds );

	void Retoss( int W, int H );
	bool IsComplete() const;
	clTile* GetTile( int i, int j ) const { return &FTiles[j * FColumns + i]; };
	void SwapTiles( int i1, int j1, int i2, int j2 ) { std::swap( FTiles[j1 * FColumns + i1], FTiles[j2 * FColumns + i2] ); }

	int FColumns, FRows;
	bool FMovingImage;
	int FClickedI, FClickedJ;
	float FOfsX, FOfsY;
	mutable std::vector<clTile> FTiles;
};

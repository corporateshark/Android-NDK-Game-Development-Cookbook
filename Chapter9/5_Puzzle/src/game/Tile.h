#pragma once
#include "VecMath.h"

class clTile
{
public:
	clTile(): FOriginX( 0 ), FOriginY( 0 ) {};
	clTile( int OriginX, int OriginY, int Columns, int Rows ): FOriginX( OriginX ), FOriginY( OriginY )
	{
		// calculate tile rect
		float TileWf = 1.0f / Columns, TileHf = 1.0f / Rows;
		float X1f = TileWf * ( OriginX + 0 ), X2f = TileWf * ( OriginX + 1 );
		float Y1f = TileHf * ( OriginY + 0 ), Y2f = TileHf * ( OriginY + 1 );
		FRect = LRect( X1f, Y1f, X2f, Y2f );
		FTarget = FCur = vec2( OriginX, OriginY );
	}
	void SetTarget( int X, int Y ) { FTarget = vec2( X, Y ); }
	void MoveTo( float X, float Y ) { FCur.x = X; FCur.y = Y; };
	void Update( float dT )
	{
		vec2 dS = FTarget - FCur;

		if ( fabs( dS.x ) < 0.001f )
		{
			dS.x = 0;
			FCur.x = FTarget.x;
		}

		if ( fabs( dS.y ) < 0.001f )
		{
			dS.y = 0;
			FCur.y = FTarget.y;
		}

		FCur += 10.0f * dT * dS;
	}
	const LRect* GetRect() const { return &FRect; };
public:
	int   FOriginX, FOriginY;
	vec2  FCur, FTarget;
private:
	LRect FRect;
};

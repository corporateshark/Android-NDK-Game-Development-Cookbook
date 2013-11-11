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

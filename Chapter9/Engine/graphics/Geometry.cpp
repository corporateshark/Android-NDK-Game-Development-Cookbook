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

#include "Geometry.h"

clVertexAttribs::clVertexAttribs()
	: FActiveVertexCount( 0 ),
	  FVertices( 0 ),
	  FTexCoords( 0 ),
	  FNormals( 0 ),
	  FColors( 0 ),
	  FCurrentTexCoords(),
	  FCurrentNormal(),
	  FCurrentColor( 1.0f, 1.0f, 1.0f, 1.0f ),
	  FStreams( L_VS_TOTAL_ATTRIBS )
{
}

clVertexAttribs::clVertexAttribs( size_t Vertices )
	: FActiveVertexCount( Vertices ),
	  FVertices( Vertices ),
	  FTexCoords( Vertices ),
	  FNormals( Vertices ),
	  FColors( Vertices ),
	  FCurrentTexCoords(),
	  FCurrentNormal(),
	  FCurrentColor( 1.0f, 1.0f, 1.0f, 1.0f ),
	  FStreams( L_VS_TOTAL_ATTRIBS )
{
}

const std::vector<const void*>& clVertexAttribs::EnumerateVertexStreams() const
{
	FStreams[ L_VS_VERTEX   ] = &FVertices[0];
	FStreams[ L_VS_TEXCOORD ] = &FTexCoords[0];
	FStreams[ L_VS_NORMAL   ] = &FNormals[0];
	FStreams[ L_VS_COLORS   ] = &FColors[0];

	return FStreams;
}

void clVertexAttribs::SetTexCoordV( const LVector2& V )
{
	FCurrentTexCoords = V;
}

void clVertexAttribs::SetNormalV( const LVector3& Vec )
{
	FCurrentNormal = Vec;
}

void clVertexAttribs::SetColorV( const LVector4& Vec )
{
	FCurrentColor = Vec;
}

void clVertexAttribs::Restart( size_t ReserveVertices )
{
	FCurrentColor     = LVector4( 1.0f, 1.0f, 1.0f, 1.0f );
	FCurrentNormal    = LVector3( 0, 0, 0 );
	FCurrentTexCoords = LVector2( 0, 0 );

	FActiveVertexCount = 0;

	FVertices.resize( ReserveVertices );
	FTexCoords.resize( ReserveVertices );
	FNormals.resize( ReserveVertices );
	FColors.resize( ReserveVertices );
}

void clVertexAttribs::EmitVertexV( const LVector3& Vec )
{
	FVertices[ FActiveVertexCount ] = Vec;
	FTexCoords[ FActiveVertexCount ] = FCurrentTexCoords;
	FNormals[ FActiveVertexCount ] = FCurrentNormal;
	FColors[ FActiveVertexCount ] = FCurrentColor;

	FActiveVertexCount++;
}

//(vX,vY) dX
//    |---/
//    |  /
// dY | /
//    |/
clPtr<clVertexAttribs> clGeomServ::CreateTriangle2D( float vX, float vY, float dX, float dY, float Z )
{
	clPtr<clVertexAttribs> VA = new clVertexAttribs();

	VA->Restart( 3 );

	VA->SetNormalV( LVector3( 0, 0, 1 ) );

	VA->SetTexCoord( 1, 1, 0 );
	VA->EmitVertexV( LVector3( vX   , vY   , Z ) );
	VA->SetTexCoord( 1, 0, 0 );
	VA->EmitVertexV( LVector3( vX   , vY - dY, Z ) );
	VA->SetTexCoord( 0, 1, 0 );
	VA->EmitVertexV( LVector3( vX + dX, vY   , Z ) );

	return VA;
}

float Coord( float A, float A1, float A2 )
{
	return ( A - A1 ) / ( A2 - A1 );
}

clPtr<clVertexAttribs> clGeomServ::CreateRect2D( float X1, float Y1, float X2, float Y2, float Z, bool FlipTexCoordsVertical, int Subdivide )
{
	int SegmentsX = Subdivide + 1;
	int SegmentsY = Subdivide + 1;

	bool Flip = FlipTexCoordsVertical;

	clPtr<clVertexAttribs> VA = new clVertexAttribs();

	VA->Restart( SegmentsX * SegmentsY * 6 );

	VA->SetNormalV( LVector3( 0, 0, 1 ) );

	float dX = ( X2 - X1 ) / static_cast<float>( SegmentsX );
	float dY = ( Y2 - Y1 ) / static_cast<float>( SegmentsY );

	for ( int i = 0; i != SegmentsX; ++i )
	{
		for ( int j = 0; j != SegmentsY; ++j )
		{
			float U1 = Coord( X1 + dX * i, X1, X2 );
			float U2 = Coord( X1 + dX * i, X1, X2 );
			float U3 = Coord( X1 + dX * ( i + 1 ), X1, X2 );
			float U4 = Coord( X1 + dX * ( i + 1 ), X1, X2 );
			float U5 = Coord( X1 + dX * i, X1, X2 );
			float U6 = Coord( X1 + dX * ( i + 1 ), X1, X2 );

			float V1 = Coord( Y1 + dY * j, Y1, Y2 );
			float V2 = Coord( Y1 + dY * ( j + 1 ), Y1, Y2 );
			float V3 = Coord( Y1 + dY * j, Y1, Y2 );
			float V4 = Coord( Y1 + dY * j, Y1, Y2 );
			float V5 = Coord( Y1 + dY * ( j + 1 ), Y1, Y2 );
			float V6 = Coord( Y1 + dY * ( j + 1 ), Y1, Y2 );

			VA->SetTexCoord( U1, Flip ? 1.0f - V1 : V1, 0 );
			VA->EmitVertex( X1 + dX * i,     Y1 + dY * j,     Z );
			VA->SetTexCoord( U2, Flip ? 1.0f - V2 : V2, 0 );
			VA->EmitVertex( X1 + dX * i,     Y1 + dY * ( j + 1 ), Z );
			VA->SetTexCoord( U3, Flip ? 1.0f - V3 : V3, 0 );
			VA->EmitVertex( X1 + dX * ( i + 1 ), Y1 + dY * j,     Z );

			VA->SetTexCoord( U4, Flip ? 1.0f - V4 : V4, 0 );
			VA->EmitVertex( X1 + dX * ( i + 1 ), Y1 + dY * j,     Z );
			VA->SetTexCoord( U5, Flip ? 1.0f - V5 : V5, 0 );
			VA->EmitVertex( X1 + dX * i,     Y1 + dY * ( j + 1 ), Z );
			VA->SetTexCoord( U6, Flip ? 1.0f - V6 : V6, 0 );
			VA->EmitVertex( X1 + dX * ( i + 1 ), Y1 + dY * ( j + 1 ), Z );
		}
	}

	return VA;
}

void clGeomServ::AddPlane( const clPtr<clVertexAttribs>& VA, float SizeX, float SizeY, int SegmentsX, int SegmentsY, float Z )
{
	VA->SetNormalV( LVector3( 0, 0, 1 ) );

	float X1 = -SizeX * 0.5f;
	float Y1 = -SizeY * 0.5f;

	float dX = SizeX / static_cast<float>( SegmentsX );
	float dY = SizeY / static_cast<float>( SegmentsY );

	for ( int i = 0; i != SegmentsX; ++i )
	{
		for ( int j = 0; j != SegmentsY; ++j )
		{
			VA->EmitVertex( X1 + dX * i,     Y1 + dY * j,     Z );
			VA->EmitVertex( X1 + dX * i,     Y1 + dY * ( j + 1 ), Z );
			VA->EmitVertex( X1 + dX * ( i + 1 ), Y1 + dY * j,     Z );

			VA->EmitVertex( X1 + dX * ( i + 1 ), Y1 + dY * j,     Z );
			VA->EmitVertex( X1 + dX * i,     Y1 + dY * ( j + 1 ), Z );
			VA->EmitVertex( X1 + dX * ( i + 1 ), Y1 + dY * ( j + 1 ), Z );
		}
	}
}

clPtr<clVertexAttribs> clGeomServ::CreatePlane( float SizeX, float SizeY, int SegmentsX, int SegmentsY, float Z )
{
	clPtr<clVertexAttribs> VA = new clVertexAttribs();

	VA->Restart( SegmentsX * SegmentsY * 6 );

	AddPlane( VA, SizeX, SizeY, SegmentsX, SegmentsY, Z );

	return VA;
}

const int NUM_CUBE_VTX = 36;

void clGeomServ::AddAxisAlignedBox( const clPtr<clVertexAttribs>& VA, const LVector3& Min, const LVector3& Max )
{
	// top
	VA->SetNormalV( LVector3( 0, 0, 1 ) );
	VA->SetTexCoord( 0, 0, 0 );
	VA->EmitVertex( Min.x, Min.y, Max.z );
	VA->SetTexCoord( 1, 0, 0 );
	VA->EmitVertex( Max.x, Min.y, Max.z );
	VA->SetTexCoord( 1, 1, 0 );
	VA->EmitVertex( Max.x, Max.y, Max.z );

	VA->SetTexCoord( 1, 1, 0 );
	VA->EmitVertex( Max.x, Max.y, Max.z );
	VA->SetTexCoord( 0, 1, 0 );
	VA->EmitVertex( Min.x, Max.y, Max.z );
	VA->SetTexCoord( 0, 0, 0 );
	VA->EmitVertex( Min.x, Min.y, Max.z );

	// bottom
	VA->SetNormalV( LVector3( 0, 0, -1 ) );
	VA->SetTexCoord( 0, 0, 0 );
	VA->EmitVertex( Min.x, Min.y, Min.z );
	VA->SetTexCoord( 0, 1, 0 );
	VA->EmitVertex( Min.x, Max.y, Min.z );
	VA->SetTexCoord( 1, 1, 0 );
	VA->EmitVertex( Max.x, Max.y, Min.z );

	VA->SetTexCoord( 1, 1, 0 );
	VA->EmitVertex( Max.x, Max.y, Min.z );
	VA->SetTexCoord( 1, 0, 0 );
	VA->EmitVertex( Max.x, Min.y, Min.z );
	VA->SetTexCoord( 0, 0, 0 );
	VA->EmitVertex( Min.x, Min.y, Min.z );

	// left
	VA->SetNormalV( LVector3( 0, 1, 0 ) );
	VA->SetTexCoord( 1, 0, 0 );
	VA->EmitVertex( Max.x, Max.y, Min.z );
	VA->SetTexCoord( 0, 0, 0 );
	VA->EmitVertex( Min.x, Max.y, Min.z );
	VA->SetTexCoord( 0, 1, 0 );
	VA->EmitVertex( Min.x, Max.y, Max.z );

	VA->SetTexCoord( 0, 1, 0 );
	VA->EmitVertex( Min.x, Max.y, Max.z );
	VA->SetTexCoord( 1, 1, 0 );
	VA->EmitVertex( Max.x, Max.y, Max.z );
	VA->SetTexCoord( 1, 0, 0 );
	VA->EmitVertex( Max.x, Max.y, Min.z );

	// right
	VA->SetNormalV( LVector3( 0, -1, 0 ) );
	VA->SetTexCoord( 0, 0, 0 );
	VA->EmitVertex( Min.x, Min.y, Min.z );
	VA->SetTexCoord( 1, 0, 0 );
	VA->EmitVertex( Max.x, Min.y, Min.z );
	VA->SetTexCoord( 1, 1, 0 );
	VA->EmitVertex( Max.x, Min.y, Max.z );

	VA->SetTexCoord( 1, 1, 0 );
	VA->EmitVertex( Max.x, Min.y, Max.z );
	VA->SetTexCoord( 0, 1, 0 );
	VA->EmitVertex( Min.x, Min.y, Max.z );
	VA->SetTexCoord( 0, 0, 0 );
	VA->EmitVertex( Min.x, Min.y, Min.z );

	// front
	VA->SetNormalV( LVector3( 1, 0, 0 ) );
	VA->SetTexCoord( 0, 0, 0 );
	VA->EmitVertex( Max.x, Min.y, Min.z );
	VA->SetTexCoord( 1, 0, 0 );
	VA->EmitVertex( Max.x, Max.y, Min.z );
	VA->SetTexCoord( 1, 1, 0 );
	VA->EmitVertex( Max.x, Max.y, Max.z );

	VA->SetTexCoord( 1, 1, 0 );
	VA->EmitVertex( Max.x, Max.y, Max.z );
	VA->SetTexCoord( 0, 1, 0 );
	VA->EmitVertex( Max.x, Min.y, Max.z );
	VA->SetTexCoord( 0, 0, 0 );
	VA->EmitVertex( Max.x, Min.y, Min.z );

	// back
	VA->SetNormalV( LVector3( -1, 0, 0 ) );
	VA->SetTexCoord( 1, 0, 0 );
	VA->EmitVertex( Min.x, Max.y, Min.z );
	VA->SetTexCoord( 0, 0, 0 );
	VA->EmitVertex( Min.x, Min.y, Min.z );
	VA->SetTexCoord( 0, 1, 0 );
	VA->EmitVertex( Min.x, Min.y, Max.z );

	VA->SetTexCoord( 0, 1, 0 );
	VA->EmitVertex( Min.x, Min.y, Max.z );
	VA->SetTexCoord( 1, 1, 0 );
	VA->EmitVertex( Min.x, Max.y, Max.z );
	VA->SetTexCoord( 1, 0, 0 );
	VA->EmitVertex( Min.x, Max.y, Min.z );
}

clPtr<clVertexAttribs> clGeomServ::CreateAxisAlignedBox( const LVector3& Min, const LVector3& Max )
{
	clPtr<clVertexAttribs> VA = new clVertexAttribs();

	// Triangles go in order to produce correct normal facing
	VA->Restart( NUM_CUBE_VTX );

	AddAxisAlignedBox( VA, Min, Max );

	return VA;
}

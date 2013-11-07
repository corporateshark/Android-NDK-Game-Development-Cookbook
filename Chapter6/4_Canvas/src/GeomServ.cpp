/**
 * \file GeomServ.cpp
 * \brief Geometry server implementation
 * \version 0.6.00
 * \date 27/01/2011
 * \author Sergey Kosarevsky, 2005-2011
 * \author support@linderdaum.com http://www.linderdaum.com
 */

#include "GeomServ.h"
#include "VertexAttribs.h"

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

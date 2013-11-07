/**
 * \file VertexAttribs.cpp
 * \brief Host-side vertex arrays
 * \version 0.6.00
 * \date 11/01/2011
 * \author Sergey Kosarevsky, 2010-2011
 * \author support@linderdaum.com http://www.linderdaum.com
 */

#include "VertexAttribs.h"

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

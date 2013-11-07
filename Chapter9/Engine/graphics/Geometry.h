#pragma once

#include "Platform.h"
#include "VecMath.h"
#include "fs/Streams.h"
#include "core/iObject.h"
#include "iIntrusivePtr.h"

#include <vector>

// vertex streams mapping (used as vertex shader inputs)
const int L_VS_VERTEX   = 0; // vec3
const int L_VS_TEXCOORD = 1; // vec2
const int L_VS_NORMAL   = 2; // vec3
const int L_VS_COLORS   = 3; // vec4

const int L_VS_TOTAL_ATTRIBS = L_VS_COLORS + 1;

/// number of float components in every stream
const int L_VS_VEC_COMPONENTS[ L_VS_TOTAL_ATTRIBS ] = { 3, 2, 3, 4 };

/// Container for vertex attribs (think about it like a mesh without internal hierarchy)
class clVertexAttribs: public iObject
{
public:
	clVertexAttribs();
	clVertexAttribs( size_t Vertices );

	void   SetActiveVertexCount( size_t Count ) { FActiveVertexCount = Count; }
	size_t GetActiveVertexCount() const { return FActiveVertexCount; }

	/// map all vertex attribs (vertices, texcoords, normals, etc) to L_VS_TOTAL_ATTRIBS vertex streams
	const std::vector<const void*>& EnumerateVertexStreams() const;

	void Restart( size_t ReserveVertices );
	void EmitVertexV( const LVector3& Vec );
	void EmitVertex( float X, float Y, float Z ) { EmitVertexV( LVector3( X, Y, Z ) ); };
	void SetTexCoord( float U, float V, float W ) { SetTexCoordV( LVector2( U, V ) ); };
	void SetTexCoordV( const LVector2& V );
	void SetNormalV( const LVector3& Vec );
	void SetColorV( const LVector4& Vec );

public:

	/// vertex position X, Y, Z
	std::vector<LVector3> FVertices;

	/// vertex tex coord U, V
	std::vector<LVector2> FTexCoords;

	/// vertex normal in object space
	std::vector<LVector3> FNormals;

	/// vertex RGBA colors
	std::vector<LVector4> FColors;

private:
	/// current number of acive vertices used for rendering
	size_t FActiveVertexCount;

	/// per-vertex generation
	LVector2 FCurrentTexCoords;
	LVector3 FCurrentNormal;
	LVector4 FCurrentColor;
	/// precached vertex streams
	mutable std::vector<const void*> FStreams;
};

/// Collection of triangulation/facetting functions
class clGeomServ
{
public:
	static clPtr<clVertexAttribs> CreateTriangle2D( float vX, float vY, float dX, float dY, float Z );
	static clPtr<clVertexAttribs> CreateRect2D( float X1, float Y1, float X2, float Y2, float Z, bool FlipTexCoordsVertical, int Subdivide );

	static void AddAxisAlignedBox( const clPtr<clVertexAttribs>& VA, const LVector3& Min, const LVector3& Max );
	static clPtr<clVertexAttribs> CreateAxisAlignedBox( const LVector3& Min, const LVector3& Max );

	static void AddPlane( const clPtr<clVertexAttribs>& VA, float SizeX, float SizeY, int SegmentsX, int SegmentsY, float Z );
	static clPtr<clVertexAttribs> CreatePlane( float SizeX, float SizeY, int SegmentsX, int SegmentsY, float Z );
};

#pragma once

#include "Platform.h"
#include "VecMath.h"
#include "iIntrusivePtr.h"

class clVertexAttribs;

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

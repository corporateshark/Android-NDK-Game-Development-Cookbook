#pragma once
#include "VecMath.h"

inline vec3 Unproject( const mtx4& _proj, const mtx4& _view, const vec3& _vec )
{
	LVector4 v4( _vec.x, _vec.y, _vec.z, 1.0f );
	LVector4 p = _view.GetInversed() * ( _proj.GetInversed() * v4 );
	vec3 res( p.x, p.y, p.z );

	if ( fabs( p.w ) > Linderdaum::Math::EPSILON ) { res /= p.w; }

	return res;
}

inline void MouseCoordsToWorldPointAndRay( const mtx4& _proj, const mtx4& _view, float mx, float my, vec3& srcPoint, vec3& dir )
{
	vec3 screenPoint ( 2.0f * mx - 1.0f, -2.0f * my + 1.0f, 0.0f );
	vec3 worldPoint = Unproject( _proj, _view, screenPoint );
	mtx4 CamRotation;
	DecomposeCameraTransformation( _view, srcPoint, CamRotation );
	dir = worldPoint - srcPoint;
}

inline bool IntersectRayToPlane( const vec3& P, const vec3& A, const vec3& N, float D, vec3& isect )
{
	float denom = A.Dot( N );

	if ( fabs( denom ) < 0.0001f ) { return false; }

	float t = -( D + P.Dot( N ) ) / denom;
	isect = P + A * t;
	return true;
}

inline bool IntersectRayToTriangle( const vec3& P, const vec3& A, const vec3& V1, const vec3& V2, const vec3& V3, vec3& isect )
{
	vec3 N = ( V3 - V2 ).Cross( V2 - V1 );

	if ( !IntersectRayToPlane( P, A, N , -N.Dot( V1 ), isect ) ) { return false; }

	// try cross products
	vec3 l1 = V1 - isect;
	vec3 l2 = V2 - isect;
	vec3 l3 = V3 - isect;
	float lam1 = ( l1 ).Cross( l2 ).Dot( N );
	float lam2 = ( l2 ).Cross( l3 ).Dot( N );
	float lam3 = ( l3 ).Cross( l1 ).Dot( N );
	// unnormalized barycentric coordinates must be of the same sign (the sign depends on N's direction and does not matter)
	return ( ( lam1 < 0 && lam2 < 0 && lam3 < 0 ) || ( lam1 > 0 && lam2 > 0 && lam3 > 0 ) );
}

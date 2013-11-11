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

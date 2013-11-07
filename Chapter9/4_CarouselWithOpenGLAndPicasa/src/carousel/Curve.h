#pragma once
#include "VecMath.h"
#include <vector>

class Curve
{
public:
	Curve() {}
	void AddControlPoint( float t, const vec3& Pos )
	{
		T.push_back( t );
		P.push_back( Pos );
	}

	/// Get the position at a given point
	vec3 GetPosition( float t ) const
	{
		int N = ( int )T.size();
		int i = N - 1;

		if ( t <= T[0] ) { return P[0]; }

		for ( int s = 0 ; s < N - 1 ; s++ )
			if ( t > T[s] && t <= T[s + 1] )
			{
				i = s;
				break;
			}

		if ( i >= N - 1 ) { return P[N - 1]; }

		vec3 k = ( P[i + 1] - P[i] ) / ( T[i + 1] - T[i] );
		return k * ( t - T[i] ) + P[i];
	}
public:
	/// Arguments and control points
	std::vector<float> T;
	std::vector<vec3>  P;
};

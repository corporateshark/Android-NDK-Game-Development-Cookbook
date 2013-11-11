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

#include "VecMath.h"
#include <cstdlib>

//
// LMatrix3
//

const LMatrix3 Mat3( LVector3( 1, 0, 0 ),
                     LVector3( 0, 1, 0 ),
                     LVector3( 0, 0, 1 ) );

const LMatrix3& LMatrix3::Identity()
{
	return Mat3;
}

void LMatrix3::Inverse()
{
	LMatrix3 Inverse;

	Inverse[0][0] = FMatrix[1][1] * FMatrix[2][2] - FMatrix[1][2] * FMatrix[2][1];
	Inverse[1][0] = FMatrix[1][2] * FMatrix[2][0] - FMatrix[1][0] * FMatrix[2][2];
	Inverse[2][0] = FMatrix[1][0] * FMatrix[2][1] - FMatrix[1][1] * FMatrix[2][0];

	float Determinant = FMatrix[0][0] * Inverse[0][0] + FMatrix[0][1] * Inverse[1][0] + FMatrix[0][2] * Inverse[2][0];

	if ( fabsf( Determinant ) < ::Linderdaum::Math::EPSILON )
	{
		return;
	}

	float InvDeterminant = 1.0f / Determinant;

	Inverse[0][1] = FMatrix[0][2] * FMatrix[2][1] - FMatrix[0][1] * FMatrix[2][2];
	Inverse[0][2] = FMatrix[0][1] * FMatrix[1][2] - FMatrix[0][2] * FMatrix[1][1];
	Inverse[1][1] = FMatrix[0][0] * FMatrix[2][2] - FMatrix[0][2] * FMatrix[2][0];
	Inverse[1][2] = FMatrix[0][2] * FMatrix[1][0] - FMatrix[0][0] * FMatrix[1][2];
	Inverse[2][1] = FMatrix[0][1] * FMatrix[2][0] - FMatrix[0][0] * FMatrix[2][1];
	Inverse[2][2] = FMatrix[0][0] * FMatrix[1][1] - FMatrix[0][1] * FMatrix[1][0];

	FMatrix[0][0] = Inverse[0][0] * InvDeterminant;
	FMatrix[0][1] = Inverse[0][1] * InvDeterminant;
	FMatrix[0][2] = Inverse[0][2] * InvDeterminant;

	FMatrix[1][0] = Inverse[1][0] * InvDeterminant;
	FMatrix[1][1] = Inverse[1][1] * InvDeterminant;
	FMatrix[1][2] = Inverse[1][2] * InvDeterminant;

	FMatrix[2][0] = Inverse[2][0] * InvDeterminant;
	FMatrix[2][1] = Inverse[2][1] * InvDeterminant;
	FMatrix[2][2] = Inverse[2][2] * InvDeterminant;
}

void LMatrix3::Orthonormalize()
{
	FMatrix[0].Normalize();
	FMatrix[2] = FMatrix[0].Cross( FMatrix[1] );
	FMatrix[2].Normalize();
	FMatrix[1] = FMatrix[2].Cross( FMatrix[0] );
	FMatrix[1].Normalize();
}

LMatrix3 LMatrix3::GetInversed() const
{
	LMatrix3 Mat = ( *this );

	Mat.Inverse();

	return Mat;
}

LMatrix3 LMatrix3::GetTransposed() const
{
	LMatrix3 Mat;

	for ( int i = 0; i != 3; i++ )
	{
		for ( int j = 0; j != 3; j++ )
		{
			Mat[ i ][ j ] = FMatrix[ j ][ i ];
		}
	}

	return Mat;
}

//
// LMatrix4
//

const LMatrix4 Mat4( LVector4( 1, 0, 0, 0 ),
                     LVector4( 0, 1, 0, 0 ),
                     LVector4( 0, 0, 1, 0 ),
                     LVector4( 0, 0, 0, 1 ) );

const LMatrix4& LMatrix4::Identity()
{
	return Mat4;
}

LMatrix4 LMatrix4::IdentityStatic()
{
	return LMatrix4( LVector4( 1, 0, 0, 0 ),
	                 LVector4( 0, 1, 0, 0 ),
	                 LVector4( 0, 0, 1, 0 ),
	                 LVector4( 0, 0, 0, 1 ) );
}

LMatrix4 LMatrix4::GetDiagonalMatrix( float Diag )
{
	return LMatrix4( vec4( Diag, 0.0f, 0.0f, 0.0f ), vec4( 0.0f, Diag, 0.0f, 0.0f ), vec4( 0.0f, 0.0f, Diag, 0.0f ), vec4( 0.0f, 0.0f, 0.0f, Diag ) );
}

LMatrix4 LMatrix4::GetDiagonalMatrixV( const LVector4& Diag )
{
	return LMatrix4( vec4( Diag.x, 0.0f, 0.0f, 0.0f ), vec4( 0.0f, Diag.y, 0.0f, 0.0f ), vec4( 0.0f, 0.0f, Diag.z, 0.0f ), vec4( 0.0f, 0.0f, 0.0f, Diag.w ) );
}

LMatrix4 LMatrix4::GetTranslateMatrix( const LVector3& Vector )
{
	LMatrix4 Mat;

	Mat.TranslateMatrix( Vector );

	return Mat;
}

LMatrix4 LMatrix4::GetScaleMatrix( const LVector3& Vector )
{
	LMatrix4 Mat;

	Mat.ScaleMatrix( Vector );

	return Mat;
}

LMatrix4 LMatrix4::GetFromPitchPanRoll( float Pitch, float Pan, float Roll )
{
	LMatrix4 Mat;

	Mat.FromPitchPanRoll( Pitch, Pan, Roll );

	return Mat;
}

LMatrix4 LMatrix4::GetRotateMatrixAxis( const float Angle, const LVector3& Axis )
{
	LMatrix4 Mat;

	Mat.RotateMatrixAxis( Angle, Axis );

	return Mat;
}

bool LMatrix4::IsIdentityMatrix() const
{
	const float* M1Ptr = ToFloatPtr();
	const float* M2Ptr = ::Linderdaum::Math::IdentityMatrix4().ToFloatPtr();

	for ( int i = 0; i != 16; ++i )
	{
		if ( M1Ptr[i] != M2Ptr[i] )
		{
			return false;
		}
	}

	return true;
}

LMatrix4 LMatrix4::GetInversed() const
{
	LMatrix4 Mat = ( *this );

	Mat.Inverse();

	return Mat;
}

LMatrix3 LMatrix4::ToMatrix3() const
{
	LMatrix3 Mat;

	Mat[0] = FMatrix[0].ToVector3();
	Mat[1] = FMatrix[1].ToVector3();
	Mat[2] = FMatrix[2].ToVector3();

	return Mat;
}

void LMatrix4::Inverse()
{
	// 2x2 sub-determinants required to calculate 4x4 Determinant
	float Det2_01_01 = FMatrix[0][0] * FMatrix[1][1] - FMatrix[0][1] * FMatrix[1][0];
	float Det2_01_02 = FMatrix[0][0] * FMatrix[1][2] - FMatrix[0][2] * FMatrix[1][0];
	float Det2_01_03 = FMatrix[0][0] * FMatrix[1][3] - FMatrix[0][3] * FMatrix[1][0];
	float Det2_01_12 = FMatrix[0][1] * FMatrix[1][2] - FMatrix[0][2] * FMatrix[1][1];
	float Det2_01_13 = FMatrix[0][1] * FMatrix[1][3] - FMatrix[0][3] * FMatrix[1][1];
	float Det2_01_23 = FMatrix[0][2] * FMatrix[1][3] - FMatrix[0][3] * FMatrix[1][2];

	// 3x3 sub-determinants required to calculate 4x4 Determinant
	float Det3_201_012 = FMatrix[2][0] * Det2_01_12 - FMatrix[2][1] * Det2_01_02 + FMatrix[2][2] * Det2_01_01;
	float Det3_201_013 = FMatrix[2][0] * Det2_01_13 - FMatrix[2][1] * Det2_01_03 + FMatrix[2][3] * Det2_01_01;
	float Det3_201_023 = FMatrix[2][0] * Det2_01_23 - FMatrix[2][2] * Det2_01_03 + FMatrix[2][3] * Det2_01_02;
	float Det3_201_123 = FMatrix[2][1] * Det2_01_23 - FMatrix[2][2] * Det2_01_13 + FMatrix[2][3] * Det2_01_12;

	float Determinant = ( - Det3_201_123 * FMatrix[3][0] + Det3_201_023 * FMatrix[3][1] - Det3_201_013 * FMatrix[3][2] + Det3_201_012 * FMatrix[3][3] );

	if ( fabsf( Determinant ) < ::Linderdaum::Math::EPSILON )
	{
		return;
	}

	float InvDeterminant = 1.0f / Determinant;

	// remaining 2x2 sub-determinants
	float Det2_03_01 = FMatrix[0][0] * FMatrix[3][1] - FMatrix[0][1] * FMatrix[3][0];
	float Det2_03_02 = FMatrix[0][0] * FMatrix[3][2] - FMatrix[0][2] * FMatrix[3][0];
	float Det2_03_03 = FMatrix[0][0] * FMatrix[3][3] - FMatrix[0][3] * FMatrix[3][0];
	float Det2_03_12 = FMatrix[0][1] * FMatrix[3][2] - FMatrix[0][2] * FMatrix[3][1];
	float Det2_03_13 = FMatrix[0][1] * FMatrix[3][3] - FMatrix[0][3] * FMatrix[3][1];
	float Det2_03_23 = FMatrix[0][2] * FMatrix[3][3] - FMatrix[0][3] * FMatrix[3][2];

	float Det2_13_01 = FMatrix[1][0] * FMatrix[3][1] - FMatrix[1][1] * FMatrix[3][0];
	float Det2_13_02 = FMatrix[1][0] * FMatrix[3][2] - FMatrix[1][2] * FMatrix[3][0];
	float Det2_13_03 = FMatrix[1][0] * FMatrix[3][3] - FMatrix[1][3] * FMatrix[3][0];
	float Det2_13_12 = FMatrix[1][1] * FMatrix[3][2] - FMatrix[1][2] * FMatrix[3][1];
	float Det2_13_13 = FMatrix[1][1] * FMatrix[3][3] - FMatrix[1][3] * FMatrix[3][1];
	float Det2_13_23 = FMatrix[1][2] * FMatrix[3][3] - FMatrix[1][3] * FMatrix[3][2];

	// remaining 3x3 sub-determinants
	float Det3_203_012 = FMatrix[2][0] * Det2_03_12 - FMatrix[2][1] * Det2_03_02 + FMatrix[2][2] * Det2_03_01;
	float Det3_203_013 = FMatrix[2][0] * Det2_03_13 - FMatrix[2][1] * Det2_03_03 + FMatrix[2][3] * Det2_03_01;
	float Det3_203_023 = FMatrix[2][0] * Det2_03_23 - FMatrix[2][2] * Det2_03_03 + FMatrix[2][3] * Det2_03_02;
	float Det3_203_123 = FMatrix[2][1] * Det2_03_23 - FMatrix[2][2] * Det2_03_13 + FMatrix[2][3] * Det2_03_12;

	float Det3_213_012 = FMatrix[2][0] * Det2_13_12 - FMatrix[2][1] * Det2_13_02 + FMatrix[2][2] * Det2_13_01;
	float Det3_213_013 = FMatrix[2][0] * Det2_13_13 - FMatrix[2][1] * Det2_13_03 + FMatrix[2][3] * Det2_13_01;
	float Det3_213_023 = FMatrix[2][0] * Det2_13_23 - FMatrix[2][2] * Det2_13_03 + FMatrix[2][3] * Det2_13_02;
	float Det3_213_123 = FMatrix[2][1] * Det2_13_23 - FMatrix[2][2] * Det2_13_13 + FMatrix[2][3] * Det2_13_12;

	float Det3_301_012 = FMatrix[3][0] * Det2_01_12 - FMatrix[3][1] * Det2_01_02 + FMatrix[3][2] * Det2_01_01;
	float Det3_301_013 = FMatrix[3][0] * Det2_01_13 - FMatrix[3][1] * Det2_01_03 + FMatrix[3][3] * Det2_01_01;
	float Det3_301_023 = FMatrix[3][0] * Det2_01_23 - FMatrix[3][2] * Det2_01_03 + FMatrix[3][3] * Det2_01_02;
	float Det3_301_123 = FMatrix[3][1] * Det2_01_23 - FMatrix[3][2] * Det2_01_13 + FMatrix[3][3] * Det2_01_12;

	FMatrix[0][0] = - Det3_213_123 * InvDeterminant;
	FMatrix[1][0] = + Det3_213_023 * InvDeterminant;
	FMatrix[2][0] = - Det3_213_013 * InvDeterminant;
	FMatrix[3][0] = + Det3_213_012 * InvDeterminant;

	FMatrix[0][1] = + Det3_203_123 * InvDeterminant;
	FMatrix[1][1] = - Det3_203_023 * InvDeterminant;
	FMatrix[2][1] = + Det3_203_013 * InvDeterminant;
	FMatrix[3][1] = - Det3_203_012 * InvDeterminant;

	FMatrix[0][2] = + Det3_301_123 * InvDeterminant;
	FMatrix[1][2] = - Det3_301_023 * InvDeterminant;
	FMatrix[2][2] = + Det3_301_013 * InvDeterminant;
	FMatrix[3][2] = - Det3_301_012 * InvDeterminant;

	FMatrix[0][3] = - Det3_201_123 * InvDeterminant;
	FMatrix[1][3] = + Det3_201_023 * InvDeterminant;
	FMatrix[2][3] = - Det3_201_013 * InvDeterminant;
	FMatrix[3][3] = + Det3_201_012 * InvDeterminant;
}

LMatrix3 LMatrix4::ExtractMatrix3() const
{
	LMatrix3 Res;

	for ( int i = 0 ; i < 3 ; i++ )
	{
		for ( int j = 0; j < 3 ; j++ )
		{
			Res[i][j] = FMatrix[i][j];
		}
	}

	return Res;
}

void LMatrix4::SetSubMatrix( const LMatrix3& Mtx )
{
	for ( int i = 0 ; i < 3 ; i++ )
	{
		for ( int j = 0; j < 3 ; j++ )
		{
			FMatrix[i][j] = Mtx[i][j];
		}
	}
}

LMatrix4 LMatrix4::FromPitchPanRoll( float Pitch, float Pan, float Roll )
{
	LMatrix4 RollMtx;

	RollMtx.IdentityMatrix();
	RollMtx.RotateMatrixAxis( Roll * ::Linderdaum::Math::DTOR, LVector3( 0, 1, 0 ) );

	LMatrix4 PanMtx;

	PanMtx.IdentityMatrix();
	PanMtx.RotateMatrixAxis( Pan * ::Linderdaum::Math::DTOR, LVector3( 1, 0, 0 ) );

	LMatrix4 PitchMtx;

	PitchMtx.IdentityMatrix();
	PitchMtx.RotateMatrixAxis( Pitch * ::Linderdaum::Math::DTOR, LVector3( 0, 0, 1 ) );

	*this = PitchMtx * PanMtx * RollMtx;

	return *this;
}

template<class T> T det33( T a11, T a12, T a13,
                           T a21, T a22, T a23,
                           T a31, T a32, T a33 )
{
	return ( a11 * ( a22 * a33 - a23 * a32 ) - a12 * ( a21 * a33 - a31 * a23 ) + a13 * ( a21 * a32 - a22 * a31 ) );
}

float LMatrix4::Det() const
{
	float d1, d2, d3, d4;

	d1 = det33( FMatrix[1][1], FMatrix[1][2], FMatrix[1][3],
	            FMatrix[2][1], FMatrix[2][2], FMatrix[2][3],
	            FMatrix[3][1], FMatrix[3][2], FMatrix[3][3] );

	d2 = det33( FMatrix[1][0], FMatrix[1][2], FMatrix[0][3],
	            FMatrix[2][0], FMatrix[2][2], FMatrix[2][3],
	            FMatrix[3][0], FMatrix[3][2], FMatrix[3][3] );

	d3 = det33( FMatrix[1][0], FMatrix[1][1], FMatrix[0][3],
	            FMatrix[2][0], FMatrix[2][1], FMatrix[2][3],
	            FMatrix[3][0], FMatrix[3][1], FMatrix[3][3] );

	d4 = det33( FMatrix[1][0], FMatrix[1][1], FMatrix[0][2],
	            FMatrix[2][0], FMatrix[2][1], FMatrix[2][2],
	            FMatrix[3][0], FMatrix[3][1], FMatrix[3][2] );

	return ( FMatrix[0][0] * d1 - FMatrix[0][1] * d2 + FMatrix[0][2] * d3 - FMatrix[0][3] * d4 );
}


// Jacobi rotations for the 3x3 matrix. From Numerical Recipes.
// returns the number of iterations
int JacobiRotations3( LMatrix3& A, LVector3* V, float* D )
{
	int nrot = 0;

	int i, j, iq, ip;
	float tresh, theta, tau, t, sm, s, h, g, c;
	float b[3], z[3];

	for ( ip = 0; ip < 3 ; ip++ )
	{
		for ( iq = 0; iq < 3 ; iq++ )
		{
			V[ip][iq] = 0.0f;
		}

		V[ip][ip] = 1.0f;
	}

	for ( ip = 0; ip < 3; ip++ )
	{
		D[ip] = b[ip] = A[ip][ip];

		z[ip] = 0.0;
	}

	nrot = 0;

	for ( i = 0 ; i < 50 ; i++ )
	{
		// sum absolute values of the off-diagonal terms
		sm = fabs( A[0][1] ) + fabs( A[0][2] ) + fabs( A[1][2] );

		if ( sm == 0.0 )
		{
			break;
		}

		if ( i < 4 )
		{
			tresh = 0.2f * sm / 9.0f;
		}
		else
		{
			tresh = 0.0f;
		}

		for ( ip = 0 ; ip < 2 ; ip++ )
		{
			for ( iq = ip + 1 ; iq < 3 ; iq++ )
			{
				g = 100.0f * fabs( A[ip][iq] );

				if ( ( i > 4 ) && ( ( fabs( D[ip] ) + g ) == fabs( D[ip] ) )
				     && ( ( fabs( D[iq] ) + g ) == fabs( D[iq] ) ) )
				{
					A[ip][iq] = 0.0f;
				}

				if ( fabs( A[ip][iq] ) > tresh )
				{
					h = D[iq] - D[ip];

					if ( ( fabs( h ) + g ) == fabs( h ) )
					{
						t = A[ip][iq] / h;
					}
					else
					{
						theta = 0.5f * h / A[ip][iq];
						t = 1.0f / ( fabs( theta ) + sqrt( 1.0f + theta * theta ) );

						if ( theta < 0.0f )
						{
							t = -t;
						}
					}

					c = 1.0f / sqrt( 1 + t * t );
					s = t * c;
					tau = s / ( 1.0f + c );
					h = t * A[ip][iq];
					z[ip] = z[ip] - h;
					z[iq] = z[iq] + h;

					D[ip] = D[ip] - h;
					D[iq] = D[iq] + h;

					A[ip][iq] = 0.0f;

					for ( j = 0 ; j < ip ; j++ )
					{
						g = A[j][ip];
						h = A[j][iq];
						A[j][ip] = g - s * ( h + g * tau );
						A[j][iq] = h + s * ( g - h * tau );
					}

					for ( j = ip + 1 ; j < iq ; j++ )
					{
						g = A[ip][j];
						h = A[j][iq];

						A[ip][j] = g - s * ( h + g * tau );
						A[j][iq] = h + s * ( g - h * tau );
					}

					for ( j = iq + 1 ; j < 3 ; j++ )
					{
						g = A[ip][j];
						h = A[iq][j];

						A[ip][j] = g - s * ( h + g * tau );
						A[iq][j] = h + s * ( g - h * tau );
					}

					for ( j = 0 ; j < 3 ; j++ )
					{
						g = V[j][ip];
						h = V[j][iq];

						V[j][ip] = g - s * ( h + g * tau );
						V[j][iq] = h + s * ( g - h * tau );
					}

					++nrot;
				}
			}
		}
	}

	for ( ip = 0 ; ip < 3 ; ip++ )
	{
		b[ip] += z[ip];
		D[ip]  = b[ip];
		z[ip]  = 0.0f;
	}

	return nrot;
}

void LMatrix3::CalculateEigenVectors( LVector3* V, float* D ) const
{
	LMatrix3 A = *this;

	int n = JacobiRotations3( A, V, D );

	if ( n > 49 )
	{
		// no convergence
		V[1] = vec3( 1, 0, 0 );
		V[2] = vec3( 0, 1, 0 );
		V[3] = vec3( 0, 0, 1 );
	}
}

/*
 * 07/03/2010
    JacobiRotations, CalcEigenVectors
 * 12/03/2007
     FromPitchPanRoll()
     GetPPRMatrix()
 * 05/06/2006
     Inverse()
     GetInversed()
 * 30/04/2006
     Cleaned
 * 02/03/2005
     LMatrix4 multiplication
 * 16/12/2004
     Perspective()
     Frustum()
     LookAt()
 * 15/12/2004
     It's here
*/

/**
 * \file LVector.cpp
 * \brief 2D/3D/4D vectors
 * \version 0.5.93
 * \date 04/11/2010
 * \author Sergey Kosarevsky, 2005-2010
 * \author support@linderdaum.com http://www.linderdaum.com
 */

LVector2::LVector2( const LVector2i& Vec )
	: x( static_cast<float>( Vec.x ) ),
	  y( static_cast<float>( Vec.y ) )
{
}

void LVector2::Normalize()
{
	float VecLength = Length();

	if ( VecLength > ::Linderdaum::Math::EPSILON ) { *this *= ( 1.0f / VecLength ); }
}

void LVector3::Normalize()
{
	float VecLength = Length();

	if ( VecLength > ::Linderdaum::Math::EPSILON ) { *this *= ( 1.0f / VecLength ); }
}

void LVector4::Normalize()
{
	float VecLength = Length();

	if ( VecLength > ::Linderdaum::Math::EPSILON ) { *this *= ( 1.0f / VecLength ); }
}

const LVector4& LVector4::Saturate()
{
	Linderdaum::Math::Clamp( x, 0.0f, 1.0f );
	Linderdaum::Math::Clamp( y, 0.0f, 1.0f );
	Linderdaum::Math::Clamp( z, 0.0f, 1.0f );
	Linderdaum::Math::Clamp( w, 0.0f, 1.0f );

	return *this;
}

/*
 * 04/11/2010
     LVector3 unary operator-
 * 03/11/2010
     LVector4i::operator==
     LVector4i::operator!=
 * 16/09/2010
     LVector3i
 * 03/06/2006
     Lerp()
 * 30/05/2005
     OrthogonalVector()
 * 19/03/2005
     More vector operators
 * 16/12/2004
     Normalize()
 * 15/12/2004
     It's here
*/

/**
 * \file LTransform.cpp
 * \brief Transformation
 * \version 0.5.91
 * \date 01/03/2010
 * \author Viktor Latypov, 2009-2010
 * \author support@linderdaum.com http://www.linderdaum.com
 */

void LTransform::SetPosition( const LVector3& P )
{
	FMatrix[3][0] = P.x;
	FMatrix[3][1] = P.y;
	FMatrix[3][2] = P.z;
}

LVector3 LTransform::GetPosition() const
{
	return FMatrix[3].ToVector3();
}

LMatrix4 ComposeTransformation( const LVector3& Pos, const LMatrix4& Rot )
{
	return LMatrix4::GetTranslateMatrix( Pos ) * Rot;
}

LMatrix4 ComposeCameraTransformation( const LVector3& Pos, const LMatrix4& Rot )
{
	return LMatrix4::GetTranslateMatrix( -Pos ) * Rot;
}

// Decompose T to rotation and translation
void DecomposeTransformation( const LMatrix4& T, LVector3& Pos, LMatrix4& Rot )
{
	LMatrix4 InvT = T.GetInversed();

	for ( int i = 0 ; i < 3 ; i++ )
	{
		for ( int j = 0 ; j < 3 ; j++ )
		{
			Rot[i][j] = InvT[j][i];
		}
	}

	Pos = Rot.GetInversed() * vec3( -InvT[3].x, -InvT[3].y, -InvT[3].z );
}

// Decompose camera T to translation and rotation
void DecomposeCameraTransformation( const LMatrix4& T, LVector3& Pos, LMatrix4& Rot )
{
	LMatrix4 InvT = T;

	Rot.IdentityMatrix();
	LMatrix4 RotInv;
	RotInv.IdentityMatrix();

	for ( int i = 0 ; i < 3 ; i++ )
	{
		for ( int j = 0 ; j < 3 ; j++ )
		{
			Rot[i][j] = InvT[i][j];
			RotInv[i][j] = InvT[j][i];
		}
	}

// vec3 Pos1 = RotInv * vec3(-InvT[3].X, -InvT[3].Y, -InvT[3].Z);
// vec3 Pos2 = RotInv * vec3(InvT[3].X, InvT[3].Y, InvT[3].Z);
// vec3 Pos3 = Rot * vec3(-InvT[3].X, -InvT[3].Y, -InvT[3].Z);
// vec3 Pos4 = Rot * vec3(InvT[3].X, InvT[3].Y, InvT[3].Z);

	Pos = RotInv * vec3( -InvT[3].x, -InvT[3].y, -InvT[3].z );
}


LTransform::LTransform()
{
	FMatrix = LMatrix4::Identity();
	FAngleSystem = Euler_zxys;
}

void LTransform::SetAngles( const LVector3& Angles )
{
	SetAngleTriple( Angles.x, Angles.y, Angles.z );
}

void LTransform::SetAngleTriple( float T1, float T2, float T3 )
{
	LMatrix4 M;
	AnglesToMatrix( FAngleSystem, M, T1, T2, T3 );

	for ( int i = 0 ; i < 3 ; i++ )
		for ( int j = 0 ; j < 3 ; j++ )
		{
			FMatrix[i][j] = M[i][j];
		}
}

LVector3 LTransform::GetAngles() const
{
	float T1, T2, T3;

	GetAngleTriple( T1, T2, T3 );

	return LVector3( T1, T2, T3 );
}

void LTransform::GetAngleTriple( float& T1, float& T2, float& T3 ) const
{
	MatrixToAngles( FAngleSystem, FMatrix.ExtractMatrix3(), T1, T2, T3 );
}

void LTransform::SetPositionAndAngles( const LVector3& Pos, float AngleX, float AngleY, float AngleZ )
{
	FMatrix = LMatrix4::GetFromPitchPanRoll( AngleZ, AngleX, AngleY ) * LMatrix4::GetTranslateMatrix( Pos );
}

void LTransform::SetPositionAndAngles( const LVector3& Pos, const LVector3& Angles )
{
	SetPositionAndAngles( Pos, Angles.x, Angles.y, Angles.z );
}

void LTransform::SetPositionAndOrientation( const LVector3& pos, const LQuaternion& quat )
{
	FMatrix = LMatrix4( quat.ToMatrix3() ) * LMatrix4::GetTranslateMatrix( pos );
}

void LTransform::SetPositionAndAxisAngle( const LVector3& Pos, const LVector3& Axis, float Angle )
{
	FMatrix = LMatrix4::GetRotateMatrixAxis( Angle, Axis ) * LMatrix4::GetTranslateMatrix( Pos );
}

void LTransform::SetPosMatrixAndAxisAngle( const LMatrix4& Pos, const LVector3& Axis, float Angle )
{
	FMatrix = LMatrix4::GetRotateMatrixAxis( Angle, Axis ) * Pos;
}

LTransform::LTransform( const LVector3& pos, const LQuaternion& quat )
{
	SetPositionAndOrientation( pos, quat );
}

LTransform::LTransform( const LMatrix4& mtx4 )
{
	SetMatrix4( mtx4 );
}

void LTransform::GetPositionAndOrientation( LVector3& Pos, LQuaternion& Q ) const
{
	LMatrix4 Mtx;

	DecomposeTransformation( FMatrix, Pos, Mtx );
	Q.FromMatrix3( Mtx.ExtractMatrix3() );
}

void LTransform::LookAt( const LVector3& From, const LVector3& To, const LVector3& Up )
{
	FMatrix = ::Linderdaum::Math::LookAt( From, To, Up );
}

// Lerp + SLerp between O1 and O2 for t in [0,1]
void LTransform::Interpolate( const LTransform& O1, const LTransform& O2, float t )
{
	LMatrix4 Mtx1, Mtx2;
	LQuaternion Q1, Q2;
	LVector3 Pos1, Pos2;

	DecomposeTransformation( O1.FMatrix, Pos1, Mtx1 );
	DecomposeTransformation( O2.FMatrix, Pos2, Mtx2 );
	Q1.FromMatrix3( Mtx1.ExtractMatrix3() );
	Q2.FromMatrix3( Mtx2.ExtractMatrix3() );

	LQuaternion Q;

	Q.SLERP( Q1, Q2, t );

	LVector3 Pos = ( 1 - t ) * Pos1 + t * Pos2;

	//
	SetPositionAndOrientation( Pos, Q );
}

using namespace Linderdaum::Math;

/// indices of the axes fora given coordinate system
int EulerAxes[12][3] =
{
	{0, 2, 0}, {0, 2, 1}, {0, 1, 0}, {0, 1, 2},
	{1, 0, 1}, {1, 0, 2}, {1, 2, 1}, {1, 2, 0},
	{2, 1, 2}, {2, 1, 0}, {2, 0, 2}, {2, 0, 1}
};

LVector3 EuclideanBasis[3] = { vec3( 1, 0, 0 ), vec3( 0, 1, 0 ), vec3( 0, 0, 1 ) };

/**
   Find a planar rotation (c=cos, s=sin) which maps (a,b) vector to (r,0)
*/
void GivensRotation( float a, float b, float& c, float& s, float& r )
{
	/**
	http://www.cgafaq.info/wiki/Euler_angles_from_matrix

	The following method, choosing either t=cot(theta) or t=tan(theta) as route,
	is a variation of that recommended by Golub and Van Loan, made continuous as suggested by Edward Anderson
	*/

	if ( fabs( b ) < Linderdaum::Math::EPSILON )
	{
		c = Sign( a );
		s = 0.0f;
		r = fabs( a );

		return;
	}

	if ( fabs( a ) < Linderdaum::Math::EPSILON )
	{
		c = 0.0f;
		s = Sign( b );
		r = fabs( b );

		return;
	}

	float t, u;

	if ( fabs( b ) > fabs( a ) )
	{
		t = a / b;
		u = Sign( b ) * sqrt( 1.0f + t * t );
		s = 1.0f / u;
		c = s * t;
		r = b * u;
	}
	else
	{
		t = b / a;
		u = Sign( a ) * sqrt( 1.0f + t * t );
		c = 1.0f / u;
		s = c * t;
		r = a * u;
	}
}

void MatrixToAngles( LEulerAngleSystem Sys, const LMatrix3& M, float& T1, float& T2, float& T3 )
{
	int i, j, k, h;
	int neg, alt, rev;

	// convert integer code I to N,A,R and (i,j,k,h) indices
	int S = static_cast<int>( Sys );

	i = S >> 3;

	neg = ( S >> 2 ) & 0x1;
	alt = ( S >> 1 ) & 0x1;
	rev =  S       & 0x1;

	static int EulerNext[4] = {1, 2, 3, 1};

	j = EulerNext[i + neg];
	k = ( 0 + 1 + 2 )/*(x+y+z)*/ - i - j;
	h = EulerNext[k + ( 1 ^ neg ^ alt )];

	// now convert the matrix to angles

	LVector3 v = M[i];

	// [a, b] := [v_h, v_k]
	float a = v[h];
	float b = v[k];

	float c, s;

	GivensRotation( a, b, c, s, v[h] );

	float s1 = c * M[j][k] - s * M[j][h];
	float c1 = c * M[k][k] - s * M[k][h];

	T1 = atan2( s1, c1 );
	T2 = atan2( v[j], v[i] );
	T3 = atan2( s, c );

	if ( alt )
	{
		T3 = -T3;
	}

	if ( neg )
	{
		T1 = -T1;
		T2 = -T2;
		T3 = -T3;
	}

	if ( rev )
	{
		/// [T1,T2,T3] -> [T3,T2,T1]
		float TT = T3;
		T3 = T1;
		T1 = TT;
	}
}

void MakeRotationMatrix( LMatrix4& R, int axis, float angle )
{
	int i1 = 1, i2 = 2;

	if ( axis == 0 )
	{
		i1 = 1;
		i2 = 2;
	}

	if ( axis == 1 )
	{
		i1 = 0;
		i2 = 2;
	}

	if ( axis == 2 )
	{
		i1 = 0;
		i2 = 1;
	}

	float s = sin( angle );
	float c = cos( angle );

	R.IdentityMatrix();

	R[i1][i1] = +c;
	R[i1][i2] = -s;
	R[i2][i1] = +s;
	R[i2][i2] = +c;
}

void AnglesToMatrix( LEulerAngleSystem Sys, LMatrix4& M, float T1, float T2, float T3 )
{
	int S = static_cast<int>( Sys );

	int Idx = ( S > 12 ) ? ( S - 12 ) : S;

	float T[3] = {T1, T2, T3};

	LMatrix4 Mt[3];

	for ( int i = 0 ; i < 3 ; i++ )
	{
		MakeRotationMatrix( Mt[i], EulerAxes[Idx][i], T[i] );
	}

	// reverse or direct
	M = ( S > 12 ) ? ( Mt[2] * Mt[1] * Mt[0] ) : ( Mt[0] * Mt[1] * Mt[2] );
}

/*
 * 01/03/2010
     Log section added
*/

/*
   (Part of Linderdaum Engine)
   Version 0.6.00
   (17/01/2011)
   (C) Kosarevsky Sergey, 2008-2011
   support@linderdaum.com
   http://www.linderdaum.com
*/

namespace Linderdaum
{
	namespace Math
	{

		LProjectionType DetermineProjectionType( const LMatrix4& Projection )
		{
			if ( IsOrthographicProjection( Projection ) )
			{
				return PROJECTION_ORTHOGRAPHIC;
			}

			if ( IsPerspectiveProjection( Projection ) )
			{
				return PROJECTION_PERSPECTIVE;
			}

			return PROJECTION_ERROR;
		}

		bool IsOrthographicProjection( const LMatrix4& M )
		{
			return ( fabs( M[3][3] - 1.0f ) < EPSILON );
		}

		bool IsPerspectiveProjection( const LMatrix4& M )
		{
			return ( fabs( M[3][3] ) < EPSILON );
		}

		LMatrix4 Ortho2D( float L, float R, float B, float T )
		{
			return Ortho( L, R, B, T, -1, 1 );
		}

		LMatrix4 Ortho( float L, float R, float B, float T, float N, float F )
		{
			LMatrix4 Matrix;

			Matrix[0][0] = 2 / ( R - L );
			Matrix[1][0] = 0;
			Matrix[2][0] = 0;
			Matrix[3][0] = -( R + L ) / ( R - L );

			Matrix[0][1] = 0;
			Matrix[1][1] = 2 / ( T - B );
			Matrix[2][1] = 0;
			Matrix[3][1] = -( T + B ) / ( T - B );

			Matrix[0][2] = 0;
			Matrix[1][2] = 0;
			Matrix[2][2] = -2 / ( F - N );
			Matrix[3][2] = -( F + N ) / ( F - N );

			Matrix[0][3] = 0;
			Matrix[1][3] = 0;
			Matrix[2][3] = 0;
			Matrix[3][3] = 1;

			return Matrix;
		}

		LMatrix4 Perspective( float FOV, float Aspect, float NearCP, float FarCP )
		{
			float YMax = NearCP * tan( FOV * ::Linderdaum::Math::DTOR * 0.5f );
			float YMin = -YMax;
			float XMin = YMin * Aspect;
			float XMax = YMax * Aspect;

			return Frustum( XMin, XMax, YMin, YMax, NearCP, FarCP );
		}

		LMatrix4 PerspectiveStereo( float FOV, float Aspect, float NearCP, float FarCP, float IOD, float FocalLength, bool WhichEye )
		{
			float D      = NearCP * tan( FOV * 0.5f );
			float Offset = ( WhichEye ? -0.5f : 0.5f ) * IOD * NearCP / FocalLength;

			float Left   = - Aspect * D;
			float Right  =   Aspect * D;
			float Bottom = - D + Offset;
			float Top    =   D + Offset;

			return Frustum( Bottom, Top, Left, Right, NearCP, FarCP );
		}

		LMatrix4 Frustum( float L, float R, float B, float T, float N, float F )
		{
			LMatrix4 Matrix;

			Matrix[0][0] =  2 * N / ( R - L );
			Matrix[1][0] =  0;
			Matrix[2][0] =  ( R + L ) / ( R - L );
			Matrix[3][0] =  0;

			Matrix[0][1] =  0;
			Matrix[1][1] =  2 * N / ( T - B );
			Matrix[2][1] =  ( T + B ) / ( T - B );
			Matrix[3][1] =  0;

			Matrix[0][2] =  0;
			Matrix[1][2] =  0;
			Matrix[2][2] = -( F + N ) / ( F - N );
			Matrix[3][2] = -2 * F * N / ( F - N );

			Matrix[0][3] =  0;
			Matrix[1][3] =  0;
			Matrix[2][3] = -1;
			Matrix[3][3] =  0;

			return Matrix;
		}

		void OrthoToParams( const LMatrix4& M, float& L, float& R, float& B, float& T, float& N, float& F )
		{
			float m00 = M[0][0];
			float m30 = M[3][0];
			float m11 = M[1][1];
			float m31 = M[3][1];
			float m22 = M[2][2];
			float m32 = M[3][2];

			N = ( 1.0f - m32 ) / m22;
			F = -( m32 + 1.0f ) / m22;
			B = ( 1.0f - m31 ) / m11;
			T = -( m32 + 1.0f ) / m22;
			L = ( 1.0f - m30 ) / m00;
			R = -( m30 + 1.0f ) / m00;
		}

		void FrustumToParams( const LMatrix4& M, float& L, float& R, float& B, float& T, float& N, float& F )
		{
			float m00 = M[0][0];
			float m20 = M[2][0];
			float m11 = M[1][1];
			float m21 = M[2][1];
			float m22 = M[2][2];
			float m32 = M[3][2];

			N = -m32 / ( 1.0f - m22 );
			F = +m32 / ( 1.0f + m22 );

			L = N * ( m20 - 1.0f ) / m00;
			R = N * ( m20 + 1.0f ) / m00;

			B = N * ( m21 - 1.0f ) / m11;
			T = N * ( m21 + 1.0f ) / m11;
		}

		void PerspectiveToParams( const LMatrix4& M, float& FOV, float& Aspect, float& NearCP, float& FarCP )
		{
			float XMin, XMax, YMin, YMax;

			FrustumToParams( M, XMin, XMax, YMin, YMax, NearCP, FarCP );

			FOV = 2.0f * atan( YMax / NearCP ) / ( ::Linderdaum::Math::DTOR );

			if ( fabs( YMin ) < 0.0f )
			{
				Aspect = XMax / YMax;
			}
			else
			{
				Aspect = XMin / YMin;
			}
		}

		LMatrix4 LookAt( LVector3 Eye, LVector3 Center, LVector3 Up )
		{
			LMatrix4 Matrix;

			LVector3 X, Y, Z;
			// make rotation matrix

			// Z vector
			Z = Eye - Center;
			Z.Normalize();

			// Y vector
			Y = Up;

			// X vector = Y cross Z
			X = Y.Cross( Z );

			// Recompute Y = Z cross X
			Y = Z.Cross( X );

			// cross product gives area of parallelogram, which is < 1.0 for
			// non-perpendicular unit-length vectors; so normalize x, y here
			X.Normalize();
			Y.Normalize();

			Matrix[0][0] = X.x;
			Matrix[1][0] = X.y;
			Matrix[2][0] = X.z;
			Matrix[3][0] = -X.Dot( Eye );
			Matrix[0][1] = Y.x;
			Matrix[1][1] = Y.y;
			Matrix[2][1] = Y.z;
			Matrix[3][1] = -Y.Dot( Eye );
			Matrix[0][2] = Z.x;
			Matrix[1][2] = Z.y;
			Matrix[2][2] = Z.z;
			Matrix[3][2] = -Z.Dot( Eye );
			Matrix[0][3] = 0;
			Matrix[1][3] = 0;
			Matrix[2][3] = 0;
			Matrix[3][3] = 1.0f;

			return Matrix;
		}

		LMatrix4 LookAtStereo( LVector3 Eye, LVector3 Center, LVector3 Up, float IOD, bool WhichEye )
		{
			const LVector3 ViewDirection = ( Center - Eye ).GetNormalized();
			const LVector3 Right         = ( ViewDirection.Cross( Up ) ).GetNormalized();

			LVector3 StereoOffset = ( WhichEye ? 0.5f : -0.5f ) * Right * IOD;
			LVector3 NewEye = Eye + StereoOffset;

			return LookAt( NewEye, NewEye + ViewDirection, Up );
		}

		LMatrix4 LookAtStereoMatrix( const LMatrix4& View, float IOD, bool WhichEye )
		{
//			const LVector3 ViewDirection = ( Center - Eye ).GetNormalized();
//			const LVector3 Right         = ( ViewDirection.Cross(Up) ).GetNormalized();
//			LVector3 StereoOffset = ( WhichEye ? 0.5f : -0.5f ) * Right * IOD;
//			LVector3 NewEye = Eye + StereoOffset;
//			return LookAt( NewEye, NewEye + ViewDirection, Up );

			const LVector3 Right         = GetSideDirection( View );

			LVector3 StereoOffset = ( WhichEye ? 0.5f : -0.5f ) * Right * IOD;

			return LMatrix4::GetTranslateMatrix( StereoOffset ) * View;
		}

		LVector3 GetViewingDirection( const LMatrix4& View )
		{
			return LVector3( -View[1][0],
			                 View[0][0],
			                 View[2][0] );
		}

		LVector3 GetSideDirection( const LMatrix4& View )
		{
			return LVector3( -View[1][2],
			                 View[0][2],
			                 View[2][2] );
		}

		LMatrix4 ProjectReflectionTexture( const LMatrix4& Projection, const LMatrix4& ModelView )
		{
			LMatrix4 Scale;
			LMatrix4 Translate = LMatrix4::GetTranslateMatrix( LVector3( 0.5f, 0.5f, 0.5f ) );
			/*
			    glLoadIdentity;
			    glTranslatef(0.5, 0.5, 0);
			    glScalef(0.5, 0.5, 0);
			    // Important: aspect ratio must be that of the main window, not that of the texture.
			    gluPerspective(60, ClientWidth/ClientHeight, 0.01, 100000);
			    glMultMatrixf(@cM);
			    glScalef(1, -1, 1);
			*/
			Scale.ScaleMatrix( LVector3( 0.5f, 0.5f, 0.5f ) );

			LMatrix4 TexProjectionMatrix = ModelView * ( Projection * ( Scale * Translate ) );

			return TexProjectionMatrix;
		}

		LVector3 ProjectPoint( LVector3 Point, const LMatrix4& Projection, const LMatrix4& ModelView )
		{
			LVector4 R( Point, 1.0f );

			// eye coordinates
			R = ModelView * R;
			// clip coordinates
			R = Projection * R;
			// NDC
// R /= R.w;
			// screen coordinates
			R.y = -R.y;
			R.x = ( R.x + 1.0f ) * 0.5f;
			R.y = ( R.y + 1.0f ) * 0.5f;

			return R.ToVector3();
		}

		LVector3 UnProjectPoint( LVector3 Point, const LMatrix4& Projection, const LMatrix4& ModelView )
		{
			LVector4 R( Point, 1.0f );

			R.x = 2.0f * R.x - 1.0f;
			R.y = 2.0f * R.y - 1.0f;
			R.y = -R.y;

			R.z = 1.0f;

			R = Projection.GetInversed() * R;

			R = ModelView.GetInversed() * R;

			return R.ToVector3();
		}

		LVector3 ProjectPointNDC( const LVector3& Point, const LMatrix4& Projection, const LMatrix4& ModelView )
		{
			LVector4 R( Point, 1.0f );

			// eye coordinates
			R = ModelView * R;
			// clip coordinates
			R = Projection * R;

			R /= R.w;

			return R.ToVector3();
		}

		LVector3 UnProjectPointNDC( const LVector3& Point, const LMatrix4& Projection, const LMatrix4& ModelView )
		{
			LVector4 R( Point, 1.0f );

			R = Projection.GetInversed() * R;

			R = ModelView.GetInversed() * R;

			R /= R.w;

			return R.ToVector3();
		}

		LMatrix4 ObliqueReflectionProjection( const LMatrix4& Projection, const LVector4& ClipPlane )
		{
			LMatrix4 Out = Projection;

			LVector3 PlaneNormal = ClipPlane.ToVector3();
			LVector3 SNormal( 0.0f, 0.0f, 0.0f );
			/*
			   if (PlaneNormal.X < 0.0f) SNormal.X = -1.0f;
			   if (PlaneNormal.Y < 0.0f) SNormal.Y = -1.0f;
			   if (PlaneNormal.Z < 0.0f) SNormal.Z = -1.0f;

			   if (PlaneNormal.X > 0.0f) SNormal.X = 1.0f;
			   if (PlaneNormal.Y > 0.0f) SNormal.Y = 1.0f;
			   if (PlaneNormal.Z > 0.0f) SNormal.Z = 1.0f;
			*/
//   LVector4 Q = LVector4( SNormal, 1.0f );
//   Q = Projection.GetInversed() * Q;
			LVector4 Q;

			Q.x = ( fsign( ClipPlane.x ) + Projection.ToFloatPtr()[8] ) / Projection.ToFloatPtr()[0];
			Q.y = ( fsign( ClipPlane.y ) + Projection.ToFloatPtr()[9] ) / Projection.ToFloatPtr()[5];

			Q.z = -1.0f;
			Q.w = ( 1.0f + Projection.ToFloatPtr()[10] ) / Projection.ToFloatPtr()[14];

//   float C = 2.0f / ( PlaneNormal * LVector3(Q.X, Q.Y, Q.Z) );
//   LVector4 CP = LVector4(PlaneNormal, ClipPlane.W + Math::EPSILON ) * C;

			LVector4 CP = ClipPlane * ( 2.0f / ClipPlane.Dot( Q ) );

			Out.ToFloatPtr()[2] = CP.x;
			Out.ToFloatPtr()[6] = CP.y;
			Out.ToFloatPtr()[10] = CP.z + 1.0f;
			Out.ToFloatPtr()[14] = CP.w;

			return Out;
		}

		void TransformRayToCoordinates( const LVector3& P, const LVector3& A, const LMatrix4& Transform, LVector3& TransP, LVector3& TransA )
		{
			// transform P to TransP
			TransP = Transform * P;

			// rotate A to TransA
			TransA = Transform.ExtractMatrix3() * A;
		}

		LMatrix4    GetClosestViewForAABB( const LVector3& MinV, const LVector3& MaxV, const LMatrix4& Proj, const LVector3& Eye, const LVector3& Up )
		{
			( void )Proj;
			LVector3 Center = 0.5f * ( MinV + MaxV );

			return LookAt( Eye, Center, Up );
		}

	} // namespace Math
} // namespace Linderdaum



namespace Linderdaum
{
	namespace Math
	{
		void Randomize( int Seed )
		{
			std::srand( Seed );
		}
		int Random( int L )
		{
			return ( int ) ( ( float ) L * ( float ) std::rand() / ( float ) RAND_MAX );
		}

		float Random( float L )
		{
			return L * ( float ) std::rand() / ( float ) RAND_MAX;
		}

		float Random()
		{
			return ( float ) std::rand() / ( float ) RAND_MAX;
		}

		float RandomInRange( float RMin, float RMax )
		{
			return RMin + Random() * ( RMax - RMin );
		}
		int RandomInRange( int RMin, int RMax )
		{
			return RMin + Random( RMax - RMin );
		}
		LVector3 RandomVector3InRange( const LVector3& RMin,
		                               const LVector3& RMax )
		{
			return LVector3( RandomInRange( RMin.x, RMax.x ),
			                 RandomInRange( RMin.y, RMax.y ),
			                 RandomInRange( RMin.z, RMax.z ) );
		}
		LVector4 RandomVector4InRange( const LVector4& RMin,
		                               const LVector4& RMax )
		{
			return LVector4( RandomInRange( RMin.x, RMax.x ),
			                 RandomInRange( RMin.y, RMax.y ),
			                 RandomInRange( RMin.z, RMax.z ),
			                 RandomInRange( RMin.w, RMax.w ) );
		}
	}
}

using namespace Linderdaum;

/// Normalize 1,2,3 or 4 dimensional vector
inline void NormalizeN( float* f, int n )
{
	int i;

	float Maximum = -1.0f;

	for ( i = 0 ; i < n ; i++ )
	{
		Maximum = Math::LMax( Math::LAbs( f[i] ), Maximum );
	}

	if ( Maximum < Math::EPSILON ) { return; }

	float Magnitude = 0.0f;

	for ( i = 0 ; i < n ; i++ )
	{
		Magnitude += ( f[i] / Maximum ) * ( f[i] / Maximum );
	}

	Magnitude = 1 / ( Maximum * sqrtf( Magnitude ) );

	for ( i = 0 ; i < n ; i++ )
	{
		f[i] *= Magnitude;
	}
}

void LNoise::InitNoise( int nDimensions, unsigned int nSeed )
{
	Math::Randomize( nSeed );

	FDimensions = nDimensions;

	if ( nDimensions > MAX_DIMENSIONS ) { nDimensions = MAX_DIMENSIONS; }

	int i, j;

	for ( i = 0; i < 256; i++ )
	{
		FMap[i] = ( unsigned char )i;

		for ( j = 0; j < FDimensions; j++ )
		{
			FBuffer[i][j] = Math::RandomInRange( -0.5f, 0.5f );
		}

		NormalizeN( FBuffer[i], FDimensions );
	}

	while ( --i )
	{
		j = Math::RandomInRange( 0, 255 );
		unsigned char Tmp = FMap[i];
		FMap[i] = FMap[j];
		FMap[j] = Tmp;
	}
}

float LNoise::Noise( float* f )
{
	int i, n[MAX_DIMENSIONS];  // Indexes to pass to lattice function
	float r[MAX_DIMENSIONS];   // Remainders to pass to lattice function
	float w[MAX_DIMENSIONS];   // Cubic values to pass to interpolation function

	for ( i = 0 ; i < FDimensions ; i++ )
	{
		n[i] = Math::Floor( f[i] );
		r[i] = f[i] - n[i];
		w[i] = Math::Cubic( r[i] );
	}

	float Value, v1, v2, v3, v4, l1, l2;

	switch ( FDimensions )
	{
		case 1:
			Value = Math::Lerp( Lattice( n[0], r[0] ), Lattice( n[0] + 1, r[0] - 1 ), w[0] );
			break;

		case 2:
			v1 = Math::Lerp( Lattice( n[0], r[0], n[1],   r[1] ),   Lattice( n[0] + 1, r[0] - 1, n[1],   r[1] ),   w[0] );
			v2 = Math::Lerp( Lattice( n[0], r[0], n[1] + 1, r[1] - 1 ), Lattice( n[0] + 1, r[0] - 1, n[1] + 1, r[1] - 1 ), w[0] );

			Value = Math::Lerp( v1, v2, w[1] );
			break;

		case 3:

			// fallthrough
		default:
			l1 = Lattice( n[0],   r[0],   n[1],   r[1],   n[2],   r[2] );
			l2 = Lattice( n[0] + 1, r[0] - 1, n[1],   r[1],   n[2],   r[2] );
			v1 = Math::Lerp( l1, l2, w[0] );

			l1 = Lattice( n[0],   r[0],   n[1] + 1, r[1] - 1, n[2],   r[2] );
			l2 = Lattice( n[0] + 1, r[0] - 1, n[1] + 1, r[1] - 1, n[2],   r[2] );
			v2 = Math::Lerp( l1, l2, w[0] );

			l1 = Lattice( n[0],   r[0],   n[1],   r[1],   n[2] + 1, r[2] - 1 );
			l2 = Lattice( n[0] + 1, r[0] - 1, n[1],   r[1],   n[2] + 1, r[2] - 1 );
			v3 = Math::Lerp( l1, l2, w[0] );

			l1 = Lattice( n[0],   r[0],   n[1] + 1, r[1] - 1, n[2] + 1, r[2] - 1 );
			l2 = Lattice( n[0] + 1, r[0] - 1, n[1] + 1, r[1] - 1, n[2] + 1, r[2] - 1 );
			v4 = Math::Lerp( l1, l2, w[0] );
			Value = Math::Lerp( Math::Lerp( v1, v2, w[1] ), Math::Lerp( v3, v4, w[1] ), w[2] );
	}

	return Math::Clamp( Value * 2.0f, -0.99999f, 0.99999f );
}

float LNoise::fBm( float* f, float Octaves )
{
	// Initialize locals
	int i;
	float Value = 0;
	float Temp[MAX_DIMENSIONS];

	for ( i = 0 ; i < FDimensions ; i++ )
	{
		Temp[i] = f[i];
	}

	// Inner loop of spectral construction, where the fractal is built
	for ( i = 0; i < Octaves; i++ )
	{
		Value += Noise( Temp ) * FExponent[i];

		for ( int j = 0 ; j < FDimensions ; j++ )
		{
			Temp[j] *= FLacunarity;
		}
	}

	// Take care of remainder in fOctaves
	Octaves -= ( int )Octaves;

	if ( Octaves > Math::EPSILON )
	{
		Value += Octaves * Noise( Temp ) * FExponent[i];
	}

	return Math::Clamp( Value, -0.99999f, 0.99999f );
}

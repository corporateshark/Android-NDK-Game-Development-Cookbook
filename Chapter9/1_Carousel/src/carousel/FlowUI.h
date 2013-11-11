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

#include "FlowFlinger.h"
#include "Curve.h"

const float ImageDist = 1.5f;
const float QuadSize = 3.5f;
const float OneImageSize = QuadSize + ImageDist;

/// Number of units between sequential control points
const float c_PointStep = 0.2f;
/// Tweaking parameter for the quad points speed. Used internally
const float c_ControlExp = 0.001f;
/// Height of the image (distance between lower and upper curves);  Thickness and slopiness of the curve
const float c_Height = 4.0f, c_Elevation = 2.0f, c_Slope = 0.3f;
/// Symmetric displacement of curve peaks; Exponential falloff and main coefficient (0.01 for Exp is ok)
const float c_PeakOffset = 3.0f, c_FlowExp = 0.01f, c_FlowMult = 4.0f;

class clFlowUI: public iObject
{
public:
	clFlowUI( clPtr<clFlowFlinger> Flinger, int NumQuads )
	{
		FFlinger = Flinger;
		// standart camera transform for the flow
		mtx4 RotationMatrix;
		RotationMatrix.FromPitchPanRoll( 0.0f, -90.0f, 0.0f );
		FView = mtx4::GetTranslateMatrix( -vec3( 0.0f, -13.2f, 1.2f ) ) * RotationMatrix;
		FProjection = Linderdaum::Math::Perspective( 45.0f, 1.33333f, 0.4f, 2000.0f );

		float Y[] = { c_Height, c_Height, 0, 0 };
		float Ofs[] = { -c_PeakOffset, c_PeakOffset, c_PeakOffset, -c_PeakOffset };
		float Coef[] = { c_Slope, -c_Slope, -c_Slope, c_Slope };

		for ( int i = 0 ; i < 4 ; i++ )
		{
			const int c_NumPoints = 100;

			for ( int j = -c_NumPoints / 2 ; j < c_NumPoints / 2 + 1 ; j++ )
			{
				float t = ( float )j * c_PointStep;
				float P = Coef[i] * ( Ofs[i] - t );
				// Simple arctangent multiplied by exp(-x^2)
				float Mult = c_FlowMult * exp( -c_FlowExp * P * P );
				vec3 Pt( -t, Mult * c_Elevation * atan( P ) / M_PI, Y[i] );
				FBaseCurve[i].AddControlPoint( t * exp( c_ControlExp * t * t ), Pt );
			}
		}

		FNumImg = NumQuads;
		/// Update limits using the current number of elements
		FFlinger->FMinValue = 0.0f;
		FFlinger->FMaxValue = OneImageSize * ( ( float )FNumImg - 1.0f );
	}
	virtual ~clFlowUI() {}

	/// Camera matrices
	mtx4 FView, FProjection;

	/// Number of assigned images
	int FNumImg;

	/// Which image is currently selected
	int GetCurrentImage() const { return ( int ) ceilf( FFlinger->FValue / OneImageSize ); }

	/// Fling handler
	clPtr<clFlowFlinger> FFlinger;

	/// Calculate coordinates for the Quad in a given position
	virtual void QuadCoords( vec3* Pts, float t_center ) const
	{
		float Ofs[] = { QuadSize, -QuadSize, -QuadSize, QuadSize };

		for ( int i = 0 ; i < 4 ; i++ )
		{
			Pts[i] = FBaseCurve[i].GetPosition( t_center - Ofs[i] / 2 );
		}
	}

	/// Trajectory control points for each base curve
	Curve FBaseCurve[4];
};

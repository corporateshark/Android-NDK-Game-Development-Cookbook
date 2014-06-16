/*
 * Copyright (C) 2013 Sergey Kosarevsky (sk@linderdaum.com)
 * Copyright (C) 2013 Viktor Latypov (vl@linderdaum.com)
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

#include "Engine.h"
#include "Wrapper_Callbacks.h"
#include "FileSystem.h"
#include "Rendering.h"
#include "VecMath.h"
#include "Gestures.h"
#include <stdio.h>

#include "FlowUI.h"
#include "FlowFlinger.h"

void BoxR( vec3* P, int color );

void RenderDirect( clPtr<clFlowUI> Control )
{
	int Num = Control->FNumImg;
	int CurImg = Control->GetCurrentImage();
	float Dist = ( float )( Num * OneImageSize );

	if ( Num < 1 ) { return; }

	// index = [curr - 2 .. curr + 2]
	/// Left -> Right -> Selected rendering order
	int ImgOrder[] = { CurImg - 3, CurImg - 2, CurImg - 1, CurImg + 3, CurImg + 2, CurImg + 1, CurImg };

	for ( int in_i = 0 ; in_i < 7 ; in_i++ )
	{
		int i = ImgOrder[in_i];

		if ( i < 0 ) { i += ( 1 - ( ( int )( i / Num ) ) ) * Num; }

		if ( i >= Num ) { i -= ( ( int )( i / Num ) ) * Num; }

		if ( i < Num && i > -1 )
		{
			vec3 Pt[4];
			Control->QuadCoords( Pt, Control->FFlinger->FValue - ( float )( i ) * OneImageSize );

			vec3 Q[4];

			for ( int j = 0 ; j < 4 ; j++ )
			{
				Q[j] = Control->FProjection * Control->FView * Pt[j];
			}

			BoxR( Q, 0xFFFFFF );
		}
	}
}

std::string g_ExternalStorage;

#include <time.h>

clPtr<clFlowUI> g_Flow;

vec2 g_MousePos;
double g_MouseTime;

vec2 Env_GetMouse()
{
	return g_MousePos;
}

double Env_GetMouseTime() { return g_MouseTime; }

void Env_Sleep( int Milliseconds )
{
#if defined _WIN32
	Sleep( Milliseconds );
#else
	// mu-sleep supports microsecond-precision
	usleep( static_cast<useconds_t>( Milliseconds ) * 1000 );
#endif
}

double Env_GetSeconds()
{
	return ( double )clock() / CLOCKS_PER_SEC;
}

clPtr<FileSystem> g_FS;

void OnStart( const std::string& RootPath )
{
	g_FrameBuffer = ( unsigned char* )malloc( ImageWidth * ImageHeight * 4 );
	memset( g_FrameBuffer, 0x00, ImageWidth * ImageHeight * 4 );

	g_FS = new FileSystem();
	g_FS->Mount( "." );
#if defined(ANDROID)
	g_FS->Mount( RootPath );
	g_FS->AddAliasMountPoint( RootPath, "assets" );
#endif

	g_Flow = new clFlowUI( new clFlowFlinger(), 10 );

	XScale = 25.0f;
	YScale = 5.5f;
	XOfs =
	   YOfs = 80.0f;
}

void GestureHandler_SendMotion( int ContactID, eMotionFlag Flag, LVector2 Pos, bool Pressed )
{
	if ( ContactID > -1 )
	{
		g_MousePos = Pos;
		g_MouseTime = Env_GetSeconds();

		if ( Flag == L_MOTION_DOWN )
		{
			g_Flow->FFlinger->OnTouch( true );
		}

		if ( Flag == L_MOTION_UP )
		{
			g_Flow->FFlinger->OnTouch( false );
		}
	}
}

void LineR( double x, double y, double x1, double y1, int color )
{
	LineW( x, y, x1, y1, color );
}

void BoxR( vec3* P, int color )
{
	LineR( P[0].x, P[0].z, P[1].x, P[1].z, color );
	LineR( P[0].x, P[0].z, P[2].x, P[2].z, color );
	LineR( P[2].x, P[2].z, P[1].x, P[1].z, color );
	LineR( P[3].x, P[3].z, P[1].x, P[1].z, color );
	LineR( P[3].x, P[3].z, P[2].x, P[2].z, color );
	LineR( P[0].x, P[0].z, P[3].x, P[3].z, color );
}

void OnDrawFrame()
{
	memset( g_FrameBuffer, 0x00, ImageWidth * ImageHeight * 4 );
	RenderDirect( g_Flow );
}

void OnTimer( float Delta ) { g_Flow->FFlinger->Update( Delta ); }

void OnKeyUp( int code ) {}
void OnKeyDown( int code ) {}
void OnMouseDown( int btn, int x, int y ) {}
void OnMouseMove( int x, int y ) {}
void OnMouseUp( int btn, int x, int y ) {}

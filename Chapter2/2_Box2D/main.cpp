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

#include <stdlib.h>

#include "Wrapper_Callbacks.h"

#if defined( ANDROID )
#  include "Wrapper_Android.h"
#else
#  include "Wrapper_Windows.h"
#endif

#include "Rendering.h"

#include "BoxLite.h"

using namespace Box2D;
const float TIME_STEP = 0.016666f;

World* Wld = NULL;
float LocalTime = 0;

#include "BoxSample.h"

static double NewTime, OldTime, ExecutionTime;

void StartTiming();
double GetSeconds();
void GenerateTicks();

void OnStart()
{
	XScale = YScale = 15.0;
	XOfs = YOfs = 0;

	g_FrameBuffer = ( unsigned char* )malloc( ImageWidth * ImageHeight * 4 );
	memset( g_FrameBuffer, 0xFF, ImageWidth * ImageHeight * 4 );

	StartTiming();

	OldTime = GetSeconds();
	NewTime = OldTime;

	ExecutionTime = 0;

	Wld = new World( Vec2( 0, 0 ), 10 );
	setup3( Wld );
}

void DrawLine2( const Vec2& v1, const Vec2& v2 )
{
	LineW( v1.x, v1.y, v2.x, v2.y, 0 ); // 0xFFFFFF);
}

void DrawBody( Body* body )
{
	Mat22 R( body->rotation );
	Vec2 x = body->position, h = 0.5f * body->width;

	Vec2 v[] = { ( x + R * Vec2( -h.x, -h.y ) ), ( x + R * Vec2( h.x, -h.y ) ), ( x + R * Vec2( h.x,  h.y ) ), ( x + R * Vec2( -h.x,  h.y ) ) };

	for ( int i = 0 ; i < 4 ; i++ ) { DrawLine2( v[i], v[( i + 1 ) % 4] ); };
}

void DrawJoint( Joint* joint )
{
	Body* b1 = joint->body1;
	Body* b2 = joint->body2;

	Mat22 R1( b1->rotation );
	Mat22 R2( b2->rotation );

	Vec2 x1 = b1->position;
	Vec2 p1 = x1 + R1 * joint->localAnchor1;

	Vec2 x2 = b2->position;
	Vec2 p2 = x2 + R2 * joint->localAnchor2;

	DrawLine2( x1, p1 );
	DrawLine2( p1, x2 );
	DrawLine2( x2, p2 );
	DrawLine2( p2, x1 );
}

void OnDrawFrame()
{
	// render physics world
	Clear( 0xFFFFFF );

	for ( std::vector<Body*>::iterator b = Wld->bodies.begin() ; b != Wld->bodies.end() ; b++ )
	{
		DrawBody( *b );
	}

	for ( std::vector<Joint*>::iterator j = Wld->joints.begin() ; j != Wld->joints.end() ; j++ )
	{
		DrawJoint( *j );
	}

	// update as fast as possible
	GenerateTicks();
}

void OnTimer( float Delta )
{
	Wld->Step( Delta );
}

void GenerateTicks()
{
	// update time
	NewTime = GetSeconds();
	float DeltaSeconds = static_cast<float>( NewTime - OldTime );
	OldTime = NewTime;

	const float TIME_QUANTUM = 0.0166666f;
	const float MAX_EXECUTION_TIME = 10.0f * TIME_QUANTUM;

	ExecutionTime += DeltaSeconds;

	/// execution limit
	if ( ExecutionTime > MAX_EXECUTION_TIME ) { ExecutionTime = MAX_EXECUTION_TIME; }

	/// Generate internal time quanta
	while ( ExecutionTime > TIME_QUANTUM )
	{
		ExecutionTime -= TIME_QUANTUM;
		OnTimer( TIME_QUANTUM );
	}
}


void OnKeyUp( int code )
{
}

void OnKeyDown( int code )
{
}

void OnMouseDown( int btn, int x, int y )
{
}

void OnMouseMove( int x, int y )
{
}

void OnMouseUp( int btn, int x, int y )
{
}

#ifndef ANDROID
#include <windows.h>
#endif

#ifndef _WIN32
#include <unistd.h>
#include <sys/time.h>
#include <sys/select.h>
#include <pthread.h>

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

const unsigned usec_per_sec = 1000000;

bool QueryPerformanceFrequency( int64_t* frequency )
{
	/* gettimeofday reports to microsecond accuracy. */
	*frequency = usec_per_sec;
	return true;
}

bool QueryPerformanceCounter( int64_t* performance_count )
{
	struct timeval Time;

	gettimeofday( &Time, NULL );
	*performance_count = Time.tv_usec + Time.tv_sec * usec_per_sec;

	return true;
}
#endif

static double RecipCyclesPerSecond = 1.0f;

void StartTiming()
{
	/// Initialize timing
#ifndef ANDROID
	LARGE_INTEGER Freq;
#else
	int64_t Freq;
#endif

	QueryPerformanceFrequency( &Freq );

#ifndef ANDROID
	double CyclesPerSecond = static_cast<double>( Freq.QuadPart );
#else
	double CyclesPerSecond = static_cast<double>( Freq );
#endif

	RecipCyclesPerSecond = 1.0 / CyclesPerSecond;
}

double GetSeconds()
{
#ifndef ANDROID
	LARGE_INTEGER T1;
#else
	int64_t T1;
#endif

	QueryPerformanceCounter( &T1 );

#ifndef ANDROID
	return RecipCyclesPerSecond * ( double )( T1.QuadPart );
#else
	return RecipCyclesPerSecond * ( double )( T1 );
#endif
}

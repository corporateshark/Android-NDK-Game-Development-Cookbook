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
#include "Bitmap.h"
#include "Rendering.h"
#include "FI_Utils.h"
#include "ScreenJoystick.h"
#include <stdio.h>

std::string g_ExternalStorage;

#include <time.h>

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

clPtr<clBitmap> g_Bmp;
clPtr<clBitmap> g_Img;
clPtr<clBitmap> g_MaskBmp;

vec2 g_pos;
float g_angle;

float g_rotation;
float g_speed;
float g_accel;

vec2 g_max, g_min;

ScreenJoystick g_Joystick;

const int ID_BUTTON_THRUST = 0;

void OnStart( const std::string& RootPath )
{
	/** */
	// Init joystick:
	float A_Y = 414.0f / 512.0f;

	sBitmapAxis B_Left;
	B_Left.FAxis1 = 0;
	B_Left.FAxis2 = 1;
	B_Left.FPosition = vec2( 55.0f / 512.f, A_Y );
	B_Left.FRadius = 40.0f / 512.0f;
	B_Left.FColour = vec4( 0.75f, 0.75f, 0.75f, 0.0f );

	sBitmapButton B_Fire;
	B_Fire.FIndex = ID_BUTTON_THRUST;
	B_Fire.FColour = vec4( 0, 0, 0, 0 );

	g_Joystick.FAxisDesc.push_back( B_Left );
	g_Joystick.FButtonDesc.push_back( B_Fire );

	g_Joystick.InitKeys();
	g_Joystick.Restart();
	/** */

	g_FrameBuffer = ( unsigned char* )malloc( ImageWidth * ImageHeight * 4 );
	memset( g_FrameBuffer, 0xFF, ImageWidth * ImageHeight * 4 );

	g_FS = new FileSystem();
	g_FS->Mount( "." );
#if defined(ANDROID)
	g_FS->Mount( RootPath );
	g_FS->AddAliasMountPoint( RootPath, "assets" );
#endif

	g_angle = 0.0f;

	g_accel = g_speed = 0.0f;
	g_rotation = 0.0f;

	g_pos = vec2( 0.0f, 0.0f );

	g_max = vec2( 10.0f, 10.0f );
	g_min = -g_max;

	XScale = 15.0f;
	YScale = 15.0f;
	XOfs = YOfs = 0.0f;

	g_Bmp = clBitmap::LoadImg( g_FS->CreateReader( "pad24.png" ), false );

	g_Img = new clBitmap();
	sBitmapParams BMPRec = sBitmapParams( 512, 512, L_BITMAP_BGRA8 );
	g_Img->ReallocImageData( &BMPRec );

	g_MaskBmp = clBitmap::LoadImg( g_FS->CreateReader( "pad_mask.png" ), true );

	for ( int i = 0 ; i < 512 * 512 ; i++ )
	{
		g_Img->FBitmapData[i * 4 + 0] = g_Bmp->FBitmapData[i * 3 + 0];
		g_Img->FBitmapData[i * 4 + 1] = g_Bmp->FBitmapData[i * 3 + 1];
		g_Img->FBitmapData[i * 4 + 2] = g_Bmp->FBitmapData[i * 3 + 2];
	}

	g_Joystick.FMaskBitmap = g_MaskBmp->FBitmapData;
}

void GestureHandler_SendMotion( int ContactID, eMotionFlag Flag, LVector2 Pos, bool Pressed )
{
	g_Joystick.HandleTouch( ContactID, Pos, Pressed, Flag );

	// check joystick controls
	g_rotation = g_Joystick.GetAxisValue( 0 );
	g_accel = g_Joystick.IsPressed( ID_BUTTON_THRUST ) ? 100.0f : 0.0f;
}

void OnDrawFrame()
{
	memcpy( g_FrameBuffer, g_Img->FBitmapData, 512 * 512 * 4 );

	// render the controlled unit
	vec2 dir( cosf( g_angle ), sinf( g_angle ) );

	vec2 pt1 = g_pos;
	vec2 pt2 = g_pos + dir * 3.0f;
	vec2 n( dir.y, -dir.x );
	vec2 pt3 = g_pos + n * 3.0f;
	vec2 pt4 = g_pos - n * 3.0f;

	LineW( pt1.x, pt1.y, pt2.x, pt2.y, 0 );
	LineW( pt2.x, pt2.y, pt3.x, pt3.y, 0 );
	LineW( pt2.x, pt2.y, pt4.x, pt4.y, 0 );
	LineW( pt3.x, pt3.y, pt4.x, pt4.y, 0 );
}

void OnTimer( float Delta )
{
	g_rotation = g_Joystick.GetAxisValue( 0 );

	g_angle += g_rotation * Delta;
	float dd = g_speed * Delta;
	vec2 dir( cosf( g_angle ), sinf( g_angle ) );
	g_pos += dd * dir;

	if ( g_accel > 0.0f ) { g_speed += g_accel * Delta; }
	else { g_speed *= 0.5f; }

	if ( g_speed > 10.0f ) { g_speed = 10.0f; }

	if ( g_pos.x > g_max.x ) { g_pos.x = g_max.x; }

	if ( g_pos.y > g_max.y ) { g_pos.y = g_max.y; }

	if ( g_pos.x < g_min.x ) { g_pos.x = g_min.x; }

	if ( g_pos.y < g_min.y ) { g_pos.y = g_min.y; }
}

void OnKeyUp( int code ) {}
void OnKeyDown( int code ) {}
void OnMouseDown( int btn, int x, int y ) {}
void OnMouseMove( int x, int y ) {}
void OnMouseUp( int btn, int x, int y ) {}

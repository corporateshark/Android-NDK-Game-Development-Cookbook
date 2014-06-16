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

#include "Engine.h"

#include <stdio.h>

#include "FlowUI.h"
#include "FlowFlinger.h"
#include "CurlWrap.h"
#include "Globals.h"

#include "ImageLoadTask.h"

#include "GalleryTable.h"

clPtr<clGallery> g_Gallery;

clAudioThread g_Audio;
clPtr<clCanvas> Canvas;

clPtr<clWorkerThread> g_Loader;

sLGLAPI* LGL3 = NULL;

clPtr<clGLTexture> Texture;

clPtr<clFlowUI> g_Flow;

clPtr<clFileSystem> g_FS;

void RenderDirect( clPtr<clFlowUI> Control )
{
	LGL3->glClearColor( 0.3f, 0.0f, 0.0f, 0.0f );
	LGL3->glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

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

			clPtr<sImageDescriptor> Img = g_Gallery->GetImage( i );
			clPtr<clGLTexture> Txt = ( !Img.GetInternalPtr() ) ? Texture : Img->FTexture;
			Canvas->Rect3D( Control->FProjection, Control->FView, Pt[1], Pt[0], Pt[3], Pt[2], Txt, NULL );
		}
	}
}

#include <time.h>


vec2   g_MousePos;
double g_MouseTime = 0.0;

vec2 Env_GetMouse()
{
	return g_MousePos;
}

double Env_GetMouseTime()
{
	return g_MouseTime;
}

double Env_GetSeconds()
{
	return ( double )clock() / CLOCKS_PER_SEC;
}

class clResponder: public iGestureResponder
{
	virtual void Event_PointerChanged( int PointerID, const LVector2& Pnt, bool Pressed )
	{
		g_MousePos = Pnt;
		g_MouseTime = Env_GetSeconds();

		if ( Pressed )
		{
			g_Flow->FFlinger->OnTouch( true );
		}
		else
		{
			g_Flow->FFlinger->OnTouch( false );
		}
	}

	virtual void Event_PointerMoved( int PointerID, const LVector2& Pnt )
	{
		g_MousePos = Pnt;
		g_MouseTime = Env_GetSeconds();
	}
} Responder;

clPtr<clGLTexture> LoadTexture( const std::string& FileName )
{
	clPtr<clBitmap> Bmp = clBitmap::LoadImg( g_FS->CreateReader( FileName ) );
	clPtr<clGLTexture> Texture = new clGLTexture();
	Texture->LoadFromBitmap( Bmp );
	return Texture;
}

void OnStart( const std::string& RootPath )
{
	g_FS = new clFileSystem();
	g_FS->Mount( "." );
#if defined(ANDROID)
	g_FS->Mount( RootPath );
	g_FS->AddAliasMountPoint( RootPath, "assets" );
#endif

	Canvas = new clCanvas();

	Texture = LoadTexture( "NoImageAvailable.png" );

	g_Flow = new clFlowUI( new clFlowFlinger(), 15 );

	g_Responder = &Responder;

	g_Loader = new clWorkerThread();
	g_Loader->Start( iThread::Priority_Normal );

	// Initialize the network and events queue
	Curl_Load();

	g_Events = new iAsyncQueue();

	g_Downloader = new clDownloader();
	g_Downloader->FEventQueue = g_Events.GetInternalPtr();

	// Start the Picasa list download
	g_Gallery = new clGallery();
	g_Gallery->StartListDownload();
}

void OnDrawFrame()
{
	RenderDirect( g_Flow );
}

void OnTimer( float Delta )
{
	// dispatch events here
	g_Events->DemultiplexEvents();

	g_Flow->FFlinger->Update( Delta );
}

void OnKey( int code, bool pressed )
{
}

void OnMouseDown( int btn, const LVector2& Pos )
{
	g_MousePos = Pos;
	g_MouseTime = Env_GetSeconds();

	g_Flow->FFlinger->OnTouch( true );
}

void OnMouseMove( const LVector2& Pos )
{
	g_MousePos = Pos;
	g_MouseTime = Env_GetSeconds();
}

void OnMouseUp( int btn, const LVector2& Pos )
{
	g_MousePos = Pos;
	g_MouseTime = Env_GetSeconds();

	g_Flow->FFlinger->OnTouch( false );
}

void OnStop()
{
}

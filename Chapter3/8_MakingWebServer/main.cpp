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

#include "Rendering.h"

#if defined( ANDROID )
#  include "Wrapper_Android.h"
#else
#  include "Wrapper_Windows.h"
#endif

#include "HTTP.h"

#include "DetectAdapters.h"

#include "bmp.h"
#include "FontData.h"

extern int fw, fh;
extern unsigned char* font;
extern int char_w, char_h;

extern volatile bool g_ServerStarted;
extern volatile bool g_StartedThread;
extern volatile bool g_GotSocket;
extern volatile bool g_RunningThread;

std::vector<sAdapterInfo> Adapters;

void OnStart()
{
	InitializeNetwork();

	g_ServerStarted = false;
	g_StartedThread = false;
	g_GotSocket     = false;
	g_RunningThread = false;

	g_FrameBuffer = ( unsigned char* )malloc( ImageWidth * ImageHeight * 4 );
	memset( g_FrameBuffer, 0x0, ImageWidth * ImageHeight * 4 );

	// 1. initialize font
	font = read_bmp_mem( font_data, &fw, &fh );

	char_w = fw / 16;
	char_h = fh / 16;

	// 2. detect adapter
	Net_EnumerateAdapters( Adapters );

	// 3. start webserver
	clHTTPServerThread* th = new clHTTPServerThread();

	if ( !Adapters.empty() )
	{
		th->FBindAddress = std::string( Adapters[0].FIP );
	}

	th->Start( iThread::Priority_Normal );
}

void OnDrawFrame()
{
	char Txt[1024];

	for ( size_t i = 0 ; i < Adapters.size() ; i++ )
	{
		sprintf( Txt, "%s, Name == %s", Adapters[i].FIP, Adapters[i].FName );
		RenderString( Txt, 10, ( int )i * 20, 0xFFFFFF );
	}

	if ( g_ServerStarted ) { RenderString( "Started server", 10, 200, 0xFFFFFF ); }

	if ( g_StartedThread ) { RenderString( "Started thread", 10, 220, 0xFFFFFF ); }

	if ( g_GotSocket ) { RenderString( "Got socket", 10, 240, 0xFFFFFF ); }

	if ( g_RunningThread ) { RenderString( "Running thread", 10, 260, 0xFFFFFF ); }
}

void OnTimer( float Delta )
{
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

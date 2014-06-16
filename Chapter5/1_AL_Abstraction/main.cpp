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

#include "Wrapper_Callbacks.h"
#include "FileSystem.h"
#include "Audio.h"
#include "WAV.h"
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

clPtr<Blob> LoadFileAsBlob( const std::string& FName )
{
	clPtr<iIStream> input = g_FS->CreateReader( FName );
	clPtr<Blob> Res = new Blob();
	Res->CopyMemoryBlock( input->MapStream(), input->GetSize() );
	return Res;
}

AudioThread g_Audio;

class SoundThread: public iThread
{
	virtual void Run()
	{
		while ( !g_Audio.FInitialized ) {}

		clPtr<AudioSource> Src = new AudioSource();

		Src->BindWaveform( new WavProvider( LoadFileAsBlob( "test.wav" ) ) );

		Src->Play();

		while ( Src->IsPlaying() ) {}

		Src = NULL;

		g_Audio.Exit( true );

		exit( 0 );
	}
};

SoundThread g_Sound;

void OnStart( const std::string& RootPath )
{
	g_FrameBuffer = ( unsigned char* )malloc( ImageWidth * ImageHeight * 4 );
	memset( g_FrameBuffer, 0xFF, ImageWidth * ImageHeight * 4 );

	g_FS = new FileSystem();
	g_FS->Mount( "." );
#if defined(ANDROID)
	g_FS->Mount( RootPath );
	g_FS->AddAliasMountPoint( RootPath, "assets" );
#endif
	g_Audio.Start( iThread::Priority_Normal );
	g_Sound.Start( iThread::Priority_Normal );
}

void OnDrawFrame() {}
void OnTimer( float Delta ) {}
void OnKeyUp( int code ) {}
void OnKeyDown( int code ) {}
void OnMouseDown( int btn, int x, int y ) {}
void OnMouseMove( int x, int y ) {}
void OnMouseUp( int btn, int x, int y ) {}

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
#include "Thread.h"
#include "LAL.h"
#include <stdio.h>

std::string g_ExternalStorage;

#pragma pack(push, 1)
struct __attribute__( ( packed, aligned( 1 ) ) ) sWAVHeader
{
   unsigned char    RIFF[4];
   unsigned int     Size;
   unsigned char    WAVE[4];
   unsigned char    FMT[4];
   unsigned int     SizeFmt;
   unsigned short   FormatTag;
   unsigned short   Channels;
   unsigned int     SampleRate;
   unsigned int     AvgBytesPerSec;
   unsigned short   nBlockAlign;
   unsigned short   nBitsperSample;
   unsigned char    Reserved[4];
   unsigned int     DataSize;
};
#pragma pack(pop)

clPtr<FileSystem> g_FS;

class SoundThread: public iThread
{
	ALuint FSourceID, FBufferID;

	ALCdevice*  FDevice;
	ALCcontext* FContext;

	bool IsPlaying()
	{
		int State;
		alGetSourcei( FSourceID, AL_SOURCE_STATE, &State );
		return State == AL_PLAYING;
	}

	void PlayBuffer( const unsigned char* Data, int DataSize )
	{
		alGenBuffers( 1, &FBufferID );
		alBufferData( FBufferID, AL_FORMAT_MONO16, Data, DataSize, 22050 );
		alSourcei( FSourceID, AL_BUFFER, FBufferID );
		alSourcePlay( FSourceID );
	}

	virtual void Run()
	{
		LoadAL();

		// We should use actual device name if the default does not work
		FDevice = alcOpenDevice( NULL );
		FContext = alcCreateContext( FDevice, NULL );
		alcMakeContextCurrent( FContext );

		// create the source
		alGenSources( 1, &FSourceID );
		alSourcef( FSourceID, AL_GAIN,    1.0 );

		// load data
		clPtr<iIStream> Sound = g_FS->CreateReader( "test.wav" );

		int DataSize = ( int )Sound->GetSize();
		const ubyte* Data = Sound->MapStream();

		// play
		PlayBuffer( Data + sizeof( sWAVHeader ), DataSize - sizeof( sWAVHeader ) );

		while ( IsPlaying() ) {}

		alSourceStop( FSourceID );

		// destroy the source
		alDeleteSources( 1, &FSourceID );
		alDeleteBuffers( 1, &FBufferID );

		// Shutdown
		alcDestroyContext( FContext );
		alcCloseDevice( FDevice );

		UnloadAL();

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
	g_Sound.Start( iThread::Priority_Normal );
}

void OnDrawFrame() {}
void OnTimer( float Delta ) {}
void OnKeyUp( int code ) {}
void OnKeyDown( int code ) {}
void OnMouseDown( int btn, int x, int y ) {}
void OnMouseMove( int x, int y ) {}
void OnMouseUp( int btn, int x, int y ) {}

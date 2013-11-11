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

#include "LAL.h"
#include "Thread.h"
#include <vector>

double Env_GetSeconds();
void Env_Sleep( int Milliseconds );

/// Provider of waveform data for streaming
class iWaveDataProvider: public iObject
{
public:
	iWaveDataProvider(): FChannels( 0 ),
		FSamplesPerSec( 0 ),
		FBitsPerSample( 0 ) {}

	virtual ubyte*                   GetWaveData() = 0;
	virtual size_t                   GetWaveDataSize() const = 0;

	/// Format of waveform data
	ALuint GetALFormat() const
	{
		if ( FBitsPerSample == 8 ) { return ( FChannels == 2 ) ? AL_FORMAT_STEREO8  : AL_FORMAT_MONO8;  }

		if ( FBitsPerSample == 16 ) { return ( FChannels == 2 ) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16; }

		return AL_FORMAT_MONO8;
	}

	int    FChannels;
	int    FSamplesPerSec;
	int    FBitsPerSample;
};

/// Audio source interface, also directly used in silent mode
class AudioSource: public iObject
{
public:
	void Play()
	{
		if ( IsPlaying() ) { return; }

		if ( !FWaveDataProvider ) { return; }

		int State;
		alGetSourcei( FSourceID, AL_SOURCE_STATE, &State );
		alSourcePlay( FSourceID );
	}

	void Stop()
	{
		alSourceStop( FSourceID );
	}

	void Pause()
	{
		alSourcePause( FSourceID );
		UnqueueAll();
	}

	void LoopSound( bool Loop )
	{
		alSourcei( FSourceID, AL_LOOPING, Loop ? 1 : 0 );
	}

	bool IsPlaying() const
	{
		int State;
		alGetSourcei( FSourceID, AL_SOURCE_STATE, &State );
		return State == AL_PLAYING;
	}

	AudioSource(): FWaveDataProvider( NULL )
	{
		alGenSources( 1, &FSourceID );

		alSourcef( FSourceID, AL_GAIN,    1.0 );
		alSourcei( FSourceID, AL_LOOPING, 0   );
	}

	virtual ~AudioSource()
	{
		Stop();
		FWaveDataProvider = NULL;

		alDeleteSources( 1, &FSourceID );
		alDeleteBuffers( 1, &FBufferID );
	}

	void SetVolume( float Volume )
	{
		alSourcef( FSourceID, AL_GAIN, Volume );
	}

	void BindWaveform( clPtr<iWaveDataProvider> Wave )
	{
		FWaveDataProvider = Wave;

		if ( !Wave ) { return; }

		alGenBuffers( 1, &FBufferID );
		alBufferData( FBufferID,
		              Wave->GetALFormat(),
		              Wave->GetWaveData(),
		              ( int )Wave->GetWaveDataSize(),
		              Wave->FSamplesPerSec );

		alSourcei( FSourceID, AL_BUFFER, FBufferID );
	}

private:
	void   UnqueueAll()
	{
		int Queued;
		alGetSourcei( FSourceID, AL_BUFFERS_QUEUED, &Queued );

		if ( Queued > 0 )
		{
			alSourceUnqueueBuffers( FSourceID, Queued, &FBufferID );
		}
	}

	clPtr<iWaveDataProvider> FWaveDataProvider;
private:
	unsigned int FSourceID;
	unsigned int FBufferID;
};

/// Manages OpenAL in a separate thread
class AudioThread: public iThread
{
public:
	AudioThread(): FDevice( NULL ), FContext( NULL ), FInitialized( false ) {}
	virtual ~AudioThread() {}

	virtual void Run()
	{
		if ( !LoadAL() ) { return; }

		// We should use actual device name if the default does not work
		FDevice = alcOpenDevice( NULL );

		FContext = alcCreateContext( FDevice, NULL );

		alcMakeContextCurrent( FContext );

		FInitialized = true;

		FPendingExit = false;

		while ( !IsPendingExit() ) { Env_Sleep( 100 ); }

		alcDestroyContext( FContext );
		alcCloseDevice( FDevice );

		UnloadAL();
	}

	bool FInitialized;
private:
	ALCdevice*     FDevice;
	ALCcontext*    FContext;
};

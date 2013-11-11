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

#include "Engine.h"

#include "LAL.h"
#include "Thread.h"
#include "Mutex.h"

#include <vector>
#include <algorithm>

double GetSeconds();
void Env_Sleep( int Milliseconds );

const int BUFFER_SIZE = 2 * 262144;

class iWaveDataProvider;

/// Audio source interface, also directly used in silent mode
class clAudioSource: public iObject
{
public:
	clAudioSource();
	virtual ~clAudioSource();

	void Play();
	void Stop()
	{
		alSourceStop( FSourceID );
	}

	void Pause()
	{
		alSourcePause( FSourceID );
		UnqueueAll();
	}

	void LoopSound( bool Loop );

	bool IsPlaying() const
	{
		int State;
		alGetSourcei( FSourceID, AL_SOURCE_STATE, &State );
		return State == AL_PLAYING;
	}

	int StreamBuffer( unsigned int BufferID, int Size );
	void Update( float DeltaSeconds );

	void SetVolume( float Volume )
	{
		alSourcef( FSourceID, AL_GAIN, Volume );
	}

	void BindWaveform( clPtr<iWaveDataProvider> Wave );

private:
	void   UnqueueAll()
	{
		int Queued;
		alGetSourcei( FSourceID, AL_BUFFERS_QUEUED, &Queued );

		if ( Queued > 0 )
		{
			alSourceUnqueueBuffers( FSourceID, Queued, &FBufferID[0] );
		}
	}

	clPtr<iWaveDataProvider> FWaveDataProvider;
private:
	unsigned int FSourceID;
	unsigned int FBufferID[2];
	int      FBuffersCount;
	bool     FLooping;
};

/// Manages OpenAL in a separate thread
class clAudioThread: public iThread
{
public:
	clAudioThread(): FDevice( NULL ), FContext( NULL ), FInitialized( false ) {}
	virtual ~clAudioThread() {}

	virtual void Run();

	void RegisterSource( clAudioSource* Src );
	void UnRegisterSource( clAudioSource* Src );

	void Wait() const volatile;
private:
	volatile bool  FInitialized;
	ALCdevice*     FDevice;
	ALCcontext*    FContext;
	std::vector< clAudioSource* > FActiveSources;
	clMutex        FMutex;
};

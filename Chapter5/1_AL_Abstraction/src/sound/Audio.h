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

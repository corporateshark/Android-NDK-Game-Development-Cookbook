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

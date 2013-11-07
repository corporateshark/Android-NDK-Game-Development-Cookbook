#include "Audio.h"

extern clAudioThread g_Audio;

clAudioSource::clAudioSource()
	: FWaveDataProvider( NULL )
	, FBuffersCount( 0 )
	, FLooping( false )
{
	alGenSources( 1, &FSourceID );

	alSourcef( FSourceID, AL_GAIN,    1.0 );
	alSourcei( FSourceID, AL_LOOPING, 0   );

	g_Audio.RegisterSource( this );
}

clAudioSource::~clAudioSource()
{
	Stop();
	FWaveDataProvider = NULL;

	alDeleteSources( 1, &FSourceID );
	alDeleteBuffers( FBuffersCount, &FBufferID[0] );

	g_Audio.UnRegisterSource( this );
}

void clAudioSource::LoopSound( bool Loop )
{
	FLooping = Loop;

	if ( FWaveDataProvider && FWaveDataProvider->IsStreaming() ) { return; }

	alSourcei( FSourceID, AL_LOOPING, Loop ? 1 : 0 );
}

int clAudioSource::StreamBuffer( unsigned int BufferID, int Size )
{
	int ActualSize = FWaveDataProvider->StreamWaveData( Size );

	ubyte* Data = FWaveDataProvider->GetWaveData();
	int Sz = ( int )FWaveDataProvider->GetWaveDataSize();

	alBufferData( BufferID, FWaveDataProvider->GetALFormat(), Data, Sz,
	              FWaveDataProvider->FSamplesPerSec );

	return ActualSize;
}

void clAudioSource::Update( float DeltaSeconds )
{
	if ( !FWaveDataProvider ) { return; }

	if ( !IsPlaying() ) { return; }

	if ( FWaveDataProvider->IsStreaming() )
	{
		int Processed;
		alGetSourcei( FSourceID, AL_BUFFERS_PROCESSED, &Processed );

		while ( Processed-- )
		{
			unsigned int BufID;
			alSourceUnqueueBuffers( FSourceID, 1, &BufID );

			StreamBuffer( BufID, BUFFER_SIZE );

			if ( FWaveDataProvider->IsEOF() )
			{
				if ( FLooping )
				{
					FWaveDataProvider->Seek( 0 );
				}

				StreamBuffer( BufID, BUFFER_SIZE );
			}

			alSourceQueueBuffers( FSourceID, 1, &BufID );
		}
	}
}

void clAudioSource::Play()
{
	if ( IsPlaying() ) { return; }

	if ( !FWaveDataProvider ) { return; }

	int State;
	alGetSourcei( FSourceID, AL_SOURCE_STATE, &State );

	if ( State != AL_PAUSED && FWaveDataProvider->IsStreaming() )
	{
		UnqueueAll();

		StreamBuffer( FBufferID[0], BUFFER_SIZE );
		StreamBuffer( FBufferID[1], BUFFER_SIZE );

		alSourceQueueBuffers( FSourceID, 2, &FBufferID[0] );
	}

	alSourcePlay( FSourceID );
}

void clAudioSource::BindWaveform( clPtr<iWaveDataProvider> Wave )
{
	FWaveDataProvider = Wave;

	if ( !Wave ) { return; }

	if ( FWaveDataProvider->IsStreaming() )
	{
		FBuffersCount = 2;
		alGenBuffers( FBuffersCount, &FBufferID[0] );
	}
	else
	{
		FBuffersCount = 1;

		alGenBuffers( FBuffersCount, &FBufferID[0] );
		alBufferData( FBufferID[0],
		              FWaveDataProvider->GetALFormat(),
		              FWaveDataProvider->GetWaveData(),
		              ( int )FWaveDataProvider->GetWaveDataSize(),
		              FWaveDataProvider->FSamplesPerSec );

		alSourcei( FSourceID, AL_BUFFER, FBufferID[0] );
	}
}

void clAudioThread::RegisterSource( clAudioSource* Src )
{
	LMutex Lock( &FMutex );

	auto i = std::find( FActiveSources.begin(), FActiveSources.end(), Src );

	if ( i != FActiveSources.end() ) { return; }

	FActiveSources.push_back( Src );
}

void clAudioThread::UnRegisterSource( clAudioSource* Src )
{
	LMutex Lock( &FMutex );

	auto i = std::find( FActiveSources.begin(), FActiveSources.end(), Src );

	if ( i != FActiveSources.end() ) { FActiveSources.erase( i ); }
}

void clAudioThread::Run()
{
	if ( !LoadAL() ) { return; }

	// We should use actual device name if the default does not work
	FDevice = alcOpenDevice( NULL );

	FContext = alcCreateContext( FDevice, NULL );

	alcMakeContextCurrent( FContext );

	FInitialized = true;

	FPendingExit = false;

	double Seconds = GetSeconds();

	while ( !IsPendingExit() )
	{
		float DeltaSeconds = static_cast<float>( GetSeconds() - Seconds );

		{
			LMutex Lock( &FMutex );

			for ( auto i = FActiveSources.begin(); i != FActiveSources.end(); i++ )
			{
				( *i )->Update( DeltaSeconds );
			}
		}

		Seconds = GetSeconds();

		Env_Sleep( 100 );
	}

	alcDestroyContext( FContext );
	alcCloseDevice( FDevice );

	UnloadAL();
}

void clAudioThread::Wait() const volatile
{
	while ( !FInitialized ) {};
}

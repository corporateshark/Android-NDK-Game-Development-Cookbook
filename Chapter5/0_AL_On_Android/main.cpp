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

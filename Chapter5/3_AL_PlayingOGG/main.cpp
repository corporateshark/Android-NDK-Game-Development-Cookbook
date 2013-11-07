#include "Wrapper_Callbacks.h"
#include "FileSystem.h"
#include "Audio.h"
#include "OGG.h"
#include "MOD.h"
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
	return ( double )clock() / 1000.0;
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

		Src->BindWaveform( new OggProvider( LoadFileAsBlob( "test.ogg" ) ) );
		// TODO: try Src->BindWaveform( new ModPlugProvider( LoadFileAsBlob( "test.it" ) ) );
		Src->Play();

		FPendingExit = false;

		double Seconds = Env_GetSeconds();

		while ( !IsPendingExit() )
		{
			float DeltaSeconds = static_cast<float>( Env_GetSeconds() - Seconds );
			Src->Update( DeltaSeconds );
			Seconds = Env_GetSeconds();
		}

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

	LoadOGG();
	LoadModPlug();

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

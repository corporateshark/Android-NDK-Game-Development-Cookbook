#include "WorkerThread.h"
#include "FileSystem.h"
#include "Event.h"
#include "Bitmap.h"
#include "Rendering.h"

iAsyncQueue* g_Events;
WorkerThread* g_Loader;
FileSystem* g_FS;

class LoadCompleteCapsule: public iAsyncCapsule
{
public:
	LoadCompleteCapsule( clPtr<Bitmap> Bmp ): FBmp( Bmp ) {}

	virtual void Invoke()
	{
		// draw bitmap on screen

		for ( int i = 0; i != FBmp->FWidth; i++ )
		{
			for ( int j = 0; j != FBmp->FHeight; j++ )
			{
				ubyte* C = &FBmp->FBitmapData[ 3 * ( j * FBmp->FHeight + i ) ];

#if defined(ANDROID)
				int Color = ( *C << 0 ) + ( *( C + 1 ) << 8 ) + ( *( C + 2 ) << 16 );
#else
				int Color = ( *C << 16 ) + ( *( C + 1 ) << 8 ) + ( *( C + 2 ) << 0 );
#endif

				set_pixel( g_FrameBuffer, ImageWidth, ImageHeight, i, j, Color );
			}
		}
	}
private:
	clPtr<Bitmap> FBmp;
};

class LoadOp_Image: public iTask
{
public:
	LoadOp_Image( clPtr<Bitmap> Bmp, clPtr<iIStream> IStream ):
		FBmp( Bmp ), FStream( IStream ) {}

	virtual void Run()
	{
#if defined(_WIN32)
		Sleep( 1000 );
#else
		struct timeval tv;
		/* sleep for 1000us (1s) */
		tv.tv_sec  = 0;
		tv.tv_usec = 1000;
		select( 0, NULL, NULL, NULL, &tv );
#endif

		FBmp->Load2DImage( FStream );

		g_Events->EnqueueCapsule( new LoadCompleteCapsule( FBmp ) );
	}
private:
	clPtr<Bitmap>  FBmp;
	clPtr<iIStream> FStream;
};

clPtr<Bitmap> LoadImg( const std::string& FileName )
{
	clPtr<iIStream> IStream = g_FS->CreateReader( FileName );

	clPtr<Bitmap> Bmp = new Bitmap( 1, 1 );

	g_Loader->AddTask( new LoadOp_Image( Bmp, IStream ) );

	return Bmp;
}

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
	g_Events = new iAsyncQueue();

	g_Loader = new WorkerThread();
	g_Loader->Start( iThread::Priority_Normal );

	clPtr<Bitmap> Bmp = LoadImg( "test.bmp" );
}

void OnDrawFrame()
{
	g_Events->DemultiplexEvents();
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

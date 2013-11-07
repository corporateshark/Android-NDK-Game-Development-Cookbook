#include "CurlWrap.h"
#include "Downloader.h"
#include "Blob.h"

#include <stdio.h>

bool g_ShouldExit;

class TestCallback: public DownloadCompleteCallback
{
public:
	TestCallback() {}

	virtual void Invoke()
	{
		printf( "Download complete\n" );

		printf( "Got response:\n%s\n", ( unsigned char* )FResult->GetData() );

		g_ShouldExit = true;
	}
};

int main()
{
	Curl_Load();

	g_ShouldExit = false;

	iAsyncQueue* Events = new iAsyncQueue();

	Downloader* d = new Downloader();
	d->FEventQueue = Events;

	Sleep( 1000 );

	d->DownloadURL( "http://api.flickr.com/services/rest/?method=flickr.test.echo&name=value", 1, new TestCallback() );

	while ( !g_ShouldExit )
	{
		// dispatch events here
		Events->DemultiplexEvents();
	}

	delete d;

	return 0;
}

#pragma once

#include "Decoders.h"
#include "DecodingProvider.h"

class clModPlugProvider: public clDecodingProvider
{
public:
	explicit clModPlugProvider( const clPtr<clBlob>& Blob ): clDecodingProvider( Blob )
	{
		FChannels = 2;
		FSamplesPerSec = 44100;
		FBitsPerSample = 16;

		FModFile = ModPlug_Load_P( ( const void* )FRawData->GetDataConst(), ( int )FRawData->GetSize() );
	}
	virtual ~clModPlugProvider() { ModPlug_Unload_P( FModFile ); }

	virtual int ReadFromFile( int Size, int BytesRead )
	{
		return ModPlug_Read_P( FModFile, &FBuffer[0] + BytesRead, Size - BytesRead );
	}

	virtual void Seek( float Time )
	{
		FEof = false;
		// convert to msecs
		ModPlug_Seek_P( FModFile, ( int )( Time * 1000.0f ) );
	}

	ModPlugFile* FModFile;
};

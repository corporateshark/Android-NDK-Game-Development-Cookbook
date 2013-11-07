#pragma once

#include "StreamingProviders.h"

class DecodingProvider: public StreamingWaveDataProvider
{
protected:
	virtual int ReadFromFile( int Size, int BytesRead ) = 0;

	clPtr<Blob> FRawData;
public:
	bool              FLoop;
	bool              FEof;

	DecodingProvider( const clPtr<Blob>& blob )
	{
		FRawData = blob;
		FEof = false;
	}

	virtual bool IsEOF() const { return FEof; }

	virtual int StreamWaveData( int Size )
	{
		int OldSize = ( int )FBuffer.size();

		if ( Size > OldSize )
		{
			FBuffer.resize( Size );

			// Fill the new bytes with zeroes, the junk can produce the noise
			for ( int i = 0 ; i < OldSize - Size ; i++ )
			{
				FBuffer[OldSize + i] = 0;
			}
		}

		if ( FEof ) { return 0; }

		int BytesRead = 0;

		while ( BytesRead < Size )
		{
			int Ret = ReadFromFile( Size, BytesRead );

			if ( Ret > 0 )
			{
				BytesRead += Ret;
			}
			else if ( Ret == 0 )
			{
				FEof = true;

				if ( FLoop )
				{
					Seek( 0 ); // rewind to the beginning
					FEof = false;
					continue;
				}

				break;
			}
			else
			{
				Seek( 0 );
				FEof = true;
				break; // some error
			}
		}

		return ( FBufferUsed = BytesRead );
	}
};

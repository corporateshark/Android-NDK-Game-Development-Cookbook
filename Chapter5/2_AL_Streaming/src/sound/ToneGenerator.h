#pragma once

#include "StreamingProviders.h"

#include <vector>
#include <cmath>

/// Streaming monotone wave generator
class ToneGenerator : public StreamingWaveDataProvider
{
public:
	ToneGenerator()
	{
		FBufferUsed = 100000;
		FBuffer.resize( 100000 );

		FChannels = 2;
		FSamplesPerSec = 44100;
		FBitsPerSample = 16;

		FAmplitude = 350.0f;
		FFrequency = 440.0f;
	}
	virtual ~ToneGenerator() {}

	virtual int    StreamWaveData( int Size )
	{
		if ( Size > static_cast<int>( FBuffer.size() ) )
		{
			FBuffer.resize( Size );
			LastOffset = 0;
		}

		for ( int i = 0 ; i < Size / 4 ; i++ )
		{
			float t = ( 2.0f * 3.141592654f * FFrequency * ( i + LastOffset ) ) / ( float )FSamplesPerSec;
			float val = FAmplitude * std::sin( t );

			short V = static_cast<short>( val );
			FBuffer[i * 4 + 0] = V & 0xFF;
			FBuffer[i * 4 + 1] = V >> 8;
			FBuffer[i * 4 + 2] = V & 0xFF;
			FBuffer[i * 4 + 3] = V >> 8;
		}

		LastOffset += Size / 2;
		LastOffset %= FSamplesPerSec;

		return ( FBufferUsed = Size );
	}

	int  FSignalFreq;
	float FFrequency;
	float FAmplitude;
private:
	int LastOffset;
};

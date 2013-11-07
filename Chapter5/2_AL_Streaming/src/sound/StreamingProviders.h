#pragma once

#include "Audio.h"
#include <vector>

class StreamingWaveDataProvider: public iWaveDataProvider
{
public:
	StreamingWaveDataProvider()
	{
	}

	virtual bool            IsStreaming() const { return true; }
	virtual ubyte*          GetWaveData() { return ( ubyte* )&FBuffer[0]; }
	virtual size_t          GetWaveDataSize() const { return FBufferUsed; }

	std::vector<char> FBuffer;
	int               FBufferUsed;
};

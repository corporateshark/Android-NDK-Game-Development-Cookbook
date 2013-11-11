/*
 * Copyright (C) 2013 Sergey Kosarevsky (sk@linderdaum.com)
 * Copyright (C) 2013 Viktor Latypov (vl@linderdaum.com)
 * Based on Linderdaum Engine http://www.linderdaum.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must display the names 'Sergey Kosarevsky' and
 *    'Viktor Latypov'in the credits of the application, if such credits exist.
 *    The authors of this work must be notified via email (sk@linderdaum.com) in
 *    this case of redistribution.
 *
 * 3. Neither the name of copyright holders nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "Engine.h"
#include "LAL.h"

#include <vector>

/// Provider of waveform data for streaming
class iWaveDataProvider: public iObject
{
public:
	iWaveDataProvider(): FChannels( 0 ),
		FSamplesPerSec( 0 ),
		FBitsPerSample( 0 ) {}

	virtual ubyte* GetWaveData() = 0;
	virtual size_t GetWaveDataSize() const = 0;

	virtual bool IsEOF() const { return true; }
	virtual void Seek( float Time ) {}
	virtual bool IsStreaming() const { return false; }
	virtual int  StreamWaveData( int Size ) { return 0; }

	/// Format of waveform data
	ALuint GetALFormat() const
	{
		if ( FBitsPerSample == 8 ) { return ( FChannels == 2 ) ? AL_FORMAT_STEREO8  : AL_FORMAT_MONO8;  }

		if ( FBitsPerSample == 16 ) { return ( FChannels == 2 ) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16; }

		return AL_FORMAT_MONO8;
	}

	int    FChannels;
	int    FSamplesPerSec;
	int    FBitsPerSample;
};

class clStreamingWaveDataProvider: public iWaveDataProvider
{
public:
	clStreamingWaveDataProvider()
	{
	}

	virtual bool            IsStreaming() const { return true; }
	virtual ubyte*          GetWaveData() { return ( ubyte* )&FBuffer[0]; }
	virtual size_t          GetWaveDataSize() const { return FBufferUsed; }

	std::vector<char> FBuffer;
	int               FBufferUsed;
};

class clDecodingProvider: public clStreamingWaveDataProvider
{
protected:
	virtual int ReadFromFile( int Size, int BytesRead ) = 0;

	clPtr<clBlob> FRawData;
public:
	bool              FLoop;
	bool              FEof;

	explicit clDecodingProvider( const clPtr<clBlob>& blob )
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

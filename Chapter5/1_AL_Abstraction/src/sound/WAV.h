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

#ifndef __WAV_inl__
#define __WAV_inl__

#include "Audio.h"
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

/// .WAV file data provider for audio streaming
class WavProvider: public iWaveDataProvider
{
public:
	WavProvider( const clPtr<Blob>& Blob )
	{
		FRawData = Blob;
		size_t DataSize = FRawData->GetSize();
		//if( sizeof( sWAVHeader ) > DataSize) { "Invalid WAV header" }

		sWAVHeader H = *( const sWAVHeader* )FRawData->GetDataConst();
		// if( ( H.RIFF[0] != 'R' ) || ( H.RIFF[1] != 'I' ) || ( H.RIFF[2] != 'F' ) || ( H.RIFF[3] != 'F' )) { "Can only read RIFF files" }

		const unsigned short FORMAT_PCM = 1;
		// if( Header.FormatTag != FORMAT_PCM) { "Can only read PCM wave files" }
		// if( Header.DataSize > DataSize - sizeof( Header )) { "Invalid WAV file" }
		FChannels      = H.Channels;
		FSamplesPerSec = H.SampleRate;
		FBitsPerSample = H.nBitsperSample;
	}
	virtual ~WavProvider() {}

	virtual ubyte*   GetWaveData() { return ( ubyte* )FRawData->GetDataConst() + sizeof( sWAVHeader ); }
	virtual size_t   GetWaveDataSize() const { return FRawData->GetSize() - sizeof( sWAVHeader ); }

	clPtr<Blob> FRawData;
};

#endif

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

#include "Decoders.h"
#include "DecodingProvider.h"

/// .OGG file data provider for audio streaming
class OggProvider: public DecodingProvider
{
public:
	OggProvider( const clPtr<Blob>& Blob ): DecodingProvider( Blob )
	{
		FOGGRawPosition = 0;

		ov_callbacks Callbacks;
		Callbacks.read_func  = OGG_ReadFunc;
		Callbacks.seek_func  = OGG_SeekFunc;
		Callbacks.close_func = OGG_CloseFunc;
		Callbacks.tell_func  = OGG_TellFunc;

		vorbis_info*      VorbisInfo;

		// check for "< 0"
		OGG_ov_open_callbacks( this, &FVorbisFile, NULL, -1, Callbacks );

		VorbisInfo    = OGG_ov_info ( &FVorbisFile, -1 );
		FChannels      = VorbisInfo->channels;
		FSamplesPerSec = VorbisInfo->rate;
		FBitsPerSample = 16;
	}
	virtual ~OggProvider() { OGG_ov_clear( &FVorbisFile ); }

	virtual int ReadFromFile( int Size, int BytesRead )
	{
		return ( int )OGG_ov_read( &FVorbisFile, &FBuffer[0] + BytesRead, Size - BytesRead, 0, /* LITTLE_ENDIAN,*/ FBitsPerSample >> 3, 1, &FOGGCurrentSection );
	}

	virtual void    Seek( float Time )
	{
		FEof = false;
		OGG_ov_time_seek( &FVorbisFile, Time );
	}
private:

#include "OGG_Callbacks.h"

	OggVorbis_File         FVorbisFile;
	ogg_int64_t            FOGGRawPosition;
	int                    FOGGCurrentSection;
};

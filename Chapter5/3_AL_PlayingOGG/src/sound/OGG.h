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

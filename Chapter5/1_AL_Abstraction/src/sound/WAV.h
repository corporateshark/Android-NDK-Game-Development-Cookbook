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

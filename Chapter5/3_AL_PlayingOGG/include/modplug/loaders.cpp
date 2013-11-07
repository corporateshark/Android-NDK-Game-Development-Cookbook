/*
 * This source code is public domain.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>,
 *          Adam Goode       <adam@evdebs.org> (Endian and char fixes for PPC)
 *          Marco Trillo     <toad@arsystel.com> (Endian fixes for SaveIT, XM->IT Sample Converter)
 *
*/

#include "stdafx.h"
#include "sndfile.h"


#ifndef _ITDEFS_H_
#define _ITDEFS_H_

#pragma pack(1)

typedef struct tagITFILEHEADER
{
	DWORD id;         // 0x4D504D49
	CHAR songname[26];
	WORD reserved1;      // 0x1004
	WORD ordnum;
	WORD insnum;
	WORD smpnum;
	WORD patnum;
	WORD cwtv;
	WORD cmwt;
	WORD flags;
	WORD special;
	BYTE globalvol;
	BYTE mv;
	BYTE speed;
	BYTE tempo;
	BYTE sep;
	BYTE zero;
	WORD msglength;
	DWORD msgoffset;
	DWORD reserved2;
	BYTE chnpan[64];
	BYTE chnvol[64];
} ITFILEHEADER;


typedef struct tagITENVELOPE
{
	BYTE flags;
	BYTE num;
	BYTE lpb;
	BYTE lpe;
	BYTE slb;
	BYTE sle;
	BYTE data[25 * 3];
	BYTE reserved;
} ITENVELOPE;

// Old Impulse Instrument Format (cmwt < 0x200)
typedef struct tagITOLDINSTRUMENT
{
	DWORD id;         // IMPI = 0x49504D49
	CHAR filename[12];   // DOS file name
	BYTE zero;
	BYTE flags;
	BYTE vls;
	BYTE vle;
	BYTE sls;
	BYTE sle;
	WORD reserved1;
	WORD fadeout;
	BYTE nna;
	BYTE dnc;
	WORD trkvers;
	BYTE nos;
	BYTE reserved2;
	CHAR name[26];
	WORD reserved3[3];
	BYTE keyboard[240];
	BYTE volenv[200];
	BYTE nodes[50];
} ITOLDINSTRUMENT;


// Impulse Instrument Format
typedef struct tagITINSTRUMENT
{
	DWORD id;
	CHAR filename[12];
	BYTE zero;
	BYTE nna;
	BYTE dct;
	BYTE dca;
	WORD fadeout;
	signed char pps;
	BYTE ppc;
	BYTE gbv;
	BYTE dfp;
	BYTE rv;
	BYTE rp;
	WORD trkvers;
	BYTE nos;
	BYTE reserved1;
	CHAR name[26];
	BYTE ifc;
	BYTE ifr;
	BYTE mch;
	BYTE mpr;
	WORD mbank;
	BYTE keyboard[240];
	ITENVELOPE volenv;
	ITENVELOPE panenv;
	ITENVELOPE pitchenv;
	BYTE dummy[4]; // was 7, but IT v2.17 saves 554 bytes
} ITINSTRUMENT;


// IT Sample Format
typedef struct ITSAMPLESTRUCT
{
	DWORD id;      // 0x53504D49
	CHAR filename[12];
	BYTE zero;
	BYTE gvl;
	BYTE flags;
	BYTE vol;
	CHAR name[26];
	BYTE cvt;
	BYTE dfp;
	DWORD length;
	DWORD loopbegin;
	DWORD loopend;
	DWORD C5Speed;
	DWORD susloopbegin;
	DWORD susloopend;
	DWORD samplepointer;
	BYTE vis;
	BYTE vid;
	BYTE vir;
	BYTE vit;
} ITSAMPLESTRUCT;

#pragma pack()

extern BYTE autovibit2xm[8];
extern BYTE autovibxm2it[8];

#endif

// MMCMP.cpp

BOOL PP20_Unpack( LPCBYTE* ppMemFile, LPDWORD pdwMemLength );

typedef struct MMCMPFILEHEADER
{
	DWORD id_ziRC; // "ziRC"
	DWORD id_ONia; // "ONia"
	WORD hdrsize;
} MMCMPFILEHEADER, *LPMMCMPFILEHEADER;

typedef struct MMCMPHEADER
{
	WORD version;
	WORD nblocks;
	DWORD filesize;
	DWORD blktable;
	BYTE glb_comp;
	BYTE fmt_comp;
} MMCMPHEADER, *LPMMCMPHEADER;

typedef struct MMCMPBLOCK
{
	DWORD unpk_size;
	DWORD pk_size;
	DWORD xor_chk;
	WORD sub_blk;
	WORD flags;
	WORD tt_entries;
	WORD num_bits;
} MMCMPBLOCK, *LPMMCMPBLOCK;

typedef struct MMCMPSUBBLOCK
{
	DWORD unpk_pos;
	DWORD unpk_size;
} MMCMPSUBBLOCK, *LPMMCMPSUBBLOCK;

#define MMCMP_COMP      0x0001
#define MMCMP_DELTA     0x0002
#define MMCMP_16BIT     0x0004
#define MMCMP_STEREO 0x0100
#define MMCMP_ABS16     0x0200
#define MMCMP_ENDIAN 0x0400

typedef struct MMCMPBITBUFFER
{
	UINT bitcount;
	DWORD bitbuffer;
	LPCBYTE pSrc;
	LPCBYTE pEnd;

	DWORD GetBits( UINT nBits );
} MMCMPBITBUFFER;


DWORD MMCMPBITBUFFER::GetBits( UINT nBits )
//---------------------------------------
{
	DWORD d;

	if ( !nBits ) { return 0; }

	while ( bitcount < 24 )
	{
		bitbuffer |= ( ( pSrc < pEnd ) ? *pSrc++ : 0 ) << bitcount;
		bitcount += 8;
	}

	d = bitbuffer & ( ( 1 << nBits ) - 1 );
	bitbuffer >>= nBits;
	bitcount -= nBits;
	return d;
}

//#define MMCMP_LOG

#ifdef MMCMP_LOG
extern void Log( LPCSTR s, ... );
#endif

const DWORD MMCMP8BitCommands[8] =
{
	0x01, 0x03, 0x07, 0x0F, 0x1E, 0x3C, 0x78, 0xF8
};

const UINT MMCMP8BitFetch[8] =
{
	3, 3, 3, 3, 2, 1, 0, 0
};

const DWORD MMCMP16BitCommands[16] =
{
	0x01, 0x03, 0x07, 0x0F, 0x1E, 0x3C, 0x78, 0xF0,
	0x1F0, 0x3F0, 0x7F0, 0xFF0, 0x1FF0, 0x3FF0, 0x7FF0, 0xFFF0
};

const UINT MMCMP16BitFetch[16] =
{
	4, 4, 4, 4, 3, 2, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 0
};


BOOL MMCMP_Unpack( LPCBYTE* ppMemFile, LPDWORD pdwMemLength )
//---------------------------------------------------------
{
	DWORD dwMemLength = *pdwMemLength;
	LPCBYTE lpMemFile = *ppMemFile;
	LPBYTE pBuffer;
	LPMMCMPFILEHEADER pmfh = ( LPMMCMPFILEHEADER )( lpMemFile );
	LPMMCMPHEADER pmmh = ( LPMMCMPHEADER )( lpMemFile + 10 );
	LPDWORD pblk_table;
	DWORD dwFileSize;

	if ( PP20_Unpack( ppMemFile, pdwMemLength ) )
	{
		return TRUE;
	}

	if ( ( dwMemLength < 256 ) || ( !pmfh ) || ( pmfh->id_ziRC != 0x4352697A ) || ( pmfh->id_ONia != 0x61694e4f ) || ( pmfh->hdrsize < 14 )
	     || ( !pmmh->nblocks ) || ( pmmh->filesize < 16 ) || ( pmmh->filesize > 0x8000000 )
	     || ( pmmh->blktable >= dwMemLength ) || ( pmmh->blktable + 4 * pmmh->nblocks > dwMemLength ) ) { return FALSE; }

	dwFileSize = pmmh->filesize;

	if ( ( pBuffer = ( LPBYTE )GlobalAllocPtr( GHND, ( dwFileSize + 31 ) & ~15 ) ) == NULL ) { return FALSE; }

	pblk_table = ( LPDWORD )( lpMemFile + pmmh->blktable );

	for ( UINT nBlock = 0; nBlock < pmmh->nblocks; nBlock++ )
	{
		DWORD dwMemPos = pblk_table[nBlock];
		LPMMCMPBLOCK pblk = ( LPMMCMPBLOCK )( lpMemFile + dwMemPos );
		LPMMCMPSUBBLOCK psubblk = ( LPMMCMPSUBBLOCK )( lpMemFile + dwMemPos + 20 );

		if ( ( dwMemPos + 20 >= dwMemLength ) || ( dwMemPos + 20 + pblk->sub_blk * 8 >= dwMemLength ) ) { break; }

		dwMemPos += 20 + pblk->sub_blk * 8;
#ifdef MMCMP_LOG
		Log( "block %d: flags=%04X sub_blocks=%d", nBlock, ( UINT )pblk->flags, ( UINT )pblk->sub_blk );
		Log( " pksize=%d unpksize=%d", pblk->pk_size, pblk->unpk_size );
		Log( " tt_entries=%d num_bits=%d\n", pblk->tt_entries, pblk->num_bits );
#endif

		// Data is not packed
		if ( !( pblk->flags & MMCMP_COMP ) )
		{
			for ( UINT i = 0; i < pblk->sub_blk; i++ )
			{
				if ( ( psubblk->unpk_pos > dwFileSize ) || ( psubblk->unpk_pos + psubblk->unpk_size > dwFileSize ) ) { break; }

#ifdef MMCMP_LOG
				Log( "  Unpacked sub-block %d: offset %d, size=%d\n", i, psubblk->unpk_pos, psubblk->unpk_size );
#endif
				memcpy( pBuffer + psubblk->unpk_pos, lpMemFile + dwMemPos, psubblk->unpk_size );
				dwMemPos += psubblk->unpk_size;
				psubblk++;
			}
		}
		else

			// Data is 16-bit packed
			if ( pblk->flags & MMCMP_16BIT )
			{
				MMCMPBITBUFFER bb;
				LPWORD pDest = ( LPWORD )( pBuffer + psubblk->unpk_pos );
				DWORD dwSize = psubblk->unpk_size >> 1;
				DWORD dwPos = 0;
				UINT numbits = pblk->num_bits;
				UINT subblk = 0, oldval = 0;

#ifdef MMCMP_LOG
				Log( "  16-bit block: pos=%d size=%d ", psubblk->unpk_pos, psubblk->unpk_size );

				if ( pblk->flags & MMCMP_DELTA ) { Log( "DELTA " ); }

				if ( pblk->flags & MMCMP_ABS16 ) { Log( "ABS16 " ); }

				Log( "\n" );
#endif
				bb.bitcount = 0;
				bb.bitbuffer = 0;
				bb.pSrc = lpMemFile + dwMemPos + pblk->tt_entries;
				bb.pEnd = lpMemFile + dwMemPos + pblk->pk_size;

				while ( subblk < pblk->sub_blk )
				{
					UINT newval = 0x10000;
					DWORD d = bb.GetBits( numbits + 1 );

					if ( d >= MMCMP16BitCommands[numbits] )
					{
						UINT nFetch = MMCMP16BitFetch[numbits];
						UINT newbits = bb.GetBits( nFetch ) + ( ( d - MMCMP16BitCommands[numbits] ) << nFetch );

						if ( newbits != numbits )
						{
							numbits = newbits & 0x0F;
						}
						else
						{
							if ( ( d = bb.GetBits( 4 ) ) == 0x0F )
							{
								if ( bb.GetBits( 1 ) ) { break; }

								newval = 0xFFFF;
							}
							else
							{
								newval = 0xFFF0 + d;
							}
						}
					}
					else
					{
						newval = d;
					}

					if ( newval < 0x10000 )
					{
						newval = ( newval & 1 ) ? ( UINT )( -( LONG )( ( newval + 1 ) >> 1 ) ) : ( UINT )( newval >> 1 );

						if ( pblk->flags & MMCMP_DELTA )
						{
							newval += oldval;
							oldval = newval;
						}
						else if ( !( pblk->flags & MMCMP_ABS16 ) )
						{
							newval ^= 0x8000;
						}

						pDest[dwPos++] = ( WORD )newval;
					}

					if ( dwPos >= dwSize )
					{
						subblk++;
						dwPos = 0;
						dwSize = psubblk[subblk].unpk_size >> 1;
						pDest = ( LPWORD )( pBuffer + psubblk[subblk].unpk_pos );
					}
				}
			}
			else
				// Data is 8-bit packed
			{
				MMCMPBITBUFFER bb;
				LPBYTE pDest = pBuffer + psubblk->unpk_pos;
				DWORD dwSize = psubblk->unpk_size;
				DWORD dwPos = 0;
				UINT numbits = pblk->num_bits;
				UINT subblk = 0, oldval = 0;
				LPCBYTE ptable = lpMemFile + dwMemPos;

				bb.bitcount = 0;
				bb.bitbuffer = 0;
				bb.pSrc = lpMemFile + dwMemPos + pblk->tt_entries;
				bb.pEnd = lpMemFile + dwMemPos + pblk->pk_size;

				while ( subblk < pblk->sub_blk )
				{
					UINT newval = 0x100;
					DWORD d = bb.GetBits( numbits + 1 );

					if ( d >= MMCMP8BitCommands[numbits] )
					{
						UINT nFetch = MMCMP8BitFetch[numbits];
						UINT newbits = bb.GetBits( nFetch ) + ( ( d - MMCMP8BitCommands[numbits] ) << nFetch );

						if ( newbits != numbits )
						{
							numbits = newbits & 0x07;
						}
						else
						{
							if ( ( d = bb.GetBits( 3 ) ) == 7 )
							{
								if ( bb.GetBits( 1 ) ) { break; }

								newval = 0xFF;
							}
							else
							{
								newval = 0xF8 + d;
							}
						}
					}
					else
					{
						newval = d;
					}

					if ( newval < 0x100 )
					{
						int n = ptable[newval];

						if ( pblk->flags & MMCMP_DELTA )
						{
							n += oldval;
							oldval = n;
						}

						pDest[dwPos++] = ( BYTE )n;
					}

					if ( dwPos >= dwSize )
					{
						subblk++;
						dwPos = 0;
						dwSize = psubblk[subblk].unpk_size;
						pDest = pBuffer + psubblk[subblk].unpk_pos;
					}
				}
			}
	}

	*ppMemFile = pBuffer;
	*pdwMemLength = dwFileSize;
	return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
//
// PowerPack PP20 Unpacker
//

typedef struct _PPBITBUFFER
{
	UINT bitcount;
	ULONG bitbuffer;
	LPCBYTE pStart;
	LPCBYTE pSrc;

	ULONG GetBits( UINT n );
} PPBITBUFFER;


ULONG PPBITBUFFER::GetBits( UINT n )
{
	ULONG result = 0;

	for ( UINT i = 0; i < n; i++ )
	{
		if ( !bitcount )
		{
			bitcount = 8;

			if ( pSrc != pStart ) { pSrc--; }

			bitbuffer = *pSrc;
		}

		result = ( result << 1 ) | ( bitbuffer & 1 );
		bitbuffer >>= 1;
		bitcount--;
	}

	return result;
}


VOID PP20_DoUnpack( const BYTE* pSrc, UINT nSrcLen, BYTE* pDst, UINT nDstLen )
{
	PPBITBUFFER BitBuffer;
	ULONG nBytesLeft;

	BitBuffer.pStart = pSrc;
	BitBuffer.pSrc = pSrc + nSrcLen - 4;
	BitBuffer.bitbuffer = 0;
	BitBuffer.bitcount = 0;
	BitBuffer.GetBits( pSrc[nSrcLen - 1] );
	nBytesLeft = nDstLen;

	while ( nBytesLeft > 0 )
	{
		if ( !BitBuffer.GetBits( 1 ) )
		{
			UINT n = 1;

			while ( n < nBytesLeft )
			{
				UINT code = BitBuffer.GetBits( 2 );
				n += code;

				if ( code != 3 ) { break; }
			}

			for ( UINT i = 0; i < n; i++ )
			{
				pDst[--nBytesLeft] = ( BYTE )BitBuffer.GetBits( 8 );
			}

			if ( !nBytesLeft ) { break; }
		}

		{
			UINT n = BitBuffer.GetBits( 2 ) + 1;
			UINT nbits = pSrc[n - 1];
			UINT nofs;

			if ( n == 4 )
			{
				nofs = BitBuffer.GetBits( ( BitBuffer.GetBits( 1 ) ) ? nbits : 7 );

				while ( n < nBytesLeft )
				{
					UINT code = BitBuffer.GetBits( 3 );
					n += code;

					if ( code != 7 ) { break; }
				}
			}
			else
			{
				nofs = BitBuffer.GetBits( nbits );
			}

			for ( UINT i = 0; i <= n; i++ )
			{
				pDst[nBytesLeft - 1] = ( nBytesLeft + nofs < nDstLen ) ? pDst[nBytesLeft + nofs] : 0;

				if ( !--nBytesLeft ) { break; }
			}
		}
	}
}


BOOL PP20_Unpack( LPCBYTE* ppMemFile, LPDWORD pdwMemLength )
{
	DWORD dwMemLength = *pdwMemLength;
	LPCBYTE lpMemFile = *ppMemFile;
	DWORD dwDstLen;
	LPBYTE pBuffer;

	if ( ( !lpMemFile ) || ( dwMemLength < 256 ) || ( *( DWORD* )lpMemFile != 0x30325050 ) ) { return FALSE; }

	dwDstLen = ( lpMemFile[dwMemLength - 4] << 16 ) | ( lpMemFile[dwMemLength - 3] << 8 ) | ( lpMemFile[dwMemLength - 2] );

	//Log("PP20 detected: Packed length=%d, Unpacked length=%d\n", dwMemLength, dwDstLen);
	if ( ( dwDstLen < 512 ) || ( dwDstLen > 0x400000 ) || ( dwDstLen > 16 * dwMemLength ) ) { return FALSE; }

	if ( ( pBuffer = ( LPBYTE )GlobalAllocPtr( GHND, ( dwDstLen + 31 ) & ~15 ) ) == NULL ) { return FALSE; }

	PP20_DoUnpack( lpMemFile + 4, dwMemLength - 4, pBuffer, dwDstLen );
	*ppMemFile = pBuffer;
	*pdwMemLength = dwDstLen;
	return TRUE;
}


/// end of MMCMP.cpp


#ifdef _MSC_VER
#pragma warning(disable:4244)
#endif

BYTE autovibit2xm[8] =
{ 0, 3, 1, 4, 2, 0, 0, 0 };

BYTE autovibxm2it[8] =
{ 0, 2, 4, 1, 3, 0, 0, 0 };

//////////////////////////////////////////////////////////
// Impulse Tracker IT file support

// for conversion of XM samples
extern WORD XMPeriodTable[96 + 8];
extern UINT XMLinearTable[768];

static inline UINT ConvertVolParam( UINT value )
//--------------------------------------------
{
	return ( value > 9 )  ? 9 : value;
}


BOOL CSoundFile::ITInstrToMPT( const void* p, INSTRUMENTHEADER* penv, UINT trkvers )
//--------------------------------------------------------------------------------
{
	if ( trkvers < 0x0200 )
	{
		const ITOLDINSTRUMENT* pis = ( const ITOLDINSTRUMENT* )p;
		memcpy( penv->name, pis->name, 26 );
		memcpy( penv->filename, pis->filename, 12 );
		penv->nFadeOut = bswapLE16( pis->fadeout ) << 6;
		penv->nGlobalVol = 64;

		for ( UINT j = 0; j < NOTE_MAX; j++ )
		{
			UINT note = pis->keyboard[j * 2];
			UINT ins = pis->keyboard[j * 2 + 1];

			if ( ins < MAX_SAMPLES ) { penv->Keyboard[j] = ins; }

			if ( note < 128 ) { penv->NoteMap[j] = note + 1; }
			else if ( note >= 0xFE ) { penv->NoteMap[j] = note; }
		}

		if ( pis->flags & 0x01 ) { penv->dwFlags |= ENV_VOLUME; }

		if ( pis->flags & 0x02 ) { penv->dwFlags |= ENV_VOLLOOP; }

		if ( pis->flags & 0x04 ) { penv->dwFlags |= ENV_VOLSUSTAIN; }

		penv->nVolLoopStart = pis->vls;
		penv->nVolLoopEnd = pis->vle;
		penv->nVolSustainBegin = pis->sls;
		penv->nVolSustainEnd = pis->sle;
		penv->nVolEnv = 25;

		for ( UINT ev = 0; ev < 25; ev++ )
		{
			if ( ( penv->VolPoints[ev] = pis->nodes[ev * 2] ) == 0xFF )
			{
				penv->nVolEnv = ev;
				break;
			}

			penv->VolEnv[ev] = pis->nodes[ev * 2 + 1];
		}

		penv->nNNA = pis->nna;
		penv->nDCT = pis->dnc;
		penv->nPan = 0x80;
	}
	else
	{
		const ITINSTRUMENT* pis = ( const ITINSTRUMENT* )p;
		memcpy( penv->name, pis->name, 26 );
		memcpy( penv->filename, pis->filename, 12 );
		penv->nMidiProgram = pis->mpr;
		penv->nMidiChannel = pis->mch;
		penv->wMidiBank = bswapLE16( pis->mbank );
		penv->nFadeOut = bswapLE16( pis->fadeout ) << 5;
		penv->nGlobalVol = pis->gbv >> 1;

		if ( penv->nGlobalVol > 64 ) { penv->nGlobalVol = 64; }

		for ( UINT j = 0; j < NOTE_MAX; j++ )
		{
			UINT note = pis->keyboard[j * 2];
			UINT ins = pis->keyboard[j * 2 + 1];

			if ( ins < MAX_SAMPLES ) { penv->Keyboard[j] = ins; }

			if ( note < 128 ) { penv->NoteMap[j] = note + 1; }
			else if ( note >= 0xFE ) { penv->NoteMap[j] = note; }
		}

		// Volume Envelope
		if ( pis->volenv.flags & 1 ) { penv->dwFlags |= ENV_VOLUME; }

		if ( pis->volenv.flags & 2 ) { penv->dwFlags |= ENV_VOLLOOP; }

		if ( pis->volenv.flags & 4 ) { penv->dwFlags |= ENV_VOLSUSTAIN; }

		if ( pis->volenv.flags & 8 ) { penv->dwFlags |= ENV_VOLCARRY; }

		penv->nVolEnv = pis->volenv.num;

		if ( penv->nVolEnv > 25 ) { penv->nVolEnv = 25; }

		penv->nVolLoopStart = pis->volenv.lpb;
		penv->nVolLoopEnd = pis->volenv.lpe;
		penv->nVolSustainBegin = pis->volenv.slb;
		penv->nVolSustainEnd = pis->volenv.sle;

		// Panning Envelope
		if ( pis->panenv.flags & 1 ) { penv->dwFlags |= ENV_PANNING; }

		if ( pis->panenv.flags & 2 ) { penv->dwFlags |= ENV_PANLOOP; }

		if ( pis->panenv.flags & 4 ) { penv->dwFlags |= ENV_PANSUSTAIN; }

		if ( pis->panenv.flags & 8 ) { penv->dwFlags |= ENV_PANCARRY; }

		penv->nPanEnv = pis->panenv.num;

		if ( penv->nPanEnv > 25 ) { penv->nPanEnv = 25; }

		penv->nPanLoopStart = pis->panenv.lpb;
		penv->nPanLoopEnd = pis->panenv.lpe;
		penv->nPanSustainBegin = pis->panenv.slb;
		penv->nPanSustainEnd = pis->panenv.sle;

		// Pitch Envelope
		if ( pis->pitchenv.flags & 1 ) { penv->dwFlags |= ENV_PITCH; }

		if ( pis->pitchenv.flags & 2 ) { penv->dwFlags |= ENV_PITCHLOOP; }

		if ( pis->pitchenv.flags & 4 ) { penv->dwFlags |= ENV_PITCHSUSTAIN; }

		if ( pis->pitchenv.flags & 8 ) { penv->dwFlags |= ENV_PITCHCARRY; }

		if ( pis->pitchenv.flags & 0x80 ) { penv->dwFlags |= ENV_FILTER; }

		penv->nPitchEnv = pis->pitchenv.num;

		if ( penv->nPitchEnv > 25 ) { penv->nPitchEnv = 25; }

		penv->nPitchLoopStart = pis->pitchenv.lpb;
		penv->nPitchLoopEnd = pis->pitchenv.lpe;
		penv->nPitchSustainBegin = pis->pitchenv.slb;
		penv->nPitchSustainEnd = pis->pitchenv.sle;

		// Envelopes Data
		for ( UINT ev = 0; ev < 25; ev++ )
		{
			penv->VolEnv[ev] = pis->volenv.data[ev * 3];
			penv->VolPoints[ev] = ( pis->volenv.data[ev * 3 + 2] << 8 ) | ( pis->volenv.data[ev * 3 + 1] );
			penv->PanEnv[ev] = pis->panenv.data[ev * 3] + 32;
			penv->PanPoints[ev] = ( pis->panenv.data[ev * 3 + 2] << 8 ) | ( pis->panenv.data[ev * 3 + 1] );
			penv->PitchEnv[ev] = pis->pitchenv.data[ev * 3] + 32;
			penv->PitchPoints[ev] = ( pis->pitchenv.data[ev * 3 + 2] << 8 ) | ( pis->pitchenv.data[ev * 3 + 1] );
		}

		penv->nNNA = pis->nna;
		penv->nDCT = pis->dct;
		penv->nDNA = pis->dca;
		penv->nPPS = pis->pps;
		penv->nPPC = pis->ppc;
		penv->nIFC = pis->ifc;
		penv->nIFR = pis->ifr;
		penv->nVolSwing = pis->rv;
		penv->nPanSwing = pis->rp;
		penv->nPan = ( pis->dfp & 0x7F ) << 2;

		if ( penv->nPan > 256 ) { penv->nPan = 128; }

		if ( pis->dfp < 0x80 ) { penv->dwFlags |= ENV_SETPANNING; }
	}

	if ( ( penv->nVolLoopStart >= 25 ) || ( penv->nVolLoopEnd >= 25 ) ) { penv->dwFlags &= ~ENV_VOLLOOP; }

	if ( ( penv->nVolSustainBegin >= 25 ) || ( penv->nVolSustainEnd >= 25 ) ) { penv->dwFlags &= ~ENV_VOLSUSTAIN; }

	return TRUE;
}


BOOL CSoundFile::ReadIT( const BYTE* lpStream, DWORD dwMemLength )
//--------------------------------------------------------------
{
	ITFILEHEADER pifh = *( ITFILEHEADER* )lpStream;
	DWORD dwMemPos = sizeof( ITFILEHEADER );
	DWORD inspos[MAX_INSTRUMENTS];
	DWORD smppos[MAX_SAMPLES];
	DWORD patpos[MAX_PATTERNS];
	BYTE chnmask[64], channels_used[64];
	MODCOMMAND lastvalue[64];

	pifh.id = bswapLE32( pifh.id );
	pifh.reserved1 = bswapLE16( pifh.reserved1 );
	pifh.ordnum = bswapLE16( pifh.ordnum );
	pifh.insnum = bswapLE16( pifh.insnum );
	pifh.smpnum = bswapLE16( pifh.smpnum );
	pifh.patnum = bswapLE16( pifh.patnum );
	pifh.cwtv = bswapLE16( pifh.cwtv );
	pifh.cmwt = bswapLE16( pifh.cmwt );
	pifh.flags = bswapLE16( pifh.flags );
	pifh.special = bswapLE16( pifh.special );
	pifh.msglength = bswapLE16( pifh.msglength );
	pifh.msgoffset = bswapLE32( pifh.msgoffset );
	pifh.reserved2 = bswapLE32( pifh.reserved2 );

	if ( ( !lpStream ) || ( dwMemLength < 0x100 ) ) { return FALSE; }

	if ( ( pifh.id != 0x4D504D49 ) || ( pifh.insnum >= MAX_INSTRUMENTS )
	     || ( !pifh.smpnum ) || ( pifh.smpnum >= MAX_INSTRUMENTS ) || ( !pifh.ordnum ) ) { return FALSE; }

	if ( dwMemPos + pifh.ordnum + pifh.insnum * 4
	     + pifh.smpnum * 4 + pifh.patnum * 4 > dwMemLength ) { return FALSE; }

	m_nType = MOD_TYPE_IT;

	if ( pifh.flags & 0x08 ) { m_dwSongFlags |= SONG_LINEARSLIDES; }

	if ( pifh.flags & 0x10 ) { m_dwSongFlags |= SONG_ITOLDEFFECTS; }

	if ( pifh.flags & 0x20 ) { m_dwSongFlags |= SONG_ITCOMPATMODE; }

	if ( pifh.flags & 0x80 ) { m_dwSongFlags |= SONG_EMBEDMIDICFG; }

	if ( pifh.flags & 0x1000 ) { m_dwSongFlags |= SONG_EXFILTERRANGE; }

	memcpy( m_szNames[0], pifh.songname, 26 );
	m_szNames[0][26] = 0;

	// Global Volume
	if ( pifh.globalvol )
	{
		m_nDefaultGlobalVolume = pifh.globalvol << 1;

		if ( !m_nDefaultGlobalVolume ) { m_nDefaultGlobalVolume = 256; }

		if ( m_nDefaultGlobalVolume > 256 ) { m_nDefaultGlobalVolume = 256; }
	}

	if ( pifh.speed ) { m_nDefaultSpeed = pifh.speed; }

	if ( pifh.tempo ) { m_nDefaultTempo = pifh.tempo; }

	m_nSongPreAmp = pifh.mv & 0x7F;

	// Reading Channels Pan Positions
	for ( int ipan = 0; ipan < 64; ipan++ ) if ( pifh.chnpan[ipan] != 0xFF )
		{
			ChnSettings[ipan].nVolume = pifh.chnvol[ipan];
			ChnSettings[ipan].nPan = 128;

			if ( pifh.chnpan[ipan] & 0x80 ) { ChnSettings[ipan].dwFlags |= CHN_MUTE; }

			UINT n = pifh.chnpan[ipan] & 0x7F;

			if ( n <= 64 ) { ChnSettings[ipan].nPan = n << 2; }

			if ( n == 100 ) { ChnSettings[ipan].dwFlags |= CHN_SURROUND; }
		}

	if ( m_nChannels < 4 ) { m_nChannels = 4; }

	// Reading Song Message
	if ( ( pifh.special & 0x01 ) && ( pifh.msglength ) && ( pifh.msgoffset + pifh.msglength < dwMemLength ) )
	{
		m_lpszSongComments = new char[pifh.msglength + 1];

		if ( m_lpszSongComments )
		{
			memcpy( m_lpszSongComments, lpStream + pifh.msgoffset, pifh.msglength );
			m_lpszSongComments[pifh.msglength] = 0;
		}
	}

	// Reading orders
	UINT nordsize = pifh.ordnum;

	if ( nordsize > MAX_ORDERS ) { nordsize = MAX_ORDERS; }

	memcpy( Order, lpStream + dwMemPos, nordsize );
	dwMemPos += pifh.ordnum;
	// Reading Instrument Offsets
	memset( inspos, 0, sizeof( inspos ) );
	UINT inspossize = pifh.insnum;

	if ( inspossize > MAX_INSTRUMENTS ) { inspossize = MAX_INSTRUMENTS; }

	inspossize <<= 2;
	memcpy( inspos, lpStream + dwMemPos, inspossize );

	for ( UINT j = 0; j < ( inspossize >> 2 ); j++ )
	{
		inspos[j] = bswapLE32( inspos[j] );
	}

	dwMemPos += pifh.insnum * 4;
	// Reading Samples Offsets
	memset( smppos, 0, sizeof( smppos ) );
	UINT smppossize = pifh.smpnum;

	if ( smppossize > MAX_SAMPLES ) { smppossize = MAX_SAMPLES; }

	smppossize <<= 2;
	memcpy( smppos, lpStream + dwMemPos, smppossize );

	for ( UINT j = 0; j < ( smppossize >> 2 ); j++ )
	{
		smppos[j] = bswapLE32( smppos[j] );
	}

	dwMemPos += pifh.smpnum * 4;
	// Reading Patterns Offsets
	memset( patpos, 0, sizeof( patpos ) );
	UINT patpossize = pifh.patnum;

	if ( patpossize > MAX_PATTERNS ) { patpossize = MAX_PATTERNS; }

	patpossize <<= 2;
	memcpy( patpos, lpStream + dwMemPos, patpossize );

	for ( UINT j = 0; j < ( patpossize >> 2 ); j++ )
	{
		patpos[j] = bswapLE32( patpos[j] );
	}

	dwMemPos += pifh.patnum * 4;

	// Reading IT Extra Info
	if ( dwMemPos + 2 < dwMemLength )
	{
		UINT nflt = bswapLE16( *( ( WORD* )( lpStream + dwMemPos ) ) );
		dwMemPos += 2;

		if ( dwMemPos + nflt * 8 < dwMemLength ) { dwMemPos += nflt * 8; }
	}

	// Reading Midi Output & Macros
	if ( m_dwSongFlags & SONG_EMBEDMIDICFG )
	{
		if ( dwMemPos + sizeof( MODMIDICFG ) < dwMemLength )
		{
			memcpy( &m_MidiCfg, lpStream + dwMemPos, sizeof( MODMIDICFG ) );
			dwMemPos += sizeof( MODMIDICFG );
		}
	}

	// Read pattern names: "PNAM"
	if ( ( dwMemPos + 8 < dwMemLength ) && ( bswapLE32( *( ( DWORD* )( lpStream + dwMemPos ) ) ) == 0x4d414e50 ) )
	{
		UINT len = bswapLE32( *( ( DWORD* )( lpStream + dwMemPos + 4 ) ) );
		dwMemPos += 8;

		if ( ( dwMemPos + len <= dwMemLength ) && ( len <= MAX_PATTERNS * MAX_PATTERNNAME ) && ( len >= MAX_PATTERNNAME ) )
		{
			m_lpszPatternNames = new char[len];

			if ( m_lpszPatternNames )
			{
				m_nPatternNames = len / MAX_PATTERNNAME;
				memcpy( m_lpszPatternNames, lpStream + dwMemPos, len );
			}

			dwMemPos += len;
		}
	}

	// 4-channels minimum
	m_nChannels = 4;

	// Read channel names: "CNAM"
	if ( ( dwMemPos + 8 < dwMemLength ) && ( bswapLE32( *( ( DWORD* )( lpStream + dwMemPos ) ) ) == 0x4d414e43 ) )
	{
		UINT len = bswapLE32( *( ( DWORD* )( lpStream + dwMemPos + 4 ) ) );
		dwMemPos += 8;

		if ( ( dwMemPos + len <= dwMemLength ) && ( len <= 64 * MAX_CHANNELNAME ) )
		{
			UINT n = len / MAX_CHANNELNAME;

			if ( n > m_nChannels ) { m_nChannels = n; }

			for ( UINT i = 0; i < n; i++ )
			{
				memcpy( ChnSettings[i].szName, ( lpStream + dwMemPos + i * MAX_CHANNELNAME ), MAX_CHANNELNAME );
				ChnSettings[i].szName[MAX_CHANNELNAME - 1] = 0;
			}

			dwMemPos += len;
		}
	}

	// Read mix plugins information
	if ( dwMemPos + 8 < dwMemLength )
	{
		dwMemPos += LoadMixPlugins( lpStream + dwMemPos, dwMemLength - dwMemPos );
	}

	// Checking for unused channels
	UINT npatterns = pifh.patnum;

	if ( npatterns > MAX_PATTERNS ) { npatterns = MAX_PATTERNS; }

	for ( UINT patchk = 0; patchk < npatterns; patchk++ )
	{
		memset( chnmask, 0, sizeof( chnmask ) );

		if ( ( !patpos[patchk] ) || ( ( DWORD )patpos[patchk] + 4 >= dwMemLength ) ) { continue; }

		UINT len = bswapLE16( *( ( WORD* )( lpStream + patpos[patchk] ) ) );
		UINT rows = bswapLE16( *( ( WORD* )( lpStream + patpos[patchk] + 2 ) ) );

		if ( ( rows < 4 ) || ( rows > 256 ) ) { continue; }

		if ( patpos[patchk] + 8 + len > dwMemLength ) { continue; }

		UINT i = 0;
		const BYTE* p = lpStream + patpos[patchk] + 8;
		UINT nrow = 0;

		while ( nrow < rows )
		{
			if ( i >= len ) { break; }

			BYTE b = p[i++];

			if ( !b )
			{
				nrow++;
				continue;
			}

			UINT ch = b & 0x7F;

			if ( ch ) { ch = ( ch - 1 ) & 0x3F; }

			if ( b & 0x80 )
			{
				if ( i >= len ) { break; }

				chnmask[ch] = p[i++];
			}

			// Channel used
			if ( chnmask[ch] & 0x0F )
			{
				if ( ( ch >= m_nChannels ) && ( ch < 64 ) ) { m_nChannels = ch + 1; }
			}

			// Note
			if ( chnmask[ch] & 1 ) { i++; }

			// Instrument
			if ( chnmask[ch] & 2 ) { i++; }

			// Volume
			if ( chnmask[ch] & 4 ) { i++; }

			// Effect
			if ( chnmask[ch] & 8 ) { i += 2; }

			if ( i >= len ) { break; }
		}
	}

	// Reading Instruments
	m_nInstruments = 0;

	if ( pifh.flags & 0x04 ) { m_nInstruments = pifh.insnum; }

	if ( m_nInstruments >= MAX_INSTRUMENTS ) { m_nInstruments = MAX_INSTRUMENTS - 1; }

	for ( UINT nins = 0; nins < m_nInstruments; nins++ )
	{
		if ( ( inspos[nins] > 0 ) && ( inspos[nins] < dwMemLength - sizeof( ITOLDINSTRUMENT ) ) )
		{
			INSTRUMENTHEADER* penv = new INSTRUMENTHEADER;

			if ( !penv ) { continue; }

			Headers[nins + 1] = penv;
			memset( penv, 0, sizeof( INSTRUMENTHEADER ) );
			ITInstrToMPT( lpStream + inspos[nins], penv, pifh.cmwt );
		}
	}

	// Reading Samples
	m_nSamples = pifh.smpnum;

	if ( m_nSamples >= MAX_SAMPLES ) { m_nSamples = MAX_SAMPLES - 1; }

	for ( UINT nsmp = 0; nsmp < pifh.smpnum; nsmp++ ) if ( ( smppos[nsmp] ) && ( smppos[nsmp] + sizeof( ITSAMPLESTRUCT ) <= dwMemLength ) )
		{
			ITSAMPLESTRUCT pis = *( ITSAMPLESTRUCT* )( lpStream + smppos[nsmp] );
			pis.id = bswapLE32( pis.id );
			pis.length = bswapLE32( pis.length );
			pis.loopbegin = bswapLE32( pis.loopbegin );
			pis.loopend = bswapLE32( pis.loopend );
			pis.C5Speed = bswapLE32( pis.C5Speed );
			pis.susloopbegin = bswapLE32( pis.susloopbegin );
			pis.susloopend = bswapLE32( pis.susloopend );
			pis.samplepointer = bswapLE32( pis.samplepointer );

			if ( pis.id == 0x53504D49 )
			{
				MODINSTRUMENT* pins = &Ins[nsmp + 1];
				memcpy( pins->name, pis.filename, 12 );
				pins->uFlags = 0;
				pins->nLength = 0;
				pins->nLoopStart = pis.loopbegin;
				pins->nLoopEnd = pis.loopend;
				pins->nSustainStart = pis.susloopbegin;
				pins->nSustainEnd = pis.susloopend;
				pins->nC4Speed = pis.C5Speed;

				if ( !pins->nC4Speed ) { pins->nC4Speed = 8363; }

				if ( pis.C5Speed < 256 ) { pins->nC4Speed = 256; }

				pins->nVolume = pis.vol << 2;

				if ( pins->nVolume > 256 ) { pins->nVolume = 256; }

				pins->nGlobalVol = pis.gvl;

				if ( pins->nGlobalVol > 64 ) { pins->nGlobalVol = 64; }

				if ( pis.flags & 0x10 ) { pins->uFlags |= CHN_LOOP; }

				if ( pis.flags & 0x20 ) { pins->uFlags |= CHN_SUSTAINLOOP; }

				if ( pis.flags & 0x40 ) { pins->uFlags |= CHN_PINGPONGLOOP; }

				if ( pis.flags & 0x80 ) { pins->uFlags |= CHN_PINGPONGSUSTAIN; }

				pins->nPan = ( pis.dfp & 0x7F ) << 2;

				if ( pins->nPan > 256 ) { pins->nPan = 256; }

				if ( pis.dfp & 0x80 ) { pins->uFlags |= CHN_PANNING; }

				pins->nVibType = autovibit2xm[pis.vit & 7];
				pins->nVibRate = pis.vis;
				pins->nVibDepth = pis.vid & 0x7F;
				pins->nVibSweep = ( pis.vir + 3 ) / 4;

				if ( ( pis.samplepointer ) && ( pis.samplepointer < dwMemLength ) && ( pis.length ) )
				{
					pins->nLength = pis.length;

					if ( pins->nLength > MAX_SAMPLE_LENGTH ) { pins->nLength = MAX_SAMPLE_LENGTH; }

					UINT flags = ( pis.cvt & 1 ) ? RS_PCM8S : RS_PCM8U;

					if ( pis.flags & 2 )
					{
						flags += 5;

						if ( pis.flags & 4 ) { flags |= RSF_STEREO; }

						pins->uFlags |= CHN_16BIT;

						// IT 2.14 16-bit packed sample ?
						if ( pis.flags & 8 ) { flags = ( ( pifh.cmwt >= 0x215 ) && ( pis.cvt & 4 ) ) ? RS_IT21516 : RS_IT21416; }
					}
					else
					{
						if ( pis.flags & 4 ) { flags |= RSF_STEREO; }

						if ( pis.cvt == 0xFF ) { flags = RS_ADPCM4; }
						else

							// IT 2.14 8-bit packed sample ?
							if ( pis.flags & 8 ) { flags =   ( ( pifh.cmwt >= 0x215 ) && ( pis.cvt & 4 ) ) ? RS_IT2158 : RS_IT2148; }
					}

					ReadSample( &Ins[nsmp + 1], flags, ( LPSTR )( lpStream + pis.samplepointer ), dwMemLength - pis.samplepointer );
				}
			}

			memcpy( m_szNames[nsmp + 1], pis.name, 26 );
		}

	// Reading Patterns
	for ( UINT npat = 0; npat < npatterns; npat++ )
	{
		if ( ( !patpos[npat] ) || ( ( DWORD )patpos[npat] + 4 >= dwMemLength ) )
		{
			PatternSize[npat] = 64;
			Patterns[npat] = AllocatePattern( 64, m_nChannels );
			continue;
		}

		UINT len = bswapLE16( *( ( WORD* )( lpStream + patpos[npat] ) ) );
		UINT rows = bswapLE16( *( ( WORD* )( lpStream + patpos[npat] + 2 ) ) );

		if ( ( rows < 4 ) || ( rows > 256 ) ) { continue; }

		if ( patpos[npat] + 8 + len > dwMemLength ) { continue; }

		PatternSize[npat] = rows;

		if ( ( Patterns[npat] = AllocatePattern( rows, m_nChannels ) ) == NULL ) { continue; }

		memset( lastvalue, 0, sizeof( lastvalue ) );
		memset( chnmask, 0, sizeof( chnmask ) );
		MODCOMMAND* m = Patterns[npat];
		UINT i = 0;
		const BYTE* p = lpStream + patpos[npat] + 8;
		UINT nrow = 0;

		while ( nrow < rows )
		{
			if ( i >= len ) { break; }

			BYTE b = p[i++];

			if ( !b )
			{
				nrow++;
				m += m_nChannels;
				continue;
			}

			UINT ch = b & 0x7F;

			if ( ch ) { ch = ( ch - 1 ) & 0x3F; }

			if ( b & 0x80 )
			{
				if ( i >= len ) { break; }

				chnmask[ch] = p[i++];
			}

			if ( ( chnmask[ch] & 0x10 ) && ( ch < m_nChannels ) )
			{
				m[ch].note = lastvalue[ch].note;
			}

			if ( ( chnmask[ch] & 0x20 ) && ( ch < m_nChannels ) )
			{
				m[ch].instr = lastvalue[ch].instr;
			}

			if ( ( chnmask[ch] & 0x40 ) && ( ch < m_nChannels ) )
			{
				m[ch].volcmd = lastvalue[ch].volcmd;
				m[ch].vol = lastvalue[ch].vol;
			}

			if ( ( chnmask[ch] & 0x80 ) && ( ch < m_nChannels ) )
			{
				m[ch].command = lastvalue[ch].command;
				m[ch].param = lastvalue[ch].param;
			}

			if ( chnmask[ch] & 1 ) // Note
			{
				if ( i >= len ) { break; }

				UINT note = p[i++];

				if ( ch < m_nChannels )
				{
					if ( note < 0x80 ) { note++; }

					m[ch].note = note;
					lastvalue[ch].note = note;
					channels_used[ch] = TRUE;
				}
			}

			if ( chnmask[ch] & 2 )
			{
				if ( i >= len ) { break; }

				UINT instr = p[i++];

				if ( ch < m_nChannels )
				{
					m[ch].instr = instr;
					lastvalue[ch].instr = instr;
				}
			}

			if ( chnmask[ch] & 4 )
			{
				if ( i >= len ) { break; }

				UINT vol = p[i++];

				if ( ch < m_nChannels )
				{
					// 0-64: Set Volume
					if ( vol <= 64 ) { m[ch].volcmd = VOLCMD_VOLUME; m[ch].vol = vol; }
					else

						// 128-192: Set Panning
						if ( ( vol >= 128 ) && ( vol <= 192 ) ) { m[ch].volcmd = VOLCMD_PANNING; m[ch].vol = vol - 128; }
						else

							// 65-74: Fine Volume Up
							if ( vol < 75 ) { m[ch].volcmd = VOLCMD_FINEVOLUP; m[ch].vol = vol - 65; }
							else

								// 75-84: Fine Volume Down
								if ( vol < 85 ) { m[ch].volcmd = VOLCMD_FINEVOLDOWN; m[ch].vol = vol - 75; }
								else

									// 85-94: Volume Slide Up
									if ( vol < 95 ) { m[ch].volcmd = VOLCMD_VOLSLIDEUP; m[ch].vol = vol - 85; }
									else

										// 95-104: Volume Slide Down
										if ( vol < 105 ) { m[ch].volcmd = VOLCMD_VOLSLIDEDOWN; m[ch].vol = vol - 95; }
										else

											// 105-114: Pitch Slide Up
											if ( vol < 115 ) { m[ch].volcmd = VOLCMD_PORTADOWN; m[ch].vol = vol - 105; }
											else

												// 115-124: Pitch Slide Down
												if ( vol < 125 ) { m[ch].volcmd = VOLCMD_PORTAUP; m[ch].vol = vol - 115; }
												else

													// 193-202: Portamento To
													if ( ( vol >= 193 ) && ( vol <= 202 ) ) { m[ch].volcmd = VOLCMD_TONEPORTAMENTO; m[ch].vol = vol - 193; }
													else

														// 203-212: Vibrato
														if ( ( vol >= 203 ) && ( vol <= 212 ) ) { m[ch].volcmd = VOLCMD_VIBRATOSPEED; m[ch].vol = vol - 203; }

					lastvalue[ch].volcmd = m[ch].volcmd;
					lastvalue[ch].vol = m[ch].vol;
				}
			}

			// Reading command/param
			if ( chnmask[ch] & 8 )
			{
				if ( i > len - 2 ) { break; }

				UINT cmd = p[i++];
				UINT param = p[i++];

				if ( ch < m_nChannels )
				{
					if ( cmd )
					{
						m[ch].command = cmd;
						m[ch].param = param;
						S3MConvert( &m[ch], TRUE );
						lastvalue[ch].command = m[ch].command;
						lastvalue[ch].param = m[ch].param;
					}
				}
			}
		}
	}

	for ( UINT ncu = 0; ncu < MAX_BASECHANNELS; ncu++ )
	{
		if ( ncu >= m_nChannels )
		{
			ChnSettings[ncu].nVolume = 64;
			ChnSettings[ncu].dwFlags &= ~CHN_MUTE;
		}
	}

	m_nMinPeriod = 8;
	m_nMaxPeriod = 0xF000;
	return TRUE;
}


#ifndef MODPLUG_NO_FILESAVE
//#define SAVEITTIMESTAMP
#ifdef _MSC_VER
#pragma warning(disable:4100)
#endif

BOOL CSoundFile::SaveIT( LPCSTR lpszFileName, UINT nPacking )
//---------------------------------------------------------
{
	DWORD dwPatNamLen, dwChnNamLen;
	ITFILEHEADER header, writeheader;
	ITINSTRUMENT iti, writeiti;
	ITSAMPLESTRUCT itss;
	BYTE smpcount[MAX_SAMPLES];
	DWORD inspos[MAX_INSTRUMENTS];
	DWORD patpos[MAX_PATTERNS];
	DWORD smppos[MAX_SAMPLES];
	DWORD dwPos = 0, dwHdrPos = 0, dwExtra = 2;
	WORD patinfo[4];
	BYTE chnmask[64];
	BYTE buf[512];
	MODCOMMAND lastvalue[64];
	FILE* f;


	if ( ( !lpszFileName ) || ( ( f = fopen( lpszFileName, "wb" ) ) == NULL ) ) { return FALSE; }

	memset( inspos, 0, sizeof( inspos ) );
	memset( patpos, 0, sizeof( patpos ) );
	memset( smppos, 0, sizeof( smppos ) );
	// Writing Header
	memset( &header, 0, sizeof( header ) );
	dwPatNamLen = 0;
	dwChnNamLen = 0;
	header.id = 0x4D504D49; // IMPM
	lstrcpyn( ( char* )header.songname, m_szNames[0], 27 );
	header.reserved1 = 0x1004;
	header.ordnum = 0;

	while ( ( header.ordnum < MAX_ORDERS ) && ( Order[header.ordnum] < 0xFF ) ) { header.ordnum++; }

	if ( header.ordnum < MAX_ORDERS ) { Order[header.ordnum++] = 0xFF; }

	header.insnum = m_nInstruments;
	header.smpnum = m_nSamples;
	header.patnum = MAX_PATTERNS;

	while ( ( header.patnum > 0 ) && ( !Patterns[header.patnum - 1] ) ) { header.patnum--; }

	header.cwtv = 0x217;
	header.cmwt = 0x200;
	header.flags = 0x0001;
	header.special = 0x0006;

	if ( m_nInstruments ) { header.flags |= 0x04; }

	if ( m_dwSongFlags & SONG_LINEARSLIDES ) { header.flags |= 0x08; }

	if ( m_dwSongFlags & SONG_ITOLDEFFECTS ) { header.flags |= 0x10; }

	if ( m_dwSongFlags & SONG_ITCOMPATMODE ) { header.flags |= 0x20; }

	if ( m_dwSongFlags & SONG_EXFILTERRANGE ) { header.flags |= 0x1000; }

	header.globalvol = m_nDefaultGlobalVolume >> 1;
	header.mv = m_nSongPreAmp;

	// clip song pre-amp values (between 0x20 and 0x7f)
	if ( header.mv < 0x20 ) { header.mv = 0x20; }

	if ( header.mv > 0x7F ) { header.mv = 0x7F; }

	header.speed = m_nDefaultSpeed;
	header.tempo = m_nDefaultTempo;
	header.sep = m_nStereoSeparation;
	dwHdrPos = sizeof( header ) + header.ordnum;
	// Channel Pan and Volume
	memset( header.chnpan, 0xFF, 64 );
	memset( header.chnvol, 64, 64 );

	for ( UINT ich = 0; ich < m_nChannels; ich++ )
	{
		header.chnpan[ich] = ChnSettings[ich].nPan >> 2;

		if ( ChnSettings[ich].dwFlags & CHN_SURROUND ) { header.chnpan[ich] = 100; }

		header.chnvol[ich] = ChnSettings[ich].nVolume;

		if ( ChnSettings[ich].dwFlags & CHN_MUTE ) { header.chnpan[ich] |= 0x80; }

		if ( ChnSettings[ich].szName[0] )
		{
			dwChnNamLen = ( ich + 1 ) * MAX_CHANNELNAME;
		}
	}

	if ( dwChnNamLen ) { dwExtra += dwChnNamLen + 8; }

#ifdef SAVEITTIMESTAMP
	dwExtra += 8; // Time Stamp
#endif

	if ( m_dwSongFlags & SONG_EMBEDMIDICFG )
	{
		header.flags |= 0x80;
		header.special |= 0x08;
		dwExtra += sizeof( MODMIDICFG );
	}

	// Pattern Names
	if ( ( m_nPatternNames ) && ( m_lpszPatternNames ) )
	{
		dwPatNamLen = m_nPatternNames * MAX_PATTERNNAME;

		while ( ( dwPatNamLen >= MAX_PATTERNNAME ) && ( !m_lpszPatternNames[dwPatNamLen - MAX_PATTERNNAME] ) ) { dwPatNamLen -= MAX_PATTERNNAME; }

		if ( dwPatNamLen < MAX_PATTERNNAME ) { dwPatNamLen = 0; }

		if ( dwPatNamLen ) { dwExtra += dwPatNamLen + 8; }
	}

	// Mix Plugins
	dwExtra += SaveMixPlugins( NULL, TRUE );

	// Comments
	if ( m_lpszSongComments )
	{
		header.special |= 1;
		header.msglength = strlen( m_lpszSongComments ) + 1;
		header.msgoffset = dwHdrPos + dwExtra + header.insnum * 4 + header.patnum * 4 + header.smpnum * 4;
	}

	// Write file header
	memcpy( &writeheader, &header, sizeof( header ) );

	// Byteswap header information
	writeheader.id = bswapLE32( writeheader.id );
	writeheader.reserved1 = bswapLE16( writeheader.reserved1 );
	writeheader.ordnum = bswapLE16( writeheader.ordnum );
	writeheader.insnum = bswapLE16( writeheader.insnum );
	writeheader.smpnum = bswapLE16( writeheader.smpnum );
	writeheader.patnum = bswapLE16( writeheader.patnum );
	writeheader.cwtv = bswapLE16( writeheader.cwtv );
	writeheader.cmwt = bswapLE16( writeheader.cmwt );
	writeheader.flags = bswapLE16( writeheader.flags );
	writeheader.special = bswapLE16( writeheader.special );
	writeheader.msglength = bswapLE16( writeheader.msglength );
	writeheader.msgoffset = bswapLE32( writeheader.msgoffset );
	writeheader.reserved2 = bswapLE32( writeheader.reserved2 );

	fwrite( &writeheader, 1, sizeof( writeheader ), f );

	fwrite( Order, 1, header.ordnum, f );

	if ( header.insnum ) { fwrite( inspos, 4, header.insnum, f ); }

	if ( header.smpnum ) { fwrite( smppos, 4, header.smpnum, f ); }

	if ( header.patnum ) { fwrite( patpos, 4, header.patnum, f ); }

	// Writing editor history information
	{
#ifdef SAVEITTIMESTAMP
		SYSTEMTIME systime;
		FILETIME filetime;
		WORD timestamp[4];
		WORD nInfoEx = 1;
		memset( timestamp, 0, sizeof( timestamp ) );
		fwrite( &nInfoEx, 1, 2, f );
		GetSystemTime( &systime );
		SystemTimeToFileTime( &systime, &filetime );
		FileTimeToDosDateTime( &filetime, &timestamp[0], &timestamp[1] );
		fwrite( timestamp, 1, 8, f );
#else
		WORD nInfoEx = 0;
		fwrite( &nInfoEx, 1, 2, f );
#endif
	}

	// Writing midi cfg
	if ( header.flags & 0x80 )
	{
		fwrite( &m_MidiCfg, 1, sizeof( MODMIDICFG ), f );
	}

	// Writing pattern names
	if ( dwPatNamLen )
	{
		DWORD d = bswapLE32( 0x4d414e50 );
		UINT len = bswapLE32( dwPatNamLen );
		fwrite( &d, 1, 4, f );
		fwrite( &len, 1, 4, f );
		fwrite( m_lpszPatternNames, 1, dwPatNamLen, f );
	}

	// Writing channel Names
	if ( dwChnNamLen )
	{
		DWORD d = bswapLE32( 0x4d414e43 );
		UINT len = bswapLE32( dwChnNamLen );
		fwrite( &d, 1, 4, f );
		fwrite( &len, 1, 4, f );
		UINT nChnNames = dwChnNamLen / MAX_CHANNELNAME;

		for ( UINT inam = 0; inam < nChnNames; inam++ )
		{
			fwrite( ChnSettings[inam].szName, 1, MAX_CHANNELNAME, f );
		}
	}

	// Writing mix plugins info
	SaveMixPlugins( f, FALSE );
	// Writing song message
	dwPos = dwHdrPos + dwExtra + ( header.insnum + header.smpnum + header.patnum ) * 4;

	if ( header.special & 1 )
	{
		dwPos += strlen( m_lpszSongComments ) + 1;
		fwrite( m_lpszSongComments, 1, strlen( m_lpszSongComments ) + 1, f );
	}

	// Writing instruments
	for ( UINT nins = 1; nins <= header.insnum; nins++ )
	{
		memset( &iti, 0, sizeof( iti ) );
		iti.id = 0x49504D49; // "IMPI"
		iti.trkvers = 0x211;

		if ( Headers[nins] )
		{
			INSTRUMENTHEADER* penv = Headers[nins];
			memset( smpcount, 0, sizeof( smpcount ) );
			memcpy( iti.filename, penv->filename, 12 );
			memcpy( iti.name, penv->name, 26 );
			iti.mbank = penv->wMidiBank;
			iti.mpr = penv->nMidiProgram;
			iti.mch = penv->nMidiChannel;
			iti.nna = penv->nNNA;
			iti.dct = penv->nDCT;
			iti.dca = penv->nDNA;
			iti.fadeout = penv->nFadeOut >> 5;
			iti.pps = penv->nPPS;
			iti.ppc = penv->nPPC;
			iti.gbv = ( BYTE )( penv->nGlobalVol << 1 );
			iti.dfp = ( BYTE )penv->nPan >> 2;

			if ( !( penv->dwFlags & ENV_SETPANNING ) ) { iti.dfp |= 0x80; }

			iti.rv = penv->nVolSwing;
			iti.rp = penv->nPanSwing;
			iti.ifc = penv->nIFC;
			iti.ifr = penv->nIFR;
			iti.nos = 0;

			for ( UINT i = 0; i < NOTE_MAX; i++ ) if ( penv->Keyboard[i] < MAX_SAMPLES )
				{
					UINT smp = penv->Keyboard[i];

					if ( ( smp ) && ( !smpcount[smp] ) )
					{
						smpcount[smp] = 1;
						iti.nos++;
					}

					iti.keyboard[i * 2] = penv->NoteMap[i] - 1;
					iti.keyboard[i * 2 + 1] = smp;
				}

			// Writing Volume envelope
			if ( penv->dwFlags & ENV_VOLUME ) { iti.volenv.flags |= 0x01; }

			if ( penv->dwFlags & ENV_VOLLOOP ) { iti.volenv.flags |= 0x02; }

			if ( penv->dwFlags & ENV_VOLSUSTAIN ) { iti.volenv.flags |= 0x04; }

			if ( penv->dwFlags & ENV_VOLCARRY ) { iti.volenv.flags |= 0x08; }

			iti.volenv.num = ( BYTE )penv->nVolEnv;
			iti.volenv.lpb = ( BYTE )penv->nVolLoopStart;
			iti.volenv.lpe = ( BYTE )penv->nVolLoopEnd;
			iti.volenv.slb = penv->nVolSustainBegin;
			iti.volenv.sle = penv->nVolSustainEnd;

			// Writing Panning envelope
			if ( penv->dwFlags & ENV_PANNING ) { iti.panenv.flags |= 0x01; }

			if ( penv->dwFlags & ENV_PANLOOP ) { iti.panenv.flags |= 0x02; }

			if ( penv->dwFlags & ENV_PANSUSTAIN ) { iti.panenv.flags |= 0x04; }

			if ( penv->dwFlags & ENV_PANCARRY ) { iti.panenv.flags |= 0x08; }

			iti.panenv.num = ( BYTE )penv->nPanEnv;
			iti.panenv.lpb = ( BYTE )penv->nPanLoopStart;
			iti.panenv.lpe = ( BYTE )penv->nPanLoopEnd;
			iti.panenv.slb = penv->nPanSustainBegin;
			iti.panenv.sle = penv->nPanSustainEnd;

			// Writing Pitch Envelope
			if ( penv->dwFlags & ENV_PITCH ) { iti.pitchenv.flags |= 0x01; }

			if ( penv->dwFlags & ENV_PITCHLOOP ) { iti.pitchenv.flags |= 0x02; }

			if ( penv->dwFlags & ENV_PITCHSUSTAIN ) { iti.pitchenv.flags |= 0x04; }

			if ( penv->dwFlags & ENV_PITCHCARRY ) { iti.pitchenv.flags |= 0x08; }

			if ( penv->dwFlags & ENV_FILTER ) { iti.pitchenv.flags |= 0x80; }

			iti.pitchenv.num = ( BYTE )penv->nPitchEnv;
			iti.pitchenv.lpb = ( BYTE )penv->nPitchLoopStart;
			iti.pitchenv.lpe = ( BYTE )penv->nPitchLoopEnd;
			iti.pitchenv.slb = ( BYTE )penv->nPitchSustainBegin;
			iti.pitchenv.sle = ( BYTE )penv->nPitchSustainEnd;

			// Writing Envelopes data
			for ( UINT ev = 0; ev < 25; ev++ )
			{
				iti.volenv.data[ev * 3] = penv->VolEnv[ev];
				iti.volenv.data[ev * 3 + 1] = penv->VolPoints[ev] & 0xFF;
				iti.volenv.data[ev * 3 + 2] = penv->VolPoints[ev] >> 8;
				iti.panenv.data[ev * 3] = penv->PanEnv[ev] - 32;
				iti.panenv.data[ev * 3 + 1] = penv->PanPoints[ev] & 0xFF;
				iti.panenv.data[ev * 3 + 2] = penv->PanPoints[ev] >> 8;
				iti.pitchenv.data[ev * 3] = penv->PitchEnv[ev] - 32;
				iti.pitchenv.data[ev * 3 + 1] = penv->PitchPoints[ev] & 0xFF;
				iti.pitchenv.data[ev * 3 + 2] = penv->PitchPoints[ev] >> 8;
			}
		}
		else
			// Save Empty Instrument
		{
			for ( UINT i = 0; i < NOTE_MAX; i++ ) { iti.keyboard[i * 2] = i; }

			iti.ppc = 5 * 12;
			iti.gbv = 128;
			iti.dfp = 0x20;
			iti.ifc = 0xFF;
		}

		if ( !iti.nos ) { iti.trkvers = 0; }

		// Writing instrument
		inspos[nins - 1] = dwPos;
		dwPos += sizeof( ITINSTRUMENT );

		memcpy( &writeiti, &iti, sizeof( ITINSTRUMENT ) );

		writeiti.fadeout = bswapLE16( writeiti.fadeout );
		writeiti.id = bswapLE32( writeiti.id );
		writeiti.trkvers = bswapLE16( writeiti.trkvers );
		writeiti.mbank = bswapLE16( writeiti.mbank );

		fwrite( &writeiti, 1, sizeof( ITINSTRUMENT ), f );
	}

	// Writing sample headers
	memset( &itss, 0, sizeof( itss ) );

	for ( UINT hsmp = 0; hsmp < header.smpnum; hsmp++ )
	{
		smppos[hsmp] = dwPos;
		dwPos += sizeof( ITSAMPLESTRUCT );
		fwrite( &itss, 1, sizeof( ITSAMPLESTRUCT ), f );
	}

	// Writing Patterns
	for ( UINT npat = 0; npat < header.patnum; npat++ )
	{
		DWORD dwPatPos = dwPos;
		UINT len;

		if ( !Patterns[npat] ) { continue; }

		patpos[npat] = dwPos;
		patinfo[0] = 0;
		patinfo[1] = bswapLE16( PatternSize[npat] );
		patinfo[2] = 0;
		patinfo[3] = 0;

		// Check for empty pattern
		if ( PatternSize[npat] == 64 )
		{
			MODCOMMAND* pzc = Patterns[npat];
			UINT iz, nz = PatternSize[npat] * m_nChannels;

			for ( iz = 0; iz < nz; iz++ )
			{
				if ( ( pzc[iz].note ) || ( pzc[iz].instr )
				     || ( pzc[iz].volcmd ) || ( pzc[iz].command ) ) { break; }
			}

			if ( iz == nz )
			{
				patpos[npat] = 0;
				continue;
			}
		}

		fwrite( patinfo, 8, 1, f );
		dwPos += 8;
		memset( chnmask, 0xFF, sizeof( chnmask ) );
		memset( lastvalue, 0, sizeof( lastvalue ) );
		MODCOMMAND* m = Patterns[npat];

		for ( UINT row = 0; row < PatternSize[npat]; row++ )
		{
			len = 0;

			for ( UINT ch = 0; ch < m_nChannels; ch++, m++ )
			{
				BYTE b = 0;
				UINT command = m->command;
				UINT param = m->param;
				UINT vol = 0xFF;
				UINT note = m->note;

				if ( note ) { b |= 1; }

				if ( ( note ) && ( note < 0x80 ) ) { note--; } // 0xfe->0x80 --Toad

				if ( m->instr ) { b |= 2; }

				if ( m->volcmd )
				{
					UINT volcmd = m->volcmd;

					switch ( volcmd )
					{
						case VOLCMD_VOLUME:
							vol = m->vol;

							if ( vol > 64 ) { vol = 64; }

							break;

						case VOLCMD_PANNING:
							vol = m->vol + 128;

							if ( vol > 192 ) { vol = 192; }

							break;

						case VOLCMD_VOLSLIDEUP:
							vol = 85 + ConvertVolParam( m->vol );
							break;

						case VOLCMD_VOLSLIDEDOWN:
							vol = 95 + ConvertVolParam( m->vol );
							break;

						case VOLCMD_FINEVOLUP:
							vol = 65 + ConvertVolParam( m->vol );
							break;

						case VOLCMD_FINEVOLDOWN:
							vol = 75 + ConvertVolParam( m->vol );
							break;

						case VOLCMD_VIBRATOSPEED:
							vol = 203 + ConvertVolParam( m->vol );
							break;

						case VOLCMD_VIBRATO:
							vol = 203;
							break;

						case VOLCMD_TONEPORTAMENTO:
							vol = 193 + ConvertVolParam( m->vol );
							break;

						case VOLCMD_PORTADOWN:
							vol = 105 + ConvertVolParam( m->vol );
							break;

						case VOLCMD_PORTAUP:
							vol = 115 + ConvertVolParam( m->vol );
							break;

						default:
							vol = 0xFF;
					}
				}

				if ( vol != 0xFF ) { b |= 4; }

				if ( command )
				{
					S3MSaveConvert( &command, &param, TRUE );

					if ( command ) { b |= 8; }
				}

				// Packing information
				if ( b )
				{
					// Same note ?
					if ( b & 1 )
					{
						if ( ( note == lastvalue[ch].note ) && ( lastvalue[ch].volcmd & 1 ) )
						{
							b &= ~1;
							b |= 0x10;
						}
						else
						{
							lastvalue[ch].note = note;
							lastvalue[ch].volcmd |= 1;
						}
					}

					// Same instrument ?
					if ( b & 2 )
					{
						if ( ( m->instr == lastvalue[ch].instr ) && ( lastvalue[ch].volcmd & 2 ) )
						{
							b &= ~2;
							b |= 0x20;
						}
						else
						{
							lastvalue[ch].instr = m->instr;
							lastvalue[ch].volcmd |= 2;
						}
					}

					// Same volume column byte ?
					if ( b & 4 )
					{
						if ( ( vol == lastvalue[ch].vol ) && ( lastvalue[ch].volcmd & 4 ) )
						{
							b &= ~4;
							b |= 0x40;
						}
						else
						{
							lastvalue[ch].vol = vol;
							lastvalue[ch].volcmd |= 4;
						}
					}

					// Same command / param ?
					if ( b & 8 )
					{
						if ( ( command == lastvalue[ch].command ) && ( param == lastvalue[ch].param ) && ( lastvalue[ch].volcmd & 8 ) )
						{
							b &= ~8;
							b |= 0x80;
						}
						else
						{
							lastvalue[ch].command = command;
							lastvalue[ch].param = param;
							lastvalue[ch].volcmd |= 8;
						}
					}

					if ( b != chnmask[ch] )
					{
						chnmask[ch] = b;
						buf[len++] = ( ch + 1 ) | 0x80;
						buf[len++] = b;
					}
					else
					{
						buf[len++] = ch + 1;
					}

					if ( b & 1 ) { buf[len++] = note; }

					if ( b & 2 ) { buf[len++] = m->instr; }

					if ( b & 4 ) { buf[len++] = vol; }

					if ( b & 8 )
					{
						buf[len++] = command;
						buf[len++] = param;
					}
				}
			}

			buf[len++] = 0;
			dwPos += len;
			patinfo[0] += len;
			fwrite( buf, 1, len, f );
		}

		fseek( f, dwPatPos, SEEK_SET );
		patinfo[0] = bswapLE16( patinfo[0] ); // byteswap -- Toad
		fwrite( patinfo, 8, 1, f );
		fseek( f, dwPos, SEEK_SET );
	}

	// Writing Sample Data
	for ( UINT nsmp = 1; nsmp <= header.smpnum; nsmp++ )
	{
		MODINSTRUMENT* psmp = &Ins[nsmp];
		memset( &itss, 0, sizeof( itss ) );
		memcpy( itss.filename, psmp->name, 12 );
		memcpy( itss.name, m_szNames[nsmp], 26 );
		itss.id = 0x53504D49;
		itss.gvl = ( BYTE )psmp->nGlobalVol;

		if ( m_nInstruments )
		{
			for ( UINT iu = 1; iu <= m_nInstruments; iu++ ) if ( Headers[iu] )
				{
					INSTRUMENTHEADER* penv = Headers[iu];

					for ( UINT ju = 0; ju < 128; ju++ ) if ( penv->Keyboard[ju] == nsmp )
						{
							itss.flags = 0x01;
							break;
						}
				}
		}
		else
		{
			itss.flags = 0x01;
		}

		if ( psmp->uFlags & CHN_LOOP ) { itss.flags |= 0x10; }

		if ( psmp->uFlags & CHN_SUSTAINLOOP ) { itss.flags |= 0x20; }

		if ( psmp->uFlags & CHN_PINGPONGLOOP ) { itss.flags |= 0x40; }

		if ( psmp->uFlags & CHN_PINGPONGSUSTAIN ) { itss.flags |= 0x80; }

		itss.C5Speed = psmp->nC4Speed;

		if ( !itss.C5Speed ) // if no C5Speed assume it is XM Sample
		{
			UINT period;

			/**
			 * C5 note => number 61, but in XM samples:
			 * RealNote = Note + RelativeTone
			 */
			period = GetPeriodFromNote( 61 + psmp->RelativeTone, psmp->nFineTune, 0 );

			if ( period )
			{
				itss.C5Speed = GetFreqFromPeriod( period, 0, 0 );
			}

			/**
			 * If it didn`t work, it may not be a XM file;
			 * so put the default C5Speed, 8363Hz.
			 */
			if ( !itss.C5Speed ) { itss.C5Speed = 8363; }
		}

		itss.length = psmp->nLength;
		itss.loopbegin = psmp->nLoopStart;
		itss.loopend = psmp->nLoopEnd;
		itss.susloopbegin = psmp->nSustainStart;
		itss.susloopend = psmp->nSustainEnd;
		itss.vol = psmp->nVolume >> 2;
		itss.dfp = psmp->nPan >> 2;
		itss.vit = autovibxm2it[psmp->nVibType & 7];
		itss.vis = psmp->nVibRate;
		itss.vid = psmp->nVibDepth;
		itss.vir = ( psmp->nVibSweep < 64 ) ? psmp->nVibSweep * 4 : 255;

		if ( psmp->uFlags & CHN_PANNING ) { itss.dfp |= 0x80; }

		if ( ( psmp->pSample ) && ( psmp->nLength ) ) { itss.cvt = 0x01; }

		UINT flags = RS_PCM8S;
#ifndef NO_PACKING

		if ( nPacking )
		{
			if ( ( !( psmp->uFlags & ( CHN_16BIT | CHN_STEREO ) ) )
			     && ( CanPackSample( ( char* )psmp->pSample, psmp->nLength, nPacking ) ) )
			{
				flags = RS_ADPCM4;
				itss.cvt = 0xFF;
			}
		}
		else
#endif // NO_PACKING
		{
			if ( psmp->uFlags & CHN_STEREO )
			{
				flags = RS_STPCM8S;
				itss.flags |= 0x04;
			}

			if ( psmp->uFlags & CHN_16BIT )
			{
				itss.flags |= 0x02;
				flags = ( psmp->uFlags & CHN_STEREO ) ? RS_STPCM16S : RS_PCM16S;
			}
		}

		itss.samplepointer = dwPos;
		fseek( f, smppos[nsmp - 1], SEEK_SET );

		itss.id = bswapLE32( itss.id );
		itss.length = bswapLE32( itss.length );
		itss.loopbegin = bswapLE32( itss.loopbegin );
		itss.loopend = bswapLE32( itss.loopend );
		itss.C5Speed = bswapLE32( itss.C5Speed );
		itss.susloopbegin = bswapLE32( itss.susloopbegin );
		itss.susloopend = bswapLE32( itss.susloopend );
		itss.samplepointer = bswapLE32( itss.samplepointer );

		fwrite( &itss, 1, sizeof( ITSAMPLESTRUCT ), f );
		fseek( f, dwPos, SEEK_SET );

		if ( ( psmp->pSample ) && ( psmp->nLength ) )
		{
			dwPos += WriteSample( f, psmp, flags );
		}
	}

	// Updating offsets
	fseek( f, dwHdrPos, SEEK_SET );

	/* <Toad> Now we can byteswap them ;-) */
	UINT WW;
	UINT WX;
	WX = ( UINT )header.insnum;
	WX <<= 2;

	for ( WW = 0; WW < ( WX >> 2 ); WW++ )
	{
		inspos[WW] = bswapLE32( inspos[WW] );
	}

	WX = ( UINT )header.smpnum;
	WX <<= 2;

	for ( WW = 0; WW < ( WX >> 2 ); WW++ )
	{
		smppos[WW] = bswapLE32( smppos[WW] );
	}

	WX = ( UINT )header.patnum;
	WX <<= 2;

	for ( WW = 0; WW < ( WX >> 2 ); WW++ )
	{
		patpos[WW] = bswapLE32( patpos[WW] );
	}

	if ( header.insnum ) { fwrite( inspos, 4, header.insnum, f ); }

	if ( header.smpnum ) { fwrite( smppos, 4, header.smpnum, f ); }

	if ( header.patnum ) { fwrite( patpos, 4, header.patnum, f ); }

	fclose( f );
	return TRUE;
}

#ifdef _MSC_VER
//#pragma warning(default:4100)
#endif
#endif // MODPLUG_NO_FILESAVE

//////////////////////////////////////////////////////////////////////////////
// IT 2.14 compression

DWORD ITReadBits( DWORD& bitbuf, UINT& bitnum, LPBYTE& ibuf, CHAR n )
//-----------------------------------------------------------------
{
	DWORD retval = 0;
	UINT i = n;

	if ( n > 0 )
	{
		do
		{
			if ( !bitnum )
			{
				bitbuf = *ibuf++;
				bitnum = 8;
			}

			retval >>= 1;
			retval |= bitbuf << 31;
			bitbuf >>= 1;
			bitnum--;
			i--;
		}
		while ( i );

		i = n;
	}

	return ( retval >> ( 32 - i ) );
}

#define IT215_SUPPORT
void ITUnpack8Bit( signed char* pSample, DWORD dwLen, LPBYTE lpMemFile, DWORD dwMemLength, BOOL b215 )
//-------------------------------------------------------------------------------------------
{
	signed char* pDst = pSample;
	LPBYTE pSrc = lpMemFile;
	DWORD wHdr = 0;
	DWORD wCount = 0;
	DWORD bitbuf = 0;
	UINT bitnum = 0;
	BYTE bLeft = 0, bTemp = 0, bTemp2 = 0;

	while ( dwLen )
	{
		if ( !wCount )
		{
			wCount = 0x8000;
			wHdr = bswapLE16( *( ( LPWORD )pSrc ) );
			pSrc += 2;
			bLeft = 9;
			bTemp = bTemp2 = 0;
			bitbuf = bitnum = 0;
		}

		DWORD d = wCount;

		if ( d > dwLen ) { d = dwLen; }

		// Unpacking
		DWORD dwPos = 0;

		do
		{
			WORD wBits = ( WORD )ITReadBits( bitbuf, bitnum, pSrc, bLeft );

			if ( bLeft < 7 )
			{
				DWORD i = 1 << ( bLeft - 1 );
				DWORD j = wBits & 0xFFFF;

				if ( i != j ) { goto UnpackByte; }

				wBits = ( WORD )( ITReadBits( bitbuf, bitnum, pSrc, 3 ) + 1 ) & 0xFF;
				bLeft = ( ( BYTE )wBits < bLeft ) ? ( BYTE )wBits : ( BYTE )( ( wBits + 1 ) & 0xFF );
				goto Next;
			}

			if ( bLeft < 9 )
			{
				WORD i = ( 0xFF >> ( 9 - bLeft ) ) + 4;
				WORD j = i - 8;

				if ( ( wBits <= j ) || ( wBits > i ) ) { goto UnpackByte; }

				wBits -= j;
				bLeft = ( ( BYTE )( wBits & 0xFF ) < bLeft ) ? ( BYTE )( wBits & 0xFF ) : ( BYTE )( ( wBits + 1 ) & 0xFF );
				goto Next;
			}

			if ( bLeft >= 10 ) { goto SkipByte; }

			if ( wBits >= 256 )
			{
				bLeft = ( BYTE )( wBits + 1 ) & 0xFF;
				goto Next;
			}

UnpackByte:

			if ( bLeft < 8 )
			{
				BYTE shift = 8 - bLeft;
				signed char c = ( signed char )( wBits << shift );
				c >>= shift;
				wBits = ( WORD )c;
			}

			wBits += bTemp;
			bTemp = ( BYTE )wBits;
			bTemp2 += bTemp;
#ifdef IT215_SUPPORT
			pDst[dwPos] = ( b215 ) ? bTemp2 : bTemp;
#else
			pDst[dwPos] = bTemp;
#endif
SkipByte:
			dwPos++;
Next:

			if ( pSrc >= lpMemFile + dwMemLength + 1 ) { return; }
		}
		while ( dwPos < d );

		// Move On
		wCount -= d;
		dwLen -= d;
		pDst += d;
	}
}


void ITUnpack16Bit( signed char* pSample, DWORD dwLen, LPBYTE lpMemFile, DWORD dwMemLength, BOOL b215 )
//--------------------------------------------------------------------------------------------
{
	signed short* pDst = ( signed short* )pSample;
	LPBYTE pSrc = lpMemFile;
	DWORD wHdr = 0;
	DWORD wCount = 0;
	DWORD bitbuf = 0;
	UINT bitnum = 0;
	BYTE bLeft = 0;
	signed short wTemp = 0, wTemp2 = 0;

	while ( dwLen )
	{
		if ( !wCount )
		{
			wCount = 0x4000;
			wHdr = bswapLE16( *( ( LPWORD )pSrc ) );
			pSrc += 2;
			bLeft = 17;
			wTemp = wTemp2 = 0;
			bitbuf = bitnum = 0;
		}

		DWORD d = wCount;

		if ( d > dwLen ) { d = dwLen; }

		// Unpacking
		DWORD dwPos = 0;

		do
		{
			DWORD dwBits = ITReadBits( bitbuf, bitnum, pSrc, bLeft );

			if ( bLeft < 7 )
			{
				DWORD i = 1 << ( bLeft - 1 );
				DWORD j = dwBits;

				if ( i != j ) { goto UnpackByte; }

				dwBits = ITReadBits( bitbuf, bitnum, pSrc, 4 ) + 1;
				bLeft = ( ( BYTE )( dwBits & 0xFF ) < bLeft ) ? ( BYTE )( dwBits & 0xFF ) : ( BYTE )( ( dwBits + 1 ) & 0xFF );
				goto Next;
			}

			if ( bLeft < 17 )
			{
				DWORD i = ( 0xFFFF >> ( 17 - bLeft ) ) + 8;
				DWORD j = ( i - 16 ) & 0xFFFF;

				if ( ( dwBits <= j ) || ( dwBits > ( i & 0xFFFF ) ) ) { goto UnpackByte; }

				dwBits -= j;
				bLeft = ( ( BYTE )( dwBits & 0xFF ) < bLeft ) ? ( BYTE )( dwBits & 0xFF ) : ( BYTE )( ( dwBits + 1 ) & 0xFF );
				goto Next;
			}

			if ( bLeft >= 18 ) { goto SkipByte; }

			if ( dwBits >= 0x10000 )
			{
				bLeft = ( BYTE )( dwBits + 1 ) & 0xFF;
				goto Next;
			}

UnpackByte:

			if ( bLeft < 16 )
			{
				BYTE shift = 16 - bLeft;
				signed short c = ( signed short )( dwBits << shift );
				c >>= shift;
				dwBits = ( DWORD )c;
			}

			dwBits += wTemp;
			wTemp = ( signed short )dwBits;
			wTemp2 += wTemp;
#ifdef IT215_SUPPORT
			pDst[dwPos] = ( b215 ) ? wTemp2 : wTemp;
#else
			pDst[dwPos] = wTemp;
#endif
SkipByte:
			dwPos++;
Next:

			if ( pSrc >= lpMemFile + dwMemLength + 1 ) { return; }
		}
		while ( dwPos < d );

		// Move On
		wCount -= d;
		dwLen -= d;
		pDst += d;

		if ( pSrc >= lpMemFile + dwMemLength ) { break; }
	}
}


UINT CSoundFile::SaveMixPlugins( FILE* f, BOOL bUpdate )
//----------------------------------------------------
{
	DWORD chinfo[64];
	CHAR s[32];
	DWORD nPluginSize, writeSwapDWORD;
	SNDMIXPLUGININFO writePluginInfo;
	UINT nTotalSize = 0;
	UINT nChInfo = 0;

	for ( UINT i = 0; i < MAX_MIXPLUGINS; i++ )
	{
		PSNDMIXPLUGIN p = &m_MixPlugins[i];

		if ( ( p->Info.dwPluginId1 ) || ( p->Info.dwPluginId2 ) )
		{
			nPluginSize = sizeof( SNDMIXPLUGININFO ) + 4; // plugininfo+4 (datalen)

			if ( ( p->pMixPlugin ) && ( bUpdate ) )
			{
				p->pMixPlugin->SaveAllParameters();
			}

			if ( p->pPluginData )
			{
				nPluginSize += p->nPluginDataSize;
			}

			if ( f )
			{
				s[0] = 'F';
				s[1] = 'X';
				s[2] = '0' + ( i / 10 );
				s[3] = '0' + ( i % 10 );
				fwrite( s, 1, 4, f );
				writeSwapDWORD = bswapLE32( nPluginSize );
				fwrite( &writeSwapDWORD, 1, 4, f );

				// Copy Information To Be Written for ByteSwapping
				memcpy( &writePluginInfo, &p->Info, sizeof( SNDMIXPLUGININFO ) );
				writePluginInfo.dwPluginId1 = bswapLE32( p->Info.dwPluginId1 );
				writePluginInfo.dwPluginId2 = bswapLE32( p->Info.dwPluginId2 );
				writePluginInfo.dwInputRouting = bswapLE32( p->Info.dwInputRouting );
				writePluginInfo.dwOutputRouting = bswapLE32( p->Info.dwOutputRouting );

				for ( UINT j = 0; j < 4; j++ )
				{
					writePluginInfo.dwReserved[j] = bswapLE32( p->Info.dwReserved[j] );
				}

				fwrite( &writePluginInfo, 1, sizeof( SNDMIXPLUGININFO ), f );
				writeSwapDWORD = bswapLE32( m_MixPlugins[i].nPluginDataSize );
				fwrite( &writeSwapDWORD, 1, 4, f );

				if ( m_MixPlugins[i].pPluginData )
				{
					fwrite( m_MixPlugins[i].pPluginData, 1, m_MixPlugins[i].nPluginDataSize, f );
				}
			}

			nTotalSize += nPluginSize + 8;
		}
	}

	for ( UINT j = 0; j < m_nChannels; j++ )
	{
		if ( j < 64 )
		{
			if ( ( chinfo[j] = ChnSettings[j].nMixPlugin ) != 0 )
			{
				nChInfo = j + 1;
				chinfo[j] = bswapLE32( chinfo[j] ); // inplace BS
			}
		}
	}

	if ( nChInfo )
	{
		if ( f )
		{
			nPluginSize = bswapLE32( 0x58464843 );
			fwrite( &nPluginSize, 1, 4, f );
			nPluginSize = nChInfo * 4;
			writeSwapDWORD = bswapLE32( nPluginSize );
			fwrite( &writeSwapDWORD, 1, 4, f );
			fwrite( chinfo, 1, nPluginSize, f );
		}

		nTotalSize += nChInfo * 4 + 8;
	}

	return nTotalSize;
}


UINT CSoundFile::LoadMixPlugins( const void* pData, UINT nLen )
//-----------------------------------------------------------
{
	const BYTE* p = ( const BYTE* )pData;
	UINT nPos = 0;

	while ( nPos + 8 < nLen )
	{
		DWORD nPluginSize;
		UINT nPlugin;

		nPluginSize = bswapLE32( *( DWORD* )( p + nPos + 4 ) );

		if ( nPluginSize > nLen - nPos - 8 ) { break; };

		if ( ( bswapLE32( *( DWORD* )( p + nPos ) ) ) == 0x58464843 )
		{
			for ( UINT ch = 0; ch < 64; ch++ ) if ( ch * 4 < nPluginSize )
				{
					ChnSettings[ch].nMixPlugin = bswapLE32( *( DWORD* )( p + nPos + 8 + ch * 4 ) );
				}
		}
		else
		{
			if ( ( p[nPos] != 'F' ) || ( p[nPos + 1] != 'X' )
			     || ( p[nPos + 2] < '0' ) || ( p[nPos + 3] < '0' ) )
			{
				break;
			}

			nPlugin = ( p[nPos + 2] - '0' ) * 10 + ( p[nPos + 3] - '0' );

			if ( ( nPlugin < MAX_MIXPLUGINS ) && ( nPluginSize >= sizeof( SNDMIXPLUGININFO ) + 4 ) )
			{
				DWORD dwExtra = bswapLE32( *( DWORD* )( p + nPos + 8 + sizeof( SNDMIXPLUGININFO ) ) );
				m_MixPlugins[nPlugin].Info = *( const SNDMIXPLUGININFO* )( p + nPos + 8 );
				m_MixPlugins[nPlugin].Info.dwPluginId1 = bswapLE32( m_MixPlugins[nPlugin].Info.dwPluginId1 );
				m_MixPlugins[nPlugin].Info.dwPluginId2 = bswapLE32( m_MixPlugins[nPlugin].Info.dwPluginId2 );
				m_MixPlugins[nPlugin].Info.dwInputRouting = bswapLE32( m_MixPlugins[nPlugin].Info.dwInputRouting );
				m_MixPlugins[nPlugin].Info.dwOutputRouting = bswapLE32( m_MixPlugins[nPlugin].Info.dwOutputRouting );

				for ( UINT j = 0; j < 4; j++ )
				{
					m_MixPlugins[nPlugin].Info.dwReserved[j] = bswapLE32( m_MixPlugins[nPlugin].Info.dwReserved[j] );
				}

				if ( ( dwExtra ) && ( dwExtra <= nPluginSize - sizeof( SNDMIXPLUGININFO ) - 4 ) )
				{
					m_MixPlugins[nPlugin].nPluginDataSize = 0;
					m_MixPlugins[nPlugin].pPluginData = new signed char [dwExtra];

					if ( m_MixPlugins[nPlugin].pPluginData )
					{
						m_MixPlugins[nPlugin].nPluginDataSize = dwExtra;
						memcpy( m_MixPlugins[nPlugin].pPluginData, p + nPos + 8 + sizeof( SNDMIXPLUGININFO ) + 4, dwExtra );
					}
				}
			}
		}

		nPos += nPluginSize + 8;
	}

	return nPos;
}

////////////////////////////////////////////////////////
// FastTracker II XM file support

#ifdef MSC_VER
#pragma warning(disable:4244)
#endif

#pragma pack(1)
typedef struct tagXMFILEHEADER
{
	DWORD size;
	WORD norder;
	WORD restartpos;
	WORD channels;
	WORD patterns;
	WORD instruments;
	WORD flags;
	WORD speed;
	WORD tempo;
	BYTE order[256];
} XMFILEHEADER;


typedef struct tagXMINSTRUMENTHEADER
{
	DWORD size;
	CHAR name[22];
	BYTE type;
	BYTE samples;
	BYTE samplesh;
} XMINSTRUMENTHEADER;


typedef struct tagXMSAMPLEHEADER
{
	DWORD shsize;
	BYTE snum[96];
	WORD venv[24];
	WORD penv[24];
	BYTE vnum, pnum;
	BYTE vsustain, vloops, vloope, psustain, ploops, ploope;
	BYTE vtype, ptype;
	BYTE vibtype, vibsweep, vibdepth, vibrate;
	WORD volfade;
	WORD res;
	BYTE reserved1[20];
} XMSAMPLEHEADER;

typedef struct tagXMSAMPLESTRUCT
{
	DWORD samplen;
	DWORD loopstart;
	DWORD looplen;
	BYTE vol;
	signed char finetune;
	BYTE type;
	BYTE pan;
	signed char relnote;
	BYTE res;
	char name[22];
} XMSAMPLESTRUCT;
#pragma pack()


BOOL CSoundFile::ReadXM( const BYTE* lpStream, DWORD dwMemLength )
//--------------------------------------------------------------
{
	XMSAMPLEHEADER xmsh;
	XMSAMPLESTRUCT xmss;
	DWORD dwMemPos, dwHdrSize;
	WORD norders = 0, restartpos = 0, channels = 0, patterns = 0, instruments = 0;
	WORD xmflags = 0, deftempo = 125, defspeed = 6;
	BOOL InstUsed[256];
	BYTE channels_used[MAX_CHANNELS];
	BYTE pattern_map[256];
	BOOL samples_used[MAX_SAMPLES];
	UINT unused_samples;

	m_nChannels = 0;

	if ( ( !lpStream ) || ( dwMemLength < 0x200 ) ) { return FALSE; }

	if ( strnicmp( ( LPCSTR )lpStream, "Extended Module", 15 ) ) { return FALSE; }

	memcpy( m_szNames[0], lpStream + 17, 20 );
	dwHdrSize = bswapLE32( *( ( DWORD* )( lpStream + 60 ) ) );
	norders = bswapLE16( *( ( WORD* )( lpStream + 64 ) ) );

	if ( ( !norders ) || ( norders > MAX_ORDERS ) ) { return FALSE; }

	restartpos = bswapLE16( *( ( WORD* )( lpStream + 66 ) ) );
	channels = bswapLE16( *( ( WORD* )( lpStream + 68 ) ) );

	if ( ( !channels ) || ( channels > 64 ) ) { return FALSE; }

	m_nType = MOD_TYPE_XM;
	m_nMinPeriod = 27;
	m_nMaxPeriod = 54784;
	m_nChannels = channels;

	if ( restartpos < norders ) { m_nRestartPos = restartpos; }

	patterns = bswapLE16( *( ( WORD* )( lpStream + 70 ) ) );

	if ( patterns > 256 ) { patterns = 256; }

	instruments = bswapLE16( *( ( WORD* )( lpStream + 72 ) ) );

	if ( instruments >= MAX_INSTRUMENTS ) { instruments = MAX_INSTRUMENTS - 1; }

	m_nInstruments = instruments;
	m_nSamples = 0;
	memcpy( &xmflags, lpStream + 74, 2 );
	xmflags = bswapLE16( xmflags );

	if ( xmflags & 1 ) { m_dwSongFlags |= SONG_LINEARSLIDES; }

	if ( xmflags & 0x1000 ) { m_dwSongFlags |= SONG_EXFILTERRANGE; }

	defspeed = bswapLE16( *( ( WORD* )( lpStream + 76 ) ) );
	deftempo = bswapLE16( *( ( WORD* )( lpStream + 78 ) ) );

	if ( ( deftempo >= 32 ) && ( deftempo < 256 ) ) { m_nDefaultTempo = deftempo; }

	if ( ( defspeed > 0 ) && ( defspeed < 40 ) ) { m_nDefaultSpeed = defspeed; }

	memcpy( Order, lpStream + 80, norders );
	memset( InstUsed, 0, sizeof( InstUsed ) );

	if ( patterns > MAX_PATTERNS )
	{
		UINT i, j;

		for ( i = 0; i < norders; i++ )
		{
			if ( Order[i] < patterns ) { InstUsed[Order[i]] = TRUE; }
		}

		j = 0;

		for ( i = 0; i < 256; i++ )
		{
			if ( InstUsed[i] ) { pattern_map[i] = j++; }
		}

		for ( i = 0; i < 256; i++ )
		{
			if ( !InstUsed[i] )
			{
				pattern_map[i] = ( j < MAX_PATTERNS ) ? j : 0xFE;
				j++;
			}
		}

		for ( i = 0; i < norders; i++ )
		{
			Order[i] = pattern_map[Order[i]];
		}
	}
	else
	{
		for ( UINT i = 0; i < 256; i++ ) { pattern_map[i] = i; }
	}

	memset( InstUsed, 0, sizeof( InstUsed ) );
	dwMemPos = dwHdrSize + 60;

	if ( dwMemPos + 8 >= dwMemLength ) { return TRUE; }

	// Reading patterns
	memset( channels_used, 0, sizeof( channels_used ) );

	for ( UINT ipat = 0; ipat < patterns; ipat++ )
	{
		UINT ipatmap = pattern_map[ipat];
		DWORD dwSize = 0;
		WORD rows = 64, packsize = 0;
		dwSize = bswapLE32( *( ( DWORD* )( lpStream + dwMemPos ) ) );

		while ( ( dwMemPos + dwSize >= dwMemLength ) || ( dwSize & 0xFFFFFF00 ) )
		{
			if ( dwMemPos + 4 >= dwMemLength ) { break; }

			dwMemPos++;
			dwSize = bswapLE32( *( ( DWORD* )( lpStream + dwMemPos ) ) );
		}

		rows = bswapLE16( *( ( WORD* )( lpStream + dwMemPos + 5 ) ) );

		if ( ( !rows ) || ( rows > 256 ) ) { rows = 64; }

		packsize = bswapLE16( *( ( WORD* )( lpStream + dwMemPos + 7 ) ) );

		if ( dwMemPos + dwSize + 4 > dwMemLength ) { return TRUE; }

		dwMemPos += dwSize;

		if ( dwMemPos + packsize + 4 > dwMemLength ) { return TRUE; }

		MODCOMMAND* p;

		if ( ipatmap < MAX_PATTERNS )
		{
			PatternSize[ipatmap] = rows;

			if ( ( Patterns[ipatmap] = AllocatePattern( rows, m_nChannels ) ) == NULL ) { return TRUE; }

			if ( !packsize ) { continue; }

			p = Patterns[ipatmap];
		}
		else { p = NULL; }

		const BYTE* src = lpStream + dwMemPos;
		UINT j = 0;

		for ( UINT row = 0; row < rows; row++ )
		{
			for ( UINT chn = 0; chn < m_nChannels; chn++ )
			{
				if ( ( p ) && ( j < packsize ) )
				{
					BYTE b = src[j++];
					UINT vol = 0;

					if ( b & 0x80 )
					{
						if ( b & 1 ) { p->note = src[j++]; }

						if ( b & 2 ) { p->instr = src[j++]; }

						if ( b & 4 ) { vol = src[j++]; }

						if ( b & 8 ) { p->command = src[j++]; }

						if ( b & 16 ) { p->param = src[j++]; }
					}
					else
					{
						p->note = b;
						p->instr = src[j++];
						vol = src[j++];
						p->command = src[j++];
						p->param = src[j++];
					}

					if ( p->note == 97 ) { p->note = 0xFF; }
					else if ( ( p->note ) && ( p->note < 97 ) ) { p->note += 12; }

					if ( p->note ) { channels_used[chn] = 1; }

					if ( p->command | p->param ) { ConvertModCommand( p ); }

					if ( p->instr == 0xff ) { p->instr = 0; }

					if ( p->instr ) { InstUsed[p->instr] = TRUE; }

					if ( ( vol >= 0x10 ) && ( vol <= 0x50 ) )
					{
						p->volcmd = VOLCMD_VOLUME;
						p->vol = vol - 0x10;
					}
					else if ( vol >= 0x60 )
					{
						UINT v = vol & 0xF0;
						vol &= 0x0F;
						p->vol = vol;

						switch ( v )
						{
								// 60-6F: Volume Slide Down
							case 0x60:
								p->volcmd = VOLCMD_VOLSLIDEDOWN;
								break;

								// 70-7F: Volume Slide Up:
							case 0x70:
								p->volcmd = VOLCMD_VOLSLIDEUP;
								break;

								// 80-8F: Fine Volume Slide Down
							case 0x80:
								p->volcmd = VOLCMD_FINEVOLDOWN;
								break;

								// 90-9F: Fine Volume Slide Up
							case 0x90:
								p->volcmd = VOLCMD_FINEVOLUP;
								break;

								// A0-AF: Set Vibrato Speed
							case 0xA0:
								p->volcmd = VOLCMD_VIBRATOSPEED;
								break;

								// B0-BF: Vibrato
							case 0xB0:
								p->volcmd = VOLCMD_VIBRATO;
								break;

								// C0-CF: Set Panning
							case 0xC0:
								p->volcmd = VOLCMD_PANNING;
								p->vol = ( vol << 2 ) + 2;
								break;

								// D0-DF: Panning Slide Left
							case 0xD0:
								p->volcmd = VOLCMD_PANSLIDELEFT;
								break;

								// E0-EF: Panning Slide Right
							case 0xE0:
								p->volcmd = VOLCMD_PANSLIDERIGHT;
								break;

								// F0-FF: Tone Portamento
							case 0xF0:
								p->volcmd = VOLCMD_TONEPORTAMENTO;
								break;
						}
					}

					p++;
				}
				else if ( j < packsize )
				{
					BYTE b = src[j++];

					if ( b & 0x80 )
					{
						if ( b & 1 ) { j++; }

						if ( b & 2 ) { j++; }

						if ( b & 4 ) { j++; }

						if ( b & 8 ) { j++; }

						if ( b & 16 ) { j++; }
					}
					else { j += 4; }
				}
				else { break; }
			}
		}

		dwMemPos += packsize;
	}

	// Wrong offset check
	while ( dwMemPos + 4 < dwMemLength )
	{
		DWORD d = bswapLE32( *( ( DWORD* )( lpStream + dwMemPos ) ) );

		if ( d < 0x300 ) { break; }

		dwMemPos++;
	}

	memset( samples_used, 0, sizeof( samples_used ) );
	unused_samples = 0;

	// Reading instruments
	for ( UINT iIns = 1; iIns <= instruments; iIns++ )
	{
		XMINSTRUMENTHEADER* pih;
		BYTE flags[32];
		DWORD samplesize[32];
		UINT samplemap[32];
		WORD nsamples;

		if ( dwMemPos + sizeof( XMINSTRUMENTHEADER ) >= dwMemLength ) { return TRUE; }

		pih = ( XMINSTRUMENTHEADER* )( lpStream + dwMemPos );

		if ( dwMemPos + bswapLE32( pih->size ) > dwMemLength ) { return TRUE; }

		if ( ( Headers[iIns] = new INSTRUMENTHEADER ) == NULL ) { continue; }

		memset( Headers[iIns], 0, sizeof( INSTRUMENTHEADER ) );
		memcpy( Headers[iIns]->name, pih->name, 22 );

		if ( ( nsamples = pih->samples ) > 0 )
		{
			if ( dwMemPos + sizeof( XMSAMPLEHEADER ) > dwMemLength ) { return TRUE; }

			memcpy( &xmsh, lpStream + dwMemPos + sizeof( XMINSTRUMENTHEADER ), sizeof( XMSAMPLEHEADER ) );
			xmsh.shsize = bswapLE32( xmsh.shsize );

			for ( int i = 0; i < 24; ++i )
			{
				xmsh.venv[i] = bswapLE16( xmsh.venv[i] );
				xmsh.penv[i] = bswapLE16( xmsh.penv[i] );
			}

			xmsh.volfade = bswapLE16( xmsh.volfade );
			xmsh.res = bswapLE16( xmsh.res );
			dwMemPos += bswapLE32( pih->size );
		}
		else
		{
			if ( bswapLE32( pih->size ) ) { dwMemPos += bswapLE32( pih->size ); }
			else { dwMemPos += sizeof( XMINSTRUMENTHEADER ); }

			continue;
		}

		memset( samplemap, 0, sizeof( samplemap ) );

		if ( nsamples > 32 ) { return TRUE; }

		UINT newsamples = m_nSamples;

		for ( UINT nmap = 0; nmap < nsamples; nmap++ )
		{
			UINT n = m_nSamples + nmap + 1;

			if ( n >= MAX_SAMPLES )
			{
				n = m_nSamples;

				while ( n > 0 )
				{
					if ( !Ins[n].pSample )
					{
						for ( UINT xmapchk = 0; xmapchk < nmap; xmapchk++ )
						{
							if ( samplemap[xmapchk] == n ) { goto alreadymapped; }
						}

						for ( UINT clrs = 1; clrs < iIns; clrs++ ) if ( Headers[clrs] )
							{
								INSTRUMENTHEADER* pks = Headers[clrs];

								for ( UINT ks = 0; ks < 128; ks++ )
								{
									if ( pks->Keyboard[ks] == n ) { pks->Keyboard[ks] = 0; }
								}
							}

						break;
					}

alreadymapped:
					n--;
				}

#ifndef MODPLUG_FASTSOUNDLIB

				// Damn! more than 200 samples: look for duplicates
				if ( !n )
				{
					if ( !unused_samples )
					{
						unused_samples = DetectUnusedSamples( samples_used );

						if ( !unused_samples ) { unused_samples = 0xFFFF; }
					}

					if ( ( unused_samples ) && ( unused_samples != 0xFFFF ) )
					{
						for ( UINT iext = m_nSamples; iext >= 1; iext-- ) if ( !samples_used[iext] )
							{
								unused_samples--;
								samples_used[iext] = TRUE;
								DestroySample( iext );
								n = iext;

								for ( UINT mapchk = 0; mapchk < nmap; mapchk++ )
								{
									if ( samplemap[mapchk] == n ) { samplemap[mapchk] = 0; }
								}

								for ( UINT clrs = 1; clrs < iIns; clrs++ ) if ( Headers[clrs] )
									{
										INSTRUMENTHEADER* pks = Headers[clrs];

										for ( UINT ks = 0; ks < 128; ks++ )
										{
											if ( pks->Keyboard[ks] == n ) { pks->Keyboard[ks] = 0; }
										}
									}

								memset( &Ins[n], 0, sizeof( Ins[0] ) );
								break;
							}
					}
				}

#endif // MODPLUG_FASTSOUNDLIB
			}

			if ( newsamples < n ) { newsamples = n; }

			samplemap[nmap] = n;
		}

		m_nSamples = newsamples;
		// Reading Volume Envelope
		INSTRUMENTHEADER* penv = Headers[iIns];
		penv->nMidiProgram = pih->type;
		penv->nFadeOut = xmsh.volfade;
		penv->nPan = 128;
		penv->nPPC = 5 * 12;

		if ( xmsh.vtype & 1 ) { penv->dwFlags |= ENV_VOLUME; }

		if ( xmsh.vtype & 2 ) { penv->dwFlags |= ENV_VOLSUSTAIN; }

		if ( xmsh.vtype & 4 ) { penv->dwFlags |= ENV_VOLLOOP; }

		if ( xmsh.ptype & 1 ) { penv->dwFlags |= ENV_PANNING; }

		if ( xmsh.ptype & 2 ) { penv->dwFlags |= ENV_PANSUSTAIN; }

		if ( xmsh.ptype & 4 ) { penv->dwFlags |= ENV_PANLOOP; }

		if ( xmsh.vnum > 12 ) { xmsh.vnum = 12; }

		if ( xmsh.pnum > 12 ) { xmsh.pnum = 12; }

		penv->nVolEnv = xmsh.vnum;

		if ( !xmsh.vnum ) { penv->dwFlags &= ~ENV_VOLUME; }

		if ( !xmsh.pnum ) { penv->dwFlags &= ~ENV_PANNING; }

		penv->nPanEnv = xmsh.pnum;
		penv->nVolSustainBegin = penv->nVolSustainEnd = xmsh.vsustain;

		if ( xmsh.vsustain >= 12 ) { penv->dwFlags &= ~ENV_VOLSUSTAIN; }

		penv->nVolLoopStart = xmsh.vloops;
		penv->nVolLoopEnd = xmsh.vloope;

		if ( penv->nVolLoopEnd >= 12 ) { penv->nVolLoopEnd = 0; }

		if ( penv->nVolLoopStart >= penv->nVolLoopEnd ) { penv->dwFlags &= ~ENV_VOLLOOP; }

		penv->nPanSustainBegin = penv->nPanSustainEnd = xmsh.psustain;

		if ( xmsh.psustain >= 12 ) { penv->dwFlags &= ~ENV_PANSUSTAIN; }

		penv->nPanLoopStart = xmsh.ploops;
		penv->nPanLoopEnd = xmsh.ploope;

		if ( penv->nPanLoopEnd >= 12 ) { penv->nPanLoopEnd = 0; }

		if ( penv->nPanLoopStart >= penv->nPanLoopEnd ) { penv->dwFlags &= ~ENV_PANLOOP; }

		penv->nGlobalVol = 64;

		for ( UINT ienv = 0; ienv < 12; ienv++ )
		{
			penv->VolPoints[ienv] = ( WORD )xmsh.venv[ienv * 2];
			penv->VolEnv[ienv] = ( BYTE )xmsh.venv[ienv * 2 + 1];
			penv->PanPoints[ienv] = ( WORD )xmsh.penv[ienv * 2];
			penv->PanEnv[ienv] = ( BYTE )xmsh.penv[ienv * 2 + 1];

			if ( ienv )
			{
				if ( penv->VolPoints[ienv] < penv->VolPoints[ienv - 1] )
				{
					penv->VolPoints[ienv] &= 0xFF;
					penv->VolPoints[ienv] += penv->VolPoints[ienv - 1] & 0xFF00;

					if ( penv->VolPoints[ienv] < penv->VolPoints[ienv - 1] ) { penv->VolPoints[ienv] += 0x100; }
				}

				if ( penv->PanPoints[ienv] < penv->PanPoints[ienv - 1] )
				{
					penv->PanPoints[ienv] &= 0xFF;
					penv->PanPoints[ienv] += penv->PanPoints[ienv - 1] & 0xFF00;

					if ( penv->PanPoints[ienv] < penv->PanPoints[ienv - 1] ) { penv->PanPoints[ienv] += 0x100; }
				}
			}
		}

		for ( UINT j = 0; j < 96; j++ )
		{
			penv->NoteMap[j + 12] = j + 1 + 12;

			if ( xmsh.snum[j] < nsamples )
			{
				penv->Keyboard[j + 12] = samplemap[xmsh.snum[j]];
			}
		}

		// Reading samples
		for ( UINT ins = 0; ins < nsamples; ins++ )
		{
			if ( ( dwMemPos + sizeof( xmss ) > dwMemLength )
			     || ( dwMemPos + xmsh.shsize > dwMemLength ) ) { return TRUE; }

			memcpy( &xmss, lpStream + dwMemPos, sizeof( xmss ) );
			xmss.samplen = bswapLE32( xmss.samplen );
			xmss.loopstart = bswapLE32( xmss.loopstart );
			xmss.looplen = bswapLE32( xmss.looplen );
			dwMemPos += xmsh.shsize;
			flags[ins] = ( xmss.type & 0x10 ) ? RS_PCM16D : RS_PCM8D;

			if ( xmss.type & 0x20 ) { flags[ins] = ( xmss.type & 0x10 ) ? RS_STPCM16D : RS_STPCM8D; }

			samplesize[ins] = xmss.samplen;

			if ( !samplemap[ins] ) { continue; }

			if ( xmss.type & 0x10 )
			{
				xmss.looplen >>= 1;
				xmss.loopstart >>= 1;
				xmss.samplen >>= 1;
			}

			if ( xmss.type & 0x20 )
			{
				xmss.looplen >>= 1;
				xmss.loopstart >>= 1;
				xmss.samplen >>= 1;
			}

			if ( xmss.samplen > MAX_SAMPLE_LENGTH ) { xmss.samplen = MAX_SAMPLE_LENGTH; }

			if ( xmss.loopstart >= xmss.samplen ) { xmss.type &= ~3; }

			xmss.looplen += xmss.loopstart;

			if ( xmss.looplen > xmss.samplen ) { xmss.looplen = xmss.samplen; }

			if ( !xmss.looplen ) { xmss.type &= ~3; }

			UINT imapsmp = samplemap[ins];
			memcpy( m_szNames[imapsmp], xmss.name, 22 );
			m_szNames[imapsmp][22] = 0;
			MODINSTRUMENT* pins = &Ins[imapsmp];
			pins->nLength = ( xmss.samplen > MAX_SAMPLE_LENGTH ) ? MAX_SAMPLE_LENGTH : xmss.samplen;
			pins->nLoopStart = xmss.loopstart;
			pins->nLoopEnd = xmss.looplen;

			if ( pins->nLoopEnd > pins->nLength ) { pins->nLoopEnd = pins->nLength; }

			if ( pins->nLoopStart >= pins->nLoopEnd )
			{
				pins->nLoopStart = pins->nLoopEnd = 0;
			}

			if ( xmss.type & 3 ) { pins->uFlags |= CHN_LOOP; }

			if ( xmss.type & 2 ) { pins->uFlags |= CHN_PINGPONGLOOP; }

			pins->nVolume = xmss.vol << 2;

			if ( pins->nVolume > 256 ) { pins->nVolume = 256; }

			pins->nGlobalVol = 64;

			if ( ( xmss.res == 0xAD ) && ( !( xmss.type & 0x30 ) ) )
			{
				flags[ins] = RS_ADPCM4;
				samplesize[ins] = ( samplesize[ins] + 1 ) / 2 + 16;
			}

			pins->nFineTune = xmss.finetune;
			pins->RelativeTone = ( int )xmss.relnote;
			pins->nPan = xmss.pan;
			pins->uFlags |= CHN_PANNING;
			pins->nVibType = xmsh.vibtype;
			pins->nVibSweep = xmsh.vibsweep;
			pins->nVibDepth = xmsh.vibdepth;
			pins->nVibRate = xmsh.vibrate;
			memcpy( pins->name, xmss.name, 22 );
			pins->name[21] = 0;
		}

#if 0

		if ( ( xmsh.reserved2 > nsamples ) && ( xmsh.reserved2 <= 16 ) )
		{
			dwMemPos += ( ( ( UINT )xmsh.reserved2 ) - nsamples ) * xmsh.shsize;
		}

#endif

		for ( UINT ismpd = 0; ismpd < nsamples; ismpd++ )
		{
			if ( ( samplemap[ismpd] ) && ( samplesize[ismpd] ) && ( dwMemPos < dwMemLength ) )
			{
				ReadSample( &Ins[samplemap[ismpd]], flags[ismpd], ( LPSTR )( lpStream + dwMemPos ), dwMemLength - dwMemPos );
			}

			dwMemPos += samplesize[ismpd];

			if ( dwMemPos >= dwMemLength ) { break; }
		}
	}

	// Read song comments: "TEXT"
	if ( ( dwMemPos + 8 < dwMemLength ) && ( bswapLE32( *( ( DWORD* )( lpStream + dwMemPos ) ) ) == 0x74786574 ) )
	{
		UINT len = *( ( DWORD* )( lpStream + dwMemPos + 4 ) );
		dwMemPos += 8;

		if ( ( dwMemPos + len <= dwMemLength ) && ( len < 16384 ) )
		{
			m_lpszSongComments = new char[len + 1];

			if ( m_lpszSongComments )
			{
				memcpy( m_lpszSongComments, lpStream + dwMemPos, len );
				m_lpszSongComments[len] = 0;
			}

			dwMemPos += len;
		}
	}

	// Read midi config: "MIDI"
	if ( ( dwMemPos + 8 < dwMemLength ) && ( bswapLE32( *( ( DWORD* )( lpStream + dwMemPos ) ) ) == 0x4944494D ) )
	{
		UINT len = *( ( DWORD* )( lpStream + dwMemPos + 4 ) );
		dwMemPos += 8;

		if ( len == sizeof( MODMIDICFG ) )
		{
			memcpy( &m_MidiCfg, lpStream + dwMemPos, len );
			m_dwSongFlags |= SONG_EMBEDMIDICFG;
		}
	}

	// Read pattern names: "PNAM"
	if ( ( dwMemPos + 8 < dwMemLength ) && ( bswapLE32( *( ( DWORD* )( lpStream + dwMemPos ) ) ) == 0x4d414e50 ) )
	{
		UINT len = *( ( DWORD* )( lpStream + dwMemPos + 4 ) );
		dwMemPos += 8;

		if ( ( dwMemPos + len <= dwMemLength ) && ( len <= MAX_PATTERNS * MAX_PATTERNNAME ) && ( len >= MAX_PATTERNNAME ) )
		{
			m_lpszPatternNames = new char[len];

			if ( m_lpszPatternNames )
			{
				m_nPatternNames = len / MAX_PATTERNNAME;
				memcpy( m_lpszPatternNames, lpStream + dwMemPos, len );
			}

			dwMemPos += len;
		}
	}

	// Read channel names: "CNAM"
	if ( ( dwMemPos + 8 < dwMemLength ) && ( bswapLE32( *( ( DWORD* )( lpStream + dwMemPos ) ) ) == 0x4d414e43 ) )
	{
		UINT len = *( ( DWORD* )( lpStream + dwMemPos + 4 ) );
		dwMemPos += 8;

		if ( ( dwMemPos + len <= dwMemLength ) && ( len <= MAX_BASECHANNELS * MAX_CHANNELNAME ) )
		{
			UINT n = len / MAX_CHANNELNAME;

			for ( UINT i = 0; i < n; i++ )
			{
				memcpy( ChnSettings[i].szName, ( lpStream + dwMemPos + i * MAX_CHANNELNAME ), MAX_CHANNELNAME );
				ChnSettings[i].szName[MAX_CHANNELNAME - 1] = 0;
			}

			dwMemPos += len;
		}
	}

	// Read mix plugins information
	if ( dwMemPos + 8 < dwMemLength )
	{
		dwMemPos += LoadMixPlugins( lpStream + dwMemPos, dwMemLength - dwMemPos );
	}

	return TRUE;
}


#ifndef MODPLUG_NO_FILESAVE

BOOL CSoundFile::SaveXM( LPCSTR lpszFileName, UINT nPacking )
//---------------------------------------------------------
{
	BYTE s[64 * 64 * 5];
	XMFILEHEADER header;
	XMINSTRUMENTHEADER xmih;
	XMSAMPLEHEADER xmsh;
	XMSAMPLESTRUCT xmss;
	BYTE smptable[32];
	BYTE xmph[9];
	FILE* f;
	int i;

	if ( ( !m_nChannels ) || ( !lpszFileName ) ) { return FALSE; }

	if ( ( f = fopen( lpszFileName, "wb" ) ) == NULL ) { return FALSE; }

	fwrite( "Extended Module: ", 17, 1, f );
	fwrite( m_szNames[0], 20, 1, f );
	s[0] = 0x1A;
	lstrcpy( ( LPSTR )&s[1], ( nPacking ) ? "MOD Plugin packed   " : "FastTracker v2.00   " );
	s[21] = 0x04;
	s[22] = 0x01;
	fwrite( s, 23, 1, f );
	// Writing song header
	memset( &header, 0, sizeof( header ) );
	header.size = sizeof( XMFILEHEADER );
	header.norder = 0;
	header.restartpos = m_nRestartPos;
	header.channels = m_nChannels;
	header.patterns = 0;

	for ( i = 0; i < MAX_ORDERS; i++ )
	{
		if ( Order[i] == 0xFF ) { break; }

		header.norder++;

		if ( ( Order[i] >= header.patterns ) && ( Order[i] < MAX_PATTERNS ) ) { header.patterns = Order[i] + 1; }
	}

	header.instruments = m_nInstruments;

	if ( !header.instruments ) { header.instruments = m_nSamples; }

	header.flags = ( m_dwSongFlags & SONG_LINEARSLIDES ) ? 0x01 : 0x00;

	if ( m_dwSongFlags & SONG_EXFILTERRANGE ) { header.flags |= 0x1000; }

	header.tempo = m_nDefaultTempo;
	header.speed = m_nDefaultSpeed;
	memcpy( header.order, Order, header.norder );
	fwrite( &header, 1, sizeof( header ), f );

	// Writing patterns
	for ( i = 0; i < header.patterns; i++ ) if ( Patterns[i] )
		{
			MODCOMMAND* p = Patterns[i];
			UINT len = 0;

			memset( &xmph, 0, sizeof( xmph ) );
			xmph[0] = 9;
			xmph[5] = ( BYTE )( PatternSize[i] & 0xFF );
			xmph[6] = ( BYTE )( PatternSize[i] >> 8 );

			for ( UINT j = m_nChannels * PatternSize[i]; j; j--, p++ )
			{
				UINT note = p->note;
				UINT param = ModSaveCommand( p, TRUE );
				UINT command = param >> 8;
				param &= 0xFF;

				if ( note >= 0xFE ) { note = 97; }
				else if ( ( note <= 12 ) || ( note > 96 + 12 ) ) { note = 0; }
				else
				{
					note -= 12;
				}

				UINT vol = 0;

				if ( p->volcmd )
				{
					UINT volcmd = p->volcmd;

					switch ( volcmd )
					{
						case VOLCMD_VOLUME:
							vol = 0x10 + p->vol;
							break;

						case VOLCMD_VOLSLIDEDOWN:
							vol = 0x60 + ( p->vol & 0x0F );
							break;

						case VOLCMD_VOLSLIDEUP:
							vol = 0x70 + ( p->vol & 0x0F );
							break;

						case VOLCMD_FINEVOLDOWN:
							vol = 0x80 + ( p->vol & 0x0F );
							break;

						case VOLCMD_FINEVOLUP:
							vol = 0x90 + ( p->vol & 0x0F );
							break;

						case VOLCMD_VIBRATOSPEED:
							vol = 0xA0 + ( p->vol & 0x0F );
							break;

						case VOLCMD_VIBRATO:
							vol = 0xB0 + ( p->vol & 0x0F );
							break;

						case VOLCMD_PANNING:
							vol = 0xC0 + ( p->vol >> 2 );

							if ( vol > 0xCF ) { vol = 0xCF; }

							break;

						case VOLCMD_PANSLIDELEFT:
							vol = 0xD0 + ( p->vol & 0x0F );
							break;

						case VOLCMD_PANSLIDERIGHT:
							vol = 0xE0 + ( p->vol & 0x0F );
							break;

						case VOLCMD_TONEPORTAMENTO:
							vol = 0xF0 + ( p->vol & 0x0F );
							break;
					}
				}

				if ( ( note ) && ( p->instr ) && ( vol > 0x0F ) && ( command ) && ( param ) )
				{
					s[len++] = note;
					s[len++] = p->instr;
					s[len++] = vol;
					s[len++] = command;
					s[len++] = param;
				}
				else
				{
					BYTE b = 0x80;

					if ( note ) { b |= 0x01; }

					if ( p->instr ) { b |= 0x02; }

					if ( vol >= 0x10 ) { b |= 0x04; }

					if ( command ) { b |= 0x08; }

					if ( param ) { b |= 0x10; }

					s[len++] = b;

					if ( b & 1 ) { s[len++] = note; }

					if ( b & 2 ) { s[len++] = p->instr; }

					if ( b & 4 ) { s[len++] = vol; }

					if ( b & 8 ) { s[len++] = command; }

					if ( b & 16 ) { s[len++] = param; }
				}

				if ( len > sizeof( s ) - 5 ) { break; }
			}

			xmph[7] = ( BYTE )( len & 0xFF );
			xmph[8] = ( BYTE )( len >> 8 );
			fwrite( xmph, 1, 9, f );
			fwrite( s, 1, len, f );
		}
		else
		{
			memset( &xmph, 0, sizeof( xmph ) );
			xmph[0] = 9;
			xmph[5] = ( BYTE )( PatternSize[i] & 0xFF );
			xmph[6] = ( BYTE )( PatternSize[i] >> 8 );
			fwrite( xmph, 1, 9, f );
		}

	// Writing instruments
	for ( i = 1; i <= header.instruments; i++ )
	{
		MODINSTRUMENT* pins;
		BYTE flags[32];

		memset( &xmih, 0, sizeof( xmih ) );
		memset( &xmsh, 0, sizeof( xmsh ) );
		xmih.size = sizeof( xmih ) + sizeof( xmsh );
		memcpy( xmih.name, m_szNames[i], 22 );
		xmih.type = 0;
		xmih.samples = 0;

		if ( m_nInstruments )
		{
			INSTRUMENTHEADER* penv = Headers[i];

			if ( penv )
			{
				memcpy( xmih.name, penv->name, 22 );
				xmih.type = penv->nMidiProgram;
				xmsh.volfade = penv->nFadeOut;
				xmsh.vnum = ( BYTE )penv->nVolEnv;
				xmsh.pnum = ( BYTE )penv->nPanEnv;

				if ( xmsh.vnum > 12 ) { xmsh.vnum = 12; }

				if ( xmsh.pnum > 12 ) { xmsh.pnum = 12; }

				for ( UINT ienv = 0; ienv < 12; ienv++ )
				{
					xmsh.venv[ienv * 2] = penv->VolPoints[ienv];
					xmsh.venv[ienv * 2 + 1] = penv->VolEnv[ienv];
					xmsh.penv[ienv * 2] = penv->PanPoints[ienv];
					xmsh.penv[ienv * 2 + 1] = penv->PanEnv[ienv];
				}

				if ( penv->dwFlags & ENV_VOLUME ) { xmsh.vtype |= 1; }

				if ( penv->dwFlags & ENV_VOLSUSTAIN ) { xmsh.vtype |= 2; }

				if ( penv->dwFlags & ENV_VOLLOOP ) { xmsh.vtype |= 4; }

				if ( penv->dwFlags & ENV_PANNING ) { xmsh.ptype |= 1; }

				if ( penv->dwFlags & ENV_PANSUSTAIN ) { xmsh.ptype |= 2; }

				if ( penv->dwFlags & ENV_PANLOOP ) { xmsh.ptype |= 4; }

				xmsh.vsustain = ( BYTE )penv->nVolSustainBegin;
				xmsh.vloops = ( BYTE )penv->nVolLoopStart;
				xmsh.vloope = ( BYTE )penv->nVolLoopEnd;
				xmsh.psustain = ( BYTE )penv->nPanSustainBegin;
				xmsh.ploops = ( BYTE )penv->nPanLoopStart;
				xmsh.ploope = ( BYTE )penv->nPanLoopEnd;

				for ( UINT j = 0; j < 96; j++ ) if ( penv->Keyboard[j + 12] )
					{
						UINT k;

						for ( k = 0; k < xmih.samples; k++ )   if ( smptable[k] == penv->Keyboard[j + 12] ) { break; }

						if ( k == xmih.samples )
						{
							smptable[xmih.samples++] = penv->Keyboard[j + 12];
						}

						if ( xmih.samples >= 32 ) { break; }

						xmsh.snum[j] = k;
					}

//				xmsh.reserved2 = xmih.samples;
			}
		}
		else
		{
			xmih.samples = 1;
//			xmsh.reserved2 = 1;
			smptable[0] = i;
		}

		xmsh.shsize = ( xmih.samples ) ? 40 : 0;
		fwrite( &xmih, 1, sizeof( xmih ), f );

		if ( smptable[0] )
		{
			MODINSTRUMENT* pvib = &Ins[smptable[0]];
			xmsh.vibtype = pvib->nVibType;
			xmsh.vibsweep = pvib->nVibSweep;
			xmsh.vibdepth = pvib->nVibDepth;
			xmsh.vibrate = pvib->nVibRate;
		}

		fwrite( &xmsh, 1, xmih.size - sizeof( xmih ), f );

		if ( !xmih.samples ) { continue; }

		for ( UINT ins = 0; ins < xmih.samples; ins++ )
		{
			memset( &xmss, 0, sizeof( xmss ) );

			if ( smptable[ins] ) { memcpy( xmss.name, m_szNames[smptable[ins]], 22 ); }

			pins = &Ins[smptable[ins]];
			xmss.samplen = pins->nLength;
			xmss.loopstart = pins->nLoopStart;
			xmss.looplen = pins->nLoopEnd - pins->nLoopStart;
			xmss.vol = pins->nVolume / 4;
			xmss.finetune = ( char )pins->nFineTune;
			xmss.type = 0;

			if ( pins->uFlags & CHN_LOOP ) { xmss.type = ( pins->uFlags & CHN_PINGPONGLOOP ) ? 2 : 1; }

			flags[ins] = RS_PCM8D;
#ifndef NO_PACKING

			if ( nPacking )
			{
				if ( ( !( pins->uFlags & ( CHN_16BIT | CHN_STEREO ) ) )
				     && ( CanPackSample( ( char* )pins->pSample, pins->nLength, nPacking ) ) )
				{
					flags[ins] = RS_ADPCM4;
					xmss.res = 0xAD;
				}
			}
			else
#endif
			{
				if ( pins->uFlags & CHN_16BIT )
				{
					flags[ins] = RS_PCM16D;
					xmss.type |= 0x10;
					xmss.looplen *= 2;
					xmss.loopstart *= 2;
					xmss.samplen *= 2;
				}

				if ( pins->uFlags & CHN_STEREO )
				{
					flags[ins] = ( pins->uFlags & CHN_16BIT ) ? RS_STPCM16D : RS_STPCM8D;
					xmss.type |= 0x20;
					xmss.looplen *= 2;
					xmss.loopstart *= 2;
					xmss.samplen *= 2;
				}
			}

			xmss.pan = 255;

			if ( pins->nPan < 256 ) { xmss.pan = ( BYTE )pins->nPan; }

			xmss.relnote = ( signed char )pins->RelativeTone;
			fwrite( &xmss, 1, xmsh.shsize, f );
		}

		for ( UINT ismpd = 0; ismpd < xmih.samples; ismpd++ )
		{
			pins = &Ins[smptable[ismpd]];

			if ( pins->pSample )
			{
#ifndef NO_PACKING

				if ( ( flags[ismpd] == RS_ADPCM4 ) && ( xmih.samples > 1 ) ) { CanPackSample( ( char* )pins->pSample, pins->nLength, nPacking ); }

#endif // NO_PACKING
				WriteSample( f, pins, flags[ismpd] );
			}
		}
	}

	// Writing song comments
	if ( ( m_lpszSongComments ) && ( m_lpszSongComments[0] ) )
	{
		DWORD d = 0x74786574;
		fwrite( &d, 1, 4, f );
		d = strlen( m_lpszSongComments );
		fwrite( &d, 1, 4, f );
		fwrite( m_lpszSongComments, 1, d, f );
	}

	// Writing midi cfg
	if ( m_dwSongFlags & SONG_EMBEDMIDICFG )
	{
		DWORD d = 0x4944494D;
		fwrite( &d, 1, 4, f );
		d = sizeof( MODMIDICFG );
		fwrite( &d, 1, 4, f );
		fwrite( &m_MidiCfg, 1, sizeof( MODMIDICFG ), f );
	}

	// Writing Pattern Names
	if ( ( m_nPatternNames ) && ( m_lpszPatternNames ) )
	{
		DWORD dwLen = m_nPatternNames * MAX_PATTERNNAME;

		while ( ( dwLen >= MAX_PATTERNNAME ) && ( !m_lpszPatternNames[dwLen - MAX_PATTERNNAME] ) ) { dwLen -= MAX_PATTERNNAME; }

		if ( dwLen >= MAX_PATTERNNAME )
		{
			DWORD d = 0x4d414e50;
			fwrite( &d, 1, 4, f );
			fwrite( &dwLen, 1, 4, f );
			fwrite( m_lpszPatternNames, 1, dwLen, f );
		}
	}

	// Writing Channel Names
	{
		UINT nChnNames = 0;

		for ( UINT inam = 0; inam < m_nChannels; inam++ )
		{
			if ( ChnSettings[inam].szName[0] ) { nChnNames = inam + 1; }
		}

		// Do it!
		if ( nChnNames )
		{
			DWORD dwLen = nChnNames * MAX_CHANNELNAME;
			DWORD d = 0x4d414e43;
			fwrite( &d, 1, 4, f );
			fwrite( &dwLen, 1, 4, f );

			for ( UINT inam = 0; inam < nChnNames; inam++ )
			{
				fwrite( ChnSettings[inam].szName, 1, MAX_CHANNELNAME, f );
			}
		}
	}
	// Save mix plugins information
	SaveMixPlugins( f );
	fclose( f );
	return TRUE;
}

#endif // MODPLUG_NO_FILESAVE


//////// WAV

#ifndef WAVE_FORMAT_EXTENSIBLE
#define WAVE_FORMAT_EXTENSIBLE   0xFFFE
#endif

/////////////////////////////////////////////////////////////
// WAV file support

BOOL CSoundFile::ReadWav( const BYTE* lpStream, DWORD dwMemLength )
//---------------------------------------------------------------
{
	DWORD dwMemPos = 0;
	WAVEFILEHEADER* phdr = ( WAVEFILEHEADER* )lpStream;
	WAVEFORMATHEADER* pfmt = ( WAVEFORMATHEADER* )( lpStream + sizeof( WAVEFILEHEADER ) );

	if ( ( !lpStream ) || ( dwMemLength < ( DWORD )sizeof( WAVEFILEHEADER ) ) ) { return FALSE; }

	if ( ( phdr->id_RIFF != IFFID_RIFF ) || ( phdr->id_WAVE != IFFID_WAVE )
	     || ( pfmt->id_fmt != IFFID_fmt ) ) { return FALSE; }

	dwMemPos = sizeof( WAVEFILEHEADER ) + 8 + pfmt->hdrlen;

	if ( ( dwMemPos + 8 >= dwMemLength )
	     || ( ( pfmt->format != WAVE_FORMAT_PCM ) && ( pfmt->format != WAVE_FORMAT_EXTENSIBLE ) )
	     || ( pfmt->channels > 4 )
	     || ( !pfmt->channels )
	     || ( !pfmt->freqHz )
	     || ( pfmt->bitspersample & 7 )
	     || ( pfmt->bitspersample < 8 )
	     || ( pfmt->bitspersample > 32 ) ) { return FALSE; }

	WAVEDATAHEADER* pdata;

	for ( ;; )
	{
		pdata = ( WAVEDATAHEADER* )( lpStream + dwMemPos );

		if ( pdata->id_data == IFFID_data ) { break; }

		dwMemPos += pdata->length + 8;

		if ( dwMemPos + 8 >= dwMemLength ) { return FALSE; }
	}

	m_nType = MOD_TYPE_WAV;
	m_nSamples = 0;
	m_nInstruments = 0;
	m_nChannels = 4;
	m_nDefaultSpeed = 8;
	m_nDefaultTempo = 125;
	m_dwSongFlags |= SONG_LINEARSLIDES; // For no resampling
	Order[0] = 0;
	Order[1] = 0xFF;
	PatternSize[0] = PatternSize[1] = 64;

	if ( ( Patterns[0] = AllocatePattern( 64, 4 ) ) == NULL ) { return TRUE; }

	if ( ( Patterns[1] = AllocatePattern( 64, 4 ) ) == NULL ) { return TRUE; }

	UINT samplesize = ( pfmt->channels * pfmt->bitspersample ) >> 3;
	UINT len = pdata->length, bytelen;

	if ( dwMemPos + len > dwMemLength - 8 ) { len = dwMemLength - dwMemPos - 8; }

	len /= samplesize;
	bytelen = len;

	if ( pfmt->bitspersample >= 16 ) { bytelen *= 2; }

	if ( len > MAX_SAMPLE_LENGTH ) { len = MAX_SAMPLE_LENGTH; }

	if ( !len ) { return TRUE; }

	// Setting up module length
	DWORD dwTime = ( ( len * 50 ) / pfmt->freqHz ) + 1;
	DWORD framesperrow = ( dwTime + 63 ) / 63;

	if ( framesperrow < 4 ) { framesperrow = 4; }

	UINT norders = 1;

	while ( framesperrow >= 0x20 )
	{
		Order[norders++] = 1;
		Order[norders] = 0xFF;
		framesperrow = ( dwTime + ( 64 * norders - 1 ) ) / ( 64 * norders );

		if ( norders >= MAX_ORDERS - 1 ) { break; }
	}

	m_nDefaultSpeed = framesperrow;

	for ( UINT iChn = 0; iChn < 4; iChn++ )
	{
		ChnSettings[iChn].nPan = ( iChn & 1 ) ? 256 : 0;
		ChnSettings[iChn].nVolume = 64;
		ChnSettings[iChn].dwFlags = 0;
	}

	// Setting up speed command
	MODCOMMAND* pcmd = Patterns[0];
	pcmd[0].command = CMD_SPEED;
	pcmd[0].param = ( BYTE )m_nDefaultSpeed;
	pcmd[0].note = 5 * 12 + 1;
	pcmd[0].instr = 1;
	pcmd[1].note = pcmd[0].note;
	pcmd[1].instr = pcmd[0].instr;
	m_nSamples = pfmt->channels;

	// Support for Multichannel Wave
	for ( UINT nChn = 0; nChn < m_nSamples; nChn++ )
	{
		MODINSTRUMENT* pins = &Ins[nChn + 1];
		pcmd[nChn].note = pcmd[0].note;
		pcmd[nChn].instr = ( BYTE )( nChn + 1 );
		pins->nLength = len;
		pins->nC4Speed = pfmt->freqHz;
		pins->nVolume = 256;
		pins->nPan = 128;
		pins->nGlobalVol = 64;
		pins->uFlags = ( WORD )( ( pfmt->bitspersample >= 16 ) ? CHN_16BIT : 0 );
		pins->uFlags |= CHN_PANNING;

		if ( m_nSamples > 1 )
		{
			switch ( nChn )
			{
				case 0:
					pins->nPan = 0;
					break;

				case 1:
					pins->nPan = 256;
					break;

				case 2:
					pins->nPan = ( WORD )( ( m_nSamples == 3 ) ? 128 : 64 );
					pcmd[nChn].command = CMD_S3MCMDEX;
					pcmd[nChn].param = 0x91;
					break;

				case 3:
					pins->nPan = 192;
					pcmd[nChn].command = CMD_S3MCMDEX;
					pcmd[nChn].param = 0x91;
					break;

				default:
					pins->nPan = 128;
					break;
			}
		}

		if ( ( pins->pSample = AllocateSample( bytelen + 8 ) ) == NULL ) { return TRUE; }

		if ( pfmt->bitspersample >= 16 )
		{
			int slsize = pfmt->bitspersample >> 3;
			signed short* p = ( signed short* )pins->pSample;
			signed char* psrc = ( signed char* )( lpStream + dwMemPos + 8 + nChn * slsize + slsize - 2 );

			for ( UINT i = 0; i < len; i++ )
			{
				p[i] = *( ( signed short* )psrc );
				psrc += samplesize;
			}

			p[len + 1] = p[len] = p[len - 1];
		}
		else
		{
			signed char* p = ( signed char* )pins->pSample;
			signed char* psrc = ( signed char* )( lpStream + dwMemPos + 8 + nChn );

			for ( UINT i = 0; i < len; i++ )
			{
				p[i] = ( signed char )( ( *psrc ) + 0x80 );
				psrc += samplesize;
			}

			p[len + 1] = p[len] = p[len - 1];
		}
	}

	return TRUE;
}


////////////////////////////////////////////////////////////////////////
// IMA ADPCM Support

#pragma pack(1)

typedef struct IMAADPCMBLOCK
{
	WORD sample;
	BYTE index;
	BYTE Reserved;
} DVI_ADPCMBLOCKHEADER;

#pragma pack()

static const int gIMAUnpackTable[90] =
{
	7,     8,     9,    10,    11,    12,    13,    14,
	16,    17,    19,    21,    23,    25,    28,    31,
	34,    37,    41,    45,    50,    55,    60,    66,
	73,    80,    88,    97,   107,   118,   130,   143,
	157,   173,   190,   209,   230,   253,   279,   307,
	337,   371,   408,   449,   494,   544,   598,   658,
	724,   796,   876,   963,  1060,  1166,  1282,  1411,
	1552,  1707,  1878,  2066,  2272,  2499,  2749,  3024,
	3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,
	7132,  7845,  8630,  9493, 10442, 11487, 12635, 13899,
	15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794,
	32767, 0
};


BOOL IMAADPCMUnpack16( signed short* pdest, UINT nLen, LPBYTE psrc, DWORD dwBytes, UINT pkBlkAlign )
//------------------------------------------------------------------------------------------------
{
	static const int gIMAIndexTab[8] =  { -1, -1, -1, -1, 2, 4, 6, 8 };
	UINT nPos;
	int value;

	if ( ( nLen < 4 ) || ( !pdest ) || ( !psrc )
	     || ( pkBlkAlign < 5 ) || ( pkBlkAlign > dwBytes ) ) { return FALSE; }

	nPos = 0;

	while ( ( nPos < nLen ) && ( dwBytes > 4 ) )
	{
		int nIndex;
		value = *( ( short int* )psrc );
		nIndex = psrc[2];
		psrc += 4;
		dwBytes -= 4;
		pdest[nPos++] = ( short int )value;

		for ( UINT i = 0; ( ( i < ( pkBlkAlign - 4 ) * 2 ) && ( nPos < nLen ) && ( dwBytes ) ); i++ )
		{
			BYTE delta;

			if ( i & 1 )
			{
				delta = ( BYTE )( ( ( *( psrc++ ) ) >> 4 ) & 0x0F );
				dwBytes--;
			}
			else
			{
				delta = ( BYTE )( ( *psrc ) & 0x0F );
			}

			int v = gIMAUnpackTable[nIndex] >> 3;

			if ( delta & 1 ) { v += gIMAUnpackTable[nIndex] >> 2; }

			if ( delta & 2 ) { v += gIMAUnpackTable[nIndex] >> 1; }

			if ( delta & 4 ) { v += gIMAUnpackTable[nIndex]; }

			if ( delta & 8 ) { value -= v; }
			else { value += v; }

			nIndex += gIMAIndexTab[delta & 7];

			if ( nIndex < 0 ) { nIndex = 0; }
			else if ( nIndex > 88 ) { nIndex = 88; }

			if ( value > 32767 ) { value = 32767; }
			else if ( value < -32768 ) { value = -32768; }

			pdest[nPos++] = ( short int )value;
		}
	}

	return TRUE;
}


////// S3M

#ifdef _MSC_VER
//#pragma warning(disable:4244)
#endif

//////////////////////////////////////////////////////
// ScreamTracker S3M file support

typedef struct tagS3MSAMPLESTRUCT
{
	BYTE type;
	CHAR dosname[12];
	BYTE hmem;
	WORD memseg;
	DWORD length;
	DWORD loopbegin;
	DWORD loopend;
	BYTE vol;
	BYTE bReserved;
	BYTE pack;
	BYTE flags;
	DWORD finetune;
	DWORD dwReserved;
	WORD intgp;
	WORD int512;
	DWORD lastused;
	CHAR name[28];
	CHAR scrs[4];
} S3MSAMPLESTRUCT;


typedef struct tagS3MFILEHEADER
{
	CHAR name[28];
	BYTE b1A;
	BYTE type;
	WORD reserved1;
	WORD ordnum;
	WORD insnum;
	WORD patnum;
	WORD flags;
	WORD cwtv;
	WORD version;
	DWORD scrm; // "SCRM" = 0x4D524353
	BYTE globalvol;
	BYTE speed;
	BYTE tempo;
	BYTE mastervol;
	BYTE ultraclicks;
	BYTE panning_present;
	BYTE reserved2[8];
	WORD special;
	BYTE channels[32];
} S3MFILEHEADER;


void CSoundFile::S3MConvert( MODCOMMAND* m, BOOL bIT ) const
//--------------------------------------------------------
{
	UINT command = m->command;
	UINT param = m->param;

	switch ( command + 0x40 )
	{
		case 'A':
			command = CMD_SPEED;
			break;

		case 'B':
			command = CMD_POSITIONJUMP;
			break;

		case 'C':
			command = CMD_PATTERNBREAK;

			if ( !bIT ) { param = ( param >> 4 ) * 10 + ( param & 0x0F ); }

			break;

		case 'D':
			command = CMD_VOLUMESLIDE;
			break;

		case 'E':
			command = CMD_PORTAMENTODOWN;
			break;

		case 'F':
			command = CMD_PORTAMENTOUP;
			break;

		case 'G':
			command = CMD_TONEPORTAMENTO;
			break;

		case 'H':
			command = CMD_VIBRATO;
			break;

		case 'I':
			command = CMD_TREMOR;
			break;

		case 'J':
			command = CMD_ARPEGGIO;
			break;

		case 'K':
			command = CMD_VIBRATOVOL;
			break;

		case 'L':
			command = CMD_TONEPORTAVOL;
			break;

		case 'M':
			command = CMD_CHANNELVOLUME;
			break;

		case 'N':
			command = CMD_CHANNELVOLSLIDE;
			break;

		case 'O':
			command = CMD_OFFSET;
			break;

		case 'P':
			command = CMD_PANNINGSLIDE;
			break;

		case 'Q':
			command = CMD_RETRIG;
			break;

		case 'R':
			command = CMD_TREMOLO;
			break;

		case 'S':
			command = CMD_S3MCMDEX;
			break;

		case 'T':
			command = CMD_TEMPO;
			break;

		case 'U':
			command = CMD_FINEVIBRATO;
			break;

		case 'V':
			command = CMD_GLOBALVOLUME;
			break;

		case 'W':
			command = CMD_GLOBALVOLSLIDE;
			break;

		case 'X':
			command = CMD_PANNING8;
			break;

		case 'Y':
			command = CMD_PANBRELLO;
			break;

		case 'Z':
			command = CMD_MIDI;
			break;

		default:
			command = 0;
	}

	m->command = command;
	m->param = param;
}


void CSoundFile::S3MSaveConvert( UINT* pcmd, UINT* pprm, BOOL bIT ) const
//---------------------------------------------------------------------
{
	UINT command = *pcmd;
	UINT param = *pprm;

	switch ( command )
	{
		case CMD_SPEED:
			command = 'A';
			break;

		case CMD_POSITIONJUMP:
			command = 'B';
			break;

		case CMD_PATTERNBREAK:
			command = 'C';

			if ( !bIT ) { param = ( ( param / 10 ) << 4 ) + ( param % 10 ); }

			break;

		case CMD_VOLUMESLIDE:
			command = 'D';
			break;

		case CMD_PORTAMENTODOWN:
			command = 'E';

			if ( ( param >= 0xE0 ) && ( m_nType & ( MOD_TYPE_MOD | MOD_TYPE_XM ) ) ) { param = 0xDF; }

			break;

		case CMD_PORTAMENTOUP:
			command = 'F';

			if ( ( param >= 0xE0 ) && ( m_nType & ( MOD_TYPE_MOD | MOD_TYPE_XM ) ) ) { param = 0xDF; }

			break;

		case CMD_TONEPORTAMENTO:
			command = 'G';
			break;

		case CMD_VIBRATO:
			command = 'H';
			break;

		case CMD_TREMOR:
			command = 'I';
			break;

		case CMD_ARPEGGIO:
			command = 'J';
			break;

		case CMD_VIBRATOVOL:
			command = 'K';
			break;

		case CMD_TONEPORTAVOL:
			command = 'L';
			break;

		case CMD_CHANNELVOLUME:
			command = 'M';
			break;

		case CMD_CHANNELVOLSLIDE:
			command = 'N';
			break;

		case CMD_OFFSET:
			command = 'O';
			break;

		case CMD_PANNINGSLIDE:
			command = 'P';
			break;

		case CMD_RETRIG:
			command = 'Q';
			break;

		case CMD_TREMOLO:
			command = 'R';
			break;

		case CMD_S3MCMDEX:
			command = 'S';
			break;

		case CMD_TEMPO:
			command = 'T';
			break;

		case CMD_FINEVIBRATO:
			command = 'U';
			break;

		case CMD_GLOBALVOLUME:
			command = 'V';
			break;

		case CMD_GLOBALVOLSLIDE:
			command = 'W';
			break;

		case CMD_PANNING8:
			command = 'X';

			if ( ( bIT ) && ( m_nType != MOD_TYPE_IT ) && ( m_nType != MOD_TYPE_XM ) )
			{
				if ( param == 0xA4 ) { command = 'S'; param = 0x91; }
				else if ( param <= 0x80 ) { param <<= 1; if ( param > 255 ) { param = 255; } }
				else
				{
					command = param = 0;
				}
			}
			else if ( ( !bIT ) && ( ( m_nType == MOD_TYPE_IT ) || ( m_nType == MOD_TYPE_XM ) ) )
			{
				param >>= 1;
			}

			break;

		case CMD_PANBRELLO:
			command = 'Y';
			break;

		case CMD_MIDI:
			command = 'Z';
			break;

		case CMD_XFINEPORTAUPDOWN:
			if ( param & 0x0F ) switch ( param & 0xF0 )
				{
					case 0x10:
						command = 'F';
						param = ( param & 0x0F ) | 0xE0;
						break;

					case 0x20:
						command = 'E';
						param = ( param & 0x0F ) | 0xE0;
						break;

					case 0x90:
						command = 'S';
						break;

					default:
						command = param = 0;
				}
			else { command = param = 0; }

			break;

		case CMD_MODCMDEX:
			command = 'S';

			switch ( param & 0xF0 )
			{
				case 0x00:
					command = param = 0;
					break;

				case 0x10:
					command = 'F';
					param |= 0xF0;
					break;

				case 0x20:
					command = 'E';
					param |= 0xF0;
					break;

				case 0x30:
					param = ( param & 0x0F ) | 0x10;
					break;

				case 0x40:
					param = ( param & 0x0F ) | 0x30;
					break;

				case 0x50:
					param = ( param & 0x0F ) | 0x20;
					break;

				case 0x60:
					param = ( param & 0x0F ) | 0xB0;
					break;

				case 0x70:
					param = ( param & 0x0F ) | 0x40;
					break;

				case 0x90:
					command = 'Q';
					param &= 0x0F;
					break;

				case 0xA0:
					if ( param & 0x0F ) { command = 'D'; param = ( param << 4 ) | 0x0F; }
					else { command = param = 0; }

					break;

				case 0xB0:
					if ( param & 0x0F ) { command = 'D'; param |= 0xF0; }
					else { command = param = 0; }

					break;
			}

			break;

		default:
			command = param = 0;
	}

	command &= ~0x40;
	*pcmd = command;
	*pprm = param;
}


BOOL CSoundFile::ReadS3M( const BYTE* lpStream, DWORD dwMemLength )
//---------------------------------------------------------------
{
	UINT insnum, patnum, nins, npat;
	DWORD insfile[128];
	WORD ptr[256];
	BYTE s[1024];
	DWORD dwMemPos;
	BYTE insflags[128], inspack[128];
	S3MFILEHEADER psfh = *( S3MFILEHEADER* )lpStream;

	psfh.reserved1 = bswapLE16( psfh.reserved1 );
	psfh.ordnum = bswapLE16( psfh.ordnum );
	psfh.insnum = bswapLE16( psfh.insnum );
	psfh.patnum = bswapLE16( psfh.patnum );
	psfh.flags = bswapLE16( psfh.flags );
	psfh.cwtv = bswapLE16( psfh.cwtv );
	psfh.version = bswapLE16( psfh.version );
	psfh.scrm = bswapLE32( psfh.scrm );
	psfh.special = bswapLE16( psfh.special );

	if ( ( !lpStream ) || ( dwMemLength <= sizeof( S3MFILEHEADER ) + sizeof( S3MSAMPLESTRUCT ) + 64 ) ) { return FALSE; }

	if ( psfh.scrm != 0x4D524353 ) { return FALSE; }

	dwMemPos = 0x60;
	m_nType = MOD_TYPE_S3M;
	memset( m_szNames, 0, sizeof( m_szNames ) );
	memcpy( m_szNames[0], psfh.name, 28 );
	// Speed
	m_nDefaultSpeed = psfh.speed;

	if ( m_nDefaultSpeed < 1 ) { m_nDefaultSpeed = 6; }

	if ( m_nDefaultSpeed > 0x1F ) { m_nDefaultSpeed = 0x1F; }

	// Tempo
	m_nDefaultTempo = psfh.tempo;

	if ( m_nDefaultTempo < 40 ) { m_nDefaultTempo = 40; }

	if ( m_nDefaultTempo > 240 ) { m_nDefaultTempo = 240; }

	// Global Volume
	m_nDefaultGlobalVolume = psfh.globalvol << 2;

	if ( ( !m_nDefaultGlobalVolume ) || ( m_nDefaultGlobalVolume > 256 ) ) { m_nDefaultGlobalVolume = 256; }

	m_nSongPreAmp = psfh.mastervol & 0x7F;
	// Channels
	m_nChannels = 4;

	for ( UINT ich = 0; ich < 32; ich++ )
	{
		ChnSettings[ich].nPan = 128;
		ChnSettings[ich].nVolume = 64;

		ChnSettings[ich].dwFlags = CHN_MUTE;

		if ( psfh.channels[ich] != 0xFF )
		{
			m_nChannels = ich + 1;
			UINT b = psfh.channels[ich] & 0x0F;
			ChnSettings[ich].nPan = ( b & 8 ) ? 0xC0 : 0x40;
			ChnSettings[ich].dwFlags = 0;
		}
	}

	if ( m_nChannels < 4 ) { m_nChannels = 4; }

	if ( ( psfh.cwtv < 0x1320 ) || ( psfh.flags & 0x40 ) ) { m_dwSongFlags |= SONG_FASTVOLSLIDES; }

	// Reading pattern order
	UINT iord = psfh.ordnum;

	if ( iord < 1 ) { iord = 1; }

	if ( iord > MAX_ORDERS ) { iord = MAX_ORDERS; }

	if ( iord )
	{
		memcpy( Order, lpStream + dwMemPos, iord );
		dwMemPos += iord;
	}

	if ( ( iord & 1 ) && ( lpStream[dwMemPos] == 0xFF ) ) { dwMemPos++; }

	// Reading file pointers
	insnum = nins = psfh.insnum;

	if ( insnum >= MAX_SAMPLES ) { insnum = MAX_SAMPLES - 1; }

	m_nSamples = insnum;
	patnum = npat = psfh.patnum;

	if ( patnum > MAX_PATTERNS ) { patnum = MAX_PATTERNS; }

	memset( ptr, 0, sizeof( ptr ) );

	// Ignore file if it has a corrupted header.
	if ( nins + npat > 256 ) { return FALSE; }

	if ( nins + npat )
	{
		memcpy( ptr, lpStream + dwMemPos, 2 * ( nins + npat ) );
		dwMemPos += 2 * ( nins + npat );

		for ( UINT j = 0; j < ( nins + npat ); ++j )
		{
			ptr[j] = bswapLE16( ptr[j] );
		}

		if ( psfh.panning_present == 252 )
		{
			const BYTE* chnpan = lpStream + dwMemPos;

			for ( UINT i = 0; i < 32; i++ ) if ( chnpan[i] & 0x20 )
				{
					ChnSettings[i].nPan = ( ( chnpan[i] & 0x0F ) << 4 ) + 8;
				}
		}
	}

	if ( !m_nChannels ) { return TRUE; }

	// Reading instrument headers
	memset( insfile, 0, sizeof( insfile ) );

	for ( UINT iSmp = 1; iSmp <= insnum; iSmp++ )
	{
		UINT nInd = ( ( DWORD )ptr[iSmp - 1] ) * 16;

		if ( ( !nInd ) || ( nInd + 0x50 > dwMemLength ) ) { continue; }

		memcpy( s, lpStream + nInd, 0x50 );
		memcpy( Ins[iSmp].name, s + 1, 12 );
		insflags[iSmp - 1] = s[0x1F];
		inspack[iSmp - 1] = s[0x1E];
		s[0x4C] = 0;
		lstrcpy( m_szNames[iSmp], ( LPCSTR )&s[0x30] );

		if ( ( s[0] == 1 ) && ( s[0x4E] == 'R' ) && ( s[0x4F] == 'S' ) )
		{
			UINT j = bswapLE32( *( ( LPDWORD )( s + 0x10 ) ) );

			if ( j > MAX_SAMPLE_LENGTH ) { j = MAX_SAMPLE_LENGTH; }

			if ( j < 4 ) { j = 0; }

			Ins[iSmp].nLength = j;
			j = bswapLE32( *( ( LPDWORD )( s + 0x14 ) ) );

			if ( j >= Ins[iSmp].nLength ) { j = Ins[iSmp].nLength - 1; }

			Ins[iSmp].nLoopStart = j;
			j = bswapLE32( *( ( LPDWORD )( s + 0x18 ) ) );

			if ( j > MAX_SAMPLE_LENGTH ) { j = MAX_SAMPLE_LENGTH; }

			if ( j < 4 ) { j = 0; }

			if ( j > Ins[iSmp].nLength ) { j = Ins[iSmp].nLength; }

			Ins[iSmp].nLoopEnd = j;
			j = s[0x1C];

			if ( j > 64 ) { j = 64; }

			Ins[iSmp].nVolume = j << 2;
			Ins[iSmp].nGlobalVol = 64;

			if ( s[0x1F] & 1 ) { Ins[iSmp].uFlags |= CHN_LOOP; }

			j = bswapLE32( *( ( LPDWORD )( s + 0x20 ) ) );

			if ( !j ) { j = 8363; }

			if ( j < 1024 ) { j = 1024; }

			Ins[iSmp].nC4Speed = j;
			insfile[iSmp] = ( ( DWORD )bswapLE16( *( ( LPWORD )( s + 0x0E ) ) ) ) << 4;
			insfile[iSmp] += ( ( DWORD )( BYTE )s[0x0D] ) << 20;

			if ( insfile[iSmp] > dwMemLength ) { insfile[iSmp] &= 0xFFFF; }

			if ( ( Ins[iSmp].nLoopStart >= Ins[iSmp].nLoopEnd ) || ( Ins[iSmp].nLoopEnd - Ins[iSmp].nLoopStart < 8 ) )
			{
				Ins[iSmp].nLoopStart = Ins[iSmp].nLoopEnd = 0;
			}

			Ins[iSmp].nPan = 0x80;
		}
	}

	// Reading patterns
	for ( UINT iPat = 0; iPat < patnum; iPat++ )
	{
		UINT nInd = ( ( DWORD )ptr[nins + iPat] ) << 4;

		if ( nInd + 0x40 > dwMemLength ) { continue; }

		WORD len = bswapLE16( *( ( WORD* )( lpStream + nInd ) ) );
		nInd += 2;
		PatternSize[iPat] = 64;

		if ( ( !len ) || ( nInd + len > dwMemLength - 6 )
		     || ( ( Patterns[iPat] = AllocatePattern( 64, m_nChannels ) ) == NULL ) ) { continue; }

		LPBYTE src = ( LPBYTE )( lpStream + nInd );
		// Unpacking pattern
		MODCOMMAND* p = Patterns[iPat];
		UINT row = 0;
		UINT j = 0;

		while ( j < len )
		{
			BYTE b = src[j++];

			if ( !b )
			{
				if ( ++row >= 64 ) { break; }
			}
			else
			{
				UINT chn = b & 0x1F;

				if ( chn < m_nChannels )
				{
					MODCOMMAND* m = &p[row * m_nChannels + chn];

					if ( b & 0x20 )
					{
						m->note = src[j++];

						if ( m->note < 0xF0 ) { m->note = ( m->note & 0x0F ) + 12 * ( m->note >> 4 ) + 13; }
						else if ( m->note == 0xFF ) { m->note = 0; }

						m->instr = src[j++];
					}

					if ( b & 0x40 )
					{
						UINT vol = src[j++];

						if ( ( vol >= 128 ) && ( vol <= 192 ) )
						{
							vol -= 128;
							m->volcmd = VOLCMD_PANNING;
						}
						else
						{
							if ( vol > 64 ) { vol = 64; }

							m->volcmd = VOLCMD_VOLUME;
						}

						m->vol = vol;
					}

					if ( b & 0x80 )
					{
						m->command = src[j++];
						m->param = src[j++];

						if ( m->command ) { S3MConvert( m, FALSE ); }
					}
				}
				else
				{
					if ( b & 0x20 ) { j += 2; }

					if ( b & 0x40 ) { j++; }

					if ( b & 0x80 ) { j += 2; }
				}

				if ( j >= len ) { break; }
			}
		}
	}

	// Reading samples
	for ( UINT iRaw = 1; iRaw <= insnum; iRaw++ ) if ( ( Ins[iRaw].nLength ) && ( insfile[iRaw] ) )
		{
			UINT flags = ( psfh.version == 1 ) ? RS_PCM8S : RS_PCM8U;

			if ( insflags[iRaw - 1] & 4 ) { flags += 5; }

			if ( insflags[iRaw - 1] & 2 ) { flags |= RSF_STEREO; }

			if ( inspack[iRaw - 1] == 4 ) { flags = RS_ADPCM4; }

			dwMemPos = insfile[iRaw];
			dwMemPos += ReadSample( &Ins[iRaw], flags, ( LPSTR )( lpStream + dwMemPos ), dwMemLength - dwMemPos );
		}

	m_nMinPeriod = 64;
	m_nMaxPeriod = 32767;

	if ( psfh.flags & 0x10 ) { m_dwSongFlags |= SONG_AMIGALIMITS; }

	return TRUE;
}


#ifndef MODPLUG_NO_FILESAVE

#ifdef _MSC_VER
#pragma warning(disable:4100)
#endif

static BYTE S3MFiller[16] =
{
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80
};


BOOL CSoundFile::SaveS3M( LPCSTR lpszFileName, UINT nPacking )
//----------------------------------------------------------
{
	FILE* f;
	BYTE header[0x60];
	UINT nbo, nbi, nbp, i;
	WORD patptr[128];
	WORD insptr[128];
	BYTE buffer[5 * 1024];
	S3MSAMPLESTRUCT insex[128];

	if ( ( !m_nChannels ) || ( !lpszFileName ) ) { return FALSE; }

	if ( ( f = fopen( lpszFileName, "wb" ) ) == NULL ) { return FALSE; }

	// Writing S3M header
	memset( header, 0, sizeof( header ) );
	memset( insex, 0, sizeof( insex ) );
	memcpy( header, m_szNames[0], 0x1C );
	header[0x1B] = 0;
	header[0x1C] = 0x1A;
	header[0x1D] = 0x10;
	nbo = ( GetNumPatterns() + 15 ) & 0xF0;

	if ( !nbo ) { nbo = 16; }

	header[0x20] = nbo & 0xFF;
	header[0x21] = nbo >> 8;
	nbi = m_nInstruments;

	if ( !nbi ) { nbi = m_nSamples; }

	if ( nbi > 99 ) { nbi = 99; }

	header[0x22] = nbi & 0xFF;
	header[0x23] = nbi >> 8;
	nbp = 0;

	for ( i = 0; Patterns[i]; i++ ) { nbp = i + 1; if ( nbp >= MAX_PATTERNS ) { break; } }

	for ( i = 0; i < MAX_ORDERS; i++ ) if ( ( Order[i] < MAX_PATTERNS ) && ( Order[i] >= nbp ) ) { nbp = Order[i] + 1; }

	header[0x24] = nbp & 0xFF;
	header[0x25] = nbp >> 8;

	if ( m_dwSongFlags & SONG_FASTVOLSLIDES ) { header[0x26] |= 0x40; }

	if ( ( m_nMaxPeriod < 20000 ) || ( m_dwSongFlags & SONG_AMIGALIMITS ) ) { header[0x26] |= 0x10; }

	header[0x28] = 0x20;
	header[0x29] = 0x13;
	header[0x2A] = 0x02; // Version = 1 => Signed samples
	header[0x2B] = 0x00;
	header[0x2C] = 'S';
	header[0x2D] = 'C';
	header[0x2E] = 'R';
	header[0x2F] = 'M';
	header[0x30] = m_nDefaultGlobalVolume >> 2;
	header[0x31] = m_nDefaultSpeed;
	header[0x32] = m_nDefaultTempo;
	header[0x33] = ( ( m_nSongPreAmp < 0x20 ) ? 0x20 : m_nSongPreAmp ) | 0x80; // Stereo
	header[0x35] = 0xFC;

	for ( i = 0; i < 32; i++ )
	{
		if ( i < m_nChannels )
		{
			UINT tmp = ( i & 0x0F ) >> 1;
			header[0x40 + i] = ( i & 0x10 ) | ( ( i & 1 ) ? 8 + tmp : tmp );
		}
		else { header[0x40 + i] = 0xFF; }
	}

	fwrite( header, 0x60, 1, f );
	fwrite( Order, nbo, 1, f );
	memset( patptr, 0, sizeof( patptr ) );
	memset( insptr, 0, sizeof( insptr ) );
	UINT ofs0 = 0x60 + nbo;
	UINT ofs1 = ( ( 0x60 + nbo + nbi * 2 + nbp * 2 + 15 ) & 0xFFF0 ) + 0x20;
	UINT ofs = ofs1;

	for ( i = 0; i < nbi; i++ ) { insptr[i] = ( WORD )( ( ofs + i * 0x50 ) / 16 ); }

	for ( i = 0; i < nbp; i++ ) { patptr[i] = ( WORD )( ( ofs + nbi * 0x50 ) / 16 ); }

	fwrite( insptr, nbi, 2, f );
	fwrite( patptr, nbp, 2, f );

	if ( header[0x35] == 0xFC )
	{
		BYTE chnpan[32];

		for ( i = 0; i < 32; i++ )
		{
			chnpan[i] = 0x20 | ( ChnSettings[i].nPan >> 4 );
		}

		fwrite( chnpan, 0x20, 1, f );
	}

	if ( ( nbi * 2 + nbp * 2 ) & 0x0F )
	{
		fwrite( S3MFiller, 0x10 - ( ( nbi * 2 + nbp * 2 ) & 0x0F ), 1, f );
	}

	ofs1 = ftell( f );
	fwrite( insex, nbi, 0x50, f );
	// Packing patterns
	ofs += nbi * 0x50;

	for ( i = 0; i < nbp; i++ )
	{
		WORD len = 64;
		memset( buffer, 0, sizeof( buffer ) );
		patptr[i] = ofs / 16;

		if ( Patterns[i] )
		{
			len = 2;
			MODCOMMAND* p = Patterns[i];

			for ( int row = 0; row < 64; row++ ) if ( row < PatternSize[i] )
				{
					for ( UINT j = 0; j < m_nChannels; j++ )
					{
						UINT b = j;
						MODCOMMAND* m = &p[row * m_nChannels + j];
						UINT note = m->note;
						UINT volcmd = m->volcmd;
						UINT vol = m->vol;
						UINT command = m->command;
						UINT param = m->param;

						if ( ( note ) || ( m->instr ) ) { b |= 0x20; }

						if ( !note ) { note = 0xFF; }
						else if ( note >= 0xFE ) { note = 0xFE; }
						else if ( note < 13 ) { note = 0; }
						else { note -= 13; }

						if ( note < 0xFE ) { note = ( note % 12 ) + ( ( note / 12 ) << 4 ); }

						if ( command == CMD_VOLUME )
						{
							command = 0;

							if ( param > 64 ) { param = 64; }

							volcmd = VOLCMD_VOLUME;
							vol = param;
						}

						if ( volcmd == VOLCMD_VOLUME ) { b |= 0x40; }
						else if ( volcmd == VOLCMD_PANNING ) { vol |= 0x80; b |= 0x40; }

						if ( command )
						{
							S3MSaveConvert( &command, &param, FALSE );

							if ( command ) { b |= 0x80; }
						}

						if ( b & 0xE0 )
						{
							buffer[len++] = b;

							if ( b & 0x20 )
							{
								buffer[len++] = note;
								buffer[len++] = m->instr;
							}

							if ( b & 0x40 )
							{
								buffer[len++] = vol;
							}

							if ( b & 0x80 )
							{
								buffer[len++] = command;
								buffer[len++] = param;
							}

							if ( len > sizeof( buffer ) - 20 ) { break; }
						}
					}

					buffer[len++] = 0;

					if ( len > sizeof( buffer ) - 20 ) { break; }
				}
		}

		buffer[0] = ( len - 2 ) & 0xFF;
		buffer[1] = ( len - 2 ) >> 8;
		len = ( len + 15 ) & ( ~0x0F );
		fwrite( buffer, len, 1, f );
		ofs += len;
	}

	// Writing samples
	for ( i = 1; i <= nbi; i++ )
	{
		MODINSTRUMENT* pins = &Ins[i];

		if ( m_nInstruments )
		{
			pins = Ins;

			if ( Headers[i] )
			{
				for ( UINT j = 0; j < 128; j++ )
				{
					UINT n = Headers[i]->Keyboard[j];

					if ( ( n ) && ( n < MAX_INSTRUMENTS ) )
					{
						pins = &Ins[n];
						break;
					}
				}
			}
		}

		memcpy( insex[i - 1].dosname, pins->name, 12 );
		memcpy( insex[i - 1].name, m_szNames[i], 28 );
		memcpy( insex[i - 1].scrs, "SCRS", 4 );
		insex[i - 1].hmem = ( BYTE )( ( DWORD )ofs >> 20 );
		insex[i - 1].memseg = ( WORD )( ( DWORD )ofs >> 4 );

		if ( pins->pSample )
		{
			insex[i - 1].type = 1;
			insex[i - 1].length = pins->nLength;
			insex[i - 1].loopbegin = pins->nLoopStart;
			insex[i - 1].loopend = pins->nLoopEnd;
			insex[i - 1].vol = pins->nVolume / 4;
			insex[i - 1].flags = ( pins->uFlags & CHN_LOOP ) ? 1 : 0;

			if ( pins->nC4Speed )
			{
				insex[i - 1].finetune = pins->nC4Speed;
			}
			else
			{
				insex[i - 1].finetune = TransposeToFrequency( pins->RelativeTone, pins->nFineTune );
			}

			UINT flags = RS_PCM8U;
#ifndef NO_PACKING

			if ( nPacking )
			{
				if ( ( !( pins->uFlags & ( CHN_16BIT | CHN_STEREO ) ) )
				     && ( CanPackSample( ( char* )pins->pSample, pins->nLength, nPacking ) ) )
				{
					insex[i - 1].pack = 4;
					flags = RS_ADPCM4;
				}
			}
			else
#endif // NO_PACKING
			{
				if ( pins->uFlags & CHN_16BIT )
				{
					insex[i - 1].flags |= 4;
					flags = RS_PCM16U;
				}

				if ( pins->uFlags & CHN_STEREO )
				{
					insex[i - 1].flags |= 2;
					flags = ( pins->uFlags & CHN_16BIT ) ? RS_STPCM16U : RS_STPCM8U;
				}
			}

			DWORD len = WriteSample( f, pins, flags );

			if ( len & 0x0F )
			{
				fwrite( S3MFiller, 0x10 - ( len & 0x0F ), 1, f );
			}

			ofs += ( len + 15 ) & ( ~0x0F );
		}
		else
		{
			insex[i - 1].length = 0;
		}
	}

	// Updating parapointers
	fseek( f, ofs0, SEEK_SET );
	fwrite( insptr, nbi, 2, f );
	fwrite( patptr, nbp, 2, f );
	fseek( f, ofs1, SEEK_SET );
	fwrite( insex, 0x50, nbi, f );
	fclose( f );
	return TRUE;
}

#ifdef _MSC_VER
#pragma warning(default:4100)
#endif

#endif // MODPLUG_NO_FILESAVE


//////////////////////////////////////////////////////////
// ProTracker / NoiseTracker MOD/NST file support

void CSoundFile::ConvertModCommand( MODCOMMAND* m ) const
//-----------------------------------------------------
{
	UINT command = m->command, param = m->param;

	switch ( command )
	{
		case 0x00:
			if ( param ) { command = CMD_ARPEGGIO; }

			break;

		case 0x01:
			command = CMD_PORTAMENTOUP;
			break;

		case 0x02:
			command = CMD_PORTAMENTODOWN;
			break;

		case 0x03:
			command = CMD_TONEPORTAMENTO;
			break;

		case 0x04:
			command = CMD_VIBRATO;
			break;

		case 0x05:
			command = CMD_TONEPORTAVOL;

			if ( param & 0xF0 ) { param &= 0xF0; }

			break;

		case 0x06:
			command = CMD_VIBRATOVOL;

			if ( param & 0xF0 ) { param &= 0xF0; }

			break;

		case 0x07:
			command = CMD_TREMOLO;
			break;

		case 0x08:
			command = CMD_PANNING8;
			break;

		case 0x09:
			command = CMD_OFFSET;
			break;

		case 0x0A:
			command = CMD_VOLUMESLIDE;

			if ( param & 0xF0 ) { param &= 0xF0; }

			break;

		case 0x0B:
			command = CMD_POSITIONJUMP;
			break;

		case 0x0C:
			command = CMD_VOLUME;
			break;

		case 0x0D:
			command = CMD_PATTERNBREAK;
			param = ( ( param >> 4 ) * 10 ) + ( param & 0x0F );
			break;

		case 0x0E:
			command = CMD_MODCMDEX;
			break;

		case 0x0F:
			command = ( param <= ( UINT )( ( m_nType & ( MOD_TYPE_XM | MOD_TYPE_MT2 ) ) ? 0x1F : 0x20 ) ) ? CMD_SPEED : CMD_TEMPO;

			if ( ( param == 0xFF ) && ( m_nSamples == 15 ) ) { command = 0; }

			break;

			// Extension for XM extended effects
		case 'G' - 55:
			command = CMD_GLOBALVOLUME;
			break;

		case 'H' - 55:
			command = CMD_GLOBALVOLSLIDE;

			if ( param & 0xF0 ) { param &= 0xF0; }

			break;

		case 'K' - 55:
			command = CMD_KEYOFF;
			break;

		case 'L' - 55:
			command = CMD_SETENVPOSITION;
			break;

		case 'M' - 55:
			command = CMD_CHANNELVOLUME;
			break;

		case 'N' - 55:
			command = CMD_CHANNELVOLSLIDE;
			break;

		case 'P' - 55:
			command = CMD_PANNINGSLIDE;

			if ( param & 0xF0 ) { param &= 0xF0; }

			break;

		case 'R' - 55:
			command = CMD_RETRIG;
			break;

		case 'T' - 55:
			command = CMD_TREMOR;
			break;

		case 'X' - 55:
			command = CMD_XFINEPORTAUPDOWN;
			break;

		case 'Y' - 55:
			command = CMD_PANBRELLO;
			break;

		case 'Z' - 55:
			command = CMD_MIDI;
			break;

		default:
			command = 0;
	}

	m->command = command;
	m->param = param;
}


WORD CSoundFile::ModSaveCommand( const MODCOMMAND* m, BOOL bXM ) const
//------------------------------------------------------------------
{
	UINT command = m->command & 0x3F, param = m->param;

	switch ( command )
	{
		case 0:
			command = param = 0;
			break;

		case CMD_ARPEGGIO:
			command = 0;
			break;

		case CMD_PORTAMENTOUP:
			if ( m_nType & ( MOD_TYPE_S3M | MOD_TYPE_IT | MOD_TYPE_STM ) )
			{
				if ( ( param & 0xF0 ) == 0xE0 ) { command = 0x0E; param = ( ( param & 0x0F ) >> 2 ) | 0x10; break; }
				else if ( ( param & 0xF0 ) == 0xF0 ) { command = 0x0E; param &= 0x0F; param |= 0x10; break; }
			}

			command = 0x01;
			break;

		case CMD_PORTAMENTODOWN:
			if ( m_nType & ( MOD_TYPE_S3M | MOD_TYPE_IT | MOD_TYPE_STM ) )
			{
				if ( ( param & 0xF0 ) == 0xE0 ) { command = 0x0E; param = ( ( param & 0x0F ) >> 2 ) | 0x20; break; }
				else if ( ( param & 0xF0 ) == 0xF0 ) { command = 0x0E; param &= 0x0F; param |= 0x20; break; }
			}

			command = 0x02;
			break;

		case CMD_TONEPORTAMENTO:
			command = 0x03;
			break;

		case CMD_VIBRATO:
			command = 0x04;
			break;

		case CMD_TONEPORTAVOL:
			command = 0x05;
			break;

		case CMD_VIBRATOVOL:
			command = 0x06;
			break;

		case CMD_TREMOLO:
			command = 0x07;
			break;

		case CMD_PANNING8:
			command = 0x08;

			if ( bXM )
			{
				if ( ( m_nType != MOD_TYPE_IT ) && ( m_nType != MOD_TYPE_XM ) && ( param <= 0x80 ) )
				{
					param <<= 1;

					if ( param > 255 ) { param = 255; }
				}
			}
			else
			{
				if ( ( m_nType == MOD_TYPE_IT ) || ( m_nType == MOD_TYPE_XM ) ) { param >>= 1; }
			}

			break;

		case CMD_OFFSET:
			command = 0x09;
			break;

		case CMD_VOLUMESLIDE:
			command = 0x0A;
			break;

		case CMD_POSITIONJUMP:
			command = 0x0B;
			break;

		case CMD_VOLUME:
			command = 0x0C;
			break;

		case CMD_PATTERNBREAK:
			command = 0x0D;
			param = ( ( param / 10 ) << 4 ) | ( param % 10 );
			break;

		case CMD_MODCMDEX:
			command = 0x0E;
			break;

		case CMD_SPEED:
			command = 0x0F;

			if ( param > 0x20 ) { param = 0x20; }

			break;

		case CMD_TEMPO:
			if ( param > 0x20 ) { command = 0x0F; break; }

		case CMD_GLOBALVOLUME:
			command = 'G' - 55;
			break;

		case CMD_GLOBALVOLSLIDE:
			command = 'H' - 55;
			break;

		case CMD_KEYOFF:
			command = 'K' - 55;
			break;

		case CMD_SETENVPOSITION:
			command = 'L' - 55;
			break;

		case CMD_CHANNELVOLUME:
			command = 'M' - 55;
			break;

		case CMD_CHANNELVOLSLIDE:
			command = 'N' - 55;
			break;

		case CMD_PANNINGSLIDE:
			command = 'P' - 55;
			break;

		case CMD_RETRIG:
			command = 'R' - 55;
			break;

		case CMD_TREMOR:
			command = 'T' - 55;
			break;

		case CMD_XFINEPORTAUPDOWN:
			command = 'X' - 55;
			break;

		case CMD_PANBRELLO:
			command = 'Y' - 55;
			break;

		case CMD_MIDI:
			command = 'Z' - 55;
			break;

		case CMD_S3MCMDEX:
			switch ( param & 0xF0 )
			{
				case 0x10:
					command = 0x0E;
					param = ( param & 0x0F ) | 0x30;
					break;

				case 0x20:
					command = 0x0E;
					param = ( param & 0x0F ) | 0x50;
					break;

				case 0x30:
					command = 0x0E;
					param = ( param & 0x0F ) | 0x40;
					break;

				case 0x40:
					command = 0x0E;
					param = ( param & 0x0F ) | 0x70;
					break;

				case 0x90:
					command = 'X' - 55;
					break;

				case 0xB0:
					command = 0x0E;
					param = ( param & 0x0F ) | 0x60;
					break;

				case 0xA0:
				case 0x50:
				case 0x70:
				case 0x60:
					command = param = 0;
					break;

				default:
					command = 0x0E;
					break;
			}

			break;

		default:
			command = param = 0;
	}

	return ( WORD )( ( command << 8 ) | ( param ) );
}


#pragma pack(1)

typedef struct _MODSAMPLE
{
	CHAR name[22];
	WORD length;
	BYTE finetune;
	BYTE volume;
	WORD loopstart;
	WORD looplen;
} MODSAMPLE, *PMODSAMPLE;

typedef struct _MODMAGIC
{
	BYTE nOrders;
	BYTE nRestartPos;
	BYTE Orders[128];
	char Magic[4];          // changed from CHAR
} MODMAGIC, *PMODMAGIC;

#pragma pack()

BOOL IsMagic( LPCSTR s1, LPCSTR s2 )
{
	return ( ( *( DWORD* )s1 ) == ( *( DWORD* )s2 ) ) ? TRUE : FALSE;
}


BOOL CSoundFile::ReadMod( const BYTE* lpStream, DWORD dwMemLength )
//---------------------------------------------------------------
{
	char s[1024];          // changed from CHAR
	DWORD dwMemPos, dwTotalSampleLen;
	PMODMAGIC pMagic;
	UINT nErr;

	if ( ( !lpStream ) || ( dwMemLength < 0x600 ) ) { return FALSE; }

	dwMemPos = 20;
	m_nSamples = 31;
	m_nChannels = 4;
	pMagic = ( PMODMAGIC )( lpStream + dwMemPos + sizeof( MODSAMPLE ) * 31 );
	// Check Mod Magic
	memcpy( s, pMagic->Magic, 4 );

	if ( ( IsMagic( s, "M.K." ) ) || ( IsMagic( s, "M!K!" ) )
	     || ( IsMagic( s, "M&K!" ) ) || ( IsMagic( s, "N.T." ) ) ) { m_nChannels = 4; }
	else if ( ( IsMagic( s, "CD81" ) ) || ( IsMagic( s, "OKTA" ) ) ) { m_nChannels = 8; }
	else if ( ( s[0] == 'F' ) && ( s[1] == 'L' ) && ( s[2] == 'T' ) && ( s[3] >= '4' ) && ( s[3] <= '9' ) ) { m_nChannels = s[3] - '0'; }
	else if ( ( s[0] >= '2' ) && ( s[0] <= '9' ) && ( s[1] == 'C' ) && ( s[2] == 'H' ) && ( s[3] == 'N' ) ) { m_nChannels = s[0] - '0'; }
	else if ( ( s[0] == '1' ) && ( s[1] >= '0' ) && ( s[1] <= '9' ) && ( s[2] == 'C' ) && ( s[3] == 'H' ) ) { m_nChannels = s[1] - '0' + 10; }
	else if ( ( s[0] == '2' ) && ( s[1] >= '0' ) && ( s[1] <= '9' ) && ( s[2] == 'C' ) && ( s[3] == 'H' ) ) { m_nChannels = s[1] - '0' + 20; }
	else if ( ( s[0] == '3' ) && ( s[1] >= '0' ) && ( s[1] <= '2' ) && ( s[2] == 'C' ) && ( s[3] == 'H' ) ) { m_nChannels = s[1] - '0' + 30; }
	else if ( ( s[0] == 'T' ) && ( s[1] == 'D' ) && ( s[2] == 'Z' ) && ( s[3] >= '4' ) && ( s[3] <= '9' ) ) { m_nChannels = s[3] - '0'; }
	else if ( IsMagic( s, "16CN" ) ) { m_nChannels = 16; }
	else if ( IsMagic( s, "32CN" ) ) { m_nChannels = 32; }
	else { m_nSamples = 15; }

	// Load Samples
	nErr = 0;
	dwTotalSampleLen = 0;

	for   ( UINT i = 1; i <= m_nSamples; i++ )
	{
		PMODSAMPLE pms = ( PMODSAMPLE )( lpStream + dwMemPos );
		MODINSTRUMENT* psmp = &Ins[i];
		UINT loopstart, looplen;

		memcpy( m_szNames[i], pms->name, 22 );
		m_szNames[i][22] = 0;
		psmp->uFlags = 0;
		psmp->nLength = bswapBE16( pms->length ) * 2;
		dwTotalSampleLen += psmp->nLength;
		psmp->nFineTune = MOD2XMFineTune( pms->finetune & 0x0F );
		psmp->nVolume = 4 * pms->volume;

		if ( psmp->nVolume > 256 ) { psmp->nVolume = 256; nErr++; }

		psmp->nGlobalVol = 64;
		psmp->nPan = 128;
		loopstart = bswapBE16( pms->loopstart ) * 2;
		looplen = bswapBE16( pms->looplen ) * 2;

		// Fix loops
		if ( ( looplen > 2 ) && ( loopstart + looplen > psmp->nLength )
		     && ( loopstart / 2 + looplen <= psmp->nLength ) )
		{
			loopstart /= 2;
		}

		psmp->nLoopStart = loopstart;
		psmp->nLoopEnd = loopstart + looplen;

		if ( psmp->nLength < 4 ) { psmp->nLength = 0; }

		if ( psmp->nLength )
		{
			UINT derr = 0;

			if ( psmp->nLoopStart >= psmp->nLength ) { psmp->nLoopStart = psmp->nLength - 1; derr |= 1; }

			if ( psmp->nLoopEnd > psmp->nLength ) { psmp->nLoopEnd = psmp->nLength; derr |= 1; }

			if ( psmp->nLoopStart > psmp->nLoopEnd ) { derr |= 1; }

			if ( ( psmp->nLoopStart > psmp->nLoopEnd ) || ( psmp->nLoopEnd <= 8 )
			     || ( psmp->nLoopEnd - psmp->nLoopStart <= 4 ) )
			{
				psmp->nLoopStart = 0;
				psmp->nLoopEnd = 0;
			}

			if ( psmp->nLoopEnd > psmp->nLoopStart )
			{
				psmp->uFlags |= CHN_LOOP;
			}
		}

		dwMemPos += sizeof( MODSAMPLE );
	}

	if ( ( m_nSamples == 15 ) && ( dwTotalSampleLen > dwMemLength * 4 ) ) { return FALSE; }

	pMagic = ( PMODMAGIC )( lpStream + dwMemPos );
	dwMemPos += sizeof( MODMAGIC );

	if ( m_nSamples == 15 ) { dwMemPos -= 4; }

	memset( Order, 0, sizeof( Order ) );
	memcpy( Order, pMagic->Orders, 128 );

	UINT nbp, nbpbuggy, nbpbuggy2, norders;

	norders = pMagic->nOrders;

	if ( ( !norders ) || ( norders > 0x80 ) )
	{
		norders = 0x80;

		while ( ( norders > 1 ) && ( !Order[norders - 1] ) ) { norders--; }
	}

	nbpbuggy = 0;
	nbpbuggy2 = 0;
	nbp = 0;

	for ( UINT iord = 0; iord < 128; iord++ )
	{
		UINT i = Order[iord];

		if ( ( i < 0x80 ) && ( nbp <= i ) )
		{
			nbp = i + 1;

			if ( iord < norders ) { nbpbuggy = nbp; }
		}

		if ( i >= nbpbuggy2 ) { nbpbuggy2 = i + 1; }
	}

	for ( UINT iend = norders; iend < MAX_ORDERS; iend++ ) { Order[iend] = 0xFF; }

	norders--;
	m_nRestartPos = pMagic->nRestartPos;

	if ( m_nRestartPos >= 0x78 ) { m_nRestartPos = 0; }

	if ( m_nRestartPos + 1 >= ( UINT )norders ) { m_nRestartPos = 0; }

	if ( !nbp ) { return FALSE; }

	DWORD dwWowTest = dwTotalSampleLen + dwMemPos;

	if ( ( IsMagic( pMagic->Magic, "M.K." ) ) && ( dwWowTest + nbp * 8 * 256 == dwMemLength ) ) { m_nChannels = 8; }

	if ( ( nbp != nbpbuggy ) && ( dwWowTest + nbp * m_nChannels * 256 != dwMemLength ) )
	{
		if ( dwWowTest + nbpbuggy * m_nChannels * 256 == dwMemLength ) { nbp = nbpbuggy; }
		else { nErr += 8; }
	}
	else if ( ( nbpbuggy2 > nbp ) && ( dwWowTest + nbpbuggy2 * m_nChannels * 256 == dwMemLength ) )
	{
		nbp = nbpbuggy2;
	}

	if ( ( dwWowTest < 0x600 ) || ( dwWowTest > dwMemLength ) ) { nErr += 8; }

	if ( ( m_nSamples == 15 ) && ( nErr >= 16 ) ) { return FALSE; }

	// Default settings
	m_nType = MOD_TYPE_MOD;
	m_nDefaultSpeed = 6;
	m_nDefaultTempo = 125;
	m_nMinPeriod = 14 << 2;
	m_nMaxPeriod = 3424 << 2;
	memcpy( m_szNames, lpStream, 20 );

	// Setting channels pan
	for ( UINT ich = 0; ich < m_nChannels; ich++ )
	{
		ChnSettings[ich].nVolume = 64;

		if ( gdwSoundSetup & SNDMIX_MAXDEFAULTPAN )
		{
			ChnSettings[ich].nPan = ( ( ( ich & 3 ) == 1 ) || ( ( ich & 3 ) == 2 ) ) ? 256 : 0;
		}
		else
		{
			ChnSettings[ich].nPan = ( ( ( ich & 3 ) == 1 ) || ( ( ich & 3 ) == 2 ) ) ? 0xC0 : 0x40;
		}
	}

	// Reading channels
	for ( UINT ipat = 0; ipat < nbp; ipat++ )
	{
		if ( ipat < MAX_PATTERNS )
		{
			if ( ( Patterns[ipat] = AllocatePattern( 64, m_nChannels ) ) == NULL ) { break; }

			PatternSize[ipat] = 64;

			if ( dwMemPos + m_nChannels * 256 >= dwMemLength ) { break; }

			MODCOMMAND* m = Patterns[ipat];
			LPCBYTE p = lpStream + dwMemPos;

			for ( UINT j = m_nChannels * 64; j; m++, p += 4, j-- )
			{
				BYTE A0 = p[0], A1 = p[1], A2 = p[2], A3 = p[3];
				UINT n = ( ( ( ( UINT )A0 & 0x0F ) << 8 ) | ( A1 ) );

				if ( ( n ) && ( n != 0xFFF ) ) { m->note = GetNoteFromPeriod( n << 2 ); }

				m->instr = ( ( UINT )A2 >> 4 ) | ( A0 & 0x10 );
				m->command = A2 & 0x0F;
				m->param = A3;

				if ( ( m->command ) || ( m->param ) ) { ConvertModCommand( m ); }
			}
		}

		dwMemPos += m_nChannels * 256;
	}

	// Reading instruments
	DWORD dwErrCheck = 0;

	for ( UINT ismp = 1; ismp <= m_nSamples; ismp++ ) if ( Ins[ismp].nLength )
		{
			LPSTR p = ( LPSTR )( lpStream + dwMemPos );
			UINT flags = 0;

			if ( dwMemPos + 5 >= dwMemLength ) { break; }

			if ( !strnicmp( p, "ADPCM", 5 ) )
			{
				flags = 3;
				p += 5;
				dwMemPos += 5;
			}

			DWORD dwSize = ReadSample( &Ins[ismp], flags, p, dwMemLength - dwMemPos );

			if ( dwSize )
			{
				dwMemPos += dwSize;
				dwErrCheck++;
			}
		}

#ifdef MODPLUG_TRACKER
	return TRUE;
#else
	return ( dwErrCheck ) ? TRUE : FALSE;
#endif
}


#ifndef MODPLUG_NO_FILESAVE

#ifdef _MSC_VER
#pragma warning(disable:4100)
#endif

BOOL CSoundFile::SaveMod( LPCSTR lpszFileName, UINT nPacking )
//----------------------------------------------------------
{
	BYTE insmap[32];
	UINT inslen[32];
	BYTE bTab[32];
	BYTE ord[128];
	FILE* f;

	if ( ( !m_nChannels ) || ( !lpszFileName ) ) { return FALSE; }

	if ( ( f = fopen( lpszFileName, "wb" ) ) == NULL ) { return FALSE; }

	memset( ord, 0, sizeof( ord ) );
	memset( inslen, 0, sizeof( inslen ) );

	if ( m_nInstruments )
	{
		memset( insmap, 0, sizeof( insmap ) );

		for ( UINT i = 1; i < 32; i++ ) if ( Headers[i] )
			{
				for ( UINT j = 0; j < 128; j++ ) if ( Headers[i]->Keyboard[j] )
					{
						insmap[i] = Headers[i]->Keyboard[j];
						break;
					}
			}
	}
	else
	{
		for ( UINT i = 0; i < 32; i++ ) { insmap[i] = ( BYTE )i; }
	}

	// Writing song name
	fwrite( m_szNames, 20, 1, f );

	// Writing instrument definition
	for ( UINT iins = 1; iins <= 31; iins++ )
	{
		MODINSTRUMENT* pins = &Ins[insmap[iins]];
		memcpy( bTab, m_szNames[iins], 22 );
		inslen[iins] = pins->nLength;

		if ( inslen[iins] > 0x1fff0 ) { inslen[iins] = 0x1fff0; }

		bTab[22] = inslen[iins] >> 9;
		bTab[23] = inslen[iins] >> 1;

		if ( pins->RelativeTone < 0 ) { bTab[24] = 0x08; }
		else if ( pins->RelativeTone > 0 ) { bTab[24] = 0x07; }
		else
		{
			bTab[24] = ( BYTE )XM2MODFineTune( pins->nFineTune );
		}

		bTab[25] = pins->nVolume >> 2;
		bTab[26] = pins->nLoopStart >> 9;
		bTab[27] = pins->nLoopStart >> 1;
		bTab[28] = ( pins->nLoopEnd - pins->nLoopStart ) >> 9;
		bTab[29] = ( pins->nLoopEnd - pins->nLoopStart ) >> 1;
		fwrite( bTab, 30, 1, f );
	}

	// Writing number of patterns
	UINT nbp = 0, norders = 128;

	for ( UINT iord = 0; iord < 128; iord++ )
	{
		if ( Order[iord] == 0xFF )
		{
			norders = iord;
			break;
		}

		if ( ( Order[iord] < 0x80 ) && ( nbp <= Order[iord] ) ) { nbp = Order[iord] + 1; }
	}

	bTab[0] = norders;
	bTab[1] = m_nRestartPos;
	fwrite( bTab, 2, 1, f );

	// Writing pattern list
	if ( norders ) { memcpy( ord, Order, norders ); }

	fwrite( ord, 128, 1, f );

	// Writing signature
	if ( m_nChannels == 4 )
	{
		lstrcpy( ( LPSTR )&bTab, "M.K." );
	}
	else
	{
		wsprintf( ( LPSTR )&bTab, "%luCHN", m_nChannels );
	}

	fwrite( bTab, 4, 1, f );

	// Writing patterns
	for ( UINT ipat = 0; ipat < nbp; ipat++ ) if ( Patterns[ipat] )
		{
			BYTE s[64 * 4];
			MODCOMMAND* m = Patterns[ipat];

			for ( UINT i = 0; i < 64; i++ ) if ( i < PatternSize[ipat] )
				{
					LPBYTE p = s;

					for ( UINT c = 0; c < m_nChannels; c++, p += 4, m++ )
					{
						UINT param = ModSaveCommand( m, FALSE );
						UINT command = param >> 8;
						param &= 0xFF;

						if ( command > 0x0F ) { command = param = 0; }

						if ( ( m->vol >= 0x10 ) && ( m->vol <= 0x50 ) && ( !command ) && ( !param ) ) { command = 0x0C; param = m->vol - 0x10; }

						UINT period = m->note;

						if ( period )
						{
							if ( period < 37 ) { period = 37; }

							period -= 37;

							if ( period >= 6 * 12 ) { period = 6 * 12 - 1; }

							period = ProTrackerPeriodTable[period];
						}

						UINT instr = ( m->instr > 31 ) ? 0 : m->instr;
						p[0] = ( ( period >> 8 ) & 0x0F ) | ( instr & 0x10 );
						p[1] = period & 0xFF;
						p[2] = ( ( instr & 0x0F ) << 4 ) | ( command & 0x0F );
						p[3] = param;
					}

					fwrite( s, m_nChannels, 4, f );
				}
				else
				{
					memset( s, 0, m_nChannels * 4 );
					fwrite( s, m_nChannels, 4, f );
				}
		}

	// Writing instruments
	for ( UINT ismpd = 1; ismpd <= 31; ismpd++ ) if ( inslen[ismpd] )
		{
			MODINSTRUMENT* pins = &Ins[insmap[ismpd]];
			UINT flags = RS_PCM8S;
#ifndef NO_PACKING

			if ( !( pins->uFlags & ( CHN_16BIT | CHN_STEREO ) ) )
			{
				if ( ( nPacking ) && ( CanPackSample( ( char* )pins->pSample, inslen[ismpd], nPacking ) ) )
				{
					fwrite( "ADPCM", 1, 5, f );
					flags = RS_ADPCM4;
				}
			}

#endif
			WriteSample( f, pins, flags, inslen[ismpd] );
		}

	fclose( f );
	return TRUE;
}

#ifdef _MSC_VER
#pragma warning(default:4100)
#endif

#endif // MODPLUG_NO_FILESAVE

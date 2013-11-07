///////// DSP

/*
 * This source code is public domain.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>
 *          Markus Fick <webmaster@mark-f.de> spline + fir-resampler
*/

#include <stdlib.h>
#include <math.h>

#include "stdafx.h"
#include "sndfile.h"
#include "tables.h"

#ifdef MODPLUG_FASTSOUNDLIB
#define MODPLUG_NO_REVERB
#endif


// Delayed Surround Filters
#ifndef MODPLUG_FASTSOUNDLIB
#define nDolbyHiFltAttn    6
#define nDolbyHiFltMask    3
#define DOLBYATTNROUNDUP   31
#else
#define nDolbyHiFltAttn    3
#define nDolbyHiFltMask    3
#define DOLBYATTNROUNDUP   3
#endif

// Bass Expansion
#define XBASS_DELAY        14 // 2.5 ms

// Buffer Sizes
#define XBASSBUFFERSIZE    64    // 2 ms at 50KHz
#define FILTERBUFFERSIZE   64    // 1.25 ms
#define SURROUNDBUFFERSIZE ((MAX_SAMPLE_RATE * 50) / 1000)
#define REVERBBUFFERSIZE   ((MAX_SAMPLE_RATE * 200) / 1000)
#define REVERBBUFFERSIZE2  ((REVERBBUFFERSIZE*13) / 17)
#define REVERBBUFFERSIZE3  ((REVERBBUFFERSIZE*7) / 13)
#define REVERBBUFFERSIZE4  ((REVERBBUFFERSIZE*7) / 19)


// DSP Effects: PUBLIC members
UINT CSoundFile::m_nXBassDepth = 6;
UINT CSoundFile::m_nXBassRange = XBASS_DELAY;
UINT CSoundFile::m_nReverbDepth = 1;
UINT CSoundFile::m_nReverbDelay = 100;
UINT CSoundFile::m_nProLogicDepth = 12;
UINT CSoundFile::m_nProLogicDelay = 20;

////////////////////////////////////////////////////////////////////
// DSP Effects internal state

// Bass Expansion: low-pass filter
static LONG nXBassSum = 0;
static LONG nXBassBufferPos = 0;
static LONG nXBassDlyPos = 0;
static LONG nXBassMask = 0;

// Noise Reduction: simple low-pass filter
static LONG nLeftNR = 0;
static LONG nRightNR = 0;

// Surround Encoding: 1 delay line + low-pass filter + high-pass filter
static LONG nSurroundSize = 0;
static LONG nSurroundPos = 0;
static LONG nDolbyDepth = 0;
static LONG nDolbyLoDlyPos = 0;
static LONG nDolbyLoFltPos = 0;
static LONG nDolbyLoFltSum = 0;
static LONG nDolbyHiFltPos = 0;
static LONG nDolbyHiFltSum = 0;

// Reverb: 4 delay lines + high-pass filter + low-pass filter
#ifndef MODPLUG_NO_REVERB
static LONG nReverbSize = 0;
static LONG nReverbBufferPos = 0;
static LONG nReverbSize2 = 0;
static LONG nReverbBufferPos2 = 0;
static LONG nReverbSize3 = 0;
static LONG nReverbBufferPos3 = 0;
static LONG nReverbSize4 = 0;
static LONG nReverbBufferPos4 = 0;
static LONG nReverbLoFltSum = 0;
static LONG nReverbLoFltPos = 0;
static LONG nReverbLoDlyPos = 0;
static LONG nFilterAttn = 0;
static LONG gRvbLowPass[8];
static LONG gRvbLPPos = 0;
static LONG gRvbLPSum = 0;
static LONG ReverbLoFilterBuffer[XBASSBUFFERSIZE];
static LONG ReverbLoFilterDelay[XBASSBUFFERSIZE];
static LONG ReverbBuffer[REVERBBUFFERSIZE];
static LONG ReverbBuffer2[REVERBBUFFERSIZE2];
static LONG ReverbBuffer3[REVERBBUFFERSIZE3];
static LONG ReverbBuffer4[REVERBBUFFERSIZE4];
#endif
static LONG XBassBuffer[XBASSBUFFERSIZE];
static LONG XBassDelay[XBASSBUFFERSIZE];
static LONG DolbyLoFilterBuffer[XBASSBUFFERSIZE];
static LONG DolbyLoFilterDelay[XBASSBUFFERSIZE];
static LONG DolbyHiFilterBuffer[FILTERBUFFERSIZE];
static LONG SurroundBuffer[SURROUNDBUFFERSIZE];

// Access the main temporary mix buffer directly: avoids an extra pointer
extern int MixSoundBuffer[MIXBUFFERSIZE * 4/*2*/];
//cextern int MixReverbBuffer[MIXBUFFERSIZE*2];
extern int MixReverbBuffer[MIXBUFFERSIZE * 2];

static UINT GetMaskFromSize( UINT len )
//-----------------------------------
{
	UINT n = 2;

	while ( n <= len ) { n <<= 1; }

	return ( ( n >> 1 ) - 1 );
}


void CSoundFile::InitializeDSP( BOOL bReset )
//-----------------------------------------
{
	if ( !m_nReverbDelay ) { m_nReverbDelay = 100; }

	if ( !m_nXBassRange ) { m_nXBassRange = XBASS_DELAY; }

	if ( !m_nProLogicDelay ) { m_nProLogicDelay = 20; }

	if ( m_nXBassDepth > 8 ) { m_nXBassDepth = 8; }

	if ( m_nXBassDepth < 2 ) { m_nXBassDepth = 2; }

	if ( bReset )
	{
		// Noise Reduction
		nLeftNR = nRightNR = 0;
	}

	// Pro-Logic Surround
	nSurroundPos = nSurroundSize = 0;
	nDolbyLoFltPos = nDolbyLoFltSum = nDolbyLoDlyPos = 0;
	nDolbyHiFltPos = nDolbyHiFltSum = 0;

	if ( gdwSoundSetup & SNDMIX_SURROUND )
	{
		memset( DolbyLoFilterBuffer, 0, sizeof( DolbyLoFilterBuffer ) );
		memset( DolbyHiFilterBuffer, 0, sizeof( DolbyHiFilterBuffer ) );
		memset( DolbyLoFilterDelay, 0, sizeof( DolbyLoFilterDelay ) );
		memset( SurroundBuffer, 0, sizeof( SurroundBuffer ) );
		nSurroundSize = ( gdwMixingFreq * m_nProLogicDelay ) / 1000;

		if ( nSurroundSize > SURROUNDBUFFERSIZE ) { nSurroundSize = SURROUNDBUFFERSIZE; }

		if ( m_nProLogicDepth < 8 ) { nDolbyDepth = ( 32 >> m_nProLogicDepth ) + 32; }
		else { nDolbyDepth = ( m_nProLogicDepth < 16 ) ? ( 8 + ( m_nProLogicDepth - 8 ) * 7 ) : 64; }

		nDolbyDepth >>= 2;
	}

	// Reverb Setup
#ifndef MODPLUG_NO_REVERB

	if ( gdwSoundSetup & SNDMIX_REVERB )
	{
		UINT nrs = ( gdwMixingFreq * m_nReverbDelay ) / 1000;
		UINT nfa = m_nReverbDepth + 1;

		if ( nrs > REVERBBUFFERSIZE ) { nrs = REVERBBUFFERSIZE; }

		if ( ( bReset ) || ( nrs != ( UINT )nReverbSize ) || ( nfa != ( UINT )nFilterAttn ) )
		{
			nFilterAttn = nfa;
			nReverbSize = nrs;
			nReverbBufferPos = nReverbBufferPos2 = nReverbBufferPos3 = nReverbBufferPos4 = 0;
			nReverbLoFltSum = nReverbLoFltPos = nReverbLoDlyPos = 0;
			gRvbLPSum = gRvbLPPos = 0;
			nReverbSize2 = ( nReverbSize * 13 ) / 17;

			if ( nReverbSize2 > REVERBBUFFERSIZE2 ) { nReverbSize2 = REVERBBUFFERSIZE2; }

			nReverbSize3 = ( nReverbSize * 7 ) / 13;

			if ( nReverbSize3 > REVERBBUFFERSIZE3 ) { nReverbSize3 = REVERBBUFFERSIZE3; }

			nReverbSize4 = ( nReverbSize * 7 ) / 19;

			if ( nReverbSize4 > REVERBBUFFERSIZE4 ) { nReverbSize4 = REVERBBUFFERSIZE4; }

			memset( ReverbLoFilterBuffer, 0, sizeof( ReverbLoFilterBuffer ) );
			memset( ReverbLoFilterDelay, 0, sizeof( ReverbLoFilterDelay ) );
			memset( ReverbBuffer, 0, sizeof( ReverbBuffer ) );
			memset( ReverbBuffer2, 0, sizeof( ReverbBuffer2 ) );
			memset( ReverbBuffer3, 0, sizeof( ReverbBuffer3 ) );
			memset( ReverbBuffer4, 0, sizeof( ReverbBuffer4 ) );
			memset( gRvbLowPass, 0, sizeof( gRvbLowPass ) );
		}
	}
	else { nReverbSize = 0; }

#endif
	BOOL bResetBass = FALSE;

	// Bass Expansion Reset
	if ( gdwSoundSetup & SNDMIX_MEGABASS )
	{
		UINT nXBassSamples = ( gdwMixingFreq * m_nXBassRange ) / 10000;

		if ( nXBassSamples > XBASSBUFFERSIZE ) { nXBassSamples = XBASSBUFFERSIZE; }

		UINT mask = GetMaskFromSize( nXBassSamples );

		if ( ( bReset ) || ( mask != ( UINT )nXBassMask ) )
		{
			nXBassMask = mask;
			bResetBass = TRUE;
		}
	}
	else
	{
		nXBassMask = 0;
		bResetBass = TRUE;
	}

	if ( bResetBass )
	{
		nXBassSum = nXBassBufferPos = nXBassDlyPos = 0;
		memset( XBassBuffer, 0, sizeof( XBassBuffer ) );
		memset( XBassDelay, 0, sizeof( XBassDelay ) );
	}
}


void CSoundFile::ProcessStereoDSP( int count )
//------------------------------------------
{
#ifndef MODPLUG_NO_REVERB

	// Reverb
	if ( gdwSoundSetup & SNDMIX_REVERB )
	{
		int* pr = MixSoundBuffer, *pin = MixReverbBuffer, rvbcount = count;

		do
		{
			int echo = ReverbBuffer[nReverbBufferPos] + ReverbBuffer2[nReverbBufferPos2]
			           + ReverbBuffer3[nReverbBufferPos3] + ReverbBuffer4[nReverbBufferPos4];  // echo = reverb signal
			// Delay line and remove Low Frequencies        // v = original signal
			int echodly = ReverbLoFilterDelay[nReverbLoDlyPos];   // echodly = delayed signal
			ReverbLoFilterDelay[nReverbLoDlyPos] = echo >> 1;
			nReverbLoDlyPos++;
			nReverbLoDlyPos &= 0x1F;
			int n = nReverbLoFltPos;
			nReverbLoFltSum -= ReverbLoFilterBuffer[n];
			int tmp = echo / 128;
			ReverbLoFilterBuffer[n] = tmp;
			nReverbLoFltSum += tmp;
			echodly -= nReverbLoFltSum;
			nReverbLoFltPos = ( n + 1 ) & 0x3F;
			// Reverb
			int v = ( pin[0] + pin[1] ) >> nFilterAttn;
			pr[0] += pin[0] + echodly;
			pr[1] += pin[1] + echodly;
			v += echodly >> 2;
			ReverbBuffer3[nReverbBufferPos3] = v;
			ReverbBuffer4[nReverbBufferPos4] = v;
			v += echodly >> 4;
			v >>= 1;
			gRvbLPSum -= gRvbLowPass[gRvbLPPos];
			gRvbLPSum += v;
			gRvbLowPass[gRvbLPPos] = v;
			gRvbLPPos++;
			gRvbLPPos &= 7;
			int vlp = gRvbLPSum >> 2;
			ReverbBuffer[nReverbBufferPos] = vlp;
			ReverbBuffer2[nReverbBufferPos2] = vlp;

			if ( ++nReverbBufferPos >= nReverbSize ) { nReverbBufferPos = 0; }

			if ( ++nReverbBufferPos2 >= nReverbSize2 ) { nReverbBufferPos2 = 0; }

			if ( ++nReverbBufferPos3 >= nReverbSize3 ) { nReverbBufferPos3 = 0; }

			if ( ++nReverbBufferPos4 >= nReverbSize4 ) { nReverbBufferPos4 = 0; }

			pr += 2;
			pin += 2;
		}
		while ( --rvbcount );
	}

#endif

	// Dolby Pro-Logic Surround
	if ( gdwSoundSetup & SNDMIX_SURROUND )
	{
		int* pr = MixSoundBuffer, n = nDolbyLoFltPos;

		for ( int r = count; r; r-- )
		{
			int v = ( pr[0] + pr[1] + DOLBYATTNROUNDUP ) >> ( nDolbyHiFltAttn + 1 );
#ifndef MODPLUG_FASTSOUNDLIB
			v *= ( int )nDolbyDepth;
#endif
			// Low-Pass Filter
			nDolbyHiFltSum -= DolbyHiFilterBuffer[nDolbyHiFltPos];
			DolbyHiFilterBuffer[nDolbyHiFltPos] = v;
			nDolbyHiFltSum += v;
			v = nDolbyHiFltSum;
			nDolbyHiFltPos++;
			nDolbyHiFltPos &= nDolbyHiFltMask;
			// Surround
			int secho = SurroundBuffer[nSurroundPos];
			SurroundBuffer[nSurroundPos] = v;
			// Delay line and remove low frequencies
			v = DolbyLoFilterDelay[nDolbyLoDlyPos];      // v = delayed signal
			DolbyLoFilterDelay[nDolbyLoDlyPos] = secho;  // secho = signal
			nDolbyLoDlyPos++;
			nDolbyLoDlyPos &= 0x1F;
			nDolbyLoFltSum -= DolbyLoFilterBuffer[n];
			int tmp = secho / 64;
			DolbyLoFilterBuffer[n] = tmp;
			nDolbyLoFltSum += tmp;
			v -= nDolbyLoFltSum;
			n++;
			n &= 0x3F;
			// Add echo
			pr[0] += v;
			pr[1] -= v;

			if ( ++nSurroundPos >= nSurroundSize ) { nSurroundPos = 0; }

			pr += 2;
		}

		nDolbyLoFltPos = n;
	}

	// Bass Expansion
	if ( gdwSoundSetup & SNDMIX_MEGABASS )
	{
		int* px = MixSoundBuffer;
		int xba = m_nXBassDepth + 1, xbamask = ( 1 << xba ) - 1;
		int n = nXBassBufferPos;

		for ( int x = count; x; x-- )
		{
			nXBassSum -= XBassBuffer[n];
			int tmp0 = px[0] + px[1];
			int tmp = ( tmp0 + ( ( tmp0 >> 31 ) & xbamask ) ) >> xba;
			XBassBuffer[n] = tmp;
			nXBassSum += tmp;
			int v = XBassDelay[nXBassDlyPos];
			XBassDelay[nXBassDlyPos] = px[0];
			px[0] = v + nXBassSum;
			v = XBassDelay[nXBassDlyPos + 1];
			XBassDelay[nXBassDlyPos + 1] = px[1];
			px[1] = v + nXBassSum;
			nXBassDlyPos = ( nXBassDlyPos + 2 ) & nXBassMask;
			px += 2;
			n++;
			n &= nXBassMask;
		}

		nXBassBufferPos = n;
	}

	// Noise Reduction
	if ( gdwSoundSetup & SNDMIX_NOISEREDUCTION )
	{
		int n1 = nLeftNR, n2 = nRightNR;
		int* pnr = MixSoundBuffer;

		for ( int nr = count; nr; nr-- )
		{
			int vnr = pnr[0] >> 1;
			pnr[0] = vnr + n1;
			n1 = vnr;
			vnr = pnr[1] >> 1;
			pnr[1] = vnr + n2;
			n2 = vnr;
			pnr += 2;
		}

		nLeftNR = n1;
		nRightNR = n2;
	}
}


void CSoundFile::ProcessMonoDSP( int count )
//----------------------------------------
{
#ifndef MODPLUG_NO_REVERB

	// Reverb
	if ( gdwSoundSetup & SNDMIX_REVERB )
	{
		int* pr = MixSoundBuffer, rvbcount = count, *pin = MixReverbBuffer;

		do
		{
			int echo = ReverbBuffer[nReverbBufferPos] + ReverbBuffer2[nReverbBufferPos2]
			           + ReverbBuffer3[nReverbBufferPos3] + ReverbBuffer4[nReverbBufferPos4];  // echo = reverb signal
			// Delay line and remove Low Frequencies        // v = original signal
			int echodly = ReverbLoFilterDelay[nReverbLoDlyPos];   // echodly = delayed signal
			ReverbLoFilterDelay[nReverbLoDlyPos] = echo >> 1;
			nReverbLoDlyPos++;
			nReverbLoDlyPos &= 0x1F;
			int n = nReverbLoFltPos;
			nReverbLoFltSum -= ReverbLoFilterBuffer[n];
			int tmp = echo / 128;
			ReverbLoFilterBuffer[n] = tmp;
			nReverbLoFltSum += tmp;
			echodly -= nReverbLoFltSum;
			nReverbLoFltPos = ( n + 1 ) & 0x3F;
			// Reverb
			int v = pin[0] >> ( nFilterAttn - 1 );
			*pr++ += pin[0] + echodly;
			pin++;
			v += echodly >> 2;
			ReverbBuffer3[nReverbBufferPos3] = v;
			ReverbBuffer4[nReverbBufferPos4] = v;
			v += echodly >> 4;
			v >>= 1;
			gRvbLPSum -= gRvbLowPass[gRvbLPPos];
			gRvbLPSum += v;
			gRvbLowPass[gRvbLPPos] = v;
			gRvbLPPos++;
			gRvbLPPos &= 7;
			int vlp = gRvbLPSum >> 2;
			ReverbBuffer[nReverbBufferPos] = vlp;
			ReverbBuffer2[nReverbBufferPos2] = vlp;

			if ( ++nReverbBufferPos >= nReverbSize ) { nReverbBufferPos = 0; }

			if ( ++nReverbBufferPos2 >= nReverbSize2 ) { nReverbBufferPos2 = 0; }

			if ( ++nReverbBufferPos3 >= nReverbSize3 ) { nReverbBufferPos3 = 0; }

			if ( ++nReverbBufferPos4 >= nReverbSize4 ) { nReverbBufferPos4 = 0; }
		}
		while ( --rvbcount );
	}

#endif

	// Bass Expansion
	if ( gdwSoundSetup & SNDMIX_MEGABASS )
	{
		int* px = MixSoundBuffer;
		int xba = m_nXBassDepth, xbamask = ( 1 << xba ) - 1;
		int n = nXBassBufferPos;

		for ( int x = count; x; x-- )
		{
			nXBassSum -= XBassBuffer[n];
			int tmp0 = *px;
			int tmp = ( tmp0 + ( ( tmp0 >> 31 ) & xbamask ) ) >> xba;
			XBassBuffer[n] = tmp;
			nXBassSum += tmp;
			int v = XBassDelay[nXBassDlyPos];
			XBassDelay[nXBassDlyPos] = *px;
			*px++ = v + nXBassSum;
			nXBassDlyPos = ( nXBassDlyPos + 2 ) & nXBassMask;
			n++;
			n &= nXBassMask;
		}

		nXBassBufferPos = n;
	}

	// Noise Reduction
	if ( gdwSoundSetup & SNDMIX_NOISEREDUCTION )
	{
		int n = nLeftNR;
		int* pnr = MixSoundBuffer;

		for ( int nr = count; nr; pnr++, nr-- )
		{
			int vnr = *pnr >> 1;
			*pnr = vnr + n;
			n = vnr;
		}

		nLeftNR = n;
	}
}


/////////////////////////////////////////////////////////////////
// Clean DSP Effects interface

// [Reverb level 0(quiet)-100(loud)], [delay in ms, usually 40-200ms]
BOOL CSoundFile::SetReverbParameters( UINT nDepth, UINT nDelay )
//------------------------------------------------------------
{
	if ( nDepth > 100 ) { nDepth = 100; }

	UINT gain = nDepth / 20;

	if ( gain > 4 ) { gain = 4; }

	m_nReverbDepth = 4 - gain;

	if ( nDelay < 40 ) { nDelay = 40; }

	if ( nDelay > 250 ) { nDelay = 250; }

	m_nReverbDelay = nDelay;
	return TRUE;
}


// [XBass level 0(quiet)-100(loud)], [cutoff in Hz 20-100]
BOOL CSoundFile::SetXBassParameters( UINT nDepth, UINT nRange )
//-----------------------------------------------------------
{
	if ( nDepth > 100 ) { nDepth = 100; }

	UINT gain = nDepth / 20;

	if ( gain > 4 ) { gain = 4; }

	m_nXBassDepth = 8 - gain;  // filter attenuation 1/256 .. 1/16
	UINT range = nRange / 5;

	if ( range > 5 ) { range -= 5; }
	else { range = 0; }

	if ( nRange > 16 ) { nRange = 16; }

	m_nXBassRange = 21 - range;   // filter average on 0.5-1.6ms
	return TRUE;
}


// [Surround level 0(quiet)-100(heavy)] [delay in ms, usually 5-50ms]
BOOL CSoundFile::SetSurroundParameters( UINT nDepth, UINT nDelay )
//--------------------------------------------------------------
{
	UINT gain = ( nDepth * 16 ) / 100;

	if ( gain > 16 ) { gain = 16; }

	if ( gain < 1 ) { gain = 1; }

	m_nProLogicDepth = gain;

	if ( nDelay < 4 ) { nDelay = 4; }

	if ( nDelay > 50 ) { nDelay = 50; }

	m_nProLogicDelay = nDelay;
	return TRUE;
}

BOOL CSoundFile::SetWaveConfigEx( BOOL bSurround, BOOL bNoOverSampling, BOOL bReverb, BOOL hqido, BOOL bMegaBass, BOOL bNR, BOOL bEQ )
//----------------------------------------------------------------------------------------------------------------------------
{
	DWORD d = gdwSoundSetup & ~( SNDMIX_SURROUND | SNDMIX_NORESAMPLING | SNDMIX_REVERB | SNDMIX_HQRESAMPLER | SNDMIX_MEGABASS | SNDMIX_NOISEREDUCTION | SNDMIX_EQ );

	if ( bSurround ) { d |= SNDMIX_SURROUND; }

	if ( bNoOverSampling ) { d |= SNDMIX_NORESAMPLING; }

	if ( bReverb ) { d |= SNDMIX_REVERB; }

	if ( hqido ) { d |= SNDMIX_HQRESAMPLER; }

	if ( bMegaBass ) { d |= SNDMIX_MEGABASS; }

	if ( bNR ) { d |= SNDMIX_NOISEREDUCTION; }

	if ( bEQ ) { d |= SNDMIX_EQ; }

	gdwSoundSetup = d;
	InitPlayer( FALSE );
	return TRUE;
}


////////// FILTER

/*
 * This source code is public domain.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>
*/

#include "stdafx.h"
#include "sndfile.h"

// AWE32: cutoff = reg[0-255] * 31.25 + 100 -> [100Hz-8060Hz]
// EMU10K1 docs: cutoff = reg[0-127]*62+100
#define FILTER_PRECISION   8192

#ifndef NO_FILTER

#ifdef MSC_VER
#define _ASM_MATH
#endif

#ifdef _ASM_MATH

// pow(a,b) returns a^^b -> 2^^(b.log2(a))
static float pow( float a, float b )
{
	long tmpint;
	float result;
	_asm
	{
		fld b          // Load b
		fld a          // Load a
		fyl2x          // ST(0) = b.log2(a)
		fist tmpint       // Store integer exponent
		fisub tmpint      // ST(0) = -1 <= (b*log2(a)) <= 1
		f2xm1          // ST(0) = 2^(x)-1
		fild tmpint       // load integer exponent
		fld1           // Load 1
		fscale            // ST(0) = 2^ST(1)
		fstp ST( 1 )      // Remove the integer from the stack
		fmul ST( 1 ), ST( 0 ) // multiply with fractional part
		faddp ST( 1 ), ST( 0 ) // add integer_part
		fstp result       // Store the result
	}
	return result;
}


#else

#include <math.h>

#endif // _ASM_MATH


DWORD CSoundFile::CutOffToFrequency( UINT nCutOff, int flt_modifier ) const
//-----------------------------------------------------------------------
{
	float Fc;

	if ( m_dwSongFlags & SONG_EXFILTERRANGE )
	{
		Fc = 110.0f * pow( 2.0f, 0.25f + ( ( float )( nCutOff * ( flt_modifier + 256 ) ) ) / ( 21.0f * 512.0f ) );
	}
	else
	{
		Fc = 110.0f * pow( 2.0f, 0.25f + ( ( float )( nCutOff * ( flt_modifier + 256 ) ) ) / ( 24.0f * 512.0f ) );
	}

	LONG freq = ( LONG )Fc;

	if ( freq < 120 ) { return 120; }

	if ( freq > 10000 ) { return 10000; }

	if ( freq * 2 > ( LONG )gdwMixingFreq ) { freq = gdwMixingFreq >> 1; }

	return ( DWORD )freq;
}


// Simple 2-poles resonant filter
void CSoundFile::SetupChannelFilter( MODCHANNEL* pChn, BOOL bReset, int flt_modifier ) const
//----------------------------------------------------------------------------------------
{
	float fc = ( float )CutOffToFrequency( pChn->nCutOff, flt_modifier );
	float fs = ( float )gdwMixingFreq;
	float fg, fb0, fb1;

	fc *= ( float )( 2.0 * 3.14159265358 / fs );
	float dmpfac = pow( 10.0f, -( ( 24.0f / 128.0f ) * ( float )pChn->nResonance ) / 20.0f );
	float d = ( 1.0f - 2.0f * dmpfac ) * fc;

	if ( d > 2.0 ) { d = 2.0; }

	d = ( 2.0f * dmpfac - d ) / fc;
	float e = pow( 1.0f / fc, 2.0f );

	fg = 1 / ( 1 + d + e );
	fb0 = ( d + e + e ) / ( 1 + d + e );
	fb1 = -e / ( 1 + d + e );

	pChn->nFilter_A0 = ( int )( fg * FILTER_PRECISION );
	pChn->nFilter_B0 = ( int )( fb0 * FILTER_PRECISION );
	pChn->nFilter_B1 = ( int )( fb1 * FILTER_PRECISION );

	if ( bReset )
	{
		pChn->nFilter_Y1 = pChn->nFilter_Y2 = 0;
		pChn->nFilter_Y3 = pChn->nFilter_Y4 = 0;
	}

	pChn->dwFlags |= CHN_FILTER;
}

#endif // NO_FILTER

/// end of FILTER


//////// SNDMIX

#ifdef MODPLUG_TRACKER
#define ENABLE_STEREOVU
#endif

// Volume ramp length, in 1/10 ms
#define VOLUMERAMPLEN   146   // 1.46ms = 64 samples at 44.1kHz

// VU-Meter
#define VUMETER_DECAY      4

// SNDMIX: These are global flags for playback control (first two configurable via SetMixConfig)
UINT CSoundFile::m_nStereoSeparation = 128;
UINT CSoundFile::m_nMaxMixChannels = 32;
LONG CSoundFile::m_nStreamVolume = 0x8000;
// Mixing Configuration (SetWaveConfig)
DWORD CSoundFile::gdwSysInfo = 0;
DWORD CSoundFile::gnChannels = 1;
DWORD CSoundFile::gdwSoundSetup = 0;
DWORD CSoundFile::gdwMixingFreq = 44100;
DWORD CSoundFile::gnBitsPerSample = 16;
// Mixing data initialized in
UINT CSoundFile::gnAGC = AGC_UNITY;
UINT CSoundFile::gnVolumeRampSamples = 64;
UINT CSoundFile::gnVUMeter = 0;
UINT CSoundFile::gnCPUUsage = 0;
LPSNDMIXHOOKPROC CSoundFile::gpSndMixHook = NULL;
PMIXPLUGINCREATEPROC CSoundFile::gpMixPluginCreateProc = NULL;
LONG gnDryROfsVol = 0;
LONG gnDryLOfsVol = 0;
LONG gnRvbROfsVol = 0;
LONG gnRvbLOfsVol = 0;
int gbInitPlugins = 0;

typedef DWORD ( MPPASMCALL* LPCONVERTPROC )( LPVOID, int*, DWORD, LPLONG, LPLONG );

extern DWORD MPPASMCALL X86_Convert32To8( LPVOID lpBuffer, int*, DWORD nSamples, LPLONG, LPLONG );
extern DWORD MPPASMCALL X86_Convert32To16( LPVOID lpBuffer, int*, DWORD nSamples, LPLONG, LPLONG );
extern DWORD MPPASMCALL X86_Convert32To24( LPVOID lpBuffer, int*, DWORD nSamples, LPLONG, LPLONG );
extern DWORD MPPASMCALL X86_Convert32To32( LPVOID lpBuffer, int*, DWORD nSamples, LPLONG, LPLONG );
extern UINT MPPASMCALL X86_AGC( int* pBuffer, UINT nSamples, UINT nAGC );
extern VOID MPPASMCALL X86_Dither( int* pBuffer, UINT nSamples, UINT nBits );
extern VOID MPPASMCALL X86_InterleaveFrontRear( int* pFrontBuf, int* pRearBuf, DWORD nSamples );
extern VOID MPPASMCALL X86_StereoFill( int* pBuffer, UINT nSamples, LPLONG lpROfs, LPLONG lpLOfs );
extern VOID MPPASMCALL X86_MonoFromStereo( int* pMixBuf, UINT nSamples );

extern int MixSoundBuffer[MIXBUFFERSIZE * 4];
extern int MixRearBuffer[MIXBUFFERSIZE * 2];
UINT gnReverbSend;


// Log tables for pre-amp
// We don't want the tracker to get too loud
const UINT PreAmpTable[16] =
{
	0x60, 0x60, 0x60, 0x70, // 0-7
	0x80, 0x88, 0x90, 0x98, // 8-15
	0xA0, 0xA4, 0xA8, 0xB0, // 16-23
	0xB4, 0xB8, 0xBC, 0xC0, // 24-31
};

const UINT PreAmpAGCTable[16] =
{
	0x60, 0x60, 0x60, 0x60,
	0x68, 0x70, 0x78, 0x80,
	0x84, 0x88, 0x8C, 0x90,
	0x94, 0x98, 0x9C, 0xA0,
};


// Return (a*b)/c - no divide error
int _muldiv( long a, long b, long c )
{
#ifdef MSC_VER
	int sign, result;
	_asm
	{
		mov eax, a
		mov ebx, b
		or eax, eax
		mov edx, eax
		jge aneg
		neg eax
		aneg:
		xor edx, ebx
		or ebx, ebx
		mov ecx, c
		jge bneg
		neg ebx
		bneg:
		xor edx, ecx
		or ecx, ecx
		mov sign, edx
		jge cneg
		neg ecx
		cneg:
		mul ebx
		cmp edx, ecx
		jae diverr
		div ecx
		jmp ok
		diverr:
		mov eax, 0x7fffffff
		ok:
		mov edx, sign
		or edx, edx
		jge rneg
		neg eax
		rneg:
		mov result, eax
	}
	return result;
#else
	return ( ( uint64_t ) a * ( uint64_t ) b ) / c;
#endif
}


// Return (a*b+c/2)/c - no divide error
int _muldivr( long a, long b, long c )
{
#ifdef MSC_VER
	int sign, result;
	_asm
	{
		mov eax, a
		mov ebx, b
		or eax, eax
		mov edx, eax
		jge aneg
		neg eax
		aneg:
		xor edx, ebx
		or ebx, ebx
		mov ecx, c
		jge bneg
		neg ebx
		bneg:
		xor edx, ecx
		or ecx, ecx
		mov sign, edx
		jge cneg
		neg ecx
		cneg:
		mul ebx
		mov ebx, ecx
		shr ebx, 1
		add eax, ebx
		adc edx, 0
		cmp edx, ecx
		jae diverr
		div ecx
		jmp ok
		diverr:
		mov eax, 0x7fffffff
		ok:
		mov edx, sign
		or edx, edx
		jge rneg
		neg eax
		rneg:
		mov result, eax
	}
	return result;
#else
	return ( ( uint64_t ) a * ( uint64_t ) b + ( c >> 1 ) ) / c;
#endif
}


BOOL CSoundFile::InitPlayer( BOOL bReset )
//--------------------------------------
{
	if ( m_nMaxMixChannels > MAX_CHANNELS ) { m_nMaxMixChannels = MAX_CHANNELS; }

	if ( gdwMixingFreq < 4000 ) { gdwMixingFreq = 4000; }

	if ( gdwMixingFreq > MAX_SAMPLE_RATE ) { gdwMixingFreq = MAX_SAMPLE_RATE; }

	gnVolumeRampSamples = ( gdwMixingFreq * VOLUMERAMPLEN ) / 100000;

	if ( gnVolumeRampSamples < 8 ) { gnVolumeRampSamples = 8; }

	gnDryROfsVol = gnDryLOfsVol = 0;
	gnRvbROfsVol = gnRvbLOfsVol = 0;

	if ( bReset )
	{
		gnVUMeter = 0;
		gnCPUUsage = 0;
	}

	gbInitPlugins = ( bReset ) ? 3 : 1;
	InitializeDSP( bReset );
	return TRUE;
}


BOOL CSoundFile::FadeSong( UINT msec )
//----------------------------------
{
	LONG nsamples = _muldiv( msec, gdwMixingFreq, 1000 );

	if ( nsamples <= 0 ) { return FALSE; }

	if ( nsamples > 0x100000 ) { nsamples = 0x100000; }

	m_nBufferCount = nsamples;
	LONG nRampLength = m_nBufferCount;

	// Ramp everything down
	for ( UINT noff = 0; noff < m_nMixChannels; noff++ )
	{
		MODCHANNEL* pramp = &Chn[ChnMix[noff]];

		if ( !pramp ) { continue; }

		pramp->nNewLeftVol = pramp->nNewRightVol = 0;
		pramp->nRightRamp = ( -pramp->nRightVol << VOLUMERAMPPRECISION ) / nRampLength;
		pramp->nLeftRamp = ( -pramp->nLeftVol << VOLUMERAMPPRECISION ) / nRampLength;
		pramp->nRampRightVol = pramp->nRightVol << VOLUMERAMPPRECISION;
		pramp->nRampLeftVol = pramp->nLeftVol << VOLUMERAMPPRECISION;
		pramp->nRampLength = nRampLength;
		pramp->dwFlags |= CHN_VOLUMERAMP;
	}

	m_dwSongFlags |= SONG_FADINGSONG;
	return TRUE;
}


BOOL CSoundFile::GlobalFadeSong( UINT msec )
//----------------------------------------
{
	if ( m_dwSongFlags & SONG_GLOBALFADE ) { return FALSE; }

	m_nGlobalFadeMaxSamples = _muldiv( msec, gdwMixingFreq, 1000 );
	m_nGlobalFadeSamples = m_nGlobalFadeMaxSamples;
	m_dwSongFlags |= SONG_GLOBALFADE;
	return TRUE;
}


UINT CSoundFile::Read( LPVOID lpDestBuffer, UINT cbBuffer )
//-------------------------------------------------------
{
	LPBYTE lpBuffer = ( LPBYTE )lpDestBuffer;
	LPCONVERTPROC pCvt = X86_Convert32To8;
	UINT lRead, lMax, lSampleSize, lCount, lSampleCount, nStat = 0;
	LONG nVUMeterMin = 0x7FFFFFFF, nVUMeterMax = -0x7FFFFFFF;
	UINT nMaxPlugins;

	{
		nMaxPlugins = MAX_MIXPLUGINS;

		while ( ( nMaxPlugins > 0 ) && ( !m_MixPlugins[nMaxPlugins - 1].pMixPlugin ) ) { nMaxPlugins--; }
	}
	m_nMixStat = 0;
	lSampleSize = gnChannels;

	if ( gnBitsPerSample == 16 ) { lSampleSize *= 2; pCvt = X86_Convert32To16; }

#ifndef MODPLUG_FASTSOUNDLIB
	else if ( gnBitsPerSample == 24 ) { lSampleSize *= 3; pCvt = X86_Convert32To24; }
	else if ( gnBitsPerSample == 32 ) { lSampleSize *= 4; pCvt = X86_Convert32To32; }

#endif
	lMax = cbBuffer / lSampleSize;

	if ( ( !lMax ) || ( !lpBuffer ) || ( !m_nChannels ) ) { return 0; }

	lRead = lMax;

	if ( m_dwSongFlags & SONG_ENDREACHED ) { goto MixDone; }

	while ( lRead > 0 )
	{
		// Update Channel Data
		if ( !m_nBufferCount )
		{
#ifndef MODPLUG_FASTSOUNDLIB

			if ( m_dwSongFlags & SONG_FADINGSONG )
			{
				m_dwSongFlags |= SONG_ENDREACHED;
				m_nBufferCount = lRead;
			}
			else
#endif
				if ( !ReadNote() )
				{
#ifndef MODPLUG_FASTSOUNDLIB

					if ( !FadeSong( FADESONGDELAY ) )
#endif
					{
						m_dwSongFlags |= SONG_ENDREACHED;

						if ( lRead == lMax ) { goto MixDone; }

						m_nBufferCount = lRead;
					}
				}
		}

		lCount = m_nBufferCount;

		if ( lCount > MIXBUFFERSIZE ) { lCount = MIXBUFFERSIZE; }

		if ( lCount > lRead ) { lCount = lRead; }

		if ( !lCount ) { break; }

		lSampleCount = lCount;
#ifndef MODPLUG_NO_REVERB
		gnReverbSend = 0;
#endif
		// Resetting sound buffer
		X86_StereoFill( MixSoundBuffer, lSampleCount, &gnDryROfsVol, &gnDryLOfsVol );

		if ( gnChannels >= 2 )
		{
			lSampleCount *= 2;
			m_nMixStat += CreateStereoMix( lCount );
			ProcessStereoDSP( lCount );
		}
		else
		{
			m_nMixStat += CreateStereoMix( lCount );

			if ( nMaxPlugins ) { ProcessPlugins( lCount ); }

			ProcessStereoDSP( lCount );
			X86_MonoFromStereo( MixSoundBuffer, lCount );
		}

		nStat++;
#ifndef NO_AGC

		// Automatic Gain Control
		if ( gdwSoundSetup & SNDMIX_AGC ) { ProcessAGC( lSampleCount ); }

#endif
		UINT lTotalSampleCount = lSampleCount;
#ifndef MODPLUG_FASTSOUNDLIB

		// Multichannel
		if ( gnChannels > 2 )
		{
			X86_InterleaveFrontRear( MixSoundBuffer, MixRearBuffer, lSampleCount );
			lTotalSampleCount *= 2;
		}

		// Hook Function
		if ( gpSndMixHook )
		{
			gpSndMixHook( MixSoundBuffer, lTotalSampleCount, gnChannels );
		}

#endif
		// Perform clipping + VU-Meter
		lpBuffer += pCvt( lpBuffer, MixSoundBuffer, lTotalSampleCount, &nVUMeterMin, &nVUMeterMax );
		// Buffer ready
		lRead -= lCount;
		m_nBufferCount -= lCount;
	}

	MixDone:

	if ( lRead ) { memset( lpBuffer, ( gnBitsPerSample == 8 ) ? 0x80 : 0, lRead * lSampleSize ); }

	// VU-Meter
	nVUMeterMin >>= ( 24 - MIXING_ATTENUATION );
	nVUMeterMax >>= ( 24 - MIXING_ATTENUATION );

	if ( nVUMeterMax < nVUMeterMin ) { nVUMeterMax = nVUMeterMin; }

	if ( ( gnVUMeter = ( UINT )( nVUMeterMax - nVUMeterMin ) ) > 0xFF ) { gnVUMeter = 0xFF; }

	if ( nStat ) { m_nMixStat += nStat - 1; m_nMixStat /= nStat; }

	return lMax - lRead;
}



/////////////////////////////////////////////////////////////////////////////
// Handles navigation/effects

BOOL CSoundFile::ProcessRow()
//---------------------------
{
	if ( ++m_nTickCount >= m_nMusicSpeed * ( m_nPatternDelay + 1 ) + m_nFrameDelay )
	{
		m_nPatternDelay = 0;
		m_nFrameDelay = 0;
		m_nTickCount = 0;
		m_nRow = m_nNextRow;

		// Reset Pattern Loop Effect
		if ( m_nCurrentPattern != m_nNextPattern ) { m_nCurrentPattern = m_nNextPattern; }

		// Check if pattern is valid
		if ( !( m_dwSongFlags & SONG_PATTERNLOOP ) )
		{
			m_nPattern = ( m_nCurrentPattern < MAX_ORDERS ) ? Order[m_nCurrentPattern] : 0xFF;

			if ( ( m_nPattern < MAX_PATTERNS ) && ( !Patterns[m_nPattern] ) ) { m_nPattern = 0xFE; }

			while ( m_nPattern >= MAX_PATTERNS )
			{
				// End of song ?
				if ( ( m_nPattern == 0xFF ) || ( m_nCurrentPattern >= MAX_ORDERS ) )
				{
					//if (!m_nRepeatCount)
					return FALSE;     //never repeat entire song

					if ( !m_nRestartPos )
					{
						m_nMusicSpeed = m_nDefaultSpeed;
						m_nMusicTempo = m_nDefaultTempo;
						m_nGlobalVolume = m_nDefaultGlobalVolume;

						for ( UINT i = 0; i < MAX_CHANNELS; i++ )
						{
							Chn[i].dwFlags |= CHN_NOTEFADE | CHN_KEYOFF;
							Chn[i].nFadeOutVol = 0;

							if ( i < m_nChannels )
							{
								Chn[i].nGlobalVol = ChnSettings[i].nVolume;
								Chn[i].nVolume = ChnSettings[i].nVolume;
								Chn[i].nPan = ChnSettings[i].nPan;
								Chn[i].nPanSwing = Chn[i].nVolSwing = 0;
								Chn[i].nOldVolParam = 0;
								Chn[i].nOldOffset = 0;
								Chn[i].nOldHiOffset = 0;
								Chn[i].nPortamentoDest = 0;

								if ( !Chn[i].nLength )
								{
									Chn[i].dwFlags = ChnSettings[i].dwFlags;
									Chn[i].nLoopStart = 0;
									Chn[i].nLoopEnd = 0;
									Chn[i].pHeader = NULL;
									Chn[i].pSample = NULL;
									Chn[i].pInstrument = NULL;
								}
							}
						}
					}

//					if (m_nRepeatCount > 0) m_nRepeatCount--;
					m_nCurrentPattern = m_nRestartPos;
					m_nRow = 0;

					if ( ( Order[m_nCurrentPattern] >= MAX_PATTERNS ) || ( !Patterns[Order[m_nCurrentPattern]] ) ) { return FALSE; }
				}
				else
				{
					m_nCurrentPattern++;
				}

				m_nPattern = ( m_nCurrentPattern < MAX_ORDERS ) ? Order[m_nCurrentPattern] : 0xFF;

				if ( ( m_nPattern < MAX_PATTERNS ) && ( !Patterns[m_nPattern] ) ) { m_nPattern = 0xFE; }
			}

			m_nNextPattern = m_nCurrentPattern;
		}

		// Weird stuff?
		if ( ( m_nPattern >= MAX_PATTERNS ) || ( !Patterns[m_nPattern] ) ) { return FALSE; }

		// Should never happen
		if ( m_nRow >= PatternSize[m_nPattern] ) { m_nRow = 0; }

		m_nNextRow = m_nRow + 1;

		if ( m_nNextRow >= PatternSize[m_nPattern] )
		{
			if ( !( m_dwSongFlags & SONG_PATTERNLOOP ) ) { m_nNextPattern = m_nCurrentPattern + 1; }

			m_nNextRow = 0;
		}

		// Reset channel values
		MODCHANNEL* pChn = Chn;
		MODCOMMAND* m = Patterns[m_nPattern] + m_nRow * m_nChannels;

		for ( UINT nChn = 0; nChn < m_nChannels; pChn++, nChn++, m++ )
		{
			pChn->nRowNote = m->note;
			pChn->nRowInstr = m->instr;
			pChn->nRowVolCmd = m->volcmd;
			pChn->nRowVolume = m->vol;
			pChn->nRowCommand = m->command;
			pChn->nRowParam = m->param;

			pChn->nLeftVol = pChn->nNewLeftVol;
			pChn->nRightVol = pChn->nNewRightVol;
			pChn->dwFlags &= ~( CHN_PORTAMENTO | CHN_VIBRATO | CHN_TREMOLO | CHN_PANBRELLO );
			pChn->nCommand = 0;
		}
	}

	// Should we process tick0 effects?
	if ( !m_nMusicSpeed ) { m_nMusicSpeed = 1; }

	m_dwSongFlags |= SONG_FIRSTTICK;

	if ( m_nTickCount )
	{
		m_dwSongFlags &= ~SONG_FIRSTTICK;

		if ( ( !( m_nType & MOD_TYPE_XM ) ) && ( m_nTickCount < m_nMusicSpeed * ( 1 + m_nPatternDelay ) ) )
		{
			if ( !( m_nTickCount % m_nMusicSpeed ) ) { m_dwSongFlags |= SONG_FIRSTTICK; }
		}

	}

	// Update Effects
	return ProcessEffects();
}


////////////////////////////////////////////////////////////////////////////////////////////
// Handles envelopes & mixer setup

BOOL CSoundFile::ReadNote()
//-------------------------
{
	if ( !ProcessRow() ) { return FALSE; }

	////////////////////////////////////////////////////////////////////////////////////
	m_nTotalCount++;

	if ( !m_nMusicTempo ) { return FALSE; }

	m_nBufferCount = ( gdwMixingFreq * 5 * m_nTempoFactor ) / ( m_nMusicTempo << 8 );
	// Master Volume + Pre-Amplification / Attenuation setup
	DWORD nMasterVol;
	{
		int nchn32 = ( m_nChannels < 32 ) ? m_nChannels : 31;

		if ( ( m_nType & MOD_TYPE_IT ) && ( m_nInstruments ) && ( nchn32 < 6 ) ) { nchn32 = 6; }

		int realmastervol = m_nMasterVolume;

		if ( realmastervol > 0x80 )
		{
			realmastervol = 0x80 + ( ( realmastervol - 0x80 ) * ( nchn32 + 4 ) ) / 16;
		}

		UINT attenuation = ( gdwSoundSetup & SNDMIX_AGC ) ? PreAmpAGCTable[nchn32 >> 1] : PreAmpTable[nchn32 >> 1];
		DWORD mastervol = ( realmastervol * ( m_nSongPreAmp + 0x10 ) ) >> 6;

		if ( mastervol > 0x200 ) { mastervol = 0x200; }

		if ( ( m_dwSongFlags & SONG_GLOBALFADE ) && ( m_nGlobalFadeMaxSamples ) )
		{
			mastervol = _muldiv( mastervol, m_nGlobalFadeSamples, m_nGlobalFadeMaxSamples );
		}

		nMasterVol = ( mastervol << 7 ) / attenuation;

		if ( nMasterVol > 0x180 ) { nMasterVol = 0x180; }
	}
	////////////////////////////////////////////////////////////////////////////////////
	// Update channels data
	m_nMixChannels = 0;
	MODCHANNEL* pChn = Chn;

	for ( UINT nChn = 0; nChn < MAX_CHANNELS; nChn++, pChn++ )
	{
		if ( ( pChn->dwFlags & CHN_NOTEFADE ) && ( !( pChn->nFadeOutVol | pChn->nRightVol | pChn->nLeftVol ) ) )
		{
			pChn->nLength = 0;
			pChn->nROfs = pChn->nLOfs = 0;
		}

		// Check for unused channel
		if ( ( pChn->dwFlags & CHN_MUTE ) || ( ( nChn >= m_nChannels ) && ( !pChn->nLength ) ) )
		{
			pChn->nVUMeter = 0;
#ifdef ENABLE_STEREOVU
			pChn->nLeftVU = pChn->nRightVU = 0;
#endif
			continue;
		}

		// Reset channel data
		pChn->nInc = 0;
		pChn->nRealVolume = 0;
		pChn->nRealPan = pChn->nPan + pChn->nPanSwing;

		if ( pChn->nRealPan < 0 ) { pChn->nRealPan = 0; }

		if ( pChn->nRealPan > 256 ) { pChn->nRealPan = 256; }

		pChn->nRampLength = 0;

		// Calc Frequency
		if ( ( pChn->nPeriod )  && ( pChn->nLength ) )
		{
			int vol = pChn->nVolume + pChn->nVolSwing;

			if ( vol < 0 ) { vol = 0; }

			if ( vol > 256 ) { vol = 256; }

			// Tremolo
			if ( pChn->dwFlags & CHN_TREMOLO )
			{
				UINT trempos = pChn->nTremoloPos & 0x3F;

				if ( vol > 0 )
				{
					int tremattn = ( m_nType & MOD_TYPE_XM ) ? 5 : 6;

					switch ( pChn->nTremoloType & 0x03 )
					{
						case 1:
								vol += ( ModRampDownTable[trempos] * ( int )pChn->nTremoloDepth ) >> tremattn;
							break;

						case 2:
								vol += ( ModSquareTable[trempos] * ( int )pChn->nTremoloDepth ) >> tremattn;
							break;

						case 3:
								vol += ( ModRandomTable[trempos] * ( int )pChn->nTremoloDepth ) >> tremattn;
							break;

						default:
								vol += ( ModSinusTable[trempos] * ( int )pChn->nTremoloDepth ) >> tremattn;
					}
				}

				if ( ( m_nTickCount ) || ( ( m_nType & ( MOD_TYPE_STM | MOD_TYPE_S3M | MOD_TYPE_IT ) ) && ( !( m_dwSongFlags & SONG_ITOLDEFFECTS ) ) ) )
				{
					pChn->nTremoloPos = ( trempos + pChn->nTremoloSpeed ) & 0x3F;
				}
			}

			// Tremor
			if ( pChn->nCommand == CMD_TREMOR )
			{
				UINT n = ( pChn->nTremorParam >> 4 ) + ( pChn->nTremorParam & 0x0F );
				UINT ontime = pChn->nTremorParam >> 4;

				if ( ( !( m_nType & MOD_TYPE_IT ) ) || ( m_dwSongFlags & SONG_ITOLDEFFECTS ) ) { n += 2; ontime++; }

				UINT tremcount = ( UINT )pChn->nTremorCount;

				if ( tremcount >= n ) { tremcount = 0; }

				if ( ( m_nTickCount ) || ( m_nType & ( MOD_TYPE_S3M | MOD_TYPE_IT ) ) )
				{
					if ( tremcount >= ontime ) { vol = 0; }

					pChn->nTremorCount = ( BYTE )( tremcount + 1 );
				}

				pChn->dwFlags |= CHN_FASTVOLRAMP;
			}

			// Clip volume
			if ( vol < 0 ) { vol = 0; }

			if ( vol > 0x100 ) { vol = 0x100; }

			vol <<= 6;

			// Process Envelopes
			if ( pChn->pHeader )
			{
				INSTRUMENTHEADER* penv = pChn->pHeader;

				// Volume Envelope
				if ( ( pChn->dwFlags & CHN_VOLENV ) && ( penv->nVolEnv ) )
				{
					int envpos = pChn->nVolEnvPosition;
					UINT pt = penv->nVolEnv - 1;

					for ( UINT i = 0; i < ( UINT )( penv->nVolEnv - 1 ); i++ )
					{
						if ( envpos <= penv->VolPoints[i] )
						{
							pt = i;
							break;
						}
					}

					int x2 = penv->VolPoints[pt];
					int x1, envvol;

					if ( envpos >= x2 )
					{
						envvol = penv->VolEnv[pt] << 2;
						x1 = x2;
					}
					else if ( pt )
					{
						envvol = penv->VolEnv[pt - 1] << 2;
						x1 = penv->VolPoints[pt - 1];
					}
					else
					{
						envvol = 0;
						x1 = 0;
					}

					if ( envpos > x2 ) { envpos = x2; }

					if ( ( x2 > x1 ) && ( envpos > x1 ) )
					{
						envvol += ( ( envpos - x1 ) * ( ( ( int )penv->VolEnv[pt] << 2 ) - envvol ) ) / ( x2 - x1 );
					}

					if ( envvol < 0 ) { envvol = 0; }

					if ( envvol > 256 ) { envvol = 256; }

					vol = ( vol * envvol ) >> 8;
				}

				// Panning Envelope
				if ( ( pChn->dwFlags & CHN_PANENV ) && ( penv->nPanEnv ) )
				{
					int envpos = pChn->nPanEnvPosition;
					UINT pt = penv->nPanEnv - 1;

					for ( UINT i = 0; i < ( UINT )( penv->nPanEnv - 1 ); i++ )
					{
						if ( envpos <= penv->PanPoints[i] )
						{
							pt = i;
							break;
						}
					}

					int x2 = penv->PanPoints[pt], y2 = penv->PanEnv[pt];
					int x1, envpan;

					if ( envpos >= x2 )
					{
						envpan = y2;
						x1 = x2;
					}
					else if ( pt )
					{
						envpan = penv->PanEnv[pt - 1];
						x1 = penv->PanPoints[pt - 1];
					}
					else
					{
						envpan = 128;
						x1 = 0;
					}

					if ( ( x2 > x1 ) && ( envpos > x1 ) )
					{
						envpan += ( ( envpos - x1 ) * ( y2 - envpan ) ) / ( x2 - x1 );
					}

					if ( envpan < 0 ) { envpan = 0; }

					if ( envpan > 64 ) { envpan = 64; }

					int pan = pChn->nPan;

					if ( pan >= 128 )
					{
						pan += ( ( envpan - 32 ) * ( 256 - pan ) ) / 32;
					}
					else
					{
						pan += ( ( envpan - 32 ) * ( pan ) ) / 32;
					}

					if ( pan < 0 ) { pan = 0; }

					if ( pan > 256 ) { pan = 256; }

					pChn->nRealPan = pan;
				}

				// FadeOut volume
				if ( pChn->dwFlags & CHN_NOTEFADE )
				{
					UINT fadeout = penv->nFadeOut;

					if ( fadeout )
					{
						pChn->nFadeOutVol -= fadeout << 1;

						if ( pChn->nFadeOutVol <= 0 ) { pChn->nFadeOutVol = 0; }

						vol = ( vol * pChn->nFadeOutVol ) >> 16;
					}
					else if ( !pChn->nFadeOutVol )
					{
						vol = 0;
					}
				}

				// Pitch/Pan separation
				if ( ( penv->nPPS ) && ( pChn->nRealPan ) && ( pChn->nNote ) )
				{
					int pandelta = ( int )pChn->nRealPan + ( int )( ( int )( pChn->nNote - penv->nPPC - 1 ) * ( int )penv->nPPS ) / ( int )8;

					if ( pandelta < 0 ) { pandelta = 0; }

					if ( pandelta > 256 ) { pandelta = 256; }

					pChn->nRealPan = pandelta;
				}
			}
			else
			{
				// No Envelope: key off => note cut
				if ( pChn->dwFlags & CHN_NOTEFADE ) // 1.41-: CHN_KEYOFF|CHN_NOTEFADE
				{
					pChn->nFadeOutVol = 0;
					vol = 0;
				}
			}

			// vol is 14-bits
			if ( vol )
			{
				// IMPORTANT: pChn->nRealVolume is 14 bits !!!
				// -> _muldiv( 14+8, 6+6, 18); => RealVolume: 14-bit result (22+12-20)
				pChn->nRealVolume = _muldiv( vol * m_nGlobalVolume, pChn->nGlobalVol * pChn->nInsVol, 1 << 20 );
			}

			if ( pChn->nPeriod < m_nMinPeriod ) { pChn->nPeriod = m_nMinPeriod; }

			int period = pChn->nPeriod;

			if ( ( pChn->dwFlags & ( CHN_GLISSANDO | CHN_PORTAMENTO ) ) ==  ( CHN_GLISSANDO | CHN_PORTAMENTO ) )
			{
				period = GetPeriodFromNote( GetNoteFromPeriod( period ), pChn->nFineTune, pChn->nC4Speed );
			}

			// Arpeggio ?
			if ( pChn->nCommand == CMD_ARPEGGIO )
			{
				switch ( m_nTickCount % 3 )
				{
					case 1:
							period = GetPeriodFromNote( pChn->nNote + ( pChn->nArpeggio >> 4 ), pChn->nFineTune, pChn->nC4Speed );
						break;

					case 2:
							period = GetPeriodFromNote( pChn->nNote + ( pChn->nArpeggio & 0x0F ), pChn->nFineTune, pChn->nC4Speed );
						break;
				}
			}

			if ( m_dwSongFlags & SONG_AMIGALIMITS )
			{
				if ( period < 113 * 4 ) { period = 113 * 4; }

				if ( period > 856 * 4 ) { period = 856 * 4; }
			}

			// Pitch/Filter Envelope
			if ( ( pChn->pHeader ) && ( pChn->dwFlags & CHN_PITCHENV ) && ( pChn->pHeader->nPitchEnv ) )
			{
				INSTRUMENTHEADER* penv = pChn->pHeader;
				int envpos = pChn->nPitchEnvPosition;
				UINT pt = penv->nPitchEnv - 1;

				for ( UINT i = 0; i < ( UINT )( penv->nPitchEnv - 1 ); i++ )
				{
					if ( envpos <= penv->PitchPoints[i] )
					{
						pt = i;
						break;
					}
				}

				int x2 = penv->PitchPoints[pt];
				int x1, envpitch;

				if ( envpos >= x2 )
				{
					envpitch = ( ( ( int )penv->PitchEnv[pt] ) - 32 ) * 8;
					x1 = x2;
				}
				else if ( pt )
				{
					envpitch = ( ( ( int )penv->PitchEnv[pt - 1] ) - 32 ) * 8;
					x1 = penv->PitchPoints[pt - 1];
				}
				else
				{
					envpitch = 0;
					x1 = 0;
				}

				if ( envpos > x2 ) { envpos = x2; }

				if ( ( x2 > x1 ) && ( envpos > x1 ) )
				{
					int envpitchdest = ( ( ( int )penv->PitchEnv[pt] ) - 32 ) * 8;
					envpitch += ( ( envpos - x1 ) * ( envpitchdest - envpitch ) ) / ( x2 - x1 );
				}

				if ( envpitch < -256 ) { envpitch = -256; }

				if ( envpitch > 256 ) { envpitch = 256; }

				// Filter Envelope: controls cutoff frequency
				if ( penv->dwFlags & ENV_FILTER )
				{
#ifndef NO_FILTER
					SetupChannelFilter( pChn, ( pChn->dwFlags & CHN_FILTER ) ? FALSE : TRUE, envpitch );
#endif // NO_FILTER
				}
				else
					// Pitch Envelope
				{
					int l = envpitch;

					if ( l < 0 )
					{
						l = -l;

						if ( l > 255 ) { l = 255; }

						period = _muldiv( period, LinearSlideUpTable[l], 0x10000 );
					}
					else
					{
						if ( l > 255 ) { l = 255; }

						period = _muldiv( period, LinearSlideDownTable[l], 0x10000 );
					}
				}
			}

			// Vibrato
			if ( pChn->dwFlags & CHN_VIBRATO )
			{
				UINT vibpos = pChn->nVibratoPos;
				LONG vdelta;

				switch ( pChn->nVibratoType & 0x03 )
				{
					case 1:
							vdelta = ModRampDownTable[vibpos];
						break;

					case 2:
							vdelta = ModSquareTable[vibpos];
						break;

					case 3:
							vdelta = ModRandomTable[vibpos];
						break;

					default:
							vdelta = ModSinusTable[vibpos];
				}

				UINT vdepth = ( ( m_nType != MOD_TYPE_IT ) || ( m_dwSongFlags & SONG_ITOLDEFFECTS ) ) ? 6 : 7;
				vdelta = ( vdelta * ( int )pChn->nVibratoDepth ) >> vdepth;

				if ( ( m_dwSongFlags & SONG_LINEARSLIDES ) && ( m_nType & MOD_TYPE_IT ) )
				{
					LONG l = vdelta;

					if ( l < 0 )
					{
						l = -l;
						vdelta = _muldiv( period, LinearSlideDownTable[l >> 2], 0x10000 ) - period;

						if ( l & 0x03 ) { vdelta += _muldiv( period, FineLinearSlideDownTable[l & 0x03], 0x10000 ) - period; }

					}
					else
					{
						vdelta = _muldiv( period, LinearSlideUpTable[l >> 2], 0x10000 ) - period;

						if ( l & 0x03 ) { vdelta += _muldiv( period, FineLinearSlideUpTable[l & 0x03], 0x10000 ) - period; }

					}
				}

				period += vdelta;

				if ( ( m_nTickCount ) || ( ( m_nType & MOD_TYPE_IT ) && ( !( m_dwSongFlags & SONG_ITOLDEFFECTS ) ) ) )
				{
					pChn->nVibratoPos = ( vibpos + pChn->nVibratoSpeed ) & 0x3F;
				}
			}

			// Panbrello
			if ( pChn->dwFlags & CHN_PANBRELLO )
			{
				UINT panpos = ( ( pChn->nPanbrelloPos + 0x10 ) >> 2 ) & 0x3F;
				LONG pdelta;

				switch ( pChn->nPanbrelloType & 0x03 )
				{
					case 1:
							pdelta = ModRampDownTable[panpos];
						break;

					case 2:
							pdelta = ModSquareTable[panpos];
						break;

					case 3:
							pdelta = ModRandomTable[panpos];
						break;

					default:
							pdelta = ModSinusTable[panpos];
				}

				pChn->nPanbrelloPos += pChn->nPanbrelloSpeed;
				pdelta = ( ( pdelta * ( int )pChn->nPanbrelloDepth ) + 2 ) >> 3;
				pdelta += pChn->nRealPan;

				if ( pdelta < 0 ) { pdelta = 0; }

				if ( pdelta > 256 ) { pdelta = 256; }

				pChn->nRealPan = pdelta;
			}

			int nPeriodFrac = 0;

			// Instrument Auto-Vibrato
			if ( ( pChn->pInstrument ) && ( pChn->pInstrument->nVibDepth ) )
			{
				MODINSTRUMENT* pins = pChn->pInstrument;

				if ( pins->nVibSweep == 0 )
				{
					pChn->nAutoVibDepth = pins->nVibDepth << 8;
				}
				else
				{
					if ( m_nType & MOD_TYPE_IT )
					{
						pChn->nAutoVibDepth += pins->nVibSweep << 3;
					}
					else if ( !( pChn->dwFlags & CHN_KEYOFF ) )
					{
						pChn->nAutoVibDepth += ( pins->nVibDepth << 8 ) / pins->nVibSweep;
					}

					if ( ( pChn->nAutoVibDepth >> 8 ) > pins->nVibDepth )
					{
						pChn->nAutoVibDepth = pins->nVibDepth << 8;
					}
				}

				pChn->nAutoVibPos += pins->nVibRate;
				int val;

				switch ( pins->nVibType )
				{
					case 4:  // Random
							val = ModRandomTable[pChn->nAutoVibPos & 0x3F];
						pChn->nAutoVibPos++;
						break;

					case 3:  // Ramp Down
							val = ( ( 0x40 - ( pChn->nAutoVibPos >> 1 ) ) & 0x7F ) - 0x40;
						break;

					case 2:  // Ramp Up
							val = ( ( 0x40 + ( pChn->nAutoVibPos >> 1 ) ) & 0x7f ) - 0x40;
						break;

					case 1:  // Square
							val = ( pChn->nAutoVibPos & 128 ) ? +64 : -64;
						break;

					default: // Sine
							val = ft2VibratoTable[pChn->nAutoVibPos & 255];
				}

				int n =  ( ( val * pChn->nAutoVibDepth ) >> 8 );

				if ( m_nType & MOD_TYPE_IT )
				{
					int df1, df2;

					if ( n < 0 )
					{
						n = -n;
						UINT n1 = n >> 8;
						df1 = LinearSlideUpTable[n1];
						df2 = LinearSlideUpTable[n1 + 1];
					}
					else
					{
						UINT n1 = n >> 8;
						df1 = LinearSlideDownTable[n1];
						df2 = LinearSlideDownTable[n1 + 1];
					}

					n >>= 2;
					period = _muldiv( period, df1 + ( ( df2 - df1 ) * ( n & 0x3F ) >> 6 ), 256 );
					nPeriodFrac = period & 0xFF;
					period >>= 8;
				}
				else
				{
					period += ( n >> 6 );
				}
			}

			// Final Period
			if ( period <= m_nMinPeriod )
			{
				if ( m_nType & MOD_TYPE_S3M ) { pChn->nLength = 0; }

				period = m_nMinPeriod;
			}

			if ( period > m_nMaxPeriod )
			{
				if ( ( m_nType & MOD_TYPE_IT ) || ( period >= 0x100000 ) )
				{
					pChn->nFadeOutVol = 0;
					pChn->dwFlags |= CHN_NOTEFADE;
					pChn->nRealVolume = 0;
				}

				period = m_nMaxPeriod;
				nPeriodFrac = 0;
			}

			UINT freq = GetFreqFromPeriod( period, pChn->nC4Speed, nPeriodFrac );

			if ( ( m_nType & MOD_TYPE_IT ) && ( freq < 256 ) )
			{
				pChn->nFadeOutVol = 0;
				pChn->dwFlags |= CHN_NOTEFADE;
				pChn->nRealVolume = 0;
			}

			UINT ninc = _muldiv( freq, 0x10000, gdwMixingFreq );

			if ( ( ninc >= 0xFFB0 ) && ( ninc <= 0x10090 ) ) { ninc = 0x10000; }

			if ( m_nFreqFactor != 128 ) { ninc = ( ninc * m_nFreqFactor ) >> 7; }

			if ( ninc > 0xFF0000 ) { ninc = 0xFF0000; }

			pChn->nInc = ( ninc + 1 ) & ~3;
		}

		// Increment envelope position
		if ( pChn->pHeader )
		{
			INSTRUMENTHEADER* penv = pChn->pHeader;

			// Volume Envelope
			if ( pChn->dwFlags & CHN_VOLENV )
			{
				// Increase position
				pChn->nVolEnvPosition++;

				// Volume Loop ?
				if ( penv->dwFlags & ENV_VOLLOOP )
				{
					UINT volloopend = penv->VolPoints[penv->nVolLoopEnd];

					if ( m_nType != MOD_TYPE_XM ) { volloopend++; }

					if ( pChn->nVolEnvPosition == volloopend )
					{
						pChn->nVolEnvPosition = penv->VolPoints[penv->nVolLoopStart];

						if ( ( penv->nVolLoopEnd == penv->nVolLoopStart ) && ( !penv->VolEnv[penv->nVolLoopStart] )
						     && ( ( !( m_nType & MOD_TYPE_XM ) ) || ( penv->nVolLoopEnd + 1 == penv->nVolEnv ) ) )
						{
							pChn->dwFlags |= CHN_NOTEFADE;
							pChn->nFadeOutVol = 0;
						}
					}
				}

				// Volume Sustain ?
				if ( ( penv->dwFlags & ENV_VOLSUSTAIN ) && ( !( pChn->dwFlags & CHN_KEYOFF ) ) )
				{
					if ( pChn->nVolEnvPosition == ( UINT )penv->VolPoints[penv->nVolSustainEnd] + 1 )
					{
						pChn->nVolEnvPosition = penv->VolPoints[penv->nVolSustainBegin];
					}
				}
				else

					// End of Envelope ?
					if ( pChn->nVolEnvPosition > penv->VolPoints[penv->nVolEnv - 1] )
					{
						if ( ( m_nType & MOD_TYPE_IT ) || ( pChn->dwFlags & CHN_KEYOFF ) ) { pChn->dwFlags |= CHN_NOTEFADE; }

						pChn->nVolEnvPosition = penv->VolPoints[penv->nVolEnv - 1];

						if ( ( !penv->VolEnv[penv->nVolEnv - 1] ) && ( ( nChn >= m_nChannels ) || ( m_nType & MOD_TYPE_IT ) ) )
						{
							pChn->dwFlags |= CHN_NOTEFADE;
							pChn->nFadeOutVol = 0;

							pChn->nRealVolume = 0;
						}
					}
			}

			// Panning Envelope
			if ( pChn->dwFlags & CHN_PANENV )
			{
				pChn->nPanEnvPosition++;

				if ( penv->dwFlags & ENV_PANLOOP )
				{
					UINT panloopend = penv->PanPoints[penv->nPanLoopEnd];

					if ( m_nType != MOD_TYPE_XM ) { panloopend++; }

					if ( pChn->nPanEnvPosition == panloopend )
					{
						pChn->nPanEnvPosition = penv->PanPoints[penv->nPanLoopStart];
					}
				}

				// Panning Sustain ?
				if ( ( penv->dwFlags & ENV_PANSUSTAIN ) && ( pChn->nPanEnvPosition == ( UINT )penv->PanPoints[penv->nPanSustainEnd] + 1 )
				     && ( !( pChn->dwFlags & CHN_KEYOFF ) ) )
				{
					// Panning sustained
					pChn->nPanEnvPosition = penv->PanPoints[penv->nPanSustainBegin];
				}
				else
				{
					if ( pChn->nPanEnvPosition > penv->PanPoints[penv->nPanEnv - 1] )
					{
						pChn->nPanEnvPosition = penv->PanPoints[penv->nPanEnv - 1];
					}
				}
			}

			// Pitch Envelope
			if ( pChn->dwFlags & CHN_PITCHENV )
			{
				// Increase position
				pChn->nPitchEnvPosition++;

				// Pitch Loop ?
				if ( penv->dwFlags & ENV_PITCHLOOP )
				{
					if ( pChn->nPitchEnvPosition >= penv->PitchPoints[penv->nPitchLoopEnd] )
					{
						pChn->nPitchEnvPosition = penv->PitchPoints[penv->nPitchLoopStart];
					}
				}

				// Pitch Sustain ?
				if ( ( penv->dwFlags & ENV_PITCHSUSTAIN ) && ( !( pChn->dwFlags & CHN_KEYOFF ) ) )
				{
					if ( pChn->nPitchEnvPosition == ( UINT )penv->PitchPoints[penv->nPitchSustainEnd] + 1 )
					{
						pChn->nPitchEnvPosition = penv->PitchPoints[penv->nPitchSustainBegin];
					}
				}
				else
				{
					if ( pChn->nPitchEnvPosition > penv->PitchPoints[penv->nPitchEnv - 1] )
					{
						pChn->nPitchEnvPosition = penv->PitchPoints[penv->nPitchEnv - 1];
					}
				}
			}
		}

#ifdef MODPLUG_PLAYER

		// Limit CPU -> > 80% -> don't ramp
		if ( ( gnCPUUsage >= 80 ) && ( !pChn->nRealVolume ) )
		{
			pChn->nLeftVol = pChn->nRightVol = 0;
		}

#endif // MODPLUG_PLAYER
		// Volume ramping
		pChn->dwFlags &= ~CHN_VOLUMERAMP;

		if ( ( pChn->nRealVolume ) || ( pChn->nLeftVol ) || ( pChn->nRightVol ) )
		{
			pChn->dwFlags |= CHN_VOLUMERAMP;
		}

#ifdef MODPLUG_PLAYER

		// Decrease VU-Meter
		if ( pChn->nVUMeter > VUMETER_DECAY ) { pChn->nVUMeter -= VUMETER_DECAY; }
		else { pChn->nVUMeter = 0; }

#endif // MODPLUG_PLAYER
#ifdef ENABLE_STEREOVU

		if ( pChn->nLeftVU > VUMETER_DECAY ) { pChn->nLeftVU -= VUMETER_DECAY; }
		else { pChn->nLeftVU = 0; }

		if ( pChn->nRightVU > VUMETER_DECAY ) { pChn->nRightVU -= VUMETER_DECAY; }
		else { pChn->nRightVU = 0; }

#endif

		// Check for too big nInc
		if ( ( ( pChn->nInc >> 16 ) + 1 ) >= ( LONG )( pChn->nLoopEnd - pChn->nLoopStart ) ) { pChn->dwFlags &= ~CHN_LOOP; }

		pChn->nNewRightVol = pChn->nNewLeftVol = 0;
		pChn->pCurrentSample = ( ( pChn->pSample ) && ( pChn->nLength ) && ( pChn->nInc ) ) ? pChn->pSample : NULL;

		if ( pChn->pCurrentSample )
		{
			// Update VU-Meter (nRealVolume is 14-bit)
#ifdef MODPLUG_PLAYER
			UINT vutmp = pChn->nRealVolume >> ( 14 - 8 );

			if ( vutmp > 0xFF ) { vutmp = 0xFF; }

			if ( pChn->nVUMeter >= 0x100 ) { pChn->nVUMeter = vutmp; }

			vutmp >>= 1;

			if ( pChn->nVUMeter < vutmp ) { pChn->nVUMeter = vutmp; }

#endif // MODPLUG_PLAYER
#ifdef ENABLE_STEREOVU
			UINT vul = ( pChn->nRealVolume * pChn->nRealPan ) >> 14;

			if ( vul > 127 ) { vul = 127; }

			if ( pChn->nLeftVU > 127 ) { pChn->nLeftVU = ( BYTE )vul; }

			vul >>= 1;

			if ( pChn->nLeftVU < vul ) { pChn->nLeftVU = ( BYTE )vul; }

			UINT vur = ( pChn->nRealVolume * ( 256 - pChn->nRealPan ) ) >> 14;

			if ( vur > 127 ) { vur = 127; }

			if ( pChn->nRightVU > 127 ) { pChn->nRightVU = ( BYTE )vur; }

			vur >>= 1;

			if ( pChn->nRightVU < vur ) { pChn->nRightVU = ( BYTE )vur; }

#endif
#ifdef MODPLUG_TRACKER
			UINT kChnMasterVol = ( pChn->dwFlags & CHN_EXTRALOUD ) ? 0x100 : nMasterVol;
#else
#define     kChnMasterVol  nMasterVol
#endif // MODPLUG_TRACKER

			// Adjusting volumes
			if ( gnChannels >= 2 )
			{
				int pan = ( ( int )pChn->nRealPan ) - 128;
				pan *= ( int )m_nStereoSeparation;
				pan /= 128;
				pan += 128;

				if ( pan < 0 ) { pan = 0; }

				if ( pan > 256 ) { pan = 256; }

#ifndef MODPLUG_FASTSOUNDLIB

				if ( gdwSoundSetup & SNDMIX_REVERSESTEREO ) { pan = 256 - pan; }

#endif
				LONG realvol = ( pChn->nRealVolume * kChnMasterVol ) >> ( 8 - 1 );

				if ( gdwSoundSetup & SNDMIX_SOFTPANNING )
				{
					if ( pan < 128 )
					{
						pChn->nNewLeftVol = ( realvol * pan ) >> 8;
						pChn->nNewRightVol = ( realvol * 128 ) >> 8;
					}
					else
					{
						pChn->nNewLeftVol = ( realvol * 128 ) >> 8;
						pChn->nNewRightVol = ( realvol * ( 256 - pan ) ) >> 8;
					}
				}
				else
				{
					pChn->nNewLeftVol = ( realvol * pan ) >> 8;
					pChn->nNewRightVol = ( realvol * ( 256 - pan ) ) >> 8;
				}
			}
			else
			{
				pChn->nNewRightVol = ( pChn->nRealVolume * kChnMasterVol ) >> 8;
				pChn->nNewLeftVol = pChn->nNewRightVol;
			}

			// Clipping volumes
			if ( pChn->nNewRightVol > 0xFFFF ) { pChn->nNewRightVol = 0xFFFF; }

			if ( pChn->nNewLeftVol > 0xFFFF ) { pChn->nNewLeftVol = 0xFFFF; }

			// Check IDO
			if ( gdwSoundSetup & SNDMIX_NORESAMPLING )
			{
				pChn->dwFlags |= CHN_NOIDO;
			}
			else
			{
				pChn->dwFlags &= ~( CHN_NOIDO | CHN_HQSRC );

				if ( pChn->nInc == 0x10000 )
				{
					pChn->dwFlags |= CHN_NOIDO;
				}
				else
				{
					if ( ( ( gdwSoundSetup & SNDMIX_HQRESAMPLER ) == 0 ) && ( ( gdwSoundSetup & SNDMIX_ULTRAHQSRCMODE ) == 0 ) )
					{
						if ( pChn->nInc >= 0xFF00 ) { pChn->dwFlags |= CHN_NOIDO; }
					}
				}
			}

			pChn->nNewRightVol >>= MIXING_ATTENUATION;
			pChn->nNewLeftVol >>= MIXING_ATTENUATION;
			pChn->nRightRamp = pChn->nLeftRamp = 0;

			// Dolby Pro-Logic Surround
			if ( ( pChn->dwFlags & CHN_SURROUND ) && ( gnChannels <= 2 ) ) { pChn->nNewLeftVol = - pChn->nNewLeftVol; }

			// Checking Ping-Pong Loops
			if ( pChn->dwFlags & CHN_PINGPONGFLAG ) { pChn->nInc = -pChn->nInc; }

			// Setting up volume ramp
			if ( ( pChn->dwFlags & CHN_VOLUMERAMP )
			     && ( ( pChn->nRightVol != pChn->nNewRightVol )
			          || ( pChn->nLeftVol != pChn->nNewLeftVol ) ) )
			{
				LONG nRampLength = gnVolumeRampSamples;
				LONG nRightDelta = ( ( pChn->nNewRightVol - pChn->nRightVol ) << VOLUMERAMPPRECISION );
				LONG nLeftDelta = ( ( pChn->nNewLeftVol - pChn->nLeftVol ) << VOLUMERAMPPRECISION );
#ifndef MODPLUG_FASTSOUNDLIB

				if ( ( gdwSoundSetup & SNDMIX_DIRECTTODISK )
				     || ( ( gdwSysInfo & ( SYSMIX_ENABLEMMX | SYSMIX_FASTCPU ) )
				          && ( gdwSoundSetup & SNDMIX_HQRESAMPLER ) && ( gnCPUUsage <= 20 ) ) )
				{
					if ( ( pChn->nRightVol | pChn->nLeftVol ) && ( pChn->nNewRightVol | pChn->nNewLeftVol ) && ( !( pChn->dwFlags & CHN_FASTVOLRAMP ) ) )
					{
						nRampLength = m_nBufferCount;

						if ( nRampLength > ( 1 << ( VOLUMERAMPPRECISION - 1 ) ) ) { nRampLength = ( 1 << ( VOLUMERAMPPRECISION - 1 ) ); }

						if ( nRampLength < ( LONG )gnVolumeRampSamples ) { nRampLength = gnVolumeRampSamples; }
					}
				}

#endif
				pChn->nRightRamp = nRightDelta / nRampLength;
				pChn->nLeftRamp = nLeftDelta / nRampLength;
				pChn->nRightVol = pChn->nNewRightVol - ( ( pChn->nRightRamp * nRampLength ) >> VOLUMERAMPPRECISION );
				pChn->nLeftVol = pChn->nNewLeftVol - ( ( pChn->nLeftRamp * nRampLength ) >> VOLUMERAMPPRECISION );

				if ( pChn->nRightRamp | pChn->nLeftRamp )
				{
					pChn->nRampLength = nRampLength;
				}
				else
				{
					pChn->dwFlags &= ~CHN_VOLUMERAMP;
					pChn->nRightVol = pChn->nNewRightVol;
					pChn->nLeftVol = pChn->nNewLeftVol;
				}
			}
			else
			{
				pChn->dwFlags &= ~CHN_VOLUMERAMP;
				pChn->nRightVol = pChn->nNewRightVol;
				pChn->nLeftVol = pChn->nNewLeftVol;
			}

			pChn->nRampRightVol = pChn->nRightVol << VOLUMERAMPPRECISION;
			pChn->nRampLeftVol = pChn->nLeftVol << VOLUMERAMPPRECISION;
			// Adding the channel in the channel list
			ChnMix[m_nMixChannels++] = nChn;

			if ( m_nMixChannels >= MAX_CHANNELS ) { break; }
		}
		else
		{
#ifdef ENABLE_STEREOVU

			// Note change but no sample
			if ( pChn->nLeftVU > 128 ) { pChn->nLeftVU = 0; }

			if ( pChn->nRightVU > 128 ) { pChn->nRightVU = 0; }

#endif

			if ( pChn->nVUMeter > 0xFF ) { pChn->nVUMeter = 0; }

			pChn->nLeftVol = pChn->nRightVol = 0;
			pChn->nLength = 0;
		}
	}

	// Checking Max Mix Channels reached: ordering by volume
	if ( ( m_nMixChannels >= m_nMaxMixChannels ) && ( !( gdwSoundSetup & SNDMIX_DIRECTTODISK ) ) )
	{
		for ( UINT i = 0; i < m_nMixChannels; i++ )
		{
			UINT j = i;

			while ( ( j + 1 < m_nMixChannels ) && ( Chn[ChnMix[j]].nRealVolume < Chn[ChnMix[j + 1]].nRealVolume ) )
			{
				UINT n = ChnMix[j];
				ChnMix[j] = ChnMix[j + 1];
				ChnMix[j + 1] = n;
				j++;
			}
		}
	}

	if ( m_dwSongFlags & SONG_GLOBALFADE )
	{
		if ( !m_nGlobalFadeSamples )
		{
			m_dwSongFlags |= SONG_ENDREACHED;
			return FALSE;
		}

		if ( m_nGlobalFadeSamples > m_nBufferCount )
		{
			m_nGlobalFadeSamples -= m_nBufferCount;
		}
		else
		{
			m_nGlobalFadeSamples = 0;
		}
	}

	return TRUE;
}

/// SND_FX

#ifdef MSC_VER
#pragma warning(disable:4244)
#endif

////////////////////////////////////////////////////////////
// Length

DWORD CSoundFile::GetLength( BOOL bAdjust, BOOL bTotal )
//----------------------------------------------------
{
	UINT dwElapsedTime = 0, nRow = 0, nCurrentPattern = 0, nNextPattern = 0, nPattern = Order[0];
	UINT nMusicSpeed = m_nDefaultSpeed, nMusicTempo = m_nDefaultTempo, nNextRow = 0;
	UINT nMaxRow = 0, nMaxPattern = 0;
	LONG nGlbVol = m_nDefaultGlobalVolume, nOldGlbVolSlide = 0;
	BYTE samples[MAX_CHANNELS];
	BYTE instr[MAX_CHANNELS];
	BYTE notes[MAX_CHANNELS];
	BYTE vols[MAX_CHANNELS];
	BYTE oldparam[MAX_CHANNELS];
	BYTE chnvols[MAX_CHANNELS];
	DWORD patloop[MAX_CHANNELS];

	memset( instr, 0, sizeof( instr ) );
	memset( notes, 0, sizeof( notes ) );
	memset( vols, 0xFF, sizeof( vols ) );
	memset( patloop, 0, sizeof( patloop ) );
	memset( oldparam, 0, sizeof( oldparam ) );
	memset( chnvols, 64, sizeof( chnvols ) );
	memset( samples, 0, sizeof( samples ) );

	for ( UINT icv = 0; icv < m_nChannels; icv++ ) { chnvols[icv] = ChnSettings[icv].nVolume; }

	nMaxRow = m_nNextRow;
	nMaxPattern = m_nNextPattern;
	nCurrentPattern = nNextPattern = 0;
	nPattern = Order[0];
	nRow = nNextRow = 0;

	for ( ;; )
	{
		UINT nSpeedCount = 0;
		nRow = nNextRow;
		nCurrentPattern = nNextPattern;
		// Check if pattern is valid
		nPattern = Order[nCurrentPattern];

		while ( nPattern >= MAX_PATTERNS )
		{
			// End of song ?
			if ( ( nPattern == 0xFF ) || ( nCurrentPattern >= MAX_ORDERS ) )
			{
				goto EndMod;
			}
			else
			{
				nCurrentPattern++;
				nPattern = ( nCurrentPattern < MAX_ORDERS ) ? Order[nCurrentPattern] : 0xFF;
			}

			nNextPattern = nCurrentPattern;
		}

		// Weird stuff?
		if ( ( nPattern >= MAX_PATTERNS ) || ( !Patterns[nPattern] ) ) { break; }

		// Should never happen
		if ( nRow >= PatternSize[nPattern] ) { nRow = 0; }

		// Update next position
		nNextRow = nRow + 1;

		if ( nNextRow >= PatternSize[nPattern] )
		{
			nNextPattern = nCurrentPattern + 1;
			nNextRow = 0;
		}

		if ( !nRow )
		{
			for ( UINT ipck = 0; ipck < m_nChannels; ipck++ ) { patloop[ipck] = dwElapsedTime; }
		}

		if ( !bTotal )
		{
			if ( ( nCurrentPattern > nMaxPattern ) || ( ( nCurrentPattern == nMaxPattern ) && ( nRow >= nMaxRow ) ) )
			{
				if ( bAdjust )
				{
					m_nMusicSpeed = nMusicSpeed;
					m_nMusicTempo = nMusicTempo;
				}

				break;
			}
		}

		MODCHANNEL* pChn = Chn;
		MODCOMMAND* p = Patterns[nPattern] + nRow * m_nChannels;

		for ( UINT nChn = 0; nChn < m_nChannels; p++, pChn++, nChn++ ) if ( *( ( DWORD* )p ) )
			{
				UINT command = p->command;
				UINT param = p->param;
				UINT note = p->note;

				if ( p->instr ) { instr[nChn] = p->instr; notes[nChn] = 0; vols[nChn] = 0xFF; }

				if ( ( note ) && ( note <= NOTE_MAX ) ) { notes[nChn] = note; }

				if ( p->volcmd == VOLCMD_VOLUME )  { vols[nChn] = p->vol; }

				if ( command ) switch ( command )
					{
							// Position Jump
						case CMD_POSITIONJUMP:
								if ( param <= nCurrentPattern ) { goto EndMod; }

							nNextPattern = param;
							nNextRow = 0;

							if ( bAdjust )
							{
								pChn->nPatternLoopCount = 0;
								pChn->nPatternLoop = 0;
							}

							break;

							// Pattern Break
						case CMD_PATTERNBREAK:
								nNextRow = param;
							nNextPattern = nCurrentPattern + 1;

							if ( bAdjust )
							{
								pChn->nPatternLoopCount = 0;
								pChn->nPatternLoop = 0;
							}

							break;

							// Set Speed
						case CMD_SPEED:
								if ( !param ) { break; }

							if ( ( param <= 0x20 ) || ( m_nType != MOD_TYPE_MOD ) )
							{
								if ( param < 128 ) { nMusicSpeed = param; }
							}

							break;

							// Set Tempo
						case CMD_TEMPO:
								if ( ( bAdjust ) && ( m_nType & ( MOD_TYPE_S3M | MOD_TYPE_IT ) ) )
								{
									if ( param ) { pChn->nOldTempo = param; }
									else { param = pChn->nOldTempo; }
								}

							if ( param >= 0x20 ) { nMusicTempo = param; }
							else

								// Tempo Slide
								if ( ( param & 0xF0 ) == 0x10 )
								{
									nMusicTempo += param & 0x0F;

									if ( nMusicTempo > 255 ) { nMusicTempo = 255; }
								}
								else
								{
									nMusicTempo -= param & 0x0F;

									if ( nMusicTempo < 32 ) { nMusicTempo = 32; }
								}

							break;

							// Pattern Delay
						case CMD_S3MCMDEX:
								if ( ( param & 0xF0 ) == 0x60 ) { nSpeedCount = param & 0x0F; break; }
								else if ( ( param & 0xF0 ) == 0xB0 ) { param &= 0x0F; param |= 0x60; }

						case CMD_MODCMDEX:
								if ( ( param & 0xF0 ) == 0xE0 ) { nSpeedCount = ( param & 0x0F ) * nMusicSpeed; }
								else if ( ( param & 0xF0 ) == 0x60 )
								{
									if ( param & 0x0F ) { dwElapsedTime += ( dwElapsedTime - patloop[nChn] ) * ( param & 0x0F ); }
									else { patloop[nChn] = dwElapsedTime; }
								}

							break;
					}

				if ( !bAdjust ) { continue; }

				switch ( command )
				{
						// Portamento Up/Down
					case CMD_PORTAMENTOUP:
						case CMD_PORTAMENTODOWN:
								if ( param ) { pChn->nOldPortaUpDown = param; }

						break;

						// Tone-Portamento
					case CMD_TONEPORTAMENTO:
							if ( param ) { pChn->nPortamentoSlide = param << 2; }

						break;

						// Offset
					case CMD_OFFSET:
							if ( param ) { pChn->nOldOffset = param; }

						break;

						// Volume Slide
					case CMD_VOLUMESLIDE:
						case CMD_TONEPORTAVOL:
							case CMD_VIBRATOVOL:
									if ( param ) { pChn->nOldVolumeSlide = param; }

						break;

						// Set Volume
					case CMD_VOLUME:
							vols[nChn] = param;
						break;

						// Global Volume
					case CMD_GLOBALVOLUME:
							if ( !( m_nType & ( MOD_TYPE_IT ) ) ) { param <<= 1; }

						if ( param > 128 ) { param = 128; }

						nGlbVol = param << 1;
						break;

						// Global Volume Slide
					case CMD_GLOBALVOLSLIDE:
							if ( param ) { nOldGlbVolSlide = param; }
							else { param = nOldGlbVolSlide; }

						if ( ( ( param & 0x0F ) == 0x0F ) && ( param & 0xF0 ) )
						{
							param >>= 4;

							if ( m_nType != MOD_TYPE_IT ) { param <<= 1; }

							nGlbVol += param << 1;
						}
						else if ( ( ( param & 0xF0 ) == 0xF0 ) && ( param & 0x0F ) )
						{
							param = ( param & 0x0F ) << 1;

							if ( m_nType != MOD_TYPE_IT ) { param <<= 1; }

							nGlbVol -= param;
						}
						else if ( param & 0xF0 )
						{
							param >>= 4;
							param <<= 1;

							if ( m_nType != MOD_TYPE_IT ) { param <<= 1; }

							nGlbVol += param * nMusicSpeed;
						}
						else
						{
							param = ( param & 0x0F ) << 1;

							if ( m_nType != MOD_TYPE_IT ) { param <<= 1; }

							nGlbVol -= param * nMusicSpeed;
						}

						if ( nGlbVol < 0 ) { nGlbVol = 0; }

						if ( nGlbVol > 256 ) { nGlbVol = 256; }

						break;

					case CMD_CHANNELVOLUME:
							if ( param <= 64 ) { chnvols[nChn] = param; }

						break;

					case CMD_CHANNELVOLSLIDE:
							if ( param ) { oldparam[nChn] = param; }
							else { param = oldparam[nChn]; }

						pChn->nOldChnVolSlide = param;

						if ( ( ( param & 0x0F ) == 0x0F ) && ( param & 0xF0 ) )
						{
							param = ( param >> 4 ) + chnvols[nChn];
						}
						else if ( ( ( param & 0xF0 ) == 0xF0 ) && ( param & 0x0F ) )
						{
							if ( chnvols[nChn] > ( int )( param & 0x0F ) ) { param = chnvols[nChn] - ( param & 0x0F ); }
							else { param = 0; }
						}
						else if ( param & 0x0F )
						{
							param = ( param & 0x0F ) * nMusicSpeed;
							param = ( chnvols[nChn] > param ) ? chnvols[nChn] - param : 0;
						}
						else { param = ( ( param & 0xF0 ) >> 4 ) * nMusicSpeed + chnvols[nChn]; }

						if ( param > 64 ) { param = 64; }

						chnvols[nChn] = param;
						break;
				}
			}

		nSpeedCount += nMusicSpeed;
		dwElapsedTime += ( 2500 * nSpeedCount ) / nMusicTempo;
	}

	EndMod:

	if ( ( bAdjust ) && ( !bTotal ) )
	{
		m_nGlobalVolume = nGlbVol;
		m_nOldGlbVolSlide = nOldGlbVolSlide;

		for ( UINT n = 0; n < m_nChannels; n++ )
		{
			Chn[n].nGlobalVol = chnvols[n];

			if ( notes[n] ) { Chn[n].nNewNote = notes[n]; }

			if ( instr[n] ) { Chn[n].nNewIns = instr[n]; }

			if ( vols[n] != 0xFF )
			{
				if ( vols[n] > 64 ) { vols[n] = 64; }

				Chn[n].nVolume = vols[n] << 2;
			}
		}
	}

	return ( dwElapsedTime + 500 ) / 1000;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
// Effects

void CSoundFile::InstrumentChange( MODCHANNEL* pChn, UINT instr, BOOL bPorta, BOOL bUpdVol, BOOL bResetEnv )
//--------------------------------------------------------------------------------------------------------
{
	BOOL bInstrumentChanged = FALSE;

	if ( instr >= MAX_INSTRUMENTS ) { return; }

	INSTRUMENTHEADER* penv = Headers[instr];
	MODINSTRUMENT* psmp = &Ins[instr];
	UINT note = pChn->nNewNote;

	if ( ( penv ) && ( note ) && ( note <= 128 ) )
	{
		if ( penv->NoteMap[note - 1] >= 0xFE ) { return; }

		UINT n = penv->Keyboard[note - 1];
		psmp = ( ( n ) && ( n < MAX_SAMPLES ) ) ? &Ins[n] : NULL;
	}
	else if ( m_nInstruments )
	{
		if ( note >= 0xFE ) { return; }

		psmp = NULL;
	}

	// Update Volume
	if ( bUpdVol ) { pChn->nVolume = ( psmp ) ? psmp->nVolume : 0; }

	// bInstrumentChanged is used for IT carry-on env option
	if ( penv != pChn->pHeader )
	{
		bInstrumentChanged = TRUE;
		pChn->pHeader = penv;
	}
	else
	{
		// Special XM hack
		if ( ( bPorta ) && ( m_nType & ( MOD_TYPE_XM | MOD_TYPE_MT2 ) ) && ( penv )
		     && ( pChn->pInstrument ) && ( psmp != pChn->pInstrument ) )
		{
			// FT2 doesn't change the sample in this case,
			// but still uses the sample info from the old one (bug?)
			return;
		}
	}

	// Instrument adjust
	pChn->nNewIns = 0;

	if ( psmp )
	{
		if ( penv )
		{
			pChn->nInsVol = ( psmp->nGlobalVol * penv->nGlobalVol ) >> 6;

			if ( penv->dwFlags & ENV_SETPANNING ) { pChn->nPan = penv->nPan; }

			pChn->nNNA = penv->nNNA;
		}
		else
		{
			pChn->nInsVol = psmp->nGlobalVol;
		}

		if ( psmp->uFlags & CHN_PANNING ) { pChn->nPan = psmp->nPan; }
	}

	// Reset envelopes
	if ( bResetEnv )
	{
		if ( ( !bPorta ) || ( !( m_nType & MOD_TYPE_IT ) ) || ( m_dwSongFlags & SONG_ITCOMPATMODE )
		     || ( !pChn->nLength ) || ( ( pChn->dwFlags & CHN_NOTEFADE ) && ( !pChn->nFadeOutVol ) ) )
		{
			pChn->dwFlags |= CHN_FASTVOLRAMP;

			if ( ( m_nType & MOD_TYPE_IT ) && ( !bInstrumentChanged ) && ( penv ) && ( !( pChn->dwFlags & ( CHN_KEYOFF | CHN_NOTEFADE ) ) ) )
			{
				if ( !( penv->dwFlags & ENV_VOLCARRY ) ) { pChn->nVolEnvPosition = 0; }

				if ( !( penv->dwFlags & ENV_PANCARRY ) ) { pChn->nPanEnvPosition = 0; }

				if ( !( penv->dwFlags & ENV_PITCHCARRY ) ) { pChn->nPitchEnvPosition = 0; }
			}
			else
			{
				pChn->nVolEnvPosition = 0;
				pChn->nPanEnvPosition = 0;
				pChn->nPitchEnvPosition = 0;
			}

			pChn->nAutoVibDepth = 0;
			pChn->nAutoVibPos = 0;
		}
		else if ( ( penv ) && ( !( penv->dwFlags & ENV_VOLUME ) ) )
		{
			pChn->nVolEnvPosition = 0;
			pChn->nAutoVibDepth = 0;
			pChn->nAutoVibPos = 0;
		}
	}

	// Invalid sample ?
	if ( !psmp )
	{
		pChn->pInstrument = NULL;
		pChn->nInsVol = 0;
		return;
	}

	// Tone-Portamento doesn't reset the pingpong direction flag
	if ( ( bPorta ) && ( psmp == pChn->pInstrument ) )
	{
		if ( m_nType & ( MOD_TYPE_S3M | MOD_TYPE_IT ) ) { return; }

		pChn->dwFlags &= ~( CHN_KEYOFF | CHN_NOTEFADE );
		pChn->dwFlags = ( pChn->dwFlags & ( 0xFFFFFF00 | CHN_PINGPONGFLAG ) ) | ( psmp->uFlags );
	}
	else
	{
		pChn->dwFlags &= ~( CHN_KEYOFF | CHN_NOTEFADE | CHN_VOLENV | CHN_PANENV | CHN_PITCHENV );
		pChn->dwFlags = ( pChn->dwFlags & 0xFFFFFF00 ) | ( psmp->uFlags );

		if ( penv )
		{
			if ( penv->dwFlags & ENV_VOLUME ) { pChn->dwFlags |= CHN_VOLENV; }

			if ( penv->dwFlags & ENV_PANNING ) { pChn->dwFlags |= CHN_PANENV; }

			if ( penv->dwFlags & ENV_PITCH ) { pChn->dwFlags |= CHN_PITCHENV; }

			if ( ( penv->dwFlags & ENV_PITCH ) && ( penv->dwFlags & ENV_FILTER ) )
			{
				if ( !pChn->nCutOff ) { pChn->nCutOff = 0x7F; }
			}

			if ( penv->nIFC & 0x80 ) { pChn->nCutOff = penv->nIFC & 0x7F; }

			if ( penv->nIFR & 0x80 ) { pChn->nResonance = penv->nIFR & 0x7F; }
		}

		pChn->nVolSwing = pChn->nPanSwing = 0;
	}

	pChn->pInstrument = psmp;
	pChn->nLength = psmp->nLength;
	pChn->nLoopStart = psmp->nLoopStart;
	pChn->nLoopEnd = psmp->nLoopEnd;
	pChn->nC4Speed = psmp->nC4Speed;
	pChn->pSample = psmp->pSample;
	pChn->nTranspose = psmp->RelativeTone;
	pChn->nFineTune = psmp->nFineTune;

	if ( pChn->dwFlags & CHN_SUSTAINLOOP )
	{
		pChn->nLoopStart = psmp->nSustainStart;
		pChn->nLoopEnd = psmp->nSustainEnd;
		pChn->dwFlags |= CHN_LOOP;

		if ( pChn->dwFlags & CHN_PINGPONGSUSTAIN ) { pChn->dwFlags |= CHN_PINGPONGLOOP; }
	}

	if ( ( pChn->dwFlags & CHN_LOOP ) && ( pChn->nLoopEnd < pChn->nLength ) ) { pChn->nLength = pChn->nLoopEnd; }
}


void CSoundFile::NoteChange( UINT nChn, int note, BOOL bPorta, BOOL bResetEnv )
//---------------------------------------------------------------------------
{
	if ( note < 1 ) { return; }

	MODCHANNEL* const pChn = &Chn[nChn];
	MODINSTRUMENT* pins = pChn->pInstrument;
	INSTRUMENTHEADER* penv = pChn->pHeader;

	if ( ( penv ) && ( note <= 0x80 ) )
	{
		UINT n = penv->Keyboard[note - 1];

		if ( ( n ) && ( n < MAX_SAMPLES ) ) { pins = &Ins[n]; }

		note = penv->NoteMap[note - 1];
	}

	// Key Off
	if ( note >= 0x80 ) // 0xFE or invalid note => key off
	{
		// Key Off
		KeyOff( nChn );

		// Note Cut
		if ( note == 0xFE )
		{
			pChn->dwFlags |= ( CHN_NOTEFADE | CHN_FASTVOLRAMP );

			if ( ( !( m_nType & MOD_TYPE_IT ) ) || ( m_nInstruments ) ) { pChn->nVolume = 0; }

			pChn->nFadeOutVol = 0;
		}

		return;
	}

	if ( !pins ) { return; }

	if ( ( !bPorta ) && ( m_nType & ( MOD_TYPE_XM | MOD_TYPE_MED | MOD_TYPE_MT2 ) ) )
	{
		pChn->nTranspose = pins->RelativeTone;
		pChn->nFineTune = pins->nFineTune;
	}

	if ( m_nType & ( MOD_TYPE_XM | MOD_TYPE_MT2 | MOD_TYPE_MED ) ) { note += pChn->nTranspose; }

	if ( note < 1 ) { note = 1; }

	if ( note > 132 ) { note = 132; }

	pChn->nNote = note;

	if ( ( !bPorta ) || ( m_nType & ( MOD_TYPE_S3M | MOD_TYPE_IT ) ) ) { pChn->nNewIns = 0; }

	UINT period = GetPeriodFromNote( note, pChn->nFineTune, pChn->nC4Speed );

	if ( period )
	{
		if ( ( !bPorta ) || ( !pChn->nPeriod ) ) { pChn->nPeriod = period; }

		pChn->nPortamentoDest = period;

		if ( ( !bPorta ) || ( ( !pChn->nLength ) && ( !( m_nType & MOD_TYPE_S3M ) ) ) )
		{
			pChn->pInstrument = pins;
			pChn->pSample = pins->pSample;
			pChn->nLength = pins->nLength;
			pChn->nLoopEnd = pins->nLength;
			pChn->nLoopStart = 0;
			pChn->dwFlags = ( pChn->dwFlags & 0xFFFFFF00 ) | ( pins->uFlags );

			if ( pChn->dwFlags & CHN_SUSTAINLOOP )
			{
				pChn->nLoopStart = pins->nSustainStart;
				pChn->nLoopEnd = pins->nSustainEnd;
				pChn->dwFlags &= ~CHN_PINGPONGLOOP;
				pChn->dwFlags |= CHN_LOOP;

				if ( pChn->dwFlags & CHN_PINGPONGSUSTAIN ) { pChn->dwFlags |= CHN_PINGPONGLOOP; }

				if ( pChn->nLength > pChn->nLoopEnd ) { pChn->nLength = pChn->nLoopEnd; }
			}
			else if ( pChn->dwFlags & CHN_LOOP )
			{
				pChn->nLoopStart = pins->nLoopStart;
				pChn->nLoopEnd = pins->nLoopEnd;

				if ( pChn->nLength > pChn->nLoopEnd ) { pChn->nLength = pChn->nLoopEnd; }
			}

			pChn->nPos = 0;
			pChn->nPosLo = 0;

			if ( pChn->nVibratoType < 4 ) { pChn->nVibratoPos = ( ( m_nType & MOD_TYPE_IT ) && ( !( m_dwSongFlags & SONG_ITOLDEFFECTS ) ) ) ? 0x10 : 0; }

			if ( pChn->nTremoloType < 4 ) { pChn->nTremoloPos = 0; }
		}

		if ( pChn->nPos >= pChn->nLength ) { pChn->nPos = pChn->nLoopStart; }
	}
	else { bPorta = FALSE; }

	if ( ( !bPorta ) || ( !( m_nType & MOD_TYPE_IT ) )
	     || ( ( pChn->dwFlags & CHN_NOTEFADE ) && ( !pChn->nFadeOutVol ) )
	     || ( ( m_dwSongFlags & SONG_ITCOMPATMODE ) && ( pChn->nRowInstr ) ) )
	{
		if ( ( m_nType & MOD_TYPE_IT ) && ( pChn->dwFlags & CHN_NOTEFADE ) && ( !pChn->nFadeOutVol ) )
		{
			pChn->nVolEnvPosition = 0;
			pChn->nPanEnvPosition = 0;
			pChn->nPitchEnvPosition = 0;
			pChn->nAutoVibDepth = 0;
			pChn->nAutoVibPos = 0;
			pChn->dwFlags &= ~CHN_NOTEFADE;
			pChn->nFadeOutVol = 65536;
		}

		if ( ( !bPorta ) || ( !( m_dwSongFlags & SONG_ITCOMPATMODE ) ) || ( pChn->nRowInstr ) )
		{
			if ( ( !( m_nType & ( MOD_TYPE_XM | MOD_TYPE_MT2 ) ) ) || ( pChn->nRowInstr ) )
			{
				pChn->dwFlags &= ~CHN_NOTEFADE;
				pChn->nFadeOutVol = 65536;
			}
		}
	}

	pChn->dwFlags &= ~( CHN_EXTRALOUD | CHN_KEYOFF );

	// Enable Ramping
	if ( !bPorta )
	{
		pChn->nVUMeter = 0x100;
		pChn->nLeftVU = pChn->nRightVU = 0xFF;
		pChn->dwFlags &= ~CHN_FILTER;
		pChn->dwFlags |= CHN_FASTVOLRAMP;
		pChn->nRetrigCount = 0;
		pChn->nTremorCount = 0;

		if ( bResetEnv )
		{
			pChn->nVolSwing = pChn->nPanSwing = 0;

			if ( penv )
			{
				if ( !( penv->dwFlags & ENV_VOLCARRY ) ) { pChn->nVolEnvPosition = 0; }

				if ( !( penv->dwFlags & ENV_PANCARRY ) ) { pChn->nPanEnvPosition = 0; }

				if ( !( penv->dwFlags & ENV_PITCHCARRY ) ) { pChn->nPitchEnvPosition = 0; }

				if ( m_nType & MOD_TYPE_IT )
				{
					// Volume Swing
					if ( penv->nVolSwing )
					{
						int d = ( ( LONG )penv->nVolSwing * ( LONG )( ( rand() & 0xFF ) - 0x7F ) ) / 128;
						pChn->nVolSwing = ( signed short )( ( d * pChn->nVolume + 1 ) / 128 );
					}

					// Pan Swing
					if ( penv->nPanSwing )
					{
						int d = ( ( LONG )penv->nPanSwing * ( LONG )( ( rand() & 0xFF ) - 0x7F ) ) / 128;
						pChn->nPanSwing = ( signed short )d;
					}
				}
			}

			pChn->nAutoVibDepth = 0;
			pChn->nAutoVibPos = 0;
		}

		pChn->nLeftVol = pChn->nRightVol = 0;
		BOOL bFlt = ( m_dwSongFlags & SONG_MPTFILTERMODE ) ? FALSE : TRUE;

		// Setup Initial Filter for this note
		if ( penv )
		{
			if ( penv->nIFR & 0x80 ) { pChn->nResonance = penv->nIFR & 0x7F; bFlt = TRUE; }

			if ( penv->nIFC & 0x80 ) { pChn->nCutOff = penv->nIFC & 0x7F; bFlt = TRUE; }
		}
		else
		{
			pChn->nVolSwing = pChn->nPanSwing = 0;
		}

#ifndef NO_FILTER

		if ( ( pChn->nCutOff < 0x7F ) && ( bFlt ) ) { SetupChannelFilter( pChn, TRUE ); }

#endif // NO_FILTER
	}
}


UINT CSoundFile::GetNNAChannel( UINT nChn ) const
//---------------------------------------------
{
	const MODCHANNEL* pChn = &Chn[nChn];
	// Check for empty channel
	const MODCHANNEL* pi = &Chn[m_nChannels];

	for ( UINT i = m_nChannels; i < MAX_CHANNELS; i++, pi++ ) if ( !pi->nLength ) { return i; }

	if ( !pChn->nFadeOutVol ) { return 0; }

	// All channels are used: check for lowest volume
	UINT result = 0;
	DWORD vol = 64 * 65536; // 25%
	DWORD envpos = 0xFFFFFF;
	const MODCHANNEL* pj = &Chn[m_nChannels];

	for ( UINT j = m_nChannels; j < MAX_CHANNELS; j++, pj++ )
	{
		if ( !pj->nFadeOutVol ) { return j; }

		DWORD v = pj->nVolume;

		if ( pj->dwFlags & CHN_NOTEFADE )
		{
			v = v * pj->nFadeOutVol;
		}
		else
		{
			v <<= 16;
		}

		if ( pj->dwFlags & CHN_LOOP ) { v >>= 1; }

		if ( ( v < vol ) || ( ( v == vol ) && ( pj->nVolEnvPosition > envpos ) ) )
		{
			envpos = pj->nVolEnvPosition;
			vol = v;
			result = j;
		}
	}

	return result;
}


void CSoundFile::CheckNNA( UINT nChn, UINT instr, int note, BOOL bForceCut )
//------------------------------------------------------------------------
{
	MODCHANNEL* pChn = &Chn[nChn];
	INSTRUMENTHEADER* penv = pChn->pHeader, *pHeader = 0;
	signed char* pSample;

	if ( note > 0x80 ) { note = 0; }

	if ( note < 1 ) { return; }

	// Always NNA cut - using
	if ( ( !( m_nType & ( MOD_TYPE_IT | MOD_TYPE_MT2 ) ) ) || ( !m_nInstruments ) || ( bForceCut ) )
	{
		if ( ( m_dwSongFlags & SONG_CPUVERYHIGH )
		     || ( !pChn->nLength ) || ( pChn->dwFlags & CHN_MUTE )
		     || ( ( !pChn->nLeftVol ) && ( !pChn->nRightVol ) ) ) { return; }

		UINT n = GetNNAChannel( nChn );

		if ( !n ) { return; }

		MODCHANNEL* p = &Chn[n];
		// Copy Channel
		*p = *pChn;
		p->dwFlags &= ~( CHN_VIBRATO | CHN_TREMOLO | CHN_PANBRELLO | CHN_MUTE | CHN_PORTAMENTO );
		p->nMasterChn = nChn + 1;
		p->nCommand = 0;
		// Cut the note
		p->nFadeOutVol = 0;
		p->dwFlags |= ( CHN_NOTEFADE | CHN_FASTVOLRAMP );
		// Stop this channel
		pChn->nLength = pChn->nPos = pChn->nPosLo = 0;
		pChn->nROfs = pChn->nLOfs = 0;
		pChn->nLeftVol = pChn->nRightVol = 0;
		return;
	}

	if ( instr >= MAX_INSTRUMENTS ) { instr = 0; }

	pSample = pChn->pSample;
	pHeader = pChn->pHeader;

	if ( ( instr ) && ( note ) )
	{
		pHeader = Headers[instr];

		if ( pHeader )
		{
			UINT n = 0;

			if ( note <= 0x80 )
			{
				n = pHeader->Keyboard[note - 1];
				note = pHeader->NoteMap[note - 1];

				if ( ( n ) && ( n < MAX_SAMPLES ) ) { pSample = Ins[n].pSample; }
			}
		}
		else { pSample = NULL; }
	}

	if ( !penv ) { return; }

	MODCHANNEL* p = pChn;

	for ( UINT i = nChn; i < MAX_CHANNELS; p++, i++ )
		if ( ( i >= m_nChannels ) || ( p == pChn ) )
		{
			if ( ( ( p->nMasterChn == nChn + 1 ) || ( p == pChn ) ) && ( p->pHeader ) )
			{
				BOOL bOk = FALSE;

				// Duplicate Check Type
				switch ( p->pHeader->nDCT )
				{
						// Note
					case DCT_NOTE:
							if ( ( note ) && ( p->nNote == note ) && ( pHeader == p->pHeader ) ) { bOk = TRUE; }

						break;

						// Sample
					case DCT_SAMPLE:
							if ( ( pSample ) && ( pSample == p->pSample ) ) { bOk = TRUE; }

						break;

						// Instrument
					case DCT_INSTRUMENT:
							if ( pHeader == p->pHeader ) { bOk = TRUE; }

						break;
				}

				// Duplicate Note Action
				if ( bOk )
				{
					switch ( p->pHeader->nDNA )
					{
							// Cut
						case DNA_NOTECUT:
								KeyOff( i );
							p->nVolume = 0;
							break;

							// Note Off
						case DNA_NOTEOFF:
								KeyOff( i );
							break;

							// Note Fade
						case DNA_NOTEFADE:
								p->dwFlags |= CHN_NOTEFADE;
							break;
					}

					if ( !p->nVolume )
					{
						p->nFadeOutVol = 0;
						p->dwFlags |= ( CHN_NOTEFADE | CHN_FASTVOLRAMP );
					}
				}
			}
		}

	if ( pChn->dwFlags & CHN_MUTE ) { return; }

	// New Note Action
	if ( ( pChn->nVolume ) && ( pChn->nLength ) )
	{
		UINT n = GetNNAChannel( nChn );

		if ( n )
		{
			MODCHANNEL* p = &Chn[n];
			// Copy Channel
			*p = *pChn;
			p->dwFlags &= ~( CHN_VIBRATO | CHN_TREMOLO | CHN_PANBRELLO | CHN_MUTE | CHN_PORTAMENTO );
			p->nMasterChn = nChn + 1;
			p->nCommand = 0;

			// Key Off the note
			switch ( pChn->nNNA )
			{
				case NNA_NOTEOFF:
						KeyOff( n );
					break;

				case NNA_NOTECUT:
						p->nFadeOutVol = 0;

				case NNA_NOTEFADE:
						p->dwFlags |= CHN_NOTEFADE;
					break;
			}

			if ( !p->nVolume )
			{
				p->nFadeOutVol = 0;
				p->dwFlags |= ( CHN_NOTEFADE | CHN_FASTVOLRAMP );
			}

			// Stop this channel
			pChn->nLength = pChn->nPos = pChn->nPosLo = 0;
			pChn->nROfs = pChn->nLOfs = 0;
		}
	}
}


BOOL CSoundFile::ProcessEffects()
//-------------------------------
{
	MODCHANNEL* pChn = Chn;
	int nBreakRow = -1, nPosJump = -1, nPatLoopRow = -1;

	for ( UINT nChn = 0; nChn < m_nChannels; nChn++, pChn++ )
	{
		UINT instr = pChn->nRowInstr;
		UINT volcmd = pChn->nRowVolCmd;
		UINT vol = pChn->nRowVolume;
		UINT cmd = pChn->nRowCommand;
		UINT param = pChn->nRowParam;
		bool bPorta = ( ( cmd != CMD_TONEPORTAMENTO ) && ( cmd != CMD_TONEPORTAVOL ) && ( volcmd != VOLCMD_TONEPORTAMENTO ) ) ? FALSE : TRUE;
		UINT nStartTick = 0;

		pChn->dwFlags &= ~CHN_FASTVOLRAMP;

		// Process special effects (note delay, pattern delay, pattern loop)
		if ( ( cmd == CMD_MODCMDEX ) || ( cmd == CMD_S3MCMDEX ) )
		{
			if ( ( !param ) && ( m_nType & ( MOD_TYPE_S3M | MOD_TYPE_IT ) ) ) { param = pChn->nOldCmdEx; }
			else { pChn->nOldCmdEx = param; }

			// Note Delay ?
			if ( ( param & 0xF0 ) == 0xD0 )
			{
				nStartTick = param & 0x0F;
			}
			else if ( !m_nTickCount )
			{
				// Pattern Loop ?
				if ( ( ( ( param & 0xF0 ) == 0x60 ) && ( cmd == CMD_MODCMDEX ) )
				     || ( ( ( param & 0xF0 ) == 0xB0 ) && ( cmd == CMD_S3MCMDEX ) ) )
				{
					int nloop = PatternLoop( pChn, param & 0x0F );

					if ( nloop >= 0 ) { nPatLoopRow = nloop; }
				}
				else

					// Pattern Delay
					if ( ( param & 0xF0 ) == 0xE0 )
					{
						m_nPatternDelay = param & 0x0F;
					}
			}
		}

		// Handles note/instrument/volume changes
		if ( m_nTickCount == nStartTick ) // can be delayed by a note delay effect
		{
			UINT note = pChn->nRowNote;

			if ( instr ) { pChn->nNewIns = instr; }

			// XM: Key-Off + Sample == Note Cut
			if ( m_nType & ( MOD_TYPE_MOD | MOD_TYPE_XM | MOD_TYPE_MT2 ) )
			{
				if ( ( note == 0xFF ) && ( ( !pChn->pHeader ) || ( !( pChn->pHeader->dwFlags & ENV_VOLUME ) ) ) )
				{
					pChn->dwFlags |= CHN_FASTVOLRAMP;
					pChn->nVolume = 0;
					note = instr = 0;
				}
			}

			if ( ( !note ) && ( instr ) )
			{
				if ( m_nInstruments )
				{
					if ( pChn->pInstrument ) { pChn->nVolume = pChn->pInstrument->nVolume; }

					if ( m_nType & ( MOD_TYPE_XM | MOD_TYPE_MT2 ) )
					{
						pChn->dwFlags |= CHN_FASTVOLRAMP;
						pChn->nVolEnvPosition = 0;
						pChn->nPanEnvPosition = 0;
						pChn->nPitchEnvPosition = 0;
						pChn->nAutoVibDepth = 0;
						pChn->nAutoVibPos = 0;
						pChn->dwFlags &= ~CHN_NOTEFADE;
						pChn->nFadeOutVol = 65536;
					}
				}
				else
				{
					if ( instr < MAX_SAMPLES ) { pChn->nVolume = Ins[instr].nVolume; }
				}

				if ( !( m_nType & MOD_TYPE_IT ) ) { instr = 0; }
			}

			// Invalid Instrument ?
			if ( instr >= MAX_INSTRUMENTS ) { instr = 0; }

			// Note Cut/Off => ignore instrument
			if ( note >= 0xFE ) { instr = 0; }

			if ( ( note ) && ( note <= 128 ) ) { pChn->nNewNote = note; }

			// New Note Action ?
			if ( ( note ) && ( note <= 128 ) && ( !bPorta ) )
			{
				CheckNNA( nChn, instr, note, FALSE );
			}

			// Instrument Change ?
			if ( instr )
			{
				MODINSTRUMENT* psmp = pChn->pInstrument;
				InstrumentChange( pChn, instr, bPorta, TRUE );
				pChn->nNewIns = 0;

				// Special IT case: portamento+note causes sample change -> ignore portamento
				if ( ( m_nType & ( MOD_TYPE_S3M | MOD_TYPE_IT ) )
				     && ( psmp != pChn->pInstrument ) && ( note ) && ( note < 0x80 ) )
				{
					bPorta = FALSE;
				}
			}

			// New Note ?
			if ( note )
			{
				if ( ( !instr ) && ( pChn->nNewIns ) && ( note < 0x80 ) )
				{
					InstrumentChange( pChn, pChn->nNewIns, bPorta, FALSE, ( m_nType & ( MOD_TYPE_XM | MOD_TYPE_MT2 ) ) ? FALSE : TRUE );
					pChn->nNewIns = 0;
				}

				NoteChange( nChn, note, bPorta, ( m_nType & ( MOD_TYPE_XM | MOD_TYPE_MT2 ) ) ? FALSE : TRUE );

				if ( ( bPorta ) && ( m_nType & ( MOD_TYPE_XM | MOD_TYPE_MT2 ) ) && ( instr ) )
				{
					pChn->dwFlags |= CHN_FASTVOLRAMP;
					pChn->nVolEnvPosition = 0;
					pChn->nPanEnvPosition = 0;
					pChn->nPitchEnvPosition = 0;
					pChn->nAutoVibDepth = 0;
					pChn->nAutoVibPos = 0;
				}
			}

			// Tick-0 only volume commands
			if ( volcmd == VOLCMD_VOLUME )
			{
				if ( vol > 64 ) { vol = 64; }

				pChn->nVolume = vol << 2;
				pChn->dwFlags |= CHN_FASTVOLRAMP;
			}
			else if ( volcmd == VOLCMD_PANNING )
			{
				if ( vol > 64 ) { vol = 64; }

				pChn->nPan = vol << 2;
				pChn->dwFlags |= CHN_FASTVOLRAMP;
			}
		}

		// Volume Column Effect (except volume & panning)
		if ( ( volcmd > VOLCMD_PANNING ) && ( m_nTickCount >= nStartTick ) )
		{
			if ( volcmd == VOLCMD_TONEPORTAMENTO )
			{
				if ( m_nType & MOD_TYPE_IT )
				{
					TonePortamento( pChn, ImpulseTrackerPortaVolCmd[vol & 0x0F] );
				}
				else
				{
					TonePortamento( pChn, vol * 16 );
				}
			}
			else
			{
				if ( vol ) { pChn->nOldVolParam = vol; }
				else { vol = pChn->nOldVolParam; }

				switch ( volcmd )
				{
					case VOLCMD_VOLSLIDEUP:
							VolumeSlide( pChn, vol << 4 );
						break;

					case VOLCMD_VOLSLIDEDOWN:
							VolumeSlide( pChn, vol );
						break;

					case VOLCMD_FINEVOLUP:
							if ( m_nType & MOD_TYPE_IT )
							{
								if ( m_nTickCount == nStartTick ) { VolumeSlide( pChn, ( vol << 4 ) | 0x0F ); }
							}
							else
							{
								FineVolumeUp( pChn, vol );
							}

						break;

					case VOLCMD_FINEVOLDOWN:
							if ( m_nType & MOD_TYPE_IT )
							{
								if ( m_nTickCount == nStartTick ) { VolumeSlide( pChn, 0xF0 | vol ); }
							}
							else
							{
								FineVolumeDown( pChn, vol );
							}

						break;

					case VOLCMD_VIBRATOSPEED:
							Vibrato( pChn, vol << 4 );
						break;

					case VOLCMD_VIBRATO:
							Vibrato( pChn, vol );
						break;

					case VOLCMD_PANSLIDELEFT:
							PanningSlide( pChn, vol );
						break;

					case VOLCMD_PANSLIDERIGHT:
							PanningSlide( pChn, vol << 4 );
						break;

					case VOLCMD_PORTAUP:
							PortamentoUp( pChn, vol << 2 );
						break;

					case VOLCMD_PORTADOWN:
							PortamentoDown( pChn, vol << 2 );
						break;
				}
			}
		}

		// Effects
		if ( cmd ) switch ( cmd )
			{
					// Set Volume
				case CMD_VOLUME:
						if ( !m_nTickCount )
						{
							pChn->nVolume = ( param < 64 ) ? param * 4 : 256;
							pChn->dwFlags |= CHN_FASTVOLRAMP;
						}

					break;

					// Portamento Up
				case CMD_PORTAMENTOUP:
						if ( ( !param ) && ( m_nType & MOD_TYPE_MOD ) ) { break; }

					PortamentoUp( pChn, param );
					break;

					// Portamento Down
				case CMD_PORTAMENTODOWN:
						if ( ( !param ) && ( m_nType & MOD_TYPE_MOD ) ) { break; }

					PortamentoDown( pChn, param );
					break;

					// Volume Slide
				case CMD_VOLUMESLIDE:
						if ( ( param ) || ( m_nType != MOD_TYPE_MOD ) ) { VolumeSlide( pChn, param ); }

					break;

					// Tone-Portamento
				case CMD_TONEPORTAMENTO:
						TonePortamento( pChn, param );
					break;

					// Tone-Portamento + Volume Slide
				case CMD_TONEPORTAVOL:
						if ( ( param ) || ( m_nType != MOD_TYPE_MOD ) ) { VolumeSlide( pChn, param ); }

					TonePortamento( pChn, 0 );
					break;

					// Vibrato
				case CMD_VIBRATO:
						Vibrato( pChn, param );
					break;

					// Vibrato + Volume Slide
				case CMD_VIBRATOVOL:
						if ( ( param ) || ( m_nType != MOD_TYPE_MOD ) ) { VolumeSlide( pChn, param ); }

					Vibrato( pChn, 0 );
					break;

					// Set Speed
				case CMD_SPEED:
						if ( !m_nTickCount ) { SetSpeed( param ); }

					break;

					// Set Tempo
				case CMD_TEMPO:
						if ( !m_nTickCount )
						{
							if ( m_nType & ( MOD_TYPE_S3M | MOD_TYPE_IT ) )
							{
								if ( param ) { pChn->nOldTempo = param; }
								else { param = pChn->nOldTempo; }
							}

							SetTempo( param );
						}

					break;

					// Set Offset
				case CMD_OFFSET:
						if ( m_nTickCount ) { break; }

					if ( param ) { pChn->nOldOffset = param; }
					else { param = pChn->nOldOffset; }

					param <<= 8;
					param |= ( UINT )( pChn->nOldHiOffset ) << 16;

					if ( ( pChn->nRowNote ) && ( pChn->nRowNote < 0x80 ) )
					{
						if ( bPorta )
						{
							pChn->nPos = param;
						}
						else
						{
							pChn->nPos += param;
						}

						if ( pChn->nPos >= pChn->nLength )
						{
							if ( !( m_nType & ( MOD_TYPE_XM | MOD_TYPE_MT2 ) ) )
							{
								pChn->nPos = pChn->nLoopStart;

								if ( ( m_dwSongFlags & SONG_ITOLDEFFECTS ) && ( pChn->nLength > 4 ) )
								{
									pChn->nPos = pChn->nLength - 2;
								}
							}
						}
					}
					else if ( ( param < pChn->nLength ) && ( m_nType & ( MOD_TYPE_MTM | MOD_TYPE_DMF ) ) )
					{
						pChn->nPos = param;
					}

					break;

					// Arpeggio
				case CMD_ARPEGGIO:
						if ( ( m_nTickCount ) || ( !pChn->nPeriod ) || ( !pChn->nNote ) ) { break; }

					if ( ( !param ) && ( !( m_nType & ( MOD_TYPE_S3M | MOD_TYPE_IT ) ) ) ) { break; }

					pChn->nCommand = CMD_ARPEGGIO;

					if ( param ) { pChn->nArpeggio = param; }

					break;

					// Retrig
				case CMD_RETRIG:
						if ( m_nType & ( MOD_TYPE_XM | MOD_TYPE_MT2 ) )
						{
							if ( !( param & 0xF0 ) ) { param |= pChn->nRetrigParam & 0xF0; }

							if ( !( param & 0x0F ) ) { param |= pChn->nRetrigParam & 0x0F; }

							param |= 0x100; // increment retrig count on first row
						}

					if ( param ) { pChn->nRetrigParam = ( BYTE )( param & 0xFF ); }
					else { param = pChn->nRetrigParam; }

					RetrigNote( nChn, param );
					break;

					// Tremor
				case CMD_TREMOR:
						if ( m_nTickCount ) { break; }

					pChn->nCommand = CMD_TREMOR;

					if ( param ) { pChn->nTremorParam = param; }

					break;

					// Set Global Volume
				case CMD_GLOBALVOLUME:
						if ( m_nTickCount ) { break; }

					if ( m_nType != MOD_TYPE_IT ) { param <<= 1; }

					if ( param > 128 ) { param = 128; }

					m_nGlobalVolume = param << 1;
					break;

					// Global Volume Slide
				case CMD_GLOBALVOLSLIDE:
						GlobalVolSlide( param );
					break;

					// Set 8-bit Panning
				case CMD_PANNING8:
						if ( m_nTickCount ) { break; }

					if ( !( m_dwSongFlags & SONG_SURROUNDPAN ) ) { pChn->dwFlags &= ~CHN_SURROUND; }

					if ( m_nType & ( MOD_TYPE_IT | MOD_TYPE_XM | MOD_TYPE_MT2 ) )
					{
						pChn->nPan = param;
					}
					else if ( param <= 0x80 )
					{
						pChn->nPan = param << 1;
					}
					else if ( param == 0xA4 )
					{
						pChn->dwFlags |= CHN_SURROUND;
						pChn->nPan = 0x80;
					}

					pChn->dwFlags |= CHN_FASTVOLRAMP;
					break;

					// Panning Slide
				case CMD_PANNINGSLIDE:
						PanningSlide( pChn, param );
					break;

					// Tremolo
				case CMD_TREMOLO:
						Tremolo( pChn, param );
					break;

					// Fine Vibrato
				case CMD_FINEVIBRATO:
						FineVibrato( pChn, param );
					break;

					// MOD/XM Exx Extended Commands
				case CMD_MODCMDEX:
						ExtendedMODCommands( nChn, param );
					break;

					// S3M/IT Sxx Extended Commands
				case CMD_S3MCMDEX:
						ExtendedS3MCommands( nChn, param );
					break;

					// Key Off
				case CMD_KEYOFF:
						if ( !m_nTickCount ) { KeyOff( nChn ); }

					break;

					// Extra-fine porta up/down
				case CMD_XFINEPORTAUPDOWN:
						switch ( param & 0xF0 )
						{
							case 0x10:
									ExtraFinePortamentoUp( pChn, param & 0x0F );
								break;

							case 0x20:
									ExtraFinePortamentoDown( pChn, param & 0x0F );
								break;

								// Modplug XM Extensions
							case 0x50:
								case 0x60:
									case 0x70:
										case 0x90:
											case 0xA0:
													ExtendedS3MCommands( nChn, param );
								break;
						}

					break;

					// Set Channel Global Volume
				case CMD_CHANNELVOLUME:
						if ( m_nTickCount ) { break; }

					if ( param <= 64 )
					{
						pChn->nGlobalVol = param;
						pChn->dwFlags |= CHN_FASTVOLRAMP;
					}

					break;

					// Channel volume slide
				case CMD_CHANNELVOLSLIDE:
						ChannelVolSlide( pChn, param );
					break;

					// Panbrello (IT)
				case CMD_PANBRELLO:
						Panbrello( pChn, param );
					break;

					// Set Envelope Position
				case CMD_SETENVPOSITION:
						if ( !m_nTickCount )
						{
							pChn->nVolEnvPosition = param;
							pChn->nPanEnvPosition = param;
							pChn->nPitchEnvPosition = param;

							if ( pChn->pHeader )
							{
								INSTRUMENTHEADER* penv = pChn->pHeader;

								if ( ( pChn->dwFlags & CHN_PANENV ) && ( penv->nPanEnv ) && ( param > penv->PanPoints[penv->nPanEnv - 1] ) )
								{
									pChn->dwFlags &= ~CHN_PANENV;
								}
							}
						}

					break;

					// Position Jump
				case CMD_POSITIONJUMP:
						nPosJump = param;
					break;

					// Pattern Break
				case CMD_PATTERNBREAK:
						nBreakRow = param;
					break;

					// Midi Controller
				case CMD_MIDI:
						if ( m_nTickCount ) { break; }

					if ( param < 0x80 )
					{
						ProcessMidiMacro( nChn, &m_MidiCfg.szMidiSFXExt[pChn->nActiveMacro << 5], param );
					}
					else
					{
						ProcessMidiMacro( nChn, &m_MidiCfg.szMidiZXXExt[( param & 0x7F ) << 5], 0 );
					}

					break;
			}
	}

	// Navigation Effects
	if ( !m_nTickCount )
	{
		// Pattern Loop
		if ( nPatLoopRow >= 0 )
		{
			m_nNextPattern = m_nCurrentPattern;
			m_nNextRow = nPatLoopRow;

			if ( m_nPatternDelay ) { m_nNextRow++; }
		}
		else

			// Pattern Break / Position Jump only if no loop running
			if ( ( nBreakRow >= 0 ) || ( nPosJump >= 0 ) )
			{
				BOOL bNoLoop = FALSE;

				if ( nPosJump < 0 ) { nPosJump = m_nCurrentPattern + 1; }

				if ( nBreakRow < 0 ) { nBreakRow = 0; }

				// Modplug Tracker & ModPlugin allow backward jumps
#ifndef MODPLUG_FASTSOUNDLIB

				if ( ( nPosJump < ( int )m_nCurrentPattern )
				     || ( ( nPosJump == ( int )m_nCurrentPattern ) && ( nBreakRow <= ( int )m_nRow ) ) )
				{
					if ( !IsValidBackwardJump( m_nCurrentPattern, m_nRow, nPosJump, nBreakRow ) )
					{
						if ( m_nRepeatCount )
						{
							if ( m_nRepeatCount > 0 ) { m_nRepeatCount--; }
						}
						else
						{
#ifdef MODPLUG_TRACKER

							if ( gdwSoundSetup & SNDMIX_NOBACKWARDJUMPS )
#endif
								// Backward jump disabled
								bNoLoop = TRUE;

							//reset repeat count incase there are multiple loops.
							//(i.e. Unreal tracks)
							m_nRepeatCount = m_nInitialRepeatCount;
						}
					}
				}

#endif   // MODPLUG_FASTSOUNDLIB

				if ( ( ( !bNoLoop ) && ( nPosJump < MAX_ORDERS ) )
				     && ( ( nPosJump != ( int )m_nCurrentPattern ) || ( nBreakRow != ( int )m_nRow ) ) )
				{
					if ( nPosJump != ( int )m_nCurrentPattern )
					{
						for ( UINT i = 0; i < m_nChannels; i++ ) { Chn[i].nPatternLoopCount = 0; }
					}

					m_nNextPattern = nPosJump;
					m_nNextRow = ( UINT )nBreakRow;
				}
			}
	}

	return TRUE;
}


////////////////////////////////////////////////////////////
// Channels effects

void CSoundFile::PortamentoUp( MODCHANNEL* pChn, UINT param )
//---------------------------------------------------------
{
	if ( param ) { pChn->nOldPortaUpDown = param; }
	else { param = pChn->nOldPortaUpDown; }

	if ( ( m_nType & ( MOD_TYPE_S3M | MOD_TYPE_IT | MOD_TYPE_STM ) ) && ( ( param & 0xF0 ) >= 0xE0 ) )
	{
		if ( param & 0x0F )
		{
			if ( ( param & 0xF0 ) == 0xF0 )
			{
				FinePortamentoUp( pChn, param & 0x0F );
			}
			else if ( ( param & 0xF0 ) == 0xE0 )
			{
				ExtraFinePortamentoUp( pChn, param & 0x0F );
			}
		}

		return;
	}

	// Regular Slide
	if ( !( m_dwSongFlags & SONG_FIRSTTICK ) || ( m_nMusicSpeed == 1 ) ) //rewbs.PortaA01fix
	{
		DoFreqSlide( pChn, -( int )( param * 4 ) );
	}
}


void CSoundFile::PortamentoDown( MODCHANNEL* pChn, UINT param )
//-----------------------------------------------------------
{
	if ( param ) { pChn->nOldPortaUpDown = param; }
	else { param = pChn->nOldPortaUpDown; }

	if ( ( m_nType & ( MOD_TYPE_S3M | MOD_TYPE_IT | MOD_TYPE_STM ) ) && ( ( param & 0xF0 ) >= 0xE0 ) )
	{
		if ( param & 0x0F )
		{
			if ( ( param & 0xF0 ) == 0xF0 )
			{
				FinePortamentoDown( pChn, param & 0x0F );
			}
			else if ( ( param & 0xF0 ) == 0xE0 )
			{
				ExtraFinePortamentoDown( pChn, param & 0x0F );
			}
		}

		return;
	}

	if ( !( m_dwSongFlags & SONG_FIRSTTICK ) || ( m_nMusicSpeed == 1 ) ) //rewbs.PortaA01fix
	{
		DoFreqSlide( pChn, ( int )( param << 2 ) );
	}
}


void CSoundFile::FinePortamentoUp( MODCHANNEL* pChn, UINT param )
//-------------------------------------------------------------
{
	if ( m_nType & ( MOD_TYPE_XM | MOD_TYPE_MT2 ) )
	{
		if ( param ) { pChn->nOldFinePortaUpDown = param; }
		else { param = pChn->nOldFinePortaUpDown; }
	}

	if ( m_dwSongFlags & SONG_FIRSTTICK )
	{
		if ( ( pChn->nPeriod ) && ( param ) )
		{
			if ( ( m_dwSongFlags & SONG_LINEARSLIDES ) && ( !( m_nType & ( MOD_TYPE_XM | MOD_TYPE_MT2 ) ) ) )
			{
				pChn->nPeriod = _muldivr( pChn->nPeriod, LinearSlideDownTable[param & 0x0F], 65536 );
			}
			else
			{
				pChn->nPeriod -= ( int )( param * 4 );
			}

			if ( pChn->nPeriod < 1 ) { pChn->nPeriod = 1; }
		}
	}
}


void CSoundFile::FinePortamentoDown( MODCHANNEL* pChn, UINT param )
//---------------------------------------------------------------
{
	if ( m_nType & ( MOD_TYPE_XM | MOD_TYPE_MT2 ) )
	{
		if ( param ) { pChn->nOldFinePortaUpDown = param; }
		else { param = pChn->nOldFinePortaUpDown; }
	}

	if ( m_dwSongFlags & SONG_FIRSTTICK )
	{
		if ( ( pChn->nPeriod ) && ( param ) )
		{
			if ( ( m_dwSongFlags & SONG_LINEARSLIDES ) && ( !( m_nType & ( MOD_TYPE_XM | MOD_TYPE_MT2 ) ) ) )
			{
				pChn->nPeriod = _muldivr( pChn->nPeriod, LinearSlideUpTable[param & 0x0F], 65536 );
			}
			else
			{
				pChn->nPeriod += ( int )( param * 4 );
			}

			if ( pChn->nPeriod > 0xFFFF ) { pChn->nPeriod = 0xFFFF; }
		}
	}
}


void CSoundFile::ExtraFinePortamentoUp( MODCHANNEL* pChn, UINT param )
//------------------------------------------------------------------
{
	if ( m_nType & ( MOD_TYPE_XM | MOD_TYPE_MT2 ) )
	{
		if ( param ) { pChn->nOldFinePortaUpDown = param; }
		else { param = pChn->nOldFinePortaUpDown; }
	}

	if ( m_dwSongFlags & SONG_FIRSTTICK )
	{
		if ( ( pChn->nPeriod ) && ( param ) )
		{
			if ( ( m_dwSongFlags & SONG_LINEARSLIDES ) && ( !( m_nType & ( MOD_TYPE_XM | MOD_TYPE_MT2 ) ) ) )
			{
				pChn->nPeriod = _muldivr( pChn->nPeriod, FineLinearSlideDownTable[param & 0x0F], 65536 );
			}
			else
			{
				pChn->nPeriod -= ( int )( param );
			}

			if ( pChn->nPeriod < 1 ) { pChn->nPeriod = 1; }
		}
	}
}


void CSoundFile::ExtraFinePortamentoDown( MODCHANNEL* pChn, UINT param )
//--------------------------------------------------------------------
{
	if ( m_nType & ( MOD_TYPE_XM | MOD_TYPE_MT2 ) )
	{
		if ( param ) { pChn->nOldFinePortaUpDown = param; }
		else { param = pChn->nOldFinePortaUpDown; }
	}

	if ( m_dwSongFlags & SONG_FIRSTTICK )
	{
		if ( ( pChn->nPeriod ) && ( param ) )
		{
			if ( ( m_dwSongFlags & SONG_LINEARSLIDES ) && ( !( m_nType & ( MOD_TYPE_XM | MOD_TYPE_MT2 ) ) ) )
			{
				pChn->nPeriod = _muldivr( pChn->nPeriod, FineLinearSlideUpTable[param & 0x0F], 65536 );
			}
			else
			{
				pChn->nPeriod += ( int )( param );
			}

			if ( pChn->nPeriod > 0xFFFF ) { pChn->nPeriod = 0xFFFF; }
		}
	}
}


// Portamento Slide
void CSoundFile::TonePortamento( MODCHANNEL* pChn, UINT param )
//-----------------------------------------------------------
{
	if ( param ) { pChn->nPortamentoSlide = param * 4; }

	pChn->dwFlags |= CHN_PORTAMENTO;

	if ( ( pChn->nPeriod ) && ( pChn->nPortamentoDest ) && ( !( m_dwSongFlags & SONG_FIRSTTICK ) ) )
	{
		if ( pChn->nPeriod < pChn->nPortamentoDest )
		{
			LONG delta = ( int )pChn->nPortamentoSlide;

			if ( ( m_dwSongFlags & SONG_LINEARSLIDES ) && ( !( m_nType & ( MOD_TYPE_XM | MOD_TYPE_MT2 ) ) ) )
			{
				UINT n = pChn->nPortamentoSlide >> 2;

				if ( n > 255 ) { n = 255; }

				delta = _muldivr( pChn->nPeriod, LinearSlideUpTable[n], 65536 ) - pChn->nPeriod;

				if ( delta < 1 ) { delta = 1; }
			}

			pChn->nPeriod += delta;

			if ( pChn->nPeriod > pChn->nPortamentoDest ) { pChn->nPeriod = pChn->nPortamentoDest; }
		}
		else if ( pChn->nPeriod > pChn->nPortamentoDest )
		{
			LONG delta = - ( int )pChn->nPortamentoSlide;

			if ( ( m_dwSongFlags & SONG_LINEARSLIDES ) && ( !( m_nType & ( MOD_TYPE_XM | MOD_TYPE_MT2 ) ) ) )
			{
				UINT n = pChn->nPortamentoSlide >> 2;

				if ( n > 255 ) { n = 255; }

				delta = _muldivr( pChn->nPeriod, LinearSlideDownTable[n], 65536 ) - pChn->nPeriod;

				if ( delta > -1 ) { delta = -1; }
			}

			pChn->nPeriod += delta;

			if ( pChn->nPeriod < pChn->nPortamentoDest ) { pChn->nPeriod = pChn->nPortamentoDest; }
		}
	}
}


void CSoundFile::Vibrato( MODCHANNEL* p, UINT param )
//-------------------------------------------------
{
	if ( param & 0x0F ) { p->nVibratoDepth = ( param & 0x0F ) * 4; }

	if ( param & 0xF0 ) { p->nVibratoSpeed = ( param >> 4 ) & 0x0F; }

	p->dwFlags |= CHN_VIBRATO;
}


void CSoundFile::FineVibrato( MODCHANNEL* p, UINT param )
//-----------------------------------------------------
{
	if ( param & 0x0F ) { p->nVibratoDepth = param & 0x0F; }

	if ( param & 0xF0 ) { p->nVibratoSpeed = ( param >> 4 ) & 0x0F; }

	p->dwFlags |= CHN_VIBRATO;
}


void CSoundFile::Panbrello( MODCHANNEL* p, UINT param )
//---------------------------------------------------
{
	if ( param & 0x0F ) { p->nPanbrelloDepth = param & 0x0F; }

	if ( param & 0xF0 ) { p->nPanbrelloSpeed = ( param >> 4 ) & 0x0F; }

	p->dwFlags |= CHN_PANBRELLO;
}


void CSoundFile::VolumeSlide( MODCHANNEL* pChn, UINT param )
//--------------------------------------------------------
{
	if ( param ) { pChn->nOldVolumeSlide = param; }
	else { param = pChn->nOldVolumeSlide; }

	LONG newvolume = pChn->nVolume;

	if ( m_nType & ( MOD_TYPE_S3M | MOD_TYPE_IT | MOD_TYPE_STM | MOD_TYPE_AMF ) )
	{
		if ( ( param & 0x0F ) == 0x0F )
		{
			if ( param & 0xF0 )
			{
				FineVolumeUp( pChn, ( param >> 4 ) );
				return;
			}
			else
			{
				if ( ( m_dwSongFlags & SONG_FIRSTTICK ) && ( !( m_dwSongFlags & SONG_FASTVOLSLIDES ) ) )
				{
					newvolume -= 0x0F * 4;
				}
			}
		}
		else if ( ( param & 0xF0 ) == 0xF0 )
		{
			if ( param & 0x0F )
			{
				FineVolumeDown( pChn, ( param & 0x0F ) );
				return;
			}
			else
			{
				if ( ( m_dwSongFlags & SONG_FIRSTTICK ) && ( !( m_dwSongFlags & SONG_FASTVOLSLIDES ) ) )
				{
					newvolume += 0x0F * 4;
				}
			}
		}
	}

	if ( ( !( m_dwSongFlags & SONG_FIRSTTICK ) ) || ( m_dwSongFlags & SONG_FASTVOLSLIDES ) )
	{
		if ( param & 0x0F ) { newvolume -= ( int )( ( param & 0x0F ) * 4 ); }
		else { newvolume += ( int )( ( param & 0xF0 ) >> 2 ); }

		if ( m_nType & MOD_TYPE_MOD ) { pChn->dwFlags |= CHN_FASTVOLRAMP; }
	}

	if ( newvolume < 0 ) { newvolume = 0; }

	if ( newvolume > 256 ) { newvolume = 256; }

	pChn->nVolume = newvolume;
}


void CSoundFile::PanningSlide( MODCHANNEL* pChn, UINT param )
//---------------------------------------------------------
{
	LONG nPanSlide = 0;

	if ( param ) { pChn->nOldPanSlide = param; }
	else { param = pChn->nOldPanSlide; }

	if ( m_nType & ( MOD_TYPE_S3M | MOD_TYPE_IT | MOD_TYPE_STM ) )
	{
		if ( ( ( param & 0x0F ) == 0x0F ) && ( param & 0xF0 ) )
		{
			if ( m_dwSongFlags & SONG_FIRSTTICK )
			{
				param = ( param & 0xF0 ) >> 2;
				nPanSlide = - ( int )param;
			}
		}
		else if ( ( ( param & 0xF0 ) == 0xF0 ) && ( param & 0x0F ) )
		{
			if ( m_dwSongFlags & SONG_FIRSTTICK )
			{
				nPanSlide = ( param & 0x0F ) << 2;
			}
		}
		else
		{
			if ( !( m_dwSongFlags & SONG_FIRSTTICK ) )
			{
				if ( param & 0x0F ) { nPanSlide = ( int )( ( param & 0x0F ) << 2 ); }
				else { nPanSlide = -( int )( ( param & 0xF0 ) >> 2 ); }
			}
		}
	}
	else
	{
		if ( !( m_dwSongFlags & SONG_FIRSTTICK ) )
		{
			if ( param & 0x0F ) { nPanSlide = -( int )( ( param & 0x0F ) << 2 ); }
			else { nPanSlide = ( int )( ( param & 0xF0 ) >> 2 ); }
		}
	}

	if ( nPanSlide )
	{
		nPanSlide += pChn->nPan;

		if ( nPanSlide < 0 ) { nPanSlide = 0; }

		if ( nPanSlide > 256 ) { nPanSlide = 256; }

		pChn->nPan = nPanSlide;
	}
}


void CSoundFile::FineVolumeUp( MODCHANNEL* pChn, UINT param )
//---------------------------------------------------------
{
	if ( param ) { pChn->nOldFineVolUpDown = param; }
	else { param = pChn->nOldFineVolUpDown; }

	if ( m_dwSongFlags & SONG_FIRSTTICK )
	{
		pChn->nVolume += param * 4;

		if ( pChn->nVolume > 256 ) { pChn->nVolume = 256; }

		if ( m_nType & MOD_TYPE_MOD ) { pChn->dwFlags |= CHN_FASTVOLRAMP; }
	}
}


void CSoundFile::FineVolumeDown( MODCHANNEL* pChn, UINT param )
//-----------------------------------------------------------
{
	if ( param ) { pChn->nOldFineVolUpDown = param; }
	else { param = pChn->nOldFineVolUpDown; }

	if ( m_dwSongFlags & SONG_FIRSTTICK )
	{
		pChn->nVolume -= param * 4;

		if ( pChn->nVolume < 0 ) { pChn->nVolume = 0; }

		if ( m_nType & MOD_TYPE_MOD ) { pChn->dwFlags |= CHN_FASTVOLRAMP; }
	}
}


void CSoundFile::Tremolo( MODCHANNEL* p, UINT param )
//-------------------------------------------------
{
	if ( param & 0x0F ) { p->nTremoloDepth = ( param & 0x0F ) << 2; }

	if ( param & 0xF0 ) { p->nTremoloSpeed = ( param >> 4 ) & 0x0F; }

	p->dwFlags |= CHN_TREMOLO;
}


void CSoundFile::ChannelVolSlide( MODCHANNEL* pChn, UINT param )
//------------------------------------------------------------
{
	LONG nChnSlide = 0;

	if ( param ) { pChn->nOldChnVolSlide = param; }
	else { param = pChn->nOldChnVolSlide; }

	if ( ( ( param & 0x0F ) == 0x0F ) && ( param & 0xF0 ) )
	{
		if ( m_dwSongFlags & SONG_FIRSTTICK ) { nChnSlide = param >> 4; }
	}
	else if ( ( ( param & 0xF0 ) == 0xF0 ) && ( param & 0x0F ) )
	{
		if ( m_dwSongFlags & SONG_FIRSTTICK ) { nChnSlide = - ( int )( param & 0x0F ); }
	}
	else
	{
		if ( !( m_dwSongFlags & SONG_FIRSTTICK ) )
		{
			if ( param & 0x0F ) { nChnSlide = -( int )( param & 0x0F ); }
			else { nChnSlide = ( int )( ( param & 0xF0 ) >> 4 ); }
		}
	}

	if ( nChnSlide )
	{
		nChnSlide += pChn->nGlobalVol;

		if ( nChnSlide < 0 ) { nChnSlide = 0; }

		if ( nChnSlide > 64 ) { nChnSlide = 64; }

		pChn->nGlobalVol = nChnSlide;
	}
}


void CSoundFile::ExtendedMODCommands( UINT nChn, UINT param )
//---------------------------------------------------------
{
	MODCHANNEL* pChn = &Chn[nChn];
	UINT command = param & 0xF0;
	param &= 0x0F;

	switch ( command )
	{
			// E0x: Set Filter
			// E1x: Fine Portamento Up
		case 0x10:
				if ( ( param ) || ( m_nType & ( MOD_TYPE_XM | MOD_TYPE_MT2 ) ) ) { FinePortamentoUp( pChn, param ); }

			break;

			// E2x: Fine Portamento Down
		case 0x20:
				if ( ( param ) || ( m_nType & ( MOD_TYPE_XM | MOD_TYPE_MT2 ) ) ) { FinePortamentoDown( pChn, param ); }

			break;

			// E3x: Set Glissando Control
		case 0x30:
				pChn->dwFlags &= ~CHN_GLISSANDO;

			if ( param ) { pChn->dwFlags |= CHN_GLISSANDO; }

			break;

			// E4x: Set Vibrato WaveForm
		case 0x40:
				pChn->nVibratoType = param & 0x07;
			break;

			// E5x: Set FineTune
		case 0x50:
				if ( m_nTickCount ) { break; }

			pChn->nC4Speed = S3MFineTuneTable[param];

			if ( m_nType & ( MOD_TYPE_XM | MOD_TYPE_MT2 ) )
			{
				pChn->nFineTune = param * 2;
			}
			else
			{
				pChn->nFineTune = MOD2XMFineTune( param );
			}

			if ( pChn->nPeriod ) { pChn->nPeriod = GetPeriodFromNote( pChn->nNote, pChn->nFineTune, pChn->nC4Speed ); }

			break;

			// E6x: Pattern Loop
			// E7x: Set Tremolo WaveForm
		case 0x70:
				pChn->nTremoloType = param & 0x07;
			break;

			// E8x: Set 4-bit Panning
		case 0x80:
				if ( !m_nTickCount ) { pChn->nPan = ( param << 4 ) + 8; pChn->dwFlags |= CHN_FASTVOLRAMP; }

			break;

			// E9x: Retrig
		case 0x90:
				RetrigNote( nChn, param );
			break;

			// EAx: Fine Volume Up
		case 0xA0:
				if ( ( param ) || ( m_nType & ( MOD_TYPE_XM | MOD_TYPE_MT2 ) ) ) { FineVolumeUp( pChn, param ); }

			break;

			// EBx: Fine Volume Down
		case 0xB0:
				if ( ( param ) || ( m_nType & ( MOD_TYPE_XM | MOD_TYPE_MT2 ) ) ) { FineVolumeDown( pChn, param ); }

			break;

			// ECx: Note Cut
		case 0xC0:
				NoteCut( nChn, param );
			break;

			// EDx: Note Delay
			// EEx: Pattern Delay
			// EFx: MOD: Invert Loop, XM: Set Active Midi Macro
		case 0xF0:
				pChn->nActiveMacro = param;
			break;
	}
}


void CSoundFile::ExtendedS3MCommands( UINT nChn, UINT param )
//---------------------------------------------------------
{
	MODCHANNEL* pChn = &Chn[nChn];
	UINT command = param & 0xF0;
	param &= 0x0F;

	switch ( command )
	{
			// S0x: Set Filter
			// S1x: Set Glissando Control
		case 0x10:
				pChn->dwFlags &= ~CHN_GLISSANDO;

			if ( param ) { pChn->dwFlags |= CHN_GLISSANDO; }

			break;

			// S2x: Set FineTune
		case 0x20:
				if ( m_nTickCount ) { break; }

			pChn->nC4Speed = S3MFineTuneTable[param & 0x0F];
			pChn->nFineTune = MOD2XMFineTune( param );

			if ( pChn->nPeriod ) { pChn->nPeriod = GetPeriodFromNote( pChn->nNote, pChn->nFineTune, pChn->nC4Speed ); }

			break;

			// S3x: Set Vibrato WaveForm
		case 0x30:
				pChn->nVibratoType = param & 0x07;
			break;

			// S4x: Set Tremolo WaveForm
		case 0x40:
				pChn->nTremoloType = param & 0x07;
			break;

			// S5x: Set Panbrello WaveForm
		case 0x50:
				pChn->nPanbrelloType = param & 0x07;
			break;

			// S6x: Pattern Delay for x frames
		case 0x60:
				m_nFrameDelay = param;
			break;

			// S7x: Envelope Control
		case 0x70:
				if ( m_nTickCount ) { break; }

			switch ( param )
			{
				case 0:
					case 1:
						case 2:
							{
							   MODCHANNEL* bkp = &Chn[m_nChannels];

							   for ( UINT i = m_nChannels; i < MAX_CHANNELS; i++, bkp++ )
						{
							if ( bkp->nMasterChn == nChn + 1 )
								{
									if ( param == 1 ) { KeyOff( i ); }
									else if ( param == 2 ) { bkp->dwFlags |= CHN_NOTEFADE; }
									else
									{ bkp->dwFlags |= CHN_NOTEFADE; bkp->nFadeOutVol = 0; }
								}
							}
			}
				break;

				case 3:
						pChn->nNNA = NNA_NOTECUT;
					break;

				case 4:
						pChn->nNNA = NNA_CONTINUE;
					break;

				case 5:
						pChn->nNNA = NNA_NOTEOFF;
					break;

				case 6:
						pChn->nNNA = NNA_NOTEFADE;
					break;

				case 7:
						pChn->dwFlags &= ~CHN_VOLENV;
					break;

				case 8:
						pChn->dwFlags |= CHN_VOLENV;
					break;

				case 9:
						pChn->dwFlags &= ~CHN_PANENV;
					break;

				case 10:
						pChn->dwFlags |= CHN_PANENV;
					break;

				case 11:
						pChn->dwFlags &= ~CHN_PITCHENV;
					break;

				case 12:
						pChn->dwFlags |= CHN_PITCHENV;
					break;
			}

			break;

			// S8x: Set 4-bit Panning
		case 0x80:
				if ( !m_nTickCount ) { pChn->nPan = ( param << 4 ) + 8; pChn->dwFlags |= CHN_FASTVOLRAMP; }

			break;

			// S9x: Set Surround
		case 0x90:
				ExtendedChannelEffect( pChn, param & 0x0F );
			break;

			// SAx: Set 64k Offset
		case 0xA0:
				if ( !m_nTickCount )
				{
					pChn->nOldHiOffset = param;

					if ( ( pChn->nRowNote ) && ( pChn->nRowNote < 0x80 ) )
					{
						DWORD pos = param << 16;

						if ( pos < pChn->nLength ) { pChn->nPos = pos; }
					}
				}

			break;

			// SBx: Pattern Loop
			// SCx: Note Cut
		case 0xC0:
				NoteCut( nChn, param );
			break;

			// SDx: Note Delay
			// case 0xD0:  break;
			// SEx: Pattern Delay for x rows
			// SFx: S3M: Funk Repeat, IT: Set Active Midi Macro
		case 0xF0:
				pChn->nActiveMacro = param;
			break;
	}
}


void CSoundFile::ExtendedChannelEffect( MODCHANNEL* pChn, UINT param )
//------------------------------------------------------------------
{
	// S9x and X9x commands (S3M/XM/IT only)
	if ( m_nTickCount ) { return; }

	switch ( param & 0x0F )
	{
			// S90: Surround Off
		case 0x00:
				pChn->dwFlags &= ~CHN_SURROUND;
			break;

			// S91: Surround On
		case 0x01:
				pChn->dwFlags |= CHN_SURROUND;
			pChn->nPan = 128;
			break;

			////////////////////////////////////////////////////////////
			// Modplug Extensions
			// S98: Reverb Off
		case 0x08:
				pChn->dwFlags &= ~CHN_REVERB;
			pChn->dwFlags |= CHN_NOREVERB;
			break;

			// S99: Reverb On
		case 0x09:
				pChn->dwFlags &= ~CHN_NOREVERB;
			pChn->dwFlags |= CHN_REVERB;
			break;

			// S9A: 2-Channels surround mode
		case 0x0A:
				m_dwSongFlags &= ~SONG_SURROUNDPAN;
			break;

			// S9B: 4-Channels surround mode
		case 0x0B:
				m_dwSongFlags |= SONG_SURROUNDPAN;
			break;

			// S9C: IT Filter Mode
		case 0x0C:
				m_dwSongFlags &= ~SONG_MPTFILTERMODE;
			break;

			// S9D: MPT Filter Mode
		case 0x0D:
				m_dwSongFlags |= SONG_MPTFILTERMODE;
			break;

			// S9E: Go forward
		case 0x0E:
				pChn->dwFlags &= ~( CHN_PINGPONGFLAG );
			break;

			// S9F: Go backward (set position at the end for non-looping samples)
		case 0x0F:
				if ( ( !( pChn->dwFlags & CHN_LOOP ) ) && ( !pChn->nPos ) && ( pChn->nLength ) )
				{
					pChn->nPos = pChn->nLength - 1;
					pChn->nPosLo = 0xFFFF;
				}

			pChn->dwFlags |= CHN_PINGPONGFLAG;
			break;
	}
}


void CSoundFile::ProcessMidiMacro( UINT nChn, LPCSTR pszMidiMacro, UINT param )
//---------------------------------------------------------------------------
{
	MODCHANNEL* pChn = &Chn[nChn];
	DWORD dwMacro = ( *( ( LPDWORD )pszMidiMacro ) ) & 0x7F5F7F5F;

	// Not Internal Device ?
	if ( dwMacro != 0x30463046 && dwMacro != 0x31463046 )
	{
		UINT pos = 0, nNib = 0, nBytes = 0;
		DWORD dwMidiCode = 0, dwByteCode = 0;

		while ( pos + 6 <= 32 )
		{
			CHAR cData = pszMidiMacro[pos++];

			if ( !cData ) { break; }

			if ( ( cData >= '0' ) && ( cData <= '9' ) ) { dwByteCode = ( dwByteCode << 4 ) | ( cData - '0' ); nNib++; }
			else if ( ( cData >= 'A' ) && ( cData <= 'F' ) ) { dwByteCode = ( dwByteCode << 4 ) | ( cData - 'A' + 10 ); nNib++; }
			else if ( ( cData >= 'a' ) && ( cData <= 'f' ) ) { dwByteCode = ( dwByteCode << 4 ) | ( cData - 'a' + 10 ); nNib++; }
			else if ( ( cData == 'z' ) || ( cData == 'Z' ) ) { dwByteCode = param & 0x7f; nNib = 2; }
			else if ( ( cData == 'x' ) || ( cData == 'X' ) ) { dwByteCode = param & 0x70; nNib = 2; }
			else if ( ( cData == 'y' ) || ( cData == 'Y' ) ) { dwByteCode = ( param & 0x0f ) << 3; nNib = 2; }
			else if ( nNib >= 2 )
			{
				nNib = 0;
				dwMidiCode |= dwByteCode << ( nBytes * 8 );
				dwByteCode = 0;
				nBytes++;

				if ( nBytes >= 3 )
				{
					UINT nMasterCh = ( nChn < m_nChannels ) ? nChn + 1 : pChn->nMasterChn;

					if ( ( nMasterCh ) && ( nMasterCh <= m_nChannels ) )
					{
						UINT nPlug = ChnSettings[nMasterCh - 1].nMixPlugin;

						if ( ( nPlug ) && ( nPlug <= MAX_MIXPLUGINS ) )
						{
							IMixPlugin* pPlugin = m_MixPlugins[nPlug - 1].pMixPlugin;

							if ( ( pPlugin ) && ( m_MixPlugins[nPlug - 1].pMixState ) )
							{
								pPlugin->MidiSend( dwMidiCode );
							}
						}
					}

					nBytes = 0;
					dwMidiCode = 0;
				}
			}

		}

		return;
	}

	// Internal device
	pszMidiMacro += 4;

	// Filter ?
	if ( pszMidiMacro[0] == '0' )
	{
		CHAR cData1 = pszMidiMacro[2];
		DWORD dwParam = 0;

		if ( ( cData1 == 'z' ) || ( cData1 == 'Z' ) )
		{
			dwParam = param;
		}
		else
		{
			CHAR cData2 = pszMidiMacro[3];

			if ( ( cData1 >= '0' ) && ( cData1 <= '9' ) ) { dwParam += ( cData1 - '0' ) << 4; }
			else if ( ( cData1 >= 'A' ) && ( cData1 <= 'F' ) ) { dwParam += ( cData1 - 'A' + 0x0A ) << 4; }

			if ( ( cData2 >= '0' ) && ( cData2 <= '9' ) ) { dwParam += ( cData2 - '0' ); }
			else if ( ( cData2 >= 'A' ) && ( cData2 <= 'F' ) ) { dwParam += ( cData2 - 'A' + 0x0A ); }
		}

		switch ( pszMidiMacro[1] )
		{
				// F0.F0.00.xx: Set CutOff
			case '0':
				{
				   int oldcutoff = pChn->nCutOff;

				if ( dwParam < 0x80 ) { pChn->nCutOff = ( BYTE )dwParam; }

#ifndef NO_FILTER
			oldcutoff -= pChn->nCutOff;

			if ( oldcutoff < 0 ) { oldcutoff = -oldcutoff; }

		if ( ( pChn->nVolume > 0 ) || ( oldcutoff < 0x10 )
			     || ( !( pChn->dwFlags & CHN_FILTER ) ) || ( !( pChn->nLeftVol | pChn->nRightVol ) ) )
		{
			SetupChannelFilter( pChn, ( pChn->dwFlags & CHN_FILTER ) ? FALSE : TRUE );
			}

#endif // NO_FILTER
		}
			break;

			// F0.F0.01.xx: Set Resonance
			case '1':
					if ( dwParam < 0x80 ) { pChn->nResonance = ( BYTE )dwParam; }

#ifndef NO_FILTER
				SetupChannelFilter( pChn, ( pChn->dwFlags & CHN_FILTER ) ? FALSE : TRUE );
#endif // NO_FILTER

				break;
		}

	}
}


void CSoundFile::RetrigNote( UINT nChn, UINT param )
//------------------------------------------------
{
	// Retrig: bit 8 is set if it's the new XM retrig
	MODCHANNEL* pChn = &Chn[nChn];
	UINT nRetrigSpeed = param & 0x0F;
	UINT nRetrigCount = pChn->nRetrigCount;
	BOOL bDoRetrig = FALSE;

	if ( m_nType & ( MOD_TYPE_S3M | MOD_TYPE_IT ) )
	{
		if ( !nRetrigSpeed ) { nRetrigSpeed = 1; }

		if ( ( nRetrigCount ) && ( !( nRetrigCount % nRetrigSpeed ) ) ) { bDoRetrig = TRUE; }

		nRetrigCount++;
	}
	else
	{
		UINT realspeed = nRetrigSpeed;

		if ( ( param & 0x100 ) && ( pChn->nRowVolCmd == VOLCMD_VOLUME ) && ( pChn->nRowParam & 0xF0 ) ) { realspeed++; }

		if ( ( m_nTickCount ) || ( param & 0x100 ) )
		{
			if ( !realspeed ) { realspeed = 1; }

			if ( ( !( param & 0x100 ) ) && ( m_nMusicSpeed ) && ( !( m_nTickCount % realspeed ) ) ) { bDoRetrig = TRUE; }

			nRetrigCount++;
		}
		else if ( m_nType & ( MOD_TYPE_XM | MOD_TYPE_MT2 ) ) { nRetrigCount = 0; }

		if ( nRetrigCount >= realspeed )
		{
			if ( ( m_nTickCount ) || ( ( param & 0x100 ) && ( !pChn->nRowNote ) ) ) { bDoRetrig = TRUE; }
		}
	}

	if ( bDoRetrig )
	{
		UINT dv = ( param >> 4 ) & 0x0F;

		if ( dv )
		{
			int vol = pChn->nVolume;

			if ( retrigTable1[dv] )
			{
				vol = ( vol * retrigTable1[dv] ) >> 4;
			}
			else
			{
				vol += ( ( int )retrigTable2[dv] ) << 2;
			}

			if ( vol < 0 ) { vol = 0; }

			if ( vol > 256 ) { vol = 256; }

			pChn->nVolume = vol;
			pChn->dwFlags |= CHN_FASTVOLRAMP;
		}

		UINT nNote = pChn->nNewNote;
		LONG nOldPeriod = pChn->nPeriod;

		if ( ( nNote ) && ( nNote <= NOTE_MAX ) && ( pChn->nLength ) ) { CheckNNA( nChn, 0, nNote, TRUE ); }

		BOOL bResetEnv = FALSE;

		if ( m_nType & ( MOD_TYPE_XM | MOD_TYPE_MT2 ) )
		{
			if ( ( pChn->nRowInstr ) && ( param < 0x100 ) ) { InstrumentChange( pChn, pChn->nRowInstr, FALSE, FALSE ); bResetEnv = TRUE; }

			if ( param < 0x100 ) { bResetEnv = TRUE; }
		}

		NoteChange( nChn, nNote, FALSE, bResetEnv );

		if ( ( m_nType & MOD_TYPE_IT ) && ( !pChn->nRowNote ) && ( nOldPeriod ) ) { pChn->nPeriod = nOldPeriod; }

		if ( !( m_nType & ( MOD_TYPE_S3M | MOD_TYPE_IT ) ) ) { nRetrigCount = 0; }
	}

	pChn->nRetrigCount = ( BYTE )nRetrigCount;
}


void CSoundFile::DoFreqSlide( MODCHANNEL* pChn, LONG nFreqSlide )
//-------------------------------------------------------------
{
	// IT Linear slides
	if ( !pChn->nPeriod ) { return; }

	if ( ( m_dwSongFlags & SONG_LINEARSLIDES ) && ( !( m_nType & ( MOD_TYPE_XM | MOD_TYPE_MT2 ) ) ) )
	{
		if ( nFreqSlide < 0 )
		{
			UINT n = ( - nFreqSlide ) >> 2;

			if ( n > 255 ) { n = 255; }

			pChn->nPeriod = _muldivr( pChn->nPeriod, LinearSlideDownTable[n], 65536 );
		}
		else
		{
			UINT n = ( nFreqSlide ) >> 2;

			if ( n > 255 ) { n = 255; }

			pChn->nPeriod = _muldivr( pChn->nPeriod, LinearSlideUpTable[n], 65536 );
		}
	}
	else
	{
		pChn->nPeriod += nFreqSlide;
	}

	if ( pChn->nPeriod < 1 )
	{
		pChn->nPeriod = 1;

		if ( m_nType & MOD_TYPE_IT )
		{
			pChn->dwFlags |= CHN_NOTEFADE;
			pChn->nFadeOutVol = 0;
		}
	}
}


void CSoundFile::NoteCut( UINT nChn, UINT nTick )
//---------------------------------------------
{
	if ( m_nTickCount == nTick )
	{
		MODCHANNEL* pChn = &Chn[nChn];
		// if (m_nInstruments) KeyOff(pChn); ?
		pChn->nVolume = 0;
		pChn->dwFlags |= CHN_FASTVOLRAMP;
	}
}


void CSoundFile::KeyOff( UINT nChn )
//--------------------------------
{
	MODCHANNEL* pChn = &Chn[nChn];
	BOOL bKeyOn = ( pChn->dwFlags & CHN_KEYOFF ) ? FALSE : TRUE;
	pChn->dwFlags |= CHN_KEYOFF;

	//if ((!pChn->pHeader) || (!(pChn->dwFlags & CHN_VOLENV)))
	if ( ( pChn->pHeader ) && ( !( pChn->dwFlags & CHN_VOLENV ) ) )
	{
		pChn->dwFlags |= CHN_NOTEFADE;
	}

	if ( !pChn->nLength ) { return; }

	if ( ( pChn->dwFlags & CHN_SUSTAINLOOP ) && ( pChn->pInstrument ) && ( bKeyOn ) )
	{
		MODINSTRUMENT* psmp = pChn->pInstrument;

		if ( psmp->uFlags & CHN_LOOP )
		{
			if ( psmp->uFlags & CHN_PINGPONGLOOP )
			{
				pChn->dwFlags |= CHN_PINGPONGLOOP;
			}
			else
			{
				pChn->dwFlags &= ~( CHN_PINGPONGLOOP | CHN_PINGPONGFLAG );
			}

			pChn->dwFlags |= CHN_LOOP;
			pChn->nLength = psmp->nLength;
			pChn->nLoopStart = psmp->nLoopStart;
			pChn->nLoopEnd = psmp->nLoopEnd;

			if ( pChn->nLength > pChn->nLoopEnd ) { pChn->nLength = pChn->nLoopEnd; }
		}
		else
		{
			pChn->dwFlags &= ~( CHN_LOOP | CHN_PINGPONGLOOP | CHN_PINGPONGFLAG );
			pChn->nLength = psmp->nLength;
		}
	}

	if ( pChn->pHeader )
	{
		INSTRUMENTHEADER* penv = pChn->pHeader;

		if ( ( ( penv->dwFlags & ENV_VOLLOOP ) || ( m_nType & ( MOD_TYPE_XM | MOD_TYPE_MT2 ) ) ) && ( penv->nFadeOut ) )
		{
			pChn->dwFlags |= CHN_NOTEFADE;
		}
	}
}


//////////////////////////////////////////////////////////
// CSoundFile: Global Effects


void CSoundFile::SetSpeed( UINT param )
//-----------------------------------
{
	UINT max = ( m_nType == MOD_TYPE_IT ) ? 256 : 128;
	// Modplug Tracker and Mod-Plugin don't do this check
#ifndef MODPLUG_TRACKER
#ifndef MODPLUG_FASTSOUNDLIB

	// Big Hack!!!
	if ( ( !param ) || ( param >= 0x80 ) || ( ( m_nType & ( MOD_TYPE_MOD | MOD_TYPE_XM | MOD_TYPE_MT2 ) ) && ( param >= 0x1E ) ) )
	{
		if ( IsSongFinished( m_nCurrentPattern, m_nRow + 1 ) )
		{
			GlobalFadeSong( 1000 );
		}
	}

#endif // MODPLUG_FASTSOUNDLIB
#endif // MODPLUG_TRACKER

	if ( ( m_nType & MOD_TYPE_S3M ) && ( param > 0x80 ) ) { param -= 0x80; }

	if ( ( param ) && ( param <= max ) ) { m_nMusicSpeed = param; }
}


void CSoundFile::SetTempo( UINT param )
//-----------------------------------
{
	if ( param < 0x20 )
	{
		// Tempo Slide
		if ( ( param & 0xF0 ) == 0x10 )
		{
			m_nMusicTempo += ( param & 0x0F ) * 2;

			if ( m_nMusicTempo > 255 ) { m_nMusicTempo = 255; }
		}
		else
		{
			m_nMusicTempo -= ( param & 0x0F ) * 2;

			if ( ( LONG )m_nMusicTempo < 32 ) { m_nMusicTempo = 32; }
		}
	}
	else
	{
		m_nMusicTempo = param;
	}
}


int CSoundFile::PatternLoop( MODCHANNEL* pChn, UINT param )
//-------------------------------------------------------
{
	if ( param )
	{
		if ( pChn->nPatternLoopCount )
		{
			pChn->nPatternLoopCount--;

			if ( !pChn->nPatternLoopCount ) { return -1; }
		}
		else
		{
			MODCHANNEL* p = Chn;

			for ( UINT i = 0; i < m_nChannels; i++, p++ ) if ( p != pChn )
				{
					// Loop already done
					if ( p->nPatternLoopCount ) { return -1; }
				}

			pChn->nPatternLoopCount = param;
		}

		return pChn->nPatternLoop;
	}
	else
	{
		pChn->nPatternLoop = m_nRow;
	}

	return -1;
}


void CSoundFile::GlobalVolSlide( UINT param )
//-----------------------------------------
{
	LONG nGlbSlide = 0;

	if ( param ) { m_nOldGlbVolSlide = param; }
	else { param = m_nOldGlbVolSlide; }

	if ( ( ( param & 0x0F ) == 0x0F ) && ( param & 0xF0 ) )
	{
		if ( m_dwSongFlags & SONG_FIRSTTICK ) { nGlbSlide = ( param >> 4 ) * 2; }
	}
	else if ( ( ( param & 0xF0 ) == 0xF0 ) && ( param & 0x0F ) )
	{
		if ( m_dwSongFlags & SONG_FIRSTTICK ) { nGlbSlide = - ( int )( ( param & 0x0F ) * 2 ); }
	}
	else
	{
		if ( !( m_dwSongFlags & SONG_FIRSTTICK ) )
		{
			if ( param & 0xF0 ) { nGlbSlide = ( int )( ( param & 0xF0 ) >> 4 ) * 2; }
			else { nGlbSlide = -( int )( ( param & 0x0F ) * 2 ); }
		}
	}

	if ( nGlbSlide )
	{
		if ( m_nType != MOD_TYPE_IT ) { nGlbSlide *= 2; }

		nGlbSlide += m_nGlobalVolume;

		if ( nGlbSlide < 0 ) { nGlbSlide = 0; }

		if ( nGlbSlide > 256 ) { nGlbSlide = 256; }

		m_nGlobalVolume = nGlbSlide;
	}
}


DWORD CSoundFile::IsSongFinished( UINT nStartOrder, UINT nStartRow ) const
//----------------------------------------------------------------------
{
	UINT nOrd;

	for ( nOrd = nStartOrder; nOrd < MAX_ORDERS; nOrd++ )
	{
		UINT nPat = Order[nOrd];

		if ( nPat != 0xFE )
		{
			MODCOMMAND* p;

			if ( nPat >= MAX_PATTERNS ) { break; }

			p = Patterns[nPat];

			if ( p )
			{
				UINT len = PatternSize[nPat] * m_nChannels;
				UINT pos = ( nOrd == nStartOrder ) ? nStartRow : 0;
				pos *= m_nChannels;

				while ( pos < len )
				{
					UINT cmd;

					if ( ( p[pos].note ) || ( p[pos].volcmd ) ) { return 0; }

					cmd = p[pos].command;

					if ( cmd == CMD_MODCMDEX )
					{
						UINT cmdex = p[pos].param & 0xF0;

						if ( ( !cmdex ) || ( cmdex == 0x60 ) || ( cmdex == 0xE0 ) || ( cmdex == 0xF0 ) ) { cmd = 0; }
					}

					if ( ( cmd ) && ( cmd != CMD_SPEED ) && ( cmd != CMD_TEMPO ) ) { return 0; }

					pos++;
				}
			}
		}
	}

	return ( nOrd < MAX_ORDERS ) ? nOrd : MAX_ORDERS - 1;
}


BOOL CSoundFile::IsValidBackwardJump( UINT nStartOrder, UINT nStartRow, UINT nJumpOrder, UINT nJumpRow ) const
//----------------------------------------------------------------------------------------------------------
{
	while ( ( nJumpOrder < MAX_PATTERNS ) && ( Order[nJumpOrder] == 0xFE ) ) { nJumpOrder++; }

	if ( ( nStartOrder >= MAX_PATTERNS ) || ( nJumpOrder >= MAX_PATTERNS ) ) { return FALSE; }

	// Treat only case with jumps in the same pattern
	if ( nJumpOrder > nStartOrder ) { return TRUE; }

	if ( ( nJumpOrder < nStartOrder ) || ( nJumpRow >= PatternSize[nStartOrder] )
	     || ( !Patterns[nStartOrder] ) || ( nStartRow >= 256 ) || ( nJumpRow >= 256 ) ) { return FALSE; }

	// See if the pattern is being played backward
	BYTE row_hist[256];
	memset( row_hist, 0, sizeof( row_hist ) );
	UINT nRows = PatternSize[nStartOrder], row = nJumpRow;

	if ( nRows > 256 ) { nRows = 256; }

	row_hist[nStartRow] = TRUE;

	while ( ( row < 256 ) && ( !row_hist[row] ) )
	{
		if ( row >= nRows ) { return TRUE; }

		row_hist[row] = TRUE;
		MODCOMMAND* p = Patterns[nStartOrder] + row * m_nChannels;
		row++;
		int breakrow = -1, posjump = 0;

		for ( UINT i = 0; i < m_nChannels; i++, p++ )
		{
			if ( p->command == CMD_POSITIONJUMP )
			{
				if ( p->param < nStartOrder ) { return FALSE; }

				if ( p->param > nStartOrder ) { return TRUE; }

				posjump = TRUE;
			}
			else if ( p->command == CMD_PATTERNBREAK )
			{
				breakrow = p->param;
			}
		}

		if ( breakrow >= 0 )
		{
			if ( !posjump ) { return TRUE; }

			row = breakrow;
		}

		if ( row >= nRows ) { return TRUE; }
	}

	return FALSE;
}


//////////////////////////////////////////////////////
// Note/Period/Frequency functions

UINT CSoundFile::GetNoteFromPeriod( UINT period ) const
//---------------------------------------------------
{
	if ( !period ) { return 0; }

	if ( m_nType & ( MOD_TYPE_MED | MOD_TYPE_MOD | MOD_TYPE_MTM | MOD_TYPE_669 | MOD_TYPE_OKT | MOD_TYPE_AMF0 ) )
	{
		period >>= 2;

		for ( UINT i = 0; i < 6 * 12; i++ )
		{
			if ( period >= ProTrackerPeriodTable[i] )
			{
				if ( ( period != ProTrackerPeriodTable[i] ) && ( i ) )
				{
					UINT p1 = ProTrackerPeriodTable[i - 1];
					UINT p2 = ProTrackerPeriodTable[i];

					if ( p1 - period < ( period - p2 ) ) { return i + 36; }
				}

				return i + 1 + 36;
			}
		}

		return 6 * 12 + 36;
	}
	else
	{
		for ( UINT i = 1; i < NOTE_MAX; i++ )
		{
			LONG n = GetPeriodFromNote( i, 0, 0 );

			if ( ( n > 0 ) && ( n <= ( LONG )period ) ) { return i; }
		}

		return NOTE_MAX;
	}
}



UINT CSoundFile::GetPeriodFromNote( UINT note, int nFineTune, UINT nC4Speed ) const
//-------------------------------------------------------------------------------
{
	if ( ( !note ) || ( note > 0xF0 ) ) { return 0; }

	if ( m_nType & ( MOD_TYPE_IT | MOD_TYPE_S3M | MOD_TYPE_STM | MOD_TYPE_MDL | MOD_TYPE_ULT | MOD_TYPE_WAV
	                 | MOD_TYPE_FAR | MOD_TYPE_DMF | MOD_TYPE_PTM | MOD_TYPE_AMS | MOD_TYPE_DBM | MOD_TYPE_AMF | MOD_TYPE_PSM ) )
	{
		note--;

		if ( m_dwSongFlags & SONG_LINEARSLIDES )
		{
			return ( FreqS3MTable[note % 12] << 5 ) >> ( note / 12 );
		}
		else
		{
			if ( !nC4Speed ) { nC4Speed = 8363; }

			return _muldiv( 8363, ( FreqS3MTable[note % 12] << 5 ), nC4Speed << ( note / 12 ) );
		}
	}
	else if ( m_nType & ( MOD_TYPE_XM | MOD_TYPE_MT2 ) )
	{
		if ( note < 13 ) { note = 13; }

		note -= 13;

		if ( m_dwSongFlags & SONG_LINEARSLIDES )
		{
			LONG l = ( ( NOTE_MAX - note ) << 6 ) - ( nFineTune / 2 );

			if ( l < 1 ) { l = 1; }

			return ( UINT )l;
		}
		else
		{
			int finetune = nFineTune;
			UINT rnote = ( note % 12 ) << 3;
			UINT roct = note / 12;
			int rfine = finetune / 16;
			int i = rnote + rfine + 8;

			if ( i < 0 ) { i = 0; }

			if ( i >= 104 ) { i = 103; }

			UINT per1 = XMPeriodTable[i];

			if ( finetune < 0 )
			{
				rfine--;
				finetune = -finetune;
			}
			else { rfine++; }

			i = rnote + rfine + 8;

			if ( i < 0 ) { i = 0; }

			if ( i >= 104 ) { i = 103; }

			UINT per2 = XMPeriodTable[i];
			rfine = finetune & 0x0F;
			per1 *= 16 - rfine;
			per2 *= rfine;
			return ( ( per1 + per2 ) << 1 ) >> roct;
		}
	}
	else
	{
		note--;
		nFineTune = XM2MODFineTune( nFineTune );

		if ( ( nFineTune ) || ( note < 36 ) || ( note >= 36 + 6 * 12 ) )
		{
			return ( ProTrackerTunedPeriods[nFineTune * 12 + note % 12] << 5 ) >> ( note / 12 );
		}
		else
		{
			return ( ProTrackerPeriodTable[note - 36] << 2 );
		}
	}
}


UINT CSoundFile::GetFreqFromPeriod( UINT period, UINT nC4Speed, int nPeriodFrac ) const
//-----------------------------------------------------------------------------------
{
	if ( !period ) { return 0; }

	if ( m_nType & ( MOD_TYPE_MED | MOD_TYPE_MOD | MOD_TYPE_MTM | MOD_TYPE_669 | MOD_TYPE_OKT | MOD_TYPE_AMF0 ) )
	{
		return ( 3546895L * 4 ) / period;
	}
	else if ( m_nType & ( MOD_TYPE_XM | MOD_TYPE_MT2 ) )
	{
		if ( m_dwSongFlags & SONG_LINEARSLIDES )
		{
			return XMLinearTable[period % 768] >> ( period / 768 );
		}
		else
		{
			return 8363 * 1712L / period;
		}
	}
	else
	{
		if ( m_dwSongFlags & SONG_LINEARSLIDES )
		{
			if ( !nC4Speed ) { nC4Speed = 8363; }

			return _muldiv( nC4Speed, 1712L << 8, ( period << 8 ) + nPeriodFrac );
		}
		else
		{
			return _muldiv( 8363, 1712L << 8, ( period << 8 ) + nPeriodFrac );
		}
	}
}

/// end of SND_FX

//////// FASTMIX

#ifdef MSC_VER
#pragma bss_seg(".modplug")
#endif

// Front Mix Buffer (Also room for interleaved rear mix)
int MixSoundBuffer[MIXBUFFERSIZE * 4];

// Reverb Mix Buffer
#ifndef MODPLUG_NO_REVERB
int MixReverbBuffer[MIXBUFFERSIZE * 2];
extern UINT gnReverbSend;
#endif

#ifndef MODPLUG_FASTSOUNDLIB
int MixRearBuffer[MIXBUFFERSIZE * 2];
float MixFloatBuffer[MIXBUFFERSIZE * 2];
#endif

#ifdef MSC_VER
#pragma bss_seg()
#endif


extern LONG gnDryROfsVol;
extern LONG gnDryLOfsVol;
extern LONG gnRvbROfsVol;
extern LONG gnRvbLOfsVol;

// 4x256 taps polyphase FIR resampling filter
extern short int gFastSinc[];
extern short int gKaiserSinc[]; // 8-taps polyphase
/*
 *-----------------------------------------------------------------------------
 cubic spline interpolation doc,
   (derived from "digital image warping", g. wolberg)

   interpolation polynomial: f(x) = A3*(x-floor(x))**3 + A2*(x-floor(x))**2 +
     A1*(x-floor(x)) + A0

   with Y = equispaced data points (dist=1), YD = first derivates of data points and IP = floor(x)
   the A[0..3] can be found by solving
     A0  = Y[IP]
     A1  = YD[IP]
     A2  = 3*(Y[IP+1]-Y[IP])-2.0*YD[IP]-YD[IP+1]
     A3  = -2.0 * (Y[IP+1]-Y[IP]) + YD[IP] - YD[IP+1]

   with the first derivates as
     YD[IP]    = 0.5 * (Y[IP+1] - Y[IP-1]);
     YD[IP+1]  = 0.5 * (Y[IP+2] - Y[IP])

   the coefs becomes
     A0 = Y[IP]
     A1 = YD[IP]
        =  0.5*(Y[IP+1] - Y[IP-1]);
     A2 =  3.0*(Y[IP+1]-Y[IP])-2.0*YD[IP]-YD[IP+1]
        =  3.0*(Y[IP+1]-Y[IP]) - 0.5*2.0*(Y[IP+1]-Y[IP-1]) - 0.5*(Y[IP+2]-Y[IP])
        =  3.0*Y[IP+1] - 3.0*Y[IP] - Y[IP+1] + Y[IP-1] - 0.5*Y[IP+2] + 0.5*Y[IP]
        = -0.5*Y[IP+2] + 2.0 * Y[IP+1] - 2.5*Y[IP] + Y[IP-1]
        = Y[IP-1] + 2 * Y[IP+1] - 0.5 * (5.0 * Y[IP] + Y[IP+2])
     A3 = -2.0*(Y[IP+1]-Y[IP]) + YD[IP] + YD[IP+1]
        = -2.0*Y[IP+1] + 2.0*Y[IP] + 0.5*(Y[IP+1]-Y[IP-1]) + 0.5*(Y[IP+2]-Y[IP])
        = -2.0*Y[IP+1] + 2.0*Y[IP] + 0.5*Y[IP+1] - 0.5*Y[IP-1] + 0.5*Y[IP+2] - 0.5*Y[IP]
        =  0.5 * Y[IP+2] - 1.5 * Y[IP+1] + 1.5 * Y[IP] - 0.5 * Y[IP-1]
   =  0.5 * (3.0 * (Y[IP] - Y[IP+1]) - Y[IP-1] + YP[IP+2])

   then interpolated data value is (horner rule)
     out = (((A3*x)+A2)*x+A1)*x+A0

   this gives parts of data points Y[IP-1] to Y[IP+2] of
     part       x**3    x**2    x**1    x**0
      Y[IP-1]    -0.5     1      -0.5    0
      Y[IP]       1.5    -2.5     0      1
      Y[IP+1]    -1.5     2       0.5    0
      Y[IP+2]     0.5    -0.5     0      0
 *---------------------------------------------------------------------------
 */
// number of bits used to scale spline coefs
#define SPLINE_QUANTBITS   14
#define SPLINE_QUANTSCALE  (1L<<SPLINE_QUANTBITS)
#define SPLINE_8SHIFT      (SPLINE_QUANTBITS-8)
#define SPLINE_16SHIFT     (SPLINE_QUANTBITS)
// forces coefsset to unity gain
#define SPLINE_CLAMPFORUNITY
// log2(number) of precalculated splines (range is [4..14])
#define SPLINE_FRACBITS 10
#define SPLINE_LUTLEN (1L<<SPLINE_FRACBITS)

class CzCUBICSPLINE
{
	public:
	CzCUBICSPLINE( );
	~CzCUBICSPLINE( );
	static signed short lut[4 * ( 1L << SPLINE_FRACBITS )];
};

signed short CzCUBICSPLINE::lut[4 * ( 1L << SPLINE_FRACBITS )];

CzCUBICSPLINE::CzCUBICSPLINE( )
{
	int _LIi;
	int _LLen      = ( 1L << SPLINE_FRACBITS );
	float _LFlen   = 1.0f / ( float )_LLen;
	float _LScale  = ( float )SPLINE_QUANTSCALE;

	for ( _LIi = 0; _LIi < _LLen; _LIi++ )
	{
		float _LCm1, _LC0, _LC1, _LC2;
		float _LX = ( ( float )_LIi ) * _LFlen;
		int _LSum, _LIdx   = _LIi << 2;
		_LCm1 = ( float )floor( 0.5 + _LScale * ( -0.5 * _LX * _LX * _LX + 1.0 * _LX * _LX - 0.5 * _LX ) );
		_LC0 = ( float )floor( 0.5 + _LScale * ( 1.5 * _LX * _LX * _LX - 2.5 * _LX * _LX + 1.0 ) );
		_LC1 = ( float )floor( 0.5 + _LScale * ( -1.5 * _LX * _LX * _LX + 2.0 * _LX * _LX + 0.5 * _LX ) );
		_LC2 = ( float )floor( 0.5 + _LScale * ( 0.5 * _LX * _LX * _LX - 0.5 * _LX * _LX ) );
		lut[_LIdx + 0] = ( signed short )( ( _LCm1 < -_LScale ) ? -_LScale : ( ( _LCm1 > _LScale ) ? _LScale : _LCm1 ) );
		lut[_LIdx + 1] = ( signed short )( ( _LC0  < -_LScale ) ? -_LScale : ( ( _LC0  > _LScale ) ? _LScale : _LC0 ) );
		lut[_LIdx + 2] = ( signed short )( ( _LC1  < -_LScale ) ? -_LScale : ( ( _LC1  > _LScale ) ? _LScale : _LC1 ) );
		lut[_LIdx + 3] = ( signed short )( ( _LC2  < -_LScale ) ? -_LScale : ( ( _LC2  > _LScale ) ? _LScale : _LC2 ) );
#ifdef SPLINE_CLAMPFORUNITY
		_LSum = lut[_LIdx + 0] + lut[_LIdx + 1] + lut[_LIdx + 2] + lut[_LIdx + 3];

		if ( _LSum != SPLINE_QUANTSCALE )
		{
			int _LMax = _LIdx;

			if ( lut[_LIdx + 1] > lut[_LMax] ) { _LMax = _LIdx + 1; }

			if ( lut[_LIdx + 2] > lut[_LMax] ) { _LMax = _LIdx + 2; }

			if ( lut[_LIdx + 3] > lut[_LMax] ) { _LMax = _LIdx + 3; }

			lut[_LMax] += ( ( signed short )SPLINE_QUANTSCALE - _LSum );
		}

#endif
	}
}

CzCUBICSPLINE::~CzCUBICSPLINE( )
{
	// nothing todo
}

CzCUBICSPLINE sspline;

/*
  ------------------------------------------------------------------------------
   fir interpolation doc,
     (derived from "an engineer's guide to fir digital filters", n.j. loy)

     calculate coefficients for ideal lowpass filter (with cutoff = fc in
   0..1 (mapped to 0..nyquist))
     c[-N..N] = (i==0) ? fc : sin(fc*pi*i)/(pi*i)

     then apply selected window to coefficients
      c[-N..N] *= w(0..N)
     with n in 2*N and w(n) being a window function (see loy)

     then calculate gain and scale filter coefs to have unity gain.
  ------------------------------------------------------------------------------
*/
// quantizer scale of window coefs
#define WFIR_QUANTBITS     15
#define WFIR_QUANTSCALE    (1L<<WFIR_QUANTBITS)
#define WFIR_8SHIFT        (WFIR_QUANTBITS-8)
#define WFIR_16BITSHIFT    (WFIR_QUANTBITS)
// log2(number)-1 of precalculated taps range is [4..12]
#define WFIR_FRACBITS      10
#define WFIR_LUTLEN        ((1L<<(WFIR_FRACBITS+1))+1)
// number of samples in window
#define WFIR_LOG2WIDTH     3
#define WFIR_WIDTH         (1L<<WFIR_LOG2WIDTH)
#define WFIR_SMPSPERWING   ((WFIR_WIDTH-1)>>1)
// cutoff (1.0 == pi/2)
#define WFIR_CUTOFF     0.90f
// wfir type
#define WFIR_HANN    0
#define WFIR_HAMMING    1
#define WFIR_BLACKMANEXACT 2
#define WFIR_BLACKMAN3T61  3
#define WFIR_BLACKMAN3T67  4
#define WFIR_BLACKMAN4T92  5
#define WFIR_BLACKMAN4T74  6
#define WFIR_KAISER4T      7
#define WFIR_TYPE    WFIR_BLACKMANEXACT
// wfir help
#ifndef M_zPI
#define M_zPI     3.1415926535897932384626433832795
#endif
#define M_zEPS    1e-8
#define M_zBESSELEPS 1e-21

class CzWINDOWEDFIR
{
	public:
	CzWINDOWEDFIR( );
	~CzWINDOWEDFIR( );
	float coef( int _PCnr, float _POfs, float _PCut, int _PWidth, int _PType )
//OLD args to coef: float _PPos, float _PFc, int _PLen )
	{
		double   _LWidthM1       = _PWidth - 1;
		double   _LWidthM1Half   = 0.5 * _LWidthM1;
		double   _LPosU          = ( ( double )_PCnr - _POfs );
		double   _LPos           = _LPosU - _LWidthM1Half;
		double   _LPIdl          = 2.0 * M_zPI / _LWidthM1;
		double   _LWc, _LSi;

		if ( fabs( _LPos ) < M_zEPS )
		{
			_LWc  = 1.0;
			_LSi  = _PCut;
		}
		else
		{
			switch ( _PType )
			{
				case WFIR_HANN:
						_LWc = 0.50 - 0.50 * cos( _LPIdl * _LPosU );
					break;

				case WFIR_HAMMING:
						_LWc = 0.54 - 0.46 * cos( _LPIdl * _LPosU );
					break;

				case WFIR_BLACKMANEXACT:
						_LWc = 0.42 - 0.50 * cos( _LPIdl * _LPosU ) +
						       0.08 * cos( 2.0 * _LPIdl * _LPosU );
					break;

				case WFIR_BLACKMAN3T61:
						_LWc = 0.44959 - 0.49364 * cos( _LPIdl * _LPosU ) +
						       0.05677 * cos( 2.0 * _LPIdl * _LPosU );
					break;

				case WFIR_BLACKMAN3T67:
						_LWc = 0.42323 - 0.49755 * cos( _LPIdl * _LPosU ) +
						       0.07922 * cos( 2.0 * _LPIdl * _LPosU );
					break;

				case WFIR_BLACKMAN4T92:
						_LWc = 0.35875 - 0.48829 * cos( _LPIdl * _LPosU ) +
						       0.14128 * cos( 2.0 * _LPIdl * _LPosU ) -
						       0.01168 * cos( 3.0 * _LPIdl * _LPosU );
					break;

				case WFIR_BLACKMAN4T74:
						_LWc = 0.40217 - 0.49703 * cos( _LPIdl * _LPosU ) +
						       0.09392 * cos( 2.0 * _LPIdl * _LPosU ) -
						       0.00183 * cos( 3.0 * _LPIdl * _LPosU );
					break;

				case WFIR_KAISER4T:
						_LWc = 0.40243 - 0.49804 * cos( _LPIdl * _LPosU ) +
						       0.09831 * cos( 2.0 * _LPIdl * _LPosU ) -
						       0.00122 * cos( 3.0 * _LPIdl * _LPosU );
					break;

				default:
						_LWc = 1.0;
					break;
			}

			_LPos  *= M_zPI;
			_LSi   = sin( _PCut * _LPos ) / _LPos;
		}

		return ( float )( _LWc * _LSi );
	}
	static signed short lut[WFIR_LUTLEN* WFIR_WIDTH];
};

signed short CzWINDOWEDFIR::lut[WFIR_LUTLEN* WFIR_WIDTH];

CzWINDOWEDFIR::CzWINDOWEDFIR()
{
	int _LPcl;
	float _LPcllen = ( float )( 1L << WFIR_FRACBITS ); // number of precalculated lines for 0..1 (-1..0)
	float _LNorm   = 1.0f / ( float )( 2.0f * _LPcllen );
	float _LCut    = WFIR_CUTOFF;
	float _LScale  = ( float )WFIR_QUANTSCALE;

	for ( _LPcl = 0; _LPcl < WFIR_LUTLEN; _LPcl++ )
	{
		float _LGain, _LCoefs[WFIR_WIDTH];
		float _LOfs    = ( ( float )_LPcl - _LPcllen ) * _LNorm;
		int _LCc, _LIdx = _LPcl << WFIR_LOG2WIDTH;

		for ( _LCc = 0, _LGain = 0.0f; _LCc < WFIR_WIDTH; _LCc++ )
		{
			_LGain   += ( _LCoefs[_LCc] = coef( _LCc, _LOfs, _LCut, WFIR_WIDTH, WFIR_TYPE ) );
		}

		_LGain = 1.0f / _LGain;

		for ( _LCc = 0; _LCc < WFIR_WIDTH; _LCc++ )
		{
			float _LCoef = ( float )floor( 0.5 + _LScale * _LCoefs[_LCc] * _LGain );
			lut[_LIdx + _LCc] = ( signed short )( ( _LCoef < -_LScale ) ? -_LScale : ( ( _LCoef > _LScale ) ? _LScale : _LCoef ) );
		}
	}
}

CzWINDOWEDFIR::~CzWINDOWEDFIR()
{
	// nothing todo
}

CzWINDOWEDFIR sfir;

// ----------------------------------------------------------------------------
// MIXING MACROS
// ----------------------------------------------------------------------------
#define SNDMIX_BEGINSAMPLELOOP8\
   register MODCHANNEL * const pChn = pChannel;\
   nPos = pChn->nPosLo;\
   const signed char *p = (signed char *)(pChn->pCurrentSample+pChn->nPos);\
   if (pChn->dwFlags & CHN_STEREO) p += pChn->nPos;\
   int *pvol = pbuffer;\
   do {

#define SNDMIX_BEGINSAMPLELOOP16\
   register MODCHANNEL * const pChn = pChannel;\
   nPos = pChn->nPosLo;\
   const signed short *p = (signed short *)(pChn->pCurrentSample+(pChn->nPos*2));\
   if (pChn->dwFlags & CHN_STEREO) p += pChn->nPos;\
   int *pvol = pbuffer;\
   do {

#define SNDMIX_ENDSAMPLELOOP\
      nPos += pChn->nInc;\
   } while (pvol < pbufmax);\
   pChn->nPos += nPos >> 16;\
   pChn->nPosLo = nPos & 0xFFFF;

#define SNDMIX_ENDSAMPLELOOP8 SNDMIX_ENDSAMPLELOOP
#define SNDMIX_ENDSAMPLELOOP16   SNDMIX_ENDSAMPLELOOP

//////////////////////////////////////////////////////////////////////////////
// Mono

// No interpolation
#define SNDMIX_GETMONOVOL8NOIDO\
   int vol = p[nPos >> 16] << 8;

#define SNDMIX_GETMONOVOL16NOIDO\
   int vol = p[nPos >> 16];

// Linear Interpolation
#define SNDMIX_GETMONOVOL8LINEAR\
   int poshi = nPos >> 16;\
   int poslo = (nPos >> 8) & 0xFF;\
   int srcvol = p[poshi];\
   int destvol = p[poshi+1];\
   int vol = (srcvol<<8) + ((int)(poslo * (destvol - srcvol)));

#define SNDMIX_GETMONOVOL16LINEAR\
   int poshi = nPos >> 16;\
   int poslo = (nPos >> 8) & 0xFF;\
   int srcvol = p[poshi];\
   int destvol = p[poshi+1];\
   int vol = srcvol + ((int)(poslo * (destvol - srcvol)) >> 8);

// spline interpolation (2 guard bits should be enough???)
#define SPLINE_FRACSHIFT ((16-SPLINE_FRACBITS)-2)
#define SPLINE_FRACMASK  (((1L<<(16-SPLINE_FRACSHIFT))-1)&~3)

#define SNDMIX_GETMONOVOL8SPLINE \
   int poshi   = nPos >> 16; \
   int poslo   = (nPos >> SPLINE_FRACSHIFT) & SPLINE_FRACMASK; \
   int vol     = (CzCUBICSPLINE::lut[poslo  ]*(int)p[poshi-1] + \
                  CzCUBICSPLINE::lut[poslo+1]*(int)p[poshi  ] + \
                  CzCUBICSPLINE::lut[poslo+3]*(int)p[poshi+2] + \
                  CzCUBICSPLINE::lut[poslo+2]*(int)p[poshi+1]) >> SPLINE_8SHIFT;

#define SNDMIX_GETMONOVOL16SPLINE \
   int poshi   = nPos >> 16; \
   int poslo   = (nPos >> SPLINE_FRACSHIFT) & SPLINE_FRACMASK; \
   int vol     = (CzCUBICSPLINE::lut[poslo  ]*(int)p[poshi-1] + \
                  CzCUBICSPLINE::lut[poslo+1]*(int)p[poshi  ] + \
                  CzCUBICSPLINE::lut[poslo+3]*(int)p[poshi+2] + \
                  CzCUBICSPLINE::lut[poslo+2]*(int)p[poshi+1]) >> SPLINE_16SHIFT;


// fir interpolation
#define WFIR_FRACSHIFT  (16-(WFIR_FRACBITS+1+WFIR_LOG2WIDTH))
#define WFIR_FRACMASK   ((((1L<<(17-WFIR_FRACSHIFT))-1)&~((1L<<WFIR_LOG2WIDTH)-1)))
#define WFIR_FRACHALVE  (1L<<(16-(WFIR_FRACBITS+2)))

#define SNDMIX_GETMONOVOL8FIRFILTER \
   int poshi  = nPos >> 16;\
   int poslo  = (nPos & 0xFFFF);\
   int firidx = ((poslo+WFIR_FRACHALVE)>>WFIR_FRACSHIFT) & WFIR_FRACMASK; \
   int vol    = (CzWINDOWEDFIR::lut[firidx+0]*(int)p[poshi+1-4]); \
            vol   += (CzWINDOWEDFIR::lut[firidx+1]*(int)p[poshi+2-4]);  \
            vol   += (CzWINDOWEDFIR::lut[firidx+2]*(int)p[poshi+3-4]);  \
            vol   += (CzWINDOWEDFIR::lut[firidx+3]*(int)p[poshi+4-4]);  \
            vol   += (CzWINDOWEDFIR::lut[firidx+4]*(int)p[poshi+5-4]);  \
            vol   += (CzWINDOWEDFIR::lut[firidx+5]*(int)p[poshi+6-4]);  \
            vol   += (CzWINDOWEDFIR::lut[firidx+6]*(int)p[poshi+7-4]);  \
            vol   += (CzWINDOWEDFIR::lut[firidx+7]*(int)p[poshi+8-4]);  \
            vol  >>= WFIR_8SHIFT;

#define SNDMIX_GETMONOVOL16FIRFILTER \
    int poshi  = nPos >> 16;\
    int poslo  = (nPos & 0xFFFF);\
    int firidx = ((poslo+WFIR_FRACHALVE)>>WFIR_FRACSHIFT) & WFIR_FRACMASK; \
    int vol1   = (CzWINDOWEDFIR::lut[firidx+0]*(int)p[poshi+1-4]);   \
        vol1  += (CzWINDOWEDFIR::lut[firidx+1]*(int)p[poshi+2-4]);   \
        vol1  += (CzWINDOWEDFIR::lut[firidx+2]*(int)p[poshi+3-4]);   \
        vol1  += (CzWINDOWEDFIR::lut[firidx+3]*(int)p[poshi+4-4]);   \
    int vol2   = (CzWINDOWEDFIR::lut[firidx+4]*(int)p[poshi+5-4]);   \
   vol2  += (CzWINDOWEDFIR::lut[firidx+5]*(int)p[poshi+6-4]);  \
   vol2  += (CzWINDOWEDFIR::lut[firidx+6]*(int)p[poshi+7-4]);  \
   vol2  += (CzWINDOWEDFIR::lut[firidx+7]*(int)p[poshi+8-4]);  \
    int vol    = ((vol1>>1)+(vol2>>1)) >> (WFIR_16BITSHIFT-1);

/////////////////////////////////////////////////////////////////////////////
// Stereo

// No interpolation
#define SNDMIX_GETSTEREOVOL8NOIDO\
    int vol_l = p[(nPos>>16)*2] << 8;\
    int vol_r = p[(nPos>>16)*2+1] << 8;

#define SNDMIX_GETSTEREOVOL16NOIDO\
    int vol_l = p[(nPos>>16)*2];\
    int vol_r = p[(nPos>>16)*2+1];

// Linear Interpolation
#define SNDMIX_GETSTEREOVOL8LINEAR\
    int poshi = nPos >> 16;\
    int poslo = (nPos >> 8) & 0xFF;\
    int srcvol_l = p[poshi*2];\
    int vol_l = (srcvol_l<<8) + ((int)(poslo * (p[poshi*2+2] - srcvol_l)));\
    int srcvol_r = p[poshi*2+1];\
    int vol_r = (srcvol_r<<8) + ((int)(poslo * (p[poshi*2+3] - srcvol_r)));

#define SNDMIX_GETSTEREOVOL16LINEAR\
    int poshi = nPos >> 16;\
    int poslo = (nPos >> 8) & 0xFF;\
    int srcvol_l = p[poshi*2];\
    int vol_l = srcvol_l + ((int)(poslo * (p[poshi*2+2] - srcvol_l)) >> 8);\
    int srcvol_r = p[poshi*2+1];\
    int vol_r = srcvol_r + ((int)(poslo * (p[poshi*2+3] - srcvol_r)) >> 8);\
 
// Spline Interpolation
#define SNDMIX_GETSTEREOVOL8SPLINE \
    int poshi  = nPos >> 16; \
    int poslo  = (nPos >> SPLINE_FRACSHIFT) & SPLINE_FRACMASK; \
    int vol_l  = (CzCUBICSPLINE::lut[poslo  ]*(int)p[(poshi-1)*2  ] + \
              CzCUBICSPLINE::lut[poslo+1]*(int)p[(poshi  )*2  ] + \
              CzCUBICSPLINE::lut[poslo+2]*(int)p[(poshi+1)*2  ] + \
              CzCUBICSPLINE::lut[poslo+3]*(int)p[(poshi+2)*2  ]) >> SPLINE_8SHIFT; \
    int vol_r  = (CzCUBICSPLINE::lut[poslo  ]*(int)p[(poshi-1)*2+1] + \
              CzCUBICSPLINE::lut[poslo+1]*(int)p[(poshi  )*2+1] + \
              CzCUBICSPLINE::lut[poslo+2]*(int)p[(poshi+1)*2+1] + \
              CzCUBICSPLINE::lut[poslo+3]*(int)p[(poshi+2)*2+1]) >> SPLINE_8SHIFT;

#define SNDMIX_GETSTEREOVOL16SPLINE \
    int poshi  = nPos >> 16; \
    int poslo  = (nPos >> SPLINE_FRACSHIFT) & SPLINE_FRACMASK; \
    int vol_l  = (CzCUBICSPLINE::lut[poslo  ]*(int)p[(poshi-1)*2  ] + \
              CzCUBICSPLINE::lut[poslo+1]*(int)p[(poshi  )*2  ] + \
              CzCUBICSPLINE::lut[poslo+2]*(int)p[(poshi+1)*2  ] + \
              CzCUBICSPLINE::lut[poslo+3]*(int)p[(poshi+2)*2  ]) >> SPLINE_16SHIFT; \
    int vol_r  = (CzCUBICSPLINE::lut[poslo  ]*(int)p[(poshi-1)*2+1] + \
              CzCUBICSPLINE::lut[poslo+1]*(int)p[(poshi  )*2+1] + \
              CzCUBICSPLINE::lut[poslo+2]*(int)p[(poshi+1)*2+1] + \
              CzCUBICSPLINE::lut[poslo+3]*(int)p[(poshi+2)*2+1]) >> SPLINE_16SHIFT;

// fir interpolation
#define SNDMIX_GETSTEREOVOL8FIRFILTER \
    int poshi   = nPos >> 16;\
    int poslo   = (nPos & 0xFFFF);\
    int firidx  = ((poslo+WFIR_FRACHALVE)>>WFIR_FRACSHIFT) & WFIR_FRACMASK; \
    int vol_l   = (CzWINDOWEDFIR::lut[firidx+0]*(int)p[(poshi+1-4)*2  ]);   \
   vol_l  += (CzWINDOWEDFIR::lut[firidx+1]*(int)p[(poshi+2-4)*2  ]);   \
   vol_l  += (CzWINDOWEDFIR::lut[firidx+2]*(int)p[(poshi+3-4)*2  ]);   \
        vol_l  += (CzWINDOWEDFIR::lut[firidx+3]*(int)p[(poshi+4-4)*2  ]);   \
        vol_l  += (CzWINDOWEDFIR::lut[firidx+4]*(int)p[(poshi+5-4)*2  ]);   \
   vol_l  += (CzWINDOWEDFIR::lut[firidx+5]*(int)p[(poshi+6-4)*2  ]);   \
   vol_l  += (CzWINDOWEDFIR::lut[firidx+6]*(int)p[(poshi+7-4)*2  ]);   \
        vol_l  += (CzWINDOWEDFIR::lut[firidx+7]*(int)p[(poshi+8-4)*2  ]);   \
   vol_l >>= WFIR_8SHIFT; \
    int vol_r   = (CzWINDOWEDFIR::lut[firidx+0]*(int)p[(poshi+1-4)*2+1]);   \
   vol_r  += (CzWINDOWEDFIR::lut[firidx+1]*(int)p[(poshi+2-4)*2+1]);   \
   vol_r  += (CzWINDOWEDFIR::lut[firidx+2]*(int)p[(poshi+3-4)*2+1]);   \
   vol_r  += (CzWINDOWEDFIR::lut[firidx+3]*(int)p[(poshi+4-4)*2+1]);   \
   vol_r  += (CzWINDOWEDFIR::lut[firidx+4]*(int)p[(poshi+5-4)*2+1]);   \
        vol_r  += (CzWINDOWEDFIR::lut[firidx+5]*(int)p[(poshi+6-4)*2+1]);   \
        vol_r  += (CzWINDOWEDFIR::lut[firidx+6]*(int)p[(poshi+7-4)*2+1]);   \
        vol_r  += (CzWINDOWEDFIR::lut[firidx+7]*(int)p[(poshi+8-4)*2+1]);   \
        vol_r >>= WFIR_8SHIFT;

#define SNDMIX_GETSTEREOVOL16FIRFILTER \
    int poshi   = nPos >> 16;\
    int poslo   = (nPos & 0xFFFF);\
    int firidx  = ((poslo+WFIR_FRACHALVE)>>WFIR_FRACSHIFT) & WFIR_FRACMASK; \
    int vol1_l  = (CzWINDOWEDFIR::lut[firidx+0]*(int)p[(poshi+1-4)*2  ]);   \
   vol1_l += (CzWINDOWEDFIR::lut[firidx+1]*(int)p[(poshi+2-4)*2  ]);   \
        vol1_l += (CzWINDOWEDFIR::lut[firidx+2]*(int)p[(poshi+3-4)*2  ]);   \
   vol1_l += (CzWINDOWEDFIR::lut[firidx+3]*(int)p[(poshi+4-4)*2  ]);   \
   int vol2_l  = (CzWINDOWEDFIR::lut[firidx+4]*(int)p[(poshi+5-4)*2  ]);    \
       vol2_l += (CzWINDOWEDFIR::lut[firidx+5]*(int)p[(poshi+6-4)*2  ]);    \
       vol2_l += (CzWINDOWEDFIR::lut[firidx+6]*(int)p[(poshi+7-4)*2  ]);    \
       vol2_l += (CzWINDOWEDFIR::lut[firidx+7]*(int)p[(poshi+8-4)*2  ]);    \
   int vol_l   = ((vol1_l>>1)+(vol2_l>>1)) >> (WFIR_16BITSHIFT-1); \
   int vol1_r  = (CzWINDOWEDFIR::lut[firidx+0]*(int)p[(poshi+1-4)*2+1]);    \
       vol1_r += (CzWINDOWEDFIR::lut[firidx+1]*(int)p[(poshi+2-4)*2+1]);    \
       vol1_r += (CzWINDOWEDFIR::lut[firidx+2]*(int)p[(poshi+3-4)*2+1]);    \
       vol1_r += (CzWINDOWEDFIR::lut[firidx+3]*(int)p[(poshi+4-4)*2+1]);    \
   int vol2_r  = (CzWINDOWEDFIR::lut[firidx+4]*(int)p[(poshi+5-4)*2+1]);    \
       vol2_r += (CzWINDOWEDFIR::lut[firidx+5]*(int)p[(poshi+6-4)*2+1]);    \
       vol2_r += (CzWINDOWEDFIR::lut[firidx+6]*(int)p[(poshi+7-4)*2+1]);    \
       vol2_r += (CzWINDOWEDFIR::lut[firidx+7]*(int)p[(poshi+8-4)*2+1]);    \
   int vol_r   = ((vol1_r>>1)+(vol2_r>>1)) >> (WFIR_16BITSHIFT-1);

/////////////////////////////////////////////////////////////////////////////

#define SNDMIX_STOREMONOVOL\
   pvol[0] += vol * pChn->nRightVol;\
   pvol[1] += vol * pChn->nLeftVol;\
   pvol += 2;

#define SNDMIX_STORESTEREOVOL\
   pvol[0] += vol_l * pChn->nRightVol;\
   pvol[1] += vol_r * pChn->nLeftVol;\
   pvol += 2;

#define SNDMIX_STOREFASTMONOVOL\
   int v = vol * pChn->nRightVol;\
   pvol[0] += v;\
   pvol[1] += v;\
   pvol += 2;

#define SNDMIX_RAMPMONOVOL\
   nRampLeftVol += pChn->nLeftRamp;\
   nRampRightVol += pChn->nRightRamp;\
   pvol[0] += vol * (nRampRightVol >> VOLUMERAMPPRECISION);\
   pvol[1] += vol * (nRampLeftVol >> VOLUMERAMPPRECISION);\
   pvol += 2;

#define SNDMIX_RAMPFASTMONOVOL\
   nRampRightVol += pChn->nRightRamp;\
   int fastvol = vol * (nRampRightVol >> VOLUMERAMPPRECISION);\
   pvol[0] += fastvol;\
   pvol[1] += fastvol;\
   pvol += 2;

#define SNDMIX_RAMPSTEREOVOL\
   nRampLeftVol += pChn->nLeftRamp;\
   nRampRightVol += pChn->nRightRamp;\
   pvol[0] += vol_l * (nRampRightVol >> VOLUMERAMPPRECISION);\
   pvol[1] += vol_r * (nRampLeftVol >> VOLUMERAMPPRECISION);\
   pvol += 2;


///////////////////////////////////////////////////
// Resonant Filters

// Mono
#define MIX_BEGIN_FILTER\
   int fy1 = pChannel->nFilter_Y1;\
   int fy2 = pChannel->nFilter_Y2;\
 
#define MIX_END_FILTER\
   pChannel->nFilter_Y1 = fy1;\
   pChannel->nFilter_Y2 = fy2;

#define SNDMIX_PROCESSFILTER\
   vol = (vol * pChn->nFilter_A0 + fy1 * pChn->nFilter_B0 + fy2 * pChn->nFilter_B1 + 4096) >> 13;\
   fy2 = fy1;\
   fy1 = vol;\
 
// Stereo
#define MIX_BEGIN_STEREO_FILTER\
   int fy1 = pChannel->nFilter_Y1;\
   int fy2 = pChannel->nFilter_Y2;\
   int fy3 = pChannel->nFilter_Y3;\
   int fy4 = pChannel->nFilter_Y4;\
 
#define MIX_END_STEREO_FILTER\
   pChannel->nFilter_Y1 = fy1;\
   pChannel->nFilter_Y2 = fy2;\
   pChannel->nFilter_Y3 = fy3;\
   pChannel->nFilter_Y4 = fy4;\
 
#define SNDMIX_PROCESSSTEREOFILTER\
   vol_l = (vol_l * pChn->nFilter_A0 + fy1 * pChn->nFilter_B0 + fy2 * pChn->nFilter_B1 + 4096) >> 13;\
   vol_r = (vol_r * pChn->nFilter_A0 + fy3 * pChn->nFilter_B0 + fy4 * pChn->nFilter_B1 + 4096) >> 13;\
   fy2 = fy1; fy1 = vol_l;\
   fy4 = fy3; fy3 = vol_r;\
 
//////////////////////////////////////////////////////////
// Interfaces

typedef VOID ( MPPASMCALL* LPMIXINTERFACE )( MODCHANNEL*, int*, int* );

#define BEGIN_MIX_INTERFACE(func)\
   VOID MPPASMCALL func(MODCHANNEL *pChannel, int *pbuffer, int *pbufmax)\
   {\
      LONG nPos;

#define END_MIX_INTERFACE()\
      SNDMIX_ENDSAMPLELOOP\
   }

// Volume Ramps
#define BEGIN_RAMPMIX_INTERFACE(func)\
   BEGIN_MIX_INTERFACE(func)\
      LONG nRampRightVol = pChannel->nRampRightVol;\
      LONG nRampLeftVol = pChannel->nRampLeftVol;

#define END_RAMPMIX_INTERFACE()\
      SNDMIX_ENDSAMPLELOOP\
      pChannel->nRampRightVol = nRampRightVol;\
      pChannel->nRightVol = nRampRightVol >> VOLUMERAMPPRECISION;\
      pChannel->nRampLeftVol = nRampLeftVol;\
      pChannel->nLeftVol = nRampLeftVol >> VOLUMERAMPPRECISION;\
   }

#define BEGIN_FASTRAMPMIX_INTERFACE(func)\
   BEGIN_MIX_INTERFACE(func)\
      LONG nRampRightVol = pChannel->nRampRightVol;

#define END_FASTRAMPMIX_INTERFACE()\
      SNDMIX_ENDSAMPLELOOP\
      pChannel->nRampRightVol = nRampRightVol;\
      pChannel->nRampLeftVol = nRampRightVol;\
      pChannel->nRightVol = nRampRightVol >> VOLUMERAMPPRECISION;\
      pChannel->nLeftVol = pChannel->nRightVol;\
   }


// Mono Resonant Filters
#define BEGIN_MIX_FLT_INTERFACE(func)\
   BEGIN_MIX_INTERFACE(func)\
   MIX_BEGIN_FILTER


#define END_MIX_FLT_INTERFACE()\
   SNDMIX_ENDSAMPLELOOP\
   MIX_END_FILTER\
   }

#define BEGIN_RAMPMIX_FLT_INTERFACE(func)\
   BEGIN_MIX_INTERFACE(func)\
      LONG nRampRightVol = pChannel->nRampRightVol;\
      LONG nRampLeftVol = pChannel->nRampLeftVol;\
      MIX_BEGIN_FILTER

#define END_RAMPMIX_FLT_INTERFACE()\
      SNDMIX_ENDSAMPLELOOP\
      MIX_END_FILTER\
      pChannel->nRampRightVol = nRampRightVol;\
      pChannel->nRightVol = nRampRightVol >> VOLUMERAMPPRECISION;\
      pChannel->nRampLeftVol = nRampLeftVol;\
      pChannel->nLeftVol = nRampLeftVol >> VOLUMERAMPPRECISION;\
   }

// Stereo Resonant Filters
#define BEGIN_MIX_STFLT_INTERFACE(func)\
   BEGIN_MIX_INTERFACE(func)\
   MIX_BEGIN_STEREO_FILTER


#define END_MIX_STFLT_INTERFACE()\
   SNDMIX_ENDSAMPLELOOP\
   MIX_END_STEREO_FILTER\
   }

#define BEGIN_RAMPMIX_STFLT_INTERFACE(func)\
   BEGIN_MIX_INTERFACE(func)\
      LONG nRampRightVol = pChannel->nRampRightVol;\
      LONG nRampLeftVol = pChannel->nRampLeftVol;\
      MIX_BEGIN_STEREO_FILTER

#define END_RAMPMIX_STFLT_INTERFACE()\
      SNDMIX_ENDSAMPLELOOP\
      MIX_END_STEREO_FILTER\
      pChannel->nRampRightVol = nRampRightVol;\
      pChannel->nRightVol = nRampRightVol >> VOLUMERAMPPRECISION;\
      pChannel->nRampLeftVol = nRampLeftVol;\
      pChannel->nLeftVol = nRampLeftVol >> VOLUMERAMPPRECISION;\
   }


/////////////////////////////////////////////////////
//

void MPPASMCALL X86_InitMixBuffer( int* pBuffer, UINT nSamples );
void MPPASMCALL X86_EndChannelOfs( MODCHANNEL* pChannel, int* pBuffer, UINT nSamples );
void MPPASMCALL X86_StereoFill( int* pBuffer, UINT nSamples, LPLONG lpROfs, LPLONG lpLOfs );
void X86_StereoMixToFloat( const int*, float*, float*, UINT nCount );
void X86_FloatToStereoMix( const float* pIn1, const float* pIn2, int* pOut, UINT nCount );

/////////////////////////////////////////////////////
// Mono samples functions

BEGIN_MIX_INTERFACE( Mono8BitMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETMONOVOL8NOIDO
SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE( Mono16BitMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETMONOVOL16NOIDO
SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE( Mono8BitLinearMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETMONOVOL8LINEAR
SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE( Mono16BitLinearMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETMONOVOL16LINEAR
SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE( Mono8BitSplineMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETMONOVOL8SPLINE
SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE( Mono16BitSplineMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETMONOVOL16SPLINE
SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE( Mono8BitFirFilterMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETMONOVOL8FIRFILTER
SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE( Mono16BitFirFilterMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETMONOVOL16FIRFILTER
SNDMIX_STOREMONOVOL
END_MIX_INTERFACE()


// Volume Ramps
BEGIN_RAMPMIX_INTERFACE( Mono8BitRampMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETMONOVOL8NOIDO
SNDMIX_RAMPMONOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE( Mono16BitRampMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETMONOVOL16NOIDO
SNDMIX_RAMPMONOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE( Mono8BitLinearRampMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETMONOVOL8LINEAR
SNDMIX_RAMPMONOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE( Mono16BitLinearRampMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETMONOVOL16LINEAR
SNDMIX_RAMPMONOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE( Mono8BitSplineRampMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETMONOVOL8SPLINE
SNDMIX_RAMPMONOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE( Mono16BitSplineRampMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETMONOVOL16SPLINE
SNDMIX_RAMPMONOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE( Mono8BitFirFilterRampMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETMONOVOL8FIRFILTER
SNDMIX_RAMPMONOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE( Mono16BitFirFilterRampMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETMONOVOL16FIRFILTER
SNDMIX_RAMPMONOVOL
END_RAMPMIX_INTERFACE()


//////////////////////////////////////////////////////
// Fast mono mix for leftvol=rightvol (1 less imul)

BEGIN_MIX_INTERFACE( FastMono8BitMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETMONOVOL8NOIDO
SNDMIX_STOREFASTMONOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE( FastMono16BitMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETMONOVOL16NOIDO
SNDMIX_STOREFASTMONOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE( FastMono8BitLinearMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETMONOVOL8LINEAR
SNDMIX_STOREFASTMONOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE( FastMono16BitLinearMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETMONOVOL16LINEAR
SNDMIX_STOREFASTMONOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE( FastMono8BitSplineMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETMONOVOL8SPLINE
SNDMIX_STOREFASTMONOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE( FastMono16BitSplineMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETMONOVOL16SPLINE
SNDMIX_STOREFASTMONOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE( FastMono8BitFirFilterMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETMONOVOL8FIRFILTER
SNDMIX_STOREFASTMONOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE( FastMono16BitFirFilterMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETMONOVOL16FIRFILTER
SNDMIX_STOREFASTMONOVOL
END_MIX_INTERFACE()


// Fast Ramps
BEGIN_FASTRAMPMIX_INTERFACE( FastMono8BitRampMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETMONOVOL8NOIDO
SNDMIX_RAMPFASTMONOVOL
END_FASTRAMPMIX_INTERFACE()

BEGIN_FASTRAMPMIX_INTERFACE( FastMono16BitRampMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETMONOVOL16NOIDO
SNDMIX_RAMPFASTMONOVOL
END_FASTRAMPMIX_INTERFACE()

BEGIN_FASTRAMPMIX_INTERFACE( FastMono8BitLinearRampMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETMONOVOL8LINEAR
SNDMIX_RAMPFASTMONOVOL
END_FASTRAMPMIX_INTERFACE()

BEGIN_FASTRAMPMIX_INTERFACE( FastMono16BitLinearRampMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETMONOVOL16LINEAR
SNDMIX_RAMPFASTMONOVOL
END_FASTRAMPMIX_INTERFACE()

BEGIN_FASTRAMPMIX_INTERFACE( FastMono8BitSplineRampMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETMONOVOL8SPLINE
SNDMIX_RAMPFASTMONOVOL
END_FASTRAMPMIX_INTERFACE()

BEGIN_FASTRAMPMIX_INTERFACE( FastMono16BitSplineRampMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETMONOVOL16SPLINE
SNDMIX_RAMPFASTMONOVOL
END_FASTRAMPMIX_INTERFACE()

BEGIN_FASTRAMPMIX_INTERFACE( FastMono8BitFirFilterRampMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETMONOVOL8FIRFILTER
SNDMIX_RAMPFASTMONOVOL
END_FASTRAMPMIX_INTERFACE()

BEGIN_FASTRAMPMIX_INTERFACE( FastMono16BitFirFilterRampMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETMONOVOL16FIRFILTER
SNDMIX_RAMPFASTMONOVOL
END_FASTRAMPMIX_INTERFACE()


//////////////////////////////////////////////////////
// Stereo samples

BEGIN_MIX_INTERFACE( Stereo8BitMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETSTEREOVOL8NOIDO
SNDMIX_STORESTEREOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE( Stereo16BitMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETSTEREOVOL16NOIDO
SNDMIX_STORESTEREOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE( Stereo8BitLinearMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETSTEREOVOL8LINEAR
SNDMIX_STORESTEREOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE( Stereo16BitLinearMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETSTEREOVOL16LINEAR
SNDMIX_STORESTEREOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE( Stereo8BitSplineMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETSTEREOVOL8SPLINE
SNDMIX_STORESTEREOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE( Stereo16BitSplineMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETSTEREOVOL16SPLINE
SNDMIX_STORESTEREOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE( Stereo8BitFirFilterMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETSTEREOVOL8FIRFILTER
SNDMIX_STORESTEREOVOL
END_MIX_INTERFACE()

BEGIN_MIX_INTERFACE( Stereo16BitFirFilterMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETSTEREOVOL16FIRFILTER
SNDMIX_STORESTEREOVOL
END_MIX_INTERFACE()


// Volume Ramps
BEGIN_RAMPMIX_INTERFACE( Stereo8BitRampMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETSTEREOVOL8NOIDO
SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE( Stereo16BitRampMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETSTEREOVOL16NOIDO
SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE( Stereo8BitLinearRampMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETSTEREOVOL8LINEAR
SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE( Stereo16BitLinearRampMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETSTEREOVOL16LINEAR
SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE( Stereo8BitSplineRampMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETSTEREOVOL8SPLINE
SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE( Stereo16BitSplineRampMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETSTEREOVOL16SPLINE
SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE( Stereo8BitFirFilterRampMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETSTEREOVOL8FIRFILTER
SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_INTERFACE()

BEGIN_RAMPMIX_INTERFACE( Stereo16BitFirFilterRampMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETSTEREOVOL16FIRFILTER
SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_INTERFACE()



//////////////////////////////////////////////////////
// Resonant Filter Mix

#ifndef NO_FILTER

// Mono Filter Mix
BEGIN_MIX_FLT_INTERFACE( FilterMono8BitMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETMONOVOL8NOIDO
SNDMIX_PROCESSFILTER
SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()

BEGIN_MIX_FLT_INTERFACE( FilterMono16BitMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETMONOVOL16NOIDO
SNDMIX_PROCESSFILTER
SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()

BEGIN_MIX_FLT_INTERFACE( FilterMono8BitLinearMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETMONOVOL8LINEAR
SNDMIX_PROCESSFILTER
SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()

BEGIN_MIX_FLT_INTERFACE( FilterMono16BitLinearMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETMONOVOL16LINEAR
SNDMIX_PROCESSFILTER
SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()

BEGIN_MIX_FLT_INTERFACE( FilterMono8BitSplineMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETMONOVOL8SPLINE
SNDMIX_PROCESSFILTER
SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()

BEGIN_MIX_FLT_INTERFACE( FilterMono16BitSplineMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETMONOVOL16SPLINE
SNDMIX_PROCESSFILTER
SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()

BEGIN_MIX_FLT_INTERFACE( FilterMono8BitFirFilterMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETMONOVOL8FIRFILTER
SNDMIX_PROCESSFILTER
SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()

BEGIN_MIX_FLT_INTERFACE( FilterMono16BitFirFilterMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETMONOVOL16FIRFILTER
SNDMIX_PROCESSFILTER
SNDMIX_STOREMONOVOL
END_MIX_FLT_INTERFACE()

// Filter + Ramp
BEGIN_RAMPMIX_FLT_INTERFACE( FilterMono8BitRampMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETMONOVOL8NOIDO
SNDMIX_PROCESSFILTER
SNDMIX_RAMPMONOVOL
END_RAMPMIX_FLT_INTERFACE()

BEGIN_RAMPMIX_FLT_INTERFACE( FilterMono16BitRampMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETMONOVOL16NOIDO
SNDMIX_PROCESSFILTER
SNDMIX_RAMPMONOVOL
END_RAMPMIX_FLT_INTERFACE()

BEGIN_RAMPMIX_FLT_INTERFACE( FilterMono8BitLinearRampMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETMONOVOL8LINEAR
SNDMIX_PROCESSFILTER
SNDMIX_RAMPMONOVOL
END_RAMPMIX_FLT_INTERFACE()

BEGIN_RAMPMIX_FLT_INTERFACE( FilterMono16BitLinearRampMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETMONOVOL16LINEAR
SNDMIX_PROCESSFILTER
SNDMIX_RAMPMONOVOL
END_RAMPMIX_FLT_INTERFACE()

BEGIN_RAMPMIX_FLT_INTERFACE( FilterMono8BitSplineRampMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETMONOVOL8SPLINE
SNDMIX_PROCESSFILTER
SNDMIX_RAMPMONOVOL
END_RAMPMIX_FLT_INTERFACE()

BEGIN_RAMPMIX_FLT_INTERFACE( FilterMono16BitSplineRampMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETMONOVOL16SPLINE
SNDMIX_PROCESSFILTER
SNDMIX_RAMPMONOVOL
END_RAMPMIX_FLT_INTERFACE()

BEGIN_RAMPMIX_FLT_INTERFACE( FilterMono8BitFirFilterRampMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETMONOVOL8FIRFILTER
SNDMIX_PROCESSFILTER
SNDMIX_RAMPMONOVOL
END_RAMPMIX_FLT_INTERFACE()

BEGIN_RAMPMIX_FLT_INTERFACE( FilterMono16BitFirFilterRampMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETMONOVOL16FIRFILTER
SNDMIX_PROCESSFILTER
SNDMIX_RAMPMONOVOL
END_RAMPMIX_FLT_INTERFACE()


// Stereo Filter Mix
BEGIN_MIX_STFLT_INTERFACE( FilterStereo8BitMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETSTEREOVOL8NOIDO
SNDMIX_PROCESSSTEREOFILTER
SNDMIX_STORESTEREOVOL
END_MIX_STFLT_INTERFACE()

BEGIN_MIX_STFLT_INTERFACE( FilterStereo16BitMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETSTEREOVOL16NOIDO
SNDMIX_PROCESSSTEREOFILTER
SNDMIX_STORESTEREOVOL
END_MIX_STFLT_INTERFACE()

BEGIN_MIX_STFLT_INTERFACE( FilterStereo8BitLinearMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETSTEREOVOL8LINEAR
SNDMIX_PROCESSSTEREOFILTER
SNDMIX_STORESTEREOVOL
END_MIX_STFLT_INTERFACE()

BEGIN_MIX_STFLT_INTERFACE( FilterStereo16BitLinearMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETSTEREOVOL16LINEAR
SNDMIX_PROCESSSTEREOFILTER
SNDMIX_STORESTEREOVOL
END_MIX_STFLT_INTERFACE()

BEGIN_MIX_STFLT_INTERFACE( FilterStereo8BitSplineMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETSTEREOVOL8SPLINE
SNDMIX_PROCESSSTEREOFILTER
SNDMIX_STORESTEREOVOL
END_MIX_STFLT_INTERFACE()

BEGIN_MIX_STFLT_INTERFACE( FilterStereo16BitSplineMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETSTEREOVOL16SPLINE
SNDMIX_PROCESSSTEREOFILTER
SNDMIX_STORESTEREOVOL
END_MIX_STFLT_INTERFACE()

BEGIN_MIX_STFLT_INTERFACE( FilterStereo8BitFirFilterMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETSTEREOVOL8FIRFILTER
SNDMIX_PROCESSSTEREOFILTER
SNDMIX_STORESTEREOVOL
END_MIX_STFLT_INTERFACE()

BEGIN_MIX_STFLT_INTERFACE( FilterStereo16BitFirFilterMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETSTEREOVOL16FIRFILTER
SNDMIX_PROCESSSTEREOFILTER
SNDMIX_STORESTEREOVOL
END_MIX_STFLT_INTERFACE()

// Stereo Filter + Ramp
BEGIN_RAMPMIX_STFLT_INTERFACE( FilterStereo8BitRampMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETSTEREOVOL8NOIDO
SNDMIX_PROCESSSTEREOFILTER
SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_STFLT_INTERFACE()

BEGIN_RAMPMIX_STFLT_INTERFACE( FilterStereo16BitRampMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETSTEREOVOL16NOIDO
SNDMIX_PROCESSSTEREOFILTER
SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_STFLT_INTERFACE()

BEGIN_RAMPMIX_STFLT_INTERFACE( FilterStereo8BitLinearRampMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETSTEREOVOL8LINEAR
SNDMIX_PROCESSSTEREOFILTER
SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_STFLT_INTERFACE()

BEGIN_RAMPMIX_STFLT_INTERFACE( FilterStereo16BitLinearRampMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETSTEREOVOL16LINEAR
SNDMIX_PROCESSSTEREOFILTER
SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_STFLT_INTERFACE()

BEGIN_RAMPMIX_STFLT_INTERFACE( FilterStereo8BitSplineRampMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETSTEREOVOL8SPLINE
SNDMIX_PROCESSSTEREOFILTER
SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_STFLT_INTERFACE()

BEGIN_RAMPMIX_STFLT_INTERFACE( FilterStereo16BitSplineRampMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETSTEREOVOL16SPLINE
SNDMIX_PROCESSSTEREOFILTER
SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_STFLT_INTERFACE()

BEGIN_RAMPMIX_STFLT_INTERFACE( FilterStereo8BitFirFilterRampMix )
SNDMIX_BEGINSAMPLELOOP8
SNDMIX_GETSTEREOVOL8FIRFILTER
SNDMIX_PROCESSSTEREOFILTER
SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_STFLT_INTERFACE()

BEGIN_RAMPMIX_STFLT_INTERFACE( FilterStereo16BitFirFilterRampMix )
SNDMIX_BEGINSAMPLELOOP16
SNDMIX_GETSTEREOVOL16FIRFILTER
SNDMIX_PROCESSSTEREOFILTER
SNDMIX_RAMPSTEREOVOL
END_RAMPMIX_STFLT_INTERFACE()


#else
// Mono
#define FilterMono8BitMix              Mono8BitMix
#define FilterMono16BitMix             Mono16BitMix
#define FilterMono8BitLinearMix           Mono8BitLinearMix
#define FilterMono16BitLinearMix       Mono16BitLinearMix
#define FilterMono8BitSplineMix           Mono8BitSplineMix
#define FilterMono16BitSplineMix       Mono16BitSplineMix
#define FilterMono8BitFirFilterMix        Mono8BitFirFilterMix
#define FilterMono16BitFirFilterMix       Mono16BitFirFilterMix
#define FilterMono8BitRampMix          Mono8BitRampMix
#define FilterMono16BitRampMix            Mono16BitRampMix
#define FilterMono8BitLinearRampMix       Mono8BitLinearRampMix
#define FilterMono16BitLinearRampMix      Mono16BitLinearRampMix
#define FilterMono8BitSplineRampMix       Mono8BitSplineRampMix
#define FilterMono16BitSplineRampMix      Mono16BitSplineRampMix
#define FilterMono8BitFirFilterRampMix    Mono8BitFirFilterRampMix
#define FilterMono16BitFirFilterRampMix      Mono16BitFirFilterRampMix
// Stereo
#define FilterStereo8BitMix               Stereo8BitMix
#define FilterStereo16BitMix           Stereo16BitMix
#define FilterStereo8BitLinearMix         Stereo8BitLinearMix
#define FilterStereo16BitLinearMix        Stereo16BitLinearMix
#define FilterStereo8BitSplineMix         Stereo8BitSplineMix
#define FilterStereo16BitSplineMix        Stereo16BitSplineMix
#define FilterStereo8BitFirFilterMix      Stereo8BitFirFilterMix
#define FilterStereo16BitFirFilterMix     Stereo16BitFirFilterMix
#define FilterStereo8BitRampMix           Stereo8BitRampMix
#define FilterStereo16BitRampMix       Stereo16BitRampMix
#define FilterStereo8BitLinearRampMix     Stereo8BitLinearRampMix
#define FilterStereo16BitLinearRampMix    Stereo16BitLinearRampMix
#define FilterStereo8BitSplineRampMix     Stereo8BitSplineRampMix
#define FilterStereo16BitSplineRampMix    Stereo16BitSplineRampMix
#define FilterStereo8BitFirFilterRampMix  Stereo8BitFirFilterRampMix
#define FilterStereo16BitFirFilterRampMix Stereo16BitFirFilterRampMix

#endif

///////////////////////////////////////////////////////////////////////////////
//
// Mix function tables
//
//
// Index is as follow:
//	[b1-b0]  format (8-bit-mono, 16-bit-mono, 8-bit-stereo, 16-bit-stereo)
//	[b2]  ramp
//	[b3]  filter
//	[b5-b4]  src type
//

#define MIXNDX_16BIT        0x01
#define MIXNDX_STEREO       0x02
#define MIXNDX_RAMP         0x04
#define MIXNDX_FILTER       0x08
#define MIXNDX_LINEARSRC    0x10
#define MIXNDX_SPLINESRC    0x20
#define MIXNDX_FIRSRC       0x30

const LPMIXINTERFACE gpMixFunctionTable[2 * 2 * 16] =
{
	// No SRC
	Mono8BitMix, Mono16BitMix, Stereo8BitMix, Stereo16BitMix,
	Mono8BitRampMix, Mono16BitRampMix, Stereo8BitRampMix,
	Stereo16BitRampMix,
	// No SRC, Filter
	FilterMono8BitMix, FilterMono16BitMix, FilterStereo8BitMix,
	FilterStereo16BitMix, FilterMono8BitRampMix, FilterMono16BitRampMix,
	FilterStereo8BitRampMix, FilterStereo16BitRampMix,
	// Linear SRC
	Mono8BitLinearMix, Mono16BitLinearMix, Stereo8BitLinearMix,
	Stereo16BitLinearMix, Mono8BitLinearRampMix, Mono16BitLinearRampMix,
	Stereo8BitLinearRampMix, Stereo16BitLinearRampMix,
	// Linear SRC, Filter
	FilterMono8BitLinearMix, FilterMono16BitLinearMix,
	FilterStereo8BitLinearMix, FilterStereo16BitLinearMix,
	FilterMono8BitLinearRampMix, FilterMono16BitLinearRampMix,
	FilterStereo8BitLinearRampMix, FilterStereo16BitLinearRampMix,

	// FirFilter SRC
	Mono8BitSplineMix, Mono16BitSplineMix, Stereo8BitSplineMix,
	Stereo16BitSplineMix, Mono8BitSplineRampMix, Mono16BitSplineRampMix,
	Stereo8BitSplineRampMix, Stereo16BitSplineRampMix,
	// Spline SRC, Filter
	FilterMono8BitSplineMix, FilterMono16BitSplineMix,
	FilterStereo8BitSplineMix, FilterStereo16BitSplineMix,
	FilterMono8BitSplineRampMix, FilterMono16BitSplineRampMix,
	FilterStereo8BitSplineRampMix, FilterStereo16BitSplineRampMix,

	// FirFilter  SRC
	Mono8BitFirFilterMix, Mono16BitFirFilterMix, Stereo8BitFirFilterMix,
	Stereo16BitFirFilterMix, Mono8BitFirFilterRampMix,
	Mono16BitFirFilterRampMix, Stereo8BitFirFilterRampMix,
	Stereo16BitFirFilterRampMix,
	// FirFilter  SRC, Filter
	FilterMono8BitFirFilterMix, FilterMono16BitFirFilterMix,
	FilterStereo8BitFirFilterMix, FilterStereo16BitFirFilterMix,
	FilterMono8BitFirFilterRampMix, FilterMono16BitFirFilterRampMix,
	FilterStereo8BitFirFilterRampMix, FilterStereo16BitFirFilterRampMix
};

const LPMIXINTERFACE gpFastMixFunctionTable[2 * 2 * 16] =
{
	// No SRC
	FastMono8BitMix, FastMono16BitMix, Stereo8BitMix, Stereo16BitMix,
	FastMono8BitRampMix, FastMono16BitRampMix, Stereo8BitRampMix,
	Stereo16BitRampMix,
	// No SRC, Filter
	FilterMono8BitMix, FilterMono16BitMix, FilterStereo8BitMix,
	FilterStereo16BitMix, FilterMono8BitRampMix, FilterMono16BitRampMix,
	FilterStereo8BitRampMix, FilterStereo16BitRampMix,
	// Linear SRC
	FastMono8BitLinearMix, FastMono16BitLinearMix, Stereo8BitLinearMix,
	Stereo16BitLinearMix, FastMono8BitLinearRampMix,
	FastMono16BitLinearRampMix, Stereo8BitLinearRampMix,
	Stereo16BitLinearRampMix,
	// Linear SRC, Filter
	FilterMono8BitLinearMix, FilterMono16BitLinearMix,
	FilterStereo8BitLinearMix, FilterStereo16BitLinearMix,
	FilterMono8BitLinearRampMix, FilterMono16BitLinearRampMix,
	FilterStereo8BitLinearRampMix, FilterStereo16BitLinearRampMix,

	// Spline SRC
	Mono8BitSplineMix, Mono16BitSplineMix, Stereo8BitSplineMix,
	Stereo16BitSplineMix, Mono8BitSplineRampMix, Mono16BitSplineRampMix,
	Stereo8BitSplineRampMix, Stereo16BitSplineRampMix,
	// Spline SRC, Filter
	FilterMono8BitSplineMix, FilterMono16BitSplineMix,
	FilterStereo8BitSplineMix, FilterStereo16BitSplineMix,
	FilterMono8BitSplineRampMix, FilterMono16BitSplineRampMix,
	FilterStereo8BitSplineRampMix, FilterStereo16BitSplineRampMix,

	// FirFilter SRC
	Mono8BitFirFilterMix, Mono16BitFirFilterMix, Stereo8BitFirFilterMix,
	Stereo16BitFirFilterMix, Mono8BitFirFilterRampMix,
	Mono16BitFirFilterRampMix, Stereo8BitFirFilterRampMix,
	Stereo16BitFirFilterRampMix,
	// FirFilter SRC, Filter
	FilterMono8BitFirFilterMix, FilterMono16BitFirFilterMix,
	FilterStereo8BitFirFilterMix, FilterStereo16BitFirFilterMix,
	FilterMono8BitFirFilterRampMix, FilterMono16BitFirFilterRampMix,
	FilterStereo8BitFirFilterRampMix, FilterStereo16BitFirFilterRampMix,
};


/////////////////////////////////////////////////////////////////////////

static LONG MPPFASTCALL GetSampleCount( MODCHANNEL* pChn, LONG nSamples )
//---------------------------------------------------------------------
{
	LONG nLoopStart = ( pChn->dwFlags & CHN_LOOP ) ? pChn->nLoopStart : 0;
	LONG nInc = pChn->nInc;

	if ( ( nSamples <= 0 ) || ( !nInc ) || ( !pChn->nLength ) ) { return 0; }

	// Under zero ?
	if ( ( LONG )pChn->nPos < nLoopStart )
	{
		if ( nInc < 0 )
		{
			// Invert loop for bidi loops
			LONG nDelta = ( ( nLoopStart - pChn->nPos ) << 16 ) - ( pChn->nPosLo & 0xffff );
			pChn->nPos = nLoopStart | ( nDelta >> 16 );
			pChn->nPosLo = nDelta & 0xffff;

			if ( ( ( LONG )pChn->nPos < nLoopStart ) ||
			     ( pChn->nPos >= ( nLoopStart + pChn->nLength ) / 2 ) )
			{
				pChn->nPos = nLoopStart;
				pChn->nPosLo = 0;
			}

			nInc = -nInc;
			pChn->nInc = nInc;
			pChn->dwFlags &= ~( CHN_PINGPONGFLAG ); // go forward

			if ( ( !( pChn->dwFlags & CHN_LOOP ) ) || ( pChn->nPos >= pChn->nLength ) )
			{
				pChn->nPos = pChn->nLength;
				pChn->nPosLo = 0;
				return 0;
			}
		}
		else
		{
			// We probably didn't hit the loop end yet
			// (first loop), so we do nothing
			if ( ( LONG )pChn->nPos < 0 ) { pChn->nPos = 0; }
		}
	}
	else

		// Past the end
		if ( pChn->nPos >= pChn->nLength )
		{
			if ( !( pChn->dwFlags & CHN_LOOP ) ) { return 0; } // not looping -> stop this channel

			if ( pChn->dwFlags & CHN_PINGPONGLOOP )
			{
				// Invert loop
				if ( nInc > 0 )
				{
					nInc = -nInc;
					pChn->nInc = nInc;
				}

				pChn->dwFlags |= CHN_PINGPONGFLAG;
				// adjust loop position
				LONG nDeltaHi = ( pChn->nPos - pChn->nLength );
				LONG nDeltaLo = 0x10000 - ( pChn->nPosLo & 0xffff );
				pChn->nPos = pChn->nLength - nDeltaHi - ( nDeltaLo >> 16 );
				pChn->nPosLo = nDeltaLo & 0xffff;

				if ( ( pChn->nPos <= pChn->nLoopStart ) ||
				     ( pChn->nPos >= pChn->nLength ) )
				{
					pChn->nPos = pChn->nLength - 1;
				}
			}
			else
			{
				if ( nInc < 0 ) // This is a bug
				{
					nInc = -nInc;
					pChn->nInc = nInc;
				}

				// Restart at loop start
				pChn->nPos += nLoopStart - pChn->nLength;

				if ( ( LONG )pChn->nPos < nLoopStart )
				{
					pChn->nPos = pChn->nLoopStart;
				}
			}
		}

	LONG nPos = pChn->nPos;

	// too big increment, and/or too small loop length
	if ( nPos < nLoopStart )
	{
		if ( ( nPos < 0 ) || ( nInc < 0 ) ) { return 0; }
	}

	if ( ( nPos < 0 ) || ( nPos >= ( LONG )pChn->nLength ) ) { return 0; }

	LONG nPosLo = ( USHORT )pChn->nPosLo, nSmpCount = nSamples;

	if ( nInc < 0 )
	{
		LONG nInv = -nInc;
		LONG maxsamples = 16384 / ( ( nInv >> 16 ) + 1 );

		if ( maxsamples < 2 ) { maxsamples = 2; }

		if ( nSamples > maxsamples ) { nSamples = maxsamples; }

		LONG nDeltaHi = ( nInv >> 16 ) * ( nSamples - 1 );
		LONG nDeltaLo = ( nInv & 0xffff ) * ( nSamples - 1 );
		LONG nPosDest = nPos - nDeltaHi + ( ( nPosLo - nDeltaLo ) >> 16 );

		if ( nPosDest < nLoopStart )
		{
			nSmpCount = ( ULONG )( ( ( ( ( LONGLONG )nPos - nLoopStart ) << 16 ) + nPosLo - 1 ) / nInv ) + 1;
		}
	}
	else
	{
		LONG maxsamples = 16384 / ( ( nInc >> 16 ) + 1 );

		if ( maxsamples < 2 ) { maxsamples = 2; }

		if ( nSamples > maxsamples ) { nSamples = maxsamples; }

		LONG nDeltaHi = ( nInc >> 16 ) * ( nSamples - 1 );
		LONG nDeltaLo = ( nInc & 0xffff ) * ( nSamples - 1 );
		LONG nPosDest = nPos + nDeltaHi + ( ( nPosLo + nDeltaLo ) >> 16 );

		if ( nPosDest >= ( LONG )pChn->nLength )
		{
			nSmpCount = ( ULONG )( ( ( ( ( LONGLONG )pChn->nLength - nPos ) << 16 ) - nPosLo - 1 ) / nInc ) + 1;
		}
	}

	if ( nSmpCount <= 1 ) { return 1; }

	if ( nSmpCount > nSamples ) { return nSamples; }

	return nSmpCount;
}


UINT CSoundFile::CreateStereoMix( int count )
//-----------------------------------------
{
	LPLONG pOfsL, pOfsR;
	DWORD nchused, nchmixed;

	if ( !count ) { return 0; }

#ifndef MODPLUG_FASTSOUNDLIB

	if ( gnChannels > 2 ) { X86_InitMixBuffer( MixRearBuffer, count * 2 ); }

#endif
	nchused = nchmixed = 0;

	for ( UINT nChn = 0; nChn < m_nMixChannels; nChn++ )
	{
		const LPMIXINTERFACE* pMixFuncTable;
		MODCHANNEL* const pChannel = &Chn[ChnMix[nChn]];
		UINT nFlags, nMasterCh;
		LONG nSmpCount;
		int nsamples;
		int* pbuffer;

		if ( !pChannel->pCurrentSample ) { continue; }

		nMasterCh = ( ChnMix[nChn] < m_nChannels ) ? ChnMix[nChn] + 1 : pChannel->nMasterChn;
		pOfsR = &gnDryROfsVol;
		pOfsL = &gnDryLOfsVol;
		nFlags = 0;

		if ( pChannel->dwFlags & CHN_16BIT ) { nFlags |= MIXNDX_16BIT; }

		if ( pChannel->dwFlags & CHN_STEREO ) { nFlags |= MIXNDX_STEREO; }

#ifndef NO_FILTER

		if ( pChannel->dwFlags & CHN_FILTER ) { nFlags |= MIXNDX_FILTER; }

#endif

		if ( !( pChannel->dwFlags & CHN_NOIDO ) )
		{
			// use hq-fir mixer?
			if ( ( gdwSoundSetup & ( SNDMIX_HQRESAMPLER | SNDMIX_ULTRAHQSRCMODE ) ) ==
			     ( SNDMIX_HQRESAMPLER | SNDMIX_ULTRAHQSRCMODE ) )
			{
				nFlags += MIXNDX_FIRSRC;
			}
			else if ( ( gdwSoundSetup & ( SNDMIX_HQRESAMPLER ) ) == SNDMIX_HQRESAMPLER )
			{
				nFlags += MIXNDX_SPLINESRC;
			}
			else
			{
				nFlags += MIXNDX_LINEARSRC;   // use
			}
		}

		if ( ( nFlags < 0x40 ) && ( pChannel->nLeftVol == pChannel->nRightVol )
		     && ( ( !pChannel->nRampLength ) || ( pChannel->nLeftRamp == pChannel->nRightRamp ) ) )
		{
			pMixFuncTable = gpFastMixFunctionTable;
		}
		else
		{
			pMixFuncTable = gpMixFunctionTable;
		}

		nsamples = count;
#ifndef MODPLUG_NO_REVERB
		pbuffer = ( gdwSoundSetup & SNDMIX_REVERB ) ? MixReverbBuffer : MixSoundBuffer;

		if ( pChannel->dwFlags & CHN_NOREVERB ) { pbuffer = MixSoundBuffer; }

		if ( pChannel->dwFlags & CHN_REVERB ) { pbuffer = MixReverbBuffer; }

		if ( pbuffer == MixReverbBuffer )
		{
			if ( !gnReverbSend ) { memset( MixReverbBuffer, 0, count * 8 ); }

			gnReverbSend += count;
		}

#else
		pbuffer = MixSoundBuffer;
#endif
		nchused++;
		////////////////////////////////////////////////////
		SampleLooping:
		UINT nrampsamples = nsamples;

		if ( pChannel->nRampLength > 0 )
		{
			if ( ( LONG )nrampsamples > pChannel->nRampLength ) { nrampsamples = pChannel->nRampLength; }
		}

		if ( ( nSmpCount = GetSampleCount( pChannel, nrampsamples ) ) <= 0 )
		{
			// Stopping the channel
			pChannel->pCurrentSample = NULL;
			pChannel->nLength = 0;
			pChannel->nPos = 0;
			pChannel->nPosLo = 0;
			pChannel->nRampLength = 0;
			X86_EndChannelOfs( pChannel, pbuffer, nsamples );
			*pOfsR += pChannel->nROfs;
			*pOfsL += pChannel->nLOfs;
			pChannel->nROfs = pChannel->nLOfs = 0;
			pChannel->dwFlags &= ~CHN_PINGPONGFLAG;
			continue;
		}

		// Should we mix this channel ?
		UINT naddmix;

		if ( ( ( nchmixed >= m_nMaxMixChannels ) && ( !( gdwSoundSetup & SNDMIX_DIRECTTODISK ) ) )
		     || ( ( !pChannel->nRampLength ) && ( !( pChannel->nLeftVol | pChannel->nRightVol ) ) ) )
		{
			LONG delta = ( pChannel->nInc * ( LONG )nSmpCount ) + ( LONG )pChannel->nPosLo;
			pChannel->nPosLo = delta & 0xFFFF;
			pChannel->nPos += ( delta >> 16 );
			pChannel->nROfs = pChannel->nLOfs = 0;
			pbuffer += nSmpCount * 2;
			naddmix = 0;
		}
		else
			// Do mixing
		{
			// Choose function for mixing
			LPMIXINTERFACE pMixFunc;
			pMixFunc = ( pChannel->nRampLength ) ? pMixFuncTable[nFlags | MIXNDX_RAMP] : pMixFuncTable[nFlags];
			int* pbufmax = pbuffer + ( nSmpCount * 2 );
			pChannel->nROfs = - *( pbufmax - 2 );
			pChannel->nLOfs = - *( pbufmax - 1 );
			pMixFunc( pChannel, pbuffer, pbufmax );
			pChannel->nROfs += *( pbufmax - 2 );
			pChannel->nLOfs += *( pbufmax - 1 );
			pbuffer = pbufmax;
			naddmix = 1;

		}

		nsamples -= nSmpCount;

		if ( pChannel->nRampLength )
		{
			pChannel->nRampLength -= nSmpCount;

			if ( pChannel->nRampLength <= 0 )
			{
				pChannel->nRampLength = 0;
				pChannel->nRightVol = pChannel->nNewRightVol;
				pChannel->nLeftVol = pChannel->nNewLeftVol;
				pChannel->nRightRamp = pChannel->nLeftRamp = 0;

				if ( ( pChannel->dwFlags & CHN_NOTEFADE ) && ( !( pChannel->nFadeOutVol ) ) )
				{
					pChannel->nLength = 0;
					pChannel->pCurrentSample = NULL;
				}
			}
		}

		if ( nsamples > 0 ) { goto SampleLooping; }

		nchmixed += naddmix;
	}

	return nchused;
}


#ifdef MSC_VER
#pragma warning (disable:4100)
#endif

// Clip and convert to 8 bit
#ifdef MSC_VER
__declspec( naked ) DWORD MPPASMCALL X86_Convert32To8( LPVOID lp16, int* pBuffer, DWORD lSampleCount, LPLONG lpMin, LPLONG lpMax )
//------------------------------------------------------------------------------
{
	_asm
	{
		push ebx
		push esi
		push edi
		mov ebx, 16[esp]     // ebx = 8-bit buffer
		mov esi, 20[esp]     // esi = pBuffer
		mov edi, 24[esp]     // edi = lSampleCount
		mov eax, 28[esp]
		mov ecx, dword ptr [eax]   // ecx = clipmin
		mov eax, 32[esp]
		mov edx, dword ptr [eax]   // edx = clipmax
		cliploop:
		mov eax, dword ptr [esi]
		inc ebx
		cdq
		and edx, ( 1 << ( 24-MIXING_ATTENUATION ) ) - 1
		add eax, edx
		cmp eax, MIXING_CLIPMIN
		jl cliplow
		cmp eax, MIXING_CLIPMAX
		jg cliphigh
		cmp eax, ecx
		jl updatemin
		cmp eax, edx
		jg updatemax
		cliprecover:
		add esi, 4
		sar eax, 24 - MIXING_ATTENUATION
		xor eax, 0x80
		dec edi
		mov byte ptr [ebx - 1], al
		jnz cliploop
		mov eax, 28[esp]
		mov dword ptr [eax], ecx
		mov eax, 32[esp]
		mov dword ptr [eax], edx
		mov eax, 24[esp]
		pop edi
		pop esi
		pop ebx
		ret
		updatemin:
		mov ecx, eax
		jmp cliprecover
		updatemax:
		mov edx, eax
		jmp cliprecover
		cliplow:
		mov ecx, MIXING_CLIPMIN
		mov edx, MIXING_CLIPMAX
		mov eax, MIXING_CLIPMIN
		jmp cliprecover
		cliphigh:
		mov ecx, MIXING_CLIPMIN
		mov edx, MIXING_CLIPMAX
		mov eax, MIXING_CLIPMAX
		jmp cliprecover
	}
}
#else //MSC_VER
//---GCCFIX: Asm replaced with C function
// The C version was written by Rani Assaf <rani@magic.metawire.com>, I believe
DWORD MPPASMCALL X86_Convert32To8( LPVOID lp8, int* pBuffer, DWORD lSampleCount, LPLONG lpMin, LPLONG lpMax )
{
	int vumin = *lpMin, vumax = *lpMax;
	unsigned char* p = ( unsigned char* )lp8;

	for ( UINT i = 0; i < lSampleCount; i++ )
	{
		int n = pBuffer[i];

		if ( n < MIXING_CLIPMIN )
		{
			n = MIXING_CLIPMIN;
		}
		else if ( n > MIXING_CLIPMAX )
		{
			n = MIXING_CLIPMAX;
		}

		if ( n < vumin )
		{
			vumin = n;
		}
		else if ( n > vumax )
		{
			vumax = n;
		}

		p[i] = ( n >> ( 24 - MIXING_ATTENUATION ) ) ^ 0x80; // 8-bit unsigned
	}

	*lpMin = vumin;
	*lpMax = vumax;
	return lSampleCount;
}
#endif //MSC_VER, else


#ifdef MSC_VER
// Clip and convert to 16 bit
__declspec( naked ) DWORD MPPASMCALL X86_Convert32To16( LPVOID lp16, int* pBuffer, DWORD lSampleCount, LPLONG lpMin, LPLONG lpMax )
//------------------------------------------------------------------------------
{
	_asm
	{
		push ebx
		push esi
		push edi
		mov ebx, 16[esp]     // ebx = 16-bit buffer
		mov eax, 28[esp]
		mov esi, 20[esp]     // esi = pBuffer
		mov ecx, dword ptr [eax]   // ecx = clipmin
		mov edi, 24[esp]     // edi = lSampleCount
		mov eax, 32[esp]
		push ebp
		mov ebp, dword ptr [eax]   // edx = clipmax
		cliploop:
		mov eax, dword ptr [esi]
		add ebx, 2
		cdq
		and edx, ( 1 << ( 16-MIXING_ATTENUATION ) ) - 1
		add esi, 4
		add eax, edx
		cmp eax, MIXING_CLIPMIN
		jl cliplow
		cmp eax, MIXING_CLIPMAX
		jg cliphigh
		cmp eax, ecx
		jl updatemin
		cmp eax, ebp
		jg updatemax
		cliprecover:
		sar eax, 16 - MIXING_ATTENUATION
		dec edi
		mov word ptr [ebx - 2], ax
		jnz cliploop
		mov edx, ebp
		pop ebp
		mov eax, 28[esp]
		mov dword ptr [eax], ecx
		mov eax, 32[esp]
		mov dword ptr [eax], edx
		mov eax, 24[esp]
		pop edi
		shl eax, 1
		pop esi
		pop ebx
		ret
		updatemin:
		mov ecx, eax
		jmp cliprecover
		updatemax:
		mov ebp, eax
		jmp cliprecover
		cliplow:
		mov ecx, MIXING_CLIPMIN
		mov ebp, MIXING_CLIPMAX
		mov eax, MIXING_CLIPMIN
		jmp cliprecover
		cliphigh:
		mov ecx, MIXING_CLIPMIN
		mov ebp, MIXING_CLIPMAX
		mov eax, MIXING_CLIPMAX
		jmp cliprecover
	}
}
#else //MSC_VER
//---GCCFIX: Asm replaced with C function
// The C version was written by Rani Assaf <rani@magic.metawire.com>, I believe
DWORD MPPASMCALL X86_Convert32To16( LPVOID lp16, int* pBuffer, DWORD lSampleCount, LPLONG lpMin, LPLONG lpMax )
{
	int vumin = *lpMin, vumax = *lpMax;
	signed short* p = ( signed short* )lp16;

	for ( UINT i = 0; i < lSampleCount; i++ )
	{
		int n = pBuffer[i];

		if ( n < MIXING_CLIPMIN )
		{
			n = MIXING_CLIPMIN;
		}
		else if ( n > MIXING_CLIPMAX )
		{
			n = MIXING_CLIPMAX;
		}

		if ( n < vumin )
		{
			vumin = n;
		}
		else if ( n > vumax )
		{
			vumax = n;
		}

		p[i] = n >> ( 16 - MIXING_ATTENUATION ); // 16-bit signed
	}

	*lpMin = vumin;
	*lpMax = vumax;
	return lSampleCount * 2;
}
#endif //MSC_VER, else

#ifdef MSC_VER
// Clip and convert to 24 bit
__declspec( naked ) DWORD MPPASMCALL X86_Convert32To24( LPVOID lp16, int* pBuffer, DWORD lSampleCount, LPLONG lpMin, LPLONG lpMax )
//------------------------------------------------------------------------------
{
	_asm
	{
		push ebx
		push esi
		push edi
		mov ebx, 16[esp]     // ebx = 8-bit buffer
		mov esi, 20[esp]     // esi = pBuffer
		mov edi, 24[esp]     // edi = lSampleCount
		mov eax, 28[esp]
		mov ecx, dword ptr [eax]   // ecx = clipmin
		mov eax, 32[esp]
		push ebp
		mov edx, dword ptr [eax]   // edx = clipmax
		cliploop:
		mov eax, dword ptr [esi]
		mov ebp, eax
		sar ebp, 31
		and ebp, ( 1 << ( 8-MIXING_ATTENUATION ) ) - 1
		add eax, ebp
		cmp eax, MIXING_CLIPMIN
		jl cliplow
		cmp eax, MIXING_CLIPMAX
		jg cliphigh
		cmp eax, ecx
		jl updatemin
		cmp eax, edx
		jg updatemax
		cliprecover:
		add ebx, 3
		sar eax, 8 - MIXING_ATTENUATION
		add esi, 4
		mov word ptr [ebx - 3], ax
		shr eax, 16
		dec edi
		mov byte ptr [ebx - 1], al
		jnz cliploop
		pop ebp
		mov eax, 28[esp]
		mov dword ptr [eax], ecx
		mov eax, 32[esp]
		mov dword ptr [eax], edx
		mov edx, 24[esp]
		mov eax, edx
		pop edi
		shl eax, 1
		pop esi
		add eax, edx
		pop ebx
		ret
		updatemin:
		mov ecx, eax
		jmp cliprecover
		updatemax:
		mov edx, eax
		jmp cliprecover
		cliplow:
		mov ecx, MIXING_CLIPMIN
		mov edx, MIXING_CLIPMAX
		mov eax, MIXING_CLIPMIN
		jmp cliprecover
		cliphigh:
		mov ecx, MIXING_CLIPMIN
		mov edx, MIXING_CLIPMAX
		mov eax, MIXING_CLIPMAX
		jmp cliprecover
	}
}
#else //MSC_VER
//---GCCFIX: Asm replaced with C function
DWORD MPPASMCALL X86_Convert32To24( LPVOID lp16, int* pBuffer, DWORD lSampleCount, LPLONG lpMin, LPLONG lpMax )
{
	UINT i ;
	int vumin = *lpMin, vumax = *lpMax;
	int n, p ;
	unsigned char* buf = ( unsigned char* )lp16 ;

	for ( i = 0; i < lSampleCount; i++ )
	{
		n = pBuffer[i];

		if ( n < MIXING_CLIPMIN )
		{
			n = MIXING_CLIPMIN;
		}
		else if ( n > MIXING_CLIPMAX )
		{
			n = MIXING_CLIPMAX;
		}

		if ( n < vumin )
		{
			vumin = n;
		}
		else if ( n > vumax )
		{
			vumax = n;
		}

		p = n >> ( 8 - MIXING_ATTENUATION ) ; // 24-bit signed
#ifdef WORDS_BIGENDIAN
		buf[i * 3 + 0] = p & 0xFF0000 >> 24;
		buf[i * 3 + 1] = p & 0x00FF00 >> 16 ;
		buf[i * 3 + 2] = p & 0x0000FF ;
#else
		buf[i * 3 + 0] = p & 0x0000FF ;
		buf[i * 3 + 1] = p & 0x00FF00 >> 16;
		buf[i * 3 + 2] = p & 0xFF0000 >> 24;
#endif
	}

	*lpMin = vumin;
	*lpMax = vumax;
	return lSampleCount * 3;
}
#endif

#ifdef MSC_VER
// Clip and convert to 32 bit
__declspec( naked ) DWORD MPPASMCALL X86_Convert32To32( LPVOID lp16, int* pBuffer, DWORD lSampleCount, LPLONG lpMin, LPLONG lpMax )
//------------------------------------------------------------------------------
{
	_asm
	{
		push ebx
		push esi
		push edi
		mov ebx, 16[esp]        // ebx = 32-bit buffer
		mov esi, 20[esp]        // esi = pBuffer
		mov edi, 24[esp]        // edi = lSampleCount
		mov eax, 28[esp]
		mov ecx, dword ptr [eax]   // ecx = clipmin
		mov eax, 32[esp]
		mov edx, dword ptr [eax]   // edx = clipmax
		cliploop:
		mov eax, dword ptr [esi]
		add ebx, 4
		add esi, 4
		cmp eax, MIXING_CLIPMIN
		jl cliplow
		cmp eax, MIXING_CLIPMAX
		jg cliphigh
		cmp eax, ecx
		jl updatemin
		cmp eax, edx
		jg updatemax
		cliprecover:
		shl eax, MIXING_ATTENUATION
		dec edi
		mov dword ptr [ebx-4], eax
		jnz cliploop
		mov eax, 28[esp]
		mov dword ptr [eax], ecx
		mov eax, 32[esp]
		mov dword ptr [eax], edx
		mov edx, 24[esp]
		pop edi
		mov eax, edx
		pop esi
		shl eax, 2
		pop ebx
		ret
		updatemin:
		mov ecx, eax
		jmp cliprecover
		updatemax:
		mov edx, eax
		jmp cliprecover
		cliplow:
		mov ecx, MIXING_CLIPMIN
		mov edx, MIXING_CLIPMAX
		mov eax, MIXING_CLIPMIN
		jmp cliprecover
		cliphigh:
		mov ecx, MIXING_CLIPMIN
		mov edx, MIXING_CLIPMAX
		mov eax, MIXING_CLIPMAX
		jmp cliprecover
	}
}
#else
//---GCCFIX: Asm replaced with C function
DWORD MPPASMCALL X86_Convert32To32( LPVOID lp16, int* pBuffer, DWORD lSampleCount, LPLONG lpMin, LPLONG lpMax )
{
	UINT i ;
	int vumin = *lpMin, vumax = *lpMax;
	int32_t* p = ( int32_t* )lp16;

	for ( i = 0; i < lSampleCount; i++ )
	{
		int n = pBuffer[i];

		if ( n < MIXING_CLIPMIN )
		{
			n = MIXING_CLIPMIN;
		}
		else if ( n > MIXING_CLIPMAX )
		{
			n = MIXING_CLIPMAX;
		}

		if ( n < vumin )
		{
			vumin = n;
		}
		else if ( n > vumax )
		{
			vumax = n;
		}

		p[i] = n << MIXING_ATTENUATION;  // 32-bit signed
	}

	*lpMin = vumin;
	*lpMax = vumax;
	return lSampleCount * 4;
}
#endif


#ifdef MSC_VER
void MPPASMCALL X86_InitMixBuffer( int* pBuffer, UINT nSamples )
//------------------------------------------------------------
{
	_asm
	{
		mov ecx, nSamples
		mov esi, pBuffer
		xor eax, eax
		mov edx, ecx
		shr ecx, 2
		and edx, 3
		jz unroll4x
		loop1x:
		add esi, 4
		dec edx
		mov dword ptr [esi-4], eax
		jnz loop1x
		unroll4x:
		or ecx, ecx
		jnz loop4x
		jmp done
		loop4x:
		add esi, 16
		dec ecx
		mov dword ptr [esi-16], eax
		mov dword ptr [esi-12], eax
		mov dword ptr [esi-8], eax
		mov dword ptr [esi-4], eax
		jnz loop4x
		done:;
	}
}
#else
//---GCCFIX: Asm replaced with C function
// Will fill in later.
void MPPASMCALL X86_InitMixBuffer( int* pBuffer, UINT nSamples )
{
	memset( pBuffer, 0, nSamples * sizeof( int ) );
}
#endif


#ifdef MSC_VER
__declspec( naked ) void MPPASMCALL X86_InterleaveFrontRear( int* pFrontBuf, int* pRearBuf, DWORD nSamples )
//------------------------------------------------------------------------------
{
	_asm
	{
		push ebx
		push ebp
		push esi
		push edi
		mov ecx, 28[esp] // ecx = samplecount
		mov esi, 20[esp] // esi = front buffer
		mov edi, 24[esp] // edi = rear buffer
		lea esi, [esi+ecx*4] // esi = &front[N]
		lea edi, [edi+ecx*4] // edi = &rear[N]
		lea ebx, [esi+ecx*4] // ebx = &front[N*2]
		interleaveloop:
		mov eax, dword ptr [esi-8]
		mov edx, dword ptr [esi-4]
		sub ebx, 16
		mov ebp, dword ptr [edi-8]
		mov dword ptr [ebx], eax
		mov dword ptr [ebx+4], edx
		mov eax, dword ptr [edi-4]
		sub esi, 8
		sub edi, 8
		dec ecx
		mov dword ptr [ebx+8], ebp
		mov dword ptr [ebx+12], eax
		jnz interleaveloop
		pop edi
		pop esi
		pop ebp
		pop ebx
		ret
	}
}
#else
//---GCCFIX: Asm replaced with C function
// Multichannel not supported.
void MPPASMCALL X86_InterleaveFrontRear( int* pFrontBuf, int* pRearBuf, DWORD nSamples )
{
}
#endif


#ifdef MSC_VER
VOID MPPASMCALL X86_MonoFromStereo( int* pMixBuf, UINT nSamples )
//-------------------------------------------------------------
{
	_asm
	{
		mov ecx, nSamples
		mov esi, pMixBuf
		mov edi, esi
		stloop:
		mov eax, dword ptr [esi]
		mov edx, dword ptr [esi+4]
		add edi, 4
		add esi, 8
		add eax, edx
		sar eax, 1
		dec ecx
		mov dword ptr [edi-4], eax
		jnz stloop
	}
}
#else
//---GCCFIX: Asm replaced with C function
VOID MPPASMCALL X86_MonoFromStereo( int* pMixBuf, UINT nSamples )
{
	UINT j;

	for ( UINT i = 0; i < nSamples; i++ )
	{
		j = i << 1;
		pMixBuf[i] = ( pMixBuf[j] + pMixBuf[j + 1] ) >> 1;
	}
}
#endif

#define OFSDECAYSHIFT   8
#define OFSDECAYMASK 0xFF


#ifdef MSC_VER
void MPPASMCALL X86_StereoFill( int* pBuffer, UINT nSamples, LPLONG lpROfs, LPLONG lpLOfs )
//------------------------------------------------------------------------------
{
	_asm
	{
		mov edi, pBuffer
		mov ecx, nSamples
		mov eax, lpROfs
		mov edx, lpLOfs
		mov eax, [eax]
		mov edx, [edx]
		or ecx, ecx
		jz fill_loop
		mov ebx, eax
		or ebx, edx
		jz fill_loop
		ofsloop:
		mov ebx, eax
		mov esi, edx
		neg ebx
		neg esi
		sar ebx, 31
		sar esi, 31
		and ebx, OFSDECAYMASK
		and esi, OFSDECAYMASK
		add ebx, eax
		add esi, edx
		sar ebx, OFSDECAYSHIFT
		sar esi, OFSDECAYSHIFT
		sub eax, ebx
		sub edx, esi
		mov ebx, eax
		or ebx, edx
		jz fill_loop
		add edi, 8
		dec ecx
		mov [edi-8], eax
		mov [edi-4], edx
		jnz ofsloop
		fill_loop:
		mov ebx, ecx
		and ebx, 3
		jz fill4x
		fill1x:
		mov [edi], eax
		mov [edi+4], edx
		add edi, 8
		dec ebx
		jnz fill1x
		fill4x:
		shr ecx, 2
		or ecx, ecx
		jz done
		fill4xloop:
		mov [edi], eax
		mov [edi+4], edx
		mov [edi+8], eax
		mov [edi+12], edx
		add edi, 8*4
		dec ecx
		mov [edi-16], eax
		mov [edi-12], edx
		mov [edi-8], eax
		mov [edi-4], edx
		jnz fill4xloop
		done:
		mov esi, lpROfs
		mov edi, lpLOfs
		mov [esi], eax
		mov [edi], edx
	}
}
#else
//---GCCFIX: Asm replaced with C function
#define OFSDECAYSHIFT    8
#define OFSDECAYMASK     0xFF
void MPPASMCALL X86_StereoFill( int* pBuffer, UINT nSamples, LPLONG lpROfs, LPLONG lpLOfs )
//----------------------------------------------------------------------------
{
	int rofs = *lpROfs;
	int lofs = *lpLOfs;

	if ( ( !rofs ) && ( !lofs ) )
	{
		X86_InitMixBuffer( pBuffer, nSamples * 2 );
		return;
	}

	for ( UINT i = 0; i < nSamples; i++ )
	{
		int x_r = ( rofs + ( ( ( -rofs ) >> 31 ) & OFSDECAYMASK ) ) >> OFSDECAYSHIFT;
		int x_l = ( lofs + ( ( ( -lofs ) >> 31 ) & OFSDECAYMASK ) ) >> OFSDECAYSHIFT;
		rofs -= x_r;
		lofs -= x_l;
		pBuffer[i * 2] = x_r;
		pBuffer[i * 2 + 1] = x_l;
	}

	*lpROfs = rofs;
	*lpLOfs = lofs;
}
#endif

#ifdef MSC_VER
void MPPASMCALL X86_EndChannelOfs( MODCHANNEL* pChannel, int* pBuffer, UINT nSamples )
//------------------------------------------------------------------------------
{
	_asm
	{
		mov esi, pChannel
		mov edi, pBuffer
		mov ecx, nSamples
		mov eax, dword ptr [esi+MODCHANNEL.nROfs]
		mov edx, dword ptr [esi+MODCHANNEL.nLOfs]
		or ecx, ecx
		jz brkloop
		ofsloop:
		mov ebx, eax
		mov esi, edx
		neg ebx
		neg esi
		sar ebx, 31
		sar esi, 31
		and ebx, OFSDECAYMASK
		and esi, OFSDECAYMASK
		add ebx, eax
		add esi, edx
		sar ebx, OFSDECAYSHIFT
		sar esi, OFSDECAYSHIFT
		sub eax, ebx
		sub edx, esi
		mov ebx, eax
		add dword ptr [edi], eax
		add dword ptr [edi+4], edx
		or ebx, edx
		jz brkloop
		add edi, 8
		dec ecx
		jnz ofsloop
		brkloop:
		mov esi, pChannel
		mov dword ptr [esi+MODCHANNEL.nROfs], eax
		mov dword ptr [esi+MODCHANNEL.nLOfs], edx
	}
}
#else
//---GCCFIX: Asm replaced with C function
// Will fill in later.
void MPPASMCALL X86_EndChannelOfs( MODCHANNEL* pChannel, int* pBuffer, UINT nSamples )
{
	int rofs = pChannel->nROfs;
	int lofs = pChannel->nLOfs;

	if ( ( !rofs ) && ( !lofs ) ) { return; }

	for ( UINT i = 0; i < nSamples; i++ )
	{
		int x_r = ( rofs + ( ( ( -rofs ) >> 31 ) & OFSDECAYMASK ) ) >> OFSDECAYSHIFT;
		int x_l = ( lofs + ( ( ( -lofs ) >> 31 ) & OFSDECAYMASK ) ) >> OFSDECAYSHIFT;
		rofs -= x_r;
		lofs -= x_l;
		pBuffer[i * 2] += x_r;
		pBuffer[i * 2 + 1] += x_l;
	}

	pChannel->nROfs = rofs;
	pChannel->nLOfs = lofs;
}
#endif


//////////////////////////////////////////////////////////////////////////////////
// Automatic Gain Control

#ifndef NO_AGC

// Limiter
#define MIXING_LIMITMAX    (0x08100000)
#define MIXING_LIMITMIN    (-MIXING_LIMITMAX)

#ifdef MSC_VER
__declspec( naked ) UINT MPPASMCALL X86_AGC( int* pBuffer, UINT nSamples, UINT nAGC )
//------------------------------------------------------------------------------
{
	__asm
	{
		push ebx
		push ebp
		push esi
		push edi
		mov esi, 20[esp]  // esi = pBuffer+i
		mov ecx, 24[esp]  // ecx = i
		mov edi, 28[esp]  // edi = AGC (0..256)
		agcloop:
		mov eax, dword ptr [esi]
		imul edi
		shrd eax, edx, AGC_PRECISION
		add esi, 4
		cmp eax, MIXING_LIMITMIN
		jl agcupdate
		cmp eax, MIXING_LIMITMAX
		jg agcupdate
		agcrecover:
		dec ecx
		mov dword ptr [esi-4], eax
		jnz agcloop
		mov eax, edi
		pop edi
		pop esi
		pop ebp
		pop ebx
		ret
		agcupdate:
		dec edi
		jmp agcrecover
	}
}

#pragma warning (default:4100)
#else
// Version for GCC
UINT MPPASMCALL X86_AGC( int* pBuffer, UINT nSamples, UINT nAGC )
{
	int x;

	while ( nSamples )
	{
		x = ( ( int64_t )( *pBuffer ) * nAGC ) >> AGC_PRECISION;

		if ( ( x < MIXING_LIMITMIN ) || ( x > MIXING_LIMITMAX ) )
		{
			nAGC--;
		}

		*pBuffer = x;

		pBuffer++;
		nSamples--;
	}

	return nAGC;
}
#endif

void CSoundFile::ProcessAGC( int count )
//------------------------------------
{
	static DWORD gAGCRecoverCount = 0;
	UINT agc = X86_AGC( MixSoundBuffer, count, gnAGC );

	// Some kind custom law, so that the AGC stays quite stable, but slowly
	// goes back up if the sound level stays below a level inversely
	// proportional to the AGC level. (J'me comprends)
	if ( ( agc >= gnAGC ) && ( gnAGC < AGC_UNITY ) && ( gnVUMeter < ( 0xFF - ( gnAGC >> ( AGC_PRECISION - 7 ) ) ) ) )
	{
		gAGCRecoverCount += count;
		UINT agctimeout = gdwMixingFreq + gnAGC;

		if ( gnChannels >= 2 ) { agctimeout <<= 1; }

		if ( gAGCRecoverCount >= agctimeout )
		{
			gAGCRecoverCount = 0;
			gnAGC++;
		}
	}
	else
	{
		gnAGC = agc;
		gAGCRecoverCount = 0;
	}
}



void CSoundFile::ResetAGC()
//-------------------------
{
	gnAGC = AGC_UNITY;
}

#endif // NO_AGC

// end of FASTMIX

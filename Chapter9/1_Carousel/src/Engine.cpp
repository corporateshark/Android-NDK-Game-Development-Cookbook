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

#include "Wrapper_Callbacks.h"

#if defined( _WIN32 )
#  include "Wrapper_Windows.h"
#else
#  include "Wrapper_Android.h"
#endif

#include <string>

static double NewTime, OldTime, ExecutionTime;
static double RecipCyclesPerSecond = -1.0f;

#ifndef _WIN32
#include <unistd.h>
#include <sys/time.h>
#include <sys/select.h>
#include <pthread.h>

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

const unsigned usec_per_sec = 1000000;

bool QueryPerformanceFrequency( int64_t* frequency )
{
	/* gettimeofday reports to microsecond accuracy. */
	*frequency = usec_per_sec;
	return true;
}

bool QueryPerformanceCounter( int64_t* performance_count )
{
	struct timeval Time;

	gettimeofday( &Time, NULL );
	*performance_count = Time.tv_usec + Time.tv_sec * usec_per_sec;

	return true;
}
#endif

void StartTiming()
{
	/// Initialize timing
#ifndef ANDROID
	LARGE_INTEGER Freq;
#else
	int64_t Freq;
#endif

	QueryPerformanceFrequency( &Freq );

#ifndef ANDROID
	double CyclesPerSecond = static_cast<double>( Freq.QuadPart );
#else
	double CyclesPerSecond = static_cast<double>( Freq );
#endif

	RecipCyclesPerSecond = 1.0 / CyclesPerSecond;
}

double GetSeconds()
{
	if ( RecipCyclesPerSecond < 0.0f ) { StartTiming(); }

#ifndef ANDROID
	LARGE_INTEGER T1;
#else
	int64_t T1;
#endif

	QueryPerformanceCounter( &T1 );

#ifndef ANDROID
	return RecipCyclesPerSecond * ( double )( T1.QuadPart );
#else
	return RecipCyclesPerSecond * ( double )( T1 );
#endif
}

void GenerateTicks()
{
	NewTime = GetSeconds();
	float DeltaSeconds = static_cast<float>( NewTime - OldTime );
	OldTime = NewTime;

	const float TIME_QUANTUM = 0.0166666f;
	const float MAX_EXECUTION_TIME = 10.0f * TIME_QUANTUM;

	ExecutionTime += DeltaSeconds;

	if ( ExecutionTime > MAX_EXECUTION_TIME ) { ExecutionTime = MAX_EXECUTION_TIME; }

	while ( ExecutionTime > TIME_QUANTUM )
	{
		ExecutionTime -= TIME_QUANTUM;
		OnTimer( TIME_QUANTUM );
	}

	OnDrawFrame();
}

void Str_AddTrailingChar( std::string* Str, char Ch )
{
	bool HasLastChar = ( !Str->empty() ) && ( Str->data()[Str->length() - 1] == Ch );

	if ( !HasLastChar ) { Str->push_back( Ch ); }
}

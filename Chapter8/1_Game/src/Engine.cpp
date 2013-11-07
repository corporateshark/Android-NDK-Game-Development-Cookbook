#include "Wrapper_Callbacks.h"

#if defined( _WIN32 )
#  include "Wrapper_Windows.h"
#else
#  include "Wrapper_Android.h"
#endif

#include <string>

static double NewTime, OldTime, ExecutionTime;
static double RecipCyclesPerSecond = -1.0f;
static const int BUFFER = 255;

#if _MSC_VER >= 1400
#  define Lvsnprintf _vsnprintf_s
#  define Lsnprintf  _snprintf_s
#else
#  define Lvsnprintf vsnprintf
#  define Lsnprintf  snprintf
#endif

#include <time.h>

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
	if ( ( !Str->empty() ) && ( Str->data()[Str->length() - 1] == Ch ) ) { return; }

	Str->push_back( Ch );
}

std::string Str_ReplaceAllSubStr( const std::string& Str, const std::string& OldSubStr, const std::string& NewSubStr )
{
	std::string Result = Str;

	for ( size_t Pos = Result.find( OldSubStr ); Pos != std::string::npos; Pos = Result.find( OldSubStr ) )
	{
		Result.replace( Pos, OldSubStr.length(), NewSubStr );
	}

	return Result;
}

void Str_PadLeft( std::string* Str, size_t Len, char Pad )
{
	while ( Str->length() < Len )
	{
		*Str = Pad + *Str;
	}
}

std::string Str_GetPadLeft( const std::string& Str, size_t Len, char Pad )
{
	std::string TempStr( Str );

	Str_PadLeft( &TempStr, Len, Pad );

	return TempStr;
}

std::string Str_GetFormatted( const char* Pattern, ... )
{
	char buf[BUFFER];

	va_list p;
	va_start( p, Pattern );

	Lvsnprintf( buf, BUFFER - 1, Pattern, p );

	va_end( p );

	return std::string( buf );
}

std::string Str_ToStr( int i )
{
	char buf[BUFFER];

	Lsnprintf( buf, BUFFER - 1, "%i", i );

	return std::string( buf );
}

void Env_Sleep( int Milliseconds )
{
#if defined _WIN32
	Sleep( Milliseconds );
#else
	// mu-sleep supports microsecond-precision
	usleep( static_cast<useconds_t>( Milliseconds ) * 1000 );
#endif
}

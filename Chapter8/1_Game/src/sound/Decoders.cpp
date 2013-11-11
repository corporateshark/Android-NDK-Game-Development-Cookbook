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

#include "Decoders.h"

#if defined(ANDROID)
// static link
#define OGG_ov_clear           ov_clear
#define OGG_ov_open_callbacks  ov_open_callbacks
#define OGG_ov_time_seek       ov_time_seek
#define OGG_ov_info            ov_info
#define OGG_ov_comment         ov_comment
#define OGG_ov_read            ov_read

#else

#include <windows.h>

HMODULE g_OGGLibrary = NULL;

ModPlug_Load_func ModPlug_Load_P;
ModPlug_Unload_func ModPlug_Unload_P;
ModPlug_Read_func ModPlug_Read_P;
ModPlug_Seek_func ModPlug_Seek_P;
ModPlug_GetSettings_func ModPlug_GetSettings_P;
ModPlug_SetSettings_func ModPlug_SetSettings_P;

HMODULE g_ModPlugLibrary = NULL;

ov_clear_func          OGG_ov_clear = NULL;
ov_open_callbacks_func OGG_ov_open_callbacks = NULL;
ov_time_seek_func      OGG_ov_time_seek = NULL;
ov_info_func           OGG_ov_info = NULL;
ov_comment_func        OGG_ov_comment = NULL;
ov_read_func           OGG_ov_read = NULL;
#endif // OGG_DYNAMIC_LINK

void LoadOGG()
{
#if defined(_WIN32)

	// exit if the OGG library is already loaded
	if ( g_OGGLibrary ) { return; }

	// _d.dll for DEBUG
	g_OGGLibrary = LoadLibrary( "vorbisfile.dll" );
	OGG_ov_read           = ( ov_read_func )GetProcAddress( g_OGGLibrary, "ov_read" );
	OGG_ov_info           = ( ov_info_func )GetProcAddress( g_OGGLibrary, "ov_info" );
	OGG_ov_comment        = ( ov_comment_func )GetProcAddress( g_OGGLibrary, "ov_comment" );
	OGG_ov_time_seek      = ( ov_time_seek_func )GetProcAddress( g_OGGLibrary, "ov_time_seek" );
	OGG_ov_open_callbacks = ( ov_open_callbacks_func )GetProcAddress( g_OGGLibrary, "ov_open_callbacks" );
	OGG_ov_clear          = ( ov_clear_func )GetProcAddress( g_OGGLibrary, "ov_clear" );
#endif // OGG_DYNAMIC_LINK
}

void UnloadOGG()
{
#if !defined( ANDROID)
	CloseHandle( g_OGGLibrary );
#endif
}

/// Default modplug options
void InitModPlug()
{
	ModPlug_Settings Settings;
	ModPlug_GetSettings_P( &Settings );

	// all "basic settings" are set before ModPlug_Load.
	Settings.mResamplingMode = MODPLUG_RESAMPLE_FIR; /* RESAMP */
	Settings.mChannels = 2;
	Settings.mBits = 16;
	Settings.mFrequency = 44100;
	Settings.mStereoSeparation = 128;
	Settings.mMaxMixChannels = 256;

	ModPlug_SetSettings_P( &Settings );
}

bool LoadModPlug()
{
#if defined( _WIN32 )
	g_ModPlugLibrary = LoadLibrary( "modplug.dll" );

	if ( !g_ModPlugLibrary ) { return false; }

#define GetModProc(Name) \
   Name##_P = (Name##_func)GetProcAddress(g_ModPlugLibrary, #Name); \
   if(!(Name##_P)) return false;

	GetModProc( ModPlug_Load );
	GetModProc( ModPlug_Unload );
	GetModProc( ModPlug_Read );
	GetModProc( ModPlug_Seek );
	GetModProc( ModPlug_GetSettings );
	GetModProc( ModPlug_SetSettings );
#endif
	InitModPlug();
	return true;
}

bool UnloadModPlug()
{
#if defined( _WIN32 )
	CloseHandle( g_ModPlugLibrary );
#endif
	return true;
}

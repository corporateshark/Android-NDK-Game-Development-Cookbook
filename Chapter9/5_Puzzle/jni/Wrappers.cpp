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

#include <stdlib.h>
#include <jni.h>
#include <android/log.h>
#include "Multitouch.h"
#include <LGLAPI.h>
#include <LGL.h>

#include "Wrapper_Callbacks.h"

#include <string>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "Game1", __VA_ARGS__))

std::string g_ExternalStorage;
std::string g_APKName;

int g_Width = 1;
int g_Height = 1;

extern sLGLAPI* LGL3;

std::string ConvertJString( JNIEnv* env, jstring str )
{
	if ( !str ) { return std::string(); }

	const jsize len = env->GetStringUTFLength( str );
	const char* strChars = env->GetStringUTFChars( str, ( jboolean* )0 );

	std::string Result( strChars, len );

	env->ReleaseStringUTFChars( str, strChars );

	return Result;
}

struct sSendMotionData
{
	int ContactID;
	eMotionFlag Flag;
	LVector2 Pos;
	bool Pressed;
};

struct sKeyData
{
	int Code;
	bool Pressed;
};

clMutex g_MotionEventsQueueMutex;
std::vector<sSendMotionData> g_MotionEventsQueue;
std::vector<sKeyData> g_KeysQueue;

void GestureHandler_SendMotion( int ContactID, eMotionFlag Flag, LVector2 Pos, bool Pressed );

extern "C"
{
	JNIEXPORT void JNICALL Java_com_packtpub_ndkcookbook_game1_Game1Activity_OnCreateNative( JNIEnv* env, jobject obj, jstring ExternalStorage )
	{
		g_ExternalStorage = ConvertJString( env, ExternalStorage );
	}

	JNIEXPORT void JNICALL Java_com_packtpub_ndkcookbook_game1_Game1Activity_SetAPKName( JNIEnv* env, jobject obj, jstring APKName )
	{
		g_APKName = ConvertJString( env, APKName );

		LOGI( "APKName = %s", g_APKName.c_str() );
	}

	JNIEXPORT void JNICALL Java_com_packtpub_ndkcookbook_game1_Game1Activity_SendKey( JNIEnv* env, jobject obj, int KeyCode, bool Pressed )
	{
		sKeyData K;

		K.Code = KeyCode;
		K.Pressed = Pressed;

		LMutex Lock( &g_MotionEventsQueueMutex );
		g_KeysQueue.push_back( K );
	}

	JNIEXPORT void JNICALL Java_com_packtpub_ndkcookbook_game1_Game1Activity_SendMotion( JNIEnv* env, jobject obj, int PointerID, int x, int y, bool Pressed, int Flag )
	{
		sSendMotionData M;
		M.ContactID = PointerID;
		M.Flag = ( eMotionFlag )Flag;
		M.Pos = LVector2( ( float )x / ( float )g_Width, ( float )y / ( float )g_Height );
		M.Pressed = Pressed;

		LMutex Lock( &g_MotionEventsQueueMutex );
		g_MotionEventsQueue.push_back( M );
	}

	JNIEXPORT void JNICALL Java_com_packtpub_ndkcookbook_game1_Game1Activity_SetSurface( JNIEnv* env, jclass clazz, jobject javaSurface )
	{
		if ( LGL3 ) { delete( LGL3 ); }

		LGL3 = new sLGLAPI;

		LGL::clGLExtRetriever* OpenGL;
		OpenGL = new LGL::clGLExtRetriever;
		OpenGL->Reload( LGL3 );
		delete( OpenGL );

		OnStart( g_APKName );
	}

	JNIEXPORT void JNICALL Java_com_packtpub_ndkcookbook_game1_Game1Activity_SetSurfaceSize( JNIEnv* env, jclass clazz, int Width, int Height )
	{
		LOGI( "SurfaceSize: %i x %i", Width, Height );

		g_Width  = Width;
		g_Height = Height;
	}

	JNIEXPORT void JNICALL Java_com_packtpub_ndkcookbook_game1_Game1Activity_DrawFrame( JNIEnv* env, jobject obj )
	{
		// send events synchronously
		{
			LMutex Lock( &g_MotionEventsQueueMutex );

			for ( auto m : g_MotionEventsQueue )
			{
				GestureHandler_SendMotion( m.ContactID, m.Flag, m.Pos, m.Pressed );
			}

			for ( auto m : g_KeysQueue )
			{
				OnKey( m.Code, m.Pressed );
			}

			g_MotionEventsQueue.clear();
			g_KeysQueue.clear();
		}

		GenerateTicks();
	}

	JNIEXPORT void JNICALL Java_com_packtpub_ndkcookbook_game1_Game1Activity_ExitNative( JNIEnv* env, jobject obj )
	{
		OnStop();

		exit( 0 );
	}

} // extern "C"

void ExitApp()
{
	OnStop();

	exit( 0 );
}

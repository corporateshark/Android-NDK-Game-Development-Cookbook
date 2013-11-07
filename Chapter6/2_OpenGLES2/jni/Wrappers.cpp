#include <stdlib.h>
#include <jni.h>
#include <android/log.h>
#include <GLES2/gl2.h>
#include <LGLAPI.h>
#include <LGL.h>

#include "Wrapper_Callbacks.h"

#include <string>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "App13", __VA_ARGS__))

std::string g_ExternalStorage;
std::string g_APKName;

int g_Width = 0;
int g_Height = 0;

extern sLGLAPI* LGL3;

std::string ConvertJString( JNIEnv* env, jstring str )
{
	if ( !str ) { std::string(); }

	const jsize len = env->GetStringUTFLength( str );
	const char* strChars = env->GetStringUTFChars( str, ( jboolean* )0 );

	std::string Result( strChars, len );

	env->ReleaseStringUTFChars( str, strChars );

	return Result;
}

extern "C"
{
	JNIEXPORT void JNICALL Java_com_packtpub_ndkcookbook_app13_App13Activity_OnCreateNative( JNIEnv* env, jobject obj, jstring ExternalStorage )
	{
		g_ExternalStorage = ConvertJString( env, ExternalStorage );

		OnStart( g_APKName );
	}

	JNIEXPORT void JNICALL Java_com_packtpub_ndkcookbook_app13_App13Activity_SetAPKName( JNIEnv* env, jobject obj, jstring APKName )
	{
		g_APKName = ConvertJString( env, APKName );

		LOGI( "APKName = %s", g_APKName.c_str() );
	}

	JNIEXPORT void JNICALL Java_com_packtpub_ndkcookbook_app13_App13Activity_SetSurface( JNIEnv* env, jclass clazz, jobject javaSurface )
	{
		if ( LGL3 ) { delete( LGL3 ); }

		LGL3 = new sLGLAPI;

		LGL::clGLExtRetriever* OpenGL;
		OpenGL = new LGL::clGLExtRetriever;
		OpenGL->Reload( LGL3 );
		delete( OpenGL );
	}

	JNIEXPORT void JNICALL Java_com_packtpub_ndkcookbook_app13_App13Activity_SetSurfaceSize( JNIEnv* env, jclass clazz, int Width, int Height )
	{
		LOGI( "SurfaceSize: %i x %i", Width, Height );

		g_Width  = Width;
		g_Height = Height;
	}

	JNIEXPORT void JNICALL Java_com_packtpub_ndkcookbook_app13_App13Activity_DrawFrame( JNIEnv* env, jobject obj )
	{
		// call our callback
		OnDrawFrame();
	}

} // extern "C"

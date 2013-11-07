#include <stdlib.h>
#include <jni.h>
#include <android/log.h>
#include <GLES2/gl2.h>

#include "Wrapper_Callbacks.h"

#include <string>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "App9", __VA_ARGS__))

extern std::string g_ExternalStorage;
std::string g_APKName;

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
	JNIEXPORT void JNICALL Java_com_packtpub_ndkcookbook_app9_App9Activity_OnCreateNative( JNIEnv* env, jobject obj, jstring ExternalStorage )
	{
		g_ExternalStorage = ConvertJString( env, ExternalStorage );

		OnStart( g_APKName );
	}

	JNIEXPORT void JNICALL Java_com_packtpub_ndkcookbook_app9_App9Activity_SetAPKName( JNIEnv* env, jobject obj, jstring APKName )
	{
		g_APKName = ConvertJString( env, APKName );

		LOGI( "APKName = %s", g_APKName.c_str() );
	}

} // extern "C"

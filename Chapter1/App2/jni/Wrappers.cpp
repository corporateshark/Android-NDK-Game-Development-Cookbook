#include <stdlib.h>
#include <jni.h>
#include <android/log.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "App2", __VA_ARGS__))

extern "C"
{

	JNIEXPORT void JNICALL Java_com_packtpub_ndkcookbook_app2_App2Activity_onCreateNative( JNIEnv* env, jobject obj )
	{
		LOGI( "Hello World!" );
	}

}

#include <stdlib.h>
#include <jni.h>

JNIEnv* GetEnv();

/// global pointer to Java VM (also shared with OpenAL implementation)
JavaVM* javaVM = NULL;

//jclass Class_JNILib;

jint JNI_OnLoad( JavaVM* vm, void* reserved )
{
	javaVM = vm;
	JNIEnv* env = GetEnv();

//	Class_JNILib = (*env)->FindClass( env, "com/linderdaum/engine/LinderdaumJNILib" );
//	Class_JNILib = (*env)->NewGlobalRef( env, Class_JNILib );

	return JNI_VERSION_1_2;
}

JNIEnv* GetEnv()
{
	JNIEnv* env = NULL;

	if ( javaVM ) { ( *javaVM )->GetEnv( javaVM, ( void** )&env, JNI_VERSION_1_2 ); }

	return env;
}

JNIEnv* GetEnvThread()
{
	JNIEnv* env = NULL;

	if ( javaVM ) { ( *javaVM )->AttachCurrentThread( javaVM, &env, NULL ); }

	return env;
}

void JavaEnter( JNIEnv** env )
{
	*env = GetEnv();

	( **env )->PushLocalFrame( *env, 4 );
}

void JavaLeave( JNIEnv* env )
{
	( *env )->PopLocalFrame( env, NULL );
}

jmethodID FindJavaStaticMethod( JNIEnv* env, jclass* Class, jmethodID* Method, const char* ClassName, const char* MethodName, const char* MethodSignature )
{
	*Class  = ( *env )->FindClass( env, ClassName );
	*Method = ( *env )->GetStaticMethodID( env, *Class, MethodName, MethodSignature );

	return *Method;
}


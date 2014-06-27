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
#include <GLES2/gl2.h>
#include "Wrapper_Android.h"

extern std::string g_ExternalStorage;
std::string g_APKName;
extern std::string g_LocaleName;

static const char g_vShaderStr[] =
   "#version 100\n"
   "precision highp float;\n"
   "attribute vec3 vPosition;\n"
   "attribute vec3 vCoords;\n"
   "varying vec2 Coords;\n"
   "void main()\n"
   "{\n"
   "   Coords = vCoords.xy;\n"
   "   gl_Position = vec4( vPosition, 1.0 );\n"
   "}\n";

static const char g_fShaderStr[] =
   "#version 100\n"
   "precision highp float;\n"
   "varying vec2 Coords;\n"
   "uniform sampler2D Texture0;\n"
   "void main()\n"
   "{\n"
   "   gl_FragColor = texture2D( Texture0, Coords );\n"
   "}\n";

static GLuint g_ProgramObject = 0;
static GLuint g_Texture = 0;

extern unsigned char* g_FrameBuffer;

int g_Width = 1;
int g_Height = 1;

static GLuint LoadShader( GLenum type, const char* shaderSrc )
{
	GLuint shader = glCreateShader( type );

	glShaderSource ( shader, 1, &shaderSrc, NULL );

	glCompileShader ( shader );

	GLint compiled;

	glGetShaderiv ( shader, GL_COMPILE_STATUS, &compiled );

	GLsizei MaxLength = 0;

	glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &MaxLength );

	char* InfoLog = new char[MaxLength];

	glGetShaderInfoLog( shader, MaxLength, &MaxLength, InfoLog );

	LOGI( "Shader info log: %s\n", InfoLog );

	return shader;
}

static void GLDebug_LoadStaticProgramObject()
{
	if ( g_ProgramObject == 0 )
	{
		GLuint vertexShader = LoadShader ( GL_VERTEX_SHADER, g_vShaderStr );
		GLuint fragmentShader = LoadShader ( GL_FRAGMENT_SHADER, g_fShaderStr );

		// Create the program object
		g_ProgramObject = glCreateProgram ( );

		glAttachShader ( g_ProgramObject, vertexShader );
		glAttachShader ( g_ProgramObject, fragmentShader );

		// Link the program
		glLinkProgram ( g_ProgramObject );

		GLint linked;

		// Check the link status
		glGetProgramiv ( g_ProgramObject, GL_LINK_STATUS, &linked );

		GLsizei Length    = 0;
		GLsizei MaxLength = 0;

		glGetProgramiv( g_ProgramObject, GL_INFO_LOG_LENGTH, &MaxLength );

		char* InfoLog = new char[MaxLength];

		glGetProgramInfoLog( g_ProgramObject, MaxLength, &Length, InfoLog );

		LOGI( "InfoLog: %s", InfoLog );

		int Texture = glGetUniformLocation( g_ProgramObject, "Texture0" );

		glUseProgram ( g_ProgramObject );
		glUniform1i( Texture, 0 );
	}
}

static void GLDebug_RenderTriangle()
{
	const GLfloat vVertices[] = { -1.0f, -1.0f, 0.0f,
	                              -1.0f,  1.0f, 0.0f,
	                              1.0f, -1.0f, 0.0f,
	                              -1.0f,  1.0f, 0.0f,
	                              1.0f, -1.0f, 0.0f,
	                              1.0f,  1.0f, 0.0f
	                            };

	const GLfloat vCoords[]   = {  0.0f,  0.0f, 0.0f,
	                               0.0f,  1.0f, 0.0f,
	                               1.0f,  0.0f, 0.0f,
	                               0.0f,  1.0f, 0.0f,
	                               1.0f,  0.0f, 0.0f,
	                               1.0f,  1.0f, 0.0f
	                            };

	glUseProgram ( g_ProgramObject );
	GLint Loc1 = glGetAttribLocation( g_ProgramObject, "vPosition" );
	GLint Loc2 = glGetAttribLocation( g_ProgramObject, "vCoords" );

	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
	glVertexAttribPointer ( Loc1, 3, GL_FLOAT, GL_FALSE, 0, vVertices );
	glVertexAttribPointer ( Loc2, 3, GL_FLOAT, GL_FALSE, 0, vCoords   );
	glEnableVertexAttribArray ( Loc1 );
	glEnableVertexAttribArray ( Loc2 );

	//glUseProgram ( g_ProgramObject );
	glDisable( GL_DEPTH_TEST );
	glDrawArrays ( GL_TRIANGLES, 0, 6 );
	glUseProgram ( 0 );
	glDisableVertexAttribArray ( Loc1 );
	glDisableVertexAttribArray ( Loc2 );
}

std::string ConvertJString( JNIEnv* env, jstring str )
{
	if ( !str ) { return std::string(); }

	const jsize len = env->GetStringUTFLength( str );
	const char* strChars = env->GetStringUTFChars( str, ( jboolean* )0 );

	std::string Result( strChars, len );

	env->ReleaseStringUTFChars( str, strChars );

	return Result;
}

extern "C"
{
	JNIEXPORT void JNICALL Java_com_packtpub_ndkcookbook_app14_App14Activity_OnCreateNative( JNIEnv* env, jobject obj, jstring ExternalStorage )
	{
		g_ExternalStorage = ConvertJString( env, ExternalStorage );
	}

	JNIEXPORT void JNICALL Java_com_packtpub_ndkcookbook_app14_App14Activity_SetAPKName( JNIEnv* env, jobject obj, jstring APKName )
	{
		g_APKName = ConvertJString( env, APKName );
		LOGI( "APKName = %s", g_APKName.c_str() );
	}

	JNIEXPORT void JNICALL Java_com_packtpub_ndkcookbook_app14_App14Activity_SetLocaleName( JNIEnv* env, jobject obj, jstring LocaleName )
	{
		g_LocaleName = ConvertJString( env, LocaleName );
		LOGI( "LocaleName = %s", g_LocaleName.c_str() );
	}

	JNIEXPORT void JNICALL Java_com_packtpub_ndkcookbook_app14_App14Activity_SetSurface( JNIEnv* env, jclass clazz, jobject javaSurface )
	{
		OnStart( g_APKName );
	}

	JNIEXPORT void JNICALL Java_com_packtpub_ndkcookbook_app14_App14Activity_SetSurfaceSize( JNIEnv* env, jclass clazz, int Width, int Height )
	{
		LOGI( "SurfaceSize: %i x %i", Width, Height );

		g_Width = Width;
		g_Height = Height;

		GLDebug_LoadStaticProgramObject();

		glGenTextures( 1, &g_Texture );
		glBindTexture( GL_TEXTURE_2D, g_Texture );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, ImageWidth, ImageHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, g_FrameBuffer );
	}

	JNIEXPORT void JNICALL Java_com_packtpub_ndkcookbook_app14_App14Activity_DrawFrame( JNIEnv* env, jobject obj )
	{
		OnDrawFrame();

		glActiveTexture( GL_TEXTURE0 );
		glBindTexture( GL_TEXTURE_2D, g_Texture );
		glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, ImageWidth, ImageHeight, GL_RGBA, GL_UNSIGNED_BYTE, g_FrameBuffer );

		// render on screen
		GLDebug_RenderTriangle();
	}

} // extern "C"

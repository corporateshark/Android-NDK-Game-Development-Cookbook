/*
 * Copyright (C) 2013 Sergey Kosarevsky (sk@linderdaum.com)
 * Copyright (C) 2013 Viktor Latypov (vl@linderdaum.com)
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

#include "Engine.h"

sLGLAPI* LGL3 = NULL;

clPtr<clVertexAttribs> Attribs;
clPtr<clGLVertexArray> VA;
clPtr<clGLSLShaderProgram> SP;
clPtr<clCanvas> Canvas;
clPtr<clGLTexture> Texture;

clPtr<FileSystem> g_FS;

static const char g_vShaderStr[] =
   "uniform mat4 in_ModelViewProjectionMatrix;\n"
   "in vec4 in_Vertex;\n"
   "in vec2 in_TexCoord;\n"
   "out vec2 Coords;\n"
   "void main()\n"
   "{\n"
   "   Coords = in_TexCoord;\n"
   "   gl_Position = in_ModelViewProjectionMatrix * in_Vertex;\n"
   "}\n";

static const char g_fShaderStr[] =
   "in vec2 Coords;\n"
   "out vec4 out_FragColor;\n"
   "void main()\n"
   "{\n"
   "   out_FragColor = vec4( Coords, 0.0, 1.0 );\n"
   "}\n";

void OnDrawFrame()
{
	static float Angle = 0;

	Angle += 0.02f;

	LGL3->glClearColor( 0.3f, 0.0f, 0.0f, 0.0f );
	LGL3->glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	LGL3->glEnable( GL_DEPTH_TEST );

	LMatrix4 Proj = Linderdaum::Math::Perspective( 45.0f, 1280.0f / 720.0f, 0.4f, 2000.0f );
	LMatrix4 MV = LMatrix4::GetRotateMatrixAxis( Angle, LVector3( 1, 1, 1 ) ) * LMatrix4::GetTranslateMatrix( LVector3( 0, 0, -5 ) );

	SP->Bind();
	SP->SetUniformNameMat4Array( "in_ModelViewProjectionMatrix", 1, MV * Proj );

	VA->Draw( false );

	Canvas->Rect2D( 0.25f, 0.25f, 0.75f, 0.75f, LVector4( 1.0f, 0.0f, 1.0f, 0.5f ) );

	Canvas->TexturedRect2D( 0.50f, 0.50f, 0.85f, 0.90f, LVector4( 1.0f, 1.0f, 1.0f, 1.0f ), Texture );
}

void OnStart( const std::string& RootPath )
{
	g_FS = new FileSystem();
	g_FS->Mount( "." );
#if defined(ANDROID)
	g_FS->Mount( RootPath );
	g_FS->AddAliasMountPoint( RootPath, "assets" );
#endif

	Canvas = new clCanvas();

	Attribs = clGeomServ::CreateAxisAlignedBox( LVector3( -1.0f ), LVector3( 1.0f ) );

	VA = new clGLVertexArray();
	VA->SetVertexAttribs( Attribs );

	SP = new clGLSLShaderProgram( g_vShaderStr, g_fShaderStr );

	clPtr<clBitmap> Bmp = clBitmap::LoadImg( g_FS->CreateReader( "test.bmp" ) );

	Texture = new clGLTexture();
	Texture->LoadFromBitmap( Bmp );
}

void OnKeyUp( int code )
{
}

void OnKeyDown( int code )
{
}

void OnMouseDown( int btn, int x, int y )
{
}

void OnMouseMove( int x, int y )
{
}

void OnMouseUp( int btn, int x, int y )
{
}

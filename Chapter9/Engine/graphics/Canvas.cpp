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

#include "Canvas.h"
#include "Geometry.h"
#include "GLClasses.h"
#include "LGL/LGLAPI.h"
#include "TextRenderer.h"

extern sLGLAPI* LGL3;

static const char RectvShaderStr[] =
   "uniform vec4 u_RectSize;\n"
   "uniform vec4 u_ClipRect;\n"
   "in vec4 in_Vertex;\n"
   "in vec2 in_TexCoord;\n"
   "out vec2 Coords;\n"
   "void main()\n"
   "{\n"
   "   Coords = in_TexCoord;\n"
   "   float X1 = u_RectSize.x;\n"
   "   float Y1 = u_RectSize.y;\n"
   "   float X2 = u_RectSize.z;\n"
   "   float Y2 = u_RectSize.w;\n"
   "   float Width  = X2 - X1;\n"
   "   float Height = Y2 - Y1;\n"
   "   float U0 = u_ClipRect.x;\n"
   "   float V0 = u_ClipRect.y;\n"
   "   float U1 = u_ClipRect.z;\n"
   "   float V1 = u_ClipRect.w;\n"
   "   float u = ( 1.0 - in_Vertex.x ) * U0 + ( in_Vertex.x ) * U1;\n"
   "   float v = ( 1.0 - in_Vertex.y ) * V0 + ( in_Vertex.y ) * V1;\n"
   "   vec4 VertexPos = vec4( X1 + in_Vertex.x * Width, Y1 + in_Vertex.y * Height, in_Vertex.z, in_Vertex.w ) * vec4( 2.0, -2.0, 1.0, 1.0 ) + vec4( -1.0, 1.0, 0.0, 0.0 );\n"
   "	 Coords = vec2( u, v );\n"
   "   gl_Position = VertexPos;\n"
   "}\n";

static const char RectfShaderStr[] =
   "uniform vec4 u_Color;\n"
   "out vec4 out_FragColor;\n"
   "in vec2 Coords;\n"
   "void main()\n"
   "{\n"
   "   out_FragColor = u_Color;\n"
   "}\n";

static const char TexRectfShaderStr[] =
   "uniform vec4 u_Color;\n"
   "uniform vec4 u_Tiles;\n"
   "uniform vec4 u_ClipRect;\n"
   "out vec4 out_FragColor;\n"
   "in vec2 Coords;\n"
   "uniform sampler2D Texture0;\n"
   "void main()\n"
   "{\n"
   "   out_FragColor = u_Color * texture( Texture0, Coords * u_Tiles.xy );\n"
   "}\n";

static const char Rect3DvShaderStr[] =
   "uniform mat4 u_MVP;\n"
   "in vec4 in_Vertex;\n"
   "in vec2 in_TexCoord;\n"
   "out vec2 Coords;\n"
   "void main()\n"
   "{\n"
   "   Coords = in_TexCoord;\n"
   "   gl_Position = u_MVP * in_Vertex;\n"
   "}\n";

static const char Rect3DfShaderStr[] =
   "out vec4 out_FragColor;\n"
   "in vec2 Coords;\n"
   "uniform sampler2D Texture0;\n"
   "void main()\n"
   "{\n"
   "   out_FragColor = texture( Texture0, Coords );\n"
   "}\n";

clCanvas::clCanvas()
{
	FRect = clGeomServ::CreateRect2D( 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, false, 1 );
	FRect3D = clGeomServ::CreateRect2D( 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, false, 1 );

	FRectVA = new clGLVertexArray();
	FRectVA->SetVertexAttribs( FRect );

	FRect3DVA = new clGLVertexArray();
	FRect3DVA->SetVertexAttribs( FRect3D );

	FRectSP = new clGLSLShaderProgram( RectvShaderStr, RectfShaderStr );
	FTexRectSP = new clGLSLShaderProgram( RectvShaderStr, TexRectfShaderStr );
	FRect3DSP = new clGLSLShaderProgram( Rect3DvShaderStr, Rect3DfShaderStr );
}

void clCanvas::Rect2D( float X1, float Y1, float X2, float Y2, const LVector4& Color )
{
	LGL3->glDisable( GL_DEPTH_TEST );

	FRectSP->Bind();
	FRectSP->SetUniformNameVec4Array( "u_Color", 1, Color );
	FRectSP->SetUniformNameVec4Array( "u_RectSize", 1, LVector4( X1, Y1, X2, Y2 ) );

	if ( Color.w < 1.0f )
	{
		LGL3->glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		LGL3->glEnable( GL_BLEND );
	}

	FRectVA->Draw( false );

	if ( Color.w < 1.0f )
	{
		LGL3->glDisable( GL_BLEND );
	}
}

void clCanvas::Rect3D( const LMatrix4& Proj, const LMatrix4& MV,
                       const LVector3& V1,
                       const LVector3& V2,
                       const LVector3& V3,
                       const LVector3& V4,
                       const clPtr<clGLTexture>& Texture,
                       const clPtr<clGLSLShaderProgram>& SP )
{
	LGL3->glDisable( GL_DEPTH_TEST );

	Texture->Bind( 0 );

	clPtr<clGLSLShaderProgram> S = SP ? SP : FRect3DSP;

	S->Bind();
	S->SetUniformNameMat4Array( "u_MVP", 1, MV * Proj );

	FRect3D->Restart( 6 );
	FRect3D->SetTexCoordV( LVector2( 0, 0 ) );
	FRect3D->EmitVertexV( V1 );
	FRect3D->SetTexCoordV( LVector2( 1, 0 ) );
	FRect3D->EmitVertexV( V2 );
	FRect3D->SetTexCoordV( LVector2( 0, 1 ) );
	FRect3D->EmitVertexV( V4 );

	FRect3D->SetTexCoordV( LVector2( 1, 0 ) );
	FRect3D->EmitVertexV( V2 );
	FRect3D->SetTexCoordV( LVector2( 1, 1 ) );
	FRect3D->EmitVertexV( V3 );
	FRect3D->SetTexCoordV( LVector2( 0, 1 ) );
	FRect3D->EmitVertexV( V4 );
	FRect3DVA->SetVertexAttribs( FRect3D );

	FRect3DVA->Draw( false );
}

void clCanvas::TexturedRect2D( float X1, float Y1, float X2, float Y2, const LVector4& Color, const clPtr<clGLTexture>& Texture )
{
	TexturedRect2DTiled( X1, Y1, X2, Y2, 1, 1, Color, Texture );
}

void clCanvas::TexturedRect2DTiled( float X1, float Y1, float X2, float Y2, int TilesX, int TilesY, const LVector4& Color, const clPtr<clGLTexture>& Texture )
{
	LGL3->glDisable( GL_DEPTH_TEST );

	Texture->Bind( 0 );

	FTexRectSP->Bind();
	FTexRectSP->SetUniformNameVec4Array( "u_Color", 1, Color );
	FTexRectSP->SetUniformNameVec4Array( "u_RectSize", 1, LVector4( X1, Y1, X2, Y2 ) );
	FTexRectSP->SetUniformNameVec4Array( "u_Tiles", 1, LVector4( TilesX, TilesY, 0, 0 ) );
	FTexRectSP->SetUniformNameVec4Array( "u_ClipRect", 1, LVector4( 0, 0, 1, 1 ) );

	LGL3->glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	LGL3->glEnable( GL_BLEND );

	FRectVA->Draw( false );

	LGL3->glDisable( GL_BLEND );
}

void clCanvas::TexturedRect2DClipped( float X1, float Y1, float X2, float Y2, const LVector4& Color, const clPtr<clGLTexture>& Texture, const LVector4& ClipRect )
{
	LGL3->glDisable( GL_DEPTH_TEST );

	Texture->Bind( 0 );

	FTexRectSP->Bind();
	FTexRectSP->SetUniformNameVec4Array( "u_Color", 1, Color );
	FTexRectSP->SetUniformNameVec4Array( "u_RectSize", 1, LVector4( X1, Y1, X2, Y2 ) );
	FTexRectSP->SetUniformNameVec4Array( "u_Tiles", 1, LVector4( 1, 1, 0, 0 ) );
	FTexRectSP->SetUniformNameVec4Array( "u_ClipRect", 1, ClipRect );

	LGL3->glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	LGL3->glEnable( GL_BLEND );

	FRectVA->Draw( false );

	LGL3->glDisable( GL_BLEND );
}

void clCanvas::TextStr( float X1, float Y1, float X2, float Y2, const std::string& Str, int Size, const LVector4& Color, const clPtr<clTextRenderer>& TR, int FontID )
{
	clPtr<clBitmap> B = TR->RenderTextWithFont( Str, FontID, Size, 0xFFFFFFFF, true );
	clPtr<clGLTexture> Texture = new clGLTexture();
	Texture->LoadFromBitmap( B );
	this->TexturedRect2D( X1, Y1, X2, Y2, Color, Texture );

	// make sure we draw everything before detroying the texture
	LGL3->glFinish();
}

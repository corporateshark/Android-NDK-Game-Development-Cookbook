#include "Canvas.h"
#include "GLVertexArray.h"
#include "VertexAttribs.h"
#include "GLSL.h"
#include "GLTexture.h"
#include "GeomServ.h"

extern sLGLAPI* LGL3;

static const char RectvShaderStr[] =
   "uniform vec4 u_RectSize;\n"
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
   "   vec4 VertexPos = vec4( X1 + in_Vertex.x * Width, Y1 + in_Vertex.y * Height, in_Vertex.z, in_Vertex.w ) * vec4( 2.0, -2.0, 1.0, 1.0 ) + vec4( -1.0, 1.0, 0.0, 0.0 );\n"
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
   "out vec4 out_FragColor;\n"
   "in vec2 Coords;\n"
   "uniform sampler2D Texture0;\n"
   "void main()\n"
   "{\n"
   "   out_FragColor = u_Color * texture( Texture0, Coords );\n"
   "}\n";

clCanvas::clCanvas()
{
	FRect = clGeomServ::CreateRect2D( 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, false, 1 );

	FRectVA = new clGLVertexArray();
	FRectVA->SetVertexAttribs( FRect );

	FRectSP = new clGLSLShaderProgram( RectvShaderStr, RectfShaderStr );
	FTexRectSP = new clGLSLShaderProgram( RectvShaderStr, TexRectfShaderStr );
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

void clCanvas::TexturedRect2D( float X1, float Y1, float X2, float Y2, const LVector4& Color, const clPtr<clGLTexture>& Texture )
{
	LGL3->glDisable( GL_DEPTH_TEST );

	Texture->Bind( 0 );

	FTexRectSP->Bind();
	FTexRectSP->SetUniformNameVec4Array( "u_Color", 1, Color );
	FTexRectSP->SetUniformNameVec4Array( "u_RectSize", 1, LVector4( X1, Y1, X2, Y2 ) );

	LGL3->glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	LGL3->glEnable( GL_BLEND );

	FRectVA->Draw( false );

	LGL3->glDisable( GL_BLEND );
}

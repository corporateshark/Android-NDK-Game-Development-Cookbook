#include "Engine.h"

sLGLAPI* LGL3 = NULL;

clPtr<clVertexAttribs> Attribs;
clPtr<clGLVertexArray> VA;
clPtr<clGLSLShaderProgram> SP;

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
   "uniform sampler2D Texture0;\n"
   "out vec4 out_FragColor;\n"
   "void main()\n"
   "{\n"
   "   out_FragColor = vec4( Coords, 0.0, 1.0 ); //texture( Texture0, Coords );\n"
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
}

void OnStart( const std::string& RootPath )
{
	Attribs = clGeomServ::CreateAxisAlignedBox( LVector3( -1.0f ), LVector3( 1.0f ) );

	VA = new clGLVertexArray();
	VA->SetVertexAttribs( Attribs );

	SP = new clGLSLShaderProgram( g_vShaderStr, g_fShaderStr );
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

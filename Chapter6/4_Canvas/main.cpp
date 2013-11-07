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

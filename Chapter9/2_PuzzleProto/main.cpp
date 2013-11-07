#include "Engine.h"

#include "Game.h"

static int TileColors[] = { 0xFFFFFF, 0xFF00FF, 0xFF0000, 0x00FF00, 0x0000FF, 0xFFFF00, 0x00FFFF, 0xAA00AA,
                            0xAA0000, 0x00AA00, 0x0000AA, 0xAAAA00, 0x00AAAA, 0xCCAACC, 0xAACCAA, 0xCCCCAA
                          };
LVector2 g_Pos;

vec2 Env_GetMouse() { return g_Pos; }

Puzzle g_Game;

sLGLAPI* LGL3 = NULL;

clPtr<clCanvas> g_Canvas;
clPtr<clFileSystem> g_FS;
clPtr<clTextRenderer > g_TextRenderer;
int g_Font;

clAudioThread g_Audio;

clPtr<clGLSLShaderProgram> BackShader;
clPtr<clGLTexture> g_Texture;

void DrawTile( Puzzle* g, int i, int j, const vec4& Color )
{
	if ( i < 0 || j < 0 || i >= g->FColumns || j >= g->FRows ) { return; }

	clTile* Tile = g->GetTile( i, j );
	Tile->SetTarget( i, j );

	float X = Tile->FCur.x, Y = Tile->FCur.y;
	float TW = 1.0f / g->FColumns, TH = 1.0f / g->FRows;

	vec4 TilePosition( TW * ( X + 0 ), TH * ( Y + 0 ), TW * ( X + 1 ), TH * ( Y + 1 ) );

	const LRect* ClipRect = Tile->GetRect();

	g_Canvas->TexturedRect2DClipped( TilePosition.x, TilePosition.y, TilePosition.z, TilePosition.w, LVector4( 1 ), g_Texture, ClipRect->ToVector4() );
}

vec4 UIntToColor( int a )
{
	float r = ( float )( a & 0xFF ) / 255.0;
	float g = ( float )( ( a >>  8 ) & 0xFF ) / 255.0;
	float b = ( float )( ( a >> 16 ) & 0xFF ) / 255.0;

	return vec4( r, g, b, 1.0f );
}

void RenderGame( Puzzle* g )
{
	LGL3->glClearColor( 0.0, 0.0, 0.0, 0.0 );
	LGL3->glClear( GL_COLOR_BUFFER_BIT );

	// draw selected tile (if any)
	if ( g->FMovingImage && g->FClickedI >= 0 && g->FClickedJ >= 0 && g->FClickedI < g->FColumns && g->FClickedJ < g->FRows )
	{
		vec2 MCI = Env_GetMouse();
		int NewI = g->FClickedI, NewJ = g->FClickedJ;
		float PosX = Linderdaum::Math::Clamp( MCI.x + g->FOfsX, 0.0f, 1.0f ) * g->FColumns;
		float PosY = Linderdaum::Math::Clamp( MCI.y + g->FOfsY, 0.0f, 1.0f ) * g->FRows;
		g->GetTile( NewI, NewJ )->MoveTo( PosX, PosY );
	}

	// draw static game field
	for ( int i = 0; i != g->FColumns; i++ )
		for ( int j = 0; j != g->FRows; j++ )
			if ( g->FClickedI != i || g->FClickedJ != j )
			{
				clTile* T = g->GetTile( i, j );
				DrawTile( g, i, j, UIntToColor( TileColors[T->FOriginY * g->FColumns + T->FOriginX] ) );
			}

	// draw moving tile
	if ( g->FClickedI >= 0 && g->FClickedJ >= 0 )
	{
		clTile* T = g->GetTile( g->FClickedI, g->FClickedJ );
		DrawTile( g, g->FClickedI, g->FClickedJ, UIntToColor( TileColors[T->FOriginY * g->FColumns + T->FOriginX] ) );
	}
}

void OnDrawFrame()
{
	LGL3->glDisable( GL_DEPTH_TEST );

	RenderGame( &g_Game );
}

clPtr<clGLTexture> LoadTexture( const std::string& FileName )
{
	clPtr<clBitmap> Bmp = clBitmap::LoadImg( g_FS->CreateReader( FileName ) );

	clPtr<clGLTexture> Texture = new clGLTexture();
	Texture->LoadFromBitmap( Bmp );

	return Texture;
}

clPtr<clBlob> LoadFileAsBlob( const std::string& FName )
{
	clPtr<iIStream> input = g_FS->CreateReader( FName );
	clPtr<clBlob> Res = new clBlob();
	Res->CopyMemoryBlock( input->MapStream(), input->GetSize() );
	return Res;
}

clPtr<clGLSLShaderProgram> LoadShader( const std::string& FileName )
{
	clPtr<iIStream> S = g_FS->CreateReader( FileName );

	std::string Text( ( const char* )S->MapStream(), S->GetSize() );

	size_t p1 = Text.find( "/*VERTEX_PROGRAM*/" ) + 19;
	size_t p2 = Text.find( "/*FRAGMENT_PROGRAM*/" );

	std::string vs = Text.substr( p1, p2 - p1 );
	std::string fs = Text.substr( p2 + 21, Text.length() - p2 - 21 );

	return new clGLSLShaderProgram( vs, fs );
}

class clResponder: public iGestureResponder
{
	virtual void Event_PointerChanged( int PointerID, const LVector2& Pnt, bool Pressed )
	{
		if ( PointerID != 0 ) { return; }

		g_Pos = Pnt;

		g_Game.OnKey( g_Pos.x, g_Pos.y, Pressed );
	}

	virtual void Event_PointerMoved( int PointerID, const LVector2& Pnt )
	{
		if ( PointerID == 0 ) { g_Pos = Pnt; }
	}

	virtual void Event_UpdateGesture( const sMotionData& Data )
	{
		if ( Data.GetNumTouchPoints() != 1 )
		{
			g_Game.OnKey( g_Pos.x, g_Pos.y, false );
		}
	}
} Responder;

void OnStart( const std::string& RootPath )
{
	g_FS = new clFileSystem();
	g_FS->Mount( "." );
#if defined(ANDROID)
	g_FS->Mount( RootPath );
	g_FS->AddAliasMountPoint( RootPath, "assets" );
#endif

	g_TextRenderer = new clTextRenderer();
	g_Font = g_TextRenderer->GetFontHandle( "default.ttf" );

	LoadOGG();
	LoadModPlug();

	g_Canvas = new clCanvas();

	g_Texture = LoadTexture( "test.bmp" );

	g_Responder = &Responder;
}

void OnStop()
{
}

void OnKey( int Key, bool KeyState )
{
	if ( !KeyState )
	{
		return;
	}
}

void OnMouseDown( int btn, const LVector2& Pos )
{
	g_Pos = Pos;

	// control with clicks
	g_Game.OnKey( Pos.x, Pos.y, true );
}

void OnMouseMove( const LVector2& Pos )
{
	g_Pos = Pos;
}

void OnMouseUp( int btn, const LVector2& Pos )
{
	g_Pos = Pos;

	g_Game.OnKey( Pos.x, Pos.y, false );
}

void OnTimer( float DeltaTime )
{
	g_Game.Timer( DeltaTime );
}

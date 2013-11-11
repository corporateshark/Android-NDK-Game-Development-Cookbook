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

#include "Globals.h"
#include "Shape.h"
#include "Field.h"

bool IsKeyPressed( int Key )
{
	return false;
}

LVector2 g_Pos;

sLGLAPI* LGL3 = NULL;

clPtr<clCanvas> Canvas;
clPtr<clFileSystem> g_FS;
clPtr<clTextRenderer> g_TextRenderer;
int g_Font;

clAudioThread g_Audio;
clPtr<clAudioSource> Music = NULL;

LVector4 g_Colors[NUM_COLORS] =
{
	LVector4( 1.0f, 0.11f, 0.11f, 1.0f ),
	LVector4( 1.0f, 1.0f, 0.0f, 1.0f ),
	LVector4( 0.0f, 0.906f, 0.0f, 1.0f ),
	LVector4( 0.0f, 0.776f, 1.0f, 1.0f ),
	LVector4( 1.0f, 1.0f, 1.0f, 1.0f ),
	LVector4( 1.0f, 0.0f, 1.0f, 1.0f ),
};

extern LVector4 g_Colors[NUM_COLORS];

clPtr<clGLSLShaderProgram> BackShader;
clPtr<clGLTexture> BackTexture;
clPtr<clGLTexture> BackTexture_Top;
clPtr<clGLTexture> BackTexture_Bottom;
clPtr<clGLTexture> BricksImage[NUM_BRICK_IMAGES] = { NULL, NULL, NULL, NULL };
clPtr<clGLTexture> MsgFrameLeft;
clPtr<clGLTexture> MsgFrameRight;
clPtr<clGLTexture> MsgFrameCenter;
clPtr<clBitmap> g_ScoreBitmap;
clPtr<clGLTexture> g_ScoreTexture;
std::string g_ScoreText;

clBricksShape g_CurrentFigure;
clBricksShape g_NextFigure;
clBricksField g_Field;

float b_Flags[7];

float g_KeyPressTime = 0.0f;

const int LK_LEFT      = 0x25;
const int LK_UP        = 0x26;
const int LK_RIGHT     = 0x27;
const int LK_DOWN      = 0x28;
const int LK_LBUTTON   = 1;

void drawQuad( float x, float y, float w, float h, float OfsX, float OfsY, const LVector4& Color )
{
	float X1 = x / 800.0f;
	float Y1 = y / 600.0f;

	float X2 = ( x + w ) / 800.0f;
	float Y2 = ( y + h ) / 600.0f;

	X1 *= Field_Width / 0.35f;
	X2 *= Field_Width / 0.35f;
	Y1 *= Field_Height / 0.75f;
	Y2 *= Field_Height / 0.75f;

	Canvas->Rect2D( X1 + OfsX, Y1 + OfsY, X2 + OfsX, Y2 + OfsY, Color );
}

void drawTexQuad( float x, float y, float w, float h, float OfsX, float OfsY, const LVector4& Color, int ImageID )
{
	float X1 = x / 800.0f;
	float Y1 = y / 600.0f;

	float X2 = ( x + w ) / 800.0f;
	float Y2 = ( y + h ) / 600.0f;

	X1 *= Field_Width / 0.35f;
	X2 *= Field_Width / 0.35f;
	Y1 *= Field_Height / 0.75f;
	Y2 *= Field_Height / 0.75f;

	Canvas->TexturedRect2D( X1 + OfsX, Y1 + OfsY, X2 + OfsX, Y2 + OfsY, Color, BricksImage[ImageID] );
}

void DrawFigure( clBricksShape* Figure, int X, int Y, float OfsX, float OfsY, float BlockSize )
{
	for ( int i = 0 ; i < Figure->FWidth ; i++ )
	{
		for ( int j = 0 ; j < Figure->FHeight ; j++ )
		{
			// skip invisible raws
			if ( Y + j < 0 ) { continue; }

			int c = Figure->GetMask( i, j );

			if ( c >= 0 && c < NUM_COLORS )
			{
				drawTexQuad( ( X + i ) * ( BlockSize + 4.0f ) + 2.0f,
				             ( Y + j ) * ( BlockSize + 4.0f ) + 2.0f,
				             BlockSize, BlockSize, OfsX, OfsY,
				             g_Colors[c], c % NUM_BRICK_IMAGES );
			}
		}
	}
}

void DrawBorder( float X1, float Y1, float X2, float Y2, float Border )
{
	Canvas->TexturedRect2D( X1, Y1, X1 + Border, Y2, LVector4( 1.0f ), MsgFrameLeft  );
	Canvas->TexturedRect2D( X2 - Border, Y1, X2, Y2, LVector4( 1.0f ), MsgFrameRight );
	Canvas->TexturedRect2DTiled( X1 + Border, Y1, X2 - Border, Y2, 3, 1, LVector4( 1.0f ), MsgFrameCenter );
}

bool MoveFigureLeft()
{
	if ( g_Field.FigureFits( g_GS.FCurX - 1, g_GS.FCurY, g_CurrentFigure ) )
	{
		g_GS.FCurX--;
		return true;
	}

	return false;
}

bool MoveFigureRight()
{
	if ( g_Field.FigureFits( g_GS.FCurX + 1, g_GS.FCurY, g_CurrentFigure ) )
	{
		g_GS.FCurX++;
		return true;
	}

	return false;
}

bool MoveFigureDown()
{
	if ( g_Field.FigureFits( g_GS.FCurX, g_GS.FCurY + 1, g_CurrentFigure ) )
	{
		g_GS.FScore += 1 + g_GS.FLevel / 2;
		g_GS.FCurY++;
		return true;
	}

	return false;
}

bool MoveFigureUp()
{
	if ( g_Field.FigureFits( g_GS.FCurX, g_GS.FCurY - 1, g_CurrentFigure ) )
	{
		g_GS.FCurY--;
		return true;
	}

	return false;
}

bool RotateFigure( bool CW )
{
	clBricksShape TempFigure( g_CurrentFigure );

	TempFigure.Rotate( CW );

	if ( g_Field.FigureFits( g_GS.FCurX, g_GS.FCurY, TempFigure ) )
	{
		g_CurrentFigure = TempFigure;
		return false;
	}

	return true;
}

void ResetGame()
{
	g_GS.Reset();

	g_CurrentFigure.GenFigure( Linderdaum::Math::Random( NUM_SHAPES ), g_GS.FCurrentColor );
	g_NextFigure.GenFigure( Linderdaum::Math::Random( NUM_SHAPES ), g_GS.FNextColor );

	g_Field.ClearField();
}

const LRect MoveLeft( 0.0f,  0.863f, 0.32f, 1.0f );
const LRect Down(     0.32f, 0.863f, 0.67f, 1.0f );
const LRect MoveRight( 0.67f, 0.863f, 1.0f,  1.0f );

const LRect TurnLeft(  0.0f,  0.7f,  0.4f,  0.863f );
const LRect TurnRight( 0.6f,  0.7f,  1.0f,  0.863f );

const LRect Reset(  0.0f, 0.0f, 0.2f, 0.1f );
const LRect Paused( 0.8f, 0.0f, 1.0f, 0.1f );

void NextFigure();

void ProcessClick( bool Pressed )
{
	b_Flags[b_MoveLeft] = 0.0f;
	b_Flags[b_MoveRight] = 0.0f;
	b_Flags[b_Down] = 0.0f;
	b_Flags[b_TurnLeft] = 0.0f;
	b_Flags[b_TurnRight] = 0.0f;
	b_Flags[b_Paused] = 0.0f;
	b_Flags[b_Reset] = 0.0f;

	bool MousePressed = Pressed;

	if ( Reset.ContainsPoint( g_Pos ) )
	{
		if ( MousePressed ) { ResetGame(); }

		b_Flags[b_Reset] = MousePressed ? 1.0f : 0.0f;
	}

	// disable everything except reset after Game Over
	if ( g_GS.FGameOver ) { if ( !Pressed ) { ResetGame(); } return; }

	if ( Pressed )
	{
		if ( MoveLeft.ContainsPoint( g_Pos ) ) { MoveFigureLeft(); b_Flags[b_MoveLeft] = 1.0f; }

		if ( MoveRight.ContainsPoint( g_Pos ) ) { MoveFigureRight(); b_Flags[b_MoveRight] = 1.0f; }

		if ( Down.ContainsPoint( g_Pos ) ) { if ( !MoveFigureDown() ) { NextFigure(); } b_Flags[b_Down] = 1.0f; }

		if ( TurnLeft.ContainsPoint( g_Pos ) ) { RotateFigure( false ); b_Flags[b_TurnLeft] = 1.0f; }

		if ( TurnRight.ContainsPoint( g_Pos ) ) { RotateFigure( true ); b_Flags[b_TurnRight] = 1.0f; }

		if ( Paused.ContainsPoint( g_Pos ) )
		{
			b_Flags[b_Paused] = 1.0f;
			g_KeyPressTime = 0.0f;
		}
	}
}

void NextFigure()
{
	// drop this figure
	g_Field.AddFigure( g_GS.FCurX, g_GS.FCurY, g_CurrentFigure );

	// add new figure
	g_CurrentFigure = g_NextFigure;
	g_GS.SwitchColor();
	g_NextFigure.GenFigure( Linderdaum::Math::Random( NUM_SHAPES ), g_GS.FCurrentColor );

	g_GS.FCurX = ( g_Field.FWidth - g_CurrentFigure.FWidth ) / 2;
	g_GS.FCurY = 0;

	// check if the figure fits
	if ( !g_Field.FigureFits( g_GS.FCurX, g_GS.FCurY, g_CurrentFigure ) )
	{
		g_GS.FGameOver = true;
		return;
	}
}

void OnDrawFrame()
{
	LGL3->glDisable( GL_DEPTH_TEST );
	BackTexture_Bottom->Bind( 2 );
	BackTexture_Top->Bind( 1 );
	BackTexture->Bind( 0 );
	BackShader->Bind();
	BackShader->SetUniformNameFloatArray( "b_MoveLeft",  1, b_Flags[b_MoveLeft]  );
	BackShader->SetUniformNameFloatArray( "b_Down",      1, b_Flags[b_Down]      );
	BackShader->SetUniformNameFloatArray( "b_MoveRight", 1, b_Flags[b_MoveRight] );
	BackShader->SetUniformNameFloatArray( "b_TurnLeft",  1, b_Flags[b_TurnLeft]  );
	BackShader->SetUniformNameFloatArray( "b_TurnRight", 1, b_Flags[b_TurnRight] );
	BackShader->SetUniformNameFloatArray( "b_Reset",     1, b_Flags[b_Reset]     );
	BackShader->SetUniformNameFloatArray( "b_Paused",    1, b_Flags[b_Paused]    );

	Canvas->GetFullscreenRect()->Draw( false );

	// 1. render cells
	for ( int i = 0 ; i < g_Field.FWidth ; i++ )
	{
		for ( int j = FIELD_INVISIBLE_RAWS ; j < g_Field.FHeight ; j++ )
		{
			int _c = g_Field.FField[i][j];

			if ( _c >= 0 && _c < NUM_COLORS )
			{
				drawTexQuad( i * 20.0f + 2.0f, ( j - FIELD_INVISIBLE_RAWS ) * 20.0f + 2.0f, 16.0f, 16.0f, Field_X1, Field_Y1, g_Colors[_c], _c % NUM_BRICK_IMAGES );
			}
		}
	}

	// 2. render current figure
	DrawFigure( &g_CurrentFigure, g_GS.FCurX, g_GS.FCurY - FIELD_INVISIBLE_RAWS, Field_X1, Field_Y1, BLOCK_SIZE );

	// 3. render next figure
	int Cx1, Cy1, Cx2, Cy2;
	g_NextFigure.GetTopLeftCorner( &Cx1, &Cy1 );
	g_NextFigure.GetBottomRightCorner( &Cx2, &Cy2 );
	LRect FigureSize = g_NextFigure.GetSize();
	float dX = ( float )Cx1 * BLOCK_SIZE_SMALL / 800.0f;
	float dY = ( float )Cy1 * BLOCK_SIZE_SMALL / 600.0f;
	float dX2 = 0.5f * ( float )Cx2 * BLOCK_SIZE_SMALL / 800.0f;
	float dY2 = 0.5f * ( float )Cy2 * BLOCK_SIZE_SMALL / 600.0f;

	DrawFigure( &g_NextFigure, 0, 0, 0.415f - dX - dX2, 0.77f - dY - dY2, BLOCK_SIZE_SMALL );

	std::string ScoreString( Str_GetFormatted( "%02i:%06i", g_GS.FLevel, g_GS.FScore ) );

	if ( g_ScoreText != ScoreString )
	{
		g_ScoreText = ScoreString;
		g_ScoreBitmap = g_TextRenderer->RenderTextWithFont( ScoreString.c_str(), g_Font, 32, 0xFFFFFFFF, true );
		g_ScoreTexture->LoadFromBitmap( g_ScoreBitmap );
	}

	LVector4 Color( 0.741f, 0.616f, 0.384f, 1.0f );

	Canvas->TexturedRect2D( 0.19f, 0.012f, 0.82f, 0.07f, Color, g_ScoreTexture );

	if ( g_GS.FGameOver )
	{
		DrawBorder( 0.05f, 0.25f, 0.95f, 0.51f, 0.19f );

		std::string ScoreStr = Str_GetPadLeft( Str_ToStr( g_GS.FScore ), 6, '0' );

		Canvas->TextStr( 0.20f, 0.33f, 0.84f, 0.37f, "Your score:", 32, LVector4( 0.796f, 0.086f, 0.086f, 1.0f ), g_TextRenderer, g_Font );
		Canvas->TextStr( 0.20f, 0.38f, 0.84f, 0.44f, ScoreStr, 32, LVector4( 0.8f, 0.0f, 0.0f, 1.0f ), g_TextRenderer, g_Font );
	}
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
	virtual void Event_UpdateGesture( const sMotionData& Data )
	{
		if ( Data.GetNumTouchPoints() != 1 )
		{
			ProcessClick( false );
			return;
		}

		g_Pos = Data.GetTouchPointPos( 0 );

		ProcessClick( true );

		g_KeyPressTime = 0;
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

	g_ScoreTexture = new clGLTexture();

	LoadOGG();
	LoadModPlug();

	Canvas = new clCanvas();

	BackTexture = LoadTexture( "back.png" );
	BackTexture_Top = LoadTexture( "back_high_top.png" );
	BackTexture_Bottom = LoadTexture( "back_high_bottom.png" );
	BackShader  = LoadShader( "back.sp" );
	BricksImage[0] = LoadTexture( "blockmask0.png" );
	BricksImage[1] = LoadTexture( "blockmask1.png" );
	BricksImage[2] = LoadTexture( "blockmask2.png" );
	BricksImage[3] = LoadTexture( "blockmask3.png" );
	MsgFrameCenter = LoadTexture( "messageframecenter.png" );
	MsgFrameCenter->SetClamping( GL_REPEAT );
	MsgFrameLeft   = LoadTexture( "messageframeleft.png" );
	MsgFrameLeft->SetClamping( GL_CLAMP_TO_EDGE );
	MsgFrameRight  = LoadTexture( "messageframeright.png" );
	MsgFrameRight->SetClamping( GL_CLAMP_TO_EDGE );

	ResetGame();

	g_Audio.Start( iThread::Priority_Normal );
	g_Audio.Wait();

	Music = new clAudioSource();
	Music->BindWaveform( new clModPlugProvider( LoadFileAsBlob( "test.xm" ) ) );
	Music->LoopSound( true );
	Music->Play();

	g_Responder = &Responder;
}

void OnStop()
{
	Music = NULL;
}

void OnKey( int Key, bool KeyState )
{
	if ( !KeyState )
	{
		ProcessClick( false );
		return;
	}

	if ( !g_GS.FGameOver )
	{
		if ( Key == LK_UP ) { RotateFigure( true ); }

		if ( Key == LK_DOWN ) { if ( !MoveFigureDown() ) { NextFigure(); } }

		if ( Key == LK_LEFT ) { MoveFigureLeft(); }

		if ( Key == LK_RIGHT ) { MoveFigureRight(); }
	}
}

void OnMouseDown( int btn, const LVector2& Pos )
{
	g_Pos = Pos;

	g_KeyPressTime = 0;

	// control with clicks
	ProcessClick( true );
}

void OnMouseMove( const LVector2& Pos )
{
	g_Pos = Pos;
}

void OnMouseUp( int btn, const LVector2& Pos )
{
	g_Pos = Pos;

	ProcessClick( false );
}

void OnTimer( float DeltaTime )
{
	if ( g_GS.FGameOver ) { return; }

	g_GS.FGameTimeCount += DeltaTime;
	g_GS.FGameTime += DeltaTime;

	g_KeyPressTime += DeltaTime;

	// control with clicks - do autorepeat
	if ( ( b_Flags[b_MoveLeft] > 0 ||
	       b_Flags[b_MoveRight] > 0 ||
	       b_Flags[b_Down] > 0 ||
	       b_Flags[b_TurnLeft] > 0 ||
	       b_Flags[b_TurnRight] > 0 ) && g_KeyPressTime > g_KeyTypematicDelay )
	{
		g_KeyPressTime -= g_KeyTypematicRate;

		ProcessClick( true );
	}

	while ( g_GS.FGameTimeCount > g_GS.FUpdateSpeed )
	{
		if ( !MoveFigureDown() )
		{
			NextFigure();
		}

		// check for lines deletion
		int Count = g_Field.DeleteRegions( BlocksToDisappear );

		if ( Count > 0 ) { g_GS.FScore += Count * Count * 7 * ( g_GS.FLevel + 1 ); }

		g_GS.FGameTimeCount -= g_GS.FUpdateSpeed;

		if ( g_GS.FGameTime > ( g_GS.FLevel + 1 ) * NextLevelTime )
		{
			g_GS.FLevel++;
			g_GS.FUpdateSpeed *= 0.95f;
		}
	}
}

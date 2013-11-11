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

#include <windows.h>

#include <stdio.h>

#include "BoxLite.h"

using namespace Box2D;

const float TIME_STEP = 0.016666f;

#include "BoxSample.h"

#define IDC_B1 (1234)
#define IDC_B2 (1235)
#define IDC_B3 (1236)

LRESULT CALLBACK MyFunc( HWND, UINT, WPARAM, LPARAM );

char szWinName[] = "MyWin";

const int MAX_X = 800;
const int MAX_Y = 600;

const int WND_W = MAX_X + 200;
const int WND_H = MAX_Y + 200;

HWND hWnd;

//int WINAPI WinMain(HINSTANCE hThisInst, HINSTANCE hPrevInst, LPSTR lpszArgs, int nWinMode)
int main()
{
	MSG msg;
	WNDCLASS wcl;

	HWND hButton1, hButton2;

	wcl.hInstance = NULL; // hThisInst;
	wcl.lpszClassName = szWinName;
	wcl.lpfnWndProc = MyFunc;
	wcl.style = 0;
	wcl.hIcon = LoadIcon( NULL, IDI_APPLICATION );
	wcl.hCursor = LoadCursor( NULL, IDC_ARROW );
	wcl.lpszMenuName = NULL;

	wcl.cbClsExtra = 0;
	wcl.cbWndExtra = 0;

	wcl.hbrBackground = ( HBRUSH ) GetStockObject( WHITE_BRUSH );

	if ( !RegisterClass( &wcl ) ) { return 0; }

	hWnd = CreateWindowA( szWinName, "Simple Window",
	                      WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, WND_W /*CW_USEDEFAULT*/, WND_H /*CW_USEDEFAULT*/, HWND_DESKTOP, NULL,
	                      NULL/*hThisInst*/, NULL );

	CreateWindowA(
	   "BUTTON", "Start", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
	   100, MAX_Y + 100, 90, 24, hWnd, ( HMENU )IDC_B1, NULL, NULL );

	CreateWindowA(
	   "BUTTON", "Stop", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
	   200, MAX_Y + 100, 90, 24, hWnd, ( HMENU )IDC_B2, NULL, NULL );

	CreateWindowA(
	   "BUTTON", "Exit", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
	   300, MAX_Y + 100, 100, 24, hWnd, ( HMENU )IDC_B3, NULL, NULL );

	ShowWindow( hWnd, SW_SHOW /*nWinMode*/ );
	UpdateWindow( hWnd );

	while ( GetMessage( &msg, NULL, 0, 0 ) )
	{
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

	return msg.wParam;
}

World* Wld;

float LocalTime = 0;

void UpdateTime( float dt )
{
	LocalTime += dt;

	while ( LocalTime > TIME_STEP )
	{
		Wld->Step( TIME_STEP );
		LocalTime -= TIME_STEP;
	}
}


//////////

void DrawLine( HDC dc, int x1, int y1, int x2, int y2 )
{
	MoveToEx( dc, x1, y1, NULL );
	LineTo( dc, x2, y2 );
}

const float XScale = 20;
const float YScale = 20;

const float XOfs = WND_W / 2;
const float YOfs = WND_H / 2 + 200;

inline int X2Screen( float X ) { return ( int )( X * XScale + XOfs ); }
inline int Y2Screen( float Y ) { return ( int )( -Y * YScale + YOfs ); }

inline Vec2 Scr2Point( int x, int y ) { return Vec2( ( ( float )x - XOfs ) / XScale, ( YOfs - ( float )y ) / YScale ); }

void DrawLine2( HDC dc, const Vec2& v1, const Vec2& v2 )
{
	int x1 = X2Screen( v1.x ); // (int)(v1.x * XScale + XOfs);
	int x2 = X2Screen( v2.x ); // (int)(v2.x * XScale + XOfs);
	int y1 = Y2Screen( v1.y ); // (int)(-v1.y * YScale + YOfs);
	int y2 = Y2Screen( v2.y ); // (int)(-v2.y * YScale + YOfs);

	DrawLine( dc, x1, y1, x2, y2 );
}

void DrawBody( HDC dc, Body* body )
{
	Mat22 R( body->rotation );
	Vec2 x = body->position;
	Vec2 h = 0.5f * body->width;

	Vec2 v[] = { ( x + R * Vec2( -h.x, -h.y ) ), ( x + R * Vec2( h.x, -h.y ) ), ( x + R * Vec2( h.x,  h.y ) ), ( x + R * Vec2( -h.x,  h.y ) ) };

	for ( int i = 0 ; i < 4 ; i++ ) { DrawLine2( dc, v[i], v[( i + 1 ) % 4] ); };
}

void DrawJoint( HDC dc, Joint* joint )
{
	Body* b1 = joint->body1;
	Body* b2 = joint->body2;

	Mat22 R1( b1->rotation );
	Mat22 R2( b2->rotation );

	Vec2 x1 = b1->position;
	Vec2 p1 = x1 + R1 * joint->localAnchor1;

	Vec2 x2 = b2->position;
	Vec2 p2 = x2 + R2 * joint->localAnchor2;

	DrawLine2( dc, x1, p1 );
	DrawLine2( dc, p1, x2 );
	DrawLine2( dc, x2, p2 );
	DrawLine2( dc, p2, x1 );
}

void DrawWorld( HDC dc, World* W )
{
	for ( std::vector<Body*>::iterator b = W->bodies.begin() ; b != W->bodies.end() ; b++ )
	{
		DrawBody( dc, *b );
	}

	for ( std::vector<Joint*>::iterator j = W->joints.begin() ; j != W->joints.end() ; j++ )
	{
		DrawJoint( dc, *j );
	}
}

//////////

void InitPhys()
{
	Wld = new World( Vec2( 0, 0 ), 10 );

	setup3( Wld );
}

void LaunchTheBomb( const Vec2& Position, const Vec2& Velocity )
{
	Body* Bomb = CreateBody( Vec2( 1, 1 ), 5.0f, 0.2f );

	Bomb->position = Position;
	Bomb->rotation = _RandomInRange( -1.5f, 1.5f );
	Bomb->velocity = Velocity;
	Bomb->angularVelocity = _RandomInRange( -10.0f, 10.0f );

	Wld->Add( Bomb );
}

// 1 - if selected, 0 - if not
void PaintIt( HDC dc )
{
	// default
	SelectObject( dc, GetStockObject( BLACK_PEN ) );

	DrawWorld( dc, Wld );
}

LRESULT CALLBACK MyFunc( HWND this_hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch ( message )
	{
		case WM_CREATE:
			InitPhys();
			break;

		case WM_DESTROY:
			PostQuitMessage( 0 );
			break;

		case WM_TIMER:
			UpdateTime( 0.02f );
			InvalidateRect( hWnd, NULL, 1 );
			break;

		case WM_PAINT:

			// repaint the entire window
			if ( this_hwnd == hWnd )
			{
				PAINTSTRUCT ps;
				HDC ThisDC = BeginPaint( hWnd, &ps );
				PaintIt( ThisDC );
				EndPaint( hWnd, &ps );
			}

			break;

		case WM_COMMAND:
		{
			switch ( ( int ) LOWORD( wParam ) )
			{
				case IDC_B1:
				{ SetTimer( hWnd, 1, 20, NULL ); break; }

				case IDC_B2:
				{ KillTimer( hWnd, 1 ); break; }

				case IDC_B3:
				{ SendMessage( hWnd, WM_DESTROY, 0, 0 ); break; }
			}
		}

		break;

	}

	return DefWindowProc( this_hwnd, message, wParam, lParam );
}

#pragma once

#include "Wrapper_Callbacks.h"

extern unsigned char* g_FrameBuffer;

extern const int Width;
extern const int Height;

extern float XScale, YScale;
extern float XOfs, YOfs;

int XToScreen( float x );
int YToScreen( float y );

float ScreenToX( int x );
float ScreenToY( int y );

void render_str( unsigned char* buf24, int w, int h, const char* str, int x, int y, int color );

void set_pixel( unsigned char* fb, int w, int h, int x, int y, int color );

void swap_int( int& v1, int& v2 );

void line_bresenham( unsigned char* fb, int w, int h, int p1x, int p1y, int p2x, int p2y, int color );

void Line( int x1, int y1, int x2, int y2, int color );

void LineW( float x1, float y1, float x2, float y2, int color );

void Clear( int color );

void RenderString( const char* str, int x, int y, int color );

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

#include "Wrapper_Callbacks.h"

extern unsigned char* g_FrameBuffer;

extern const int Width;
extern const int Height;

extern float XScale, YScale;
extern float XOfs, YOfs;

inline int XToScreen( float x ) { return Width / 2 + x * XScale + XOfs; }
inline int YToScreen( float y ) { return Height / 2 - y * YScale + YOfs; }

inline float ScreenToX( int x ) { return ( ( float )( x - Width / 2 )  - XOfs ) / XScale; }
inline float ScreenToY( int y ) { return -( ( float )( y - Height / 2 ) - YOfs ) / YScale; }

inline void set_pixel( unsigned char* fb, int w, int h, int x, int y, int color )
{
	if ( x < 0 || y < 0 || x > w - 1 || y > h - 1 ) { return; }

	fb[( ( h - 1 - y ) * w + x ) * 4 + 0] = ( color       ) & 0xFF;
	fb[( ( h - 1 - y ) * w + x ) * 4 + 1] = ( color >> 8  ) & 0xFF;
	fb[( ( h - 1 - y ) * w + x ) * 4 + 2] = ( color >> 16 ) & 0xFF;
	fb[( ( h - 1 - y ) * w + x ) * 4 + 3] = 0;
}

inline void swap_int( int& v1, int& v2 ) { int t = v1; v1 = v2; v2 = t; }

void line_bresenham( unsigned char* fb, int w, int h, int p1x, int p1y, int p2x, int p2y, int color );

inline void Line( int x1, int y1, int x2, int y2, int color )
{
	line_bresenham( g_FrameBuffer, Width, Height, x1, y1, x2, y2, color );
}

void Clear( int color );

inline void LineW( float x1, float y1, float x2, float y2, int color )
{
	Line( XToScreen( x1 ), YToScreen( y1 ), XToScreen( x2 ), YToScreen( y2 ), color );
}

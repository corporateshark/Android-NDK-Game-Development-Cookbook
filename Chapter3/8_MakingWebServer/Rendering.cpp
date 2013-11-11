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

#include "Rendering.h"

const int Width = ImageWidth;
const int Height = ImageHeight;

float XScale, YScale;
float XOfs, YOfs;

unsigned char* g_FrameBuffer;

unsigned char* font;
int fw, fh;
int char_w, char_h;

int XToScreen( float x ) { return Width / 2 + x * XScale + XOfs; }
int YToScreen( float y ) { return Height / 2 - y * YScale + YOfs; }

float ScreenToX( int x ) { return ( ( float )( x - Width / 2 )  - XOfs ) / XScale; }
float ScreenToY( int y ) { return -( ( float )( y - Height / 2 ) - YOfs ) / YScale; }

void set_pixel( unsigned char* fb, int w, int h, int x, int y, int color )
{
	if ( x < 0 || y < 0 || x > w - 1 || y > h - 1 ) { return; }

	fb[( ( h - 1 - y ) * w + x ) * 4 + 0] = ( color       ) & 0xFF;
	fb[( ( h - 1 - y ) * w + x ) * 4 + 1] = ( color >> 8  ) & 0xFF;
	fb[( ( h - 1 - y ) * w + x ) * 4 + 2] = ( color >> 16 ) & 0xFF;
	fb[( ( h - 1 - y ) * w + x ) * 4 + 3] = 0; // ( color >> 16 ) & 0xFF;
}

void swap_int( int& v1, int& v2 ) { int t = v1; v1 = v2; v2 = t; }

void line_bresenham( unsigned char* fb, int w, int h, int p1x, int p1y, int p2x, int p2y, int color );

void Line( int x1, int y1, int x2, int y2, int color )
{
	line_bresenham( g_FrameBuffer, Width, Height, x1, y1, x2, y2, color );
}

void LineW( float x1, float y1, float x2, float y2, int color )
{
	Line( XToScreen( x1 ), YToScreen( y1 ), XToScreen( x2 ), YToScreen( y2 ), color );
}

int get_pixel24( unsigned char* buffer, int width, int height, int x, int y )
{
	int offset = ( y * width + x ) * 3;
	return ( buffer[offset + 0] << 16 ) + ( buffer[offset + 1] << 8 ) + buffer[offset + 2];
}
/*
void put_pixel(unsigned char* buffer, int width, int height, int x, int y, int color)
{
   int offset = (y * width + x) * 3;

   buffer[offset + 0] = (color >> 16) & 0xFF;
   buffer[offset + 1] = (color >>  8) & 0xFF;
   buffer[offset + 2] = (color >>  0) & 0xFF;
}
*/
void render_char( unsigned char* buf, int w, int h, char c, int x, int y, int color )
{
	int u = ( ( ( int )c ) % 16 ) * char_w;
	int v = char_h / 2 + ( ( ( ( int )c ) >> 4 ) - 1 ) * char_h;

	int x1, y1;

	for ( y1 = v ; y1 < v + char_h ; y1++ )
	{
		for ( x1 = u ; x1 <= u + char_w ; x1++ )
		{
			int m_color = get_pixel24( font, fw, fh, x1, y1 );

			if ( m_color != 0 )
			{
				set_pixel( buf, w, h, x + x1 - u, y + y1 - v, m_color );
			}
		}
	}
}

void render_str( unsigned char* buf24, int w, int h, const char* str, int x, int y, int color )
{
	const char* c = str;

	while ( *c )
	{
		render_char( buf24, w, h, *c, x, y, color );

		c++;

		x += char_w;
	}
}

void line_bresenham( unsigned char* fb, int w, int h, int p1x, int p1y, int p2x, int p2y, int color )
{
	int F, x, y;

	if ( p1x > p2x ) // Swap points if p1 is on the right of p2
	{
		swap_int( p1x, p2x );
		swap_int( p1y, p2y );
	}

	// Handle trivial cases separately for algorithm speed up.
	// Trivial case 1: m = +/-INF (Vertical line)
	if ( p1x == p2x )
	{
		// Swap y-coordinates if p1 is above p2
		if ( p1y > p2y ) { swap_int( p1y, p2y ); }

		x = p1x;
		y = p1y;

		while ( y <= p2y )
		{
			set_pixel( fb, w, h, x, y, color );
			y++;
		}

		return;
	}
	// Trivial case 2: m = 0 (Horizontal line)
	else if ( p1y == p2y )
	{
		x = p1x;
		y = p1y;

		while ( x <= p2x )
		{
			set_pixel( fb, w, h, x, y, color );
			x++;
		}

		return;
	}

	int dy            = p2y - p1y;  // y-increment from p1 to p2
	int dx            = p2x - p1x;  // x-increment from p1 to p2
	int dy2           = ( dy << 1 ); // dy << 1 == 2*dy
	int dx2           = ( dx << 1 );
	int dy2_minus_dx2 = dy2 - dx2;  // precompute constant for speed up
	int dy2_plus_dx2  = dy2 + dx2;

	if ( dy >= 0 )  // m >= 0
	{
		// Case 1: 0 <= m <= 1 (Original case)
		if ( dy <= dx )
		{
			F = dy2 - dx;    // initial F

			x = p1x;
			y = p1y;

			while ( x <= p2x )
			{
				set_pixel( fb, w, h, x, y, color );

				if ( F <= 0 )
				{
					F += dy2;
				}
				else
				{
					y++;
					F += dy2_minus_dx2;
				}

				x++;
			}
		}
		// Case 2: 1 < m < INF (Mirror about y=x line
		// replace all dy by dx and dx by dy)
		else
		{
			F = dx2 - dy;    // initial F

			y = p1y;
			x = p1x;

			while ( y <= p2y )
			{
				set_pixel( fb, w, h, x, y, color );

				if ( F <= 0 )
				{
					F += dx2;
				}
				else
				{
					x++;
					F -= dy2_minus_dx2;
				}

				y++;
			}
		}
	}
	else    // m < 0
	{
		// Case 3: -1 <= m < 0 (Mirror about x-axis, replace all dy by -dy)
		if ( dx >= -dy )
		{
			F = -dy2 - dx;    // initial F

			x = p1x;
			y = p1y;

			while ( x <= p2x )
			{
				set_pixel( fb, w, h, x, y, color );

				if ( F <= 0 )
				{
					F -= dy2;
				}
				else
				{
					y--;
					F -= dy2_plus_dx2;
				}

				x++;
			}
		}
		// Case 4: -INF < m < -1 (Mirror about x-axis and mirror
		// about y=x line, replace all dx by -dy and dy by dx)
		else
		{
			F = dx2 + dy;    // initial F

			y = p1y;
			x = p1x;

			while ( y >= p2y )
			{
				set_pixel( fb, w, h, x, y, color );

				if ( F <= 0 )
				{
					F += dx2;
				}
				else
				{
					x++;
					F += dy2_plus_dx2;
				}

				y--;
			}
		}
	}
}

void Clear( int color )
{
	int w = Width, h = Height;

	unsigned char r = ( color & 0xFF );
	unsigned char g = ( ( color >> 8 ) & 0xFF );
	unsigned char b = ( ( color >> 16 ) & 0xFF );

	unsigned char* ptr = g_FrameBuffer;

	for ( int y = 0 ; y < h ; y++ )
		for ( int x = 0 ; x < w ; x++ )
		{
			*ptr++ = r;
			*ptr++ = g;
			*ptr++ = b;
			ptr++;
		}
}

void RenderString( const char* str, int x, int y, int color )
{
	render_str( g_FrameBuffer, Width, Height, str, x, y, color );
}


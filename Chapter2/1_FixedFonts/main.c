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

#include "bmp.h"
#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>

unsigned char* bmp;

unsigned char* font;

int w = 1000, h = 1000;

int fw, fh;

int char_w, char_h;

void render_char( unsigned char* buf24, char c, int x, int y, int color )
{
	int u = ( ( ( int )c ) % 16 ) * char_w;
	int v = char_h / 2 + ( ( ( ( int )c ) >> 4 ) - 1 ) * char_h;

	int x1, y1;

	for ( y1 = v ; y1 < v + char_h ; y1++ )
	{
		for ( x1 = u ; x1 <= u + char_w ; x1++ )
		{
			int m_color = get_pixel( font, fw, fh, x1, y1 );

			float c = ( float )( m_color & 0xFF ) / 255.0f;

			if ( m_color != 0 )
			{
				put_pixel( buf24, w, h, x + x1 - u, y + y1 - v, ( int )( c * color ) );
			}
		}
	}
}

void render_str( unsigned char* buf24, const char* str, int x, int y, int color )
{
	const char* c = str;

	while ( *c )
	{
		render_char( buf24, *c, x, y, color );

		c++;

		x += char_w;
	}
}

int main( int argc, char** argv )
{
	font = read_bmp( "font.bmp", &fw, &fh );

	char_w = fw / 16;
	char_h = fh / 16;

	bmp = ( unsigned char* )malloc( w * h * 3 );
	clear_img( bmp, w, h );

	render_str ( bmp, "Test string", 10, 10, 0x0000FF );

	write_bmp( "test.bmp", w, h, bmp );

	free( bmp );
	return 0;
}

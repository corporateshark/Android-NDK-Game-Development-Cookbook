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

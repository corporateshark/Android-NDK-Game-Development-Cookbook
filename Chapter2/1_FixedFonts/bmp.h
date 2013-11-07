#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

void write_bmp_gen( const char* fname, int w, int h, int bpp, unsigned char* img )
{
	int bytespp = bpp / 8;

	FILE* f = fopen( fname, "wb" );
	unsigned char bfh[54] = {0x42, 0x4d,
	                         /* bfSize [2]*/ 54, 0, 0, 0, /**/
	                         /* reserved [6]*/ 0, 0, 0, 0, /**/
	                         /* biOffBits [10]*/ 54, 0, 0, 0, /**/
	                         /* biSize [14]*/ 40, 0, 0, 0, /**/
	                         /* width [18]*/ 0, 0, 0, 0, /**/
	                         /* height [22]*/ 0, 0, 0, 0, /**/
	                         /* planes [26]*/ 1, 0, /**/
	                         /* bitcount [28]*/ bpp, 0,/**/
	                         /* compression [30]*/ 0, 0, 0, 0, /**/
	                         /* size image [34]*/ 0, 0, 0, 0, /**/
	                         /* xpermeter [38]*/ 0, 0, 0, 0, /**/
	                         /* ypermeter [42]*/ 0, 0, 0, 0, /**/
	                         /* clrused [46]*/ 0, 0, 0, 0, /**/
	                         /* clrimportant [50]*/ 0, 0, 0, 0 /**/
	                        };
	int realw = w * ( bpp / 8 ), rem = w % 4, isz = ( realw + rem ) * h, fsz = isz + 54;
	//bfh.bfSize = fsz;
	bfh[2] = ( fsz & 0xFF );
	bfh[3] = ( fsz >> 8 ) & 0xFF;
	bfh[4] = ( fsz >> 16 ) & 0xFF;
	bfh[5] = ( fsz >> 24 ) & 0xFF;
	//bfh.biSize = isz
	bfh[34] = ( isz & 0xFF );
	bfh[35] = ( isz >> 8 ) & 0xFF;
	bfh[36] = ( isz >> 16 ) & 0xFF;
	bfh[37] = ( isz >> 24 ) & 0xFF;
	//bfh.biWidth = w;
	bfh[18] = ( w & 0xFF );
	bfh[19] = ( w >> 8 ) & 0xFF;
	bfh[20] = ( w >> 16 ) & 0xFF;
	bfh[21] = ( w >> 24 ) & 0xFF;
	//bfh.biHeight = h;
	bfh[22] = ( h & 0xFF );
	bfh[23] = ( h >> 8 ) & 0xFF;
	bfh[24] = ( h >> 16 ) & 0xFF;
	bfh[25] = ( h >> 24 ) & 0xFF;

	fwrite( ( void* )&bfh[0], 54, 1, f );
	unsigned char* bstr = ( unsigned char* )malloc( realw ), *remstr = 0;

	if ( rem != 0 ) { remstr = ( unsigned char* )malloc( rem ); memset( remstr, 0, rem ); }

	int i, j, k;

	if ( bpp == 24 )
	{
		for ( j = h - 1 ; j > -1 ; j-- )
		{
			for ( i = 0 ; i < w ; i++ )
			{
				for ( k = 0 ; k < 3 ; k++ ) { bstr[i * 3 + k] = img[( j * realw + i * 3 ) + ( 2 - k )]; }
			}

			fwrite( bstr, realw, 1, f );

			if ( rem != 0 ) { fwrite( remstr, rem, 1, f ); }
		}
	}
	else
	{
		for ( j = h - 1 ; j > -1 ; j-- )
		{
			for ( i = 0 ; i < w ; i++ )
			{
				bstr[i * 4] = img[( j * realw + i * 4 )];

				for ( k = 0 ; k < 3 ; k++ ) { bstr[i * 4 + k + 1] = img[( j * realw + i * 4 ) + 1 + ( 2 - k )]; }
			}

			fwrite( bstr, realw, 1, f );

			if ( rem != 0 ) { fwrite( remstr, rem, 1, f ); }
		}
	}

	free( bstr );

	if ( remstr ) { free( remstr ); }

	fclose( f );
}

void write_bmp( const char* fname, int w, int h, unsigned char* img )
{
	write_bmp_gen( fname, w, h, 24, img );
}

void clear_img( unsigned char* FB, int W, int H )
{
	memset( FB, 0, W * H * 3 );
}

void put_pixel( unsigned char* FB, int W, int H, int x, int y, int Color)
{
   if(y < 0 || x < 0 || y > H - 1 || x > W - 1) { return; }
   int ofs = y * W + x;
   FB[ofs * 3 + 0] = Color & 0xFF;
   FB[ofs * 3 + 1] = (Color >> 8) & 0xFF;
   FB[ofs * 3 + 2] = (Color >> 16) & 0xFF;
}

int get_pixel( unsigned char* buffer, int width, int height, int x, int y )
{
	int offset = ( y * width + x ) * 3;
	return ( buffer[offset + 0] << 16 ) + ( buffer[offset + 1] << 8 ) + buffer[offset + 2];
}

unsigned char* read_bmp( const char* fname, int* _w, int* _h )
{
	unsigned char head[54];
	FILE* f = fopen( fname, "rb" );
	fread( ( void* )&head[0], 1, 54, f );
	*_w = head[18] + ( ( ( int )head[19] ) << 8 ) + ( ( ( int )head[20] ) << 16 ) + ( ( ( int )head[21] ) << 24 );
	*_h = head[22] + ( ( ( int )head[23] ) << 8 ) + ( ( ( int )head[24] ) << 16 ) + ( ( ( int )head[25] ) << 24 );
	const int fileSize = ( ( *_w ) * 3 + ( ( *_w ) % 4 ) ) * ( *_h );
	unsigned char* img = ( unsigned char* )malloc( ( *_w ) * ( *_h ) * 3 ), *data = ( unsigned char* )malloc( fileSize );
	fseek( f, 54, SEEK_SET );
	fread( data, 1, fileSize, f );
	int i, j, k, rev_j;

	for ( j = 0, rev_j = ( *_h ) - 1; j < ( *_h ) ; j++, rev_j-- )
	{
		for ( i = 0 ; i < ( *_w ) ; i++ )
		{
			int fpos = j * ( ( *_w ) * 3 + ( *_h ) % 4 ) + i * 3, pos = rev_j * ( *_w ) * 3 + i * 3;

			for ( k = 0 ; k < 3 ; k++ ) { img[pos + k] = data[fpos + ( 2 - k )]; }
		}
	}

	free( data );
	return img;
}

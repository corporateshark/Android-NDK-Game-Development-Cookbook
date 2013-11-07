#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

unsigned char* read_bmp_mem( unsigned char* data, int* w, int* h )
{
	unsigned char* head = data;
	*w = head[18] + ( ( ( int )head[19] ) << 8 ) + ( ( ( int )head[20] ) << 16 ) + ( ( ( int )head[21] ) << 24 );
	*h = head[22] + ( ( ( int )head[23] ) << 8 ) + ( ( ( int )head[24] ) << 16 ) + ( ( ( int )head[25] ) << 24 );
	const int fileSize = ( ( *w ) * 3 + ( ( *w ) % 4 ) ) * ( *h );
	unsigned char* img = ( unsigned char* )malloc( ( *w ) * ( *h ) * 3 );
	data += 54;
	int i, j, k, rev_j;

	for ( j = 0, rev_j = ( *h ) - 1; j < ( *h ) ; j++, rev_j-- )
	{
		for ( i = 0 ; i < ( *w ) ; i++ )
		{
			int fpos = j * ( ( *w ) * 3 + ( *h ) % 4 ) + i * 3, pos = rev_j * ( *w ) * 3 + i * 3;

			for ( k = 0 ; k < 3 ; k++ ) { img[pos + k] = data[fpos + ( 2 - k )]; }
		}
	}

	return img;
}

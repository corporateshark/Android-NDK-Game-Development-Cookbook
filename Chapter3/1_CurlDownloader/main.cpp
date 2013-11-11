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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CurlWrap.h"

struct MemoryStruct
{
	char* memory;
	size_t size;
};

static size_t WriteMemoryCallback( void* contents, size_t size, size_t nmemb, void* userp )
{
	size_t realsize = size * nmemb;
	struct MemoryStruct* mem = ( struct MemoryStruct* )userp;

	mem->memory = ( char* )realloc( mem->memory, mem->size + realsize + 1 );

	if ( mem->memory == NULL )
	{
		printf( "Not enough memory (realloc returned NULL)\n" );
		exit( EXIT_FAILURE );
	}

	memcpy( &( mem->memory[mem->size] ), contents, realsize );
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

int main( void )
{
	if ( !Curl_Load() ) { return 1; }

	CURL* curl_handle;

	struct MemoryStruct chunk;

	chunk.memory = ( char* )malloc( 1 );
	chunk.size = 0;

	curl_global_init_P( CURL_GLOBAL_ALL );
	curl_handle = curl_easy_init_P();

	curl_easy_setopt_P( curl_handle, CURLOPT_FOLLOWLOCATION, 1 );
	curl_easy_setopt_P( curl_handle, CURLOPT_URL, "http://api.flickr.com/services/rest/?method=flickr.test.echo&name=value" );
	curl_easy_setopt_P( curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback );
	curl_easy_setopt_P( curl_handle, CURLOPT_WRITEDATA, ( void* )&chunk );
	curl_easy_setopt_P( curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0" );

	curl_easy_perform_P( curl_handle );
	curl_easy_cleanup_P( curl_handle );

	if ( chunk.memory )
	{
		chunk.memory[chunk.size - 1] = 0;
		FILE* in_mem = fopen( "getinmem.txt", "w" );
		fwrite( chunk.memory, 1, chunk.size, in_mem );
		fclose( in_mem );
		free( chunk.memory );
	}

	curl_global_cleanup_P();
	return 0;
}

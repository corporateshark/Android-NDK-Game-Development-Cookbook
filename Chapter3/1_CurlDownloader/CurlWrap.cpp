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

#include "CurlWrap.h"

#ifdef _WIN32
#include <windows.h>
#endif

#define GetCurlProc(Name) \
   /*printf("Loading function %s\n", #Name);*/ \
   Name##_P = (Name##_func)GetProcAddress(g_CurlLibrary, #Name); \
   if(!(Name##_P)) return false;

/// cURL function pointers

/// Base

curl_formadd_func curl_formadd_P;
curl_formget_func curl_formget_P;
curl_formfree_func curl_formfree_P;
curl_getenv_func curl_getenv_P;
curl_version_func curl_version_P;
curl_easy_escape_func curl_easy_escape_P;
curl_escape_func curl_escape_P;
curl_easy_unescape_func curl_easy_unescape_P;
curl_unescape_func curl_unescape_P;
curl_free_func curl_free_P;
curl_global_init_func curl_global_init_P;
curl_global_init_mem_func curl_global_init_mem_P;
curl_global_cleanup_func curl_global_cleanup_P;
curl_slist_append_func curl_slist_append_P;
curl_slist_free_all_func curl_slist_free_all_P;
curl_getdate_func curl_getdate_P;
curl_share_init_func curl_share_init_P;
curl_share_setopt_func curl_share_setopt_P;
curl_share_cleanup_func curl_share_cleanup_P;
curl_version_info_func curl_version_info_P;
curl_easy_strerror_func curl_easy_strerror_P;
curl_share_strerror_func curl_share_strerror_P;
curl_easy_pause_func curl_easy_pause_P;

/// Easy

curl_easy_init_func    curl_easy_init_P;
curl_easy_setopt_func  curl_easy_setopt_P;
curl_easy_perform_func curl_easy_perform_P;
curl_easy_cleanup_func curl_easy_cleanup_P;
curl_easy_getinfo_func curl_easy_getinfo_P;
curl_easy_duphandle_func curl_easy_duphandle_P;
curl_easy_reset_func curl_easy_reset_P;
curl_easy_recv_func curl_easy_recv_P;
curl_easy_send_func curl_easy_send_P;

#ifdef CURL_DYNAMIC_LINK
/// Library handle
HINSTANCE g_CurlLibrary = NULL;
#endif

int Curl_Initialized = 0;

bool Curl_Load()
{
#ifdef CURL_DYNAMIC_LINK

	if ( !g_CurlLibrary )
	{
#ifdef _WIN64
		g_CurlLibrary = LoadLibrary( "libcurl64.dll" );
#else
# ifdef _WIN32
		g_CurlLibrary = LoadLibrary( "libcurl32.dll" );
# endif
#endif
	}

	/*
	Env->Logger->LogP(L_DEBUG, "Loading function %s", "curl_formadd");
	curl_formadd_P = (curl_formadd_func)g_CurlLibrary->GetProcAddress("curl_formadd");
	//if(!(Name##_P)) return false;
	*/
	GetCurlProc( curl_formadd )
	GetCurlProc( curl_formget )
	GetCurlProc( curl_formfree )
	GetCurlProc( curl_getenv )
	GetCurlProc( curl_version )
	GetCurlProc( curl_easy_escape )
	GetCurlProc( curl_escape )
	GetCurlProc( curl_easy_unescape )
	GetCurlProc( curl_unescape )
	GetCurlProc( curl_free )
	GetCurlProc( curl_global_init )
	GetCurlProc( curl_global_init_mem )
	GetCurlProc( curl_global_cleanup )
	GetCurlProc( curl_slist_append )
	GetCurlProc( curl_slist_free_all )
	GetCurlProc( curl_getdate )
	GetCurlProc( curl_share_init )
	GetCurlProc( curl_share_setopt )
	GetCurlProc( curl_share_cleanup )
	GetCurlProc( curl_version_info )
	GetCurlProc( curl_easy_strerror )
	GetCurlProc( curl_share_strerror )
	GetCurlProc( curl_easy_pause )

	GetCurlProc( curl_easy_init )
	GetCurlProc( curl_easy_setopt )
	GetCurlProc( curl_easy_perform )
	GetCurlProc( curl_easy_getinfo )
	GetCurlProc( curl_easy_duphandle )
	GetCurlProc( curl_easy_reset )
	GetCurlProc( curl_easy_recv )
	GetCurlProc( curl_easy_send )
	GetCurlProc( curl_easy_cleanup )
#else

	curl_formadd_P = &curl_formadd;
	curl_formget_P = &curl_formget;
	curl_formfree_P = &curl_formfree;
	curl_getenv_P = &curl_getenv;
	curl_version_P = &curl_version;
	curl_easy_escape_P = &curl_easy_escape;
	curl_escape_P = &curl_escape;
	curl_easy_unescape_P = &curl_easy_unescape;
	curl_unescape_P = &curl_unescape;
	curl_free_P = &curl_free;
	curl_global_init_P = &curl_global_init;
	curl_global_init_mem_P = &curl_global_init_mem;
	curl_global_cleanup_P = &curl_global_cleanup;
	curl_slist_append_P = &curl_slist_append;
	curl_slist_free_all_P = &curl_slist_free_all;
	curl_getdate_P = &curl_getdate;
	curl_share_init_P = &curl_share_init;
	curl_share_setopt_P = &curl_share_setopt;
	curl_share_cleanup_P = &curl_share_cleanup;
	curl_version_info_P = &curl_version_info;
	curl_easy_strerror_P = &curl_easy_strerror;
	curl_share_strerror_P = &curl_share_strerror;
	curl_easy_pause_P = &curl_easy_pause;

	curl_easy_init_P      = &curl_easy_init;
	curl_easy_setopt_P    = &curl_easy_setopt;
	curl_easy_perform_P   = &curl_easy_perform;
	curl_easy_cleanup_P   = &curl_easy_cleanup;
	curl_easy_getinfo_P   = &curl_easy_getinfo;
	curl_easy_duphandle_P = &curl_easy_duphandle;
	curl_easy_reset_P     = &curl_easy_reset;
	curl_easy_recv_P      = &curl_easy_recv;
	curl_easy_send_P      = &curl_easy_send;
#endif

	Curl_Initialized = 1;

	return true;
}

bool Curl_Unload()
{
	if ( !Curl_Initialized ) { return false; }

#ifdef CURL_DYNAMIC_LINK


	if ( g_CurlLibrary )
	{
		CloseHandle( g_CurlLibrary );
		g_CurlLibrary = NULL;
	}

#endif

	Curl_Initialized = 0;

	return true;
}

#undef GetCurlProc

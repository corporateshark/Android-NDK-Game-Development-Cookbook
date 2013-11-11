/*
 * Copyright (C) 2013 Sergey Kosarevsky (sk@linderdaum.com)
 * Copyright (C) 2013 Viktor Latypov (vl@linderdaum.com)
 * Based on Linderdaum Engine http://www.linderdaum.com
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

#include "ft_load.h"

#if defined(_WIN32)
#  include <windows.h>
#  include <stdio.h>
#endif

FT_Init_FreeType_func FT_Init_FreeTypePTR;

FT_Activate_Size_func FT_Activate_SizePTR;
FT_Done_FreeType_func FT_Done_FreeTypePTR;

FT_New_Face_func FT_New_FacePTR;
FT_New_Memory_Face_func FT_New_Memory_FacePTR;

FT_Glyph_Get_CBox_func FT_Glyph_Get_CBoxPTR;
FT_Get_Kerning_func FT_Get_KerningPTR;

FTC_Manager_New_func FTC_Manager_NewPTR;
FTC_Manager_Done_func FTC_Manager_DonePTR;

FTC_ImageCache_New_func FTC_ImageCache_NewPTR;
FTC_ImageCache_Lookup_func FTC_ImageCache_LookupPTR;

FTC_CMapCache_New_func FTC_CMapCache_NewPTR;
FTC_CMapCache_Lookup_func FTC_CMapCache_LookupPTR;

FTC_Node_Unref_func FTC_Node_UnrefPTR;

FTC_Manager_LookupFace_func FTC_Manager_LookupFacePTR;
FTC_Manager_LookupSize_func FTC_Manager_LookupSizePTR;

// HMODULE for Windows
void* hFTDll = NULL;

/// Number of initializations for multiple instances of clTextRenderer
int FTLoadCount = 0;

bool LoadFT()
{
	if ( hFTDll != NULL ) { return true; }

#if defined(_WIN32)

#define LOAD_FT_FUNC(Func_) \
   Func_##PTR = (Func_##_func) GetProcAddress( (HMODULE)hFTDll, #Func_ ); \
   if(! Func_##PTR) { printf("Cannot load %s function !\n", #Func_ ); return false; }

#ifdef _WIN64
	const char* LibName = "libfreetype-6-64.dll";
#else
	const char* LibName = "libfreetype-6-32.dll";
#endif

	*( ( HMODULE* )&hFTDll ) = LoadLibrary( LibName );

	if ( hFTDll == INVALID_HANDLE_VALUE )
	{
		printf( "Cannot load FreeType dll !\n" );
		return false;
	}

#else

#    define LOAD_FT_FUNC(Func_) Func_##PTR = &Func_;

#endif

	LOAD_FT_FUNC( FT_Init_FreeType )

	LOAD_FT_FUNC( FT_Activate_Size )
	LOAD_FT_FUNC( FT_Done_FreeType )

	LOAD_FT_FUNC( FT_New_Face )
	LOAD_FT_FUNC( FT_New_Memory_Face )

	LOAD_FT_FUNC( FT_Glyph_Get_CBox )
	LOAD_FT_FUNC( FT_Get_Kerning )

	LOAD_FT_FUNC( FTC_Manager_New )
	LOAD_FT_FUNC( FTC_Manager_Done )

	LOAD_FT_FUNC( FTC_ImageCache_New )
	LOAD_FT_FUNC( FTC_ImageCache_Lookup )

	LOAD_FT_FUNC( FTC_CMapCache_New )
	LOAD_FT_FUNC( FTC_CMapCache_Lookup )

	LOAD_FT_FUNC( FTC_Node_Unref )

	LOAD_FT_FUNC( FTC_Manager_LookupFace )
	LOAD_FT_FUNC( FTC_Manager_LookupSize )

#undef LOAD_FT_FUNC

	FTLoadCount++;

	return true;
}

void UnloadFT()
{
	if ( hFTDll == NULL ) { return; }

	FTLoadCount--;

	if ( FTLoadCount > 0 ) { return; }

#if defined(_WIN32)
	CloseHandle( hFTDll );
#endif
	hFTDll = NULL;
}

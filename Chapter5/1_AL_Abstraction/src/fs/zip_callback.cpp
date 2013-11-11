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

static voidpf ZCALLBACK zip_fopen ( voidpf opaque, const void* filename, int mode )
{
	( ( iIStream* )opaque )->Seek( 0 );
	( void )filename;
	( void )mode;
	return opaque;
}

static uLong ZCALLBACK zip_fread ( voidpf opaque, voidpf stream, void* buf, uLong size )
{
	iIStream* S = ( iIStream* )stream;
	int64 CanRead = ( int64 )size;
	int64 Sz = S->GetSize();
	int64 Ps = S->GetPos();

	if ( CanRead + Ps >= Sz ) { CanRead = Sz - Ps; }

	if ( CanRead > 0 ) {  S->Read( buf, ( uint64 )CanRead ); }
	else { CanRead = 0; }

	return ( uLong )CanRead;
}

static ZPOS64_T ZCALLBACK zip_ftell ( voidpf opaque, voidpf stream )
{
	return ( ZPOS64_T )( ( iIStream* )stream )->GetPos();
}

static long ZCALLBACK zip_fseek ( voidpf  opaque, voidpf stream, ZPOS64_T offset, int origin )
{
	iIStream* S = ( iIStream* )stream;
	int64 NewPos = ( int64 )offset;
	int64 Sz = ( int64 )S->GetSize();

	switch ( origin )
	{
		case ZLIB_FILEFUNC_SEEK_CUR:
			NewPos += ( int64 )S->GetPos();
			break;

		case ZLIB_FILEFUNC_SEEK_END:
			NewPos = Sz - 1 - NewPos;
			break;

		case ZLIB_FILEFUNC_SEEK_SET:
			break;

		default:
			return -1;
	}

	if ( NewPos >= 0 && ( NewPos < Sz ) ) { S->Seek( ( uint64 )NewPos ); }
	else { return -1; }

	return 0;
}

static int ZCALLBACK zip_fclose ( voidpf opaque, voidpf stream ) { return 0; }
static int ZCALLBACK zip_ferror ( voidpf opaque, voidpf stream ) { return 0; }

void fill_functions( iIStream* Stream, zlib_filefunc64_def*  pzlib_filefunc_def )
{
	pzlib_filefunc_def->zopen64_file = zip_fopen;
	pzlib_filefunc_def->zread_file = zip_fread;
	pzlib_filefunc_def->zwrite_file = NULL;
	pzlib_filefunc_def->ztell64_file = zip_ftell;
	pzlib_filefunc_def->zseek64_file = zip_fseek;
	pzlib_filefunc_def->zclose_file = zip_fclose;
	pzlib_filefunc_def->zerror_file = zip_ferror;
	pzlib_filefunc_def->opaque = Stream;
}

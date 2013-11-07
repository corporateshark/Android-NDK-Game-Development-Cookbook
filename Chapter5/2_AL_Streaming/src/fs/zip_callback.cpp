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

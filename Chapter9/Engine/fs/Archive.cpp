/**
 * \file Archive.cpp
 * \brief Archive reader
 * \author Viktor Latypov, 2011-2012
 * \author viktor@linderdaum.com
 */

#include <stdio.h>

#include "Files.h"
#include "FileSystem.h"
#include "Archive.h"

#include "libcompress.h"

#include "MountPoint.h"

// dummy error handler for the bzip2 library
extern "C" void bz_internal_error( int e_code ) { ( void )e_code; }

bool ArchiveReader::CloseArchive()
{
	/// Clear containers
	FFileInfos.clear();
	FFileInfoIdx.clear();
	FFileNames.clear();
	FRealFileNames.clear();
	ClearExtracted();

	FSourceFile = NULL;

	return true;
}

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

/*
   .ZIP reading code is based on the following source:

   sample part of the MiniZip project - ( http://www.winimage.com/zLibDll/minizip.html )
   Copyright (C) 1998-2010 Gilles Vollant (minizip) ( http://www.winimage.com/zLibDll/minizip.html )
   Copyright (C) 2007-2008 Even Rouault
   Copyright (C) 2009-2010 Mathias Svensson ( http://result42.com )
*/

#define WRITEBUFFERSIZE (8192)

/// Internal routine to extract a single file from ZIP archive
int ExtractCurrentFile_ZIP( unzFile uf, const char* password, int* abort_flag, float* progress, const clPtr<iOStream>& fout )
{
	char filename_inzip[256];
	int err = UNZ_OK;
	void* buf;
	uInt size_buf;
	unz_file_info64 file_info;

	err = unzGetCurrentFileInfo64( uf, &file_info, filename_inzip, sizeof( filename_inzip ), NULL, 0, NULL, 0 );

	if ( err != UNZ_OK ) { return err; }

	uint64 file_size = ( uint64 )file_info.uncompressed_size;
	uint64 total_bytes = 0;

	unsigned char _buf[WRITEBUFFERSIZE];
	size_buf = WRITEBUFFERSIZE;
	buf = ( void* )_buf;

	err = unzOpenCurrentFilePassword( uf, password );

	if ( err != UNZ_OK ) { return err; }

	do
	{
		err = unzReadCurrentFile( uf, buf, size_buf );

		if ( err < 0 ) { break; }

		if ( err > 0 ) { total_bytes += err; fout->Write( buf, err ); }
	}
	while ( err > 0 );

	int close_err = unzCloseCurrentFile ( uf );

	if ( close_err != UNZ_OK ) { return close_err; }

	return err;
}

bool ArchiveReader::Enumerate_ZIP()
{
	clPtr<iIStream> TheSource = FSourceFile;
	FSourceFile->Seek( 0 );

	zlib_filefunc64_def ffunc;
	fill_functions( TheSource.GetInternalPtr(), &ffunc );

	unzFile uf = unzOpen2_64( "", &ffunc );

	unz_global_info64 gi;
	int err = unzGetGlobalInfo64( uf, &gi );

	for ( uLong i = 0; i < gi.number_entry; i++ )
	{
		char filename_inzip[256];
		unz_file_info64 file_info;
		err = unzGetCurrentFileInfo64( uf, &file_info, filename_inzip, sizeof( filename_inzip ), NULL, 0, NULL, 0 );

		if ( err != UNZ_OK ) { break; }

		if ( ( i + 1 ) < gi.number_entry )
		{
			err = unzGoToNextFile( uf );

			// WARNING: "error %d with zipfile in unzGoToNextFile\n", err
			if ( err != UNZ_OK ) { break; }
		}

		sFileInfo Info;
		Info.FOffset = 0;
		Info.FCompressedSize = file_info.compressed_size;
		Info.FSize = file_info.uncompressed_size;
		FFileInfos.push_back( Info );

		std::string TheName = Arch_FixFileName( filename_inzip );
		FFileInfoIdx[TheName] = ( int )FFileNames.size();
		FFileNames.push_back( TheName );
		FRealFileNames.push_back( std::string( filename_inzip ) );
	}

	unzClose( uf );
	return true;
}

/// end of .ZIP stuff

bool ArchiveReader::ExtractSingleFile( const std::string& FName, const std::string& Password, int* AbortFlag, float* Progress, const clPtr<iOStream>& FOut )
{
	int err = UNZ_OK;

	std::string ZipName = FName;
	std::replace( ZipName.begin(), ZipName.end(), '\\', '/' );

	clPtr<iIStream> TheSource = FSourceFile;
	FSourceFile->Seek( 0 );

	/// Decompress the data
	zlib_filefunc64_def ffunc;
	fill_functions( TheSource.GetInternalPtr(), &ffunc );

	unzFile uf = unzOpen2_64( "", &ffunc );

	if ( unzLocateFile( uf, ZipName.c_str(), 0/*CASESENSITIVITY - insensitive*/ ) != UNZ_OK )
	{
		// WARNING: "File %s not found in the zipfile\n", FName.c_str()
		return false;
	}

	err = ExtractCurrentFile_ZIP( uf, Password.empty() ? NULL : Password.c_str(), AbortFlag, Progress, FOut );

	unzClose( uf );

	return ( err == UNZ_OK );
}

const void* ArchiveReader::GetFileData_ZIP( size_t idx )
{
	if ( FExtractedFromArchive.count( ( int )idx ) > 0 ) { return FExtractedFromArchive[( int )idx]; }

	clPtr<MemFileWriter> FOut = CreateMemWriter( "mem_blob", FFileInfos[idx].FSize );

	void* DataPtr = NULL;

	if ( ExtractSingleFile( FRealFileNames[idx], "", NULL, NULL, FOut ) )
	{
		size_t Sz = static_cast<size_t>( FOut->GetFilePos() );

		clPtr<clBlob> B = FOut->GetContainer();
		B->SafeResize( Sz );

		const void* BlobPtr = B->GetDataConst();
		DataPtr = AllocData( Sz );

		::memcpy( DataPtr, BlobPtr, Sz );
	}

	return DataPtr;
}

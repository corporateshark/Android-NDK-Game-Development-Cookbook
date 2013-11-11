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

#include "zip_callback.cpp"

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

		clPtr<Blob> B = FOut->GetContainer();
		B->SafeResize( Sz );

		const void* BlobPtr = B->GetDataConst();
		DataPtr = AllocData( Sz );

		::memcpy( DataPtr, BlobPtr, Sz );
	}

	return DataPtr;
}

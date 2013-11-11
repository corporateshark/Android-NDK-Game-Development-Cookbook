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

#pragma once

#include "Streams.h"
#include <map>
#include <vector>

/// Encapsulation of .zip archive management
class ArchiveReader: public iObject
{
private:
	/// Internal file information
	struct sFileInfo
	{
		/// Offset to the file
		uint64 FOffset;
		/// (Uncompressed) File size
		uint64 FSize;
		/// Compressed file size
		uint64 FCompressedSize;
		/// Compressed data
		void* FSourceData;
	};

	/// File information
	std::vector<sFileInfo> FFileInfos;

	/// File names (adapted for our VFS - upper case, slash-separated path)
	std::vector<std::string>   FFileNames;

	/// Real (in-archive) file names
	std::vector<std::string>   FRealFileNames;

	/// File information indices (name to index map)
	mutable std::map<std::string, int> FFileInfoIdx;

	/// Source file
	clPtr<iIStream> FSourceFile;
public:
	ArchiveReader(): FSourceFile( NULL ) {}
	virtual ~ArchiveReader() { CloseArchive(); }

	/// Assign the source stream and set the internal ownership flag
	bool    OpenArchive( const clPtr<iIStream>& Source )
	{
		if ( !CloseArchive() ) { return false; }

		FSourceFile = Source;

		return ( !FSourceFile ) ? false : Enumerate_ZIP();
	}

	/// Extract a file from archive
	bool    ExtractSingleFile( const std::string& FName, const std::string& Password, int* AbortFlag, float* Progress, const clPtr<iOStream>& FOut );

	/// Free everything and optionally close the source stream
	bool    CloseArchive();

	clPtr<iIStream> GetSourceFile() const { return FSourceFile; }

	/// Check if such a file exists in the archive
	bool    FileExists( const std::string& FileName ) const { return ( GetFileIdx( FileName ) > -1 ); }

	/// Get the size of file
	uint64 GetFileSize( const std::string& FileName ) const
	{
		int idx = GetFileIdx( FileName );
		return ( idx > -1 ) ? FFileInfos[idx].FSize : 0;
	}

	/// Get the data for this file
	const void*   GetFileData( const std::string& FileName )
	{
		int idx = GetFileIdx( FileName );
		const void* DataPtr = NULL;

		if ( idx > -1 )
		{
			/// Check if we already have this data in cache
			if ( FExtractedFromArchive.count( idx ) > 0 ) { return FExtractedFromArchive[idx]; }

			/// Decompress/extract the data
			DataPtr = GetFileData_ZIP( idx );

			if ( DataPtr ) { FExtractedFromArchive[idx] = DataPtr; }
		}

		return DataPtr;
	}

	/// Convert file name to an internal linear index
	int     GetFileIdx( const std::string& FileName ) const { return ( FFileInfoIdx.count( FileName ) > 0 ) ? FFileInfoIdx[FileName] : -1; }

	/// Get the number of files in archive
	size_t  GetNumFiles() const { return FFileInfos.size(); }

	/// Get i-th file name in archive
	std::string GetFileName( int idx ) { return FFileNames[idx]; }
public:
	/// The number of files in archive, available soon after the Enumerate() is called
	int FTotalFiles;

private:
	/// Internal function to enumerate the files in archive
	bool Enumerate_ZIP();

	const void* GetFileData_ZIP( size_t idx );

	/// Remove each extracted file from tmp container
	void ClearExtracted()
	{
		for ( std::map<int, const void*>::iterator i = FExtractedFromArchive.begin() ; i != FExtractedFromArchive.end() ; i++ )
		{
			this->FreeData( const_cast<void*>( i->second ) );
		}

		FExtractedFromArchive.clear();
	}

	inline void* AllocData( size_t Sz ) { return ::malloc( Sz ); }

	inline void FreeData( void* DataPtr ) { ::free( DataPtr ); }

	/// Temporary cache for the extracted files. Removed on CloseArchive() call
	std::map<int, const void*> FExtractedFromArchive;
};

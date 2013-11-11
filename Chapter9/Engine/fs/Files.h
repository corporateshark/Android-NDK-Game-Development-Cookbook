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
#include "Blob.h"

#ifdef _WIN32
#  include <windows.h>
#else
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <sys/mman.h>
#  include <fcntl.h>
#  include <errno.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

class iRawFile: public iObject
{
public:
	iRawFile() {};
	virtual ~iRawFile() {};

	void       SetVirtualFileName( const std::string& VFName ) { FVirtualFileName = VFName; }
	void       SetFileName( const std::string& FName )         { FFileName        = FName;  }

	std::string    GetVirtualFileName() const { return FVirtualFileName; }
	std::string    GetFileName()        const { return FFileName; }

	virtual const ubyte*    GetFileData() const = 0;
	virtual uint64          GetFileSize() const = 0;
protected:
	std::string    FFileName;
	std::string    FVirtualFileName;
};

/// Physical file representation
class RawFile: public iRawFile
{
public:
	RawFile() {}
	virtual ~RawFile() { Close(); }

	bool Open( const std::string& FileName, const std::string& VirtualFileName )
	{
		SetFileName( FileName );
		SetVirtualFileName( VirtualFileName );

		FSize = 0;
		FFileData = NULL;

#ifdef _WIN32
		FMapFile = CreateFileA( FFileName.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
		                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, NULL );
		// FMapFile == INVALID_HANDLE_VALUE ?
#else
		FFileHandle = open( FileName.c_str(), O_RDONLY );
#endif

#ifdef _WIN32
		FMapHandle = CreateFileMapping( FMapFile, NULL, PAGE_READONLY, 0, 0, NULL );
		FFileData = ( ubyte* )MapViewOfFile( FMapHandle, FILE_MAP_READ, 0, 0, 0 );

		DWORD dwSizeLow = 0, dwSizeHigh = 0;
		dwSizeLow = ::GetFileSize( FMapFile, &dwSizeHigh );
		FSize = ( ( uint64 )dwSizeHigh << 32 ) | ( uint64 )dwSizeLow;
#else
		struct stat FileInfo;
		fstat( FFileHandle, &FileInfo );
		// Optionally check strerror( errno ), if fstat returns -1

		FSize = static_cast<uint64>( FileInfo.st_size );

		// don't call mmap() for zero-sized files
		if ( FSize )
		{
			// create share/read-only file mapping
			FFileData = ( ubyte* )( mmap( NULL, FSize, PROT_READ, MAP_PRIVATE, FFileHandle, 0 ) );
		}

		close( FFileHandle );
#endif
		return true;
	}

	void Close()
	{
#ifdef _WIN32

		if ( FFileData  ) { UnmapViewOfFile( FFileData ); }

		if ( FMapHandle ) { CloseHandle( ( HANDLE )FMapHandle ); }

		CloseHandle( ( HANDLE )FMapFile );
#else

		if ( FFileData ) { munmap( reinterpret_cast<void*>( FFileData ), FSize ); }

#endif
	}

	virtual const ubyte* GetFileData()        const { return FFileData; }
	virtual uint64       GetFileSize()        const { return FSize;     }
private:
#ifdef _WIN32
	HANDLE     FMapFile;
	HANDLE     FMapHandle;
#else
	int        FFileHandle;
#endif
	ubyte*    FFileData;
	uint64    FSize;
};

class MemRawFile: public iRawFile
{
public:
	MemRawFile()
	{
		FBuffer      = NULL;
		FBufferSize  = 0;
		FOwnsBuffer  = false;
	}

	virtual ~MemRawFile() { DeleteBuffer(); }

	virtual const ubyte* GetFileData() const { return reinterpret_cast<const ubyte*>( FBuffer ); }
	virtual uint64       GetFileSize() const { return FBufferSize; }

	void CreateFromString( const std::string& InString )
	{
		DeleteBuffer();
		FBufferSize = InString.length();

		if ( !InString.empty() )
		{
			char* LocalBuffer = new char[ InString.length() ];
			memcpy( LocalBuffer, InString.c_str(), static_cast<size_t>( FBufferSize ) );
			FBuffer = LocalBuffer;
			FOwnsBuffer = true;
		}
	}

	void CreateFromBuffer( const void* BufPtr, uint64 BufSize )
	{
		DeleteBuffer();
		FBuffer     = BufPtr;
		FBufferSize = BufSize;
		FOwnsBuffer = true;
	}

	void CreateFromManagedBuffer( const void* BufPtr, uint64 BufSize )
	{
		DeleteBuffer();
		FBuffer     = BufPtr;
		FBufferSize = BufSize;
		FOwnsBuffer = false;
	}

private:
	void DeleteBuffer()
	{
		if ( FBuffer )
		{
			if ( FOwnsBuffer )
			{
				delete [] reinterpret_cast<const char*>( FBuffer );
			}

			FBuffer = NULL;
		}
	}

	// do we own the buffer ?
	bool           FOwnsBuffer;
	const void*    FBuffer;
	uint64        FBufferSize;
};

class ManagedMemRawFile: public iRawFile
{
public:
	ManagedMemRawFile(): FBlob( NULL ) {}

	virtual const ubyte* GetFileData() const { return ( const ubyte* )FBlob->GetData(); }
	virtual uint64       GetFileSize() const { return FBlob->GetSize(); }

	void SetBlob( const clPtr<clBlob>& Ptr ) { FBlob = Ptr; }
private:
	clPtr<clBlob> FBlob;
};

class FileMapper: public iIStream
{
public:
	FileMapper( clPtr<iRawFile> File ): FFile( File ), FPosition( 0 ) {}
	virtual ~FileMapper() {}

	virtual std::string  GetVirtualFileName() const { return FFile->GetVirtualFileName(); }
	virtual std::string  GetFileName() const { return FFile->GetFileName(); }
	virtual uint64       Read( void* Buf, uint64 Size )
	{
		uint64 RealSize = ( Size > GetBytesLeft() ) ? GetBytesLeft() : Size;

		memcpy( Buf, ( FFile->GetFileData() + FPosition ), static_cast<size_t>( RealSize ) );

		FPosition += RealSize;

		return RealSize;
	}

	virtual void         Seek( const uint64 Position )     { FPosition  = Position; }

	virtual uint64       GetSize() const { return FFile->GetFileSize(); }
	virtual uint64       GetPos()  const { return FPosition; }
	virtual bool         Eof() const         { return ( FPosition >= FFile->GetFileSize() ); }

	virtual const ubyte* MapStream()   const { return FFile->GetFileData(); }
	virtual const ubyte* MapStreamFromCurrentPos() const { return ( FFile->GetFileData() + FPosition ); }

	virtual std::string ReadLine()
	{
		const size_t MAX_LINE_WIDTH = 65535;

		char Buf[ MAX_LINE_WIDTH + 1 ];

		const ubyte* C = MapStreamFromCurrentPos();
		char* Out      = Buf;
		char* End      = Buf + MAX_LINE_WIDTH;

		while ( !Eof() && Out < End )
		{
			FPosition++;

			char Ch = ( *C++ );

			if ( Ch == 13   ) { continue; }   // kill char

			if ( Ch == 10   ) { break; }

			*Out++ = Ch;
		}

		( *Out ) = 0;

		return std::string( Buf );
	}

private:
	clPtr<iRawFile> FFile;
	uint64          FPosition;
};

class FileWriter: public iOStream
{
public:
	FileWriter(): FPosition( 0 ) {}
	virtual ~FileWriter() { Close(); }

	bool Open( const std::string& FileName )
	{
		FFileName = FileName;
#ifdef _WIN32
		FMapFile = CreateFile( FFileName.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL,
		                       CREATE_ALWAYS,
		                       FILE_ATTRIBUTE_NORMAL, NULL );

		return !( FMapFile == ( void* )INVALID_HANDLE_VALUE );
#else
		FMapFile = open( FFileName.c_str(), O_WRONLY | O_CREAT );
		FPosition = 0;
		return !( FMapFile == -1 );
#endif
	}

	void Close()
	{
#ifdef _WIN32
		CloseHandle( FMapFile );
#else

		if ( FMapFile != -1 ) { close( FMapFile ); }

#endif
	}

	virtual std::string GetFileName() const { return FFileName; }
	virtual uint64      GetFilePos() const { return FPosition; }
	virtual void        Seek( const uint64 Position )
	{
#ifdef _WIN32
		SetFilePointerEx( FMapFile, *reinterpret_cast<const LARGE_INTEGER*>( &Position ), NULL, FILE_BEGIN );
#else

		if ( FMapFile != -1 ) { lseek( FMapFile, Position, SEEK_SET ); }

#endif
		FPosition = Position;
	}

	virtual uint64      Write( const void* Buf, const uint64 Size )
	{
#ifdef _WIN32
		DWORD written;
		WriteFile( FMapFile, Buf, DWORD( Size ), &written, NULL );
#else

		if ( FMapFile != -1 ) { write( FMapFile, Buf, Size ); }

#endif
		FPosition += Size;
		return Size;
	}

private:
	std::string FFileName;
#ifdef _WIN32
	HANDLE FMapFile;
#else
	int    FMapFile;
#endif
	uint64    FPosition;
};

/// File writer for some dynamically-sized clBlob
class MemFileWriter: public iOStream
{
public:
	MemFileWriter( clPtr<clBlob> Container ): FPosition( 0 ), FMaxSize(), FContainer( Container ), FFileName( "" ) {}
	virtual ~MemFileWriter() {}

	virtual void SetFileName( const std::string& FName ) { FFileName = FName; }
	virtual std::string GetFileName() const { return FFileName; }
	virtual uint64 GetFilePos() const { return FPosition; }

	/// Get/Set maximum allowed size for the in-mem file
	uint64 GetMaxSize() const { return FMaxSize; }
	void   SetMaxSize( uint64 MaxSize ) { FMaxSize = MaxSize; }

	/// Change absolute file position to Position
	virtual void    Seek( const uint64 Position )
	{
		if ( Position > FContainer->GetSize() )
		{
			/// Check for oversize
			if ( Position > FMaxSize - 1 ) { return; }

			/// Resize the clBlob
			if ( !FContainer->SafeResize( static_cast<size_t>( Position ) + 1 ) ) { return; }
		}

		FPosition = Position;
	}

	/// Write Size bytes from Buf
	virtual uint64    Write( const void* Buf, const uint64 Size )
	{
		uint64 ThisPos = FPosition;

		/// Ensure there is enough space
		Seek( ThisPos + Size );

		if ( FPosition + Size > FMaxSize ) { return 0; }

		void* DestPtr = ( void* )( &( ( ( ubyte* )( FContainer->GetData() ) )[ThisPos] ) );

		/// Write the data
		memcpy( DestPtr, Buf, static_cast<size_t>( Size ) );

		return Size;
	}

	/// Access internal data container
	clPtr<clBlob> GetContainer() const { return FContainer; }
	void          SetContainer( const clPtr<clBlob>& B ) { FContainer = B; }
private:
	/// Actual file contents
	clPtr<clBlob> FContainer;
	/// Maximum allowable size
	uint64 FMaxSize;
	/// Virtual file name
	std::string FFileName;
	/// Current position
	uint64 FPosition;
};

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

#ifndef _FileSystem_
#define _FileSystem_

#include "Files.h"

#include <vector>

class iMountPoint;

class FileSystem: public iObject
{
public:
	FileSystem() {}
	virtual ~FileSystem() {}

	clPtr<iIStream> CreateReader( const std::string& FileName ) const;
	clPtr<iIStream> ReaderFromString( const std::string& Str ) const;
	clPtr<iIStream> ReaderFromMemory( const void* BufPtr, uint64 BufSize, bool OwnsData ) const;
	clPtr<iIStream> ReaderFromBlob( const clPtr<Blob>& Blob ) const;

	clPtr<Blob> LoadFileAsBlob( const std::string& FName ) const
	{
		clPtr<iIStream> input = CreateReader( FName );
		clPtr<Blob> Res = new Blob();
		Res->CopyMemoryBlock( input->MapStream(), input->GetSize() );
		return Res;
	}

	void        Mount( const std::string& PhysicalPath );
	void        AddAliasMountPoint( const std::string& SrcPath, const std::string& AliasPrefix );
	void        AddMountPoint( const clPtr<iMountPoint>& MP );

	std::string VirtualNameToPhysical( const std::string& Path ) const;
	bool        FileExists( const std::string& Name ) const;
private:
	clPtr<iMountPoint> FindMountPointByName( const std::string& ThePath );
	/// Search for a mount point for this file
	clPtr<iMountPoint>  FindMountPoint( const std::string& FileName ) const;
	std::vector< clPtr<iMountPoint> > FMountPoints;
};

inline clPtr<MemFileWriter> CreateMemWriter( const std::string& FileName, uint64 InitialSize )
{
	clPtr<Blob> B = new Blob();
	B->SetSize( static_cast<size_t>( InitialSize ) );

	clPtr<MemFileWriter> Stream = new MemFileWriter( B );
	Stream->SetMaxSize( InitialSize * 2 ); // default value
	Stream->SetFileName( FileName );

	return Stream;
}

#endif

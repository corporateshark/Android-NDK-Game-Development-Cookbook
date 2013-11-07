#pragma once

#include "Files.h"

#include <vector>

class iMountPoint;

class clFileSystem: public iObject
{
public:
	clFileSystem() {}
	virtual ~clFileSystem() {}

	clPtr<iIStream> CreateReader( const std::string& FileName ) const;
	clPtr<iIStream> ReaderFromString( const std::string& Str ) const;
	clPtr<iIStream> ReaderFromMemory( const void* BufPtr, uint64 BufSize, bool OwnsData ) const;
	clPtr<iIStream> ReaderFromBlob( const clPtr<clBlob>& Blob ) const;

	clPtr<clBlob> LoadFileAsBlob( const std::string& FName ) const
	{
		clPtr<iIStream> input = CreateReader( FName );
		clPtr<clBlob> Res = new clBlob();
		Res->CopyMemoryBlock( input->MapStream(), ( size_t )input->GetSize() );
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
	clPtr<clBlob> B = new clBlob();
	B->SetSize( static_cast<size_t>( InitialSize ) );

	clPtr<MemFileWriter> Stream = new MemFileWriter( B );
	Stream->SetMaxSize( InitialSize * 2 ); // default value
	Stream->SetFileName( FileName );

	return Stream;
}

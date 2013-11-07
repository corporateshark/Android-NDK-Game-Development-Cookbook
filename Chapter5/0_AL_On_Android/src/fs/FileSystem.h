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

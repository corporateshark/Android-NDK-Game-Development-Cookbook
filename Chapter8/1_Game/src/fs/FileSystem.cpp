#include "FileSystem.h"
#include "Files.h"
#include "MountPoint.h"
#include "Archive.h"

#include "Blob.h"

#include <algorithm>

#ifdef _WIN32
#  include <direct.h>
#  include <windows.h>
#else  // any POSIX
#  include <dirent.h>
#  include <stdlib.h>
#endif

clPtr<iIStream> clFileSystem::CreateReader( const std::string& FileName ) const
{
	std::string Name = Arch_FixFileName( FileName );

	clPtr<iMountPoint> MountPoint = FindMountPoint( Name );
	clPtr<iRawFile> RAWFile = MountPoint->CreateReader( Name );

	if ( !RAWFile->GetFileData() ) { LOGI( "ERROR: unable to load file %s\n", FileName.c_str() ); }

	return new FileMapper( RAWFile );
}

clPtr<iIStream> clFileSystem::ReaderFromString( const std::string& Str ) const
{
	MemRawFile* RawFile = new MemRawFile();
	RawFile->CreateFromString( Str );
	return new FileMapper( RawFile );
}

clPtr<iIStream> clFileSystem::ReaderFromMemory( const void* BufPtr, uint64 BufSize, bool OwnsData ) const
{
	MemRawFile* RawFile = new MemRawFile();
	OwnsData ? RawFile->CreateFromBuffer( BufPtr, BufSize ) : RawFile->CreateFromManagedBuffer( BufPtr, BufSize );
	return new FileMapper( RawFile );
}

clPtr<iIStream> clFileSystem::ReaderFromBlob( const clPtr<clBlob>& Blob ) const
{
	ManagedMemRawFile* RawFile = new ManagedMemRawFile();
	RawFile->SetBlob( Blob );
	return new FileMapper( RawFile );
}

bool clFileSystem::FileExists( const std::string& Name ) const
{
	if ( Name.empty() || Name == "." ) { return false; }

	clPtr<iMountPoint> MPD = FindMountPoint( Name );
	return MPD ? MPD->FileExists( Name ) : false;
}

std::string clFileSystem::VirtualNameToPhysical( const std::string& Path ) const
{
	if ( FS_IsFullPath( Path ) ) { return Path; }

	clPtr<iMountPoint> MP = FindMountPoint( Path );
	return ( !MP ) ? Path : MP->MapName( Path );
}

void clFileSystem::Mount( const std::string& PhysicalPath )
{
	clPtr<iMountPoint> MPD = NULL;

	if ( PhysicalPath.find( ".apk" ) != std::string::npos || PhysicalPath.find( ".zip" ) != std::string::npos )
	{
		clPtr<ArchiveReader> Reader = new ArchiveReader();

		Reader->OpenArchive( CreateReader( PhysicalPath ) );

		MPD = new ArchiveMountPoint( Reader );
	}
	else
	{
#if !defined( OS_ANDROID )

		if ( !FS_FileExistsPhys( PhysicalPath ) )
		{
			// WARNING: "Unable to mount: '" + PhysicalPath + "' not found"
			return;
		}

#endif
		MPD = new PhysicalMountPoint( PhysicalPath );
	}

	if ( MPD )
	{
		MPD->SetName( PhysicalPath );
		AddMountPoint( MPD );
	}
}

void clFileSystem::AddAliasMountPoint( const std::string& SrcPath, const std::string& AliasPrefix )
{
	clPtr<iMountPoint> MP = FindMountPointByName( SrcPath );

	if ( !MP ) { return; }

	clPtr<AliasMountPoint> AMP = new AliasMountPoint( MP );
	AMP->SetAlias( AliasPrefix );
	AddMountPoint( AMP );
}

clPtr<iMountPoint> clFileSystem::FindMountPointByName( const std::string& ThePath )
{
	for ( size_t i = 0 ; i != FMountPoints.size() ; i++ )
		if ( FMountPoints[i]->GetName() == ThePath ) { return FMountPoints[i]; }

	return NULL;
}

void clFileSystem::AddMountPoint( const clPtr<iMountPoint>& MP )
{
	if ( !MP ) { return; }

	if ( std::find( FMountPoints.begin(), FMountPoints.end(), MP ) == FMountPoints.end() ) { FMountPoints.push_back( MP ); }
}

clPtr<iMountPoint> clFileSystem::FindMountPoint( const std::string& FileName ) const
{
	if ( FMountPoints.empty() ) { return NULL; }

	if ( ( *FMountPoints.begin() )->FileExists( FileName ) )
	{
		return ( *FMountPoints.begin() );
	}

	// reverse order
	for ( std::vector<clPtr<iMountPoint> >::const_reverse_iterator i = FMountPoints.rbegin(); i != FMountPoints.rend(); ++i )
		if ( ( *i )->FileExists( FileName ) ) { return ( *i ); }

	return *( FMountPoints.begin() );
}

#pragma once

#include "Engine.h"
#include "Files.h"
#include "Archive.h"
#include <sys/stat.h>

#include <algorithm>

#if defined( _WIN32 )
const char PATH_SEPARATOR = '\\';
#else
const char PATH_SEPARATOR = '/';
#endif // OS_WINDOWS

inline std::string Arch_FixFileName( const std::string& VName )
{
	std::string s( VName );
	std::replace( s.begin(), s.end(), '\\', PATH_SEPARATOR );
	std::replace( s.begin(), s.end(), '/', PATH_SEPARATOR );
	return s;
}

/// Mount point interface for virtual file system
class iMountPoint: public iObject
{
public:
	iMountPoint(): FName( "" ) {}
	virtual ~iMountPoint() {}

	/// Check if virtual file VirtualName exists at this mount point
	virtual bool         FileExists( const std::string& VirtualName ) const = 0;
	/// Convert local file VirtualName to global name
	virtual std::string      MapName( const std::string& VirtualName ) const = 0;
	/// Create appropriate file reader for the specified VirtualName
	virtual clPtr<iRawFile>  CreateReader( const std::string& VirtualName ) const = 0;

	/// Set internal mount point name
	virtual void    SetName( const std::string& N ) { FName = N; }
	/// Get internal mount point name
	virtual std::string GetName() const { return FName; }
private:
	std::string       FName;
};

inline bool FS_IsFullPath( const std::string& Path )
{
	return ( Path.find( ":\\" ) != std::string::npos ||
#if !defined( _WIN32 )
	         ( Path.length() && Path[0] == '/' ) ||
#endif
	         Path.find( ":/" )  != std::string::npos ||
	         Path.find( ".\\" ) != std::string::npos );
}

inline std::string FS_ValidatePath( const std::string& PathName )
{
	std::string Result = PathName;

	for ( size_t i = 0; i != Result.length(); ++i )
		if ( Result[i] == '/' || Result[i] == '\\' )
		{
			Result[i] = PATH_SEPARATOR;
		}

	return Result;
}

inline bool FS_FileExistsPhys( const std::string& PhysicalName )
{
#ifdef _WIN32
	struct _stat buf;
	int Result = _stat( FS_ValidatePath( PhysicalName ).c_str(), &buf );
#else
	struct stat buf;
	int Result = stat( FS_ValidatePath( PhysicalName ).c_str(), &buf );
#endif
	return Result == 0;
}

/// Mount point implementation for the physical folder
class PhysicalMountPoint: public iMountPoint
{
public:
	PhysicalMountPoint( const std::string& PhysicalName ): FPhysicalName( PhysicalName )
	{
		Str_AddTrailingChar( &FPhysicalName, PATH_SEPARATOR );
	}
	virtual ~PhysicalMountPoint() {}

	void SetUseVirtualFileNames( bool Use ) { FUseVirtualFileNames = Use; };
	bool GetUseVirtualFileNames() const { return FUseVirtualFileNames; };

	virtual bool         FileExists( const std::string& VirtualName ) const { return FS_FileExistsPhys( MapName( VirtualName ) ); }
	virtual std::string  MapName( const std::string& VirtualName ) const
	{
		return ( !FUseVirtualFileNames || FS_IsFullPath( VirtualName ) ) ? VirtualName : ( FPhysicalName + VirtualName );
	}

	virtual clPtr<iRawFile>    CreateReader( const std::string& VirtualName ) const
	{
		std::string PhysName = FS_IsFullPath( VirtualName ) ? VirtualName : MapName( VirtualName );

		clPtr<RawFile> File = new RawFile();
		return !File->Open( FS_ValidatePath( PhysName ), VirtualName ) ? NULL : File;
	}
private:
	std::string FPhysicalName;
	bool    FUseVirtualFileNames;
};

/// The decorator to allow file name dereferencing
class AliasMountPoint: public iMountPoint
{
public:
	AliasMountPoint( const clPtr<iMountPoint>& Src ): FMP( Src ) {}
	virtual ~AliasMountPoint() {}

	/// Set alias directory
	void    SetAlias( const std::string& N )
	{
		FAlias = N;
		Str_AddTrailingChar( &FAlias, PATH_SEPARATOR );
	}

	/// Get internal mount point name
	std::string GetAlias() const { return FAlias; }

	virtual bool            FileExists( const std::string& VirtualName ) const { return FMP->FileExists( FAlias + VirtualName ); }
	virtual std::string     MapName( const std::string& VirtualName ) const { return FMP->MapName( FAlias + VirtualName ); }
	virtual clPtr<iRawFile> CreateReader( const std::string& VirtualName ) const { return FMP->CreateReader( FAlias + VirtualName ); }
private:
	/// Name to append to each file in this mount point
	std::string FAlias;

	/// The actual file container
	clPtr<iMountPoint> FMP;
};

/// Implementation of a mount point for the .zip files
class ArchiveMountPoint: public iMountPoint
{
public:
	ArchiveMountPoint( const clPtr<ArchiveReader>& R ): FReader( R ) {}
	virtual ~ArchiveMountPoint() {}

	virtual clPtr<iRawFile>    CreateReader( const std::string& VirtualName ) const
	{
		std::string FName = Arch_FixFileName( VirtualName );

		MemRawFile* File = new MemRawFile();

		File->SetFileName( VirtualName );
		File->SetVirtualFileName( VirtualName );

		const void* DataPtr = FReader->GetFileData( FName );
		uint64 FileSize = FReader->GetFileSize( FName );

		File->CreateFromManagedBuffer( DataPtr, FileSize );
		return File;
	}

	virtual bool FileExists( const std::string& VirtualName ) const { return FReader->FileExists( Arch_FixFileName( VirtualName ) ); }
	virtual std::string      MapName( const std::string& VirtualName ) const { return VirtualName; }
private:
	clPtr<ArchiveReader> FReader;
};

#include "FileSystem.h"

#include "Archive.h"

#include <iostream>

int main()
{
	clPtr<RawFile> File = new RawFile();
	File->Open( "test.zip", "vfn.zip" );

	clPtr<ArchiveReader> a = new ArchiveReader();
	a->OpenArchive( new FileMapper( File ) );

	/// Iterate the files in archive
	for ( size_t i = 0 ; i < a->GetNumFiles() ; i++ )
	{
		std::cout << "File[" << i << "]: " << a->GetFileName( i ) << std::endl;
	}

	return 0;
}

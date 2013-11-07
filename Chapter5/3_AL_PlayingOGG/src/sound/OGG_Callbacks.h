static size_t OGG_ReadFunc( void* Ptr, size_t Size, size_t NMemB, void* DataSource )
{
	OggProvider* OGG = static_cast<OggProvider*>( DataSource );

	size_t DataSize = OGG->FRawData->GetSize();

	ogg_int64_t BytesRead = DataSize - OGG->FOGGRawPosition;
	ogg_int64_t BytesSize = Size * NMemB;

	if ( BytesSize < BytesRead ) { BytesRead = BytesSize; }

	memcpy( Ptr, ( ubyte* )OGG->FRawData->GetDataConst() + OGG->FOGGRawPosition, ( size_t )BytesRead );

	OGG->FOGGRawPosition += BytesRead;

	return ( size_t )BytesRead;
}
static int OGG_SeekFunc( void* DataSource, ogg_int64_t Offset, int Whence )
{
	OggProvider* OGG = static_cast<OggProvider*>( DataSource );

	size_t DataSize = OGG->FRawData->GetSize();

	if ( Whence == SEEK_SET )
	{
		OGG->FOGGRawPosition = Offset;
	}
	else if ( Whence == SEEK_CUR )
	{
		OGG->FOGGRawPosition += Offset;
	}
	else if ( Whence == SEEK_END )
	{
		OGG->FOGGRawPosition = DataSize + Offset;
	}

	if ( OGG->FOGGRawPosition > ( ogg_int64_t )DataSize )
	{
		OGG->FOGGRawPosition = ( ogg_int64_t )DataSize;
	}

	return static_cast<int>( OGG->FOGGRawPosition );
}
static int OGG_CloseFunc( void* DataSource )
{
	return 0;
}
static long OGG_TellFunc( void* DataSource )
{
	return ( int )( ( ( OggProvider* )DataSource )->FOGGRawPosition );
}

#pragma once

#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#include "iObject.h"
#include "iIntrusivePtr.h"

#undef min
#undef max

class clBlob: public iObject
{
public:
	clBlob(): FData( NULL ), FOwnsData( true ), FSize( 0 ), FCurrentPos( 0 ), FAllocatedSize( 0 ) {}
	virtual ~clBlob() { Delete(); }

	/// Set the blob data pointer to some external memory block
	void SetExternalData( void* Ptr, size_t Sz )
	{
		Delete();

		FData = Ptr;
		FAllocatedSize = Sz;
		FSize = Sz;
		FOwnsData = false;
	}

	/// Constant access to the blob's data
	inline void* GetDataConst() const { return FData; }

	/// Direct access to blob's data
	inline void* GetData() { return FData; }

	/// Alias (setter method) for the reallocator
	void SetSize( size_t NewSize ) { Reallocate( NewSize ); }

	/// Get current blob size
	size_t GetSize() const { return FSize; }

	/// Check if this blob manages its own contents
	bool OwnsData() const { return FOwnsData; }

	/// Change ownership of the memory block
	void SetOwnership( bool Ownership ) { FOwnsData = Ownership; }

	/// Make a local copy of the other blob. Can change memory ownership of this blob on reallocation
	bool CopyBlob( const clPtr<clBlob>& Other ) { return CopyMemoryBlock( Other->GetDataConst(), Other->GetSize() ); }

	/// Check if this blob refers to some data
	bool HasData() const  { return ( FData != NULL ); }

	/**
	   \brief Binary comparison with other blob (strcmp() analogue)
	   Error codes: 0 if equal, -1/+1 if other is longer/shorter than this
	   0xFF - one of the blobs contains null data
	*/
	int CompareContents( const clPtr<clBlob>& Other ) const
	{
		if ( !FData || !Other->HasData() ) { return 0xFF; }

		if ( this->FSize == Other->GetSize() ) { return memcmp( FData, Other->GetDataConst(), this->FSize ); }

		return ( this->FSize > Other->GetSize() ) ? 1 : -1;
	}

	/// Copy blob contents from external source
	bool CopyMemoryBlock( const void* FromData, size_t FromSize )
	{
		if ( ( !FromData ) || ( FromSize <= 0 ) ) { return false; }

		// only re-allocate if not enough space
		Reallocate( FromSize );

		this->FSize = FromSize;

		memcpy( this->FData, FromData, FromSize );

		return true;
	}

	/// Template Set method to avoid inclusion of custom type headers (vec3/mtx etc)
	template<class T>
	void SetPOD( size_t Offset, T TheValue ) { SetBytes( Offset, sizeof( T ), ( ubyte* )&TheValue ); }

	/// Template Get method to avoid inclusion of custom type headers (vec3/mtx etc)
	template<class T>
	T GetPOD( size_t Offset ) { T Tmp; GetBytes( Offset, sizeof( T ), ( ubyte* )&Tmp ); return Tmp; }

	/// Item access
	void SetByte( size_t Offset, ubyte TheByte ) { ( ( ubyte* )FData )[Offset] = TheByte; }

	/// Quick access to the specififed byte. No range checking
	ubyte GetByte( size_t Offset ) const { return ( ( ubyte* )FData )[Offset]; }

	void SetBytes( size_t Offset, size_t Num, const ubyte* Src ) { memcpy( ( ubyte* )FData + Offset, Src, Num ); }
	void GetBytes( size_t Offset, size_t Num, ubyte* Out ) const { memcpy( Out, ( ubyte* )FData + Offset, Num ); }
	bool AppendBytes( void* Data, size_t Size )
	{
		const size_t BlobInitialSize = 8192;

		bool ShouldGrow = ( GetSize() + Size ) > FAllocatedSize;

		if ( ShouldGrow )
		{
			if ( !FOwnsData ) { return false; }

			size_t NewSize1 = GetSize() + Size;
			size_t NewSize2 = NewSize1; // FAllocatedSize * 2;

			size_t NewSize = std::max( NewSize1, NewSize2 );

			if ( NewSize < BlobInitialSize ) { NewSize = BlobInitialSize; }

			FData = realloc( FData, NewSize );
			FAllocatedSize = NewSize;
		}

		memcpy( ( ubyte* )FData + FSize, Data, Size );

		FSize += Size;

		return true;
	}

	/// Ensure there is enough space for the byte array
	bool SafeSetBytes( size_t Offset, size_t Num, const ubyte* Src )
	{
		if ( Offset + Num > FSize )
		{
			if ( !SafeResize( Offset + Num ) ) { return false; }
		}

		SetBytes( Offset, Num, Src );

		return true;
	}

	/// Copy Count bytes from another blob
	void CopyFrom( const clPtr<clBlob>& Other, size_t SrcOffset, size_t DestOffset, size_t Count )
	{
		SetBytes( DestOffset, Count, ( ( ubyte* )Other->GetDataConst() ) + SrcOffset );
	}

	/// Resize and do not spoil the contents
	bool SafeResize( size_t NewSize )
	{
		if ( !FOwnsData ) { return false; }

		if ( !FData ) { Allocate( NewSize ); return true; }

		/// No reallocations needed ?
		if ( NewSize <= FSize )
		{
			FSize = NewSize;

			return true;
		}

		FData = ::realloc( FData, NewSize );

		if ( !FData ) { return false; }

		FSize = FAllocatedSize = NewSize;

		return true;
	}

private:
	/// Pointer to the blob data
	void*  FData;

	/// Effective size of the blob
	size_t FSize;

	/// Actually allocated size (used for faster resize and copy operations)
	size_t FAllocatedSize;

	/// True if this Blob manages and deallocates the memory block
	bool   FOwnsData;

	#pragma region Memory management

	/// Allocate new block and change ownership type
	inline void Allocate( size_t NewSize )
	{
		FData = ::malloc( NewSize );
		FSize = FAllocatedSize = NewSize;
		FOwnsData = true;
	}

	/// Try to delete the memory block. Not exposed as a public method, because direct access here can cause troubles.
	inline void Delete()
	{
		if ( !FOwnsData || FData == NULL ) { return; }

		::free( FData );
		FData = NULL;
		FSize = FAllocatedSize = 0;
		FOwnsData = false;
	}

	/// Reallocate the data block if it is required
	void Reallocate( size_t NewSize )
	{
		if ( !FData ) { Allocate( NewSize ); }

		if ( FData != NULL )
		{
			if ( NewSize > FAllocatedSize )
			{
				// try to delete
				Delete();

				// change ownership
				Allocate( NewSize );
			}
			else if ( NewSize <= FAllocatedSize )
			{
				// do nothing, just change the size
				FSize = NewSize;
			}
		}
	}

	#pragma endregion

public:
	#pragma region Utility procedures

	/// Restart blob reading
	void RestartRead() { FCurrentPos = 0; }

	/// Restart blob writing
	void RestartWrite() { FSize = 0; FCurrentPos = 0; }

	ubyte ReadByte() { return GetByte( FCurrentPos++ ); }
	void WriteByte( ubyte TheByte ) { SetByte( FCurrentPos, TheByte ); FCurrentPos++; }

	void ReadBytes( ubyte* Out, size_t Num ) { GetBytes( FCurrentPos, Num, Out ); FCurrentPos += Num; }
	void WriteBytes( const ubyte* Src, size_t Num ) { if ( SafeSetBytes( FCurrentPos, Num, Src ) ) { FCurrentPos += Num; } }

	/// Current read/write position
	size_t FCurrentPos;

	#pragma endregion
};

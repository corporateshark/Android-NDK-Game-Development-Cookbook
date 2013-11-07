#ifndef _iOStream_
#define _iOStream_

#include "iObject.h"
#include <string>

/// Input stream
class iIStream: public iObject
{
public:
	iIStream() {}
	virtual ~iIStream() {}
	virtual std::string GetVirtualFileName() const = 0;
	virtual std::string GetFileName() const = 0;
	virtual void   Seek( const uint64 Position ) = 0;
	virtual uint64 Read( void* Buf, uint64 Size ) = 0;
	virtual bool   Eof() const = 0;
	virtual uint64 GetSize() const = 0;
	virtual uint64 GetPos() const = 0;
	virtual uint64 GetBytesLeft() const { return GetSize() - GetPos(); };

	virtual std::string ReadLine() = 0;

	/// Return pointer to the shared memory corresponding to this file
	virtual const ubyte*  MapStream() const = 0;
	/// Return pointer to the shared memory corresponding to the current position in this file
	virtual const ubyte*  MapStreamFromCurrentPos() const = 0;
};

/// Output stream
class iOStream: public iObject
{
public:
	iOStream() {};
	virtual ~iOStream() {};
	virtual std::string GetFileName() const = 0;
	virtual void   Seek( const uint64 Position ) = 0;
	virtual uint64 GetFilePos() const = 0;
	virtual uint64 Write( const void* Buf, const uint64 Size ) = 0;
};

#endif

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

	/// Return pointer to the shared memory corresponding to this file
	virtual const ubyte*  MapStream() const = 0;
	/// Return pointer to the shared memory corresponding to the current position in this file
	virtual const ubyte*  MapStreamFromCurrentPos() const = 0;

	virtual std::string ReadLine() = 0;
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

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

#ifndef __Network__h__included__
#define __Network__h__included__

#include <string>

#define PORT_ANY -1

#define LNET_LOOPBACK   (0)
#define LNET_IP         (1)
#define LNET_BROADCAST  (2)

#if defined(_WIN32)
typedef size_t SOCKET_T;
#else
typedef int SOCKET_T;
#endif

#include <queue>

/// \brief IP Network Address wrapper
struct LNetworkAddress
{
public:
	int type;
	unsigned char ip[4]; // ipv4. should be no problems with ipv6 ?
	int port;  // -1 for PORT_ANY
public:
	LNetworkAddress()
	{
		for ( int i = 0 ; i < 4 ; i++ ) { ip[i] = 0; }

		port = -1;
		type = LNET_LOOPBACK;
	}

	LNetworkAddress( const std::string& S ) { FromString( S ); }

	LNetworkAddress( const LNetworkAddress& Other ) { Assign( Other ); }

	LNetworkAddress& operator = ( const LNetworkAddress& Other )
	{
		Assign( Other );
		return *this;
	}

	/// Compare two addresses
	bool operator == ( const LNetworkAddress& Other ) const
	{
		for ( int i = 0 ; i < 4 ; i++ ) { if ( ip[i] != Other.ip[i] ) { return false; } }

		return ( port == Other.port ) && ( type == Other.type );
	}

	void Assign( const LNetworkAddress& Other )
	{
		for ( int i = 0 ; i < 4 ; i++ ) { ip[i] = Other.ip[i]; }

		type = Other.type;
		port = Other.port;
	}

	/// Convert from std::string
	bool FromString( const std::string& str );

	/// Convert to std::string
	std::string ToString() const;

	/// Convert IP to std::string
	std::string IPToString() const;

	/// Compare base address, without port
	bool CompareBase( const LNetworkAddress& other ) const;

	/// Compare complete address
	bool Compare( const LNetworkAddress& other ) const;

	/// From sockaddr_in (wsock/BSDSockets)
	void FromSockadr( void* s );

	/// To sockaddr_in (wsock/BSDSockets)
	void ToSockadr( void* s ) const;
};

/**
   Initialization of WinSock (for Win32)
   Two static variables are OK here. Even with multiple environments there will be no errors
**/

void InitializeNetwork();
void ShutdownNetwork();

std::string LNetwork_LastErrorAsString();

class LSocket
{
public:
	LSocket();
	virtual ~LSocket();

	virtual bool Open();
	virtual bool Close();

	/// Check for data availability using select() call
	virtual bool CheckData( bool Read, bool Write, bool Error, int msec, int musec );

	virtual bool Bind( const std::string& sourceAddress, int port );

	/// Receive a long string with possible internal EOLNs
	std::string ReceiveBytes( int MaxLen );

	/// Receive a string terminated by Newline marker
	std::string ReceiveLine( int MaxLen );

	/// Send a line terminated by NewLine marker. Used in HTTP-like protocols
	void SendLine( const std::string& s ) { SendBytes( s + std::string( "\x0D\x0A" ) ); }

	/// Send a long string
	bool SendBytes( const std::string& S );

	virtual int  WriteBytes( const unsigned char* Buf, int num );
	virtual int  ReadBytes( unsigned char* Buf, int MaxBytes );

	// were there any fatal errors in Read/Write ?
	virtual bool IsError() const { return FError; }
	virtual std::string GetError() const { return FLastError; }

	virtual bool IsOpened() const;
	virtual bool IsWouldBlock() const;

	virtual bool SetNonBlocking( bool nonBlocking );

	// for TCP accept() and connect()
	void SetSocketHandle( SOCKET_T Sock ) { FSocket = Sock; }

	void SetSocketAddress( const std::string& addr, int port )
	{
		FAddress = addr;
		FPort = port;
	}

public:
	SOCKET_T    FSocket;
	int       FPort;
	std::string   FAddress;
	bool      FError;
	std::string   FLastError;
protected:
	virtual int GetIPProtocol() = 0;
	virtual int GetSockType() = 0;
};

class LUDPSocket : public LSocket
{
public:
	LUDPSocket() {}
	virtual ~LUDPSocket() {}

	bool EnableBroadcast( bool bcast );
protected:
	virtual int GetIPProtocol();
	virtual int GetSockType();
};

class LTCPSocket: public LSocket
{
public:
	LTCPSocket() {}
	virtual ~LTCPSocket() {}

	// returns NULL if no incoming connections and the socket is non-blocking
	virtual LTCPSocket* Accept();
	virtual int Listen( int BackLog );
	virtual bool Connect( const std::string& addr, int port );
protected:
	virtual int GetIPProtocol();
	virtual int GetSockType();
};

#endif // if defined NETWORK_H_INCLUDED

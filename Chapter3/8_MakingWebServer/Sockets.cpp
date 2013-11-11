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

#include "Sockets.h"

#include <errno.h>

#if !defined(_WIN32)
typedef int SOCKET_T;
#define INVALID_SOCKET (SOCKET_T)(~0)
#endif

#ifdef _WIN32
# include <winsock2.h>
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef _MSC_VER
#define SAFE_strcpy(dest, src) strcpy_s(dest, 128, src)
#define LNET_snprintf _snprintf_s
#else
#define SAFE_strcpy(dest, src) strcpy(dest, src)
#define LNET_snprintf snprintf
#endif

#ifndef _WIN32

#include <memory.h>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <errno.h>

#include <sys/types.h>
#include <arpa/inet.h>

#include <linux/ip.h>
#include <linux/icmp.h>

#define ULONG unsigned int

// define our local names for functions
#define LNet_CLOSESOCKET close
#define LNet_IOCTLSOCKET ioctl

#endif

#ifdef _WIN32
#include <winsock2.h>

#define LNet_CLOSESOCKET closesocket
#define LNet_IOCTLSOCKET ioctlsocket

#define hack_IOC_VENDOR 0x18000000
#define hack_IOC_IN 0x80000000
#define hack_WSAIOW(x,y) (hack_IOC_IN|(x)|(y))
#define SIO_UDP_CONNRESET           hack_WSAIOW(hack_IOC_VENDOR,12)

#endif

#define LNet_SETSOCKOPT setsockopt

#define LNet_SELECT  select
#define LNet_FDISSET FD_ISSET
#define LNet_BIND bind
#define LNet_SEND send
#define LNet_RECV recv
#define LNet_SENDTO sendto
#define LNet_RECVFROM recvfrom
#define LNet_SOCKET socket
#define LNet_ACCEPT accept
#define LNet_LISTEN listen
#define LNet_CONNECT connect

#define LNet_GETHOSTBYNAME gethostbyname
#define LNet_INET_ADDR inet_addr
#define LNet_HTONL htonl
#define LNet_HTONS htons
#define LNet_NTOHL ntohl
#define LNet_NTOHS ntohs

// identical for win32/linux
/*
localhost
192.168.0.1
10.0.22.15:1000
*/
bool LStringToSockaddr ( const char* s, struct sockaddr* sadr )
{
	struct hostent* h;
	char*  colon;
	char  copy[128];

	memset ( sadr, 0, sizeof( *sadr ) );
	( ( struct sockaddr_in* )sadr )->sin_family = AF_INET;

	( ( struct sockaddr_in* )sadr )->sin_port = 0;

#ifdef _MSC_VER
	SAFE_strcpy ( copy, s );
#else
	SAFE_strcpy ( copy, s );
#endif

	// strip off a trailing :port if present
	for ( colon = copy ; *colon ; colon++ )
		if ( *colon == ':' )
		{
			*colon = 0;
			( ( struct sockaddr_in* )sadr )->sin_port = LNet_HTONS( ( short )atoi( colon + 1 ) );
		}

	if ( copy[0] >= '0' && copy[0] <= '9' )
	{
		*( int* )&( ( struct sockaddr_in* )sadr )->sin_addr = LNet_INET_ADDR( copy );
	}
	else
	{
		h = LNet_GETHOSTBYNAME( copy );

		if ( ! ( h ) )
		{
			return 0;
		}

		*( int* )&( ( struct sockaddr_in* )sadr )->sin_addr = *( int* )h->h_addr_list[0];
	}

	return true;
}

void IPAndPortToSockAddr( const std::string& addrStr, int port, struct sockaddr_in* address )
{
#ifdef _WIN32

	if ( addrStr.empty() || !_stricmp( addrStr.c_str(), "localhost" ) )
#else
	if ( addrStr.empty() || !strcasecmp( addrStr.c_str(), "localhost" ) )
#endif
	{
		address->sin_addr.s_addr = INADDR_ANY;
	}
	else
	{
		LStringToSockaddr ( addrStr.c_str(), ( struct sockaddr* )address );
	}

	if ( port == PORT_ANY )
	{
		address->sin_port = 0;
	}
	else
	{
		address->sin_port = LNet_HTONS( ( short )port );
	}

	address->sin_family = AF_INET;
}

bool LNetworkAddress::FromString ( const std::string& s )
{
#ifdef _WIN32
	struct sockaddr sadr;
#else
	struct sockaddr_in sadr;
#endif

	if ( !strcmp ( s.c_str(), "localhost" ) )
	{
		memset ( ip, 0, sizeof( ip ) );
		port = 0;

		type = LNET_LOOPBACK;
		return true;
	}

	if ( !LStringToSockaddr( s.c_str(), ( sockaddr* )&sadr ) )
	{
		return false;
	}

	FromSockadr( &sadr );

	return true;
}

void LNetworkAddress::ToSockadr( void* s ) const
{
	memset ( s, 0, sizeof( struct sockaddr_in ) );

	if ( type == LNET_BROADCAST )
	{
		( ( struct sockaddr_in* )s )->sin_family = AF_INET;
		( ( struct sockaddr_in* )s )->sin_port = LNet_HTONS( static_cast<u_short>( port ) );
		( ( struct sockaddr_in* )s )->sin_addr.s_addr = INADDR_BROADCAST;
	}
	else if ( type == LNET_IP )
	{
		( ( struct sockaddr_in* )s )->sin_family = AF_INET;
		( ( struct sockaddr_in* )s )->sin_addr.s_addr = *( int* )&ip;
		( ( struct sockaddr_in* )s )->sin_port = LNet_HTONS(   static_cast<u_short>( port ) );
	}
}

void LNetworkAddress::FromSockadr( void* s )
{
	*( int* )&ip = ( ( struct sockaddr_in* )s )->sin_addr.s_addr;
	port = LNet_NTOHS ( ( ( struct sockaddr_in* )s )->sin_port );
	// s->sa_family is always IP
	type = LNET_IP;
}

// convert to std::string
std::string LNetworkAddress::ToString() const
{
	// no static vars to avoid problems with DLLs
	char s[64];

	s[0] = '\0';

	if ( type == LNET_LOOPBACK )
	{
		LNET_snprintf ( s, sizeof( s ), "127.0.0.1:%d", /*LNet_NTOHS*/( static_cast<u_short>( port ) ) );
	}
	else if ( type == LNET_IP )
	{
		LNET_snprintf ( s, sizeof( s ), "%i.%i.%i.%i:%i", ip[0], ip[1], ip[2], ip[3], /*LNet_NTOHS*/( static_cast<u_short>( port ) ) );
	}

	return std::string( s );
}

std::string LNetworkAddress::IPToString() const
{
	// no static vars to avoid problems with DLLs
	char s[64];

	s[0] = '\0';

	if ( type == LNET_LOOPBACK )
	{
		LNET_snprintf ( s, sizeof( s ), "loopback" );
	}
	else if ( type == LNET_IP )
	{
		LNET_snprintf ( s, sizeof( s ), "%i.%i.%i.%i", ip[0], ip[1], ip[2], ip[3] );
	}

	return std::string( s );
}

// compare base address, without port
bool LNetworkAddress::CompareBase( const LNetworkAddress& other ) const
{
	if ( other.type != type )
	{
		return false;
	}

	return ( ip[0] == other.ip[0] ) && ( ip[1] == other.ip[1] ) && ( ip[2] == other.ip[2] ) && ( ip[3] == other.ip[3] );
}

// compare complete address
bool LNetworkAddress::Compare( const LNetworkAddress& other ) const
{
	return ( CompareBase( other ) && ( other.port == port ) );
}

////////////////

#ifdef _WIN32
int Static_NetworkInitialized = -1;
WSADATA Static_WinsockStartData;
#endif

void InitializeNetwork()
{
#ifdef _WIN32
	Static_NetworkInitialized = ( Static_NetworkInitialized < 0 ) ? 1 : Static_NetworkInitialized + 1;

	// request version 2.0 - we need a workaround for async sockets in Win2000
	if ( WSAStartup( MAKEWORD( 2, 0 ), &Static_WinsockStartData ) != 0 )
	{
		MessageBoxW( 0, static_cast<LPCWSTR>( L"Can't load WinSock" ), static_cast<LPCWSTR>( L"Error" ), 0 );
		Static_NetworkInitialized++;
	}

#endif
}

void ShutdownNetwork()
{
#ifdef _WIN32
	Static_NetworkInitialized--;

	// deinitialize only when there are no more 'clients'
	if ( Static_NetworkInitialized == 0 )
	{
		WSACleanup();
	}

#endif
}

std::string LNetwork_LastErrorAsString()
{
	char errBuf[128];
#ifndef _WIN32
	// standart libc routine
	SAFE_strcpy( errBuf, strerror( errno ) );
#else
	//  Convert WinSock error code to ASCII std::string
	int ErrorCode = WSAGetLastError();

#define WSA_STRING(Err) case Err :  SAFE_strcpy(errBuf, #Err ); break;

	printf( "ECode = %x\n", ErrorCode );

	switch ( ErrorCode )
	{
			WSA_STRING( WSAEINTR )
			WSA_STRING( WSAEBADF )
			WSA_STRING( WSAEACCES )
			WSA_STRING( WSAEDISCON )
			WSA_STRING( WSAEFAULT )
			WSA_STRING( WSAEINVAL )
			WSA_STRING( WSAEMFILE )
			WSA_STRING( WSAEWOULDBLOCK )
			WSA_STRING( WSAEINPROGRESS )
			WSA_STRING( WSAEALREADY )
			WSA_STRING( WSAENOTSOCK )
			WSA_STRING( WSAEDESTADDRREQ )
			WSA_STRING( WSAEMSGSIZE )
			WSA_STRING( WSAEPROTOTYPE )
			WSA_STRING( WSAENOPROTOOPT )
			WSA_STRING( WSAEPROTONOSUPPORT )
			WSA_STRING( WSAESOCKTNOSUPPORT )
			WSA_STRING( WSAEOPNOTSUPP )
			WSA_STRING( WSAEPFNOSUPPORT )
			WSA_STRING( WSAEAFNOSUPPORT )
			WSA_STRING( WSAEADDRINUSE )
			WSA_STRING( WSAEADDRNOTAVAIL )
			WSA_STRING( WSAENETDOWN )
			WSA_STRING( WSAENETUNREACH )
			WSA_STRING( WSAENETRESET )
			WSA_STRING( WSAECONNABORTED )
			WSA_STRING( WSAECONNRESET )
			WSA_STRING( WSAENOBUFS )
			WSA_STRING( WSAEISCONN )
			WSA_STRING( WSAENOTCONN )
			WSA_STRING( WSAESHUTDOWN )
			WSA_STRING( WSAETOOMANYREFS )
			WSA_STRING( WSAETIMEDOUT )
			WSA_STRING( WSAECONNREFUSED )
			WSA_STRING( WSAELOOP )
			WSA_STRING( WSAENAMETOOLONG )
			WSA_STRING( WSAEHOSTDOWN )
			WSA_STRING( WSASYSNOTREADY )
			WSA_STRING( WSAVERNOTSUPPORTED )
			WSA_STRING( WSANOTINITIALISED )
			WSA_STRING( WSAHOST_NOT_FOUND )
			WSA_STRING( WSATRY_AGAIN )
			WSA_STRING( WSANO_RECOVERY )
			WSA_STRING( WSANO_DATA )

		default:
			SAFE_strcpy( errBuf, "NO ERROR" );
			break;
	}

#undef WSA_STRING

#endif

	return std::string( errBuf );
}

//
// Socket wrapper
//

LSocket::LSocket()
{
	FSocket      = INVALID_SOCKET;
	FError       = false;
	FLastError   = "";
}

LSocket::~LSocket()
{
}

bool LSocket::IsOpened() const
{
	return ( FSocket != INVALID_SOCKET );
}

bool LSocket::CheckData( bool CheckRead, bool CheckWrite, bool CheckError, int msec, int musec )
{
	if ( FSocket == INVALID_SOCKET ) { return false; }

	fd_set ReadFDS, WriteFDS, ErrorFDS;

#ifdef _MSC_VER
// We cannot rewrite strange WinSock headers so we disable this
#pragma warning(disable:4127)
#endif

	if ( CheckRead ) { FD_ZERO( &ReadFDS ); FD_SET( FSocket, &ReadFDS ); }

	if ( CheckWrite ) { FD_ZERO( &WriteFDS ); FD_SET( FSocket, &WriteFDS ); }

	if ( CheckError ) { FD_ZERO( &ErrorFDS ); FD_SET( FSocket, &ErrorFDS ); }

	struct timeval Timeout;

	// sleep for 10ms (10000us)
	Timeout.tv_sec  = msec / 1000;

	Timeout.tv_usec = musec + ( msec % 1000 ) * 1000;

	int Res = LNet_SELECT( 1, CheckRead ? &ReadFDS : NULL, CheckWrite ? &WriteFDS : NULL, CheckError ? &ErrorFDS : NULL, &Timeout );

	if ( Res == -1 )
	{
		FError = true;
		FLastError = LNetwork_LastErrorAsString();
		return false;
	}

	bool Result = false;

	if ( CheckRead ) { Result |= ( LNet_FDISSET( FSocket, &ReadFDS )  != 0 ); }

	if ( CheckWrite ) { Result |= ( LNet_FDISSET( FSocket, &WriteFDS ) != 0 ); }

	if ( CheckError ) { Result |= ( LNet_FDISSET( FSocket, &ErrorFDS ) != 0 ); }

	return Result;
}

LTCPSocket* LTCPSocket::Accept()
{
	struct  sockaddr_in sad;
	memset( ( char* )&sad, 0, sizeof( sad ) );

#ifdef _WIN32
	int alen = sizeof( sad );
#else
	socklen_t alen = static_cast<socklen_t>( sizeof( sad ) );
#endif
	SOCKET_T NewSocket = LNet_ACCEPT( FSocket, ( struct sockaddr* )&sad, &alen );

	if ( NewSocket != INVALID_SOCKET )
	{
		LNetworkAddress RemoteAddr;
		RemoteAddr.FromSockadr( &sad );
		std::string S = RemoteAddr.ToString();

		// create new socket
		LTCPSocket* Sock = new LTCPSocket();
		Sock->SetSocketAddress( S, RemoteAddr.port );
		Sock->SetSocketHandle( NewSocket );

		return Sock;
	}

	return NULL;
}

int LTCPSocket::Listen( int BackLog )
{
	return ( LNet_LISTEN( FSocket, BackLog ) == 0 );
}

bool LTCPSocket::Connect( const std::string& addr, int port )
{
	struct  sockaddr_in sad;
	memset( ( char* )&sad, 0, sizeof( sad ) );

	IPAndPortToSockAddr( addr, port, &sad );

	int NewSocket = LNet_CONNECT( FSocket, ( struct sockaddr* )&sad, sizeof( sad ) );

	return ( NewSocket < 0 ) ? false : true;
}

bool LUDPSocket::EnableBroadcast( bool bcast )
{
	int val = bcast ? 1 : 0;

	if ( LNet_SETSOCKOPT( FSocket, SOL_SOCKET, SO_BROADCAST, ( const char* )&val, sizeof( int ) ) == -1 )
	{
		FLastError = "setsockopt SO_BROADCAST: " + LNetwork_LastErrorAsString();
		FError = true;

		return false;
	}

	return true;
}

bool LSocket::Open()
{
	// create socket
	FSocket = LNet_SOCKET( AF_INET, GetSockType(), GetIPProtocol() );

	if ( FSocket == INVALID_SOCKET )
	{
		FLastError = "OpenSocket error : " + LNetwork_LastErrorAsString();
		FError = true;

		printf( "FLastError: %s\n", FLastError.c_str() );
		return false;
	}

#ifdef _WIN32

	if ( GetIPProtocol() != IPPROTO_UDP ) { return true; }

	DWORD dwBytesReturned = 0;
	BOOL bNewBehavior = FALSE;
	DWORD status;

	// disable  new behavior using
	// IOCTL: SIO_UDP_CONNRESET
	status = WSAIoctl( FSocket, SIO_UDP_CONNRESET, &bNewBehavior, sizeof( bNewBehavior ), NULL, 0, &dwBytesReturned, NULL, NULL );

	if ( SOCKET_ERROR == status )
	{
		DWORD dwErr = WSAGetLastError();

		if ( WSAEWOULDBLOCK != dwErr )
		{
			FLastError = std::string( "WSAIoctl(SIO_UDP_CONNRESET) Error: " ) + LNetwork_LastErrorAsString();
			return false;
		}
	}

#endif

	return true;
}

bool LSocket::Bind( const std::string& sourceAddress, int port )
{
	// 1. resolve address and port
	FAddress = sourceAddress;
	FPort    = port;

	struct sockaddr_in address;

	IPAndPortToSockAddr( sourceAddress, port, &address );

	// 2. bind socket to address
	if ( LNet_BIND ( FSocket, ( struct sockaddr* )/*(void *)*/&address, sizeof( address ) ) == -1 )
	{
		FLastError = LNetwork_LastErrorAsString();

		if ( FLastError != "NO ERROR" )
		{
			FLastError = "OpenSocket: bind: " + FLastError;
			FError = true;

			LNet_CLOSESOCKET( FSocket );

			return false;
		}
	}

	return true;
}

bool LSocket::SetNonBlocking( bool nonBlocking )
{
	ULONG _flag = nonBlocking ? 1 : 0;

	if ( LNet_IOCTLSOCKET( FSocket, FIONBIO, &_flag ) == -1 )
	{
		FLastError = "ioctl FIONBIO error : " + LNetwork_LastErrorAsString();
		FError = true;

		return false;
	}

	return true;
}

bool LSocket::Close()
{
	return ( LNet_CLOSESOCKET( FSocket ) == -1 );
}

bool LSocket::IsWouldBlock() const
{
#ifdef _WIN32
	return ( WSAGetLastError() == WSAEWOULDBLOCK );
#else
	return ( errno == EWOULDBLOCK );
#endif
}

int  LSocket::WriteBytes( const unsigned char* Buf, int num )
{
	return LNet_SEND ( FSocket, ( char* )Buf, num, 0 );
}

int  LSocket::ReadBytes( unsigned char* Buf, int MaxBytes )
{
	return LNet_RECV ( FSocket, ( char* )Buf, MaxBytes, 0 );
}

std::string LSocket::ReceiveBytes( int MaxLen )
{
	std::string Ret;

	static const int MAX_STRING_LEN = 1024;

	char buf[MAX_STRING_LEN];

	if ( MaxLen > MAX_STRING_LEN ) { MaxLen = MAX_STRING_LEN; }

	for ( ;; )
	{
		unsigned long NumAvail = 0;

		/// Check data availability
		if ( LNet_IOCTLSOCKET( FSocket, FIONREAD, &NumAvail ) != 0 ) { break; }

		if ( NumAvail == 0 ) { break; }

		if ( ( int )NumAvail > MaxLen ) { NumAvail = ( unsigned long )MaxLen; }

		int NumRead = LNet_RECV( FSocket, buf, NumAvail, 0 );

		if ( NumRead <= 0 ) { break; }

		std::string T;

		T.assign ( buf, NumRead );

		Ret += T;
	}

	return Ret;
}

// TODO: timeout would be nice here
std::string LSocket::ReceiveLine( int MaxLen )
{
	std::string Ret;

	for ( int Len = 0 ; Len < MaxLen ; )
	{
		char R;

		switch ( LNet_RECV( FSocket, &R, 1, 0 ) )
		{
				/// Connection terminated
			case 0:
				return "";

				/// Blocking condition or something critical
			case -1:
#ifdef _WIN32
				if ( ! IsWouldBlock() ) { return ( errno == EAGAIN ) ? Ret : ""; }

#else
				return Ret;
#endif
		}

		if ( !IsWouldBlock() )
		{
			Ret += R;

			if ( R == '\n' )
			{
				return Ret;
			}

			Len++;
		}
	}

	return Ret;
}

bool LSocket::SendBytes( const std::string& S )
{
	return ( LNet_SEND( FSocket, S.c_str(), ( int )S.length(), 0 ) != -1 );
}

int LUDPSocket::GetIPProtocol() { return IPPROTO_UDP; }
int LTCPSocket::GetIPProtocol() { return IPPROTO_TCP; }

int LUDPSocket::GetSockType() { return SOCK_DGRAM; }
int LTCPSocket::GetSockType() { return SOCK_STREAM;}

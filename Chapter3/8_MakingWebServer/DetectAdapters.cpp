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

#include "DetectAdapters.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef min
#undef max
#include <iphlpapi.h>

HMODULE IPHlpHandle = NULL;

typedef DWORD ( WINAPI* GetAdaptersInfo_func )( PIP_ADAPTER_INFO, PULONG );

GetAdaptersInfo_func pGetAdaptersInfo = NULL;

/// Link to IP helper library and get the ip enumerator function
void Net_ReloadIPHlp()
{
	if ( IPHlpHandle != NULL ) { return; }

	IPHlpHandle = LoadLibrary( "iphlpapi.dll" ); // std, but not very std...

	pGetAdaptersInfo = ( GetAdaptersInfo_func )GetProcAddress( IPHlpHandle, "GetAdaptersInfo" );
}

#else

/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstring>
#include <new>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>

#include <net/if.h>

#include <memory.h>

#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <errno.h>

#include <arpa/inet.h>

#include <linux/ip.h>
#include <linux/icmp.h>


// Android (bionic) doesn't have getifaddrs(3)/freeifaddrs(3).
// We fake it here, so java_net_NetworkInterface.cpp can use that API
// with all the non-portable code being in this file.

// Source-compatible subset of the BSD struct.
struct ifaddrs
{
	// Pointer to next struct in list, or NULL at end.
	ifaddrs* ifa_next;

	// Interface name.
	char* ifa_name;

	// Interface flags.
	unsigned int ifa_flags;

	// Interface address.
	sockaddr* ifa_addr;

	ifaddrs( ifaddrs* next )
		: ifa_next( next ), ifa_name( NULL ), ifa_flags( 0 ), ifa_addr( NULL )
	{
	}

	~ifaddrs()
	{
		delete ifa_next;
		delete[] ifa_name;
		delete ifa_addr;
	}

	// Sadly, we can't keep the interface index for portability with BSD.
	// We'll have to keep the name instead, and re-query the index when
	// we need it later.
	bool setNameAndFlagsByIndex( int interfaceIndex )
	{
		// Get the name.
		char buf[IFNAMSIZ];
		char* name = if_indextoname( interfaceIndex, buf );

		if ( name == NULL )
		{
			return false;
		}

		ifa_name = new char[strlen( name ) + 1];
		strcpy( ifa_name, name );

		// Get the flags.
		int fd = socket( AF_INET, SOCK_DGRAM, 0 );

		if ( fd == -1 )
		{
			return false;
		}

		ifreq ifr;
		memset( &ifr, 0, sizeof( ifr ) );
		strcpy( ifr.ifr_name, name );
		int rc = ioctl( fd, SIOCGIFFLAGS, &ifr );

		if ( rc == -1 )
		{
			close( fd );
			return false;
		}

		ifa_flags = ifr.ifr_flags;
		close( fd );
		return true;
	}

	// Netlink gives us the address family in the header, and the
	// sockaddr_in or sockaddr_in6 bytes as the payload. We need to
	// stitch the two bits together into the sockaddr that's part of
	// our portable interface.
	void setAddress( int family, void* data, size_t byteCount )
	{
		sockaddr_storage* ss = new sockaddr_storage;
		ss->ss_family = family;

		if ( family == AF_INET )
		{
			void* dst = &reinterpret_cast<sockaddr_in*>( ss )->sin_addr;
			memcpy( dst, data, byteCount );
		}
		else if ( family == AF_INET6 )
		{
			void* dst = &reinterpret_cast<sockaddr_in6*>( ss )->sin6_addr;
			memcpy( dst, data, byteCount );
		}

		ifa_addr = reinterpret_cast<sockaddr*>( ss );
	}
};

// FIXME: use iovec instead.
struct addrReq_struct
{
	nlmsghdr netlinkHeader;
	ifaddrmsg msg;
};

inline bool sendNetlinkMessage( int fd, const void* data, size_t byteCount )
{
	ssize_t sentByteCount = TEMP_FAILURE_RETRY( send( fd, data, byteCount, 0 ) );
	return ( sentByteCount == static_cast<ssize_t>( byteCount ) );
}

inline ssize_t recvNetlinkMessage( int fd, char* buf, size_t byteCount )
{
	return TEMP_FAILURE_RETRY( recv( fd, buf, byteCount, 0 ) );
}

// Source-compatible with the BSD function.
inline int getifaddrs( ifaddrs** result )
{
	// Simplify cleanup for callers.
	*result = NULL;

	// Create a netlink socket.
	int fd = socket( PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE );

	if ( fd < 0 )
	{
		return -1;
	}

	// Ask for the address information.
	addrReq_struct addrRequest;
	memset( &addrRequest, 0, sizeof( addrRequest ) );
	addrRequest.netlinkHeader.nlmsg_flags = NLM_F_REQUEST | NLM_F_MATCH;
	addrRequest.netlinkHeader.nlmsg_type = RTM_GETADDR;
	addrRequest.netlinkHeader.nlmsg_len = NLMSG_ALIGN( NLMSG_LENGTH( sizeof( addrRequest ) ) );
	addrRequest.msg.ifa_family = AF_UNSPEC; // All families.
	addrRequest.msg.ifa_index = 0; // All interfaces.

	if ( !sendNetlinkMessage( fd, &addrRequest, addrRequest.netlinkHeader.nlmsg_len ) )
	{
		close( fd );
		return -1;
	}

	// Read the responses.
//    LocalArray<0> buf(65536); // We don't necessarily have std::vector.
	const int buf_size = 65536;

	char* buf = ( char* )alloca( buf_size );

	ssize_t bytesRead;

	while ( ( bytesRead  = recvNetlinkMessage( fd, &buf[0], buf_size ) ) > 0 )
	{
		nlmsghdr* hdr = reinterpret_cast<nlmsghdr*>( &buf[0] );

		for ( ; NLMSG_OK( hdr, bytesRead ); hdr = NLMSG_NEXT( hdr, bytesRead ) )
		{
			switch ( hdr->nlmsg_type )
			{
				case NLMSG_DONE:
					close( fd );
					return 0;

				case NLMSG_ERROR:
					close( fd );
					return -1;

				case RTM_NEWADDR:
				{
					ifaddrmsg* address = reinterpret_cast<ifaddrmsg*>( NLMSG_DATA( hdr ) );
					rtattr* rta = IFA_RTA( address );
					size_t ifaPayloadLength = IFA_PAYLOAD( hdr );

					while ( RTA_OK( rta, ifaPayloadLength ) )
					{
						if ( rta->rta_type == IFA_LOCAL )
						{
							int family = address->ifa_family;

							if ( family == AF_INET || family == AF_INET6 )
							{
								*result = new ifaddrs( *result );

								if ( !( *result )->setNameAndFlagsByIndex( address->ifa_index ) )
								{
									close( fd );
									return -1;
								}

								( *result )->setAddress( family, RTA_DATA( rta ), RTA_PAYLOAD( rta ) );
							}
						}

						rta = RTA_NEXT( rta, ifaPayloadLength );
					}
				}
				break;
			}
		}
	}

	close( fd );
	// We only get here if recv fails before we see a NLMSG_DONE.
	return -1;
}

// Source-compatible with the BSD function.
inline void freeifaddrs( ifaddrs* addresses ) { delete addresses; }

#endif  // end of crazy android stuff


bool Net_EnumerateAdapters( std::vector<sAdapterInfo>& Adapters )
{
// use GetAdapterInfo()  - iphlpapi.dll
#ifdef _WIN32
	Net_ReloadIPHlp();

	/* Declare and initialize variables */
	PIP_ADAPTER_INFO pAdapterInfo, pAdapter = NULL;
	DWORD dwRetVal = 0;

	ULONG ulOutBufLen = sizeof( IP_ADAPTER_INFO );
	pAdapterInfo = ( IP_ADAPTER_INFO* ) malloc( ulOutBufLen );

	if ( pAdapterInfo == NULL ) { printf( "Error in malloc/GetAdaptersinfo\n" ); return 1; }

	// Make an initial call to GetAdaptersInfo to get the necessary size into the ulOutBufLen variable
	if ( pGetAdaptersInfo( pAdapterInfo, &ulOutBufLen ) == ERROR_BUFFER_OVERFLOW )
	{
		free( pAdapterInfo );
		pAdapterInfo = ( IP_ADAPTER_INFO* ) malloc( ulOutBufLen );

		if ( pAdapterInfo == NULL ) { printf( "Error in malloc/GetAdaptersinfo(2)\n" ); return 1; }
	}

	dwRetVal = pGetAdaptersInfo( pAdapterInfo, &ulOutBufLen );

	if ( dwRetVal == NO_ERROR )
	{
		pAdapter = pAdapterInfo;

		while ( pAdapter )
		{
			sAdapterInfo Info;
			strcpy( Info.FName, pAdapter->Description );
			strcpy( Info.FID, pAdapter->AdapterName );
			strcpy( Info.FIP, pAdapter->IpAddressList.IpAddress.String );

			Adapters.push_back( Info );

			pAdapter = pAdapter->Next;
		}
	}
	else
	{
		/* FLastError = "GetAdaptersInfo failed with error: %d\n", dwRetVal */;
		return false;
	}

	if ( pAdapterInfo ) { free( pAdapterInfo ); }

#else
	// universal POSIX getifaddrs() - linux/android, macos
	struct ifaddrs* MyAddrs, *ifa;
	void* in_addr;
	char buf[64];

	if ( getifaddrs( &MyAddrs ) != 0 ) { return false; }

	int Idx = 0;

	for ( ifa = MyAddrs; ifa != NULL; ifa = ifa->ifa_next )
	{
		if ( ( ifa->ifa_addr == NULL ) || !( ifa->ifa_flags & IFF_UP ) ) { continue; }

		switch ( ifa->ifa_addr->sa_family )
		{
			case AF_INET:
			{ in_addr = &( ( struct sockaddr_in* )ifa->ifa_addr )->sin_addr;   break; }

			case AF_INET6:
			{ in_addr = &( ( struct sockaddr_in6* )ifa->ifa_addr )->sin6_addr; break; }

			default:
				continue;
		}

		if ( inet_ntop( ifa->ifa_addr->sa_family, in_addr, buf, sizeof( buf ) ) )
		{
			sAdapterInfo Info;
			strcpy( Info.FName, ifa->ifa_name );
			strcpy( Info.FIP, buf );
			sprintf( Info.FID, "%d", Idx );
			Adapters.push_back( Info );
			Idx++;
		}
	}

	freeifaddrs( MyAddrs );
#endif

	return true;
}

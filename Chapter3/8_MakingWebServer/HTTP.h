/*
   Simple HTTP server derived from Rene Nyffenegger's source code.
   - Removed any OS dependencies
   - Reformatted source code
   - Binary file uploading in GET method
   - Added support for POST method to upload files

   Original source code notice:
   ============================

   Copyright (C) 2003-2004 René Nyffenegger

   This source code is provided 'as-is', without any express or implied
   warranty. In no event will the author be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this source code must not be misrepresented; you must not
      claim that you wrote the original source code. If you use this source code
      in a product, an acknowledgment in the product documentation would be
      appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
      misrepresented as being the original source code.

   3. This notice may not be removed or altered from any source distribution.

   René Nyffenegger rene.nyffenegger@adp-gmbh.ch

   http://www.adp-gmbh.ch/
*/

#ifndef _HTTP_H_INCLUDED_
#define _HTTP_H_INCLUDED_

#include "Sockets.h"
#include "Thread.h"
#include <map>

/// \brief HTTP server request/response structure
struct sHTTPServerRequest
{
	/// Empty constructor
	sHTTPServerRequest() {}

	#pragma region Binary responce buffer

	/// Is the answer binary. If yes, the FData is used. FAnswer string is used otherwise.
	bool FBinaryAnswer;

	/// Data for binary response. Managed externally. Mus be freed with delete[] call
	unsigned char* FData;

	/// Size of binary response buffer
	int FDataLen;

	#pragma endregion

	/// Internal TCP socket handle
	LTCPSocket* FSocket;

	/// GET or POST method signature
	std::string FMethod;

	/// Path part of the URI
	std::string FPath;

	/// Dictionary with all HTTP request parameters (URI part after '?' symbol)
	std::map<std::string, std::string> FParams;

	/// Information about referer
	std::string FReferer;

	#pragma region

	std::string FAccept;
	std::string FAcceptLanguage;
	std::string FAcceptEncoding;
	std::string FUserAgent;

	#pragma endregion

	/// Server error status (202, 404 etc) stored as string
	std::string FStatus;

	/// The actual response body
	std::string FAnswer;

	#pragma region POST method support

	std::string FContent;
	std::string FContentType;
	std::string FContentLength;
	std::string FContentDisposition;

	#pragma endregion
};

class clHTTPServerThread: public iThread
{
public:
	clHTTPServerThread(): FBindAddress( "127.0.0.1" ), FPort( 8080 ), FMaxConnections( 5 ) {}
	virtual ~clHTTPServerThread() {}

	virtual void Init() {}
	virtual void Run();
	virtual void Finish() {}

public:
	/// Maximum number of simultaneous connections
	int FMaxConnections;

	/// Address which the server is bound to
	std::string FBindAddress;

	/// Port on which the server resides
	int FPort;

	/// Socket accepting incoming connections
	LTCPSocket* FSocket;
};

#endif

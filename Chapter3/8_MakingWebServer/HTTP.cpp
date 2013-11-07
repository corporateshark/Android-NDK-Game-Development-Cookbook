#include "HTTP.h"

#if _MSC_VER >= 1400
#  define Lvsnprintf _vsnprintf_s
#  define Lsnprintf  _snprintf_s
#else
#  define Lvsnprintf vsnprintf
#  define Lsnprintf  snprintf
#endif


#include <sstream>
#include <stdio.h>

#if defined( ANDROID )
#  include "Wrapper_Android.h"
#else
#  include "Wrapper_Windows.h"
#endif

volatile bool g_ServerStarted;
volatile bool g_StartedThread;
volatile bool g_GotSocket;
volatile bool g_RunningThread;

std::string IntToStr( int FromUInt32 )
{
	char buf[64];

	Lsnprintf( buf, 63, "%i", FromUInt32 );

	return std::string( buf );
}

void HTTPSplitGetReq( std::string _req, std::string& path, std::map<std::string, std::string>& params )
{
	// Remove trailing newlines
	for ( int i = 0 ; i < 2 ; i++ )
		if ( _req[_req.size() - 1] == '\x0d' || _req[_req.size() - 1] == '\x0a' ) { _req = _req.substr( 0, _req.size() - 1 ); }

	// Remove potential Trailing HTTP/1.x
	if ( _req.size() > 7 )
	{
		if ( _req.substr( _req.size() - 8, 7 ) == "HTTP/1." )
		{
			_req = _req.substr( 0, _req.size() - 9 );
		}
	}

	std::string::size_type qm = _req.find( "?" );

	if ( qm == std::string::npos )
	{
		path = _req;
		return;
	}

	std::string url_params = _req.substr( qm + 1 );

	path = _req.substr( 0, qm );

	// Appending a '&' so that there are as many '&' as name-value pairs.
	// It makes it easier to split the url for name value pairs, he he he
	url_params += "&";

	std::string::size_type next_amp = url_params.find( "&" );

	while ( next_amp != std::string::npos )
	{
		std::string name_value = url_params.substr( 0, next_amp );
		url_params         = url_params.substr( next_amp + 1 );
		next_amp           = url_params.find( "&" );

		std::string::size_type pos_equal = name_value.find( "=" );

		std::string nam = name_value.substr( 0, pos_equal );
		std::string val = name_value.substr( pos_equal + 1 );

		std::string::size_type pos_plus;

		while ( ( pos_plus = val.find( "+" ) ) != std::string::npos )
		{
			val.replace( pos_plus, 1, " " );
		}

		// Replacing %xy notation
		std::string::size_type pos_hex = 0;

		while ( ( pos_hex = val.find( "%", pos_hex ) ) != std::string::npos )
		{
			int i;
			std::stringstream h;
			h << val.substr( pos_hex + 1, 2 );
			h << std::hex;
			h >> i;

			val.replace( pos_hex, 3, IntToStr( i ) );
			pos_hex ++;
		}

		params.insert( std::map<std::string, std::string>::value_type( nam, val ) );
	}
}

void SetVariableValue( const std::string& VarName, const std::string& VarValue )
{
}

std::string GetVariableValue( const std::string& VarName )
{
	return "";
}

class clHTTPRequestThread: public iThread
{
public:
	clHTTPRequestThread() {}
	virtual ~clHTTPRequestThread() {}

	virtual void Run();

	void HandleRequest( sHTTPServerRequest* r );

	/// Link to parent server
	clHTTPServerThread* FServer;

	/// Link to the incoming socket
	LTCPSocket* FSocket;
};

std::string CompressSpaces( const std::string& s )
{
	std::string out = "";

	for ( size_t j = 0 ; j < s.length() ; j++ )
	{
		char cc = s[j];

		if ( cc == ' ' || cc == '\n' || cc == '\r' ) { continue; }

		out += cc;
	}

	return out;
}

/// TODO: Handle %0A etc.
std::string Htmlize( const std::string _in )
{
	std::string _out = "<pre>";

	for ( size_t j = 0 ; j < _in.length() ; j++ )
	{
		if ( _in[j] == 10 || _in[j] == 13 ) { _out += "</pre><br><pre>"; }
		else { _out += _in[j]; }
	}

	return _out + "</pre>";
}

void clHTTPRequestThread::HandleRequest( sHTTPServerRequest* r )
{
	std::string title;
	std::string body;
	std::string bgcolor = "#ffffff";
	std::string links =
	   "<br><a href='/RemoteConsole'>Remote Console</a> "
	   "<br><a href='/Variables'>Change and inspect variables</a> "
	   "<br><a href='/UploadFile'>Upload a file to the engine</a> ";

	std::string connection_info =
	   std::string( "<br><hr>" ) +
	   std::string( "Connection information" ) +
	   std::string( "<table>" ) +
	   "<hr>"
	   "<tr><td>Referer:</td><td>"         + r->FReferer        + "</td></tr>" +
	   "<tr><td>Accept:</td><td>"          + r->FAccept         + "</td></tr>" +
	   "<tr><td>Accept-Encoding:</td><td>" + r->FAcceptEncoding + "</td></tr>" +
	   "<tr><td>Accept-Language:</td><td>" + r->FAcceptLanguage + "</td></tr>" +
	   "<tr><td>User-Agent:</td><td>"      + r->FUserAgent      + "</td></tr>" +
	   "</table>";

	r->FBinaryAnswer = false;

	if ( r->FPath == "/" )
	{
		title = "Test Web Server";

		body  = std::string( "<h1>Test Web Server</h1>" ) + links + "<br>" + connection_info;
	}
	else if ( r->FPath == "/Variables" )
	{
		title   = "Manage variables";

		body =
		   "<b>Set variable value</b>"
		   "<hr>"

		   "<form name=\"test\" action='/SetValue'>"
		   "<p><b>Variable:</b><br>"
		   "<input name='VarName' type='text' size='60'>"
		   "<p><b>Value:</b><br>"
		   "<input name='VarValue' type='text' size='60'>"
		   "<p><input type=\"submit\" value=\"Set\">"
		   "<input type=\"reset\" value=\"Clear\"></p>"
		   "</form>"
		   "</p>"
		   "<hr>"

		   "<b>Get variable value</b>"
		   "<hr>"

		   "<form name=\"test\" action='/GetValue'>"
		   "<p><b>Variable:</b><br>"
		   "<input name='VarName' type='text' size='40'>"
		   "<p><input type=\"submit\" value=\"Get\">"
		   "<input type=\"reset\" value=\"Clear\"></p>"
		   " </form>"
		   "<hr>"

		   "<hr><br><a href='/'>Back to main</a> ";
	}
	else if ( r->FPath == "/SetValue" )
	{
		body += "<br>Var name : " + r->FParams["VarName"];
		body += "<br>Var value: " + r->FParams["VarValue"];

		SetVariableValue( r->FParams["VarName"], r->FParams["VarValue"] );

		body += "<hr><br><a href='/Variables'>Set another variable</a> ";
		body += "<br><a href='/'>Back to main</a> ";
	}
	else if ( r->FPath == "/GetValue" )
	{
		body += "<br>Var name : " + r->FParams["VarName"];
		body += "<br>Var value: " + GetVariableValue( r->FParams["VarName"] );

		body += "<hr><br><a href='/Variables'>Get another variable</a> ";
		body += "<br><a href='/'>Back to main</a> ";
	}
	else if ( r->FPath == "/UploadFile" )
	{
		/// Test large file uploading
		title = "Large file upload";

		body =
		   "<b>Enter file name and click Submit</b>"
		   "<form method=\"POST\" action=\"/upload.php\" enctype=\"multipart/form-data\">"
		   "<input type='file' name='myfile1' size='60'>"
		   "<input type='submit' value='Submit'>"
		   "<input type='reset' value='Reset' name='B2'>"
		   "</form>"

		   "<hr><br><a href='/'>Back to main</a> ";
	}
	else
	{
		/// Were we able to find the file ?
		if ( !r->FBinaryAnswer )
		{
			r->FStatus = "404 Not Found";
			title      = "Unsupported service URL";
			body       = "<h1>Unsupported service URL or invalid shared file name</h1>";
			body      += "Path is : &gt;" + r->FPath + "&lt;";
		}
	}

	if ( !r->FBinaryAnswer )
	{
		r->FAnswer  = "<html><head><title>";
		r->FAnswer += title;
		r->FAnswer += "</title></head><body bgcolor='" + bgcolor + "'>";
		r->FAnswer += body;
		r->FAnswer += "</body></html>";
	}
}

void clHTTPRequestThread::Run()
{
	int MaxLineLen = 1024;

	g_RunningThread = true;

	FSocket->SetNonBlocking( true );

	std::string line = FSocket->ReceiveLine( MaxLineLen );

	if ( line.empty() ) { return; }

	sHTTPServerRequest req;

	if ( line.find( "GET" ) == 0 )
	{
		req.FMethod = "GET";
	}
	else if ( line.find( "POST" ) == 0 )
	{
		req.FMethod = "POST";
	}

	std::string path;
	std::map<std::string, std::string> params;

	// cut POST/GET from the beginning of the string
	size_t posStartPath = line.find_first_not_of( " ", req.FMethod.length() );

	HTTPSplitGetReq( line.substr( posStartPath ), path, params );

	req.FStatus = "202 OK";
	req.FSocket = FSocket;
	req.FPath   = path;
	req.FParams = params;

	/// Ugly ad-hoc parser of request. Sorry, no boost::Spirit or even GoldParser here.

	static const std::string accept          = "Accept: "             ;
	static const std::string accept_language = "Accept-Language: "    ;
	static const std::string accept_encoding = "Accept-Encoding: "    ;
	static const std::string user_agent      = "User-Agent: "         ;

	static const std::string referer = "Referer: ";

	static const std::string content_length      = "Content-Length: ";
	static const std::string content_type        = "Content-Type: ";
	static const std::string content_disposition = "Content-Disposition: ";

	std::string the_boundary;
	req.FBinaryAnswer = false;
	req.FContent.clear();
	req.FContentType.clear();
	req.FContentLength.clear();
	req.FContentDisposition.clear();

	bool reading_data = false;

	FPendingExit = false;

	/// We also check for timeouts and abnormal terminations here
	while ( !IsPendingExit() )
	{
		line = FSocket->ReceiveLine( MaxLineLen );

		if ( req.FMethod == "GET" )
			if ( line.empty() ) { break; }

		int pos_cr_lf = ( int )line.find_first_of( "\x0a\x0d" );

		if ( req.FMethod == "GET" )
			if ( pos_cr_lf == 0 ) { break; }

		line = line.substr( 0, ( size_t )pos_cr_lf );

		if ( line.empty() )
		{
			break;
		}
		else if ( line.substr( 0, 2 ) == "--" )
		{
			if ( the_boundary.empty() )
			{
				the_boundary = line;
				reading_data = true;
			}
			else
			{
				break;
			}
		}
		else if ( line.substr( 0, referer.size() ) == referer )
		{
			req.FReferer = line.substr( referer.size() );
		}
		else if ( line.substr( 0, accept.size() ) == accept )
		{
			req.FAccept = line.substr( accept.size() );
		}
		else if ( line.substr( 0, accept_language.size() ) == accept_language )
		{
			req.FAcceptLanguage = line.substr( accept_language.size() );
		}
		else if ( line.substr( 0, accept_encoding.size() ) == accept_encoding )
		{
			req.FAcceptEncoding = line.substr( accept_encoding.size() );
		}
		else if ( line.substr( 0, user_agent.size() ) == user_agent )
		{
			req.FUserAgent = line.substr( user_agent.size() );
		}
		else if ( line.substr( 0, content_length.size() ) == content_length )
		{
			req.FContentLength = line.substr( content_length.size() );
		}
		else if ( line.substr( 0, content_type.size() ) == content_type )
		{
			req.FContentType = line.substr( content_type.size() );
		}
		else if ( line.substr( 0, content_disposition.size() ) == content_disposition )
		{
			req.FContentDisposition = line.substr( content_disposition.size() );
		}
		else
		{
			if ( !the_boundary.empty() && reading_data )
			{
				// request body for POST
				req.FContent += line + std::string( "\r\n" );
			}
		}
	}

	HandleRequest( &req );

	std::string SizeString = req.FBinaryAnswer ? IntToStr( req.FDataLen ) : IntToStr( ( int )( req.FAnswer.size() ) );

	static std::string const serverName = "Test Web server (Platform info)";

	FSocket->SendBytes( "HTTP/1.1 " );

	FSocket->SendLine( req.FStatus );
	FSocket->SendLine( std::string( "Server: " ) + serverName );
	FSocket->SendLine( "Connection: close" );

	if ( req.FBinaryAnswer )
	{
		FSocket->SendLine( "Content-Type: application/octet-stream" );
	}
	else
	{
		FSocket->SendLine( "Content-Type: text/html; charset=ISO-8859-1" );
	}

	FSocket->SendLine( "Content-Length: " + SizeString );
	FSocket->SendLine( "" );

	if ( req.FBinaryAnswer )
	{
		if ( req.FData )
		{
			FSocket->WriteBytes( req.FData, req.FDataLen );
			delete[] req.FData;
		}
	}
	else
	{
		FSocket->SendLine( req.FAnswer );
	}

	FSocket->Close();

	delete FSocket;
	delete this;
}

/////////////////////////// End of client thread

void clHTTPServerThread::Run()
{
	LTCPSocket* in = new LTCPSocket();

	if ( !in->Open() )
	{
		LOGI( "WARNING: Cannot open TCP socket" );
		return;
	}

	in->SetNonBlocking( true );

	if ( !in->Bind( FBindAddress, FPort ) )
	{
		LOGI( "WARNING: Cannot bind TCP socket to %d port", FPort );
		LOGI( "WARNING: Net error: %s", LNetwork_LastErrorAsString().c_str() );
		delete in;
		return;
	}

	if ( !in->Listen( FMaxConnections ) )
	{
		LOGI( "WARNING: Cannot start listening TCP socket for HTTP server" );
		delete in;
		return;
	}

	// Running threads' list
	FPendingExit = false;

	LOGI( "INFO: Starting HTTP server at %s", FBindAddress.c_str() );

	g_ServerStarted = true;

	while ( !IsPendingExit() )
	{
		LTCPSocket* NewSocket = in->Accept();

		g_GotSocket = true;

		if ( NewSocket != 0 )
		{
			// Got socket
			// Add new thread
			clHTTPRequestThread* T = new clHTTPRequestThread();
			T->FServer = this;
			T->FSocket = NewSocket;

			g_StartedThread = true;

			T->Start( iThread::Priority_Lowest );
		}
	}

	in->Close();
	delete in;
}

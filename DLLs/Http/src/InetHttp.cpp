#include "stdafx.h"
#include "InetHttp.h"

CInetHttp::CInetHttp( )
{
	this->m_szSystemError = NULL;
	m_IsMultiPart         = false;

	TCHAR *szScheme = new TCHAR[ 1000 ];
	TCHAR *szHostName = new TCHAR[ 1000 ];
	TCHAR *szPassword = new TCHAR[ 1000 ];
	TCHAR *szUrlPath = new TCHAR[ 1000 ];
	TCHAR *szUserName = new TCHAR[ 1000 ];
	TCHAR *szExtraInfo = new TCHAR[ 1000 ];

	components.dwStructSize = sizeof( components );
	components.lpszExtraInfo = szExtraInfo;
	components.lpszHostName  = szHostName;
	components.lpszPassword  = szPassword;
	components.lpszScheme    = szScheme;
	components.lpszUrlPath   = szUrlPath;
	components.lpszUserName  = szUserName;

	m_hRequest = 0;
	m_hConnect = 0;
	m_hOpen = 0;

	byteBuffer = new BYTE[ BUFFER_REQUEST_IMAGE ];

	m_strProxy     = "";
	m_strUserProxy = "";
	m_strPwdProxy  = "";

	dwPositionBuffer = 0;
}

CInetHttp::~CInetHttp( )
{
	delete components.lpszExtraInfo;
	delete components.lpszHostName;
	delete components.lpszPassword;
	delete components.lpszScheme;
	delete components.lpszUrlPath;
	delete components.lpszUserName;
	delete byteBuffer;

	Close( );
}

bool CInetHttp::Request( const CString & url, const int method, CString agent )
{
	bool ret;

	if ( ! Connect( url, agent ) ) return( false );

	switch( method )
	{
		case CInetHttp::RequestPostMethod :
		default:
			if( m_IsMultiPart ) ret = RequestPostMultiPart( );
			else ret = RequestPost( );
	}

	if( ret ) GetResponse( );
	Close( );

	return( ret );
}

void CInetHttp::AddArguments(const CString & name, const int value)
{
	CString str;
	
	str.Format( L"%i", value );
	AddArguments( name, str );
}

void CInetHttp::AddArguments(const CString & name, const CString & value, ContentType type)
{
	HTTPArgument arg;

	arg.strName  = name;
	arg.strValue = value;
	arg.dwType   = type;

	if ( type != Normal )
		m_IsMultiPart = true;

	this->m_vArguments.push_back( arg );
}

CString CInetHttp::GetArguments(void)
{
	std::vector<HTTPArgument>::const_iterator iter;
	CString ret;

	ret = "";
	for( iter = m_vArguments.begin( ); iter < m_vArguments.end( );)
	{
		ret += iter->strName;
		ret += L"=";
		ret += iter->strValue;
		
		if( ++iter < m_vArguments.end( ) )
			ret += L"&";
	}

	return( ret );
}

BOOL CInetHttp::IsConnected(const CString url)
{
	return InternetCheckConnection(url, FLAG_ICC_FORCE_CONNECTION, 0);
}

DWORD CInetHttp::GetMultiPartArguments(BYTE *buffer)
{
	DWORD  dwPosition = 0;
	HANDLE hFile;


	std::vector<HTTPArgument>::const_iterator iter;
	CString strNormal = L"--%s\r\n"
					    L"Content-Disposition: form-data; name=\"%s\"\r\n"
					    L"\r\n"
					    L"%s\r\n";

	CString strBinary = L"--%s\r\n"
						L"Content-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\n"
						L"Content-Type: %s\r\n"
						L"\r\n";
	CString str;
	CString boundary = L"--MULTI-PARTS-FORM-DATA-BOUNDARY";
	
	for( iter = m_vArguments.begin( ); iter < m_vArguments.end( ); iter++ )
	{
		switch( iter->dwType )
		{
			case Normal:
				str = "";
				str.Format( strNormal, boundary, iter->strName, iter->strValue );
				CopyByte( ConvertStringToByte( str ), ( buffer + dwPosition ) , str.GetLength( ) );
				dwPosition += str.GetLength( );
				break;
			case Binary:
				str = "";
				str.Format( strBinary, boundary, iter->strName, iter->strValue,
							GetContentType( ( LPCTSTR ) iter->strValue ) );

				CopyByte( ConvertStringToByte( str ), ( buffer + dwPosition ), str.GetLength( ) );
				dwPosition += str.GetLength( );

				if ( ( hFile = CreateFile( iter->strValue, GENERIC_READ, FILE_SHARE_READ,
										   NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
										   NULL ) ) == INVALID_HANDLE_VALUE )
				{
					SetError( );
					return( 0 );
				}

				DWORD dwSize = GetFileSize( hFile, NULL );

				if( dwSize == 0xFFFFFFFF )
				{
					SetError( );
					return( 0 );
				}

				BYTE  pFileBuffer[ 200 ];
				DWORD dwBytesToRead = 200;
				DWORD dwBytesRead   = 0;


				//while( ( dwBytesRead = fread( pFileBuffer, sizeof( pFileBuffer ), 200, file ) ) )
				//{
				while( ReadFile( hFile, ( BYTE * )pFileBuffer, dwBytesToRead, &dwBytesRead, NULL ) &&
					   dwBytesRead > 0 )
				{
					CopyByte( pFileBuffer, ( buffer + dwPosition ), dwBytesRead );
					dwPosition += dwBytesRead;
					memset( pFileBuffer, 0, 200 );
				}

				CloseHandle( hFile );
				*( buffer + dwPosition ) = '\r';
				dwPosition++;
				*( buffer + dwPosition ) = '\n';
				dwPosition++;

				break;
		}
	}

	BYTE final1[ 2 ] = { '-', '-' };
	BYTE final2[ 4 ] = { '-', '-', '\r', '\n' };

	CopyByte( final1, ( buffer + dwPosition ), 2 );
	dwPosition += 2;
	CopyByte( ConvertStringToByte( boundary ), ( buffer + dwPosition ), boundary.GetLength( ) );
	dwPosition += boundary.GetLength( );
	CopyByte( final2, ( buffer + dwPosition ), 4 );
	dwPosition += 4;

	return( dwPosition );
}

CString CInetHttp::GetContentType( CString file )
{
	HKEY    hKEY;
	CString strExt = file.Right( 4 );
	TCHAR   pValue[1024];
	DWORD   dwLen = 1024, dwType = 0;
	CString ret;

	ret = "application/octet-stream";
	if( RegOpenKeyEx( HKEY_CLASSES_ROOT, ( LPCTSTR ) strExt, 0, KEY_QUERY_VALUE, &hKEY )
		== ERROR_SUCCESS )
	{
		if( RegQueryValueEx( hKEY, L"Content Type", NULL, &dwType, ( LPBYTE ) pValue, &dwLen) == ERROR_SUCCESS)
			ret = pValue;

		RegCloseKey(hKEY);
	}

	return( ret );
}

bool CInetHttp::Connect(const CString & url, const CString & agent)
{
	memset( components.lpszExtraInfo, '\0', 1000 );
	memset( components.lpszHostName, '\0', 1000 );
	memset( components.lpszPassword, '\0', 1000 );
	memset( components.lpszScheme, '\0', 1000 );
	memset( components.lpszUrlPath, '\0', 1000 );
	memset( components.lpszUserName, '\0', 1000 );

	components.dwExtraInfoLength = 1000;
	components.dwHostNameLength  = 1000;
	components.dwPasswordLength  = 1000;
	components.dwSchemeLength    = 1000;
	components.dwUrlPathLength   = 1000;
	components.dwUserNameLength  = 1000;

	if ( ! InternetCrackUrl( ( LPCTSTR ) url, 0, 0, &components ) )
	{
		SetError( );
		return( false );
	}

	if ( ! ( m_hOpen = InternetOpen( ( LPCTSTR ) agent, INTERNET_OPEN_TYPE_PRECONFIG,
									  m_strProxy.GetLength( ) > 0 ? ( LPCTSTR ) m_strProxy : 0,
									  NULL, NULL ) ) )
	{
		SetError( );
		return( false );
	}

	if ( ! ( m_hConnect = InternetConnect( m_hOpen, components.lpszHostName,
										   components.nPort, NULL,
										   NULL, INTERNET_SERVICE_HTTP,
										   INTERNET_FLAG_NO_CACHE_WRITE |
										   INTERNET_FLAG_KEEP_CONNECTION,
										   0 ) ) )
	{
		SetError( );
		return( false );
	}

	return( true );
}

bool CInetHttp::Close(void)
{
	if( m_hRequest )
		::InternetCloseHandle( m_hRequest );

	if( m_hConnect )
		::InternetCloseHandle( m_hConnect );

	if( m_hOpen )
		::InternetCloseHandle( m_hOpen );

	m_hRequest = 0;
	m_hConnect = 0;
	m_hOpen = 0;

	return( true );
}

void CInetHttp::SetError( )
{
	m_dwSystemError = GetLastError( );

	//if( m_szSystemError ) delete m_szSystemError;

	if( m_dwSystemError > 12000 )
	{
		HMODULE hModule = GetModuleHandle( L"wininet.dll" );
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
					   hModule,
					   m_dwSystemError,
					   LANG_USER_DEFAULT,
					   (LPTSTR)&m_szSystemError,
					   0,
					   NULL);
	}
	else if( m_dwSystemError )
	{
		FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
					   NULL,
					   m_dwSystemError,
					   LANG_USER_DEFAULT,
					   (LPTSTR)&m_szSystemError,
					   0,
					   NULL);
	}

	if( m_hRequest )
	{
		DWORD dwBufferLength = 1000;
		DWORD dwSize = sizeof( DWORD );

		HttpQueryInfo( m_hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
					   &m_dwInternetError, &dwSize, NULL );

		HttpQueryInfo( m_hRequest, HTTP_QUERY_STATUS_TEXT, m_szInternetError, &dwBufferLength,
					   NULL );
	}
}

void CInetHttp::SetProxy(CString proxy, int port, CString user, CString pwd)
{
	CString s;
	s.Format(L"%s:%d", proxy, port);
	SetProxy(s, user, pwd);
}

void CInetHttp::SetProxy(CString proxy, CString user, CString pwd)
{
	this->m_strProxy     = proxy;
	this->m_strUserProxy = user;
	this->m_strPwdProxy  = pwd;
}

bool CInetHttp::RequestPost(void)
{
	static TCHAR szContentType[] = L"Content-Type: application/x-www-form-urlencoded; charset=ISO-8859-1";
	CString strArg = GetArguments( );

	BYTE *arg;
	if( ! strArg.IsEmpty( ) )
		arg = ConvertStringToByte( strArg );
	else
	{
		arg = new BYTE[ 1 ];
		*arg = '\0';
	}

	bool ret = DoRequest( L"POST", szContentType, ( DWORD ) _tcslen( szContentType ),
						  arg, strArg.GetLength( ) );

	delete [] arg;

	return( ret );
}

bool CInetHttp::RequestPostMultiPart(void)
{
	BYTE *pPost = new BYTE[ 2000000 ];

	memset( pPost, 0, 2000000 );
	DWORD dwPostBufferLength = GetMultiPartArguments( pPost );

	CONST TCHAR *szAcceptType[2] = {L"Accept: */*", NULL};
	LPCTSTR szAccept = L"Accept: */*\r\n";

	CString strContentLength;
	LPCTSTR szContentType= L"Content-Type: multipart/form-data; charset=ISO-8859-1; boundary=--MULTI-PARTS-FORM-DATA-BOUNDARY\r\n";
	strContentLength.Format( L"Content-Length: %d\r\n", dwPostBufferLength );

	DWORD dwCodeQuery           = 0;
	DWORD dwSize                = sizeof( DWORD );
	DWORD dwOutPostBufferLength = 0;

	INTERNET_BUFFERS InternetBufferIn={0};
	InternetBufferIn.dwStructSize = sizeof( INTERNET_BUFFERS );
	InternetBufferIn.Next         = NULL;	

	do
	{
		if ( ! ( m_hRequest = HttpOpenRequest( m_hConnect, L"POST", components.lpszUrlPath,
											   HTTP_VERSION,
											   L"", szAcceptType, 
											   INTERNET_FLAG_NO_CACHE_WRITE |
											   INTERNET_FLAG_RELOAD |
											   INTERNET_FLAG_KEEP_CONNECTION |
											   INTERNET_FLAG_FORMS_SUBMIT,
											   NULL ) ) )
		{
			SetError( );
			return( false );
		}

		if ( ! HttpAddRequestHeaders( m_hRequest, szAccept, ( DWORD ) _tcslen( szAccept ),
									  HTTP_ADDREQ_FLAG_REPLACE ) )
		{
			SetError( );
			return( false );
		}

		if ( ! HttpAddRequestHeaders( m_hRequest, szContentType, ( DWORD ) _tcslen( szContentType ),
									  HTTP_ADDREQ_FLAG_ADD_IF_NEW ) )
		{
			SetError( );
			return( false );
		}

		if ( ! HttpAddRequestHeaders( m_hRequest, ( LPCTSTR ) strContentLength,
									  ( DWORD ) strContentLength.GetLength( ),
									  HTTP_ADDREQ_FLAG_ADD_IF_NEW ) )
		{
			SetError( );
			return( false );
		}

		if( ! ::HttpSendRequestEx( m_hRequest, &InternetBufferIn, NULL, HSR_INITIATE | HSR_SYNC, 0 ) )
		{
			SetError( );
			if( ! CheckCert( ) ) return( false );
		}

		dwOutPostBufferLength=0;
		if( ! ::InternetWriteFile( m_hRequest, pPost, dwPostBufferLength, &dwOutPostBufferLength ) )
		{
			SetError( );
			return( false );
		}

		if( ! ::HttpEndRequest( m_hRequest, NULL, 0, 0 ) )
		{
			SetError( );
			return( false );
		}

		if( ! HttpQueryInfo( m_hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
							 &dwCodeQuery, &dwSize, NULL ) )
		{
			SetError( );
			return( false );
		}

		if( dwCodeQuery != 200 )
		{
			if( ! CheckAccess( dwCodeQuery ) )
				return( false );
			else
				::InternetCloseHandle( m_hRequest );
		}

	}while( dwCodeQuery != 200 );

	delete [] pPost;

	return( true );
}

bool CInetHttp::SetDataProxy(void)
{
	static int numberTried = 0;

	if( ! m_strProxy.IsEmpty( ) )
	{
		InternetSetOption(m_hRequest, INTERNET_OPTION_PROXY, 
						  ( LPVOID )( LPCTSTR ) m_strProxy, m_strProxy.GetLength( ) );

		InternetSetOption(m_hRequest, INTERNET_OPTION_PROXY_USERNAME, 
						  ( LPVOID )( LPCTSTR ) m_strUserProxy, m_strUserProxy.GetLength( ) );

		InternetSetOption(m_hRequest, INTERNET_OPTION_PROXY_PASSWORD, 
						  ( LPVOID )( LPCTSTR ) m_strPwdProxy, m_strPwdProxy.GetLength( ) );

		if ( ++numberTried > 3 && numberTried < 6 )
		{
			if( ::InternetErrorDlg( ::GetActiveWindow( ), m_hRequest,
									ERROR_INTERNET_INCORRECT_PASSWORD,
									FLAGS_ERROR_UI_FILTER_FOR_ERRORS |
									FLAGS_ERROR_UI_FLAGS_GENERATE_DATA | 
									FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS, NULL
									) == ERROR_CANCELLED )
				return( false );
		}
	}
	else
	{
		DWORD ret;
		HWND hw = ::GetActiveWindow( );
		if( ( ret = ::InternetErrorDlg( ::GetDesktopWindow( ), m_hRequest,
								  ERROR_INTERNET_INCORRECT_PASSWORD,
								  FLAGS_ERROR_UI_FILTER_FOR_ERRORS |
								  FLAGS_ERROR_UI_FLAGS_GENERATE_DATA | 
								  FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS, NULL
								) ) == ERROR_CANCELLED )
			return( false );

		int x = 0;
		if( ret == ERROR_INTERNET_FORCE_RETRY )
			x++;
		else if( ret == ERROR_INVALID_HANDLE )
			x++;
		else if( ret == ERROR_SUCCESS )
			x++;
	}


	return( true );
}

void CInetHttp::GetResponse(void)
{
	BYTE szBuffer[ 200 ];
	DWORD dwBuffer = 0;
	DWORD dwLength = 0;

	DWORD dwSize = 200; //sizeof( DWORD );


	if( ! m_hRequest )
		return;

	BOOL ret;
	m_strResponse = "";
	while( ( ret = InternetReadFile( m_hRequest, (LPVOID)szBuffer, 200, &dwBuffer ) ) && dwBuffer )
		m_strResponse += ConvertByteToString( szBuffer, dwBuffer );

	SetError( );
}

BYTE * CInetHttp::ConvertStringToByte(CString & value)
{
	int total = value.GetLength( );
	BYTE *ret = new BYTE[ value.GetLength( ) ];

	for( int x = 0; x < total; x++ )
		*( ret + x ) = ( BYTE ) value[ x ];
	
	return( ret );
}

CString CInetHttp::ConvertByteToString(BYTE * value, DWORD size)
{
	CString ret;

	for( DWORD x = 0; x < size; x++ )
		ret += *( value + x );

	return( ret);
}

void CInetHttp::CopyByte( BYTE *source, BYTE *dest, DWORD size )
{
	for( DWORD x = 0; x < size; x++ )
		*( dest + x ) = *( source + x );
}

bool CInetHttp::Download(const CString & url, CString target, CString agent)
{
	BYTE   szBuffer[ 200 ];
	DWORD  dwBuffer = 0;
	DWORD  dwBytesWritten = 0;
	HANDLE hFile;

	if( ! Connect( url, agent ) ) return( false );
	if( DoRequest( NULL, NULL, 0, NULL, 0 ) )
	{
		if( ! ( hFile = CreateFile( ( LPCTSTR ) target, GENERIC_WRITE, FILE_SHARE_WRITE,
									NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, NULL ) ) )
		{
			SetError( );
			Close( );
			return( false );
		}

		while( InternetReadFile( m_hRequest, (LPVOID)szBuffer, 200,
			                     &dwBuffer ) && dwBuffer )
		{
			if( ! WriteFile( hFile, szBuffer, dwBuffer, &dwBytesWritten, NULL ) )
			{
				SetError( );
				Close( );
				CloseHandle( hFile );
				return( false );
			}
		}

		if( ! CloseHandle( hFile ) )
		{
			SetError( );
			Close( );
			return( false );
		}
	}

	Close( );
	return( true );
}

bool CInetHttp::DoRequest( LPCTSTR method, LPCTSTR headers, DWORD headerLength, BYTE * arg,
					   DWORD argLength )
{
	BOOL  bSend;
	CONST TCHAR *szAcceptType[2] = {L"Accept: */*", NULL};

	DWORD dwCodeQuery = 0;
	DWORD dwSize      = sizeof( DWORD );

	if ( ! ( m_hRequest = HttpOpenRequest( m_hConnect, method, components.lpszUrlPath,
										   NULL,
										   HTTP_VERSION, szAcceptType, 
										   NULL, NULL ) ) )
	{
		SetError( );
		STLOG_WRITE(L"CInetHttp::DoRequest(): Tentativa de abrir conexão a [%s] falhou. Erro [%d]", components.lpszUrlPath,
			GetLastErrorCode());
		return( false );
	}

	LPCTSTR szAccept = L"Accept: */*\r\n";
	if ( ! HttpAddRequestHeaders( m_hRequest, szAccept, ( DWORD ) _tcslen( szAccept ),
								  HTTP_ADDREQ_FLAG_REPLACE ) )
	{
		SetError( );
		return( false );
	}

	do
	{
		if ( ! ( bSend = HttpSendRequest( m_hRequest, headers, headerLength,
										  ( LPVOID ) arg, argLength ) ) )
		{
			SetError( );
			STLOG_WRITE(L"CInetHttp::DoRequest(): Tentativa de requisitar conexão a [%s] falhou. Erro [%d]", components.lpszUrlPath,
				GetLastErrorCode());
			if( ! CheckCert( ) ) return( false );
		}

		if( ! HttpQueryInfo( m_hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
							 &dwCodeQuery, &dwSize, NULL ) )
		{
			SetError( );
			return( false );
		}

		STLOG_WRITE(L"CInetHttp::DoRequest(): Status de conexão a [%s]: [%d]", components.lpszUrlPath,
			dwCodeQuery);

		if( ! CheckAccess( dwCodeQuery ) )
		{
			SetError( );
			return( false );
		}
	} while( dwCodeQuery != 200 );

	return true;
}

bool CInetHttp::CheckCert(void)
{
	DWORD dwFlags;
	DWORD dwBuffLen   = sizeof( dwFlags );

	if( m_dwSystemError == ERROR_INTERNET_INVALID_CA )
	{
		InternetQueryOption( m_hRequest, INTERNET_OPTION_SECURITY_FLAGS,
							( LPVOID ) &dwFlags, &dwBuffLen);

		dwFlags |= SECURITY_FLAG_IGNORE_UNKNOWN_CA;
		InternetSetOption ( m_hRequest, INTERNET_OPTION_SECURITY_FLAGS,
							&dwFlags, sizeof (dwFlags) );
	}
	else if ( m_dwSystemError == ERROR_INTERNET_SEC_CERT_CN_INVALID )	
	{
		InternetQueryOption( m_hRequest, INTERNET_OPTION_SECURITY_FLAGS,
							( LPVOID ) &dwFlags, &dwBuffLen);

		dwFlags |= SECURITY_FLAG_IGNORE_CERT_CN_INVALID;
		InternetSetOption ( m_hRequest, INTERNET_OPTION_SECURITY_FLAGS,
							&dwFlags, sizeof (dwFlags) );

	}
	else return( false );

	return( true );
}

bool CInetHttp::CheckAccess( long nCode )
{
	BYTE szData[ 51 ];
	DWORD dwSize = sizeof( DWORD );

	if( nCode == HTTP_STATUS_PROXY_AUTH_REQ || nCode == HTTP_STATUS_DENIED )
	{
		do
		{
			InternetReadFile(m_hRequest, (LPVOID)szData, 50, &dwSize);
		}while( dwSize != 0 );

		if( ! SetDataProxy( ) )
		{
			SetError( );
			return( false );
		}
	}
	else if( nCode != 200 )
	{
		SetError( );
		return( false );
	}

	return( true );
}

DWORD CInetHttp::RequestMotionImage( const CString & url, LPBYTE lpImage,
							    DWORD dwLength,
								bool bKeepConection,
							    const int method,
							    CString agent )
{

	if( ! m_hRequest )
	{
		if ( ! Connect( url, agent ) ) return( 0 );

		switch( method )
		{
			case CInetHttp::RequestPostMethod :
			default:
				if( m_IsMultiPart )
				{
					if( ! RequestPostMultiPart( ) )
					{
						Close( );
						return( 0 );
					}
				}
				else
				{
					if( ! RequestPost( ) )
					{
						Close( );
						return( 0 );
					}
					::Sleep( 2000 );
				}
		}
	}

	DWORD dwBuffer = 0;
	CString contentLength;
	DWORD dwContentLength;
	DWORD dwContentPosition;

	memcpy( lpImage, L"\0", dwLength );

	m_strResponse = "";

	InternetReadFile( m_hRequest, (LPVOID) ( byteBuffer + dwPositionBuffer ),
					  BUFFER_REQUEST_IMAGE - dwPositionBuffer, &dwBuffer );

	int x = 0;
	int y = 0;
	for( x = 0; x < BUFFER_REQUEST_IMAGE + 1 && y < 2; x++ && ( ( *( byteBuffer + x ) == '\n' ) ? y++ : y = y ) );
	x++;
	for( ; *( byteBuffer + x ) != '\n'; x++ )
		contentLength += *( byteBuffer + x );
	x += 2;

	if( contentLength.Left(15) != L"Content-Length:" )
		STLOG_WRITE("CInetHttp::RequestMotionImage: Content-Length não encontrado");
		//TRACE( L"Content-Length não encontrado" );

	contentLength.Replace( L"Content-Length:", L"" );
	contentLength.Trim( );
	dwContentLength = _wtoi( contentLength );


	dwContentPosition = dwPositionBuffer + dwBuffer - x;
	if( dwContentLength < dwContentPosition )
	{
		dwContentPosition = dwContentLength;
		memcpy( ( LPVOID ) lpImage, ( LPVOID ) ( byteBuffer + x ), dwContentPosition );

		dwPositionBuffer = BUFFER_REQUEST_IMAGE - dwContentLength - x;
		//dwBuffer = BUFFER_REQUEST_IMAGE - dwPositionBuffer;
		memcpy( byteBuffer, byteBuffer + dwContentLength + x, dwPositionBuffer );
	}
	else
	{
		memcpy( ( LPVOID ) lpImage, ( LPVOID ) ( byteBuffer + x ), dwContentPosition );

		while( dwContentLength > dwContentPosition )
		{
			InternetReadFile( m_hRequest, (LPVOID)byteBuffer, BUFFER_REQUEST_IMAGE, &dwBuffer );

			if( ( dwContentPosition + dwBuffer ) >= dwContentLength )
			{
				dwPositionBuffer = ( dwContentPosition + dwBuffer ) - dwContentLength;
				dwBuffer -= dwPositionBuffer;

				memcpy( ( LPVOID ) ( lpImage + dwContentPosition ),
						( LPVOID ) byteBuffer, dwBuffer );

				memcpy( byteBuffer, byteBuffer + dwBuffer, dwPositionBuffer );
			}
			else
				memcpy( ( LPVOID ) ( lpImage + dwContentPosition ),
						( LPVOID ) byteBuffer, dwBuffer );

			dwContentPosition += dwBuffer;
		}
	}

	SetError( );

	if( ! bKeepConection )
	{
		dwPositionBuffer = 0;
		Close( );
	}

	return( dwContentPosition );
}

      

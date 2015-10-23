#include "StdAfx.h"
#include "HTTPCtrl.h"
#include "Connection.h"
#include "ChunkedTE.h"
#include "HttpResponse.h"
#include "search.h"
#include "ZipHandler.h"
#include "Base64.h"

const long CHTTPCtrl::REQUEST_SIZE_NOT_KNOWN = -1;

CHTTPCtrl::CHTTPCtrl(void)
	: m_connection(0)
	, m_port(DEF_HTTP_PORT)
	, m_timeout(15000)
	, m_proxyPort(0)
	, m_authDepth(0)
	, m_maxRedirects(5)
	, m_redirectOnPost(FALSE)
	, m_redirects(0)
	, m_requestSize(REQUEST_SIZE_NOT_KNOWN)
	, m_inHeaders(TRUE)
	, m_compression(FALSE)
	, m_compressionIsGzip(FALSE)
	, m_compressionLevel(-1) // Z_DEFAULT_COMPRESSION
	, m_method("GET")
	, m_sendConnectionCloseHeader(FALSE)
	, m_tfrDecoder(NULL)
	, m_requestBody(NULL)
{
	m_headers.AddDefault("User-Agent", USER_AGENT_STRING);
	m_headers.AddDefault("Accept-Encoding", DEFLATE);
	m_headers.AddDefault("Accept", "*/*");
	m_headers.AddDefault("Content-Type", "text/xml");
	m_headers.AddDefault("Connection", "Close");
	m_headers.AddDefault("Pragma", "no-cache");
}

CHTTPCtrl::~CHTTPCtrl(void)
{
	//STLOG_WRITE("%s(%d): Ponto de apoio. Cleaning Memory...", __FUNCTION__, __LINE__);
	if(m_connection != NULL)
	{
		m_connection->Disconnect();
		delete m_connection;
		m_connection = NULL;
	}

	if(m_tfrDecoder != NULL)
	{
		delete m_tfrDecoder;
		m_tfrDecoder = NULL;
	}

	if(m_requestBody != NULL)
	{
		delete m_requestBody;
		m_requestBody = NULL;
	}
	m_headers.Clear();
}

CStringA CHTTPCtrl::BuildCurrentUrl(void)
{
	CStringA url;

	if(m_ssl)
		url.Format("https://%s:%d%s", m_server, m_port, m_uri);
	else
		url.Format("http://%s:%d%s", m_server, m_port, m_uri);

	return url;
}

BOOL CHTTPCtrl::InternalRead(LPVOID __pvx, DWORD __cb, DWORD *__pcbRead)
{
	LPSTR pv = (LPSTR)__pvx;
	*__pcbRead = 0;

	// empty out the responseBuffer first
	LONG cs = 0;
	if(m_responseBuffer.GetCount() > 0)
	{
		cs = min((DWORD)m_responseBuffer.GetCount(), __cb);
	
		for(INT_PTR i = 0; i < cs; i++)
		{
			BYTE ch = m_responseBuffer.GetAt(i);
			pv[i] = ch;
		}

		__cb -= cs;
		pv += cs;
		*__pcbRead += cs;

		for(INT_PTR i = 0; i < cs; i++)
			m_responseBuffer.RemoveAt(0);

		m_responseBuffer.FreeExtra();

		//TRACE(L"CHTTPCtrl::InternalRead (%ld)\n", m_responseBuffer.GetCount());
	}
	else
	{
		if(__cb)
		{
			//AC: make sure m_connection is not null before using it
			int retval = 0;
			if(m_connection != NULL) 		
				retval = m_connection->ReadChunk(pv, __cb);

			if(retval == SOCKET_ERROR)
			{
				DWORD wsErr = WSAGetLastError();
				if(WSAENOTSOCK != wsErr)
				{
					m_connection->Disconnect();
					if(WSAETIMEDOUT == wsErr)
						Log(_T("Timeout waiting for response"), wsErr);
					else
						Log(_T("Error while receiving data"), wsErr);

					return FALSE;
				}

				retval = 0;
			}

			*__pcbRead += retval;
		}
	}	

	if(!m_inHeaders)
	{
		if(m_connection && (*__pcbRead == 0))
		{
			if(m_closeWhenDone)
				m_connection->Disconnect();
			else
				thePool.ReturnToPool(m_connection);

			m_connection = 0;
		}
	}

	return TRUE;
}

BOOL CHTTPCtrl::Read(LPVOID __pvx, DWORD __cb, DWORD *__pcbRead)
{
	//STLOG_WRITE("Entrando em READ()");
	BOOL hr;
	if(m_tfrDecoder != NULL)
	{
		//STLOG_WRITE("Entrando em Read Decoder");
		hr = m_tfrDecoder->Read((BYTE *)__pvx, __cb, __pcbRead);
	}
	else
	{
		//STLOG_WRITE("Entrando em Internal Read");
		hr = InternalRead(__pvx, __cb, __pcbRead);
	}

	return hr;
}

void CHTTPCtrl::BufferForNextRead(BYTE *__pvx, DWORD __cb)
{
	for(DWORD i = 0; i < __cb; i++)
		m_responseBuffer.InsertAt(i, __pvx[i]);
}

// Send the request from m_requestBody
BOOL CHTTPCtrl::SendInit(CString endpoint)
{
	USES_CONVERSION;

	if(m_compression)
	{
		CStreamReader *comp = 0;

		// if we're going to compress the request, then we insert a compressing reader
		// into the source chain, and reset the request length
		if(m_compressionIsGzip)
			comp = new CGzipHandler(m_requestBody);
		else
			comp = new CDeflateHandler(m_requestBody);

		m_requestBody = comp;
		m_requestSize = REQUEST_SIZE_NOT_KNOWN;
	}

	// for now, if we need to calc the length, we just iterate over the 
	// stream to find the size, we can remove this step once we add 
	// support for sending a chunked encoded request
	if(REQUEST_SIZE_NOT_KNOWN == m_requestSize)
	{
		BYTE buff[PH_BUFFER_SIZE];
		DWORD cb=0, total=0;

		do
		{
			m_requestBody->Read(buff, sizeof(buff), &cb);
			total += cb;

		} while ( cb > 0 );

		m_requestSize = total;
		m_requestBody->Reset();
	}

	m_authDepth = 0;
	m_redirects	= 0;

	m_cookies.SetDirty(FALSE);

	LPCSTR szEndpoint = T2A(endpoint);
	CrackURL(szEndpoint);
	BuildHeaders(szEndpoint);

	if(m_tfrDecoder != NULL)
	{
		delete m_tfrDecoder;
		m_tfrDecoder = NULL;
	}

	return Send();
}

BOOL CHTTPCtrl::BuildHeaders(LPCSTR szEndpoint)
{
	// build the headers...
	m_szHeaders.Empty();
	m_szHeaders += m_method;
	m_szHeaders += " ";

	if((m_proxyPort != 0) && !m_ssl)
		m_szHeaders += szEndpoint;
	else
		m_szHeaders += m_uri;
	 
	m_szHeaders += " HTTP/1.1\r\nHost: ";
	m_szHeaders += m_server;

	char buff[10] ;
	if(((!m_ssl) && (m_port != DEF_HTTP_PORT)) || (m_ssl && (m_port != DEF_HTTPS_PORT)))
	{
		m_szHeaders += ":";
		_ltoa(m_port, buff, 10);
		m_szHeaders += buff;
	}

	m_szHeaders += "\r\n";

	m_szHeaders += m_headers.BuildHeaderA();

	// forcar o encerramento da conexao qdo terminado...
	if (m_sendConnectionCloseHeader)
		m_szHeaders += "Connection: close\r\n";

	// request eh comprimido
	if(m_compression)
	{
		m_szHeaders += "Content-Encoding: ";
		if(m_compressionIsGzip)
			m_szHeaders += "gzip";
		else
			m_szHeaders += "deflate";

		m_szHeaders += "\r\n";
	}

	// Somente enviar o CL se o tamanho for conhecido...
	if(m_requestSize != REQUEST_SIZE_NOT_KNOWN)
	{
		// Nao setar o CL se for GET, sem informação
		if(!(( m_requestSize == 0 ) && (CString(m_method).CompareNoCase(L"GET") == 0)))
		{
			CStringA s;
			s.Format("Content-Length: %ld\r\n", m_requestSize);
			m_szHeaders += s;
		}
	}

	m_cookies.AddCookieHeaders(m_server, m_uri, m_szHeaders);

	return TRUE;
}

BOOL CHTTPCtrl::Send()
{
	//STLOG_WRITE("%s(%d): Entrando", __FUNCTION__, __LINE__);
	// sometimes Send() gets called multiple times for a given 
	// request [redirects, authentication etc] so we check the 
	// cookiesDirty flag, and rebuild the headers if we need to
	if(m_cookies.IsDirty())
	{
		CStringA url = BuildCurrentUrl();
		BuildHeaders(url);
	}

	//STLOG_WRITE("%s(%d): Ponto 1", __FUNCTION__, __LINE__);

	sockaddr_in sa;
	
	ConnectionKey ck;
	if(!ConstructConnectionKey(ck))
		return FALSE;

	//STLOG_WRITE("%s(%d): Ponto 2", __FUNCTION__, __LINE__);

	const DWORD buff_size = PH_BUFFER_SIZE;

	// Tamanho dos headers...
	DWORD cbHeaders = m_szHeaders.GetLength() + m_authHeader.GetLength() + m_proxyAuthHeader.GetLength() + 2;
	ASSERT( cbHeaders <= PH_BUFFER_SIZE );

	if(cbHeaders > PH_BUFFER_SIZE)
	{
		Log(_T("Wow the request HTTP headers are way too big, how'd that happen?"));
		return FALSE;
	}

	//STLOG_WRITE("%s(%d): Ponto 3", __FUNCTION__, __LINE__);

	DWORD cb = 0;
	BYTE buff[PH_BUFFER_SIZE];
	LPBYTE pos = buff;

	memcpy(pos, m_szHeaders, m_szHeaders.GetLength());
	pos += m_szHeaders.GetLength();

	// Adicionar os headers de autenticacao, se necessario...
	if(m_authHeader.GetLength())
	{
		memcpy(pos, m_authHeader, m_authHeader.GetLength());
		pos += m_authHeader.GetLength() ;
	}

	//STLOG_WRITE("%s(%d): Ponto 4", __FUNCTION__, __LINE__);
	// Adicionar o header de proxy, se necessario...
	if(m_proxyAuthHeader.GetLength())
	{
		memcpy(pos, m_proxyAuthHeader, m_proxyAuthHeader.GetLength());
		pos += m_proxyAuthHeader.GetLength();
	}
	//STLOG_WRITE("%s(%d): Ponto 5", __FUNCTION__, __LINE__);

	memcpy(pos, "\r\n", 2);
	pos += 2;

	// preencher o resto do primeiro chunk com o inicio do corpo do request
	// Para debugar usando as funcoes abaixo, comente estas duas linhas para
	// que o pointer do buffer esteja no inicio do mesmo.
	if(m_requestBody != NULL)
		m_requestBody->Read(pos, buff_size - (pos-buff), &cb);

	//STLOG_WRITE("%s(%d): Ponto 6", __FUNCTION__, __LINE__);

	//////////////////////////////////////////////////////////////////////////
	// Para o debug usando as funcoes abaixo, note:
	// Somente use uma por vez, e comente as duas linhas anteriores
	// Coloque o breakpoint apos as funcoes de debug e pare o programa 
	// qdo terminar, nao prossiga pois os resultados sao imprevisiveis
	//Debug_ToFile();
	//Debug_UnzipInMemory();

	pos += cb;	
	BOOL usingPooledConn = TRUE;
	BOOL b = FALSE;
	do
	{
		// Criar o socket...
		if(m_connection != NULL)
			m_connection->Disconnect();

		if(!thePool.FindConnection(ck, &m_connection))
		{
			Log(_T("Error getting connection from pool"));
			return FALSE;
		}
Log(CString(m_server));
		usingPooledConn = TRUE;
		if(INVALID_SOCKET == m_connection->socket)
		{
			m_connection->socket = socket(AF_INET, SOCK_STREAM, 0);

			if(INVALID_SOCKET == m_connection->socket)
			{
				Log(_T("Failed creating socket"), WSAGetLastError());
				return FALSE;
			}

			if(!m_connection->PreConnect(m_server, m_timeout))
			{
				Log(_T("Error on pre connection fase"));
				return FALSE;
			}

			// conectar o socket...
			m_connection->PopulateSockAddr(sa);
			if(connect(m_connection->socket, (struct sockaddr*)&sa, sizeof(sa)) == SOCKET_ERROR) 
			{
				CString s = MakeFailedOpeningSocketError(WSAGetLastError(), sa);
				Log(s);
				return FALSE;
			}

			if(!m_connection->PostConnect(m_server, m_proxyAuthHeader))
			{
				Log(_T("Error on post connection fase"));
				return FALSE;
			}

			usingPooledConn = FALSE;
		}

		// Setar o timeout...
		m_connection->SetTimeout(m_timeout);

		// Enviar as informacoes atuais (primeiro chunk)...
		long size = pos - buff;
		b = m_connection->SendChunk(buff, size);

		if(!b && (!usingPooledConn))
			return FALSE;

	} while(!b);

	//STLOG_WRITE("%s(%d): Ponto de apoio. Buffer: %s", __FUNCTION__, __LINE__, buff);
	// Se houver mais dados para mandar...
	while(cb > 0) 
	{
		if(m_requestBody != NULL)
			m_requestBody->Read(buff, buff_size, &cb);
		
		if(cb > 0)
			m_connection->SendChunk(buff, cb);
	} 
	
	//STLOG_WRITE("%s(%d): Saindo", __FUNCTION__, __LINE__);
	return TRUE;
}

CStringW CHTTPCtrl::MakeFailedOpeningSocketError(DWORD __wsaErr, sockaddr_in &__sa)
{
	CStringW s;
	s.Format(L"Failed opening socket for address:%hs:%d (WSAErrorCode=%d)", 
			 inet_ntoa(__sa.sin_addr), 
			 ntohs(__sa.sin_port), 
			 __wsaErr);

	return s;
}

BOOL CHTTPCtrl::CrackURL(LPCSTR __url)
{
	if(strlen(__url) < strlen(PROTO_HTTP))
	{
		Log(_T("Invalid URL specified, expecting http:// or https://"));
		return FALSE;
	}

	m_ssl = FALSE;
	// http ?
	if(_strnicmp(__url, PROTO_HTTP, strlen(PROTO_HTTP)) != 0)
	{
		// https ?
		if(_strnicmp(__url, PROTO_HTTPS, strlen(PROTO_HTTPS)) == 0)
			m_ssl = TRUE;
		else
		{
			Log(_T("Invalid URL specified, expecting http:// or https://"));
			return FALSE;
		}
	}

	// Pular o protocolo...
	LPCSTR p = __url + strlen(PROTO_HTTP);
	if(m_ssl) 
	{
		// https ? soma um...
		++p ;
		m_port = DEF_HTTPS_PORT;
	}
	else
		m_port = DEF_HTTP_PORT;

	// Procurar o separador de path e porta...
	LPCSTR p2 = strchr(p, '/');
	LPCSTR p3 = strchr(p, ':');

	// raiz...
	if(!p2)
		p2 = p + strlen(p);

	LPCSTR pEndOfServer = p2;

	// pegar a porta...
	if(p3 && p2 && (p3 < p2))
	{
		m_port = atoi(p3+1);
		pEndOfServer = p3;
	}

	m_server.Empty();
	m_server = CStringA(p, pEndOfServer-p);

	m_uri.Empty();

	if(strlen(p2))
		m_uri += p2;
	else
		m_uri += "/";

	return TRUE;
}

BOOL CHTTPCtrl::Send(LPCTSTR __endpoint, CHttpResponse *__pResponse)
{
	if(m_requestBody != NULL)
	{
		delete m_requestBody;
		m_requestBody = NULL;
	}

	m_requestSize = 0;
	m_method = "GET";

	if(!SendInit(__endpoint))
		return FALSE;

	return Response(__pResponse);
}

BOOL CHTTPCtrl::Send(LPCTSTR __endpoint, LPCTSTR __srcPayload, CHttpResponse *__pResponse)
{
	//STLOG_WRITE("%s(%d): Entrando", __FUNCTION__, __LINE__);
	if(m_requestBody != NULL)
	{
		delete m_requestBody;
		m_requestBody = NULL;
	}

	m_requestBody = new CStringStreamReader();
	((CStringStreamReader *)m_requestBody)->SetData(__srcPayload);
	m_requestSize = ((CStringStreamReader *)m_requestBody)->GetSize();

	//STLOG_WRITE("%s(%d): Ponto 1", __FUNCTION__, __LINE__);

	m_method = "POST";
	GetHeaders()->Remove("Content-Type");
	GetHeaders()->AddDefault("Content-Type", "application/x-www-form-urlencoded");
	//STLOG_WRITE("%s(%d): Ponto 2", __FUNCTION__, __LINE__);

	if(!SendInit(__endpoint))
		return FALSE;

	//STLOG_WRITE("%s(%d): Ponto 3", __FUNCTION__, __LINE__);

	return Response(__pResponse);

	//STLOG_WRITE("%s(%d): Saindo", __FUNCTION__, __LINE__);
}

BOOL CHTTPCtrl::Send(LPCTSTR __endpoint, CArray<BYTE> __srcPayload, CHttpResponse *__pResponse)
{
	if(m_requestBody != NULL)
	{
		delete m_requestBody;
		m_requestBody = NULL;
	}
	
	m_requestBody = new CByteStreamReader();
	((CByteStreamReader *)m_requestBody)->SetData(__srcPayload);
	m_requestSize = ((CByteStreamReader *)m_requestBody)->GetSize();
	m_method = "POST";

	if(!SendInit(__endpoint))
		return FALSE;

	return Response(__pResponse);
}

BOOL CHTTPCtrl::SendForm(LPCTSTR __endpoint, CFormData *__form, CHttpResponse *__pResponse)
{
	m_method = "POST";

	GetHeaders()->Remove("Content-Type");
	GetHeaders()->AddDefault("Content-Type", __form->GetContentType());

	m_requestBody = new CFormStreamReader();
	//STLOG_WRITE("%s(%d): Ponto de apoio", __FUNCTION__, __LINE__);
	((CFormStreamReader *)m_requestBody)->ProcessForm(__form);

	m_requestSize = ((CFormStreamReader *)m_requestBody)->GetSize();
	//STLOG_WRITE("%s(%d): Ponto de apoio. Tamanho do buffer: %ld", __FUNCTION__, __LINE__, m_requestSize);

	if(!SendInit(__endpoint))
		return FALSE;

	return Response(__pResponse);
}

BOOL CHTTPCtrl::ConstructConnectionKey(ConnectionKey &key)
{
	//STLOG_WRITE("%s(%d): Entrando", __FUNCTION__, __LINE__);
	// ssl
	key.ssl = m_ssl;

	// auth info
	key.SetAuthInfo(m_userName, m_password, m_proxyUserName, m_proxyPassword);
	//STLOG_WRITE("%s(%d): Ponto 1", __FUNCTION__, __LINE__);

	key.port = htons(m_port);
	//STLOG_WRITE("%s(%d): Ponto 2", __FUNCTION__, __LINE__);

	// proxy info
	if(m_proxyServer.GetLength() > 0)
	{
		key.addr = 0;
		key.px_port = htons(m_proxyPort);
		if (key.ssl)
			key.SetTargetUrlHash(m_server);

		//STLOG_WRITE("%s(%d): Saida A", __FUNCTION__, __LINE__);
		return ResolveName(m_proxyServer, &key.px_addr);
	}

	//STLOG_WRITE("%s(%d): Ponto 3", __FUNCTION__, __LINE__);

	key.px_addr = 0;
	key.px_port = 0;

	//STLOG_WRITE("%s(%d): Saida B", __FUNCTION__, __LINE__);
	// destination info
	return ResolveName(m_server, &key.addr);
}

BOOL CHTTPCtrl::Response(CHttpResponse *__pResponse)
{
	USES_CONVERSION ;
	if ((!m_connection) || (INVALID_SOCKET == m_connection->socket))
	{
		Log(_T("Invalid connection, call Send first"));
		return FALSE;
	}

	m_responseBuffer.RemoveAll();
	int statusCode = 0;
	m_closeWhenDone = FALSE;
	int retval = 1;
	CHAR buff[PH_BUFFER_SIZE];
	DWORD cbRead = 0;
	LPSTR status = 0;
	m_inHeaders = TRUE;
	LPSTR endOfHeaders = NULL;
	CStringA results;
	BUFFER byteArray;

	do
	{
		// Ler todo o header...
#ifdef _WIN32_WCE
		while(NULL == (endOfHeaders = strstr(results, EOLEOL)))
#else
		while(NULL == (endOfHeaders = (LPSTR)strstr(results, EOLEOL)))
#endif
		{
			Read(buff, PH_BUFFER_SIZE, &cbRead);
			//STLOG_WRITE("Resposta completa do servidor: %s", buff);
			if(0 == cbRead) 
			{
				Log(_T("No response data available from the server"));
				return FALSE;
			}

			results += CStringA(buff, cbRead);

			for(DWORD i = 0; i < cbRead; i++)
				byteArray.Add((BYTE)buff[i]);
		}

		// Verificar o status HTTP...
		if(results.GetLength())
#ifdef _WIN32_WCE
			status = strchr(results, ' ');
#else
			status = (LPSTR)strchr(results, ' ');
#endif

		if(status)
			statusCode = atoi(++status);
		else
			statusCode = 500;

		////STLOG_WRITE("HTTP response: %d", statusCode);

		if(100 == statusCode)
		{
			// Saltar o '100 continue'
#ifdef _WIN32_WCE
			LPSTR nextHeader = strstr(results, EOLEOL);
#else
			LPSTR nextHeader = (LPSTR)strstr(results, EOLEOL);	
#endif
			if(nextHeader)
				results.Delete(0, nextHeader + 4 - results);
		}

	} while (statusCode == 100);

	// verificar se eh HTTP
	if(results.Mid(0,5).Compare("HTTP/") != 0)
	{
		Log(_T("Response was not HTTP"));
		return FALSE;
	}

	BOOL http10 = FALSE;
	if(results.Mid(5,3).Compare("1.0") == 0)
	{
		// em HTTP/1.0, nao temos keep-alives
		m_closeWhenDone = TRUE;
		http10 = TRUE ;
	}

	// Recuperar somente o header...
	int pos = results.Find(EOLEOL);

	// Fazer o parse...
	CHttpHeader headers;
	headers.Parse(results.Mid(0, pos));

	// Parser dos cookies...
	m_cookies.ExtractCookies(&headers, m_server, m_uri);

	// Verificar a resposta e tratar se for o caso...
	if(300 == statusCode || 301 == statusCode || 302 == statusCode || 307 == statusCode)
		return OnRedirect(__pResponse, headers);
	else if(401 == statusCode)
		return OnServerAuth(__pResponse, headers);
	else if(407 == statusCode)
		return OnProxyAuth(__pResponse, headers);

	// Processar o header connection...
	CStringA connection;
	if(headers.GetDefaultAttribute("connection", connection))
	{
		if(_stricmp(connection, "close") == 0)
			m_closeWhenDone = TRUE;
	}

	// Selecionar o transfer-encoding...
	CStringA transferEncoding;
	if(!headers.GetDefaultAttribute("transfer-encoding", transferEncoding))
	{
		CStringA contentLength;
		if(headers.GetDefaultAttribute("content-length", contentLength))
		{
			//TRACE(L"LEN: %S\n", contentLength);
			m_tfrDecoder = new CContentLengthTE(this, atol(contentLength));
		}
		else
		{
			if(m_closeWhenDone)
				m_tfrDecoder = new CConnectionWillClose(this);
			else
			{
				Log(_T("Content-Length must be specified when not using chunked-encoding with HTTP/1.1"));
				return FALSE;
			}
		}
	}
	else if(_stricmp(transferEncoding, "chunked") == 0)
		m_tfrDecoder = new CChunkedTE(this);
	else 
	{
		Log(_T("Unsupported Transfer-Encoding returned by server"));
		return FALSE;
	}

	// verificar o content-encoding, se for deflate ou gzip, vamos encadear a
	// descompactacao, isto eh o read vai para o zip e ele chama o read do 
	// socket, descompacta e assim por diante...
	CStringA contentEncoding;
	if(headers.GetDefaultAttribute("content-encoding", contentEncoding))
	{
		if(_stricmp(contentEncoding, "deflate") == 0)
		{
			// Cria um novo decoder, passando o atual...
			CTransferDecoder *inflater = new CInflateHandler(m_tfrDecoder);
			// substitui o decoder...
			m_tfrDecoder = inflater;
		}
		else if(_stricmp(contentEncoding, "gzip") == 0)
		{
			CTransferDecoder *inflater = new CUnGzipHandler(m_tfrDecoder);
			m_tfrDecoder = inflater;
		}
		else
		{
			Log(_T("Unsupported Content-Encoding returned by server"));
			return FALSE;
		}
	}
/*
	TRACE(_T("is HTTP/1.1 : %s close after this transaction %s\n"), 
		  http10 ? L"false" : L"true", 
		  m_closeWhenDone ? L"true" : L"false" );
*/
	// Adicionar os bytes apos o final do header no buffer de resposta...
	int pos1 = results.Find(EOLEOL);
	if(pos1 >= 0)
	{
		int j = 0;
		for(INT_PTR i = pos1+4; i < byteArray.GetCount(); i++)
			m_responseBuffer.InsertAt(j++, byteArray[i]);
	}

	m_inHeaders = FALSE;

	__pResponse->Init(this, &headers, statusCode);

	return TRUE;
}

BOOL CHTTPCtrl::OnRedirect(CHttpResponse *__pResponse, CHttpHeader &__headers)
{
	++m_redirects;

	if((!m_redirectOnPost) && (_stricmp(m_method, "POST") == 0))
	{
		Log(_T("Follow redirect on POST is disabled"));
		return FALSE;
	}

	if(m_redirects >= m_maxRedirects)
	{
		Log(_T("Reached maximum re-direct depth"));
		return FALSE;
	}

	// Determinar o destino...
	CStringA location;
	if(!__headers.GetDefaultAttribute("Location", location))
	{
		Log(_T("HTTP redirect received, but no location header found"));
		return FALSE;
	}

	// Atualizar o URI baseado na nova URI...
	ApplyURI(location);
	//TRACE(_T("Doing redirect : depth=%d newLoc=%s\n"), m_redirects, location);
	
	// Fazer o rebuild da URL...
	CStringA newURL = BuildCurrentUrl();
	//TRACE(_T("revised URL : %s\n"), newURL);

	// Rebuild dos headers...
	BuildHeaders(newURL);

	// re-enviar a requisicao...
	m_connection->Disconnect();
	m_connection = 0;

	m_requestBody->Reset();

	if(!Send())
		return FALSE;

	return Response(__pResponse);
}

void CHTTPCtrl::ApplyURI(CStringA &uri)
{
	if(_strnicmp(PROTO_HTTP, uri, strlen(PROTO_HTTP)) == 0  || 
	   _strnicmp(PROTO_HTTPS, uri, strlen(PROTO_HTTPS)) == 0 )
	{
		CrackURL(uri);
	}
	else if(*uri == '/')
	{
		m_uri = uri;
	}
	else
	{
		LPCSTR lastSlash = strrchr(m_uri, '/');
		CStringA newURI;

		if(lastSlash)
			newURI += CStringA(m_uri, lastSlash - m_uri+1);
		else
			newURI += "/";

		newURI += uri;

		m_uri = newURI;
	}
}

BOOL CHTTPCtrl::OnServerAuth(CHttpResponse *__pResponse, CHttpHeader &__headers)
{
	// find the www-authenticate header
	static const char WWW_AUTHENTICATE[] = "WWW-Authenticate"; 
	static const TCHAR WWW_BASIC[]		 = L"Basic";

	bool bCanAuth = false;

	CItemHeader *pItem = NULL;
	if((pItem = m_headers.Find(WWW_AUTHENTICATE)) != NULL)
	{
		POSITION p = pItem->m_attributes.GetStartPosition();
		while(p)
		{
			CStringW key;
			CAttribute *value;
			pItem->m_attributes.GetNextAssoc(p, key, value);
			if(key.Find(WWW_BASIC) >= 0)
			{
				bCanAuth = true;
				break ;
			}
		}
	}

	if((! bCanAuth) || m_authDepth > 1 || m_userName.GetLength() == 0)
	{
		Log(_T("Server Authentication failed"));
		return FALSE;
	}

	BuildWWAuthHeader();
	++m_authDepth;

	// re-send the request
	m_connection->Disconnect();
	m_connection = 0;
	m_requestBody->Reset();

	Send();

	return Response(__pResponse);
}

BOOL CHTTPCtrl::OnProxyAuth(CHttpResponse *__pResponse, CHttpHeader &__headers) 
{
	// proxy authentication needed 
	static const char PROXY_AUTHENTICATE[] = "Proxy-Authenticate";
	static const TCHAR PROXY_BASIC[]	   = L"Basic";
	
	bool bCanAuth = false;

	CItemHeader *pItem = NULL;
	if((pItem = m_headers.Find(PROXY_AUTHENTICATE)) != NULL)
	{
		POSITION p = pItem->m_attributes.GetStartPosition();
		while(p)
		{
			CStringW key;
			CAttribute *value;
			pItem->m_attributes.GetNextAssoc(p, key, value);
			if(key.Find(PROXY_BASIC) >= 0)
			{
				bCanAuth = true;
				break ;
			}
		}
	}

	if((!bCanAuth) || m_authDepth > 1 || m_proxyUserName.GetLength() == 0)
	{
		Log(_T("Proxy Authentication failed"));
		return FALSE;
	}

	BuildProxyAuthHeader();
	++m_authDepth ;

	// re-send the request
	m_connection->Disconnect();
	m_connection = 0;
	m_requestBody->Reset();

	Send();
	return Response(__pResponse);

	return FALSE;
}

void CHTTPCtrl::Debug_ToFile()
{
	DWORD cb = 0;
	CFile f;
	f.Open(L"\\buffer.gz", CFile::modeCreate|CFile::modeWrite, NULL);
	BYTE buffer1[8192];
	m_requestBody->Read(buffer1, 8192, &cb);
	f.Write(buffer1, cb);
	m_requestBody->Read(buffer1+cb, 8192-cb, &cb);
	f.Write(buffer1, cb);
	f.Close();
}

void CHTTPCtrl::Debug_UnzipInMemory()
{
	DWORD cb = 0;
	
	CGZippedDecoder td(m_requestBody);
	
	CTransferDecoder *gz = NULL;

	if(m_compressionIsGzip)
		gz = new CUnGzipHandler(&td);
	else
		gz = new CInflateHandler(&td);

	
	BYTE buffer1[8192];
	while(gz->Read(buffer1, 8192, &cb))	
	{
		TRACE(L"");
	}

	delete gz;
}

void CHTTPCtrl::BuildWWAuthHeader() 
{
	USES_CONVERSION ;
	return BuildAuthHeader(m_authHeader, 
						   "Authorization: Basic ", 
						   OLE2A(m_userName), 
						   OLE2A(m_password));
}

// this builds a basic authentication authorization header into the header string buffer
void CHTTPCtrl::BuildAuthHeader(CStringA &__header, const char *__name, const char *__userName, const char *__password)
{
	USES_CONVERSION;
	__header.Empty();
	__header += __name;

	CStringA credentials;
	credentials.Format("%s:%s", __userName, __password);

	DWORD cbEncCred = credentials.GetLength();
	cbEncCred  = cbEncCred * 4 / 3;
	cbEncCred += cbEncCred %4 > 0 ? 4 - cbEncCred %4 : 0;
	
	char * encCred = (char *)alloca(cbEncCred + sizeof(char));
	memset(encCred, 0, cbEncCred + sizeof(char));

	base64<char>::BufferEncode64(encCred, cbEncCred, (const BYTE *)(LPCSTR)credentials, credentials.GetLength());

	__header += encCred;
	__header += "\r\n";
}

void CHTTPCtrl::SetAuthInfo(LPCTSTR __szUser, LPCTSTR __szPass)
{
	m_userName = __szUser;
	m_password = __szPass;

	// Preparar o header de autorizacao...
	if(m_userName.GetLength() > 0)
		BuildWWAuthHeader();
	else
		m_authHeader.Empty(); // Limpar...
}

void CHTTPCtrl::BuildProxyAuthHeader() 
{
	USES_CONVERSION ;
	BuildAuthHeader(m_proxyAuthHeader, 
					"Proxy-Authorization: Basic ", 
					OLE2A(m_proxyUserName), 
					OLE2A(m_proxyPassword));
}

void CHTTPCtrl::ProxyAuthentication(LPCTSTR __szServer, u_short __port, LPCTSTR __username, LPCTSTR __password)
{
	m_proxyPort		= __port;
	m_proxyServer   = __szServer;
	m_proxyUserName = __username;
	m_proxyPassword = __password;

	if(m_proxyUserName.GetLength() > 0)
		BuildProxyAuthHeader();
	else
		m_proxyAuthHeader.Empty();
}

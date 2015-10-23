#include "StdAfx.h"
#include "SimpleRequest.h"
#include "Logger.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <sslsock.h>
#include "Utils2.h"
#include "Base64.h"

#define TRACING
///#define DBG_HTTP_LIB

const char BUFF[] = "POST %s HTTP/1.1\r\n"
					"Host: %s\r\n"
					"User-Agent: HTTPLib/1.3.2\r\n"
					"Connection: Close\r\n"
					"Accept: */*\r\n"
					"Accept-Encoding: deflate\r\n"
					"Content-Type: application/x-www-form-urlencoded\r\n"
					"Pragma: no-cache\r\n"
					"Content-Length: %d\r\n"
					"%s"
					"\r\n";

const char BUFF_MULT[] = "POST %s HTTP/1.1\r\n"
					"Host: %s\r\n"
					"User-Agent: HTTPLib/1.3.2\r\n"
					"Connection: Close\r\n"
					"Accept: */*\r\n"
					"Accept-Encoding: deflate\r\n"
					"Content-Type: multipart/form-data; boundary=BOUNDARY\r\n"
					"Pragma: no-cache\r\n"
					"Content-Length: %d\r\n"
					"%s"
					"\r\n";

const char BUFF_CONN[] = "CONNECT %s:443 HTTP/1.1\r\n"
						"User-Agent: HTTPLib/1.3.2\r\n"
						"Host: %s\r\n"
						"Proxy-Authorization: Basic %s\r\n"
						"\r\n";


const int TIMEOUT = 15000;
///const int TIMEOUT = 90000;

CSimpleRequest::CSimpleRequest(void)
{
	m_bFile = FALSE;
	m_https = FALSE;
}

CSimpleRequest::~CSimpleRequest(void)
{
	m_SendBuffer.RemoveAll();
	m_RecvBuffer.RemoveAll();
}

int CALLBACK CSimpleRequest::SSLValidateCertHook(DWORD  dwType, LPVOID pvArg, DWORD  dwChainLen, LPBLOB pCertChain, DWORD dwFlags)
{
   return SSL_ERR_OKAY;
}

BOOL CSimpleRequest::Request(LPCTSTR szURL, BOOL checkConn)
{
	CString sURL(szURL);

	if(sURL.Find(L"https://") > -1)
	{
		m_https = TRUE;
		sURL.Replace(L"https://", L"");
	}
	else
	{
		sURL.Replace(L"http://", L"");
	}

	m_sHost = CStringA(sURL.Left(sURL.Find(L"/")));

	sURL.Replace(CString(m_sHost), L"");

	m_sURI = CStringA(sURL);

	if(!checkConn)
	{
		if(!DoRequest())
			return FALSE;
		else
			return TRUE;
	}
	else
	{
		return _IsConnected();
	}

}

void CSimpleRequest::AddArguments(LPCTSTR szVar, LPCTSTR szValue)
{
	if(m_bFile)
	{
		m_sArgs.AppendFormat("--BOUNDARY\r\nContent-Disposition: form-data; name=\"%S\"\r\n\r\n%S\r\n", szVar, szValue);
	}
	else
	{
		m_sArgs.AppendFormat("%S=%S&", szVar, szValue);	
	}

	for(int i=0; i<m_sArgs.GetLength(); i++)
	{
		m_SendBuffer.Add(m_sArgs[i]);
	}

	m_sArgs.Empty();
}

void CSimpleRequest::AddArguments(LPCTSTR szVar, LPCTSTR szValue, BOOL bFile)
{
	m_bFile = TRUE;
	CStringA sMime;

	GetMime(GetFileExt(szValue), sMime);

	m_sArgs.AppendFormat("--BOUNDARY\r\n--BOUNDARY\r\nContent-Disposition: form-data; name=\"%S\"; filename=\"%s\"\r\nContent-Type: %s\r\n\r\n",
		szVar, GetFileName(szValue), sMime);

	for(int i=0; i<m_sArgs.GetLength(); i++)
	{
		m_SendBuffer.Add(m_sArgs[i]);
	}

	m_sArgs.Empty();

	GetFile(szValue);

	m_SendBuffer.Add('\r');
	m_SendBuffer.Add('\n');

}

void CSimpleRequest::SetProxy(LPCTSTR szProxy, int nPort, LPCTSTR szUser, LPCTSTR szPass)
{
	m_sProxyServer = CStringA(szProxy);
	m_nProxyPort = nPort;
	m_sProxyUser = CStringA(szUser);
	m_sProxyPass = CStringA(szPass);


	CStringA credentials;
	credentials.Format("%s:%s", m_sProxyUser, m_sProxyPass);

	DWORD cbEncCred = credentials.GetLength();
	cbEncCred  = cbEncCred * 4 / 3;
	cbEncCred += cbEncCred %4 > 0 ? 4 - cbEncCred %4 : 0;
	
	char * encCred = (char *)alloca(cbEncCred + sizeof(char));
	memset(encCred, 0, cbEncCred + sizeof(char));

	base64<char>::BufferEncode64(encCred, cbEncCred, (const BYTE *)(LPCSTR)credentials, credentials.GetLength());
	m_sProxyAuthDig = encCred;
	
}

void CSimpleRequest::ResetArguments(void)
{
	m_bFile = FALSE;
	m_sArgs.Empty();
	m_SendBuffer.RemoveAll();
}

void CSimpleRequest::GetFile(LPCTSTR szValue)
{
	CFile f;
	if(f.Open(szValue, CFile::modeRead, NULL))
	{
		BYTE buff[4096];
		UINT readed = 0;
		ULONG total = 0;

		do
		{
			ZeroMemory(&buff, sizeof(buff));
			readed = f.Read(&buff, sizeof(buff));
#define CRIPTOGRAFA_XML
#ifdef CRIPTOGRAFA_XML
			if(CString(szValue).Find(L".xml") > -1)
			{
				if (readed > 0)
				{
					char password[100]= "";

					BYTE bTemp[4096];
					ZeroMemory(&bTemp, sizeof(bTemp));
					memcpy(&bTemp, &buff, readed);
					ZeroMemory(&buff, sizeof(buff));
					int ret = CUtils2::CriptoBuffer((char *)&bTemp, (char *)&buff, &password[0], readed, TRUE); //decrypt
					if (!(ret > 0))
					{
						STLOG_WRITE(L"%S(%d): Falha ao descriptografar xml", __FUNCTION__, __LINE__ );
					}
					readed = ret;
				}
			}
#endif
			for(UINT i = 0; i < readed; i++)
				m_SendBuffer.Add(buff[i]);
			
			total += readed;

		} while(readed != 0);

		f.Close();
	}
}

void CSimpleRequest::GetMime(LPCSTR szValue, CStringA& sMime)
{
	HKEY   hkey;
	DWORD dwType, dwSize;
	wchar_t buffer[255];

	ZeroMemory(&buffer, sizeof(buffer));

	CStringW s;
	s.Format(L"\\.%S", szValue);

	if(RegOpenKeyEx(HKEY_CLASSES_ROOT, s, 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS)
	{
		dwType = REG_SZ;
		dwSize = sizeof(buffer);
		RegQueryValueEx(hkey, TEXT("Content Type"), 0, &dwType, (LPBYTE)&buffer, &dwSize);
        
		RegCloseKey(hkey);
	}

	sMime = CStringA(buffer);
}

CStringA CSimpleRequest::GetFileName(LPCTSTR szValue)
{
	CStringA sFileName(szValue);
	if(sFileName.Find('\\') >= 0 || sFileName.Find('/') >= 0)
	{
		int pos = sFileName.ReverseFind('\\');
		if(pos < 0)
			pos = sFileName.ReverseFind('/');

		sFileName = sFileName.Mid(pos+1);
	}

	return sFileName;
}

CStringA CSimpleRequest::GetFileExt(LPCTSTR szValue)
{
	CStringA s1(szValue);
	int pos = 0;
	if((pos = s1.ReverseFind('.')) > 0)
		s1 = s1.Right(s1.GetLength()-pos-1);
	
	return s1;
}

BOOL CSimpleRequest::_IsConnected()
{
	struct hostent *hp;
	unsigned long addr;

	if(inet_addr(m_sHost)==INADDR_NONE)
	{
		hp=gethostbyname(m_sHost);
	}
	else
	{
		addr=inet_addr(m_sHost);
		hp=gethostbyaddr((char*)&addr,sizeof(addr),AF_INET);
	}

	if(hp==NULL)
	{
		STLOG_WRITE("%s(%d): Erro gethostbyname: %d", __FUNCTION__, __LINE__, WSAGetLastError());
		return FALSE;
	}
	else
		return TRUE;

	/*struct addrinfo hints, *result = NULL;
	
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	
	int iResult = getaddrinfo(m_sHost, "80", &hints, &result);

	STLOG_WRITE("%s(%d): Resultado: %d", __FUNCTION__, __LINE__, iResult);

	if(iResult != 0)
		return FALSE;
	else
		return TRUE;*/

	/*struct hostent *hp;
	hp=gethostbyname(m_sHost);

	if(hp==NULL)
		return FALSE;
	else
		return TRUE;*/
}

bool IsAllHex (CStringA sText)
{
	char copy_of_param [64];

	return (
		strtok (
		strcpy (copy_of_param, sText),
		"0123456789ABCDEFabcdef") == NULL);
}

BOOL CSimpleRequest::DoRequest(void)
{
	//char buff[1024 * 8];
	char buffTmp[1024 * 2];


	if(m_SendBuffer.GetSize() > 0)
	{
		if(m_bFile)
		{
			m_sArgs.Append("--BOUNDARY\r\n--BOUNDARY--");

			for(int i=0; i<m_sArgs.GetLength(); i++)
			{
				m_SendBuffer.Add(m_sArgs[i]);
			}

			m_sArgs.Empty();
		}
		else
		{
			if(m_SendBuffer.GetAt(m_SendBuffer.GetCount()-1) == '&')
			{
				m_SendBuffer.RemoveAt(m_SendBuffer.GetCount()-1);
			}
		}
	}

	if(m_sProxyServer.IsEmpty())
		sprintf(buffTmp, (m_bFile ? BUFF_MULT : BUFF), m_sURI, m_sHost, m_SendBuffer.GetCount(), "");
	else if(m_https)
	{
		CStringA proxy;
		proxy.Format("Proxy-Authorization: Basic %s\r\n", m_sProxyAuthDig);

		sprintf(buffTmp, (m_bFile ? BUFF_MULT : BUFF), m_sURI, m_sHost, m_SendBuffer.GetCount(), proxy);
	}
	else
	{
		CStringA proxy, URIfmt;
		
		URIfmt.Format("http://%s%s", m_sHost, m_sURI);
		proxy.Format("Proxy-Authorization: Basic %s\r\n", m_sProxyAuthDig);

		sprintf(buffTmp, (m_bFile ? BUFF_MULT : BUFF), URIfmt, m_sHost, m_SendBuffer.GetCount(), proxy);
	}
		

	int lb = strlen(buffTmp);
	int lbs = m_SendBuffer.GetSize();
	m_iBufLen = lb + lbs;

	char *buff = (char *)malloc(m_iBufLen + 8);
	memset(buff,0,m_iBufLen + 8);
	strcpy (buff, buffTmp);

	///strncat(buff, (char*)m_SendBuffer.GetData(), m_SendBuffer.GetSize());
	char * pPt = (char*)m_SendBuffer.GetData();
	for (int i = lb; i < m_iBufLen; i++, pPt++)
	{
		buff[i] = *pPt;
	}


	char bf1[512];
#ifdef TRACING	
	STLOG_WRITE("%s(%d): [%s]", __FUNCTION__, __LINE__, buff);
#endif
	CStringA s;
	WSADATA wsaData;
	struct hostent *hp;
	unsigned int addr;
	struct sockaddr_in server;
	CStringA servername(m_sHost); 
	CStringA filepath;
	CStringA filename;	
	DWORD dwErr;
//ParseURL(m_url,servername,filepath,filename);
////	int wsaret=WSAStartup(0x101,&wsaData);
////	if(wsaret)	
////		return FALSE;
////#ifdef TRACING
////	s.Format("Initialized WinSock");
////	//m_list.AddString(s);
////	STLOG_WRITE("%s(%d): [%s]", __FUNCTION__, __LINE__, s);
////#endif
	
	SOCKET conn;
	conn=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(conn==INVALID_SOCKET)
	{
		STLOG_WRITE("%s(%d): Erro criando socket: %d", __FUNCTION__, __LINE__, WSAGetLastError());
		return FALSE;
	}
#ifdef TRACING
	s.Format("SOCKET created");
	//m_list.AddString(s);
	STLOG_WRITE("%s(%d): [%s]", __FUNCTION__, __LINE__, s);
#endif

	if(!m_sProxyServer.IsEmpty())
		servername = m_sProxyServer;

	addr = inet_addr(servername);
	hp=gethostbyname(servername);
	
	/*if(inet_addr(servername)==INADDR_NONE)
	{
		hp=gethostbyname(servername);
	}
	else
	{
		addr=inet_addr(servername);
		hp=gethostbyaddr((char*)&addr,sizeof(addr),AF_INET);
	}*/
	if(hp==NULL)
	{
		STLOG_WRITE("%s(%d): Erro gethostbyname: %d", __FUNCTION__, __LINE__, WSAGetLastError());
		closesocket(conn);
		return FALSE;
	}
#ifdef TRACING
	s.Format("hostname/ipaddress resolved");
	//m_list.AddString(s);
	STLOG_WRITE("%s(%d): [%s]", __FUNCTION__, __LINE__, s);
#endif

	server.sin_addr.s_addr=*((unsigned long*)hp->h_addr);
	server.sin_family=AF_INET;

	if(!m_sProxyServer.IsEmpty())
		server.sin_port=htons(m_nProxyPort);
	else
		server.sin_port=htons((m_https ? 443: 80));

	if(m_https)
	{
		SSLVALIDATECERTHOOK hook;
		DWORD dwVal = SO_SEC_SSL;
		DWORD dwFlags;

		// set the socket to secure mode
		if(setsockopt(conn, SOL_SOCKET, SO_SECURE, (LPSTR)&dwVal, sizeof(DWORD)) != 0) 
		{
#ifdef TRACING
	s.Format("setsockopt Ret. FALSE!");
	STLOG_WRITE("%s(%d): [%s]", __FUNCTION__, __LINE__, s);
#endif
			closesocket(conn);
			return FALSE;
		}

		hook.HookFunc = SSLValidateCertHook;
		hook.pvArg = (PVOID) conn;

		// set the certificate validation callback.
		if(WSAIoctl(conn, SO_SSL_SET_VALIDATE_CERT_HOOK, &hook, sizeof(SSLVALIDATECERTHOOK), NULL, 0, NULL, NULL, NULL) == SOCKET_ERROR) 
		{
#ifdef TRACING
	s.Format("WSAIoctl 1 Ret. FALSE!");
	STLOG_WRITE("%s(%d): [%s]", __FUNCTION__, __LINE__, s);
#endif
			closesocket(conn);
			return FALSE;
		}

		// select deferred handshake mode. Security protocols will not be negotiated until the SO_SSL_PERFORM_HANDSHAKE ioctl is issued
		dwFlags = SSL_FLAG_DEFER_HANDSHAKE;
		if(WSAIoctl(conn, SO_SSL_SET_FLAGS, &dwFlags, sizeof(DWORD), NULL, 0, NULL, NULL, NULL) == SOCKET_ERROR) 
		{
#ifdef TRACING
	s.Format("WSAIoctl 2 Ret. FALSE!");
	STLOG_WRITE("%s(%d): [%s]", __FUNCTION__, __LINE__, s);
#endif
			closesocket(conn);
			return FALSE;
		}
	}

	////Connect time-out

	BOOL bConn = FALSE;

	TIMEVAL Timeout;
	Timeout.tv_sec = TIMEOUT;
	Timeout.tv_usec = 0;

	unsigned long iMode = 1;
	int iResult = ioctlsocket(conn, FIONBIO, &iMode);
	if(iResult != NO_ERROR)
	{
		//Error
	}
	
	if(connect(conn,(struct sockaddr*)&server,sizeof(server)))
	{
		//Error
	}

	iMode = 0;
	iResult = ioctlsocket(conn, FIONBIO, &iMode);
	if(iResult != NO_ERROR)
	{
		//Error
	}

	fd_set Write, Err;
	FD_ZERO(&Write);
	FD_ZERO(&Err);
	FD_SET(conn, &Write);
	FD_SET(conn, &Err);

	select(0, NULL, &Write, &Err, &Timeout);
	if(FD_ISSET(conn, &Write))
	{
		bConn = TRUE;
	}
	

	///////
	if(!bConn)
	{
#ifdef TRACING
		s.Format("connect  Ret. ERRO: %d", WSAGetLastError());
	STLOG_WRITE("%s(%d): [%s]", __FUNCTION__, __LINE__, s);
#endif
		closesocket(conn);
		return FALSE;	
	}

	if(!m_sProxyServer.IsEmpty() && m_https)
	{
		CStringA bufConn;
		bufConn.Format(BUFF_CONN, m_sHost, m_sHost, m_sProxyAuthDig);

		int lb = bufConn.GetLength();

		char *buffconn = (char *)malloc(lb + 8);
		memset(buffconn,0,lb + 8);
		strcpy (buffconn, bufConn);

		send(conn, buffconn, lb, 0);
#ifdef TRACING
		s.Format("Buffer do proxy: %s", buffconn);
		STLOG_WRITE("%s(%d): [%s]", __FUNCTION__, __LINE__, s);
#endif

		free(buffconn);

		int lenrcv = 0;
		
		lenrcv = recv(conn, bf1, 512, 0);

		CStringA res(bf1);
#ifdef TRACING
		s.Format("Recv buffer do proxy: %s", res);
		STLOG_WRITE("%s(%d): [%s]", __FUNCTION__, __LINE__, s);
#endif

		if(res.Find("200 Connection established") == -1 && res.Find("200 Connected") == -1)
		{
			closesocket(conn);
			return FALSE;
		}
		//WSAIoctl(conn, SO_SSL_PERFORM_HANDSHAKE, (void *)(servername), strlen(servername)+1, NULL, 0, NULL, NULL, NULL)
	}
	///////////

	if(m_https)
	{
		// Negotiate security protocols
		if(WSAIoctl(conn, SO_SSL_PERFORM_HANDSHAKE, NULL, 0, NULL, 0, NULL, NULL, NULL) == SOCKET_ERROR) 
		{
			dwErr = WSAGetLastError();
			closesocket(conn);
#ifdef TRACING
	s.Format("WSAIoctl 3 Ret. FALSE!");
	STLOG_WRITE("%s(%d): [%s]", __FUNCTION__, __LINE__, s);
#endif

			return FALSE;
		}
	}
#ifdef TRACING
	s.Format("Connected to server :- %s",servername);
	//m_list.AddString(s);
	STLOG_WRITE("%s(%d): [%s]", __FUNCTION__, __LINE__, s);
#endif
	
	//sprintf(buff,"GET %s\r\n\r\n",filepath);
	////////////////////////Select para o send

	fd_set writefds;
	FD_ZERO(&writefds);
	FD_SET(conn, &writefds);
	timeval tv;
	tv.tv_sec = TIMEOUT / 1000;
	tv.tv_usec = (TIMEOUT % 1000) * 1000;
	long __timeout = TIMEOUT;

	int rc = select(0, NULL, &writefds, NULL, &tv);

	if(SOCKET_ERROR == rc)
	{
#ifdef TRACING
	s.Format("select Ret. ERRO!");
	STLOG_WRITE("%s(%d): [%s]", __FUNCTION__, __LINE__, s);
#endif
		closesocket(conn);
		return FALSE;
	}

	setsockopt(conn,IPPROTO_TCP,SO_RCVTIMEO,(char *)&__timeout,sizeof(__timeout));
	setsockopt(conn,IPPROTO_TCP,SO_SNDTIMEO,(char *)&__timeout,sizeof(__timeout));

	m_size = strlen(buff);
	m_size = strlen(buff) - m_SendBuffer.GetSize();

	///////////////////////

	
#ifdef DBG_HTTP_LIB
	{
		DWORD cb = 0;
		FILE* f;
		f = fopen("\\rsend.txt", "a+");
		if(f)
		{
			fputs("\r\n=================send=========================\r\n", f);
			fputs(buff, f);
			fputs("\r\n=================send=========================\r\n", f);
			fclose(f);
		}
	}
#endif

#ifdef TRACING
	STLOG_WRITE("%s(%d): Quantidade a ser enviada para %s: %d", __FUNCTION__, __LINE__, m_sURI, strlen(buff));
#endif
	///////////
	///send(conn,buff,strlen(buff),0);
	send(conn,buff,m_iBufLen,0);
#ifdef TRACING
	s.Format("sending Buffer: %s to server: %s", buff, filepath);
	STLOG_WRITE("%s(%d): [%s]", __FUNCTION__, __LINE__, s);
#endif
	free(buff);


	int y;

	/*CFile f;
	int y;
	CString fname="c:\\";
	fname+=filename;
	f.Open(fname,CFile::modeCreate | CFile::modeWrite);
	s.Format("starting to receive file");
	*///m_list.AddString(s);


	///////////////////////Select para o recv
	fd_set readfds[1];
	FD_ZERO(readfds);
	FD_SET (conn, readfds);
	timeval timeout;
	timeout.tv_sec = TIMEOUT / 1000 ;
	timeout.tv_usec = ( TIMEOUT % 1000 ) * 1000 ;

	rc = select(0, readfds, NULL, NULL, &timeout);
	if(0 == rc || (!FD_ISSET( conn, readfds)))
	{
		// timeout
		WSASetLastError(WSAETIMEDOUT);
#ifdef TRACING
	s.Format("select Ret. ERRO TIMEOUT!");
	STLOG_WRITE("%s(%d): [%s]", __FUNCTION__, __LINE__, s);
#endif
		closesocket(conn);
		return FALSE;
	}
	
	if( SOCKET_ERROR == rc)
	{
#ifdef TRACING
	s.Format("select Ret. ERRO SOCKET_ERROR!");
	STLOG_WRITE("%s(%d): [%s]", __FUNCTION__, __LINE__, s);
#endif
		closesocket(conn);
		return FALSE;
	}

	/////////////////////////////
	dwErr=0;

	CStringA res;
	do
	{
		y = recv(conn, bf1, 512, 0);
		dwErr = WSAGetLastError();
#ifdef TRACING
		STLOG_WRITE("%s(%d): WSAGetLastError recv: %d size: %d", __FUNCTION__, __LINE__, dwErr, y);
#endif
		if ( y > 0 )
		{
			for(int i=0; i<y; i++)
				m_RecvBuffer.Add(bf1[i]);
			//CStringA s(bf1, y); 
			//res += s;
		}

	} while (( y > 0)/*  && (strstr(res, "\r\n\r\n") == 0 )*/);

	int size = m_RecvBuffer.GetSize();

#ifdef TRACING
	STLOG_WRITE("%s(%d): RecvBuff size: %d", __FUNCTION__, __LINE__, size);
#endif
	///////////
#ifdef DBG_HTTP_LIB
	{
		DWORD cb = 0;
		FILE* f;
		f = fopen("\\rsend.txt", "a+");
		if(f)
		{
			fputs("\r\n==================receive========================\r\n", f);
			fputs("URI:",f);
			fputs(m_sURI,f);
			fputs("\r\n",f);
			fputs("Args:",f);
			fputs(m_sArgs.Left(256), f);
			fputs("\r\n",f);
			for(int i=0; i<m_RecvBuffer.GetCount(); i++)
			{
				fputc(m_RecvBuffer.GetAt(i), f);
			}
			fputs("\r\n==================receive========================\r\n", f);
			fclose(f);
		}
	}
#endif
#ifdef TRACING
	STLOG_WRITE("%s(%d): Quantidade a ser recebida para %s: %d", __FUNCTION__, __LINE__, m_sURI, m_RecvBuffer.GetSize());
#endif
	///////////

	res = CStringA(m_RecvBuffer.GetData()).Left(size);

	m_RecvBuffer.RemoveAll();

#ifdef DBG_HTTP_LIB
	STLOG_WRITE("%s(%d):Receive: [%s]", __FUNCTION__, __LINE__, res);
#endif

	/// Processa retorno
	int pos = res.Find("\r\n\r\n");
	CStringA out = res.Right(res.GetLength() - pos);

	
	char *end = "";
	char *sep = "\r\n";
	CStringA sFinal;
	m_sResp = "";

	if(res.Find("Transfer-Encoding: chunked") > -1)
	{
		int pos1;
		int size;
		int curPos=0;
		CStringA resToken= out.Tokenize(sep,curPos);
		while (resToken != "")
		{		
			if(IsAllHex(resToken))
			{
				size = strtoul(resToken, &end, 16);
				m_sResp += out.Mid(curPos+1, size);
				curPos += 2 + size;
			}
			resToken= out.Tokenize(sep,curPos);
		}
	}
	else
	{
		int a = res.Find("Content-Length: ");
		if(a != -1)
		{
			CStringA sResp;
			sResp = res.Mid(a, res.Find("\r\n", a)-a);
			sResp.Replace("Content-Length: ", "");
			m_sResp = res.Mid(pos+4, atoi(sResp));
			//out = buff.Right(atoi(sResp));
			//dwRemain = strtoul(out, &end, 10);
		}
	}


	//////

	
	

	/*CHttpResponse* m_Resp = new CHttpResponse();
	m_Resp->Init();
	m_Resp->GetString();*/

	//m_sResp = m_sResp.Right(m_sResp.GetLength() - m_sResp.Find("\r\n", 4) - 2);
	//m_sResp = m_sResp.Left(m_sResp.GetLength()-5);

	//f.Close();
	//s.Format("File downloaded and saved :- %s",fname);
	//m_list.AddString(s);
	closesocket(conn);
#ifdef TRACING
	s.Format("SOCKET closed");
	//m_list.AddString(s);
	STLOG_WRITE("%s(%d): [%s]", __FUNCTION__, __LINE__, s);
#endif

	////WSACleanup();
#ifdef TRACING
	s.Format("De-Initialized WinSock");
	//m_list.AddString(s);
	STLOG_WRITE("%s(%d): [%s]", __FUNCTION__, __LINE__, s);
#endif
	
	return TRUE;
}

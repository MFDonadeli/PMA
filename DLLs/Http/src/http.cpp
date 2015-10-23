#include "stdafx.h"
#include "Http.h"
#include "HTTPCtrl.h"
#include "HttpResponse.h"

#ifdef _WIN32_WCE
	LPCTSTR cszLogFileName = _T("\\temp\\http.log");
#else
	LPCTSTR cszLogFileName = _T("c:\\temp\\http.log");
#endif

CHttp::CHttp()
	: m_pResp(0)
{
	/*m_pCtrl = new CHTTPCtrl();*/
}

CHttp::~CHttp()
{
	ResetArguments();

	if(m_pCtrl != NULL)
	{
		delete m_pCtrl;
		m_pCtrl = NULL;
	}

	if(m_pResp != NULL)
	{
		delete m_pResp;
		m_pResp = NULL;
	}

	m_map.RemoveAll();
}

void CHttp::SetProxy(LPCTSTR proxy, const int port, LPCTSTR user, LPCTSTR pwd)
{
	m_sProxyServer = proxy;
	m_nPort = port;
	m_sProxyUser = user;
	m_sProxyPassword = pwd;
	//m_pCtrl->ProxyAuthentication(proxy, (u_short)port, user, pwd);
}

void CHttp::AddArguments(const CString & name, const int value)
{
	CString str;
	str.Format( L"%i", value );
	AddArguments( name, str );
}
void CHttp::AddArguments(const CString & name, const CString & value)
{
	ParamInfo *pi = new ParamInfo();
	pi->_value = value;
	pi->_label = name;
	pi->_file = false;

	m_map.SetAt(name, pi);
}

void CHttp::AddArguments(const CString & name, const CString & value, ContentType type)
{
	ParamInfo *pi = new ParamInfo();
	pi->_value = value;
	pi->_label = name;
	pi->_file = true;
	pi->_type = type;

	m_map.SetAt(name, pi);
}

void CHttp::ResetArguments()
{
	POSITION p = m_map.GetStartPosition();
	while(p)
	{
		CString key;
		ParamInfo *info;
		m_map.GetNextAssoc(p, key, info);
		delete info;
	}

	m_map.RemoveAll();
}

int GetAvailableMemory()
{
	MEMORYSTATUS ms;
	GlobalMemoryStatus(&ms);
	
	//return dwMemSize;
	//return ms.dwAvailPhys/1024;
	return ms.dwAvailVirtual/1024;
}

bool CHttp::Request(LPCTSTR url, const int method/*=RequestPostMethod*/, LPCTSTR agent/*=L"POCKET PC 0.1"*/)
{ 
	//STLOG_WRITE("%s(%d): Memoria Disponivel: [%d]", __FUNCTION__, __LINE__, GetAvailableMemory());
	bool bRet = false;
	//STLOG_WRITE("%s(%d): Locking...: %S", __FUNCTION__, __LINE__, url);
	//m_lock.Lock();
	m_pCtrl = new CHTTPCtrl();
	SystemIdleTimerReset();
	//STLOG_WRITE("%s(%d): Locked...: %S", __FUNCTION__, __LINE__, url);
	m_pCtrl->SetCompression(FALSE);


	if(!m_sProxyServer.IsEmpty())
	{
		m_pCtrl->ProxyAuthentication(m_sProxyServer, m_nPort, m_sProxyUser, m_sProxyPassword);
	}

	POSITION p = m_map.GetStartPosition();
	CString key;
	CString sQuery;
	ParamInfo *value = 0;
	ParamInfo file_value;

	//STLOG_WRITE("%s(%d): Ponto de apoio. URL [%S]. No. Parametros: %d", __FUNCTION__, __LINE__, url, m_map.GetCount());

	while(p)
	{
		m_map.GetNextAssoc(p, key, value);
		if(!value->_file)
		{			
			sQuery += key;
			sQuery += _T("=");
			sQuery += value->_value;
			sQuery += _T("&");
		}
		else
		{		
			file_value = *value;
		}
	}

	//STLOG_WRITE("%s(%d): Memoria Disponivel: [%d]", __FUNCTION__, __LINE__, GetAvailableMemory());

	if(!sQuery.IsEmpty() && sQuery.GetAt(sQuery.GetLength()-1) == '&')
		sQuery = sQuery.Mid(0, sQuery.GetLength()-1);

	if(m_pResp != NULL)
	{
		delete m_pResp;
		m_pResp = NULL;
	}

	m_pResp = new CHttpResponse();

	//Log(url);
	//Log(sQuery);

	//STLOG_WRITE("%s(%d): Ponto de apoio. URL [%S]. File: [%S]", __FUNCTION__, __LINE__, url, file_value._value);
	//Envio de Arquivo
	if(file_value._file)
	{	
		CFormData form;
		p = m_map.GetStartPosition();
		while(p)
		{
			m_map.GetNextAssoc(p, key, value);			
			if(!value->_file)
				form.AddItem(key, value->_value);			
		}

		try
		{
			if(wcslen(file_value._label) == 0 || wcslen(file_value._value) == 0)
			{
				//STLOG_WRITE("%s(%d): UnLocking...", __FUNCTION__, __LINE__);
				//STLOG_WRITE("%s(%d): UnLocked...", __FUNCTION__, __LINE__);
				bRet = false;
				goto end_request;
			}

			if(file_value._label.GetLength() == 0 || file_value._value.GetLength() == 0)
			{
				//STLOG_WRITE("%s(%d): UnLocking...", __FUNCTION__, __LINE__);
				//STLOG_WRITE("%s(%d): UnLocked...", __FUNCTION__, __LINE__);
				bRet = false;
				goto end_request;
			}
		}
		catch( int ret)
		{	
			//STLOG_WRITE("%s(%d): UnLocking...", __FUNCTION__, __LINE__);
			//STLOG_WRITE("%s(%d): UnLocked...", __FUNCTION__, __LINE__);
			bRet = false;
			goto end_request;
		}

		form.AddFile(file_value._label, file_value._value);

		//STLOG_WRITE("%s(%d): Sending...", __FUNCTION__, __LINE__);
		if(m_pCtrl->SendForm(url, &form, m_pResp))
		{
			m_strResp = m_pResp->GetString();
			//STLOG_WRITE("%s(%d): UnLocking...", __FUNCTION__, __LINE__);
			//STLOG_WRITE("%s(%d): UnLocked...", __FUNCTION__, __LINE__);
			bRet = true;
			goto end_request;
		}
	}
	else //Envio Normal
	{		
		//STLOG_WRITE("%s(%d): Sending...", __FUNCTION__, __LINE__);
		if(m_pCtrl->Send(url, sQuery, m_pResp))
		{
			m_strResp = m_pResp->GetString();
			//STLOG_WRITE("%s(%d): UnLocking...", __FUNCTION__, __LINE__);
			//STLOG_WRITE("%s(%d): UnLocked...", __FUNCTION__, __LINE__);
			bRet = true;
			goto end_request;
		}
	}

end_request:
	//STLOG_WRITE("%s(%d): UnLocking...", __FUNCTION__, __LINE__);
	delete m_pCtrl;
	//m_lock.Unlock();
	//STLOG_WRITE("%s(%d): UnLocked...", __FUNCTION__, __LINE__);
	delete m_pResp;
	m_pResp = NULL;
	ResetArguments();
	//STLOG_WRITE("%s(%d): Memoria Disponivel: [%d]", __FUNCTION__, __LINE__, GetAvailableMemory());
	return bRet; 
}

CString CHttp::Response(void) 
{ 
	//m_strResp = m_pResp->GetString();
	//STLOG_WRITE("%s(%d): Resposta: %S", __FUNCTION__, __LINE__, m_strResp);
	return m_strResp;
}

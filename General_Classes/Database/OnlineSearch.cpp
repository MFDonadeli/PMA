#include "StdAfx.h"
#include "OnlineSearch.h"
#include "Utils.h"

COnlineSearch::COnlineSearch(void)
{
}

COnlineSearch::~COnlineSearch(void)
{
}

DWORD WINAPI COnlineSearch::_OnLineSearchThrd( LPVOID lpParam )
{
	COnlineSearch *pOnline = (COnlineSearch*)lpParam;
	pOnline->DoThread();

	return 0;
}

void COnlineSearch::DoThread()
{
	m_sRet = _DoSearchOnline(m_sQuery);
}

CString COnlineSearch::DoSearch(LPCTSTR sQuery, const CString sURL)
{
	//CString sQuery;
	m_sURL = sURL;
	CString sTalonario = CUtil::GetIDTalao();
	CString sContrato = L"demo";
	//CString sFmt = L"5|arquivo=veiculos.egb|placa=%s|contrato=%s|cd_equipamento=%s|agente=%s|";
	//sQuery.Format(sFmt, szText, sContrato, sTalonario, ((CBaseDlg *)m_pParent)->GetParamValue(L"codigo"));

	HANDLE hThrd = NULL;
	DWORD dwThrd;

	m_sQuery = sQuery;

	m_pHttp = new CHttp();

	//hThrd = CreateThread(NULL, 0, _OnLineSearchThrd, this, 0, &dwThrd);

	if(hThrd == NULL)
	{
		CloseHandle(hThrd);
	}

	WaitForSingleObject(hThrd, 15000);

	delete m_pHttp;
	m_pHttp = 0;

	CloseHandle(hThrd);

	STLOG_WRITE("%s(%d): Resultado da consulta [%S]", __FUNCTION__, __LINE__, m_sRet);
	return m_sRet;
}

CString COnlineSearch::_DoSearchOnline(LPCTSTR szText)
{

#ifdef _WIN32_WCE
	if(!CUtil::IsOnline())
		return FALSE;
#endif

	//CHttp *http = new CHttp();
	//if(http != NULL)
	{
		if(m_proxyInfo.bProxy)		
			/*CHttp::GetInstance().*/m_pHttp->SetProxy(CStringW(m_proxyInfo.sServer), m_proxyInfo.nPort, CStringW(m_proxyInfo.sUser), CStringW(m_proxyInfo.sPass));		

		/*CHttp::GetInstance().m_pHttp->ResetArguments();*/

		CStringArray arr;
		CString s(szText);
		int pos = s.Find('|');
		int num = _wtol(s.Mid(0, pos));
		s = s.Mid(pos+1);
		CUtil::Tokenize(s, arr, num);

		for(int i = 0; i < arr.GetCount(); i++)
		{
			CString sParam = arr.GetAt(i);
			pos = sParam.Find('=');
			/*CHttp::GetInstance().*/m_pHttp->AddArguments(sParam.Mid(0, pos).Trim(), sParam.Mid(pos+1).Trim());
		}

		if(/*CHttp::GetInstance().*/m_pHttp->Request(CStringW(m_sURL)))
		{
			CString sText = /*CHttp::GetInstance().*/m_pHttp->Response();

			int nCode;

			if((sText.IsEmpty() || !CUtil::ValidatePost(&sText, TRUE, &nCode)) /*&& m_lpfnNoRecords != NULL*/)
			{
				STLOG_WRITE("%s: Sem registros ou erro na recepção. Retorno servidor: [%d: %S]", __FUNCTION__, nCode, sText); 
				//m_lpfnNoRecords(GetParent(), this);
			}
			else
			{
				STLOG_WRITE(sText);

				//if(m_lpfnSelection != NULL)
				//if(_listener != NULL)
				//{
				//	//m_lpfnSelection(GetParent(), this, s1);
				//	_listener->OnSelection(GetParent(), this, s1);
				//	//delete http;

				return sText;
				//}
			}
		}
		else
		{
			STLOG_WRITE("CEditDB::_DoSearchOnline: Erro na pesquisa online");
		}

		//delete http;
	}

	return L"";
}

void COnlineSearch::SetProxyInfo(CProxyInfo *p)
{
	m_proxyInfo.bDiscagem = p->bDiscagem;
	m_proxyInfo.bProxy	  = p->bProxy;
	m_proxyInfo.nPort	  = p->nPort;
	m_proxyInfo.sServer	  = p->sServer;
	m_proxyInfo.sUser	  = p->sUser;
	m_proxyInfo.sPass	  = p->sPass;
}


///////////////////////////////
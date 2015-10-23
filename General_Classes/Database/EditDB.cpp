#include "stdafx.h"
#include "EditDB.h"
#include "CppSQLite3.h"
#include "CStr.h"
#include "MsgWindow.h"
#include "SimpleRequest.h"
#include "Utils.h"
#include "Registry.h"
//#include "OnlineSearch.h"
#include "InetHttp.h"

CString CEditDB::m_sURL;

IMPLEMENT_DYNAMIC(CEditDB, CEditEx)



/**
\brief Construtor da classe
\param void
\return void
*/
CEditDB::CEditDB()
{
	
}


/**
\brief Destrutor da classe
\param void
\return void
*/
CEditDB::~CEditDB()
{
	//delete [] m_txtColumns;
}


BEGIN_MESSAGE_MAP(CEditDB, CEditEx)
END_MESSAGE_MAP()

// CEditDB message handlers

/**
\brief Realiza busca na base de dados do SD card
\param LPCTSTR szText Query de pesquisa
\todo
\return BOOL 
*/
BOOL CEditDB::_DoTextSearch(LPCTSTR szText)
{
	return FALSE;
}


/**
\brief Realiza busca online, utilizando URL do sistema Web_TEM
\param LPCTSTR szText Query de pesquisa
\return BOOL 
*/
BOOL CEditDB::_DoSearchOnline(LPCTSTR szText)
{
	CString sRet;
	//COnlineSearch ols;
	//ols.SetProxyInfo(&m_proxyInfo);
	//sRet = ols.DoSearch(szText, m_sURL);

	//int nCode;

	//if((sRet.IsEmpty()) /*&& m_lpfnNoRecords != NULL*/)
	//{
	//	STLOG_WRITE("%s: Sem registros ou erro na recepção. Retorno servidor: [%d: %S]", __FUNCTION__, nCode, sRet); 
	//	//m_lpfnNoRecords(GetParent(), this);
	//}
	//else
	//{
	//	STLOG_WRITE(sRet);

	//	//if(m_lpfnSelection != NULL)
	//	if(_listener != NULL)
	//	{
	//		//m_lpfnSelection(GetParent(), this, s1);
	//		_listener->OnSelection(GetParent(), this, sRet);
	//		//delete http;

	//		return TRUE;
	//	}
	//}

#ifdef _WIN32_WCE
	if(!CUtil::IsOnline())
		return FALSE;
#endif

	//CInetHttp http;
	CSimpleRequest *http = new CSimpleRequest();
	//CHttp *http = new CHttp();
	//if(http != NULL)
	{
		if(m_proxyInfo.bProxy)		
			http->SetProxy(CStringW(m_proxyInfo.sServer), m_proxyInfo.nPort, CStringW(m_proxyInfo.sUser), CStringW(m_proxyInfo.sPass));		

		http->ResetArguments();

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
			http->AddArguments(sParam.Mid(0, pos).Trim(), sParam.Mid(pos+1).Trim());
		}

		if(http->Request(CStringW(m_sURL)))
		{
			CString s1 = http->Response();
			

			int nCode;

			if((s1.IsEmpty() || !CUtil::ValidatePost(&s1, TRUE, &nCode)) /*&& m_lpfnNoRecords != NULL*/)
			{
				STLOG_WRITE("%s: Sem registros ou erro na recepção. Retorno servidor: [%d: %S]", __FUNCTION__, nCode, s1); 
				_listener->OnNoRecords(GetParent(), this);
				//m_lpfnNoRecords(GetParent(), this);
			}
			else
			{
				STLOG_WRITE(s1);

				//if(m_lpfnSelection != NULL)
				if(_listener != NULL)
				{
					//m_lpfnSelection(GetParent(), this, s1);
					_listener->OnSelection(GetParent(), this, s1);
					delete http;
					return TRUE;
				}
			}
		}
		else
		{
			STLOG_WRITE("CEditDB::_DoSearchOnline: Erro na pesquisa online");
		}
	}
	delete http;
	return FALSE;
}


/**
\brief Realiza busca na base de dados da aplicação preenchendo lista com resultados encontrados
\param LPCTSTR szText Query de pesquisa
\return BOOL 
*/
BOOL CEditDB::_DoDBSearch(LPCTSTR szText)
{		
	CStr sQuery(szText);
	BOOL bRet = FALSE;

	try
	{		
		CppSQLite3Query q = CppSQLite3DB::getInstance()->execQuery(sQuery);
		if(!q.eof())
		{
			CString s1;

			do
			{
				s1 = L"1|";

				for(int i = 0; i < q.numFields(); i++)
					s1 += CString(q.getStringField(i)) + _T("|");			

				if(GetResultArray() == NULL)
					break;
				
				GetResultArray()->Add(s1);
				q.nextRow();
			}
			while(!q.eof());

			//if(m_lpfnSelection != NULL)
			if(_listener != NULL)
			{
				//m_lpfnSelection(GetParent(), this, s1);
				_listener->OnSelection(GetParent(), this, s1);
				bRet = TRUE;					
			}
		}	
		

		q.finalize();

		return bRet;
	}
	catch(CppSQLite3Exception e)
	{
		STLOG_WRITE("CEditDB::_DoDBSearch: ERRO executando query", 
					e.errorMessage());
		STLOG_WRITE(sQuery);
	}

	return FALSE;
}


/**
\brief Verifica se uma string é formada de números
\param LPCTSTR szText String a ser verificada
\return BOOL
*/
BOOL CEditDB::IsNumeric(LPCTSTR szText)
{
	return _IsFindByCode(szText);
}


/**
\brief Método público que chamada aos métodos de pesquisa _DoSearchOnline(), _DoTextSearch() e _DoDBSearch()
\param LPCTSTR szText Query de pesquisa
\return void
*/
void CEditDB::Search(LPCTSTR szText)
{
	if(szText == NULL || _tcslen(szText) == 0)
	{
		//if(m_lpfnError != NULL)
		//	m_lpfnError(GetParent(), this, -1);
		if(_listener != NULL)
			_listener->OnError(GetParent(), this, -1);

		return;
	}

	CMsgWindow* msg;
	msg = CMsgWindow::getInstance();
	msg->Create(GetParent());
	msg->Show(m_sWaitMsg);

	CString sText(szText), sQuery;

	BOOL bCode = _IsFindByCode(sText);

	//Pesq online...
	if(_listener != NULL)
		_listener->OnGetQuery(GetParent(), this, TYPE_ONLINE, bCode, sText, sQuery);

	//m_lpfnGetQuery(GetParent(), this, TYPE_ONLINE, bCode, sText, sQuery);
	if(!sQuery.IsEmpty())
	{
		if(_DoSearchOnline(sQuery))
		{
			msg->Destroy();
			return;
		}
	}

	sQuery = L"";


	//Pesq texto...
	if(_listener != NULL)
		_listener->OnGetQuery(GetParent(), this, TYPE_TEXT, bCode, sText, sQuery);

	//m_lpfnGetQuery(GetParent(), this, TYPE_TEXT, bCode, sText, sQuery);
	if(!sQuery.IsEmpty())
	{
		if(_DoTextSearch(sQuery))
		{
			msg->Destroy();
			return;
		}
	}

	sQuery = L"";

	//Pesq offline...
	if(_listener != NULL)
		_listener->OnGetQuery(GetParent(), this, TYPE_DATABASE, bCode, sText, sQuery);

	//m_lpfnGetQuery(GetParent(), this, TYPE_DATABASE, bCode, sText, sQuery);
	if(!sQuery.IsEmpty())
	{
		if(_DoDBSearch(sQuery))
		{
			msg->Destroy();
			return;
		}
	}
	sQuery = L"";
	
	msg->Destroy();
}

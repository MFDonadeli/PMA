#include "StdAfx.h"
#include "SearchBase.h"
#include "MsgWindow.h"


CSearchBase::CSearchBase(void)
	: /*m_lpfnGetQuery(NULL)
	, m_lpfnError(NULL)
	, m_lpfnSelection(NULL)
	, m_lpfnNoRecords(NULL)
	, */m_sWaitMsg(L"Pesquisando...")
	, m_bShowCode(FALSE)
	, m_nMinChars(2)
	, m_pWndParent(NULL)
	, _listener(NULL)
{
	m_nMaxChars = -1;
}

CSearchBase::~CSearchBase(void)
{
}

void CSearchBase::SetListener(CSearchInterface *__listener)
{
	_listener = __listener;
}
/*
void CSearchBase::SetQueryCallback(LPFNGETQUERY lpfnGetQuery)
{
	m_lpfnGetQuery = lpfnGetQuery;
}

void CSearchBase::SetErrorCallback(LPFNCBERROR lpfnError)
{
	m_lpfnError = lpfnError;
}

void CSearchBase::SetSelectionCallback(LPFNCBSELECTION lpfnSelection)
{
	m_lpfnSelection = lpfnSelection;
}

void CSearchBase::SetNoRecordsCallback(LPFNCBNORECORDS lpfnNoRecords)
{
	m_lpfnNoRecords = lpfnNoRecords;
}
*/
BOOL CSearchBase::_IsFindByCode(const CString &s)
{
	BOOL bOK = TRUE;
	for(int i = 0; i < s.GetLength(); i++)
	{
		if(!_istdigit(s.GetAt(i)))
			return FALSE;
	}

	return TRUE;
}

void CSearchBase::Search(LPCTSTR szText)
{
	CMsgWindow* msg;
	msg = CMsgWindow::getInstance();
	msg->Create(NULL);
	msg->Show(m_sWaitMsg);

	CString sText(szText), sQuery;

	if(m_bShowCode)
	{
		int pos = sText.Find(L"]");
		if(pos > 0)
			sText = sText.Mid(1, pos-1);
	}

	BOOL bCode = _IsFindByCode(sText);

	//Pesq online...
	if(_listener != NULL)
		_listener->OnGetQuery(m_pWndParent, NULL, TYPE_ONLINE, bCode, sText, sQuery);

	//m_lpfnGetQuery(m_pWndParent, NULL, TYPE_ONLINE, bCode, sText, sQuery);
	if(!sQuery.IsEmpty())
	{
		if(_DoSearchOnline(sQuery))
		{
			msg->Destroy();
			return;
		}
	}

	//Pesq texto...
	if(_listener != NULL)
		_listener->OnGetQuery(m_pWndParent, NULL, TYPE_TEXT, bCode, sText, sQuery);

	//m_lpfnGetQuery(m_pWndParent, NULL, TYPE_TEXT, bCode, sText, sQuery);
	if(!sQuery.IsEmpty())
	{
		if(_DoTextSearch(sQuery))
		{
			msg->Destroy();
			return;
		}
	}

	//Pesq offline...
	if(_listener != NULL)
		_listener->OnGetQuery(m_pWndParent, NULL, TYPE_DATABASE, bCode, sText, sQuery);

	//m_lpfnGetQuery(m_pWndParent, NULL, TYPE_DATABASE, bCode, sText, sQuery);
	if(!sQuery.IsEmpty())
	{
		if(_DoDBSearch(sQuery))
		{
			msg->Destroy();
			return;
		}
	}

	msg->Destroy();
}

void CSearchBase::SetProxyInfo(CProxyInfo *p)
{
	m_proxyInfo.bDiscagem = p->bDiscagem;
	m_proxyInfo.bProxy	  = p->bProxy;
	m_proxyInfo.nPort	  = p->nPort;
	m_proxyInfo.sServer	  = p->sServer;
	m_proxyInfo.sUser	  = p->sUser;
	m_proxyInfo.sPass	  = p->sPass;
}

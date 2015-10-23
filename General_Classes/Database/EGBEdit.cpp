#include "stdafx.h"
#include "EGBEdit.h"
#include "CppSQLite3.h"
#include "CStr.h"
#include "MsgWindow.h"
#include "Http.h"
#include "Utils.h"
#include "TxtSearchWrapper.h"
#include "Registry.h"

IMPLEMENT_DYNAMIC(CEGBEdit, CEditEx)



/**
\brief Construtor da classe
\param void
\return void
*/
CEGBEdit::CEGBEdit()
{
}


/**
\brief Destrutor da classe
\param void
\return void
*/
CEGBEdit::~CEGBEdit()
{
}


BEGIN_MESSAGE_MAP(CEGBEdit, CEditEx)
END_MESSAGE_MAP()

// CEGBEdit message handlers

/**
\brief Realiza busca na base de dados do SD card
\param LPCTSTR szText Query de pesquisa
\return BOOL 
*/
BOOL CEGBEdit::_DoTextSearch(LPCTSTR szText)
{
	DWORD dwIni = GetTickCount();

	if(!m_sSDPath.IsEmpty())
	{
		CUtil::GetPathFromVariable(m_sSDPath);
		CTxtSearchWrapper vw;

		//int iveic[] = {1, 7, 6, 1, 3, 3, 1, 4};

		//vw.Setup(iveic, sizeof(iveic)/sizeof(int), 1);

		vw.Setup(m_txtColumns, m_txtColumnsCount, m_txtMainColumn);

		BOOL bOK = FALSE;
		CString s;
		vw.Find(m_sSDPath, szText, s, &bOK);
		if(bOK)
		{
			//if(m_lpfnSelection != NULL)
			//	m_lpfnSelection(GetParent(), this, s);

			if(_listener != NULL)
				_listener->OnSelection(GetParent(), this, s);

			return TRUE;
		}

		//m_lpfnNoRecords(GetParent(), this);
		/*if(_listener != NULL)
			_listener->OnNoRecords(GetParent(), this);*/
	}
	else
	{
		STLOG_WRITE("CEGBEdit::_DoTextSearch(): SD card path esta vazio");
	}


	return FALSE;
}

/**
\brief Método público que chamada aos métodos de pesquisa _DoSearchOnline(), _DoTextSearch() e _DoDBSearch()
\param LPCTSTR szText Query de pesquisa
\return void
*/
void CEGBEdit::Search(LPCTSTR szText)
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

	if(_listener != NULL)
		_listener->OnNoRecords(GetParent(), this);
	
	msg->Destroy();
}

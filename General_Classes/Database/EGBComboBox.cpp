// Copyright (c) 2007 Marcos Mori de Siqueira. All Rights Reserved. 
// mori@softfactory.com.br
// Classe que encapsula as pesquisas sql em um combo box...

#include "stdafx.h"
#include "EGBComboBox.h"
#include "CppSQLite3.h"
#include "CStr.h"
#include "MsgWindow.h"

IMPLEMENT_DYNAMIC(CEGBComboBox, CComboBox)


/**
\brief Construtor da classe
\param void
\return void
*/
CEGBComboBox::CEGBComboBox()
	: m_pDb(NULL)
	, m_bDropped(FALSE)
	, m_bIgnoreCbnDropDown(FALSE)
	, m_hWndList(NULL)
{
}


/**
\brief Destrutor da classe
\param void
\return void
*/
CEGBComboBox::~CEGBComboBox()
{
}

BEGIN_MESSAGE_MAP(CEGBComboBox, CComboBox)
	ON_CONTROL_REFLECT(CBN_DROPDOWN, OnDropDown)
	ON_CONTROL_REFLECT(CBN_CLOSEUP, OnCbnCloseup)
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_CONTROL_REFLECT(CBN_SELENDOK, &CEGBComboBox::OnCbnSelendok)
	ON_MESSAGE(WM_INTERNAL_MSG, OnInform)
	ON_MESSAGE(WM_CTLCOLORLISTBOX,  OnCtlColorListBox)
END_MESSAGE_MAP()


/**
\brief Chamado quando o evento de clique para expandir combo box é recebido
\details É disparado automaticamento quando a mensagem CBN_DROPDOWN é enviada à aplicação
\param void
\return void
*/
void CEGBComboBox::OnDropDown()
{
	m_bDropped = TRUE;

	if(!m_bIgnoreCbnDropDown)
	{
		CString sText;
		//if(m_lpfnGetQuery != NULL)
		if(_listener != NULL)
		{
			GetWindowText(sText);
			sText.Trim();
			if(sText.IsEmpty())
			{
				return;
			}
			Search(sText);
		}
	}

	if(m_hWndList)
	{
		RECT r;
		::GetWindowRect(m_hWndList, &r);
		::SetWindowPos(m_hWndList, NULL, 0, 0, r.right - r.left, 60, SWP_NOMOVE | SWP_NOZORDER);
	}
}


/**
\brief Realiza busca na base de dados do SD card
\param LPCTSTR szText Query de pesquisa
\return BOOL 
\todo Necessita implementação
*/
BOOL CEGBComboBox::_DoTextSearch(LPCTSTR szText)
{
	m_values.RemoveAll();
	ResetContent();
	return FALSE;
}


/**
\brief Realiza busca online, utilizando URL do sistema Web_TEM
\param LPCTSTR szText Query de pesquisa
\return BOOL 
\todo Necessita implementação
*/
BOOL CEGBComboBox::_DoSearchOnline(LPCTSTR szText)
{
	m_values.RemoveAll();
	ResetContent();
	return TRUE;
}



/**
\brief Realiza busca na base de dados da aplicação preenchendo lista com resultados encontrados
\param LPCTSTR szText Query de pesquisa
\return BOOL 
*/
BOOL CEGBComboBox::_DoDBSearch(LPCTSTR szText)
{
	m_values.RemoveAll();
	ResetContent();
	CStr sQuery(szText);

	try
	{
		BOOL bRet = FALSE;
		CppSQLite3Query q = m_pDb->execQuery(sQuery);

		if(!q.eof())
		{
			STLOG_WRITE(L"NENHUMA LINHA RETORNADA");
		}

		while(!q.eof())
		{
			bRet = TRUE;

			CString s;

			if(m_bShowCode)
			{
				s.Format(L"[%ld] - %s", 
						 q.getIntField(0),
						 CString(q.getStringField(1)).Trim());
			}
			else
			{
				s.Format(L"%S", q.getStringField(1));
			}

			int idx = AddString(s.Trim());

//			if(q.numFields() == 2)
//			{
//				SetItemData(idx, q.getIntField(0));	
//			}
//			else
//			{
				s.Empty();
				//if(m_lpfnSelection != NULL)
				if(_listener != NULL)
				{
					for(int i = 0; i < q.numFields(); i++)
						s += CString(q.getStringField(i)) + _T("|");

					POSITION p = m_values.AddTail(s);
					SetItemData(idx, (DWORD_PTR)p);
				}
//////////////////////
				else
				{
					SetItemData(idx, q.getIntField(0));	
				}
//////////////////////
//			}

			q.nextRow();
		}

		q.finalize();

		return bRet;
	}
	catch(CppSQLite3Exception e)
	{
		STLOG_WRITE("CEGBComboBox::_DoDBSearch: ERRO executando query", 
					e.errorMessage());
		STLOG_WRITE(sQuery);
	}

	return FALSE;
}


/**
\brief Obtém item selecionado no combo box
\param void
\return long Valor selecionado
*/
long CEGBComboBox::GetSelectedCode()
{
	int idx = GetCurSel();
	if(idx != CB_ERR)
	{
		//if(m_lpfnSelection == NULL)
		if(_listener == NULL)
			return (long) GetItemData(idx);
		else
		{
			POSITION p = (POSITION)GetItemData(idx);
			CString s;
			if(p)
			{
				s = m_values.GetAt(p);
				int pos = s.Find('|');
				s = s.Mid(0, pos);
			}
			TCHAR *endp;
			return _tcstol(s, &endp, 10);
		}
	}

	return -1;
}


/**
\brief Disparado quando o botão direito do mouse é liberado, click up
\param UINT nFlags
\param CPoint point
\return void
*/
void CEGBComboBox::OnLButtonUp(UINT nFlags, CPoint point)
{
	CComboBox::OnLButtonUp(nFlags, point);
}



/**
\brief Disparado quando o botão direito do mouse é pressionado, click down
\details Verifica antes de mostrar o dropdown, caso tenha erro nem abre o combobox
\param UINT nFlags
\param CPoint point
\return void
*/
void CEGBComboBox::OnLButtonDown(UINT nFlags, CPoint point)
{
	SetDroppedWidth(DRA::SCALEX(240));

	if(!m_bIgnoreCbnDropDown)
	{
		CRect r;
		GetClientRect(&r);
		r.left = r.right - DRA::SCALEX(12);	

		if(r.PtInRect(point))
		{
			if(!m_bDropped)
			{
				CString sText;
				GetWindowText(sText);
				sText.Trim();
				if(sText.GetLength() < m_nMinChars)
				{
					if(!m_bFixedContent) ResetContent();
					SetWindowText(sText);

					//if(m_lpfnError != NULL)
					//	m_lpfnError(GetParent(), this, m_nMinChars);
					if(_listener != NULL)
						_listener->OnError(GetParent(), this, m_nMinChars);
					return;
				}
			}
		}
	}

	CComboBox::OnLButtonDown(nFlags, point);
}


/**
\brief Chamado quando o evento de clique para recolher combo box é recebido
\details É disparado automaticamento quando a mensagem CBN_CLOSEUP é enviada à aplicação
\param void
\return void
*/
void CEGBComboBox::OnCbnCloseup()
{
	m_bDropped = FALSE;
}


/**
\brief Chamado quando o evento de seleção de opção do combo box é recebido
\details É disparado automaticamento quando a mensagem CBN_SELENDOK é enviada à aplicação. Retorna o item selecionado a janela pai
\param void
\return void
*/
void CEGBComboBox::OnCbnSelendok()
{
	//if(m_lpfnSelection != NULL)
	if(_listener != NULL)
	{
		if(GetCurSel() != CB_ERR)
		{
			POSITION p = (POSITION)GetItemData(GetCurSel());
			CString s = m_values.GetAt(p);
			//m_lpfnSelection(GetParent(), this, s);
			_listener->OnSelection(GetParent(), this, s);
		}
	}
}


/**
\brief Sem uso
\param WPARAM w
\param LPARAM l
\return LRESULT
*/
LRESULT CEGBComboBox::OnInform(WPARAM w, LPARAM l)
{
	//if(m_lpfnNoRecords != NULL)
	//	m_lpfnNoRecords(GetParent(), this);

	if(_listener != NULL)
		_listener->OnNoRecords(GetParent(), this);

	return 0L;
}



/**
\brief Método público que chamada aos métodos de pesquisa _DoSearchOnline(), _DoTextSearch() e _DoDBSearch()
\param LPCTSTR szText Query de pesquisa
\return void
*/
void CEGBComboBox::Search(LPCTSTR szText)
{
	STLOG_WRITE("%s(%d): Janela: %d", __FUNCTION__, __LINE__, CMsgWindow::getInstance()->GetSafeHwnd());

	CMsgWindow* msg;
	msg = CMsgWindow::getInstance();
	msg->Create(GetParent());
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
		_listener->OnGetQuery(GetParent(), this, TYPE_ONLINE, bCode, sText, sQuery);

	//m_lpfnGetQuery(GetParent(), this, TYPE_ONLINE, bCode, sText, sQuery);
	if(!sQuery.IsEmpty())
	{
		if(_DoSearchOnline(sQuery))
		{
			SetWindowText(sText);
			msg->Destroy();

			if(GetCount() == 1)
			{
				SetCurSel(0);
				OnCbnSelendok();
			}
			return;
		}
	}

	//Pesq texto...
	if(_listener != NULL)
		_listener->OnGetQuery(GetParent(), this, TYPE_TEXT, bCode, sText, sQuery);

	//m_lpfnGetQuery(GetParent(), this, TYPE_TEXT, bCode, sText, sQuery);
	if(!sQuery.IsEmpty())
	{
		if(_DoTextSearch(sQuery))
		{
			SetWindowText(sText);
			msg->Destroy();

			if(GetCount() == 1)
			{
				SetCurSel(0);
				OnCbnSelendok();
			}
			return;
		}
	}

	//Pesq offline...
	if(_listener != NULL)
		_listener->OnGetQuery(GetParent(), this, TYPE_DATABASE, bCode, sText, sQuery);

	//m_lpfnGetQuery(GetParent(), this, TYPE_DATABASE, bCode, sText, sQuery);
	if(!sQuery.IsEmpty())
	{
		if(_DoDBSearch(sQuery))
		{
			SetWindowText(sText);
			msg->Destroy();

			if(GetCount() == 1)
			{
				SetCurSel(0);
				OnCbnSelendok();
			}
			return;
		}
	}

	PostMessage(WM_INTERNAL_MSG);

	SetWindowText(sText);
	msg->Destroy();
}


LRESULT CEGBComboBox::OnCtlColorListBox(WPARAM wParam, LPARAM lParam)
{
        //
        // Check if the listbox is not subclassed yet
        //
        if(!m_hWndList)
        {
                m_hWndList      = (HWND) lParam;        // Get listbox HWND

                ////
                //// Subclass the listbox
                ////
                //m_pWndProc = (WNDPROC)GetWindowLong(m_hWndList, GWL_WNDPROC);
                //SetWindowLong(m_hWndList, GWL_WNDPROC, (LONG)ListWndProc);

                ////
                //// Associate the listbox HWND and this
                ////
                //g_map.insert(CCheckMap::value_type(m_hWndList, this));

                ////
                //// Set the item height
                ////
                //::SendMessage(m_hWndList, LB_SETITEMHEIGHT,
                //                                0, MAKELPARAM(m_nItemHeight,0));

                ////
                //// Fill in the list
                ////
                //OnDropDown();
        }

        return DefWindowProc(WM_CTLCOLORLISTBOX, wParam, lParam);
}
// Copyright (c) 2007 Marcos Mori de Siqueira. All Rights Reserved. 
// mori@softfactory.com.br
// Classe de interface, encapsula a montagem dinamica dos controles
// e a navegacao. Deve ser extendida por todos os modulos de dados
#include "stdafx.h"
#include "UIDialog.h"
#include "ModParam.h"
#include "Utils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define ID_SCROLLBAR	0x343
#define SCROLL_WIDTH	15
#define WM_CHANGEPAGE	(WM_USER + 100)
#define PANEL_HEIGHT	20

IMPLEMENT_DYNAMIC(CUIDialog, CDialogEx)

/**
\brief 
	Construtor da classe
\details
	Funções executadas neste módulo:
	- Inicialização de variáveis globais

\param 
	UINT nResID: ID do form que esta classe usará
	CWnd* pParent: Ponteiro para a janela que será pai desta
*/
CUIDialog::CUIDialog(UINT nID, CWnd* pParent /*=NULL*/)
	: CDialogEx(nID, pParent)
{
	m_bKeepSIPUp = FALSE;
	m_bHandleItemDelete = TRUE;
	m_nAtualPage = 0;

//debug only
//m_bDisableCtrlColor = TRUE;
//SetBackColor(RGB(222,222,222));
}

/**
\brief 
	Configura as trocas e validações dos campos desta janela (União de controles e variáveis)
\param 
	CDataExchange* pDx: Ponteiro para a classe que faz essa troca
\return 
	void
*/
void CUIDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CUIDialog, CDialogEx)
	ON_WM_VSCROLL()
	ON_WM_SIZE()
	ON_WM_SETTINGCHANGE()
	ON_MESSAGE(WM_CHANGEPAGE, OnChangePage)
	ON_WM_TIMER()
END_MESSAGE_MAP()

/**
\brief
	Início desta janela.
\details
	Funções executadas neste módulo:
	- Modificar o estilo da janela para adequar aos padrões do aplicativo
	- Criar barra de rolagem na tela
	- Mostra SIP se for o caso

\return 
	TRUE se a execução ocorrer com sucesso
*/
BOOL CUIDialog::OnInitDialog()
{
	//m_waitDlg.Create(this);
	CDialogEx::OnInitDialog();

	VERIFY(m_scroll.Create(WS_CHILD|SBS_VERT, 
						   CRect(0, 0, 0, 0), 
						   this, 
						   ID_SCROLLBAR));

	VERIFY(CUtil::CreateFont(&m_font, L"MS Sans Serif", -11));
	SetFont(&m_font);

	if(m_bKeepSIPUp)
		ShowSIP();

	return TRUE;
}

/**
\brief
	Método para execução pós inicialização
\details
	Funções executadas neste método:
	- Pintar o fundo da tela cliente
	- Reposicionar o dialog, para incluir o SIP
	- Enviar a mensagem a janela pai que a tela acabou de carregar

\param
	void
\return
	void
*/
void CUIDialog::PostInitDialog()
{
	CRect rectB;
	GetClientRect(&rectB);
#ifdef _WIN32_WCE
	if(m_banner.GetSafeHwnd() != NULL && IsWindow(m_banner.GetSafeHwnd()))
		m_banner.GetClientRect(&rectB);
#endif

	if(m_panel.GetSafeHwnd() == NULL)
	{
		VERIFY(m_panel.Create(this, 
					   CRect(0, 
					   rectB.Height(), 
					   rectB.Width() - SCROLL_WIDTH, 
					   rectB.Height() + PANEL_HEIGHT), 
					   (UINT)-1));

		m_panel.SetBkColor(RGB(255, 255, 255));
	}

	// Reposiciona o dialog, pois o SIP esta visivel...
	if(m_bKeepSIPUp)
	{
		CRect rectCli;
		GetWindowRect(&rectCli);

		// Descontar o SIP e o CommandBar...
		rectCli.bottom -= 80 + 26;
		SetWindowPos(NULL, 
					 0, 
					 0, 
					 rectCli.Width(), 
					 rectCli.Height(), 
					 SWP_NOMOVE|SWP_NOZORDER);
	}

	SetTimer(1, 10, NULL);

#ifdef _WIN32_WCE
	/*** Deprecated: Banner antigo ***/ 
	//m_banner.ShowWindow(SW_SHOW);
#endif

	// Avisar o pai que a inicializacao encerrou...
	// Encerra o Wait cursor...
	if(GetParent() != NULL)
		GetParent()->PostMessage(CModParam::WM_MODULE_READY, 0, (LPARAM) GetSafeHwnd());

	SetForegroundWindow();  
	// Resize the window over the taskbar area.
	CRect rect;
	rect = CRect(0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
	MoveWindow(&rect, TRUE);
#ifdef _WIN32_WCE
	SHFullScreen(m_hWnd, SHFS_HIDESTARTICON|SHFS_HIDETASKBAR|SHFS_SHOWSIPBUTTON);
#endif
}

/**
\brief
	Posiciona a janela, de acordo com o tamanho.
\details
	Funções executadas neste método:
	- Inicia a função de posicionamento

\param
	UINT nType: Tipo de posicionamento requerido
\param
	int cx: Largura da tela do equipamento 
\param
	int cy: Altura da tela do equipamento
\return
	void
	
*/
void CUIDialog::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
	_OnSize(cx, cy);
};

/**
\brief
	Método de reposicionamento da barra de rolagem
\details
	A barra de rolagem é posicionada de acordo com a visibilidade do SIP
\param
	int cx: Posição x da barra de rolagem
\param
	int cy: Posição y da barra de rolagem
*/
void CUIDialog::_OnSize(int cx, int cy)
{
#ifdef _WIN32_WCE
	// Reposicionar o scrollbar...
	if(IsWindow(m_scroll.GetSafeHwnd()) && IsWindow(m_banner.GetSafeHwnd()))
	{
		BOOL bSIPVisible = FALSE;
		// Recupera os dados do banner...
		CRect rect;
		rect.SetRectEmpty();
		m_banner.GetWindowRect(&rect);

		// Dados da area de trabalho...
		CRect r;
		VERIFY(SystemParametersInfo(SPI_GETWORKAREA, 0, &r, NULL) == 1);

		// Dados do SIP...
		SIPINFO si = {0};
		si.cbSize = sizeof(si);
		SHSipInfo(SPI_GETSIPINFO, 0, &si, 0);
		bSIPVisible = si.fdwFlags & SIPF_ON;


		CRect rectScroll = CRect(cx-SCROLL_WIDTH, 
								 rect.Height(),
								 cx,
								 bSIPVisible ? cy : r.Height()); 

		m_scroll.MoveWindow(rectScroll);
	}
#endif
}

/**
\brief
	Método chamado quando é mudado algum parâmetro de sistema na janela.
\details
	Funções executadas por este método:
	- Manter a janela no lugar ao chamar/esconder o teclado virtual
*/
void CUIDialog::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
#ifdef _WIN32_WCE
	if(m_bKeepSIPUp)
		SHSipPreference(GetSafeHwnd(), SIP_UP);
	else
#endif
		CDialogEx::OnSettingChange(uFlags, lpszSection);
}

/**
\brief
	Método executado quando há uma ação na barra de rolagem vertical
\details
	A função deste método é mover o cursor da barra de rolagem, atualizando seu status
\param
	UINT nSBCode: Tipo de movimento da barra de rolagem
\param
	UINT nPos: Posição da barra de rolagem após a ação
\param
	CScrollBar *pScrollBar: Ponteiro para a barra de rolagem que recebeu a ação
\return
	void
*/
void CUIDialog::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar)
{
	ASSERT_VALID(this);
	ASSERT(pScrollBar == &m_scroll);

	SCROLLINFO info;
	pScrollBar->GetScrollInfo(&info, SIF_ALL);
	int curPos = info.nPos;

	if(info.nPage == 0) { info.nPage = 1; }

	switch (nSBCode)
	{
		case SB_LEFT:      // Scroll to far left.
			curPos = 0;
			break;

		case SB_RIGHT:      // Scroll to far right.
			curPos = info.nMax;
			break;

		case SB_ENDSCROLL:   // End scroll.
			break;

		case SB_LINELEFT:      // Scroll left.
			if(curPos > info.nMin)
				curPos--;
			break;

		case SB_LINERIGHT:   // Scroll right.
			if(curPos < info.nMax)
				curPos++;
			break;

		case SB_PAGELEFT:    // Scroll one page left.
   			if(curPos > info.nMin)
				curPos = max(info.nMin, curPos - (int)info.nPage);
			break;

		case SB_PAGERIGHT:      // Scroll one page right
			if(curPos < info.nMax)
				curPos = min(info.nMax, curPos + (int) info.nPage); 
			break;

		case SB_THUMBPOSITION: // Scroll to absolute position. nPos is the position
			curPos = nPos;     // of the scroll box at the end of the drag operation.
			break;

		case SB_THUMBTRACK:	   // Drag scroll box to specified position. nPos is the
			curPos = nPos;     // position that the scroll box has been dragged to.
			break;
	}

	//TRACE(L"Page %ld\n", curPos);

	// Se chamar metodos de show/hide aqui dentro, dah exception...
	PostMessage(WM_CHANGEPAGE, curPos, m_nAtualPage);

	pScrollBar->SetScrollPos(curPos);
	CDialogEx::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CUIDialog::SetNumPages(int num)
{
	m_scroll.SetScrollRange(0, num);
}

__UIGroup *CUIDialog::CreateGroup(LPCTSTR szGroup)
{
	__UIGroup *pGroup = new __UIGroup();
	if(szGroup != NULL && _tcslen(szGroup) > 0)
		m_sPageTitle = szGroup;

	// Se nao tiver texto, usa o do anterior...
	pGroup->m_sText = m_sPageTitle;
	pGroup->m_page  = m_groups.GetCount();
	m_groups.AddTail(pGroup);
	return pGroup; 
}

void CUIDialog::AddItem(__UIGroup *pGroup, CWnd *pItem)
{
	pGroup->m_items.AddTail(pItem);
}

BOOL CUIDialog::DestroyWindow()
{
	POSITION p = m_groups.GetHeadPosition();
	while(p)
	{
		__UIGroup *pGroup = m_groups.GetNext(p);

		// Somente deve se deletar se os controles forem
		// criados com 'new'...
		if(m_bHandleItemDelete)
		{
			POSITION p1 = pGroup->m_items.GetHeadPosition();
			while(p1)
			{
				CWnd *pItem = pGroup->m_items.GetNext(p1);
				pItem->DestroyWindow();
				delete pItem;	
			}

			pGroup->m_items.RemoveAll();
		}

		delete pGroup;
	}

	m_groups.RemoveAll();

	return CDialogEx::DestroyWindow();
}

void CUIDialog::AddLabel(__UIGroup *pGroup, CStatic *pWnd, LPCTSTR szText, int x, int y, BOOL bVisible)
{
	ASSERT(pGroup != NULL);
	ASSERT(pWnd != NULL);
	ASSERT(szText != NULL);

	DWORD dwFlags = WS_CHILD|SS_LEFT;
	if(bVisible)
		dwFlags |= WS_VISIBLE;

	CRect r;
	r.left = x;
	r.top  = y;
	_GetTextRect(szText, r);

	r.top	 += 2;
	r.bottom += 2;

	//TRACE(L"AddLabel-SIZE: %d, %d, %d, %d\n", r.left, r.top, r.Width(), r.Height());

	VERIFY(pWnd->Create(szText, dwFlags, r, this));
	AddItem(pGroup, pWnd);

	pWnd->SetFont(&m_font);
}

void CUIDialog::AddCheckBox(__UIGroup *pGroup, CButton *pWnd, LPCTSTR szText, const RECT &rect, UINT nID, BOOL bVisible)
{
	DWORD dwFlags = WS_CHILD|WS_TABSTOP|BS_AUTOCHECKBOX|BS_CHECKBOX;
	if(bVisible)
		dwFlags |= WS_VISIBLE;

	_AddButton(dwFlags, pGroup, pWnd, szText, rect, nID);
}

void CUIDialog::AddButton(__UIGroup *pGroup, CButton *pWnd, LPCTSTR szText, const RECT &rect, UINT nID, BOOL bVisible)
{
	DWORD dwFlags = WS_CHILD|WS_TABSTOP|BS_TEXT|BS_PUSHBUTTON;
	if(bVisible)
		dwFlags |= WS_VISIBLE;

	_AddButton(dwFlags, pGroup, pWnd, szText, rect, nID);
}

void CUIDialog::AddOptionBox(__UIGroup *pGroup, CButton *pWnd, LPCTSTR szText, const RECT &rect, UINT nID, BOOL bVisible)
{
	DWORD dwFlags = WS_CHILD|WS_TABSTOP|BS_AUTORADIOBUTTON;//|BS_RADIOBUTTON;
	if(bVisible)
		dwFlags |= WS_VISIBLE;

	_AddButton(dwFlags, pGroup, pWnd, szText, rect, nID);
}

void CUIDialog::_AddButton(DWORD dwFlags, 
						   __UIGroup *pGroup, 
						   CButton *pWnd, 
						   LPCTSTR szText, 
						   const RECT &rect, 
						   UINT nID)
{
	ASSERT(pGroup != NULL);
	ASSERT(pWnd != NULL);

	CRect r;
	r.left   = rect.left;
	r.top    = rect.top + _GetTop();
	r.right  = r.left + rect.right;
	r.bottom = r.top + rect.bottom;

	if(((dwFlags & BS_CHECKBOX) == BS_CHECKBOX) || ((dwFlags & BS_AUTORADIOBUTTON) == BS_AUTORADIOBUTTON))
		r.right  += 20;
	else if((dwFlags &BS_TEXT) == BS_TEXT)
		r.bottom += 2; 

	//TRACE(L"_AddButton-SIZE: %d, %d, %d, %d\n", r.left, r.top, r.Width(), r.Height());

	VERIFY(pWnd->Create(szText, dwFlags, r, this, nID));
	AddItem(pGroup, pWnd);

	pWnd->SetFont(&m_font);
}

void CUIDialog::AddTextBox(__UIGroup *pGroup, 
						   CEdit *pWnd, 
						   LPCTSTR szText, 
						   const RECT &rect, 
						   int len, 
						   UINT nID, 
						   BOOL bMultiLine, 
						   BOOL bVisible)
{
	ASSERT(pGroup != NULL);
	ASSERT(pWnd != NULL);

	DWORD dwFlags = WS_CHILD|WS_BORDER|WS_TABSTOP;
	if(bVisible)
		dwFlags |= WS_VISIBLE;

	if(bMultiLine)
		dwFlags |= ES_MULTILINE|ES_WANTRETURN|WS_VSCROLL;
	else
		dwFlags |= ES_AUTOHSCROLL;

	CRect r;
	r.top    = rect.top + _GetTop();
	r.bottom = r.top + rect.bottom + 2;
	r.left   = rect.left;
	r.right  = r.left + rect.right;

	//TRACE(L"AddTextBox-SIZE: %d, %d, %d, %d\n", r.left, r.top, r.Width(), r.Height());

	VERIFY(pWnd->Create(dwFlags, r, this, nID));
	AddItem(pGroup, pWnd);

	if(szText != NULL && _tcslen(szText) > 0)
		pWnd->SetWindowText(szText);

	pWnd->SetFont(&m_font);

	if(len > 0)
		pWnd->SetLimitText(len);
}

void CUIDialog::AddComboBox(__UIGroup *pGroup, CComboBox *pWnd, LPCTSTR szText, const RECT &rect, int height, UINT nID, BOOL bEdit, BOOL bVisible)
{
	DWORD dwFlags = WS_CHILD|WS_BORDER|WS_TABSTOP|WS_VSCROLL;

	if(bEdit)
		dwFlags |= CBS_DROPDOWN;
	else
		dwFlags |= CBS_DROPDOWNLIST;

	if(bVisible)
		dwFlags |= WS_VISIBLE;

	CRect r;
	r.top    = rect.top + _GetTop();
	r.bottom = r.top + rect.bottom + 2 + height;
	r.left   = rect.left;
	r.right  = r.left + rect.right;

	//TRACE(L"AddComboBox-SIZE: %d, %d, %d, %d\n", r.left, r.top, r.Width(), r.Height());

	VERIFY(pWnd->Create(dwFlags, r, this, nID));
	AddItem(pGroup, pWnd);
	pWnd->SetFont(&m_font);
	//pWnd->SetDroppedWidth();
}
//DTS_SHOWNONE
void CUIDialog::AddDateCtrl(__UIGroup *pGroup, CDateTimeCtrl *pWnd, const RECT &rect, UINT nID, BOOL bVisible, BOOL bEnable)
{
	DWORD dwFlags = WS_CHILD|WS_BORDER|WS_TABSTOP|DTS_SHORTDATECENTURYFORMAT|DTS_SHOWNONE;
	if(bVisible)
		dwFlags |= WS_VISIBLE;

	_AddDateTimeCtrl(dwFlags, pGroup, pWnd, rect, nID, bEnable);
}

void CUIDialog::AddTimeCtrl(__UIGroup *pGroup, CDateTimeCtrl *pWnd, const RECT &rect, UINT nID, BOOL bVisible)
{
	DWORD dwFlags = WS_CHILD|WS_BORDER|WS_TABSTOP|DTS_TIMEFORMAT;
	if(bVisible)
		dwFlags |= WS_VISIBLE;

	_AddDateTimeCtrl(dwFlags, pGroup, pWnd, rect, nID);
}

void CUIDialog::_AddDateTimeCtrl(DWORD dwFlags, __UIGroup *pGroup, CDateTimeCtrl *pWnd, const RECT &rect, UINT nID, BOOL bEnable)
{
	CRect r;
	r.top    = rect.top + _GetTop();
	r.bottom = r.top + rect.bottom + 2;
	r.left   = rect.left;
	r.right  = r.left + rect.right;

	//TRACE(L"_AddDateTimeCtrl-SIZE: %d, %d, %d, %d\n", r.left, r.top, r.Width(), r.Height());

	VERIFY(pWnd->Create(dwFlags, r, this, nID));
	AddItem(pGroup, pWnd);
	pWnd->SetFont(&m_font);

	SYSTEMTIME st;
	GetSystemTime(&st);
	if(!bEnable)
		pWnd->SendMessage(DTM_SETSYSTEMTIME, GDT_NONE, (LPARAM) &st);
}

void CUIDialog::AddListBox(__UIGroup *pGroup, CListBox *pWnd, LPCTSTR szText, const RECT &rect, UINT nID, BOOL bVisible)
{
	DWORD dwFlags = WS_CHILD|WS_BORDER|WS_TABSTOP;
	if(bVisible)
		dwFlags |= WS_VISIBLE;

	CRect r;
	r.top    = rect.top + _GetTop();
	r.bottom = r.top + rect.bottom + 2;
	r.left   = rect.left;
	r.right  = r.left + rect.right;

	//TRACE(L"AddComboBox-SIZE: %d, %d, %d, %d\n", r.left, r.top, r.Width(), r.Height());

	VERIFY(pWnd->Create(dwFlags, r, this, nID));
	AddItem(pGroup, pWnd);
	pWnd->SetFont(&m_font);
}

void CUIDialog::_TogglePage(int page, BOOL bVisible)
{
	if(page < 0)
		return;

	POSITION p = m_groups.FindIndex(page);
	if(p != NULL)
	{
		__UIGroup *pGroup = m_groups.GetAt(p);
		ASSERT(pGroup != NULL);

		POSITION p1 = pGroup->m_items.GetHeadPosition();
		while(p1)
		{
			CWnd *pItem = pGroup->m_items.GetNext(p1);
			ASSERT(pItem != NULL);
			if(bVisible)
				pItem->ShowWindow(SW_SHOW);
			else
				pItem->ShowWindow(SW_HIDE);
		}

		if(pGroup->m_pFirst)
			pGroup->m_pFirst->SetFocus();
	}
}

LRESULT CUIDialog::OnChangePage(WPARAM wParam, LPARAM lParam)
{
	int curPos = (int)wParam;
	int oldPos = (int)lParam;

	if(curPos == oldPos)
		return 0L;

	_TogglePage(oldPos, FALSE);
	_TogglePage(curPos, TRUE);
	m_nAtualPage = curPos;

	_UpdateButtons();
	_UpdatePanel();

	m_scroll.SetScrollPos(curPos);

	return 0L;
}

void CUIDialog::_GetTextRect(LPCTSTR szText, CRect &r)
{
	CRect cliRect;
	GetClientRect(&cliRect);

	CDC *pDC = GetDC();
	ASSERT(pDC != NULL);

	int top = _GetTop();
	r.top += top;

	// Verifica se tenho multiplas linhas...
	if((_tcsstr(szText, _T("\r")) != NULL) || 
	   (_tcsstr(szText, _T("\n")) != NULL)   )
	{
		int lines  = 0;
		int max    = 0;
		int height = 0;

		CString s(szText);
		if(s.GetAt(s.GetLength()-1) != '\r' &&
		   s.GetAt(s.GetLength()-1) != '\n'  )
		   s += _T("\n");

		while(TRUE)
		{
			if(s.IsEmpty())
				break;

			int pos = s.FindOneOf(_T("\r\n"));
			if(pos < 0)
				break;

			CString s1 = s.Mid(0, pos);
			CSize sz = pDC->GetTextExtent(s1);
			lines++;

			height = sz.cy;
			if(sz.cx > max)
				max = sz.cx;

			s = s.Mid(pos+1);
		};

		r.top   -= 3; // subir o top... 
		r.right  = r.left + max;
		r.bottom = r.top  + (height * lines);
	}
	else
	{
		CSize sz = pDC->GetTextExtent(szText, _tcslen(szText));

		// Se nao couber na linha...
		if((sz.cx + r.left + SCROLL_WIDTH) > cliRect.Width())
		{
			r.right  = cliRect.Width() - r.left - SCROLL_WIDTH;
			r.bottom = r.top  + (sz.cy * 2);
		}
		else
		{
			r.right  = r.left + sz.cx + 5;
			r.bottom = r.top  + sz.cy;
		}
	}
}

void CUIDialog::OnPrev()
{
	if(m_nAtualPage > 0)
	{
		m_scroll.SetScrollPos(m_nAtualPage-1);
		PostMessage(WM_CHANGEPAGE, m_nAtualPage-1, m_nAtualPage);
	}
}

void CUIDialog::OnNext()
{
	if(m_nAtualPage <= m_groups.GetCount())
	{
		m_scroll.SetScrollPos(m_nAtualPage+1);
		PostMessage(WM_CHANGEPAGE, m_nAtualPage+1, m_nAtualPage);
	}
}

void CUIDialog::_UpdateButtons()
{
#ifdef _WIN32_WCE
	// Recuperar o style do botao 'voltar'
	UINT nStyle = m_dlgCommandBar.GetButtonStyle(0);

	// Se estamos no root, desabilitar o botao...
	if(IsFirstPage())
		nStyle |= TBBS_DISABLED; 
	else
		nStyle &= ~TBBS_DISABLED; 

	// Setar o style novamente...
	m_dlgCommandBar.SetButtonStyle(0, nStyle);

	// Recuperar o style do botao 'proximo'
	nStyle = m_dlgCommandBar.GetButtonStyle(1);

	// Se estamos no root, desabilitar o botao...
	if(IsLastPage())
		nStyle |= TBBS_DISABLED; 
	else
		nStyle &= ~TBBS_DISABLED; 

	// Setar o style novamente...
	m_dlgCommandBar.SetButtonStyle(1, nStyle);
#endif
}

void CUIDialog::_UpdatePanel()
{
	if(m_panel.GetSafeHwnd() == NULL)
		return;

	CString s;

	// Setar o numero da pagina e o titulo...
	POSITION p = m_groups.FindIndex(m_nAtualPage);
	if(p != NULL)
	{
		__UIGroup *pGroup = m_groups.GetAt(p);
		if(pGroup != NULL)
		{
			CString sTitle = pGroup->m_sText;
			if(sTitle.IsEmpty())
				sTitle = m_sPageTitle;

			// Sempre maiusculo...
			sTitle.MakeUpper();

			s.Format(L"%d/%d %s", m_nAtualPage + 1, m_groups.GetCount(), sTitle);
			m_panel.SetText(s);
		}
	}
}

void CUIDialog::OnTimer(UINT_PTR nIDEvent)
{
	if(nIDEvent == 1)
	{
		KillTimer(1);

		// As classes derivadas devem criar os controles... 
		if(!_CreateControls())
		{
			CString msg;
			msg.LoadString(IDS_ERROUICONTROL);

			MessageBox(msg, L"Mensagem", MB_ICONERROR|MB_OK);
			STLOG_WRITE(L"CUIDialog::OnTimer(): Erro criando controles.");
			return;
		}

		_UpdatePanel();
		_TogglePage(m_nAtualPage);

		m_scroll.ShowWindow(SW_SHOW);
		m_panel.ShowWindow(SW_SHOW);

#ifdef _WIN32_WCE
		SHSipPreference(GetSafeHwnd(), SIP_UP);
#endif
	}

	CDialogEx::OnTimer(nIDEvent);
}

int CUIDialog::_GetTop()
{
	CRect rect(0,0,0,0);
#ifdef _WIN32_WCE
	if(IsWindow(m_banner.GetSafeHwnd()))
		m_banner.GetWindowRect(&rect);
#endif

	CRect rectP(0,0,0,0);
	if(IsWindow(m_panel.GetSafeHwnd()))
		m_panel.GetWindowRect(&rectP);

	return rect.Height() + rectP.Height();
}

BOOL CUIDialog::PreTranslateMessage(MSG* pMsg)
{
	// Tratar o sobe desce do joystick...
	if(pMsg->message == WM_KEYDOWN || pMsg->message == WM_KEYUP)
	{
		if(pMsg->lParam == 1 && pMsg->wParam < 132)
		{
			if(pMsg->wParam == VK_DOWN)
				OnNext();
			else if(pMsg->wParam == VK_UP)
				OnPrev();
		}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

void CUIDialog::_SelectControl(CWnd *pWnd)
{
	POSITION p = m_groups.GetHeadPosition();
	while(p)
	{
		__UIGroup *pGroup = m_groups.GetNext(p);
		ASSERT(pGroup);

		if(pGroup->m_items.Find(pWnd) != NULL)
		{
			OnChangePage(pGroup->m_page, m_nAtualPage); 
			pWnd->SetFocus();
			break;
		}
	}
}

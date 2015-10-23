// Copyright (c) 2007 Marcos Mori de Siqueira. All Rights Reserved. 
// mori@softfactory.com.br
// Classe base para a criacao de dialogs, encapsula controle de 
// fullscreen, banner etc.
#include "stdafx.h"
#include "DialogEx.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MENU_HEIGHT 26

IMPLEMENT_DYNAMIC(CDialogEx, CDialog)

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
CDialogEx::CDialogEx(UINT nID, CWnd* pParent /*=NULL*/)
	: CDialog(nID, pParent)
{
	m_bUseBackColor = FALSE;
	m_crBackground  = GetSysColor(COLOR_WINDOW);
	m_bSipVisible   = FALSE;
	m_bDisableCtrlColor = FALSE;
	m_hBackBrush.CreateSolidBrush(m_crBackground); 
	m_crBackgroundR = RGB(230,230,230);
	m_hBackBrushR.CreateSolidBrush(m_crBackgroundR); 
}

/**
\brief 
	Configura as trocas e validações dos campos desta janela (União de controles e variáveis)
\param 
	CDataExchange* pDx: Ponteiro para a classe que faz essa troca
\return 
	void
*/
void CDialogEx::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CDialogEx, CDialog)
	ON_WM_CTLCOLOR()
	ON_WM_ERASEBKGND()
	ON_WM_ACTIVATE()
	ON_WM_SETTINGCHANGE()
END_MESSAGE_MAP()

/**
\brief
	Cria o banner da parte superior da tela
\param
	UINT nResID: ID do bitmap a ser usado
	BOOL bStretch: Indica se o banner será ou não esticado
	int height: altura do banner
	LPCTSTR szText: Texto a ser exibido
	int nLeftPos: Espaçamento para exibição do ícone no bitmap
\return
	void
*/
void CDialogEx::_CreateBanner(UINT nResID, BOOL bStretch, int height, LPCTSTR szText, int nLeftPos)
{
	CRect rect;
	GetWindowRect(&rect);

#ifdef _WIN32_WCE		
	CWnd *pWnd = FindWindow(_T("HHTaskBar"),_T(""));
	if(pWnd != NULL)
	{
		pWnd->GetWindowRect(&rect);
	}

	height = rect.Height();

	// Criar o banner que fica sobre a lista...
	m_banner.Create(CRect(0, 0, rect.Width(), height), this, 0x345, FALSE);
	m_banner.SetStretch(TRUE);
	if(nResID!=-1)
	{
		m_banner.Load(nResID);
	}
	else
	{
		#if (_WIN32_WCE >= 0x500)
			m_banner.LoadJPG(L"\\imgs\\Login.png");
		#endif
	}

	if(szText != NULL && _tcslen(szText) > 0)
	{
		m_banner.SetText(szText);
		// Dar espaco para mostrar o icone no bitmap...
		m_banner.SetTextLeftPos(nLeftPos);
	}
#endif
}

/**
\brief Atualiza título do banner
\param LPCTSTR szTitle: Texto a ser exibido
\return void
*/
void CDialogEx::_CreateBanner(LPCTSTR szText)
{	
#ifdef _WIN32_WCE	
	//Atualiza título do banner...
	CBannerWindow::getInstance()->UpdateText(szText);
#endif
}

/**
\brief
	Redimensiona a janela para ocupar toda a tela do equipamento
\param
	void
\return
	void
*/
void CDialogEx::_FullScreen()
{
#ifdef _WIN32_WCE
	SetForegroundWindow();  
	// Resize the window over the taskbar area.
	CRect rect = CRect(0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
	MoveWindow(&rect, TRUE);

	HWND win = ::FindWindow(_T("HHTaskBar"), _T(""));
	::EnableWindow(win, FALSE);
	::ShowWindow(win,SW_HIDE);

	SHFullScreen(m_hWnd, SHFS_HIDESTARTICON|SHFS_HIDETASKBAR);
#endif
}

/**
\brief
	Função chamada quando um controle é desenhado
\details
	Esta função irá definir a cor de fundo de alguns controles
\param
	CDC* pDC: Ponteiro para o contexto de visualização para o controle
\param
	CWnd* pWnd: Ponteiro para o controle que requisita a troca de cor
\param
	UINT nCtlColor: Tipo do controle
\return
	HBRUSH: Um handle que será utilizado para pintar o controle
*/
HBRUSH CDialogEx::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) 
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);

	if(m_bUseBackColor)
	{
		TCHAR szClassName[128];
		::GetClassName(pWnd->GetSafeHwnd(), szClassName, 127);
		CString sClassName(szClassName);

		if(sClassName.CompareNoCase(_T("EDIT")) == 0 ||
		   sClassName.CompareNoCase(_T("WCE_EDIT")) == 0)
		{
			if(pWnd->GetStyle() & ES_READONLY)
			{
				pDC->SetBkColor(m_crBackground);
				hbr = (HBRUSH)m_hBackBrush;
				Invalidate(TRUE);
			}

			if(!pWnd->IsWindowEnabled())
			{
				pDC->SetBkColor(m_crBackgroundR);
				hbr = (HBRUSH)m_hBackBrushR;
				Invalidate(TRUE);
			}
		}

		switch (nCtlColor)
		{	
			case CTLCOLOR_MSGBOX:
			case CTLCOLOR_EDIT:
				{
					// Aqui podemos mudar o background do edit 
					// qdo esta no foco, por exemplo...
					if(sClassName.CompareNoCase(_T("EDIT")) == 0 ||
					   sClassName.CompareNoCase(_T("WCE_EDIT")) == 0)
					{
						if(((CEdit *)pWnd)->GetStyle() & ES_READONLY)
						{
							pDC->SetBkColor(m_crBackground);
							hbr = (HBRUSH)m_hBackBrush;

							Invalidate(TRUE);
						}
					}
				}
				break;

			case CTLCOLOR_LISTBOX:
			case CTLCOLOR_BTN:
				break;

			case CTLCOLOR_STATIC:
				{
					if(!m_bDisableCtrlColor)
					{
						// Pintar o fundo dos statics...
						if(sClassName.CompareNoCase(_T("BUTTON")) != 0)
						{
							pDC->SetBkColor(m_crBackground);
							hbr = (HBRUSH)m_hBackBrush;
						}
						else
						{
							if((pWnd->GetStyle() & BS_GROUPBOX) == BS_GROUPBOX)
							{
								pDC->SetBkColor(m_crBackground);
								hbr = (HBRUSH)m_hBackBrush;
							}
						}
					}
				}
				break;
		}
	}

	return hbr;
}

/**
\brief
	Método chamado quando o background de uma janela precisa ser apagado
\details
	Este método redesenha o background da janela
\param 
	CDC* pDC: Ponteiro para o contexto de visualização da janela que está sendo alterada
\return
	TRUE, sempre
*/
BOOL CDialogEx::OnEraseBkgnd(CDC* pDC)
{
	CRect rect;
	GetClientRect(&rect);
	pDC->FillSolidRect(rect, m_crBackground);
	return TRUE;
}

/**
\brief
	Pinta o fundo de uma janela
\param
	COLORREF cr: Contém a cor que a janela irá receber
\return
	void
*/
void CDialogEx::SetBackColor(COLORREF cr) 
{
	m_bUseBackColor = TRUE;
	m_crBackground = cr; 

	if(m_hBackBrush.GetSafeHandle())
		m_hBackBrush.DeleteObject();

	m_hBackBrush.CreateSolidBrush(m_crBackground); 
}

/**
\brief
	Retorna a altura do teclado virtual
\param
	void
\return
	int: Altura do teclado virtual
*/
int CDialogEx::GetSIPHeight()
{
#ifdef _WIN32_WCE
	SIPINFO si = {0};
	si.cbSize = sizeof(si);
	SHSipInfo(SPI_GETSIPINFO, 0, &si, 0);
	return si.rcVisibleDesktop.bottom - si.rcVisibleDesktop.top;
#else
	return 0;
#endif
}

/**
\brief
	Início desta janela.
\details
	Funções executadas neste módulo:
	- Modificar o estilo da janela para adequar aos padrões do aplicativo

\return 
	TRUE se a execução ocorrer com sucesso
*/
BOOL CDialogEx::OnInitDialog()
{
#ifdef _WIN32_WCE
	m_bFullScreen = FALSE;
#endif

	CDialog::OnInitDialog();

#ifdef _WIN32_WCE
	SHINITDLGINFO shidi;
	shidi.hDlg    = m_hWnd;
	shidi.dwMask  = SHIDIM_FLAGS;
	shidi.dwFlags = SHIDIF_SIZEDLGFULLSCREEN;
	::SHInitDialog(&shidi);

	::SHDoneButton(m_hWnd, SHDB_HIDE);
	ModifyStyle(0, WS_NONAVDONEBUTTON, SWP_NOSIZE);
#endif

	return TRUE;
}

/**
\brief
	Método executado quando uma janela for ativada
\details
	Funções executadas neste método:
	- Colocar a janela em tela cheia
*/
void CDialogEx::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
#ifdef _WIN32_WCE	
	CDialog::OnActivate(nState, pWndOther, bMinimized);
	BOOL bSIPVisible = FALSE;
	
	switch(nState)
	{
		case WA_ACTIVE:
		case WA_CLICKACTIVE:
			_FullScreen();
			break;
	};
#endif
}

/**
\brief
	Método chamado quando é mudado algum parâmetro de sistema na janela
\details
	Funções executadas por este método:
	- Manter a janela no lugar ao chamar/esconder o teclado virtual

\param
	UINT uFlags: Parâmetro do sistema a ser mudado
	LPCTSTR lpszSection: Nome da sessão a ser modificada
\return
	void
*/
void CDialogEx::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CDialog::OnSettingChange(uFlags, lpszSection);

#ifdef _WIN32_WCE
	BOOL bSIPVisible = FALSE;

	SIPINFO si = {0};
	si.cbSize = sizeof(si);
	SHSipInfo(SPI_GETSIPINFO, 0, &si, 0);
	bSIPVisible = si.fdwFlags & SIPF_ON;

	// Se houver mudancas no SIP, vamos setar Fullscreen novamente...
	if(/*!m_bSipVisible && */bSIPVisible)
	{
		m_bSipVisible = TRUE;

		// Descontar o tamanho do SIP...
		CRect rect;
		if(SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, NULL) == 1)
			MoveWindow(CRect(0, 0, rect.Width(), rect.Height()- 80), TRUE);

		SHFullScreen(m_hWnd, SHFS_HIDESTARTICON|SHFS_HIDETASKBAR);
	}
	else/* if(m_bSipVisible && !bSIPVisible)*/
	{
		// Fullscreen novamente...
		m_bSipVisible = FALSE;
		_FullScreen();
	}
#endif
}

/**
\brief
	Processa qualquer mensagem da janela
\details
	Funções executadas neste método:
	- Iniciar a pintura um controle Static, neste caso um GROUPBOX
\param
	UINT message: Mensagem a ser processada
\param
	WPARAM wParam: Handle para o contexto de visualização (DC) do controle que está recebendo a mensagem
\param
	LPARAM lParam: Handle para a janela que está recebendo a mensagem
\return
	LRESULT: Resultado do processo da mensagem
*/
LRESULT CDialogEx::DefWindowProc( UINT message, WPARAM wParam, LPARAM lParam )
{
	switch(message)
	{
		//
		// Para o GROUPBOX ser pintado corretamente, 
		// use um id diferente de ID_STATIC
		//
		case WM_CTLCOLORSTATIC:
		{
			if(IsWindowVisible())
			{
				LRESULT lRes = 0;
				if((lRes = _ChangeControlColor(
							::GetDlgCtrlID((HWND)lParam), (HDC) wParam)) != -1)
				{
					return lRes;
				}
			}

			break;
		}
	}

	return CDialog::DefWindowProc(message, wParam, lParam);
}

/**
\brief
	Pinta o texto de um GROUPBOX
\param
	UINT nID: ID do groupbox que será pintado
\param
	HDC hdc: Handle para o contexto de visualização (DC) do controle que está recebendo a mensagem
\return
	LRESULT: Resultado do processo da mensagem
*/
LRESULT CDialogEx::_ChangeControlColor(UINT nID, HDC hdc)
{
	if(nID > 0)
	{
		CWnd *pWnd = GetDlgItem(nID);
		if(pWnd != NULL)
		{
			CDC *pDC = CDC::FromHandle(hdc);

			TCHAR szClassName[128];
			::GetClassName(pWnd->GetSafeHwnd(), szClassName, 127);
			CString s(szClassName);

			//
			// Para o GROUPBOX ser pintado corretamente, use
			// um id diferente de ID_STATIC
			//
			if(s.CompareNoCase(_T("BUTTON")) == 0)
			{
				if((pWnd->GetStyle() & BS_GROUPBOX) == BS_GROUPBOX)
					pDC->SetTextColor(RGB(255,0,0));
			}
			return 0;
		}
	}

	return -1;
}

/**
\brief
	Mostra o painel de entrada de dados (teclado virtual)
\param
	void
\return 
	void
*/
void CDialogEx::ShowSIP()
{
#ifdef _WIN32_WCE
	m_bSipVisible = TRUE;
	SHSipPreference(GetSafeHwnd(), SIP_UP);
#endif
}

/**
\brief
	Esconde o painel de entrada de dados (teclado virtual)
\param
	void
\return 
	void
*/
void CDialogEx::HideSIP()
{
#ifdef _WIN32_WCE
	m_bSipVisible = FALSE;
	SHSipPreference(GetSafeHwnd(), SIP_FORCEDOWN);
#endif
}

/**
\brief
	Redimensiona a janela para ocupar toda a tela do equipamento, descontando a altura do teclado virtual
\param
	void
\return
	void
*/
void CDialogEx::_FullScreen1()
{
#ifdef _WIN32_WCE
	// Descontar o tamanho do SIP...
	CRect rect;
	if(SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, NULL) == 1)
		MoveWindow(CRect(0, 0, rect.Width(), rect.Height()- 80), TRUE);

	HWND win = ::FindWindow(_T("HHTaskBar"), _T(""));
	::EnableWindow(win, FALSE);
	::ShowWindow(win,SW_HIDE);

	SHFullScreen(m_hWnd, SHFS_HIDESTARTICON|SHFS_HIDETASKBAR);
#endif
}

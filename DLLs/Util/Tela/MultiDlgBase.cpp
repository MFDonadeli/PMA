#include "StdAfx.h"

#include "MultiDlgBase.h"
#include "TabSheetBase.h"
#include "Utils.h"
#include "BannerWindow.h"


#define ID_SCROLLBAR	0x3441
#define SCROLL_WIDTH	15
#define WM_CHANGEPAGE	(WM_USER + 101)

IMPLEMENT_DYNAMIC(CMultiDlgBase, CDialogEx);

CString CMultiDlgBase::m_paramTrdBtn;

/**
\brief Construtor da classe
\details
	Funções executadas neste método
	- Início de variáveis globais.

\param 
	UINT nResID: ID do form que será o pai de todos os outros forms menores (páginas)
	UINT nBmpBannerID: ID do bitmap que será o banner (parte superior) da tela
	UINT nIDBtnUp: ID do bitmap que será o ícone do botão de retorno à pagina anterior
	UINT nIDBtnDn: ID do bitmap que será o ícone do botão de retorno à próxima página
	CWnd* pParent: Ponteiro para a janela que será pai desta
*/
CMultiDlgBase::CMultiDlgBase(UINT nResID, UINT nBmpBannerID, UINT nButtons[], CWnd* pParent/* = NULL*/, BOOL bCreatePanel/* = TRUE*/)
	: CDialogEx(nResID, pParent)
{
	m_pCurSheet      = NULL;
	m_nTop		     = 0;
	m_nBmpBannerID   = nBmpBannerID;
	m_nSelectedIndex = 0;
	m_nButtons		 = nButtons;
	m_bCreatePanel   = bCreatePanel;

#ifndef _WIN32_WCE
	m_Rect_X = 0;
	m_Rect_Y = 0;
#endif

#ifndef _NOSCROLL_
	m_bScrollVisible = FALSE;
#endif

IsSIPVisibleWhenNavigating = TRUE;

//debug only
//m_bDisableCtrlColor = TRUE;
//SetBackColor(RGB(222,222,222));
}

/**
\brief Destrutor da classe
*/
CMultiDlgBase::~CMultiDlgBase(void)
{
}

/**
\brief Configura as trocas e validações dos campos desta janela (União de controles e variáveis)
\param CDataExchange* pDx: Ponteiro para a classe que faz essa troca
\return void
*/
void CMultiDlgBase::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CMultiDlgBase, CDialogEx)
#ifndef _NOSCROLL_
	ON_WM_VSCROLL()
#endif
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_MESSAGE(WM_CHANGEPAGE, OnChangePage)
#ifdef _WIN32_WCE
	ON_REGISTERED_MESSAGE(CPanelButton::WM_PANELBUTTON_MSG, OnPanelMessage)
#endif
	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()

/**
\brief Início desta janela.
\details
	Funções executadas neste módulo:
	- Configurar a janela para se adequar o padrão do PMA:
		- Configuração da barra de comando;
		- Criar o banner da parte superior;
		- Criar o painel de título;
		- Configurar tela cheia.
	Ao finalizar as configurações envia uma mensagem ao pai indicando fim da execução.

\return TRUE se a execução ocorrer com sucesso
*/
BOOL CMultiDlgBase::OnInitDialog() 
{
	CDialogEx::OnInitDialog();

	// Criar o tab em tempo de execucao...
	CRect r;
	GetClientRect(&r);

#ifdef _WIN32_WCE
	// Fazer com que o SIP fique disponivel...
	if(!m_dlgCommandBar.Create(this))
	{
		ASSERT(0);
		//STLOG_WRITE("CMultiDlgBase::OnInitDialog(): Failed to create CommandBar");
		return FALSE;      // fail to create
	}

	/*** Deprecated: Banner antigo ***/ 
	//_CreateBanner(m_nBmpBannerID, FALSE, 28);	
	//m_banner.ShowWindow(SW_SHOW);

	// Cria o panel de titulo...
	if(m_bCreatePanel)
	{
		VERIFY(m_panel.Create(this, m_nButtons, CRect(0, 0, 0, 0), 0x5678));
		m_panel.SetBkColor(RGB(255, 255, 255));
		//Trata exibição do botão da câmera 
		m_panel.ShowCameraBtn(m_bFlgCamera);
		m_panel.ShowWindow(SW_SHOW);	
	}

	
	

#ifndef _NOSCROLL_
	VERIFY(m_scroll.Create(WS_CHILD|WS_VISIBLE|SBS_VERT,
						   CRect(0, 0, 0, 0), 
						   this, 
						   ID_SCROLLBAR));

	m_bScrollVisible = TRUE;
#endif

	_FullScreen();		
#endif
	
	if(GetParent() != NULL)
		GetParent()->PostMessage(CModParam::WM_MODULE_READY, 0, (LPARAM) GetSafeHwnd());

	return TRUE;
}

/**
\brief 
	Fim da janela
\details 
	Destrói todas as janelas criadas como abas
\param 
	void
\return 
	void
*/
void CMultiDlgBase::OnDestroy() 
{
	for(int i = 0; i < m_sheets.GetSize(); i++)
	{
		__TABINFO *pInfo = m_sheets.GetAt(i);
		if(pInfo && pInfo->m_pSheet)
		{
			pInfo->m_pSheet->DestroyWindow();
			delete pInfo->m_pSheet;
			delete pInfo;
		}
	}
	
	m_sheets.RemoveAll();

	CDialogEx::OnDestroy();
}

/**
\brief 
	Insere um novo form a ser exibido como uma página
\details 
	Este método inicia a configuração da página
\param 
	LPCTSTR szCaption: Título desta página a ser exibido no painel de titulo
	UINT nDlgResID: ID do dialog a ser exibido
	UINT nMenuResID: ID do menu a ser exibido para esta página
	CTabSheetBase *pSheet: Ponteiro para a nova janela a ser criada
\return
	TRUE se a execução ocorrer com sucesso. FALSE se o ponteiro para a janela não existir ou ocorrer erro de configuração
*/
BOOL CMultiDlgBase::InsertPage(LPCTSTR szCaption, UINT nDlgResID, UINT nMenuResID, CTabSheetBase *pSheet)
{
	if(pSheet == NULL)
		return FALSE;

	if(_SetupItem(pSheet, nDlgResID, nMenuResID, szCaption) == NULL)
		return FALSE;
#ifndef _NOSCROLL_
	m_scroll.SetScrollRange(0, m_sheets.GetCount()-1);
#endif

	return TRUE;
}

/**
\brief 
	Faz a configuração da página
\details
	Funções executadas neste módulo:
	- Criar a janela a partir do ID dado e colocar esta classe como pai;
	- Posicionar a janela no seu local;
	- Preencher a classe de informações da aba;
	- Adicionar esta janela no array de janelas.

\param 
	CTabSheetBase *pDlg: Ponteiro para a janela a ser configurada
	UINT nDlgResID: ID do dialog a ser exibido
	UINT nMenuResID: ID do menu a exibido
	LPCTSTR szCaption: Título da página a ser exibido no painel de título
\return 
	CMultiDlgBase::__TABINFO*: Ponteiro para as informações da aba que representa a página
*/
CMultiDlgBase::__TABINFO *CMultiDlgBase::_SetupItem(CTabSheetBase *pDlg, UINT nDlgResID, UINT nMenuResID, LPCTSTR szCaption)
{
	__TABINFO *pInfo = new __TABINFO;

	pInfo->m_pSheet = pDlg;
	int nRes = pDlg->Create(nDlgResID, this);
	
	CRect rect;

#ifdef _WIN32_WCE
	GetClientRect(&rect);
	rect.bottom -= DRA::SCALEY(21);

#ifndef _NOSCROLL_
	if(m_bScrollVisible)
		rect.right -= SCROLL_WIDTH;
#endif

	pDlg->SetWindowPos(&wndTop, 
					   m_nTop, 
					   15, //0
					   rect.Width(), 
					   rect.Height() - m_nTop, 
					   SWP_HIDEWINDOW);
#else
	rect = _GetNextRect();

	//ScreenToClient(rect);

	if(!pDlg->IsFromRcGen())
	{
		pDlg->SetWindowPos(&wndTop,
						rect.left,
						rect.top,
						rect.Width(),
						rect.Height(), SWP_SHOWWINDOW);
	}

	pDlg->ShowWindow(SW_SHOW);
#endif


	pInfo->m_sName      = szCaption;

	pInfo->m_nMenuResID = nMenuResID;
	pInfo->m_bShow = TRUE;

	m_sheets.Add(pInfo);

	return pInfo;
}

///Em Win32, pega o próximo retângulo para posicionar a janela.
CRect CMultiDlgBase::_GetNextRect()
{
#ifndef _WIN32_WCE
	const int tam_x = 250;
	const int tam_y = 330;

	CRect r(m_Rect_X, m_Rect_Y, m_Rect_X+240, m_Rect_Y+320);

	if(m_Rect_X > tam_x * 3)
	{
		m_Rect_X = 0;
		m_Rect_Y += tam_y;
	}
	else
	{
		m_Rect_X += tam_x;
	}

	return r;
#else
	return CRect(0,0,0,0);
#endif
}

/**
\brief 
	Seleciona uma pagina e exibe
\details
	Funções executadas neste módulo:
	- Trazer as informações da aba a ser exibida;
	- Fazer algum pré-processamento da janela;
	- Configurar a janela para ser exibida corretamente;
	- Exibir a janela;
	- Posicionar o cursor no primeiro controle da tela (Isto somente será feito se bValidate for FALSE)

\param 
	int idx: Indice da pagina a ser exibida
	BOOL bValidate: Valor padrão FALSE. Indica se é para ocorrer a validação dos dados da página atual antes da exibição da próxima.
\return 
	void
*/
void CMultiDlgBase::SelectPage(int idx, BOOL bValidate)
{
#ifdef _WIN32_WCE
	// Se estourar...
	if(idx >= m_sheets.GetSize())
		idx = 0;

	__TABINFO *pItem = m_sheets.GetAt(idx);
	if(pItem)
	{
		if(!pItem->m_bShow)
		{
			BOOL b;
			b = idx<m_nSelectedIndex;
			m_nSelectedIndex = idx;

			if(b)
				Previous();
			else
				Next();

			
			return;
		}
		m_nSelectedIndex = idx;

#ifndef _NOSCROLL_
		m_scroll.SetScrollPos(m_nSelectedIndex);
#endif
		if(pItem->m_pSheet == NULL)
		{
			MessageBox(_T("ERROR PAGE OBJECT IS NULL"), L"Mensagem", MB_ICONERROR|MB_OK);
		}
		else
		{
			if(m_pCurSheet != NULL)
			{
				// Deixa a pagina processar algo antes de esconder...
				m_pCurSheet->ProcessShowWindow(FALSE);
				m_pCurSheet->ShowWindow(SW_HIDE);
			}

			m_pCurSheet = pItem->m_pSheet;
			m_pCurSheet->ShowWindow(SW_SHOW);
			if(IsSIPVisibleWhenNavigating)
				m_pCurSheet->ShowSIP();
			SetDlgCaptionText(pItem->m_sName, RGB(0,0,128));			
			if(!bValidate) m_pCurSheet->SetFirstFieldFocus();

			VERIFY(m_dlgCommandBar.InsertMenuBar(pItem->m_nMenuResID));

			// Dar chance da pagina efetuar algum 
			// procedimento qdo mostramos...
			 m_pCurSheet->ProcessShowWindow(TRUE);
		}

		if(IsSIPVisibleWhenNavigating)
			FullScreen();
	}
	else // ISSO NAO DEVE ACONTECER...
	{
		ASSERT(0);
	}
#endif
}

/**
\brief 
	Processa qualquer comando recebido pela tela padrão
\param 
	WPARAM wParam: Parâmetro da mensagem
	LPARAM lParam: Parâmetro da mensagem
\return
	TRUE se a execução ocorrer com sucesso
*/
BOOL CMultiDlgBase::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	// Se processar... encerrar...
	if(CDialogEx::OnCommand(wParam, lParam))
		return TRUE;

	// Deixar o dialog corrente processar os comandos ...
	if(m_pCurSheet != NULL)
		m_pCurSheet->SendMessage(WM_COMMAND, wParam, lParam);

	return TRUE;
}

/**
\brief Posiciona a janela, de acordo com o tamanho
\param UINT nType: Tipo de posicionamento requerido
\param int cx: Largura da tela do equipamento
\param int cy: Altura da tela do equipamento
\return void
*/
void CMultiDlgBase::OnSize(UINT nType, int cx, int cy) 
{
	CDialogEx::OnSize(nType, cx, cy);
	_OnSize(cx, cy);
}

/**
\brief Posiciona as janelas e os ítens da tela
\details
	Funções executadas neste módulo:
	- Posicionar o painel superior (Que contém o título e os botões)
	- Posicionar a barra de rolagem vertical (se existente)
	- Posicionar as janelas (páginas) no lugar em que elas devem ser exibidas

\param int cx: Largura da tela do equipamento
\param int cy: Altura da tela do equipamento
\return void
*/
void CMultiDlgBase::_OnSize(int cx, int cy) 
{
#ifdef _WIN32_WCE
	BOOL bSIPVisible = FALSE;
	if(m_pCurSheet != NULL)
		bSIPVisible = m_pCurSheet->IsSIPVisible();


	CRect rect;
	CWnd *pWnd = FindWindow(_T("HHTaskBar"),_T(""));
	if(pWnd != NULL)
	{
		pWnd->GetWindowRect(&rect);
	}

	int ph = rect.Height()-1;
	m_nTop = ph;

	if(IsWindow(m_panel.GetSafeHwnd()))
	{
		//STLOG_WRITE(L"CMultiDlgBase::_OnSize(): Janela do painel: 0 [%d] [%d] [%d]", m_nTop, cx, m_nTop + DRA::SCALEY(PANEL_HEIGHT));
		m_panel.MoveWindow(CRect(0, m_nTop, cx, m_nTop + DRA::SCALEY(PANEL_HEIGHT)));
	}

#ifndef _NOSCROLL_
	if(IsWindow(m_scroll.GetSafeHwnd()))
	{
		// Dados da area de trabalho...
		CRect r;
		VERIFY(SystemParametersInfo(SPI_GETWORKAREA, 0, &r, NULL) == 1);

		CRect rectScroll = CRect(cx-SCROLL_WIDTH, 
								 m_nTop + PANEL_HEIGHT - 1,
								 cx,
								 bSIPVisible ? cy : r.Height()); 

		m_scroll.MoveWindow(rectScroll);
	}
#endif

	for(int i = 0; i < m_sheets.GetSize(); i++)
	{
		__TABINFO *pInfo = m_sheets.GetAt(i);
		if(pInfo && pInfo->m_pSheet)
		{
			if(pInfo->m_pSheet->GetSafeHwnd() != NULL)
			{
				CRect r;
				pInfo->m_pSheet->GetWindowRect(&r);
				BOOL bScroll = (r.Height() > cy) ? TRUE : FALSE;
				int x = cx;

#ifndef _NOSCROLL_
				if(m_bScrollVisible)
					x = cx - SCROLL_WIDTH;
#endif
				//STLOG_WRITE(L"CMultiDlgBase::_OnSize(): m_pSheet-SettingWindowPos: 0 [%d] [%d] [%d]", m_nTop+DRA::SCALEY(PANEL_HEIGHT), x, cy - m_nTop);
				pInfo->m_pSheet->SetWindowPos(NULL, 
											  0, 
											  m_nTop + (IsWindow(m_panel.GetSafeHwnd()) ? DRA::SCALEY(PANEL_HEIGHT) : 0), 
											  x, 
											  cy - m_nTop,
											  SWP_NOREPOSITION|SWP_NOZORDER); 
			}
		}
	}
#endif
}

/**
\brief Move para a proxima pagina
\param void
\return void
*/
void CMultiDlgBase::Next()
{
	PreprocessNext();

	if(m_nSelectedIndex+1 <  m_sheets.GetSize())
	{
		CString s;
		
		/*if(!GetCurPage()->Validate(s))
		{
			MessageBox(s, L"Mensagem", MB_ICONINFORMATION|MB_OK);
			FullScreen();
			return;
		}*/

		SelectPage(m_nSelectedIndex+1); 
	}
}

/**
\brief Move para a pagina anterior
\param void
\return void
*/
void CMultiDlgBase::Previous()
{
	if(m_nSelectedIndex-1 >= 0)
	{
		//CString s;
		//if(!GetCurPage()->Validate(s))
		//{
		//	MessageBox(s, L"Mensagem", MB_ICONINFORMATION|MB_OK);
		//	FullScreen();
		//	return;
		//}

		SelectPage(m_nSelectedIndex-1, TRUE);
	}
}

/**
\brief Inicia Camera
\param void
\return void
*/
void CMultiDlgBase::Camera()
{
#ifdef _WIN32_WCE

	DWORD dwErr = 0;
	CString sMod;
	sMod = CUtil::GetAppPath() + L"\\Camera.dll";

	STLOG_WRITE(L"CMultiDlgBase::Camera(): Tentando executar [%s], com parâmetro [%s]", sMod, m_paramTrdBtn);

	if(m_paramTrdBtn.IsEmpty())
		return;
	
///#define _TESTE_EMULADOR  ////****para testes com o emulador ****
#ifdef _TESTE_EMULADOR

	if(!GetParent()->SendMessage(CModParam::WM_MODULE_EXECUTE, (WPARAM) sMod.AllocSysString(), (LPARAM) m_paramTrdBtn.AllocSysString()))
	{
		dwErr = GetLastError();
		STLOG_WRITE(L"Erro [%d] Postando mensagem", dwErr);
	}
	m_panel.EnableCameraBtn(1);
#endif
	//DWORD dwErr = 0;	
	//CString sMod;
	//sMod = CUtil::GetAppPath() + L"\\Camera.dll";

	//STLOG_WRITE(L"CMultiDlgBase::Camera(): Tentando executar [%s], com parâmetro [%s]", sMod, m_paramTrdBtn);

	//if(m_paramTrdBtn.IsEmpty())
	//	return;
	//
	///*if(!GetParent()->SendMessage(CModParam::WM_MODULE_EXECUTE, (WPARAM) sMod.AllocSysString(), (LPARAM) m_paramTrdBtn.AllocSysString()))
	//{
	//	dwErr = GetLastError();
	//	STLOG_WRITE(L"Erro [%d] Postando mensagem", dwErr);
	//}*/

	//CStringArray sCameraClassName;
	//sCameraClassName.Add(L"Camera View");
	//sCameraClassName.Add(L"CAMERAAP");

	//for(int i=0; i<sCameraClassName.GetSize(); i++)
	//{
	//	HWND hWnd = ::FindWindow(sCameraClassName[i], _T("Foto"));
	//	if(NULL != hWnd)
	//	{
	//		// Colocar no foco...
	//		::SetForegroundWindow(hWnd);
	//		m_panel.EnableCameraBtn(1);
	//		return;
	//	}
	//}

#ifndef _TESTE_EMULADOR

	CString sParam;

	///sParam.Format(L"%s_%s.jpg", m_paramTrdBtn,CUtil::GetCurrentTimeStamp());
	sParam.Format(L"%s_", m_paramTrdBtn/*,CUtil::GetCurrentTimeStamp()*/);

	CString sExeFile;
	sExeFile.Format(L"%s\\GerencFoto.exe", CUtil::GetMainAppPath());
	///sExeFile.Format(L"%s\\CameraApp.exe", CUtil::GetMainAppPath());
	///sExeFile.Format(L"%s\\CECamera.exe", CUtil::GetMainAppPath());

	///CUtil::ExecuteExternalProgram(L"\\Program Files\\PMA\\CameraApp.exe", sParam, NULL);
	CUtil::ExecuteExternalProgram(sExeFile, sParam, NULL);


	m_panel.EnableCameraBtn(1);
#endif
#endif
}

/**
\brief Mostra ou esconde a barra de exibição do título da janela
\param BOOL b: Flag que diz se o texto será exibido ou não
\return void
*/
void CMultiDlgBase::ShowDlgCaption(BOOL b)
{
#ifdef _WIN32_WCE
	CRect r;
	GetClientRect(&r);
	
	if(b)
		m_panel.ShowWindow(SW_SHOW);
	else
		m_panel.ShowWindow(SW_HIDE);

	_OnSize(r.Width(), r.Height());
#endif
}

/**
\brief Indica o texto da janela a ser exibida
\param BOOL b: Flag que diz se o texto será exibido ou não
\return void
*/
void CMultiDlgBase::SetDlgCaptionText(LPCTSTR szText, COLORREF color)
{
#ifdef _WIN32_WCE	
	if(IsWindow(m_panel.GetSafeHwnd()))
	{
		m_panel.SetText(szText);
		m_panel.SetColor(color);
	}
#endif
}

/**
\brief Modifica a página exibida
\details Esta função é disparada no movimento da barra de rolagem vertical, esta função 
		  tem que ser declara para evitar exceptions
\param WPARAM x: Posição inicial da barra de rolagem 
\param LPARAM l: Posição final da barra de rolagem
\return 0, sempre
*/
LRESULT CMultiDlgBase::OnChangePage(WPARAM x, LPARAM l)
{
	int curPos = (int)x;
	int oldPos = (int)l;

	if(curPos != oldPos)
	{
		SelectPage(curPos);
		m_nSelectedIndex = curPos;
	}

	return 0L;
}

#ifndef _NOSCROLL_
/**
\brief Trata o clique na barra de rolagem vertical
\details Ao clicar na barra de rolagem esta função irá indicar o destino do cursor, ou seja, qual tela irá ser exibida
\param UINT nSBCode: Indica o tipo de movimento da barra de rolagem
\param UINT nPos: Posição final ao rolar a barra de rolagem
\param CScrollBar *pScrollBar: Barra de rolagem que recebeu o evento
*/
void CMultiDlgBase::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar)
{
	ASSERT(pScrollBar == &m_scroll);

	SCROLLINFO info;
	pScrollBar->GetScrollInfo(&info, SIF_ALL);
	int curPos = info.nPos;

	if(info.nPage == 0) { info.nPage = 1; }

	switch (nSBCode)
	{
		case SB_LEFT:		// Scroll to far left.
			curPos = 0;
			break;

		case SB_RIGHT:      // Scroll to far right.
			curPos = info.nMax;
			break;

		case SB_ENDSCROLL:  // End scroll.
			break;

		case SB_LINELEFT:   // Scroll left.
			if(curPos > info.nMin)
				curPos--;
			break;

		case SB_LINERIGHT:  // Scroll right.
			if(curPos < info.nMax)
				curPos++;
			break;

		case SB_PAGELEFT:   // Scroll one page left.
   			if(curPos > info.nMin)
				curPos = max(info.nMin, curPos - (int)info.nPage);
			break;

		case SB_PAGERIGHT:  // Scroll one page right
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

	// Se chamar metodos de show/hide aqui dentro, dah exception...
	PostMessage(WM_CHANGEPAGE, curPos, m_nSelectedIndex);

	pScrollBar->SetScrollPos(curPos);
	CDialogEx::OnVScroll(nSBCode, nPos, pScrollBar);
}
#endif

/**
\brief Retorna uma janela qualquer a partir do seu número na lista
\param int idx: Número da janela a ser retornada
\return CTabSheetBase *: Ponteiro que indica a janela retornada
*/
CTabSheetBase *CMultiDlgBase::GetPage(int idx)
{
	if(idx >= 0 && idx < m_sheets.GetCount())
		return m_sheets.GetAt(idx)->m_pSheet;

	return NULL;
}

/**
\brief Retorna a janela atual
\param void
\return CTabSheetBase *: Ponteiro que indica a janela retornada
*/
CTabSheetBase *CMultiDlgBase::GetCurPage()
{
	return m_pCurSheet;
}

/**
\brief Inicia a validação de todas as janelas filhas desta classe
\details Se ocorrer algum erro será exibida uma mensagem e será exibida a janela em que o erro ocorreu
\return void
*/
void CMultiDlgBase::Validate()
{
	for(int i = 0; i < m_sheets.GetSize(); i++)
	{
		__TABINFO *pInfo = m_sheets.GetAt(i);
		if(pInfo && pInfo->m_pSheet)
		{
			CString s;
			if(!pInfo->m_pSheet->Validate(s))
			{
				MessageBox(s, L"Mensagem", MB_ICONINFORMATION|MB_OK);
				FullScreen();
				SelectPage(i);
				break;
			}
		}
	}
}

/**
\brief Atualiza título do banner
\param LPCTSTR szTitle: Texto a ser exibido
\return void
*/
void CMultiDlgBase::SetBannerTitle(LPCTSTR szTitle)
{
#ifdef _WIN32_WCE
	/*** Deprecated: Banner antigo ***/
	//m_banner.SetText(szTitle);
	_CreateBanner(szTitle);
#endif
}

/**
\brief Captura uma mensagem a partir de um comando no painel superior
\details Esta função indica qual um clique no botão avançar ou retornar
\param WPARAM w: Qual botão foi clicado
\param LPARAM l: sem uso
\return 0, sempre.
*/
LRESULT CMultiDlgBase::OnPanelMessage(WPARAM w, LPARAM l)
{
	if(w == 0) // Up
		Previous();
	else if(w == 1) // Down
		Next();
	else if(w == 2) // Cam
	{
		HideSIP();
		Camera();
	}

	_UpdateButtons();

	return 0L;
}

/**
\brief Indica se é a primeira página (primeiro form exibido)
\param void
\return TRUE se verdadeiro
*/
BOOL CMultiDlgBase::IsFirstPage()
{
	return (m_nSelectedIndex == 0); 
}

/**
\brief Indica se é a última página (último form exibido)
\param void
\return TRUE se verdadeiro
*/
BOOL CMultiDlgBase::IsLastPage()
{
	return (m_nSelectedIndex >= m_sheets.GetSize()-1);
}

/**
\brief Modifica os estilos dos botões de avançar/retornar para que sejam travados quando for a primeira/última página
\param void
\return void
*/
void CMultiDlgBase::_UpdateButtons()
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
	m_panel.Enable(0, !(nStyle & TBBS_DISABLED));

	// Recuperar o style do botao 'proximo'
	nStyle = m_dlgCommandBar.GetButtonStyle(1);

	// Se estamos no root, desabilitar o botao...
	if(IsLastPage())
		nStyle |= TBBS_DISABLED; 
	else
		nStyle &= ~TBBS_DISABLED; 

	// Setar o style novamente...
	m_dlgCommandBar.SetButtonStyle(1, nStyle);
	m_panel.Enable(1, !(nStyle & TBBS_DISABLED));
#endif
}

/**
\brief Função que indica se a página será exibida ou não
\param int iPage: Número da página que receberá o status
\param BOOL status: Indica se a página será ou não exibida
\return void
*/
void CMultiDlgBase::SetPageVisibility(int iPage, BOOL status)
{
	__TABINFO *pInfo = m_sheets.GetAt(iPage);
	if(pInfo && pInfo->m_pSheet)
	{
		pInfo->m_bShow = status;
		m_sheets.SetAt(iPage, pInfo);
	}
}

/**
\brief Função que mostra se a página é exibida ou não
\param int iPage: Número da página a ser consultada
\return TRUE se invisível
*/
BOOL CMultiDlgBase::CheckVisibility(int iPage)
{
	__TABINFO *pInfo = m_sheets.GetAt(iPage);
	if(pInfo && pInfo->m_pSheet)
	{
		return pInfo->m_bShow;
	}
}
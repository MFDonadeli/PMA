// Copyright (c) 2007 Marcos Mori de Siqueira. All Rights Reserved. 
// mori@softfactory.com.br
// Classe de dialog principal, gerencia a lista e os menus disparando
// os processos como DLL's
#include "stdafx.h"
#include "PMA.h"
#include "PMADlg.h"
#include "utils.h"
#include "ModParam.h"
#include "Module.h"
#include "InitInfo.h"
#include "Consultas.h"
#include "AboutBoxDlg.h"
#include "ProxyTable.h"
#include "Process_TEM.h"
#include "VersaoApp.h"
#ifdef _WIN32_WCE
	#include "gx.h"
	#include "hook.h"
#endif


//#include <connmgr_status.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define PANEL_HEIGHT	18
#define BATERY_HEIGHT	18

//#define CONNMGR_STATUS_CHANGE_NOTIFICATION_MSG TEXT("CONNMGR_STATUS_CHANGE_NOTIFICATION_MSG") 

CMsgWindow* CPMADlg::m_waitDlg;
CSplash CPMADlg::m_pSplash;
HHOOK CPMADlg::m_hInstalledLLKBDhook = NULL;

/**
\brief Construtor da classe
\details
	Funções executadas neste módulo:
	- Inicialização de variáveis globais

\param CWnd* pParent: Ponteiro para a janela que será pai desta
*/
CPMADlg::CPMADlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CPMADlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pParentItem = NULL;
	m_bLoaded = FALSE;
	m_nModuleID = 0;
	m_bRestartPrinter = FALSE;
	m_bSkipBTAlert = FALSE;
	m_isModuleRunning = FALSE;
	CUtil::m_bAutenticando = FALSE;
	//CONNMGR_CHANGE = RegisterWindowMessage(TEXT("CONNMGR_STATUS_CHANGE_NOTIFICATION_MSG"));

}

/**
\brief Configura as trocas e validações dos campos desta janela (União de controles e variáveis)
\param CDataExchange* pDx: Ponteiro para a classe que faz essa troca
\return void
*/
void CPMADlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_listCtrl);	
}

BEGIN_MESSAGE_MAP(CPMADlg, CDialogEx)
	ON_WM_SIZE()
	ON_COMMAND(ID_VOLTAR, &CPMADlg::OnVoltar)
	//ON_NOTIFY(NM_DBLCLK, IDC_LIST1, &CPMADlg::OnDblClick)
	ON_NOTIFY(NM_CLICK,  IDC_LIST1, OnClick)
	ON_WM_INITMENUPOPUP()
	ON_WM_TIMER()
	ON_COMMAND_RANGE(CModuleInfo::ID_START, CModuleInfo::ID_END, OnMenuOption)

	ON_COMMAND(CModuleInfo::ID_END + 1, OnAbout)

	// Os modulos mandam esta mensagem ao final do carregamento...
	ON_REGISTERED_MESSAGE(CModParam::WM_MODULE_READY, OnModuleReady)

	ON_REGISTERED_MESSAGE(CModParam::WM_CHANGE_PROXY, OnChangeProxy)
#ifdef _WIN32_WCE
	// O monitor de bateria envia mensagens qdo o nivel chegar ao determinado...
	ON_REGISTERED_MESSAGE(CPowerMonCtrl::WM_BATMON_ALERT, OnPowerAlert)

	// Captura os hotkeys...
	ON_MESSAGE(WM_HOTKEY, OnHotKey)
	ON_REGISTERED_MESSAGE(CHotKeyMgr::WM_HKSHOWWINDOW, OnHotKeyShowWindow)

	ON_REGISTERED_MESSAGE(CBTManager::WM_BTMON_ALERT, OnBlueToothAlert)
#endif
	
	// Plugins sinalizam o pedido de backup...
	ON_REGISTERED_MESSAGE(CModParam::WM_BACKUP_REQUEST, OnBackupRequest)

	// Realiza logout
	ON_REGISTERED_MESSAGE(CModParam::WM_RETORNO_LOGIN, OnLogout)

	//Executa um módulo
	ON_REGISTERED_MESSAGE(CModParam::WM_MODULE_EXECUTE, OnModuleExecute)

	//Atualiza banner
	ON_REGISTERED_MESSAGE(CModParam::WM_UPDATE_BANNER, OnTextBanner)

	ON_REGISTERED_MESSAGE(CModParam::WM_SINCRO_BACKUP, OnSincroBackup)

	//ON_COMMAND(CONNMGR_STATUS_CHANGE_NOTIFICATION_MSG , OnConnMgrNotify)
	ON_WM_COPYDATA()
END_MESSAGE_MAP()

/**
\brief Início desta janela.
\details	Funções executadas neste módulo:
	- Configurar a janela para se adequar o padrão do PMA:
		- Configuração da barra de comando;
		- Criar o banner da parte superior;
		- Configurar tela cheia;
	- Iniciar a lista de imagens inicial;
	- Desenhar o logotipo do cliente no topo;
	- Criar a indicação de bateria;
	- Iniciar a configuração dos botões de atalho do equipamento;
	- Criar o timer.

\param void
\return TRUE se a execução ocorrer com sucesso
*/
BOOL CPMADlg::OnInitDialog()
{

	CDialogEx::OnInitDialog();

#ifndef _WIN32_WCE
	CRect rect;
	if(SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, NULL))
		MoveWindow(rect.left, rect.top, rect.Width(), rect.Height());
#endif


	_SetupHotKeys();


	SetIcon(m_hIcon, TRUE);		// Set big icon
	SetIcon(m_hIcon, FALSE);	// Set small icon

#ifdef _WIN32_WCE
	HideSIP();

	if (!m_dlgCommandBar.Create(this) ||
	    !m_dlgCommandBar.InsertMenuBar(IDR_MAINFRAME))
	{
		CPMADlg::m_pSplash.Hide();
		AfxMessageBox(IDS_COMMANDBAR_ERROR);
		_FullScreen();
		STLOG_WRITE("CPAMDlg::OnInitDialog() : Failed to create CommandBar");
		return FALSE;      // fail to create
	}
#endif
	
	// Setar o image lista no listctrl...
	m_listCtrl.SetImageList(theApp.GetImageList(), LVSIL_NORMAL);

#ifdef _WIN32_WCE
	m_listCtrl.SetIconSpacing(DRA::SCALEX(75),DRA::SCALEY(80));
#endif

	//_SetListBackground();


	IniciarRegistroLogin_Logout();
	IniciarRegistroAltera_Senha();

	SetTimer(1, 10, NULL);

	CRect r;
	GetClientRect(&r);

	m_logoHeight = 0;
	CString sImgPath = theApp.GetInitInfo()->GetLogoMenuPath();
	if(!sImgPath.IsEmpty())
	{
				
#ifdef _WIN32_WCE		
			m_logo.Create(CRect(0, DRA::SCALEY(BANNER_HEIGHT), DRA::SCALEX(150), DRA::SCALEY(36+BANNER_HEIGHT)), this, 0x878, FALSE);
			m_logo.SetStretch(TRUE);
			if(m_logo.LoadJPG(sImgPath))
			{
				m_logoHeight = DRA::SCALEY(36);
			}
#else
			m_logo.Create(L"", 0, CRect(0, BANNER_HEIGHT, 150, 36+BANNER_HEIGHT), this, 0x878);
			if(m_logo.Load(sImgPath))
			{
				m_logo.Draw();
				m_logo.ShowWindow(SW_SHOW);
				m_logoHeight = 36;
			}
#endif
	}

#ifdef _WIN32_WCE

	// Inicia BT se habilitado no sistema...
	if(theApp.GetInitInfo()->InitBTOnLogin())
	{	
		m_btManager.Start(this);
	}

	// Criação do banner superior...
	bannerWnd = CBannerWindow::getInstance();	

	// Criacao do monitor de bateria...
	int posHoriz = DRA::SCALEY((BANNER_HEIGHT - BATERY_HEIGHT)/2);
	m_pwrCtrl.Create(bannerWnd,
					CRect(r.Width() - DRA::SCALEX(41), 
					posHoriz,
					r.Width() - DRA::SCALEX(5), 
					DRA::SCALEY(BATERY_HEIGHT)+ posHoriz ), 
					0x890);
	
	m_pwrCtrl.Start(30);	

	//Criacao do painel para traçar as linhas acima e abaixo do logotipo
    m_panel.Create(this, CRect(0, m_logoHeight+DRA::SCALEY(BANNER_HEIGHT)+1, r.Width(), m_logoHeight+DRA::SCALEY(BANNER_HEIGHT)+2), 0x565);
    m_panel.SetBkColor(RGB(0,0,0));



	
	RECT m_gpsRect = CRect(r.Width() - DRA::SCALEX(70), 
						   DRA::SCALEY(2) /*+ (m_logoHeight > 0 ? 10 : 0)*/, 
						   r.Width() - DRA::SCALEX(5), 
						   DRA::SCALEY(PANEL_HEIGHT) + /*(m_logoHeight > 0 ? 10 : 0)+*/7);

	RECT m_msgRect = CRect(r.Width() - DRA::SCALEX(100), 
						   DRA::SCALEY(2) /*+ (m_logoHeight > 0 ? 10 : 0)*/, 
						   r.Width() - DRA::SCALEX(75), 
						   DRA::SCALEY(PANEL_HEIGHT) + /*(m_logoHeight > 0 ? 10 : 0)+*/7);

	//Configura área do ícone GPS
	if(!theApp.GetInitInfo()->HasGPS())
	{
		m_gpsManager.HasGPS(FALSE);
		m_InfoSender.HasGPS(FALSE);
	}
	m_gpsManager.SetGPSIconArea(bannerWnd, m_gpsRect, 0x890);
	//m_msgManager.SetMSGIconArea(bannerWnd, m_msgRect, 0x891);
	

#endif

	_FullScreen();

	// Recuperar o path do database e remover o nome do mesmo...
	CString sPathDBWName = theApp.GetInitInfo()->GetDBPath();
	int pos = sPathDBWName.ReverseFind('\\');
	CString sPathDB = sPathDBWName.Mid(0, pos);

	/*HRESULT res = ConnMgrRegisterForStatusChangeNotification(TRUE, this->GetSafeHwnd());
	if(res!=S_OK)
		MessageBox(L"Não deu certo");*/

#if 0
	// BACKUP STUFF...
	// Somente se estiver habilitado...
	if(theApp.GetInitInfo()->IsBackupActive())
	{
		if(CreateBackupThread(this, sPathDB))
		{
			//_DoBackup();
			STLOG_WRITE("CPAMDlg::OnInitDialog() : thread de backup iniciada");
		}
		else
		{
			STLOG_WRITE("CPAMDlg::OnInitDialog() : thread de backup nao iniciada");
			AfxMessageBox(IDS_BACKUPTHREAD_START_ERROR);
			_FullScreen();
		}
	}
#endif

#ifdef _WIN32_WCE
	bannerWnd->_CreateBanner(IDB_MENU_HEADER, FALSE, BANNER_HEIGHT, L"", BANNER_TEXT_LEFT);
	//bannerWnd->UpdateText(L"");
#endif

	/* Este codigo faz a sobrecarga de funções de botões do equipamento como a tecla verde do telefone
	::SendMessage(SHFindMenuBar(m_hWnd), 
        SHCMBM_OVERRIDEKEY, 
        VK_TTALK,
        MAKELPARAM(SHMBOF_NODEFAULT | SHMBOF_NOTIFY, SHMBOF_NODEFAULT | SHMBOF_NOTIFY));*/

	_RunStartup();

	_SetupAutoLogoff();

	return TRUE;
}

/**
\brief Método iniciado quando a janela é redimensionada
\details	Funções executadas neste módulo:
	- Reposicionar a Lista Principal para ocupar a tela inteira

\param UINT nType: Tipo do redimensionamento
\param int cx: Largura
\param int cy: Altura
\return void
*/
void CPMADlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	// Reposicionar o listctrl para ocupar a tela toda...
	if(IsWindow(GetSafeHwnd()))
	{
		BOOL bSIPVisible = FALSE;
		// Dados da area de trabalho...
		CRect r;
		VERIFY(SystemParametersInfo(SPI_GETWORKAREA, 0, &r, NULL) == 1);

#ifdef _WIN32_WCE
		// Dados do SIP...
		SIPINFO si = {0};
		si.cbSize = sizeof(si);
		SHSipInfo(SPI_GETSIPINFO, 0, &si, 0);
		bSIPVisible = si.fdwFlags & SIPF_ON;

		int top = ((m_logoHeight == 0) ? DRA::SCALEY(PANEL_HEIGHT) : m_logoHeight) + 4;
		
		CRect rect = CRect(0, 
				DRA::SCALEY(BANNER_HEIGHT) + top,
						   r.Width(),
						   bSIPVisible ? cy : r.Height()); 
#else
		int top = ((m_logoHeight == 0) ? PANEL_HEIGHT : m_logoHeight) + 4;

		CRect rect = CRect(0, 
				BANNER_HEIGHT + top,
						   r.Width(),
						   r.Height()); 

		//MoveWindow(rect.left, rect.top, rect.Width(), rect.Height());
#endif
			
		m_listCtrl.MoveWindow(rect.left, rect.top, rect.Width(), rect.Height());
	}
}

/**
\brief Método iniciado quando se clica no ícone voltar no menu principal
\details	Funções executadas neste módulo:
	- Recarrega lista anterior.

\return void
*/
void CPMADlg::OnVoltar()
{
	// Se clicou em voltar...
	if(m_pParentItem != NULL)
		_RefreshItems(m_pParentItem);
}

/**
\brief Método iniciado quando é dado um clique na janela do PMA
\details	Funções executadas neste módulo:
	- Processar a ação do clique:
		- Se for em um ítem irá abri-lo,
		- Se for em um folder irá abri-lo e atualizar a lista,
		- Se for em voltar recarregará a lista anterior.

\param NMHDR *pNMHDR: Ponteiro para a estrura que identifica o que recebeu esta ação.
\param LRESULT *pResult: Ponteiro para o resultado da ação.
\return void
*/
void CPMADlg::OnClick(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLISTVIEW *pNMListView = (NMLISTVIEW *)pNMHDR;

	if(pNMListView->iItem < 0)
		return;

	// Clique em um item...
	pItem = (__MenuItem *) m_listCtrl.GetItemData(pNMListView->iItem);
	if(pItem != NULL)
	{
		// Se for folder, vamos atualizar a lista...
		if(pItem->nType == __MenuItem::TYPE_POPUP)
			_RefreshItems(pItem);
		else if(pItem->nType == __MenuItem::TYPE_BACK)
		{
			// Se for voltar... carregar tambem...
			_RefreshItems(pItem->pParent);
		}
		else if(pItem->nType == __MenuItem::TYPE_ITEM)
		{
			// Se for item, vamos ver se eh saida... 
			if(pItem->nID == IDCANCEL)
			{
				if(theApp.GetInitInfo()->HasLogin())
					_OnExit();
				else
					EndDialog(IDOK);
			}
			else if(pItem->nID == IDALIGNSCREEN)
			{
				CUtil::CalibrateScreen();
			}
			else
			{
				if(pItem->nID <= CModuleInfo::ID_END)
				{
					CPMADlg::m_waitDlg->Destroy();
					CPMADlg::m_waitDlg->Create(GetTopWindow());

					if(pItem->sModulePath.MakeUpper().Find(L".EXE") > 0)
						CPMADlg::m_waitDlg->Show(L"Executando Aplicativo");
					else
						CPMADlg::m_waitDlg->Show(L"Carregando Dados");

					// Caso contrario, carregar o modulo e executar...
					m_sModulePath  = pItem->sModulePath;
					m_sModuleParam = pItem->sParameter;
					m_nModuleID    = pItem->nID;

					SetTimer(2, 5, NULL);
				}
				else
				{
					// Se o id for maior que o max... eh interno...
					SendMessage(WM_COMMAND, MAKEWPARAM(pItem->nID, 0), 0);
				}
			}
		}
	}

	*pResult = 0;
}

/**
\brief Desenha e atualiza os ítens da lista e do menu principal do aplicativo.
\param __MenuItem *_pItem: Ponteiro para o ítem do menu que conterá as estruturas dos ítens a serem montados.
\return TRUE se a montagem do menu e da lista ocorrer com sucesso.
*/
///BOOL CPMADlg::_RefreshItems(const __MenuItem *_pItem) ///retirado const para testar insersão de aplicativos externos
BOOL CPMADlg::_RefreshItems( __MenuItem *_pItem)
{
	///CString sUserPerms = CConsultas::GetUserPerm(CUtil::GetLoggedUser());
	CString sUserPerms = CUtil::GetLoggedUserPerms();

#ifdef _WIN32_WCE
	if(!m_isModuleRunning)
		_CreateBanner(_pItem->sTitle);
#endif

	// Setar o redraw pra false, fica mais rapido...
	m_listCtrl.SetRedraw(FALSE);

	// Apagar tudo...
	m_listCtrl.DeleteAllItems();

	// Vamos setar o parent para saber pra onde voltar...
	m_pParentItem = _pItem->pParent;

	CString sTst = _pItem->sText;
	if (sTst.CompareNoCase(L"Aplicativos") == 0)
	{
		__MenuItem *pItem = new __MenuItem();

		pItem = _pItem->m_children.GetTail();
		_pItem->m_children.RemoveAll();

		_LoadConfigAplics(_pItem);
		
		_pItem->m_children.AddTail(pItem);

	}


#ifdef _WIN32_WCE
	// Recuperar o style do botao 'voltar'
	UINT nStyle = m_dlgCommandBar.GetButtonStyle(0);

	// Se estamos no root, desabilitar o botao...
	if(m_pParentItem == NULL)
		nStyle |= TBBS_DISABLED; 
	else
		nStyle &= ~TBBS_DISABLED; 

	// Setar o style novamente...
	m_dlgCommandBar.SetButtonStyle(0, nStyle);
#endif

	int idx;

	// Varrer a lista de filhos para preencher o listctrl...
	POSITION p = _pItem->m_children.GetHeadPosition();
	while(p)
	{
		__MenuItem *pItem = _pItem->m_children.GetNext(p);

		if(pItem->nType == __MenuItem::TYPE_BACK)
		{
			// Se for folder, vamos usar um icone diferente...
			CString s;
			s.LoadString(IDS_BACK);
			idx = m_listCtrl.InsertItem(m_listCtrl.GetItemCount(), s, pItem->nIconIdx);
			m_listCtrl.SetItemData(idx, (DWORD_PTR) pItem);
		}
		else if(pItem->nType == __MenuItem::TYPE_SEPARATOR)
		{
			// Nao inserir separadores...
		}
		else
		{
			BOOL bAdd = TRUE;
			if(pItem->sModulePath.MakeUpper().Find(L".EXE") > 0)
			{
				// Se for executavel, soh adicionar se existir...
				if(!CUtil::FileExists(pItem->sModulePath))
					bAdd = FALSE;
			}

			if(pItem->sModulePath.MakeUpper().Find(L".LNK") > 0)
			{
				// Se for executavel, soh adicionar se existir...
				if(!CUtil::FileExists(pItem->sModulePath))
					bAdd = FALSE;
			}

			//Verifica permissão de usuários
			if(sUserPerms.Find(pItem->sPermissao)==-1)
			{
				bAdd = FALSE;
			}
		
			if(bAdd)
			{
				idx = m_listCtrl.InsertItem(m_listCtrl.GetItemCount(), 
											pItem->sText, 
											pItem->nIconIdx);

				m_listCtrl.SetItemData(idx, (DWORD_PTR) pItem);
			}
		}
	}

	// Depois de prrencher a lista vamos ligar o redraw...
	m_listCtrl.SetRedraw(TRUE);

	return TRUE;
}

/**
\brief Constrói e acessa o menu da tela.
\param CMenu* pPopupMenu: Ponteiro para o menu a ser construído.
\param UINT nIndex: Indice do menu. 
\param BOOL bSysMenu: Indica se é um menu de controle.
\return void
*/
void CPMADlg::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
	// No acesso ao menu, temos a chamada a esta funcao, aproveitamos
	// a primeira para montar o menu baseado no xml...
	if(!m_bLoaded)
	{
		// Apenas uma vez...
		m_bLoaded = TRUE;

		// Remover o dummy...
		pPopupMenu->RemoveMenu(0, MF_BYPOSITION);

		// Construir o menu...
		theApp.GetModuleList()->BuildMenu(pPopupMenu);
	}

	CDialog::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);
}

/**
\brief Sem uso.
\param void
\return void
*/
void CPMADlg::_SetListBackground()
{
#if _WIN32_WCE > 0x420 && defined _WIN32_WCE
	// Setup the list background...
	CBitmap bmp;
	bmp.LoadBitmap(IDB_BACKGROUND);

	if(bmp.GetSafeHandle())
	{
		LVBKIMAGE info;
		info.hbm = (HBITMAP)bmp.GetSafeHandle();
		info.ulFlags = LVBKIF_SOURCE_HBITMAP|LVBKIF_STYLE_TILE|LVBKIF_FLAG_TILEOFFSET;
		info.xOffsetPercent = 0;
		info.yOffsetPercent = 0;

		ListView_SetBkImage(m_listCtrl.GetSafeHwnd(), &info);
	}
#endif
}

/**
\brief Faz a preparação e a execução do login.
\details Esta função tenta iniciar o bluetooth
O módulo de login indica se o sistema deve ou não sair.
\param BOOL &bExit: Flag que indica se o sistema deve se preparar para sair ou não.
\return TRUE se a execução ocorrer com sucesso
*/
BOOL CPMADlg::_DoLogin(BOOL &bExit)
{	
	bExit = FALSE;

	if(CPMADlg::m_pSplash)
		CPMADlg::m_pSplash.Hide();

	if(!theApp.GetInitInfo()->HasLogin())
	{

		CModParam param;

		ConfigurarParametros(&param);
		LONG size = 0;

		// Como o tamanho alocado na memoria vai ser alterado...
		LPBYTE pBufferIO = param.GetBuffer(&size, FALSE);

		//Login ok, ou saida...
		CModParam param1;
		VERIFY(param1.SetBuffer(pBufferIO, size));
		delete pBufferIO;

		
		// Recuperar o codigo do usuario conectado...
		m_sUserCodigo = L"1";

		// Start da thread do GPS...
		#if _WIN32_WCE != 0x420				
			if(theApp.GetInitInfo()->HasGPS())
			{	
				STLOG_WRITE("CPMADlg::_DoLogin(): Uso do GPS habilitado no sistema");							
				IniciarMonitorGPS();
			}
		#endif

		IniciarEnvioInformacoesTalao();
		IniciarRegistroLogin_Logout();
		IniciarBuscaMensagens();

		_FullScreen();

		if(theApp.GetInitInfo()->HasCamera())
		{
			//Se não existir dir de fotos cria
			CString sBaseFotoDir = CUtil::GetMainAppPath() + L"\\Fotos";
			CUtil::CreateDirIfNotExist(sBaseFotoDir);
		}	

		//Inicia thread httpsender no login...
		_StartHttpSenderThreads();
		
		//Inicia BlueTooth no login?
		if(theApp.GetInitInfo()->InitBTOnLogin())
		{
			// Start da thread de monitoramento...
			m_btManager.Run();
		}

		return TRUE;
	}
	else
	{
	#if 0
		if(!theApp.SetupPrinter(this))
		{
			CPMADlg::m_pSplash.Hide();
			STLOG_WRITE("CPMADlg::_DoLogin(): Impressora nao pronta.");
			MessageBox(L"Impressora nao está pronta", L"Mensagem", MB_ICONERROR|MB_OK);
			_FullScreen();
		}

		if(!theApp.m_printer.Init(theApp.m_btCOMMPort))
		{
			CPMADlg::m_pSplash.Hide();
			STLOG_WRITE("CPMADlg::_DoLogin(): Impressora nao pronta.");
			MessageBox(L"Impressora nao está pronta", L"Mensagem", MB_ICONERROR|MB_OK);
			_FullScreen();
		}
	#endif


		if(theApp.GetModuleList()->GetLoginItem()->sModulePath.GetLength() == 0)
		{
			CPMADlg::m_pSplash.Hide();
			STLOG_WRITE("path para o modulo de login invalido.");
			CString msg;
			msg.LoadString(IDS_ERROLOGIN);
			MessageBox(msg, L"Mensagem", MB_ICONERROR|MB_OK);
			_FullScreen();
			return FALSE;
		}

		// Carregar o modulo de login...
		CModule module;
		if(!module.Create(theApp.GetModuleList()->GetLoginItem()->sModulePath))
		{
			CPMADlg::m_pSplash.Hide();
			STLOG_WRITE("Erro carregando módulo de login.");
			CString msg;
			msg.LoadString(IDS_ERROLOGIN);
			MessageBox(msg, L"Mensagem", MB_ICONERROR|MB_OK);
			_FullScreen();
			return FALSE;
		}
		theApp.m_pModuleEmUse = &module;

		CModParam param;
		ConfigurarParametros(&param);
		LONG size = 0;

		// Como o tamanho alocado na memoria vai ser alterado...
		LPBYTE pBufferIO = param.GetBuffer(&size, FALSE);

		if(module.Run(AfxGetInstanceHandle(), 
					   GetSafeHwnd(), 
					   &pBufferIO, 
					   &size))
		{
			//Login ok, ou saida...
			CModParam param1;
			VERIFY(param1.SetBuffer(pBufferIO, size));
			
			param.ReleaseBuffer();
			//delete pBufferIO;
			//pBufferIO = NULL;

			CString s1 = param1.GetValue(_T("sair"));
			if(s1.CompareNoCase(_T("TRUE")) == 0)
			{
				//Encerra thread httpsender do GPS...
				#if _WIN32_WCE != 0x420	&& defined _WIN32_WCE
					//Encerra thread HttpSender GPS
					if(m_GPSThreadSender.IsThreadRunning())
						m_GPSThreadSender.DestroyHttpSenderThread();
				#endif
				
				// Se digitou o codigo secreto...
				bExit = TRUE;
				EndDialog(IDOK);
				return FALSE;
			}
			else
			{
				// Recuperar o codigo do usuario conectado...
				m_sUserCodigo = param1.GetValue(_T("codigo"));

				if(theApp.GetInitInfo()->HasCamera())
				{
					//Se não existir dir de fotos cria
					CString sBaseFotoDir = CUtil::GetMainAppPath() + L"\\Fotos";
					CUtil::CreateDirIfNotExist(sBaseFotoDir);
				}	

				// Se o update foi executado no inicio, nao fazer novamente...
				if(!theApp.m_bAlreadyUpdated &&
				   param1.GetValue(_T("atualizado")).CompareNoCase(L"FALSE") == 0)
				{
					_DoUpdates();
				}
				else
				{
					//deleta logs/arquivos antigos utilizados pelo httpsender
					CPMADlg::m_waitDlg->Show(L"Limpando banco de dados...");
					CConsultas::DeleteOldHttpSenderLogAndFiles(theApp.GetDatabaseRef(), 30);
					CPMADlg::m_waitDlg->Destroy();
					//DoBackup();
					CString sDbPath = theApp.GetInitInfo()->GetDBPath();
					CString sBackupPath = theApp.GetInitInfo()->GetBackupPath();
					OnSincroBackup(1, 0);
					if (!CUtil::doBackup(sDbPath, sBackupPath, FALSE))
					{
						CUtil::m_bBackupOK=FALSE;
						/// problema no backup - não permitir a abertura de novas AITs
						STLOG_WRITE("CPMADlg::_DoLogin(): erro na atualização do backup");
					#if _WIN32_WCE == 0x420
						HideSIP();
						CString msg;
						msg.LoadString(IDS_BACKUPERROR);
						MessageBox(msg, L"Atenção!!!", MB_ICONERROR|MB_OK);
						//return FALSE;
					#endif
					}
					else
					{
						CUtil::m_bBackupOK=TRUE;
					}

				}

				//Inicia thread httpsender no login...
				_StartHttpSenderThreads();
				
				// Start da thread do GPS...
				#if _WIN32_WCE != 0x420	&& defined _WIN32_WCE			
					if(theApp.GetInitInfo()->HasGPS())
					{
						IniciarMonitorGPS();
					}
					
				#endif
				IniciarEnvioInformacoesTalao();
				IniciarRegistroLogin_Logout();
				IniciarBuscaMensagens();
			}
		}
		else
		{
			CPMADlg::m_pSplash.Hide();
			STLOG_WRITE("Erro carregando módulo de login (1).");
			CString msg;
			msg.LoadString(IDS_ERROLOGIN);
			MessageBox(msg, L"Mensagem", MB_ICONERROR|MB_OK);
			_FullScreen();
			return FALSE;
		}

		module.Destroy();
		theApp.m_pModuleEmUse = NULL;

		////Mensagem de data e hora incorreta:
		if( (!theApp.GetInitInfo()->HasGPS() && !CUtil::IsOnline()) || _WIN32_WCE == 0x420 )
		{
			CString sData, msg;
			msg.LoadString(IDS_DATAATUALIZACAO);
			sData.Format(msg, CUtil::GetCurrentDateTime(L"DATA_COMUM"), CUtil::GetCurrentDateTime(L"HORA"));
			
			MessageBox(sData, L"Data/Hora", MB_ICONINFORMATION);
		}

		///////

		m_btManager.Start(this);

		_FullScreen();

		// antes de iniciar o btman, vamos ver se esta ligado, caso nao esteja
		// mostrar o dialog de configuracao, somente para o stack microsoft...
		/*if(!m_btManager.IsWidComm())
		{
			if(!m_btManager.IsTurnedOn())
			{
				CString sPath = theApp.m_init.GetMSBtMgrPath();
				if(!sPath.IsEmpty())
					CUtil::ExecuteExternalProgram(sPath, NULL, this);
				else
				{
					STLOG_WRITE("Path para WirelessManager.exe nao definido no PMA.XML");
					MessageBox(L"caminho para módulo de config. bluetooth não definido no arquivo de configuração.", L"Mensagem", MB_ICONERROR|MB_OK);
					_FullScreen();
				}
			}
		}*/

	#ifdef _WIN32_WCE
		//Inicia BlueTooth no login?
		if(theApp.GetInitInfo()->InitBTOnLogin())
		{
			// Start da thread de monitoramento...
			m_btManager.Run();
		}

		_SetupAutoLogoff();
		return TRUE;
	#endif
	}
}

/**
\brief Método que é executado quando a janela recebe um aviso de estouro de timer.
\details	Funções executadas por este método:
	- Encerra o timer;
	- Se nIDEvent for 1: Desenha a primeira janela, ou não faz nada dependendo da resposta do módulo do Login.
	- Se nIDEvent for 2: Executa um módulo já estabelecido.

\param UINT_PTR nIDEvent: Flag que o id do timer.
\return void
*/
void CPMADlg::OnTimer(UINT_PTR nIDEvent)
{
	static BOOL bFirstTime = TRUE;

	if(nIDEvent == 1)
	{
		KillTimer(1);

		m_gpsManager.VerificaTelaGPS();

		// Tempinho pro splash...
		//if(bFirstTime)
		//	Sleep(1000);

 		CUtil::m_bAutenticando = TRUE;
		BOOL bExit;
		_DoLogin(bExit);
		CUtil::m_bAutenticando = FALSE;

		if(bExit)
			return;
#ifdef _WIN32_WCE
		if(!m_pwrCtrl.IsWindowVisible())
			m_pwrCtrl.ShowWindow(SW_SHOW);
	
		if(!m_gpsManager.IsWindowVisible())
			m_gpsManager.ShowWindow(SW_SHOW);

		/*if(!m_msgManager.IsWindowVisible())
			m_msgManager.ShowWindow(SW_SHOW);*/
		
#endif

		if(bFirstTime)
		{
			bFirstTime = FALSE;
#ifdef _WIN32_WCE			
			if(m_logoHeight > 0)
			{
				m_logo.ShowWindow(SW_SHOW);
				m_panel.ShowWindow(SW_SHOW);
			}
#endif
		}		

		// Iniciar o preenchimento da lista com o root...
		_RefreshItems(theApp.GetModuleList()->GetRootItem());

		// Verificart se tem pendencias...
		// Para pendências de transmissão
		int iTotal = 0;
		/*if(CConsultas::CountPendentesTransmissao(theApp.GetDatabaseRef(), iTotal))
		{
			if(iTotal > 0)
			{
				CString sMsg;
				sMsg.Format(L"Existem %d infrações não transmitidas", iTotal);
				MessageBox(sMsg, L"Mensagem", MB_ICONINFORMATION|MB_OK);
			}
		}*/		
	}
	else if(nIDEvent == 2)
	{
		KillTimer(2);
		// Caso contrario, carregar o modulo e executar...
		_ExecuteModule(m_sModulePath, m_sModuleParam);
	}
	else if(nIDEvent == 3)
	{
		if(theApp.m_pModuleEmUse || CUtil::IsProcessRunning())
			_SetupAutoLogoff();
		else
			_OnExit();
	}
	
	CDialogEx::OnTimer(nIDEvent);
}

/**
\brief Ao invés de fechar esta janela, somente cria o timer.
\param void
\return void
*/
void CPMADlg::_OnExit()
{
	//EndDialog(0);
	//_StopThreads();
	SetTimer(1, 10, NULL);	
}

LRESULT CPMADlg::OnLogout(WPARAM wParam, LPARAM lParam)
{
	_OnExit();
	return 0;
}

LRESULT CPMADlg::OnModuleExecute(WPARAM wParam, LPARAM lParam)
{
	BSTR b = (BSTR) wParam;
	CString sMod(b);
	SysFreeString(b);

	b = (BSTR) lParam;
	CString sParam(b);
	SysFreeString(b);

	_ExecuteModule(sMod, sParam);
	return 0;
}

LRESULT CPMADlg::OnTextBanner(WPARAM wParam, LPARAM lParam)
{
	BSTR b = (BSTR) wParam;
	CString sMod(b);
	SysFreeString(b);

	CBannerWindow::getInstance()->UpdateText(sMod);
	return 0;
}

LRESULT CPMADlg::OnSincroBackup(WPARAM wParam, LPARAM lParam)
{
	if(!CUtil::FileExists(theApp.GetInitInfo()->GetBackupPath()))
	{
		return 0;
	}

	CStringA sQuery;
	
	try
	{

		sQuery.Format("ATTACH DATABASE '%S' AS BACK", theApp.GetInitInfo()->GetBackupPath());
		CppSQLite3DB::getInstance()->execQuery(sQuery);
		
		if(wParam != 2)
		{
			sQuery = "INSERT INTO aiip SELECT * FROM BACK.aiip WHERE BACK.aiip.serie NOT IN (SELECT serie FROM aiip)";
			CppSQLite3DB::getInstance()->execQuery(sQuery);

			sQuery = "INSERT INTO aiip_ad SELECT * FROM BACK.aiip_ad WHERE BACK.aiip_ad.serie NOT IN (SELECT serie FROM aiip_ad)";
			CppSQLite3DB::getInstance()->execQuery(sQuery);

			sQuery = "INSERT INTO aiip_med_adm SELECT * FROM BACK.aiip_med_adm WHERE BACK.aiip_med_adm.serie NOT IN (SELECT serie FROM aiip_med_adm)";
			CppSQLite3DB::getInstance()->execQuery(sQuery);

			sQuery = "INSERT INTO aiip_fotos SELECT * FROM BACK.aiip_fotos WHERE BACK.aiip_fotos.serie NOT IN (SELECT serie FROM aiip_fotos)";
			CppSQLite3DB::getInstance()->execQuery(sQuery);

			sQuery = "DELETE FROM aipte";
			CppSQLite3DB::getInstance()->execQuery(sQuery);

			sQuery = "INSERT INTO aipte SELECT * FROM BACK.aipte";
			CppSQLite3DB::getInstance()->execQuery(sQuery);
		}
		else
		{
			sQuery = "UPDATE aiip SET status = 'C' WHERE serie IN (SELECT a.serie FROM aiip a, BACK.aiip b WHERE a.serie = b.serie AND a.status != b.status)";
			CppSQLite3DB::getInstance()->execQuery(sQuery);
		}

		sQuery = "DETACH DATABASE BACK";
		CppSQLite3DB::getInstance()->execQuery(sQuery);
	}
	catch(CppSQLite3Exception e)
	{
		STLOG_WRITE("%s(%d): Erro executando sincronização. Motivo: %s", __FUNCTION__, __LINE__, e.errorMessage());	
	}

	if(wParam != 1)
		CProcess_TEM::RecreateXMLFiles(FALSE);

}


/**
\brief Ao invés de fechar esta janela, somente cria o timer.
\param void
\return void
*/
void CPMADlg::OnCancel()
{
	//EndDialog(0);
	SetTimer(1, 10, NULL);	
}

/**
\brief Sem uso.
\param void
\return void
*/
void CPMADlg::OnOK()
{
	OutputDebugString(L"Ok do PMADLG\r\n");
	return;
}

/**
\brief Método executado quando se clica em um ítem do menu.
\details	Funções executadas por este método:
	- Prepara a execução de algum programa associado com o ítem escolhido. 
	   A execução é feita pelo Timer com a opção 2.

\param UINT nID: ID do ítem clicado.
\return void
*/
void CPMADlg::OnMenuOption(UINT nID)
{
	__MenuItem *pInfo = theApp.GetModuleList()->GetInfoByMenuID(nID);
	//CString sModule = theApp.GetModuleList()->GetModuleByID(nID);
	CString sModule = pInfo->sModulePath;

	if(!sModule.IsEmpty())
	{
		CPMADlg::m_waitDlg->Destroy();
		CPMADlg::m_waitDlg->Create(GetTopWindow());

		if(sModule.MakeUpper().Find(L".EXE") > 0)
			CPMADlg::m_waitDlg->Show(L"Executando Aplicativo");
		else
			CPMADlg::m_waitDlg->Show(L"Carregando Dados");

		m_sModulePath  = sModule;
		m_sModuleParam = pInfo->sParameter;
		m_nModuleID    = nID;

		SetTimer(2, 5, NULL);
	}
}

/**
\brief Prepara e executa um módulo do PMA.
\param LPCTSTR szModulePath: Caminho do módulo.
\param LPCTSTR szParam: Parâmetros para o módulo.
\return TRUE se a execução ocorrer com sucesso.
*/
BOOL CPMADlg::_ExecuteModule(LPCTSTR szModulePath, LPCTSTR szParam)
{

	// Modulo especial...
	CString ss(szModulePath);
	ss.MakeUpper();
	if(ss.Find(L"ATUALIZACAO.DLL") > 0)
	{
		CPMADlg::m_waitDlg->Hide();
		return _DoUpdates();		
	}

	// Se o modulo for um executavel...
	if(ss.MakeUpper().Find(L".EXE") > 0)
	{
		CPMADlg::m_waitDlg->Hide();
		//_ExecuteExternalProgram(ss, szParam);
		CUtil::ExecuteExternalProgram(ss, szParam, this);
		return TRUE;
	}

	// Se o modulo for um atalho...
	if (ss.MakeUpper().Find(L".LNK") > 0)
	{
		CPMADlg::m_waitDlg->Hide();
		//_ExecuteExternalProgram(ss, szParam);
		CUtil::ExecuteExternalShortCut(ss, szParam, this);
		return TRUE;
	}

	CModule module;	
	if(!module.Create(szModulePath))
	{
		CPMADlg::m_waitDlg->Hide();

		CString s, msg;
		msg.LoadString(IDS_ERROMODULO);
		s.Format(msg, szModulePath);
		MessageBox(s, L"Mensagem", MB_ICONERROR|MB_OK);

		STLOG_WRITE(L"Erro carregando modulo %s.", szModulePath);
		return FALSE;
	}

	theApp.m_pModuleEmUse = &module;

	// Enviar o codigo do usuario para todos os modulos.
	CModParam param;
	param.SetHInstance(AfxGetInstanceHandle());
	param.SetHwnd(CPMADlg::m_waitDlg->GetSafeHwnd());
	param.AddPair(_T("codigo"), m_sUserCodigo);
	STLOG_WRITE(L"%s(%d): Agente logado: %s", __FUNCTION__, __LINE__, m_sUserCodigo);

#if 0
	HANDLE hPrinter = theApp.GetPrinterHandle();
	CString sPrinterHandle;
	sPrinterHandle.Format(L"%08d", hPrinter);
	param.AddPair(_T("printer"), sPrinterHandle);	
#endif

	param.AddPair(_T("printer"), theApp.m_sPrinterAddress);	

	CString sContrato = theApp.m_contratoInfo.sContrato; //theApp.GetInitInfo()->GetContrato();
	ASSERT(!sContrato.IsEmpty());
	param.AddPair(_T("contrato"), sContrato);

	CModuleInfo *pList = theApp.GetModuleList();
	__MenuItem *pItem = pList->CModuleInfo::GetInfoByMenuID(m_nModuleID);
	if(pItem != NULL)
	{
		param.AddPair(_T("param"), pItem->sParameter);
		param.AddPair(_T("text"),  pItem->sText);
	}
	
	if(wcslen(szParam)>0) param.AddPair(_T("param"), szParam);
	


	// Adicionar o path do sd
	//param.AddPair(_T("veiculos_egb"), theApp.m_sVeiculoPath);
	param.AddPair(_T("orgao_autuador"), theApp.GetInitInfo()->GetOrgaoAutuador());
	param.AddPair(_T("atz_online"), theApp.GetInitInfo()->IsOnline() ? L"TRUE" : L"FALSE");

	// Simbolos
	param.AddPair(_T("simbolo_path"), theApp.GetInitInfo()->GetSymbolPath());
	param.AddPair(_T("simbolo_prefix"), theApp.GetInitInfo()->GetSymbolPrefix());

	// Printer
	param.AddPair(_T("printer_name"), theApp.GetInitInfo()->GetPrinterName());

	// Camera
	param.AddPair(_T("flg_camera"), theApp.GetInitInfo()->HasCamera() ? L"TRUE" : L"FALSE");
	param.AddPair(_T("camera_driver"), theApp.GetInitInfo()->GetCametaDriver());

	// Id Talao
	param.AddPair(_T("id_talao"), CUtil::GetIDTalao());

	// Versao do sistema
	param.AddPair(_T("version"), CVersao::GetAppVersion());

	// Qtde mínima de séries
	param.AddPair(_T("series_alocadas"), theApp.GetInitInfo()->GetQtSeries());

	// É estadual
	param.AddPair(_T("estadual"), theApp.GetInitInfo()->IsEstadual() ? L"1" : L"0");

	// Caminho para a Base de Dados
	param.AddPair(_T("db_path"), theApp.GetInitInfo()->GetDBPath());

	// Caminho para o Backup de Dados
	param.AddPair(_T("backup_path"), theApp.GetInitInfo()->GetBackupPath());

	// Caminho do módulo de atualização para eventuais execuções específicas de contrato/sistema
	param.AddPair(_T("upd_path"), theApp.GetModuleList()->GetUpdateItem()->sModulePath);

	// Inserir dados do proxy...
	CString sPort;
	sPort.Format(L"%ld", theApp.m_proxyInfo.nPort);

	param.AddPair(_T("DISCAGEM"), theApp.m_proxyInfo.bDiscagem ? L"TRUE" : L"FALSE");
	param.AddPair(_T("PROXY"),	  theApp.m_proxyInfo.bProxy ? L"TRUE" : L"FALSE");
	param.AddPair(_T("SERVER"),   theApp.m_proxyInfo.sServer);
	param.AddPair(_T("PORT"),	  sPort);
	param.AddPair(_T("USER"),	  theApp.m_proxyInfo.sUser);
	param.AddPair(_T("PASS"),	  theApp.m_proxyInfo.sPass);

	// Adicionar todas as urls...
	theApp.AddURLS(&param);

	// Adicionar todas os files...
	__URLSMAP *pFileMap = theApp.GetInitInfo()->GetFileMapPtr();
	POSITION p1 = pFileMap->GetStartPosition();
	while(p1)
	{
		CString sURL, sKey, sValue;
		pFileMap->GetNextAssoc(p1, sKey, sValue);

		CUtil::GetPathFromVariable(sValue);

		sKey.Append(L"_egb");

		STLOG_WRITE(_T("FILENAMES:"));
		STLOG_WRITE(_T("[%s]->[%s]"), sKey, sValue);

		param.AddPair(sKey, sValue);
	}


	//Add informações das threads HttpSender criadas após o login, para uso exclusivo do sincro
	if(ss.Find(L"SINCRO.DLL") > 0)
	{
		//Add Num de threads HttpSender
		param.AddPair(_T("num_httpsenderthread"), m_sNumHttpSenderThread);		

		//Add nome do obj e descrição da thread HttpSender
		POSITION p = m_HttpSenderItems.GetStartPosition();	
		while(p)
		{				
			CString sThreadObjName, sThreadObjDescr;
			m_HttpSenderItems.GetNextAssoc(p, sThreadObjDescr, sThreadObjName);	
			param.AddPair(sThreadObjDescr.Trim(), sThreadObjName.Trim());	
		}
	}

	LONG size;
	LPBYTE pBuff = param.GetBuffer(&size);

	STLOG_WRITE(L"Executando modulo %s.", szModulePath);

	//Module running...
	m_isModuleRunning = TRUE;

	module.Run(AfxGetInstanceHandle(), 
				GetSafeHwnd(), 
				&pBuff, 
				&size);
	
	//Module running finished...
	m_isModuleRunning = FALSE;	

	_SetupAutoLogoff();
	
	//Refresh banner title...
	#ifdef _WIN32_WCE
		if(!m_isModuleRunning && pItem->pParent != NULL)
			bannerWnd->UpdateText(pItem->pParent->sTitle);
	#endif	

	param.ReleaseBuffer();

/*	if(module.Run1(AfxGetInstanceHandle(), GetSafeHwnd(), theApp.GetDatabaseRef(), &p, &size))
	{
		CModParam param1;
		VERIFY(param1.SetBuffer(p, size));
		delete [] p;

		if(!param1.GetValue(L"module").IsEmpty())
		{
			m_sModulePath = param1.GetValue(L"module");
			SetTimer(2, 5, NULL);
		}
	}*/
	
	// Se o modulo executado foi o de configuracao de proxy...
	if(theApp.GetInitInfo()->GetProxyConfPath().CompareNoCase(szModulePath) == 0)
	{
		// Vamos atualizar as informacoes de proxy...
		CProxyTable proxy;
		proxy.Init();
		proxy.Load(theApp.GetDatabaseRef(), &theApp.m_proxyInfo);

		STLOG_WRITE("Informacoes de proxy atualizadas.");
	}

	module.Destroy();
	theApp.m_pModuleEmUse = NULL;

	_FullScreen1();
	HideSIP();
	_FullScreen();

	return TRUE;
}

/**
\brief Método executado ao fim do carregamento de um módulo (Por mensagem)
\details Ao executar este método, a janela de wait é destruída.
	Este método pode ser usado para execução de alguma tarefa posterior ao carregamento do módulo.

\param WPARAM wParam: Parâmetro
\param LPARAM lParam: Parâmetro. Pode conter alguma informações sobre o que devemos executar agora.
\return 0, sempre.
*/
LRESULT CPMADlg::OnModuleReady(WPARAM wParam, LPARAM lParam)
{
	static bool bFirstTime = true;
	if(bFirstTime)
	{
		CPMADlg::m_pSplash.Destroy();
		bFirstTime = false;
	}

	CPMADlg::m_waitDlg->Destroy();
	return 0L;
}

/**
\brief Método executado a cada troca de informações do proxy
\param WPARAM wParam: Parâmetro
\param LPARAM lParam: Parâmetro.
\return 0, sempre.
*/
LRESULT CPMADlg::OnChangeProxy(WPARAM wParam, LPARAM lParam)
{
	CProxyTable proxy;
	proxy.Init();
	proxy.Load(CppSQLite3DB::getInstance(), &theApp.m_proxyInfo);

	__HTTPSENDERTHREADMAP *pHttpSenderMap = theApp.GetInitInfo()->GetHttpSenderThreadMapPtr();
	POSITION p = pHttpSenderMap->GetStartPosition();	
	
	while(p)
	{
		CString sThreadName, sUrl2Send;
		__HttpSenderThreadItem *pList = NULL;

		pHttpSenderMap->GetNextAssoc(p, sThreadName, pList);	

		if(pList)
		{
			if(pList->objThread->IsThreadRunning())
			{
				pList->objThread->SetProxy(&theApp.m_proxyInfo);

				OutputDebugString(L"Proxy mudado ");
				OutputDebugString(sThreadName);
				OutputDebugString(L"\r\n");
			}
		}

		

		//delete pList;
	}

	//Atualiza relogio a cada envio
	#if _WIN32_WCE != 0x420
	m_GPSThreadSender.UpdateClockAfterSend(&theApp.m_proxyInfo, L"", theApp.m_contratoInfo.sContrato);
	#endif

	//Atualiza relogio a cada envio
	m_RegLoginLogoutThreadSender.UpdateClockAfterSend(&theApp.m_proxyInfo, L"", theApp.m_contratoInfo.sContrato);

	return 0L;
}

void OnConnMgrNotify()
{
//	return 0L;
}

/**
\brief Método executado ao destruir esta janela.
\details	Funções executadas neste método:
	- Destruir das janelas wait e do splash;
	- Parar a execução da thread de gerenciamento do BlueTooth;
	- Habilitar do TASKBAR do Windows Mobile.

\param void
\return TRUE se a execução ocorrer do sucesso.
*/
BOOL CPMADlg::DestroyWindow()
{
#ifdef _WIN32_WCE
	// Parar a thread de backup...
	DestroyBackupThread();


	if(theApp.GetInitInfo()->InitBTOnLogin())
		m_btManager.Stop();

	CWnd *pWnd = FindWindow(_T("HHTaskBar"),_T(""));
	if(pWnd != NULL)
	{
		pWnd->ShowWindow(SW_SHOW);
		pWnd->EnableWindow(TRUE);
	}

	m_pwrCtrl.Stop();	
	CPMADlg::m_pSplash.Destroy();
	CPMADlg::m_waitDlg->Destroy();
	m_pwrCtrl.DestroyWindow();

	#if _WIN32_WCE != 0x420
		if(theApp.GetInitInfo()->HasGPS())
		{
			m_gpsManager.Stop();
			m_gpsManager.DestroyWindow();
		}
	#endif
#endif

	//_StopHttpSenderThread();

	return CDialogEx::DestroyWindow();
}

/**
\brief Método executado quando a bateria estiver baixa. (Por mensagem)
\details	Funções executadas neste método:
	- Toca um som, para indicar bateria baixa.

\param WPARAM wParam: sem uso.
\param LPARAM lParam: sem uso.
\return LRESULT 0, sempre.
*/
LRESULT CPMADlg::OnPowerAlert(WPARAM wParam, LPARAM lParam)
{
#ifdef _WIN32_WCE
	PlaySound(_T("\\windows\\Lowbatt.wav"), NULL, SND_SYNC | SND_FILENAME);
#endif
	return 0L;

}

/**
\brief Sem uso.
\param MSG* pMsg: Sem uso.
\return BOOL
*/
BOOL CPMADlg::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message == WM_LBUTTONDOWN)
	{
		_SetupAutoLogoff();
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

/**
\brief Sem uso.
\param WPARAM wParam: sem uso.
\param LPARAM lParam: sem uso.
\return LRESULT
*/
LRESULT CPMADlg::OnHotKey(WPARAM wParam, LPARAM lParam)
{
#ifdef _WIN32_WCE
	return m_hkMan.OnHotKey(wParam, lParam);
#endif
	return 0L;
}

/**
\brief Faz a reconfigurações dos botões do equipamento.
\param void
\return void
*/
void CPMADlg::_SetupHotKeys()
{
#ifdef _WIN32_WCE
	#if _WIN32_WCE == 0x420
	CInitInfo *pInit = theApp.GetInitInfo();
	m_hkMan.Init(GetSafeHwnd());

	CStringList *pSL = pInit->GetHotKeyPaths();
	POSITION p = pSL->GetHeadPosition();
	while(p)
		m_hkMan.AddHK(pSL->GetNext(p));
	#else
		_HookActivate();
	#endif
#endif
}

/**
\brief Ao pressionar a tecla de atalho do equipamento, esconde ou mostra a janela principal e a do módulo.
\param WPARAM wParam: Valor que define se a janela principal irá ser mostrada ou escondida.
\param LPARAM lParam: sem uso.
\return 0, sempre
*/
LRESULT CPMADlg::OnHotKeyShowWindow(WPARAM wParam, LPARAM lParam)
{
	ShowWindow(wParam == TRUE ? SW_SHOW : SW_HIDE);

	if(theApp.m_pModuleEmUse != NULL)
		theApp.m_pModuleEmUse->ShowWindows(wParam == TRUE);

	return 0L;
}

/**
\brief Chama a função que configura a execução da atualização.
\param void
\return TRUE se a execução ocorrer com sucesso.
*/
BOOL CPMADlg::_DoUpdates()
{
	if(!theApp.GetInitInfo()->HasAtualizacao())
	{
		HideSIP();
		return TRUE;
	}

	if(!theApp._ExecuteUpdate(theApp.GetModuleList()->GetUpdateItem()->sModulePath, 
							  theApp.GetInitInfo()->GetDBPath(),
							  FALSE, GetSafeHwnd()))
	{
		STLOG_WRITE("CPMADlg::_DoUpdates() : Erro executando _ExecuteUpdate()");

		CString msg;
		msg.LoadString(IDS_ERRO_ATUALIZACAO);
		MessageBox(msg, L"Mensagem", MB_ICONERROR|MB_OK);

		return FALSE;
	}

	return TRUE;
}

/**
\brief Mensagem de alerta referente ao status do início do BlueTooth.
\param WPARAM wParam: status do BlueTooth.
\param LPARAM lParam: sem uso
\return 0, sempre.
*/
LRESULT CPMADlg::OnBlueToothAlert(WPARAM wParam, LPARAM lParam)
{
	if(wParam == BT_ERROR)
	{
		CString msg;
		msg.LoadString(IDS_ERRO_BLUETOOTH);
		MessageBox(msg, L"Mensagem", MB_ICONERROR|MB_OK);
	}
	else if(wParam == BT_ON)
	{
		if(m_bRestartPrinter)
		{
			m_bRestartPrinter = FALSE;
#if 0
			if(lParam != 1000)
				theApp.m_printer.Init(theApp.m_btCOMMPort);
			// Informar o modulo atual o novo handle da printer...
			if(m_pModuleEmUse != NULL)
			{
				HANDLE hPrinter = theApp.GetPrinterHandle();
				CString sPrinterHandle;
				sPrinterHandle.Format(L"%08d", hPrinter);
				m_pModuleEmUse->RestartPrinter(sPrinterHandle);
			}
#endif
		}
	}
	else if(wParam == BT_OFF)
	{
		m_bRestartPrinter = TRUE;

		// Se for stack microsoft, nao temos como reinicializar o
		// bluetooth, entao vamos informar o usuario...
		/*
		if(!m_btManager.IsWidComm())
		{
			// Evitar dar mensagens durante a exibicao do Manager...
			if(m_bSkipBTAlert)
				return 0L;

			m_bSkipBTAlert = TRUE;

			// Confirmar se esta no ar...
			if(!m_btManager.IsTurnedOn())
			{
				if(MessageBox(L"O bluetooth está desativado, "
							  L"sem ele você não conseguirá imprimir, "
							  L"Você gostaria de ativá-lo.", 
							  L"Mensagem", 
							  MB_ICONQUESTION|MB_YESNO) == IDYES)
				{
					CString sPath = theApp.m_init.GetMSBtMgrPath();
					if(!sPath.IsEmpty())
						CUtil::ExecuteExternalProgram(sPath, NULL, this);
					else
					{
						STLOG_WRITE("Path para WirelessManager.exe nao definido no PMA.XML");
						MessageBox(L"caminho para módulo de config. bluetooth não definido no arquivo de configuração.", 
								   L"Mensagem", 
								   MB_ICONERROR|MB_OK);

						_FullScreen();
					}
				}
			}

			m_bSkipBTAlert = FALSE;
		} 
		*/
	}

	return 0L;
}

/**
\brief Método executado ao se clicar no ítem Sobre.
\details Funções executadas neste método:
	- Abre a janela Sobre.

\param void
\return void
*/
void CPMADlg::OnAbout()
{
	CAboutBoxDlg dlg(this);
	dlg.m_sContrato = theApp.m_contratoInfo.sContrato; //theApp.GetInitInfo()->GetContrato();
	dlg.m_sVersion  = CVersao::GetAppVersion();
	dlg.DoModal();

	//Refresh banner title...
	#ifdef _WIN32_WCE
		if(!m_isModuleRunning && pItem->pParent != NULL)
			bannerWnd->UpdateText(pItem->pParent->sTitle);
	#endif
}

/**
\brief Faz a execução de um programa externo a aplicação.
\param LPCTSTR szFile: Caminho do programa a ser executado.
\param LPCTSTR szParam: Parâmetros a ser passado para o programa.
\param BOOL bHideTaskbar: Indica se a Barra de Tarefas do Windows Mobile estará liberada para uso.
\return void
*/
void CPMADlg::_ExecuteExternalProgram(LPCTSTR szFile, LPCTSTR szParam, BOOL bHideTaskbar)
{
	CString ss(szFile);

	if(CUtil::FileExists(ss))
	{
		CWnd *pWnd = FindWindow(_T("HHTaskBar"),_T(""));
		pWnd->ShowWindow(SW_SHOW);
		
		if(pWnd != NULL && !bHideTaskbar)
		{
			pWnd->EnableWindow(TRUE);
			pWnd->Invalidate();
			pWnd->UpdateWindow();
		}

		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(STARTUPINFO));
		ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
		si.cb = sizeof(STARTUPINFO);

		if(!CreateProcessW(ss,
#ifdef _WIN32_WCE
					   szParam,
#else
					   (LPWSTR)szParam,
#endif
					   NULL, 
					   NULL, 
					   NULL, 
					   CREATE_NEW_CONSOLE, 
					   NULL, 
					   NULL, 
					   &si, 
					   &pi))
		{
			CString msg;
			msg.LoadString(IDS_ERRO_APP);
			
			MessageBox(msg, L"Mensagem", MB_ICONERROR|MB_OK);
			if(pWnd != NULL && !bHideTaskbar)
				pWnd->ShowWindow(SW_HIDE);

			return;
		}

		// NOTA: Nao podemos esperar o processo encerrar porque muitos
		// ao clicar OK ficam minimizados e nao retornam nunca...

		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
}

/**
\brief Inicia a criação/atualização do backup.
\details Esta função é iniciada por mensagem.
\param WPARAM wParam: Não usado.
\param LPARAM lParam: Não usado.
\return 0, sempre
*/
LRESULT CPMADlg::OnBackupRequest(WPARAM wParam, LPARAM lParam)
{
#ifdef _WIN32_WCE	
	if(!IsBackupRunning())
		DoBackup();
#endif

	return 0L;
}

void CPMADlg::_RunStartup()
{
	// Carregar os modulos de startup e executar...
	__MenuItem *pStartup = theApp.m_modules.GetStartupItem();
	if(pStartup != NULL)
	{
		POSITION p = pStartup->m_children.GetHeadPosition();
		while(p)
		{
			__MenuItem *pItem = pStartup->m_children.GetNext(p);
			if(pItem != NULL && pItem->nType == __MenuItem::TYPE_STARTUP)
			{
				_ExecuteModule(pItem->sModulePath, pItem->sParameter);
			}
		}
	}
}


void CPMADlg::_StopThreads()
{
	__HTTPSENDERTHREADMAP *pHttpSenderMap = theApp.GetInitInfo()->GetHttpSenderThreadMapPtr();
	POSITION p = pHttpSenderMap->GetStartPosition();	
	
	while(p)
	{
		CString sThreadName, sUrl2Send;
		__HttpSenderThreadItem *pList;

		pHttpSenderMap->GetNextAssoc(p, sThreadName, pList);	

		if(pList->objThread->IsThreadRunning())
		{
			pList->objThread->DestroyHttpSenderThread();
		}

		//delete pList;
	}

	if(theApp.GetInitInfo()->InitBTOnLogin())
		m_btManager.Stop();

	IniciarRegistroLogin_Logout();
#if _WIN32_WCE != 0x420 && defined _WIN32_WCE

	if(m_GPSThreadSender.IsThreadRunning())
	{
		m_GPSThreadSender.DestroyHttpSenderThread();
		m_gpsManager.UninitDevice();
		if(!m_GPSThreadSender.IsThreadRunning())
		{
			IniciarMonitorGPS();
		}
	}
#endif

}

/**
\brief Inicia execução da thread de envio de arquivos HttpSender
\return void
*/
void CPMADlg::_StartHttpSenderThreads()
{		
	CModParam param;	
	// Adicionar todas as urls...
	theApp.AddURLS(&param);
	int i = 1;

	//Recupera lista de thread HttpSender a serem criadas. Lidas do pma.xml
	__HTTPSENDERTHREADMAP *pHttpSenderMap = theApp.GetInitInfo()->GetHttpSenderThreadMapPtr();
	POSITION p = pHttpSenderMap->GetStartPosition();	

	//Num de threads	
	m_sNumHttpSenderThread.Format(L"%d", pHttpSenderMap->GetCount());	

	while(p)
	{		
		CString sThreadName, sUrl2Send;
		__HttpSenderThreadItem *pList;

		pHttpSenderMap->GetNextAssoc(p, sThreadName, pList);				

		//Verifica se sURL2Send possui o caracter &, se tiver faz o replace pela URL 
		//Ex. &recebe& será trocado pela URL recebe configurada no pma.xml (param.GetValue(L"recebe"))
		if(pList->sURL2Send.Find(_T("$")) >= 0)
		{
			sUrl2Send = pList->sURL2Send;
			sUrl2Send.Replace(L"$", L"");
			sUrl2Send = param.GetValue(sUrl2Send);
		}
		else
		{
			sUrl2Send = pList->sURL2Send;
		}
		
		CString sParamName, sParamConteudo;
		sParamName.Format(L"httpSenderThread_%d", i);
		sParamConteudo.Format(L"%s|%s", pList->sDescr, sThreadName);
		
		//Alimenta array com threads cridadas
		m_HttpSenderItems.SetAt(sParamName, sParamConteudo);

		pList->objThread = new CHttpSenderThread();
		//Inicia thread HttpSender
		if(!pList->objThread->IsThreadRunning())
		{
			if(sThreadName.Find(L"FOTO") > -1)
			{
				pList->objThread->DeleteFileAfterSend(TRUE);
			}
			pList->objThread->DeleteFileAfterSend(TRUE);  ///deletar XML
			pList->objThread->CreateHttpSenderThread(sThreadName, theApp.GetDatabaseRef(), sUrl2Send, pList->iTime);	
			pList->objThread->SetProxy(&theApp.m_proxyInfo);
		}

		i++;

		//delete pList->objThread;
		//delete pList;
	}
}

void CPMADlg::_SetupAutoLogoff()
{
	if(theApp.GetInitInfo()->AutoLogoffEnabled())
	{
		KillTimer(3);
 		SetTimer(3, theApp.GetInitInfo()->GetAutoLogoffTime()*1000*60, NULL);
	}
}



/**
\brief Inicia execução da thread de envio de Localização do Talão
\param void
\return void
*/
void CPMADlg::IniciarMonitorGPS(void)
{
	// Start da thread do GPS...
	#if _WIN32_WCE != 0x420	&& defined _WIN32_WCE
	CModParam param;
	ConfigurarParametros(&param);

	if(theApp.GetInitInfo()->HasGPS())
	{
		////if(m_GPSThreadSender.IsThreadRunning())
		////{
		////	m_GPSThreadSender.DestroyHttpSenderThread();
		////	m_gpsManager.UninitDevice();
		////}

		/************************** Inicia Thread do GPS **************************/
		STLOG_WRITE("CPMADlg::OnInitDialog() Uso do GPS habilitado no sistema");							
		if(!m_GPSThreadSender.IsThreadRunning())
		{

			//Configura informações: agente, contrato e url de envio do xml gps
			//m_gpsManager.SetDataBase(theApp.GetDatabaseRef());
			m_gpsManager.SetCodAgente(m_sUserCodigo);
			m_gpsManager.SetContrato(theApp.m_contratoInfo.sContrato);					
			
			//Ativar rastreamento?
			m_gpsManager.SetAVL(theApp.GetInitInfo()->EnableAVL());
			//Distancia entre pontos p/ atualização da coordenada
			m_gpsManager.SetBetweenPointsDistance(theApp.GetInitInfo()->GetBetweenPointsDistance());
			//Tempo de validade mínima de criação do job
			m_gpsManager.SetExpirationTimeWhenMoving(theApp.GetInitInfo()->GetExpirationTimeWhenMoving());	
			//Tempo de validade máxima de criação do job
			m_gpsManager.SetExpirationTimeWhenStatic(theApp.GetInitInfo()->GetExpirationTimeWhenStatic());				
			
			//Inicia thread GPS
			m_gpsManager.InitDevice();	
		}

		/*************** Inicia HttpSenderThread de envio de coord GPS **************/
		//URL que recebe XML das coordenadas captadas pelo GPS
		//CString sUrlTxGps = theApp.GetInitInfo()->GetURL(L"tx_gps_position");
		CString sUrlTxGps = theApp.GetUrl(theApp.GetInitInfo()->GetURL(L"tx_gps_position")); 						
		if(!m_GPSThreadSender.IsThreadRunning())
		{
			//Não vai gravar log na tabela httpsender_log
			m_GPSThreadSender.WorkWithLog(TRUE);
			
			//Deleta XML após envio
			m_GPSThreadSender.DeleteFileAfterSend(TRUE);

			//Atualiza relogio a cada envio
			m_GPSThreadSender.UpdateClockAfterSend(&theApp.m_proxyInfo, param.GetValue(L"util"), theApp.m_contratoInfo.sContrato);

			//Cria Thread
			m_GPSThreadSender.CreateHttpSenderThread(L"XML_GPS", theApp.GetDatabaseRef(), sUrlTxGps, 10);
		}
	}

	#endif			
}


/**
\brief Inicia execução da thread de envio de Informações do Talão
\param void
\return void
*/
void CPMADlg::IniciarEnvioInformacoesTalao(void)
{
	// Start da thread do GPS...
	#if _WIN32_WCE != 0x420	&& defined _WIN32_WCE
	CModParam param;
	ConfigurarParametros(&param);

	/************************** Inicia Thread de Envio de Informações do Talão **************************/
	////STLOG_WRITE("CPMADlg::OnInitDialog() Uso do GPS habilitado no sistema");							
	if(!m_RegInfoTalaoThreadSender.IsThreadRunning())
	{

		//Configura informações: agente, contrato e url de envio do xml gps
		//m_gpsManager.SetDataBase(theApp.GetDatabaseRef());
		m_InfoSender.SetCodAgente(m_sUserCodigo);
		m_InfoSender.SetContrato(theApp.m_contratoInfo.sContrato);
		m_InfoSender.SetVersaoSist(param.GetValue(L"version"));
		
		
		//Inicia thread Situacao
		m_InfoSender.InitDevice();	
				
	}
	///CHttpSenderThread m_RegInfoTalaoThreadSender;
	/*************** Inicia HttpSenderThread de envio de Informação do Talão **************/
	//URL que recebe XML de Informação do Talão
	CString sUrlTxInfo = theApp.GetUrl(theApp.GetInitInfo()->GetURL(L"tx_info_talao")); 						
	if(!m_RegInfoTalaoThreadSender.IsThreadRunning())
	{
		//Não vai gravar log na tabela httpsender_log
		m_RegInfoTalaoThreadSender.WorkWithLog(TRUE);
		
		//Deleta XML após envio
		m_RegInfoTalaoThreadSender.DeleteFileAfterSend(TRUE);

		//Atualiza relogio a cada envio
		m_RegInfoTalaoThreadSender.UpdateClockAfterSend(&theApp.m_proxyInfo, param.GetValue(L"util"), theApp.m_contratoInfo.sContrato);

		//Cria Thread
		m_RegInfoTalaoThreadSender.CreateHttpSenderThread(L"XML_INFO", theApp.GetDatabaseRef(), sUrlTxInfo, 10);
	}


	#endif			
}



/**
\brief Inicia execução da thread de envio de Registros de Login e Logout
\param void
\return void
*/
void CPMADlg::IniciarRegistroLogin_Logout(void)
{

#ifdef USA_TEM
	// Start da thread do Login_Logout...
	///#if _WIN32_WCE != 0x420	&& defined _WIN32_WCE
	CModParam param;
	ConfigurarParametros(&param);


	////if(m_RegLoginLogoutThreadSender.IsThreadRunning())
	////{
	////	m_RegLoginLogoutThreadSender.DestroyHttpSenderThread();
	////}

	/*************** Inicia HttpSenderThread de envio de Login/Logout **************/
	//URL que recebe as informações e Login/Logout
	CUtil::m_sUrlTxLoginLogout = theApp.GetUrl(theApp.GetInitInfo()->GetURL(L"login_logout")); 
	if(!m_RegLoginLogoutThreadSender.IsThreadRunning())
	{
		//Grava log na tabela httpsender_log
		m_RegLoginLogoutThreadSender.WorkWithLog(TRUE);
		
		//Deleta XML após envio
		m_RegLoginLogoutThreadSender.DeleteFileAfterSend(TRUE);

		//Atualiza relogio a cada envio
		m_RegLoginLogoutThreadSender.UpdateClockAfterSend(&theApp.m_proxyInfo, param.GetValue(L"util"), theApp.m_contratoInfo.sContrato);

		//Cria Thread
		m_RegLoginLogoutThreadSender.CreateHttpSenderThread(L"LOGIN_LOGOUT", theApp.GetDatabaseRef(), CUtil::m_sUrlTxLoginLogout, 60);
	}
#endif	
}

/**
\brief Inicia execução da thread de envio de Registros de Login e Logout
\param void
\return void
*/
void CPMADlg::IniciarRegistroAltera_Senha(void)
{
	// Start da thread do Login_Logout...
	///#if _WIN32_WCE != 0x420	&& defined _WIN32_WCE
	CModParam param;
	ConfigurarParametros(&param);


	/*************** Inicia HttpSenderThread de envio de Alteração de Senha **************/
	//URL que recebe as informações e Login/Logout
	CUtil::m_sUrlTxAlteraSenha = theApp.GetUrl(theApp.GetInitInfo()->GetURL(L"senha")); 
	if(!m_RegAlteraSenhaThreadSender.IsThreadRunning())
	{
		//Grava log na tabela httpsender_log
		m_RegAlteraSenhaThreadSender.WorkWithLog(TRUE);
		
		//Deleta XML após envio
		m_RegAlteraSenhaThreadSender.DeleteFileAfterSend(TRUE);

		//Atualiza relogio a cada envio
		m_RegAlteraSenhaThreadSender.UpdateClockAfterSend(&theApp.m_proxyInfo, param.GetValue(L"util"), theApp.m_contratoInfo.sContrato);

		//Cria Thread
		m_RegAlteraSenhaThreadSender.CreateHttpSenderThread(L"SENHA", theApp.GetDatabaseRef(), CUtil::m_sUrlTxAlteraSenha, 60);
	}
	
}



void CPMADlg::ConfigurarParametros(CModParam *param)
{
		//CModParam param;
		param->SetHInstance(AfxGetInstanceHandle());
		param->SetHwnd(this);
		param->AddPair(_T("codigo"), _T(""));
		param->AddPair(_T("sair"), _T(""));
		param->AddPair(_T("logo"), theApp.GetInitInfo()->GetLogoLoginPath());
		param->AddPair(_T("contrato"), theApp.m_contratoInfo.sContrato);
		param->AddPair(_T("version"), CVersao::GetAppVersion());
		param->AddPair(_T("upd_path"), theApp.GetModuleList()->GetUpdateItem()->sModulePath);
		param->AddPair(_T("db_path"), theApp.GetInitInfo()->GetDBPath());
		param->AddPair(_T("backup_path"), theApp.GetInitInfo()->GetBackupPath());
		param->AddPair(_T("atz_online"), theApp.GetInitInfo()->IsOnline() ? L"TRUE" : L"FALSE");
		param->AddPair(_T("enable_atualizacao"), theApp.GetInitInfo()->HasAtualizacao() ? L"TRUE" : L"FALSE");
		param->AddPair(_T("id_talao"), CUtil::GetIDTalao());		
		param->AddPair(_T("qt_series"), theApp.GetInitInfo()->GetQtSeries());
		param->AddPair(_T("blocked"), theApp.GetInitInfo()->GetBlockedPath());
		param->AddPair(_T("enable_gerincid"), theApp.GetInitInfo()->HasGerincid() ? L"TRUE" : L"FALSE");
		param->AddPair(_T("dias_antigas"), theApp.GetInitInfo()->GetDiasAntigas());
		
		// Adicionar todas as urls...
		theApp.AddURLS(param);
}


BOOL CPMADlg::_HookActivate()
{
	SetWindowsHookEx		= NULL;
	CallNextHookEx			= NULL;
	UnhookWindowsHookEx	= NULL;

	HINSTANCE hInstance = AfxGetInstanceHandle();

	m_hHookApiDLL = LoadLibrary(_T("coredll.dll"));
	if(m_hHookApiDLL == NULL) 
	{
		return FALSE;
	}
	else
	{
		SetWindowsHookEx = (_SetWindowsHookExW)GetProcAddress(m_hHookApiDLL, _T("SetWindowsHookExW"));
		if(SetWindowsHookEx == NULL) 
		{
			return FALSE;
		}
		else
		{
			m_hInstalledLLKBDhook = SetWindowsHookEx(WH_KEYBOARD_LL, _KeyboardHookCallback, hInstance, 0);
			if(m_hInstalledLLKBDhook == NULL) 
				return FALSE;

			CallNextHookEx = (_CallNextHookEx)GetProcAddress(m_hHookApiDLL, _T("CallNextHookEx"));
			if(CallNextHookEx == NULL) 
				return FALSE;

			UnhookWindowsHookEx = (_UnhookWindowsHookEx)GetProcAddress(m_hHookApiDLL, _T("UnhookWindowsHookEx"));
			if(UnhookWindowsHookEx == NULL) 
				return FALSE;
		}
	}

}

BOOL CPMADlg::_HookDeactivate()
{
	if(m_hInstalledLLKBDhook != NULL)
	{
		UnhookWindowsHookEx(m_hInstalledLLKBDhook);		
		m_hInstalledLLKBDhook = NULL;
	}

	if(m_hHookApiDLL != NULL)
	{
		FreeLibrary(m_hHookApiDLL);
		m_hHookApiDLL = NULL;
	}
	
	return TRUE;
}

LRESULT CALLBACK CPMADlg::_KeyboardHookCallback(int nCode, WPARAM wParam, LPARAM lParam)
{
	PKBDLLHOOKSTRUCT pkbhData = (PKBDLLHOOKSTRUCT)lParam;
	HWND hWnd;

	if (nCode == HC_ACTION) 
	{
		CString s;
		s.Format(L"Tecla apertada: %d(%x)\r\n", pkbhData->vkCode,pkbhData->vkCode);
		OutputDebugString(s);
		CString txt;

		switch(pkbhData->vkCode)
		{
		case 0x72:
			///if(!theApp.GetInitInfo()->IsPhoneEnabled()) /// não verifica mais a flag no PMA.XML
			if (!IsPhoneEnabled()) /// passa a verificar a permissão do usuário
				return -1;
		break;
		case 0x8f://Camera do Opticon em conjunto com 0x8c
			hWnd = ::FindWindow(NULL, L"Camera2"); 
			if(hWnd != NULL)
				::SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(32773, 0), 0); //Pede para a aplicação tirar foto
			return -1;
		break;
		case 0xc1:
		case 0xc2:
		case 0xc3:
		case 0xc4:
		case 0xc5:
		case 0xc6://Botão de gravação
		case 0x73://Botão vermelho
			CUtil::ShowPMA();
			return -1;
		break;
		case 0x5c:
			CWnd* wnd = GetForegroundWindow();
			wnd->GetWindowText(txt);
			s.Format(L"Janela ativa: %x. Nome: %s.\r\n", wnd->GetSafeHwnd(), txt);
			OutputDebugString(s);
		}
	}

	return CallNextHookEx(m_hInstalledLLKBDhook, nCode, wParam, lParam);
}


BOOL CPMADlg::_LoadConfigAplics( __MenuItem *pParentItem )
{
	//CInitInfo		init;
	CModuleInfo		modules;
	CString s;
	s.Format(_T("%s\\confaplic.xml"), CUtil::GetAppPath());


	// Fazer o parse e carregar os modulos para o menu...
	modules.SetCurrentItem( pParentItem );
   	if(!modules.LoadXML(s, TRUE))
	{
		CPMADlg::m_pSplash.Hide();
		///_ShowError(L"Erro Carregando aplicação.");
		STLOG_WRITE("Erro Carregando dados de aplicativos externos. Processamento do XML");
		return FALSE;
	}
	theApp._LoadIcons(pParentItem);

	
	return TRUE;
}


BOOL CPMADlg::IsPhoneEnabled()
{
	CString sUserPerms = CUtil::GetLoggedUserPerms();

	if (sUserPerms.Find(L"FONE") == -1)
	{
		return FALSE;
	}
	return TRUE;
}

BOOL CPMADlg::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct)
{
	if(pCopyDataStruct->dwData == 1) //Insert
	{
		BYTE byte[2048] = "";

		CopyMemory(byte, pCopyDataStruct->lpData, pCopyDataStruct->cbData);
		CString s((LPCSTR) &byte, pCopyDataStruct->cbData);

		CString sVal;
		CConsultas::DoQuery(CppSQLite3DB::getInstance(), CStringA(s), sVal);
	}
	if(pCopyDataStruct->dwData == 3)
	{
		BYTE byte[255];

		CopyMemory(byte, pCopyDataStruct->lpData, pCopyDataStruct->cbData);

		CString s((LPCSTR) &byte, pCopyDataStruct->cbData);

		CBannerWindow::getInstance()->UpdateText(s);
	}

	return __super::OnCopyData(pWnd, pCopyDataStruct);
}


/**
\brief Inicia execução da thread de envio de Informações do Talão
\param void
\return void
*/
void CPMADlg::IniciarBuscaMensagens(void)
{
	// Start da thread do GPS...
	#if _WIN32_WCE != 0x420	&& defined _WIN32_WCE
	CModParam param;
	ConfigurarParametros(&param);

	/************************** Inicia Thread de Envio de Informações do Talão **************************/
	////STLOG_WRITE("CPMADlg::OnInitDialog() Uso do GPS habilitado no sistema");							
	if(!m_RegBuscaMensagemThreadSender.IsThreadRunning())
	{

		//Configura informações: agente, contrato e url de envio do xml gps
		//m_gpsManager.SetDataBase(theApp.GetDatabaseRef());
		CString sUrlBuscaMen = theApp.GetUrl(theApp.GetInitInfo()->GetURL(L"consulta_mensagem")); 
		m_BuscaMensagem.SetCodAgente(m_sUserCodigo);
		m_BuscaMensagem.SetContrato(theApp.m_contratoInfo.sContrato);
		m_BuscaMensagem.SetIdTalao(CUtil::GetIDTalao());
		m_BuscaMensagem.SetUrl(sUrlBuscaMen);
		m_BuscaMensagem.SetParam(&param); 
		
		
		//Inicia thread Situacao
		m_BuscaMensagem.InitDevice();	
				
	}
	///CHttpSenderThread m_RegInfoTalaoThreadSender;
	/*************** Inicia HttpSenderThread de envio de Informação do Talão **************/
	//URL que recebe XML de Informação do Talão
	////CString sUrlBuscaMen = theApp.GetUrl(theApp.GetInitInfo()->GetURL(L"consulta_mensagem")); 						
	////if(!m_RegBuscaMensagemThreadSender.IsThreadRunning())
	////{
	////	//Não vai gravar log na tabela httpsender_log
	////	m_RegBuscaMensagemThreadSender.WorkWithLog(TRUE);
	////	
	////	//Deleta XML após envio
	////	m_RegBuscaMensagemThreadSender.DeleteFileAfterSend(TRUE);

	////	//Atualiza relogio a cada envio
	////	m_RegBuscaMensagemThreadSender.UpdateClockAfterSend(&theApp.m_proxyInfo, param.GetValue(L"util"), theApp.m_contratoInfo.sContrato);

	////	//Cria Thread
	////	///m_RegBuscaMensagemThreadSender.CreateHttpSenderThread(L"XML_INFO", theApp.GetDatabaseRef(), sUrlTxInfo, 10);
	////}


	#endif			
}


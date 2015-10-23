// Copyright (c) 2007 Marcos Mori de Siqueira. All Rights Reserved. 
// mori@softfactory.com.br
// Classe de de aplicacao principal - onde tudo se inicia...
#include "stdafx.h"
#include "PMA.h"
#include "PMADlg.h"
#include "Utils.h"
#include "Module.h"
#include "CStr.h"
#include "ProxyTable.h"
#include "Registry.h"
#include "ContratoTable.h"
#include "VersaoApp.h"
#include <afxsock.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CSplash *CPMAApp::m_pBlocked = NULL;

BEGIN_MESSAGE_MAP(CPMAApp, CWinApp)
END_MESSAGE_MAP()

/**
\brief Construtor da classe
\param void
*/
CPMAApp::CPMAApp()
	: CWinApp()
{
	m_hMutex = NULL;
	m_bAlreadyUpdated = FALSE;
	m_pModuleEmUse = NULL;
}

CPMAApp theApp;

void CPMAApp::_SetConfToRegistry()
{
	Registry reg(HKEY_LOCAL_MACHINE, REG_KEY_APP_PARAMS);
	reg.Open();

	reg.SetValue(L"DBPath", m_init.GetDBPath());
	reg.SetValue(L"Path", m_init.GetPath());
	reg.SetValue(L"BlockedPath", m_init.GetBlockedPath());
	reg.SetValue(L"SymbolPath", m_init.GetSymbolPath());
	reg.SetValue(L"IsBackupActive", m_init.IsBackupActive());
	reg.SetValue(L"BackupPath", m_init.GetBackupPath());
	reg.SetValue(L"PhoneEnabled", m_init.IsPhoneEnabled());
	reg.SetValue(L"Online", m_init.IsOnline());
	reg.SetValue(L"HasCamera", m_init.HasCamera());
	reg.SetValue(L"Estadual", m_init.IsEstadual());
	reg.SetValue(L"PrinterName", m_init.GetPrinterName());
	reg.SetValue(L"EnableGPS", m_init.HasGPS());
	reg.SetValue(L"EnableAVL", m_init.EnableAVL());

	__URLSMAP* url = m_init.GetURLMapPtr();
	__URLSMAP* file = m_init.GetFileMapPtr();
	__URLSMAP* server = m_init.GetServerMapPtr();

	POSITION p = url->GetStartPosition();
	while(p)
	{
		CString sURL, sKey, sValue;
		url->GetNextAssoc(p, sKey, sValue);	

		reg.SetValue(sKey, GetUrl(sValue));
	}

	p = file->GetStartPosition();
	while(p)
	{
		CString sURL, sKey, sValue;
		file->GetNextAssoc(p, sKey, sValue);	

		CUtil::GetPathFromVariable(sValue);
		reg.SetValue(sKey, sValue);
	}

	p = server->GetStartPosition();
	while(p)
	{
		CString sURL, sKey, sValue;
		server->GetNextAssoc(p, sKey, sValue);	

		reg.SetValue(sKey, sValue);
	}
}

/**
\brief Ponto de início da execução.
\details
	As funções executadas por este método:
	- Verificar se o sistema já está ou não executado;
	- Iniciar a leitura do arquivo XML de configuração;
	- Executar o splash do início;
	- Montagem do menu principal e da tela principal;
	- Verificar a existência do banco;
	- Recuperar do Backup, se existente;
	- Abrir o banco de dados;
	- Iniciar a criação o banco de dados;
	- Verificar o bloqueio do equipamento;
	- Verificar o cadastro do equipamento na central.
	- Caso seja a primeira instalação, faz a chamado para a execução da atualização e a configuração de proxy e contrato;

\param void
\return TRUE se a execução acontece com sucesso
		FALSE se a aplicação já estiver em uso;
		      algum parâmetro necessário não for encontrado;
			  arquivo XML de configuração não existir, ou estiver com erro;
			  não conseguir executar módulos de atualização, proxy ou contrato.
*/
BOOL CPMAApp::InitInstance()
{
	COleDateTime odt1 = COleDateTime::GetCurrentTime();

#ifndef _SIMPLE_REQUEST
	CUtil::InitCriticalSection();
#endif

///#if 0
	if (!AfxSocketInit())
	{
		///AfxMessageBox(L"AfxSocketInit() falhou!");
		STLOG_WRITE("%s(%d): erro em WSAStartup Av. Mem.: [%d]", __FUNCTION__, __LINE__, CUtil::GetAvailableMemory());
		return FALSE;
	}
///#endif

	//STLOG_WRITE(L"%S(%d): 1: [%ld] 2: [%ld] 3: [%ld]", nFreeToCaller, nTotal, nFree);

#if 0
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD( 2, 2 );

	int err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 ) {
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.*/
		STLOG_WRITE("%s(%d): erro [%d] em WSAStartup Av. Mem.: [%d]", __FUNCTION__, __LINE__, err, CUtil::GetAvailableMemory());
		return FALSE;
	}
#endif

	// Usamos um mutex para saber se a aplicacao jah esta rodando...
    m_hMutex = CreateMutex(0, 0, _T("__PMA_EXE_MUTEX__"));
    if(NULL != m_hMutex)
    {
        if(ERROR_ALREADY_EXISTS == GetLastError())
        {
            // Jah esta rodando...
			HWND hWnd = ::FindWindow(_T("Dialog"), _T("PMA"));
            if(NULL != hWnd)
            {
				// Colocar no foco...
                SetForegroundWindow(hWnd);
				CloseHandle(m_hMutex);

				// Sair...
				return FALSE;
            }
        }
	}

//	::CoInitializeEx(NULL, 0);
//	AfxEnableControlContainer();

	STLOG_WRITE("Iniciando aplicação");	
#ifdef _WIN32_WCE
	STLOG_WRITE("Dispositivo: [%S - %S]", CUtil::GetDeviceManufactureName(), CUtil::GetIDTalao());
	STLOG_WRITE(CUtil::ListStorages());
	STLOG_WRITE(CUtil::ListFilesInDir(CUtil::GetMainAppPath()));
	STLOG_WRITE(CUtil::ListFilesInDir(CUtil::GetMainAppPath() + L"\\Modules"));

	//Acerta o formato da data e da hora no equipamento
	CString sFmt = L"dd/MM/yyyy";
	SetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_SSHORTDATE, sFmt);
	sFmt = L"HH:mm:ss";
	SetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_STIMEFORMAT, sFmt);

	//CString s = CUtil::LatLongByCellTower();


#if defined(WIN32_PLATFORM_PSPC) || defined(WIN32_PLATFORM_WFSP)
    // SHInitExtraControls should be called once during your application's 
	// initialization to initialize any of the Windows Mobile specific 
	// controls such as CAPEDIT and SIPPREF.
    SHInitExtraControls();
#endif // WIN32_PLATFORM_PSPC || WIN32_PLATFORM_WFSP

#endif

	SetRegistryKey(_T("engebras"));

#if 0
	// Inicializa o registry...
	m_btCOMMPort = GetProfileString(L"Bluetooth", L"Porta", NULL);
	if(m_btCOMMPort.IsEmpty())
	{
		m_btCOMMPort = L"COM8:";
		WriteProfileString(L"Bluetooth", L"Porta", L"COM8:");
	}
#endif

	CString s;
	s.Format(_T("%s\\pma.xml"), CUtil::GetAppPath());

	//Inicia a leitura do arquivo XML
	if(!m_init.LoadXML(s))
	{
		_ShowError(L"Erro Carregando aplicação.");
		STLOG_WRITE("Erro Carregando aplicacao. Processamento do XML");
		return FALSE;
	}

	_SetConfToRegistry();

	//Exibição do Splash de início
	if(CPMADlg::m_pSplash.Create(NULL, m_init.GetPath(), m_init.GetWidth(), m_init.GetHeight()))
		CPMADlg::m_pSplash.Show(3);

	CPMADlg::m_waitDlg = CMsgWindow::getInstance();
	CPMADlg::m_waitDlg->Create(NULL);

	STLOG_WRITE(L"ONLINE MODE: %s", m_init.IsOnline() ? L"TRUE" : L"FALSE");
	STLOG_WRITE(L"Versão do aplicativo: %s", CVersao::GetAppVersion());

#ifdef _WIN32_WCE
	CMapStringToString strMap;
	if(!CUtil::ListFlashCards(strMap))
	{
		STLOG_WRITE("Erro listando os devices flash.");
		CPMADlg::m_pSplash.Hide();
		_ShowError(L"Erro Carregando aplicação.");
		return FALSE;
	}
	//else
	//{
	//	if(m_init.GetVeiculosArqName().IsEmpty())
	//	{
	//		STLOG_WRITE("nome do arquivo b-tree de veiculos esta vazio.");

	//		CPMADlg::m_pSplash.Hide();
	//		_ShowError(L"Erro Carregando aplicação.");
	//		//return FALSE;
	//	}
	//	else
	//	{
	//		POSITION pa = strMap.GetStartPosition();

	//		while(pa)
	//		{
	//			CString s;
	//			CString k, v;
	//			strMap.GetNextAssoc(pa, k, v);

	//			s.Format(L"\\%s\\%s", k, m_init.GetVeiculosArqName());
	//			if(CUtil::FileExists(s))
	//			{
	//				STLOG_WRITE(L"path para pesq. offline veiculos: %s.", s);
	//				m_sVeiculoPath = s;
	//				break;
	//			}
	//		}

	//		if(m_sVeiculoPath.IsEmpty())
	//		{
	//			//CPMADlg::m_pSplash.Hide();
	//			STLOG_WRITE(L"O arquivo veiculos.egb nao foi encontrado.");
	//			//_ShowError(L"O arquivo veiculos.egb nao foi encontrado.");
	//		}
	//	}
	//}
#endif

	// Se o path do database estiver vazio...
	if(m_init.GetDBPath().IsEmpty())
	{
		CPMADlg::m_pSplash.Hide();
		_ShowError(L"Erro Carregando aplicação.");
		STLOG_WRITE("Database path nao encontrado no XML.");
		return FALSE;
	}

	CStr sFile(m_init.GetDBPath());

	// Fazer o parse e carregar os modulos para o menu...
	if(!m_modules.LoadXML(s, TRUE))
	{
		CPMADlg::m_pSplash.Hide();
		_ShowError(L"Erro Carregando aplicação.");
		STLOG_WRITE("Erro Carregando aplicacao. Processamento do XML");
		return FALSE;
	}

	// Adicionar o aboutbox...
	__MenuItem *pItem = new __MenuItem();
	m_modules.GetRootItem()->m_children.InsertBefore(m_modules.GetRootItem()->m_children.GetTailPosition(), pItem);
	pItem->nType      = __MenuItem::TYPE_ITEM;
	pItem->level      = 1;
	pItem->sText      = L"Sobre";
	pItem->nID	      = CModuleInfo::ID_END + 1; // fora do range...
	pItem->nIconResID = -1;

	// Separator...
	pItem = new __MenuItem();
	m_modules.GetRootItem()->m_children.InsertBefore(m_modules.GetRootItem()->m_children.GetTailPosition(), pItem);
	pItem->nType      = __MenuItem::TYPE_SEPARATOR;
	pItem->level      = 1;

	// Inicializar o image list...
#ifdef _WIN32_WCE
	/// solic. 61326 
	///m_imgList.Create(IDB_IMAGELIST, 36, 0, RGB(255,0,255)); 

	CString sTst;
	BOOL largeIcons = FALSE;
	alt  = GetSystemMetrics(SM_CYSCREEN);
	larg = GetSystemMetrics(SM_CXSCREEN);
	sTst.Format(L"Screen %dX%d", larg, alt );
	m_bLargeIcons = FALSE;
	if ((larg >= 480) && (alt >= 480))
	{
		m_bLargeIcons = TRUE;
	}
	if (m_bLargeIcons)
	{
		m_imgList.Create(IDB_IMAGELIST_G, 64, 0, RGB(255,0,255));
	}
	else
	{
		m_imgList.Create(IDB_IMAGELIST1, 32, 0,  RGB(255,0,255));  /// era 36
	}

#else
	m_imgList.Create(36, 36, ILC_MASK | ILC_COLOR24, 0, 0);

	CBitmap bmp;
	bmp.LoadBitmap(IDB_IMAGELIST);
	COLORREF rgbTransparentColor = RGB(255,0,255);
	m_imgList.Add(&bmp, rgbTransparentColor);

#endif
	_LoadIcons(m_modules.GetRootItem());

	// Verifica se tenho o path para o modulo de atualizacao...
	if(m_modules.GetUpdateItem() == NULL)
	{
		CPMADlg::m_pSplash.Hide();
		_ShowError(L"Erro Carregando aplicação.");
		STLOG_WRITE("Database nao existe e modulo de inicializacao nao definido no xml.");
		return FALSE;
	}

	BOOL bAddProxyInfo = FALSE;
#ifdef _WIN32_WCE
	CString sBackupFile = CUtil::GetBackupPath();
	if(!sBackupFile.IsEmpty())
	{
		if(sBackupFile.GetAt(sBackupFile.GetLength()-1) != '\\')
			sBackupFile += '\\';

		sBackupFile += L"BACKUP.DB";
	}


#endif

	// Verificar se o database existe...
	if(CUtil::FileExists(CString(sFile)))
	{
		CFile file;
		CFileException fError;
		if(!file.Open(CString(sFile), CFile::modeRead, &fError))
		{
			TCHAR szError[1024];
			fError.GetErrorMessage(szError, 1024);

			CString sErr;
			sErr.Format(L"Não foi possível abrir o arquivo %s. \r\nMotivo: %s", fError.m_strFileName, szError);

			//AfxMessageBox(sErr);

			//return FALSE;

		}

		/*if(file.GetLength()==0)
		{
			file.Close();
			CFile::Remove(CString(sFile));
		}*/
	}
#ifdef _WIN32_WCE
	else if(CUtil::FileExists(sBackupFile))
	{
		if(!CopyFile(sBackupFile, CString(sFile), FALSE))
		{
			STLOG_WRITE("CPMAApp::InitInstance() : Erro restaurando arquivo de backup [%S] [%s]", sBackupFile, sFile);
		}
		else
		{
			STLOG_WRITE("CPMAApp::InitInstance() : Arquivo de backup restaurado");
		}
	}
#endif
	else if(!CUtil::FileExists(CString(sFile)))
	{
		bAddProxyInfo = TRUE;

		// Se o database nao existe... conf. o proxy e o contrato...
		if(!_ExecContratoConf())
		{
			return FALSE;
		}
		/*if(!_ExecProxyConf())
		{
			return FALSE;
		}*/

		// Executar o modulo de atualizacao...
		if(!_ExecuteUpdate(m_modules.GetUpdateItem()->sModulePath, 
						   CString(sFile), 
						   FALSE))
		{
			STLOG_WRITE("Erro executando o update (1a vez - initialization mode).");
			return FALSE;
		}

		m_bAlreadyUpdated = TRUE;
	}
	else
	{
		m_bAlreadyUpdated = FALSE;
	}

	// Tentar abrir o banco de dados...
	try
	{
		if(!CppSQLite3DB::getInstance()->isOpen())
		{
			m_database = CppSQLite3DB::getInstance();
			m_database->open(sFile);
			m_database->setBusyHandler();
			//m_database->key("1234");
		}

		STLOG_WRITE("%s(%d): Ponteiro do BD: %x", __FUNCTION__, __LINE__, CppSQLite3DB::getInstance());
	}
    catch(CppSQLite3Exception e)
    {
		CPMADlg::m_pSplash.Hide();

		STLOG_WRITE("Abrindo database path '%s', erro %s", 
					m_init.GetDBPath(), 
					e.errorMessage());

		_ShowError(L"Erro Carregando aplicação.");
		return FALSE;
    }	

	if(bAddProxyInfo)
	{
		// Se atualizou com sucesso, vamos salvar as informacoes do proxy
		// no database...
		CProxyTable proxy;
		proxy.Init();
		proxy.SetValues(&m_proxyInfo);
		if(!proxy.Insert(GetDatabaseRef()))
		{
			STLOG_WRITE("Erro inserindo dados do proxy no database.");
		}

		/*CContratoTable contrato;
		contrato.Init();
		contrato.SetValues(&m_contratoInfo);
		if(!contrato.Insert(GetDatabaseRef()))
		{
			STLOG_WRITE("Erro inserindo dados do contrato no database.");
		}*/
	}
	else
	{
		CProxyTable proxy;
		proxy.Init();
		if(!proxy.Load(GetDatabaseRef(), &m_proxyInfo))
		{
			STLOG_WRITE("Erro recuperando dados do proxy no database.");
		}

		CContratoTable contrato;
		contrato.Init();
		if(!contrato.Load(GetDatabaseRef(), &m_contratoInfo))
		{
			STLOG_WRITE("Erro recuperando dados do contrato no database.");
			return FALSE;
		}
		else if(m_contratoInfo.sContrato.Compare(L"VAZIO")==0)
		{
			if(!_ExecContratoConf())
			{
				STLOG_WRITE("Erro recuperando dados do contrato no database.");
				return FALSE;
			}

			contrato.SetValues(&m_contratoInfo);
			if(!contrato.Insert(GetDatabaseRef()))
			{
				STLOG_WRITE("Erro inserindo dados do contrato no database.");
			}
		}

	}
	
	// Verificar se o pocket esta bloqueado...
	CString sResp;
	CString sURL = GetUrl(m_init.GetURL(L"util")); //VERIFICAR DEPOIS: Era pra ser status
	CString sContrato = m_contratoInfo.sContrato; //m_init.GetContrato();

	if(!GetInitInfo()->HasLogin())
	{
		CUtil::IsOnline();
		CUtil::SetDataHoraServidor(&m_proxyInfo, sURL, sContrato);

		STLOG_MARKER(_T("CPMAApp::InitInstance"));

		COleDateTime odt2 = COleDateTime::GetCurrentTime();
		COleDateTimeSpan odts = odt2 - odt1;
		STLOG_WRITE(_T("Tempo carregamento: %ld segundos"), odts.GetSeconds());

		// Agora inicializar o menu...
		CPMADlg dlg;
		m_pMainWnd = &dlg;
		dlg.DoModal();		
	}
	else
	{
		if(CUtil::IsOnline(&m_proxyInfo, sURL, sContrato) &&
			!CUtil::IsValidTalao(&m_proxyInfo, sURL, sContrato, &sResp))
		{
			if(sResp.Find(L"Bloqueado")!=-1)
			{
				CPMADlg::m_pSplash.Hide();
				STLOG_WRITE(L"Block: Erro de acesso %s.", sResp);
				CPMAApp::m_pBlocked = new CSplash();
				if(CPMAApp::m_pBlocked->Create(NULL, m_init.GetBlockedPath(), m_init.GetWidth(), m_init.GetHeight()))
					CPMAApp::m_pBlocked->Show();

				// CPU A 100 %...
				while(1) {}
			}
			else
			{
				AfxMessageBox(sResp);
				CWnd *pWnd = CWnd::FindWindow(_T("HHTaskBar"),_T(""));
				if(NULL != pWnd)
				{
					pWnd->ShowWindow(SW_SHOW);
					pWnd->EnableWindow(TRUE);
				}
			}
		}
		else
		{
			STLOG_MARKER(_T("CPMAApp::InitInstance"));

			COleDateTime odt2 = COleDateTime::GetCurrentTime();
			COleDateTimeSpan odts = odt2 - odt1;
			STLOG_WRITE(_T("Tempo carregamento: %ld segundos"), odts.GetSeconds());

			// Agora inicializar o menu...
			CPMADlg dlg;
			m_pMainWnd = &dlg;
			dlg.DoModal();
		}
	}

	return FALSE;
}

/**
\brief Faz a preparação e a execução da atualização (baixa de arquivos e procedimentos necessários para que o equipamento funcione
\param LPCTSTR szModPath: Caminho do módulo
\param LPCTSTR szDBPath: Caminho do banco de dados da aplicação
\param BOOL bCreateDatabase: Se TRUE força a criação do banco de dados
\param HWND hParent: Handle da janela que será a janela pai do módulo de atualização, valor padrão 0
\return TRUE se a execução ocorrer com sucesso
*/
BOOL CPMAApp::_ExecuteUpdate(LPCTSTR szModPath, LPCTSTR szDBPath, BOOL bCreateDatabase, HWND hParent)
{
	CModule module;
	BOOL bRet = module.Create(szModPath);
	
	m_pModuleEmUse = &module;
	if(bRet)
	{
		CModParam param;
		param.AddPair(L"db_path", szDBPath);
		param.AddPair(L"create_db", bCreateDatabase ? L"TRUE" : L"FALSE");

		CString sBackupPath = GetInitInfo()->GetBackupPath();
		ASSERT(!sBackupPath.IsEmpty());
		param.AddPair(_T("backup_path"), sBackupPath);


		CString sContrato = m_contratoInfo.sContrato; //GetInitInfo()->GetContrato();
		ASSERT(!sContrato.IsEmpty());
		param.AddPair(_T("contrato"), sContrato);
		param.AddPair(_T("cod_autuador"), m_contratoInfo.sCodAutuador);

		CString sOnLine = GetInitInfo()->IsOnline() ? L"TRUE" : L"FALSE";		
		param.AddPair(_T("atz_online"), sOnLine);		

		CString sQtSeries = GetInitInfo()->GetQtSeries();
		param.AddPair(_T("qt_series"), sQtSeries);

		CString sDiasAntigas = GetInitInfo()->GetDiasAntigas();
		param.AddPair(_T("dias_antigas"), sDiasAntigas);

		param.AddPair(_T("blocked"), GetInitInfo()->GetBlockedPath());

		// Adicionar todas as urls...
		AddURLS(&param);

		// Adicionar todas os files...
		__URLSMAP *pFileMap = GetInitInfo()->GetFileMapPtr();
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

		CString sPort;
		sPort.Format(L"%ld", m_proxyInfo.nPort);

		// Inserir dados do proxy...
		param.AddPair(_T("DISCAGEM"), m_proxyInfo.bDiscagem ? L"TRUE" : L"FALSE");
		param.AddPair(_T("PROXY"),	  m_proxyInfo.bProxy ? L"TRUE" : L"FALSE");
		param.AddPair(_T("SERVER"),   m_proxyInfo.sServer);
		param.AddPair(_T("PORT"),	  sPort);
		param.AddPair(_T("USER"),	  m_proxyInfo.sUser);
		param.AddPair(_T("PASS"),	  m_proxyInfo.sPass);
		param.AddPair(_T("id_talao"), CUtil::GetIDTalao());

		LONG size = 0;
		LPBYTE p = param.GetBuffer(&size);
		bRet = module.Run(NULL, hParent, &p, &size);
		module.Destroy();
		param.ReleaseBuffer();
	}

	if(!bRet)
	{
		CPMADlg::m_pSplash.Hide();
		_ShowError(L"Erro Inicializando a aplicação.");
		STLOG_WRITE(L"CPMAApp::_ExecuteUpdate(): Erro no modulo de atualizacao '%s'.", szModPath);
		return FALSE;
	}

	m_pModuleEmUse = NULL;

	return TRUE;
}

/**
\brief Carrega os ícones de acordo com o tipo de ítem no menu
\param __MenuItem *_pItem: Item do menu
\return void
*/
void CPMAApp::_LoadIcons(__MenuItem *_pItem, HWND hWnd)
{
	if(_pItem != NULL)
	{
		POSITION p = _pItem->m_children.GetHeadPosition();
		while(p)
		{
			__MenuItem *pItem = _pItem->m_children.GetNext(p);
			if(pItem != NULL)
			{
				if(pItem->nType == __MenuItem::TYPE_BACK)
				{
					pItem->nIconIdx = IDX_ICON_BACK;
				}
				else if(pItem->nType == __MenuItem::TYPE_POPUP)
				{
					pItem->nIconIdx = IDX_ICON_FOLDER;
					if(!hWnd) 
						_LoadIcons(pItem);
					else
						_LoadIcons(pItem, hWnd);
				}
				else if(pItem->nType == __MenuItem::TYPE_ITEM)
				{
					// Icone da saida...
					if(pItem->nID == IDOK || pItem->nID == IDCANCEL)
					{
						pItem->nIconIdx = IDX_ICON_EXIT;
					}
					else if(pItem->nID == CModuleInfo::ID_END + 1)
					{
						pItem->nIconIdx = IDX_ICON_HELP;
					}
					else
					{
						pItem->nIconIdx = IDX_ICON_UNKNOWN;

						if(pItem->sModulePath.Find(L".exe") > 0 || 
						   pItem->sModulePath.Find(L".EXE") > 0  )
						{
							if(CUtil::FileExists(pItem->sModulePath))
							{

								static SHFILEINFO s_sFi          = {0};
								static HIMAGELIST s_himl         = NULL;

								s_himl = (HIMAGELIST)SHGetFileInfo(pItem->sModulePath, 0, &s_sFi, sizeof(s_sFi), SHGFI_ICON | SHGFI_LARGEICON);

								if(s_himl != NULL) 
								{
									///pItem->nIconIdx = m_imgList.Add(s_sFi.hIcon,RGB(0, 0, 0)); ///m_imgList.Add(res);
									pItem->nIconIdx = m_imgList.Add(s_sFi.hIcon);

								}
								else
								{
									pItem->nIconIdx = IDX_ICON_UNKNOWN;
								}
								DestroyIcon(s_sFi.hIcon);

							}
						}
						else if(pItem->sModulePath.Find(L".lnk") > 0  || pItem->sModulePath.Find(L".LNK") > 0)
						{
							///altalhos
							static SHFILEINFO s_sFi          = {0};
							static HIMAGELIST s_himl         = NULL;

							s_himl = (HIMAGELIST)SHGetFileInfo(pItem->sModulePath, 0, &s_sFi, sizeof(s_sFi), SHGFI_ICON | SHGFI_LARGEICON);

							CString sAtalho;
							sAtalho.Format(L"Nome: [%s] Icone: [%d]\r\n", pItem->sModulePath, s_sFi.hIcon);
							OutputDebugString(sAtalho);
		
							if(s_himl != NULL) 
								pItem->nIconIdx = m_imgList.Add(s_sFi.hIcon);///m_imgList.Add(res);
							else
								pItem->nIconIdx = IDX_ICON_UNKNOWN;

							DestroyIcon(s_sFi.hIcon);
							//DestroyIcon(hImg);

						}
						else
						{
							HICON hImg, hImgSml;
							HICON res = (HICON)ExtractIconEx(pItem->sModulePath, -pItem->nIconResID, &hImg, &hImgSml, 1);

							HICON hIco;
							hIco = hImg;
							//mapIcon.Lookup(pItem->nID, hIco);
							if(hIco)							
								pItem->nIconIdx = m_imgList.Add(hIco);
							else
								pItem->nIconIdx = IDX_ICON_UNKNOWN;

							DestroyIcon(res);
							DestroyIcon(hImg);
							DestroyIcon(hImgSml);
						}
					}
				}
			}
		}
	}

	//
}

/**
\brief Ponto de fim da aplicação.
\details
	Neste método o banco de dados é fechado e a aplicação é encerrada.

\param void
\return número inteiro indicando o código de finalização
*/
int CPMAApp::ExitInstance()
{

	////m_database->close();

	CppSQLite3DB::getInstance()->close();

    CloseHandle(m_hMutex);

	return CWinApp::ExitInstance();
}

/**
\brief Exibe um Message Box com uma mensagem de erro
\param LPCTSTR szMess : Mensagem a ser exibida
\return void
*/
void CPMAApp::_ShowError(LPCTSTR szMess)
{
	::MessageBox(::GetActiveWindow(), szMess, L"Mensagem", MB_ICONERROR|MB_OK);
}

/**
\brief Sem uso
*/
BOOL CPMAApp::SetupPrinter(CWnd *pWnd)
{
// Somente pocket pc 2003 ....
#if 0// _WIN32_WCE == 0x420

	BT2003Lib::IPrinterPtr printer;
	HRESULT hResult = printer.CreateInstance(L"BT2003.Printer.1");
	if(SUCCEEDED(hResult))
	{
		BSTR bstrAddress(L"");
		hResult = printer->ChoosePrinter((wireHWND)pWnd->GetSafeHwnd(), &bstrAddress);

		if(SUCCEEDED(hResult))
		{
			m_sPrinterAddress = bstrAddress;
//		WriteProfileString(L"Bluetooth", L"Address", s);
//		CString s(L"0:11:36:48:84:234:2");
//		printer->Print(NULL, _bstr_t(s), _bstr_t(L"TESTE DE IMPRESSAO"), 18);
			printer->Cleanup();
			printer.Release();
		
			return TRUE;
		}
	}
	else
	{
		STLOG_WRITE(L"Erro iniciando o OCX de impressao 0x%08X", hResult);
	}
#endif

	return FALSE;
}

/**
\brief Faz a preparação e a execução da tela de configuração do proxy
\param void
\return TRUE se a execução ocorrer com sucesso
*/
BOOL CPMAApp::_ExecProxyConf()
{
	if(GetInitInfo()->GetProxyConfPath().IsEmpty())
	{
		STLOG_WRITE(L"Nome do modulo de proxy esta nulo");
		return FALSE;
	}

	CPMADlg::m_pSplash.Hide();

	// Como estamos executando a configuracao antes de termos o
	// database...
	CModule module;
	BOOL bRet = module.Create(GetInitInfo()->GetProxyConfPath());
	if(bRet)
	{
		CModParam param;

		CString sContrato = m_contratoInfo.sContrato;
		ASSERT(!sContrato.IsEmpty());
		param.AddPair(_T("contrato"), sContrato);

		// Adicionar todas as urls...
		AddURLS(&param);

		LONG size = 0;
		LPBYTE p = param.GetBuffer(&size);
		bRet = module.Run(NULL, NULL, &p, &size);

		// Recuperar dados enviados pelo modulo...
		CModParam param1;
		VERIFY(param1.SetBuffer(p, size));

		m_proxyInfo.bDiscagem  = param1.GetValue(L"DISCAGEM").CompareNoCase(L"TRUE") == 0;
		m_proxyInfo.bProxy     = param1.GetValue(L"PROXY").CompareNoCase(L"TRUE") == 0;
		m_proxyInfo.sServer	   = param1.GetValue(L"SERVER");
		m_proxyInfo.nPort      = _wtol(param1.GetValue(L"PORT"));
		m_proxyInfo.sUser	   = param1.GetValue(L"USER");
		m_proxyInfo.sPass      = param1.GetValue(L"PASS");

		STLOG_WRITE(L"Discagem  %d", m_proxyInfo.bDiscagem);
		STLOG_WRITE(L"Use Proxy %d", m_proxyInfo.bProxy);
		STLOG_WRITE(L"Servidor  %s", m_proxyInfo.sServer);
		STLOG_WRITE(L"Porta		%d", m_proxyInfo.nPort);
		STLOG_WRITE(L"Usuario   %s", m_proxyInfo.sUser);
		STLOG_WRITE(L"Senha		%s", m_proxyInfo.sPass);

		param.ReleaseBuffer();

		module.Destroy();
		return TRUE;
	}
	
	STLOG_WRITE(L"erro criando o modulo de conf. de proxy");
	return FALSE;
}

/**
\brief Faz a preparação e a execução da tela de configuração do contrato
\param void
\return TRUE se a execução ocorrer com sucesso
*/
BOOL CPMAApp::_ExecContratoConf()
{
	if(GetModuleList()->GetLoginItem()->sModulePath.IsEmpty())
	{
		STLOG_WRITE(L"Nome do modulo de proxy esta nulo");
		return FALSE;
	}

	CPMADlg::m_pSplash.Hide();

	// Como estamos executando a configuracao antes de termos o
	// database...
	CModule module;
	BOOL bRet = module.Create(GetModuleList()->GetLoginItem()->sModulePath);
	if(bRet)
	{
		CModParam param;
		
		param.AddPair(_T("tela"), _T("contrato"));
		param.AddPair(_T("version"), CVersao::GetAppVersion());

		LONG size = 0;
		LPBYTE p = param.GetBuffer(&size);
		bRet = module.Run(NULL, NULL, &p, &size);

		// Recuperar dados enviados pelo modulo...
		CModParam param1;
		VERIFY(param1.SetBuffer(p, size));

		m_contratoInfo.sContrato = param1.GetValue(L"CONTRATO");
		m_contratoInfo.sCodAutuador = param1.GetValue(L"COD_ORGAO_AUTUADOR"); 

		param.ReleaseBuffer();
		param1.ReleaseBuffer();

		module.Destroy();
		return TRUE;
	}
	
	STLOG_WRITE(L"erro criando o modulo de conf. de proxy");
	return FALSE;
}


/**
\brief Pesquisa e excluir arquivos de log antigos
\param void
\return void
*/
void CPMAApp::_DeleteOldLogs()
{
	HANDLE hFind;
	WIN32_FIND_DATA FindFileData;
	FILETIME ft_local, ft;
	int nErrorDateDiff;

	CMsgWindow* wnd;
	wnd = CMsgWindow::getInstance();
	wnd->Show(L"Movendo logs antigos...");

	int iLogDays = 10;

	//Dorme um segundo para que no início o equipamento reconheça o SDCard
	Sleep(1000);

#ifdef _WIN32_WCE
	CString sTempName = CUtil::GetSDCardPath();
	if(sTempName.IsEmpty())
	{
		iLogDays = 1;
		sTempName = L"\\temp";
	}
#else
	CString sTempName = L"c:\\temp";
#endif

	sTempName.AppendFormat(L"\\%s", CUtil::GetIDTalao());
	CreateDirectory(sTempName, NULL);

	CString sFileName = L"*_log.txt";
	sFileName.Format(L"%s\\*_log.txt", CUtil::GetAppPath());

	hFind = FindFirstFile(sFileName, &FindFileData);

	if(hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			CString sName, sNewName;
			sName.Format(L"%s\\%s", CUtil::GetAppPath(), FindFileData.cFileName);
			sNewName.Format(L"%s\\%s_bkp_%s", sTempName, CUtil::GetCurrentTimeStamp(), FindFileData.cFileName);

			MoveFile(sName, sNewName);
		}
		while(FindNextFile(hFind, &FindFileData));
	}

	sFileName.Format(L"%s\\*_log.txt", sTempName);

	hFind = FindFirstFile(sFileName, &FindFileData);

	if(hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			CString sFileDate;
			ft = FindFileData.ftCreationTime;
			SYSTEMTIME st;
			
			FileTimeToLocalFileTime(&ft, &ft_local);
			FileTimeToSystemTime(&ft_local, &st);
			sFileDate.Format(L"%d/%d/%d", st.wDay, st.wMonth, st.wYear);
			
			if(!CUtil::IsIntervaloDataTimeValid(sFileDate, 'D', iLogDays, nErrorDateDiff))
			{
				DeleteFile(sTempName + CString(L"\\") + CString(FindFileData.cFileName));
			}
		}
		while(FindNextFile(hFind, &FindFileData));

		FindClose(hFind);
	}

	wnd->Hide();
}


/**
\brief Formata uma URL colocando o servidor correspondente.
\param CString& sUrl: URL a ser formatada.
\return CString: URL formatada
*/
CString CPMAApp::GetUrl(CString& sUrl)
{
	CString sTmpsrv, sRet, sVal; 
	sRet = sUrl;


	sTmpsrv = sUrl.Left(sUrl.Find(L'$', 1));
	sTmpsrv.Replace(L"$", L"");
	sVal = m_init.GetServer(sTmpsrv);
	sTmpsrv.Format(L"$%s$", sTmpsrv);
	
	sRet.Replace(sTmpsrv, sVal);

	return sRet;
}

/**
\brief Prepara todas as URLs para serem passadas como parâmetro para um módulo.
\param CModParam *__p: Ponteiro para os parâmetros que serão passados aos módulos.
\return void
*/
void CPMAApp::AddURLS(CModParam *__p)
{
	// Adicionar todas as urls...
	__URLSMAP *pMap = theApp.GetInitInfo()->GetURLMapPtr();
	POSITION p = pMap->GetStartPosition();
	while(p)
	{
		CString sURL, sKey, sValue;
		pMap->GetNextAssoc(p, sKey, sValue);

		sURL = GetUrl(sValue);

		//sTmpsrv = sValue.Left(sValue.Find(L'$', 1));
		//sTmpsrv.Replace(L"$", L"");
		//pMapServer->Lookup(sTmpsrv, sServer);

		//// Adiciona o server...
		//sURL = sServer;
		//if(sURL.GetAt(sURL.GetLength()-1) != '/')
		//	sURL += L"/";
		//sURL += sValue;

		__p->AddPair(sKey, sURL);
	}
}
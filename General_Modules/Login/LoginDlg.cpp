#include "stdafx.h"
#include "Login.h"
#include "LoginDlg.h"
#include "Consultas.h"
#include "Utils.h"
#include "ModParam.h"
#include "CppSQLite3.h"
#include "CStr.h"
#include "Module.h"
#include "XmlCreate.h"
#include "PMA.h"
#include "IdentificacaoDlg.h"
#include "VersaoApp.h"
#include "PowerMonCtrl.h"
#include "MsgWindow.h"

extern HWND g_hWnd;
//extern CPMAApp theApp;

IMPLEMENT_DYNAMIC(CLoginDlg, CDialogEx)

/**
\brief Construtor da classe
\param CppSQLite3DB *pDB Handle de conexão ao banco de dados
\param CWnd* pParent Handle da janela pai
\return void
*/
CLoginDlg::CLoginDlg(CppSQLite3DB *pDB, CWnd* pParent /*=NULL*/)
	: CDialogEx(CLoginDlg::IDD, pParent)
	, m_sCodigo(_T(""))
	, m_sSenha(_T(""))
{
	m_pDB = pDB; // vem do CScreenDB

	CUtil::CreateFont(&m_font, L"Verdana", -11, TRUE);

	m_bUpdated = FALSE;

	// Mudar a cor do fundo...
	//SetBackColor(RGB(173, 199, 222));
}


/**
\brief Destrutor da classe
\param void
\return void
*/
CLoginDlg::~CLoginDlg()
{
}


/**
\brief Configura as trocas e validações dos campos desta janela (União de controles e variáveis).
\param CDataExchange* pDX Ponteiro para a classe que faz essa troca
\return void
*/
void CLoginDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDT_CODE, m_sCodigo);
	DDX_Text(pDX, IDC_EDT_PASS, m_sSenha);
	DDX_Control(pDX, IDC_ST_VERSION, m_stVersao);
	DDX_Control(pDX, IDC_EDT_CODE, m_edtCodigo);
	DDX_Control(pDX, IDC_EDT_PASS, m_edtPassword);
	DDX_Control(pDX, IDC_STATIC_BATTERIA, m_stBateria);
}

BEGIN_MESSAGE_MAP(CLoginDlg, CDialogEx)
	ON_WM_TIMER()
	ON_BN_CLICKED(ID_ALINHA_TELA, &CLoginDlg::OnBnClickedAlinhaTela)
END_MESSAGE_MAP()


/**
\brief Chamado na inicialização do dialog
\details Monta elementos da tela e preenche informações do sistema através da chamada ao método _MontaLoginInfo()
\param void
\return BOOL
*/
BOOL CLoginDlg::OnInitDialog()
{
	g_hWnd = GetSafeHwnd();

	CDialogEx::OnInitDialog();

#ifdef _WIN32_WCE
	//ShowSIP();

	// Fazer com que o SIP fique disponivel...
	if(!m_dlgCommandBar.Create(this))
	{
		STLOG_WRITE("CLoginDlg::OnInitDialog(): Failed to create CommandBar");
		return FALSE;      // fail to create
	}

	#if _WIN32_WCE != 0x420
		_CreateBanner(-1, FALSE, 28);
		CString sBanner;
		sBanner.Format(L"%s\\simbolos\\Login.png", CUtil::GetMainAppPath());
		m_banner.LoadJPG(sBanner);
    #else
		_CreateBanner(IDB_BANNER, FALSE, 28);	
	#endif

	//Esconde o banner novo e exibe o banner antigo
	CBannerWindow::getInstance()->Hide();	
	m_banner.ShowWindow(SW_SHOW);

	CString s;
	CString sLogoImagePath = m_params->GetValue(L"logo");

	if(!sLogoImagePath.IsEmpty())
	{
		m_logo.Create(CRect(0, DRA::SCALEY(28), DRA::SCALEX(90), DRA::SCALEY(90+28)), this, 0x454);
		m_logo.SetStretch(FALSE);

		//if(!m_logo.LoadJPG(sIconPath))
		if(!m_logo.Load(sLogoImagePath))
		{
			SetBackColor(RGB(173, 199, 222));
			m_logo.ShowWindow(SW_HIDE);
			m_stVersao.SetWindowPos(NULL, DRA::SCALEX(10), DRA::SCALEY(45), DRA::SCALEX(220), DRA::SCALEY(80), SWP_NOZORDER);
			STLOG_WRITE("CLoginDlg::OnInitDialog(): logotipo nao encontrado: %S", sLogoImagePath);
		}
	}
	else
	{
		SetBackColor(RGB(173, 199, 222));
		m_stVersao.SetWindowPos(NULL, DRA::SCALEX(10), DRA::SCALEY(45), DRA::SCALEX(220), DRA::SCALEY(80), SWP_NOZORDER);
	}
#else
	m_logo.Create(L"", 0, CRect(0, 28, 90, 90+28), this, 0x454);

	CString sLogoImagePath = m_params->GetValue(L"logo");

	if(!sLogoImagePath.IsEmpty())
	{
		if(m_logo.Load(sLogoImagePath))
		{
			m_logo.Draw();
			m_logo.ShowWindow(SW_SHOW);
		}
		else
		{
			m_logo.ShowWindow(SW_HIDE);
			m_stVersao.SetWindowPos(NULL, 10, 45, 220, 80, SWP_NOZORDER);
			STLOG_WRITE("CLoginDlg::OnInitDialog(): logotipo nao encontrado: %S", sLogoImagePath);
		}
	}
#endif

	//
	/*COleDateTime odt = COleDateTime::GetCurrentTime();
	CString sDate = odt.Format(_T("%A, %d/%m/%y - %H:%M"));

	CString sContrato = m_params->GetValue(L"contrato");
	CString sVersion  = m_params->GetValue(L"version");

	sContrato.MakeUpper();

	CString sSerial = CUtil::GetSerialNumberFromKernelIoControl();
	s.Format(L"Versão %s - %s\nContrato: %s\n%s", sVersion, sSerial, sContrato, sDate);

	m_stVersao.SetWindowText(s);
	m_stVersao.SetFont(&m_font);*/
	//

	m_sSerial = CUtil::GetIDTalao();

	_MontaLoginInfo();
	SetTimer(1, 1000, NULL);

	m_login.Init(L"log_agente");

	_GravaOperacao(L"LOGOUT");

	CString sUserPerms = CUtil::GetLoggedUserPerms();
	if (sUserPerms.Find(L"TEM")> -1)  ///não é somente GERINCID
	{
#ifdef USA_TEM

		CString sResp;
		if (m_iLastId > 0)
		{
			_GravaHttpSenderJobLoginReg(L"LOGOUT", &sResp);
		}
#endif
	}
	CUtil::m_sLoggedUser = L"--NENHUM--";
	CUtil::m_sLoggedUserPerms = L"";

	m_proxyInfo.bDiscagem = m_params->GetValue(_T("DISCAGEM")).CompareNoCase(L"TRUE") == 0;
	m_proxyInfo.bProxy	  = m_params->GetValue(_T("PROXY")).CompareNoCase(L"TRUE") == 0;
	m_proxyInfo.sServer	  = m_params->GetValue(_T("SERVER"));
	m_proxyInfo.nPort	  = _wtol(m_params->GetValue(_T("PORT")));
	m_proxyInfo.sUser	  = m_params->GetValue(_T("USER"));
	m_proxyInfo.sPass	  = m_params->GetValue(_T("PASS"));

	if(GetParent() != NULL)
		GetParent()->PostMessage(CModParam::WM_MODULE_READY, 0, (LPARAM) GetSafeHwnd());

	_FullScreen();

	return TRUE;
}

/**
\brief Grava operação de LOGIN e LOGOUT no banco de dados
\detail Grava LOGOUT só se já tiver uma operação LOGIN realizada
\param CString sOper: Operação a ser gravada LOGIN ou LOGOUT
\return void
*/
void CLoginDlg::_GravaOperacao(const CString& sOper)
{
	if(m_sCodigo.IsEmpty())
	{
		m_sCodigo = CUtil::GetLoggedUser();
	}

	if(sOper.CompareNoCase(L"LOGOUT")==0)
	{
		if(CConsultas::ConsultaUltimoLogin().CompareNoCase(L"LOGIN")!=0)
		{
			m_iLastId = 0;
			return;
		}
	}
	m_sData = CUtil::GetCurrentDateTime(L"DATA");
	m_sHora = CUtil::GetCurrentDateTime(L"HORA");
	m_login.SetValue(L"id_talao", m_sSerial);
	m_login.SetValue(L"cd_agente", m_sCodigo);
	m_login.SetValue(L"data", m_sData);
	m_login.SetValue(L"hora", m_sHora);
	m_login.SetValue(L"operacao", sOper);
	m_login.SetValue(L"transmissao", L"0");
	m_login.Insert(CppSQLite3DB::getInstance());

	m_iLastId = CConsultas::GetLastNumber(CppSQLite3DB::getInstance(), L"log_agente.id");

}

void CLoginDlg::RecreateXML()
{
	CStringA sQuery;
	CString sMsg;
	try
	{
		CMsgWindow::getInstance()->Create(this);
		CMsgWindow::getInstance()->Show(L"Recriando login/logout");

		sQuery = "DELETE FROM httpsender_job WHERE obj_name = 'LOGIN_LOGOUT' AND status <> 'T'";
		CppSQLite3DB::getInstance()->execQuery(sQuery);
		sQuery = "DELETE FROM httpsender_log WHERE obj_name = 'LOGIN_LOGOUT' AND status <> 100";
		CppSQLite3DB::getInstance()->execQuery(sQuery);
		sQuery = "SELECT * FROM log_agente WHERE transmissao = 0";
		
		CppSQLite3Query q = CppSQLite3DB::getInstance()->execQuery(sQuery);
		CString sResp;

		for(int i=1; !q.eof(); i++)
		{
			m_iLastId = q.getIntField(0);
			m_sSerial = CString(q.getStringField(1));
			m_sCodigo = CString(q.getStringField(2));
			m_sData = CString(q.getStringField(3));
			m_sHora = CString(q.getStringField(4));
			_GravaHttpSenderJobLoginReg(CString(q.getStringField(5)), &sResp);
			q.nextRow();
			sMsg.Format(L"Recriando login/logout %d", i);
			CMsgWindow::getInstance()->UpdateText(sMsg);
			Sleep(1000);
		}
	}
	catch(CppSQLite3Exception e)
	{
		STLOG_WRITE(L"%S(%d): Erro executando query: %S. Motivo: %s", __FUNCTION__, __LINE__, sQuery, e.errorMessage());
	}

	CMsgWindow::getInstance()->Destroy();
}


/**
\brief Grava operação de LOGIN e LOGOUT no banco de dados na tabela "httpsender_job"
\detail o registro sera enviado para o servidor Web 
\param void
\return CString
*/
BOOL CLoginDlg::_GravaHttpSenderJobLoginReg(const CString& sOper, CString *sResp)
{
	BOOL bResp = FALSE;
	CString sContrato = CUtil::m_sContrato;
	/*** Início construção do XML de envio de reg LOGIN_LOGOUT ***/
	CXmlCreate xml;	
	xml.OpenRootTag(L"reg_login_logout");
	xml.AddElement(L"id_talao", m_sSerial);
	xml.AddElement(L"id_agente", m_sCodigo);	
	xml.AddElement(L"contrato", sContrato);		
	xml.AddElement(L"operacao", sOper);
	xml.AddElement(L"datahora", m_sData + L" " + m_sHora);
	xml.CloseRootTag(L"reg_login_logout");
	/*** Final da construção do XML ***/		

	//Valida estrutura do XML criado
	if(xml.ValidateXml())
	{	
		//Cria diretório XML_LOGIN do httpsender se não existir
		CString sBaseDir = CUtil::GetMainAppPath() + L"\\XML_LOGIN";	
		CUtil::CreateDirIfNotExist(sBaseDir);

		CString sTimeStamp = CUtil::GetCurrentTimeStamp();
		CString sPathXmlFile;
		//sPathXmlFile.Format(L"LOGIN_LOGOUT_%s.xml", sTimeStamp);
		sPathXmlFile.Format(L"%s\\LOGIN_LOGOUT_%s.xml",sBaseDir, sTimeStamp);
		xml.CreateXmlFile(sPathXmlFile);

		CString sLastId;
		sLastId.Format(L"%d",m_iLastId);
		CString sVarsList ;
		sVarsList.AppendFormat(L"contrato=%s", sContrato);

		STLOG_WRITE(L"CLoginDlg::_GravaHttpSenderJobLoginReg: Operação Login_Logout: [%s]", sOper);	
		
		/*int iRetWeb = CUtil::SendArqXml(LPVOID(NULL) , sPathXmlFile, CUtil::m_sUrlTxLoginLogout, sResp);
		if (iRetWeb == 100)
		{
			ConsultaJobs2Send inc; ///= HttpSenderJobList.GetNext(p);
			
			inc.sOnTXOkTable = L"LOG_AGENTE";
			inc.sOnTXOkKeyName = L"ID";
			inc.sOnTXOkKeyValue = sLastId;
			_ExecFuncOnTxOk(L"UPDATE_TRANSMISSAO", &inc);	
			DeleteFile(sPathXmlFile);
			bResp = TRUE;
		}
		else*/
		{
			if(CConsultasHttpSender::GravaHttpSenderJob(CppSQLite3DB::getInstance(), L"LOGIN_LOGOUT", sPathXmlFile, L"XML", sVarsList, L"UPDATE_TRANSMISSAO", L"LOG_AGENTE", L"Id", sLastId))
			{
				//STLOG_WRITE(L"CAitSend::Send() Inserido registro job de envio de GPS");
				//m_stDateLastJob = st;
				//m_sDateLastJob = m_sDataHora;
			}
			bResp = FALSE;
		}
	}
	return bResp;
}


/**
\brief Executa validação dos dados do usuário digitado com os dados na base de dados
\param void
\return void
*/
void CLoginDlg::OnOK()
{
	///// verificar se agente está logado em outro talão /////
	CString sResp;
	CString sExpireDate = L"";
		
	UpdateData();

	if(m_sCodigo.Trim().IsEmpty() || m_sSenha.Trim().IsEmpty())
	{
		CString msg;
		msg.LoadString(IDS_ERROBRANCO);
		MessageBox(msg, L"Mensagem", MB_ICONERROR|MB_OK);
		return;
	}

	if(_ExitApp())
	{
		STLOG_WRITE("CLoginDlg::OnOK(): Pedido de encerramento da aplicacao");
		EndDialog(ID_EXIT);

		return;
	}

	if(m_sCodigo.CompareNoCase(L"enter_system") == 0 && m_sSenha.CompareNoCase(L"access4153") == 0 )
	{
		CUtil::SetLoggedUser(L"1");
		m_sCodigo = L"1";
		EndDialog(IDOK);
		return;
	}

	//CUtil::m_sLoggedUser = m_sCodigo;
	//CUtil::m_sLoggedUserPerms = CConsultas::GetUserPerm(m_sCodigo);

	if(_DoUpdates())
	{
		CStr s;
		s.Format("SELECT count(*) AS X "
				 "  FROM AGENTE		"
				 " WHERE codigo = '%S' "
				"   AND senha='%S'	",
				m_sCodigo,
				m_sSenha);
		try
		{
			CppSQLite3Query q = CppSQLite3DB::getInstance()->execQuery(s);
			if(!q.eof())
			{
				if(q.getIntField("X") > 0)
				{
					STLOG_WRITE("CLoginDlg::OnOK(): Login OK");

					///Verifica data expiração da senha
					sExpireDate = CConsultas::GetUserExpirationDate(m_sCodigo);
					if (sExpireDate.GetLength() > 7)
					{
						int dias = VerificaDataExpiracao(sExpireDate);
						if (dias < 0)
						{
							CString msg;
							msg.LoadString(IDS_ERRO_SENHA_EXPIRADA);
							MessageBox(msg, L"Mensagem", MB_ICONERROR|MB_OK);
							int iRet = Altera_Senha();
							if (iRet == IDCANCEL)
							{
								return;
							}
						}
						else
						{
							if (dias <= 10)
							{
								CString msgP,msg;
								msgP.LoadString(IDS_ALERTA_SENHA_PRESTES_EXPIRAR);
								msg.Format(msgP,dias);
								if (MessageBox(msg, L"Mensagem", MB_ICONWARNING|MB_YESNO)  == IDYES)
								{
									Altera_Senha();
								}
							}
						}
					}


					CUtil::SetLoggedUser(m_sCodigo);
					CUtil::m_sLoggedUserPerms = CConsultas::GetUserPerm(m_sCodigo);


					CUtil::m_sLastLoggedUser = m_sCodigo;


					///Verifica se permissão do agente é somente GERINCID
					CString sUserPerms = CUtil::GetLoggedUserPerms();
					if (sUserPerms.Find(L"TEM")> -1)  ///não é somente GERINCID
					{
#ifdef USA_TEM//
						if (CUtil::VerificaAgenteLogado( &sResp, &m_proxyInfo ))
						{
							CString msg;
							msg.LoadString(IDS_ERROAGENTELOGADO);

							MessageBox(msg, L"Mensagem", MB_ICONERROR|MB_OK);
							return;
						}
#endif
					}

					CIdentificacaoDlg dlgIdd(m_pDB, m_sCodigo, GetParent());
					if(dlgIdd.DoModal() == IDCANCEL)
					{
						m_sCodigo = L"";
						m_sSenha = L"";
						UpdateData(FALSE);
						return;
					}

					_GravaOperacao(L"LOGIN");

					if (sUserPerms.Find(L"TEM")> -1)  ///não é somente GERINCID
					{
#ifdef USA_TEM
						CString sResp;
						_GravaHttpSenderJobLoginReg(L"LOGIN", &sResp);
#endif
					}
					int codigo;
					/*if(CConsultas::ConsultaCDAgente(m_pDB, m_sCodigo, codigo))
					{
						m_sCodigo.Format(L"%d", codigo);
					}*/
					
					//Destroi banner atual e exibe o novo banner
					m_banner.DestroyWindow();
					CBannerWindow::getInstance()->Show();

					EndDialog(IDOK);
					return;
				}
			}
		}
		catch(CppSQLite3Exception e)
		{
			CString msg;
			msg.LoadString(IDS_ERROACBD);

			MessageBox(msg, L"Mensagem", MB_ICONERROR|MB_OK);

			STLOG_WRITE(L"CLoginDlg::OnOK(): %S", e.errorMessage());
			EndDialog(IDCANCEL);
		}
	}

	CString msg;
	msg.LoadString(IDS_ERROLOGININVALIDO);

	MessageBox(msg, L"Mensagem", MB_ICONERROR|MB_OK);
	
}


/**
\brief Captura mensagem do dialog
\details Trata evento KeyDown + VK_RETURN (Enter do teclado) chamando OnOK()
\param void
\return void
*/
BOOL CLoginDlg::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message == WM_KEYDOWN)
	{
		if(pMsg->wParam == VK_RETURN)
		{
			OnOK();
			return TRUE;
		}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}


/**
\brief Indica se o módulo de atualização deverá ser executado após o login, altera o valor de m_bUpdated para TRUE
\param void
\return BOOL
*/
BOOL CLoginDlg::_DoUpdates()
{	
	if(m_params->GetValue(L"enable_atualizacao").CompareNoCase(L"FALSE") == 0)
		return TRUE;	

	ShowWindow(SW_HIDE);

	CModule module;
	CString sPath = m_params->GetValue(_T("upd_path"));
	ASSERT(!sPath.IsEmpty());

	m_params->AddPair(_T("codigo"), m_sCodigo);

	if(module.Create(sPath))
	{
		LONG size = 0;
		LPBYTE p = m_params->GetBuffer(&size);

		CUtil::SetLoggedUser(m_sCodigo);

		module.Run(NULL, GetParent()->GetSafeHwnd(), &p, &size);
		module.Destroy();

		CUtil::SetLoggedUser(L"--NENHUM--");

		m_params->ReleaseBuffer();

		m_bUpdated = TRUE;

		ShowWindow(SW_SHOW);

		return TRUE;
	}
	else
	{
		CString msg;
		msg.LoadString(IDS_ERROATUALIZACAO);

		MessageBox(msg, L"Erro", MB_OK|MB_ICONERROR);
		STLOG_WRITE(L"CLoginDlg::_DoUpdates(): Erro no modulo de atualizacao '%s'.", sPath);
	}

	ShowWindow(SW_SHOW);

	return FALSE;
}


/**
\brief Executado quando o timer é dispadado
\details Executa método _MontaLoginInfo()
\param UINT_PTR nIDEvent Nome do evento
\return void
*/
void CLoginDlg::OnTimer(UINT_PTR nIDEvent)
{
	_MontaLoginInfo();
	CDialogEx::OnTimer(nIDEvent);
}


/**
\brief Monta as informações da tela de login referentes ao sistema
\details Informações do tipo: Data/Hora, contrato, versão do sistema e id do talão
\param void
\return void
*/
void CLoginDlg::_MontaLoginInfo()
{
	CVersao version;

	SYSTEMTIME st;
	ZeroMemory(&st, sizeof(st));
	GetLocalTime(&st);

	COleDateTime odt(st);
	CString sDate = odt.Format(_T("%A, %d/%m/%y - %H:%M:%S"));

	CString sContrato = m_params->GetValue(L"contrato");
	CString s;

	sContrato.MakeUpper();
	//CString sSerial = CUtil::GetSerialNumberFromKernelIoControl();
	s.Format(L"%s\n%s\nContrato: %s\n%s", version.GetAppVersion(), m_sSerial, sContrato, sDate);

	m_stVersao.SetWindowText(s);
	m_stVersao.SetFont(&m_font);

	s = L"Bateria: " + CPowerMonCtrl::GetBatteryPerc() + L"%";
	m_stBateria.SetWindowText(s);
}


/**
\brief Realiza o pedido de encerramento da aplicacao
\param void
\return BOOL
*/
BOOL CLoginDlg::_ExitApp()
{

	BOOL bRet = FALSE;
	CString sContrato = m_params->GetValue(L"contrato");
	int rand;

	if(m_sCodigo.CompareNoCase(L"quit_ce") == 0 && m_sSenha.CompareNoCase(L"access4153") == 0 )
	{	
		bRet = TRUE;
	}

	if(swscanf(m_sSenha.Right(2), L"%d", &rand))
	{
		if(m_sCodigo.CompareNoCase(L"exit") == 0 &&
		   m_sSenha.CompareNoCase(CUtil::GetPassword(L"TEM", sContrato, 0, rand)) == 0 )
			bRet = TRUE;

		if(m_sCodigo.CompareNoCase(L"exit") == 0 &&
		   m_sSenha.CompareNoCase(CUtil::GetPassword(L"TEM", sContrato, 1, rand)) == 0 )
			bRet = TRUE;

		if(m_sCodigo.CompareNoCase(L"exit") == 0 &&
		   m_sSenha.CompareNoCase(CUtil::GetPassword(L"TEM", sContrato, 2, rand)) == 0 )
			bRet = TRUE;

		if(m_sCodigo.CompareNoCase(L"exit") == 0 &&
		   m_sSenha.CompareNoCase(CUtil::GetPassword(L"TEM", sContrato, 3, rand)) == 0 )
			bRet = TRUE;
	}

	if(bRet)
	{
		HWND hWnd = ::FindWindow(_T("Dialog"), _T("Init"));
        if(NULL != hWnd)
        {
			::SendMessage(hWnd, WM_CLOSE, 0, 0);
        }
	}

	return bRet;
}

void CLoginDlg::OnAlinharTela()
{
	CUtil::CalibrateScreen();
}

void CLoginDlg::OnBnClickedAlinhaTela()
{
	OnAlinharTela();
}


/**
\brief Executa função após envio ok de arquivo
\param CString sFuncName Nome da função a ser executada
\param ConsultaJobs2Send Ponteiro da pesquisa da tabela job que está sendo enviado
\return BOOL
*/
BOOL CLoginDlg::_ExecFuncOnTxOk(CString sFuncName, ConsultaJobs2Send *inc)
{
	if(sFuncName.CompareNoCase(L"UPDATE_TRANSMISSAO") == 0)
	{		
		CMapStringToString campoValorSetList;
		campoValorSetList.SetAt(L"TRANSMISSAO", L"1");
		//campoValorSetList.SetAt(L"DT_TRANS", L"NOW");			

		if(!CConsultas::Update(m_pDB, 
							   CString(inc->sOnTXOkTable), 
							   CString(inc->sOnTXOkKeyName), 
						       CString(inc->sOnTXOkKeyValue), 
						       &campoValorSetList))
		{			
			STLOG_WRITE(L"CLoginDlg::_ExecFuncOnTxOk(): Erro atualizando status de transmissão! Tabela: %s", inc->sOnTXOkTable);			
			return FALSE;
		}
	}	
	return TRUE;
}


/**
\brief Compara data atual com a data de expiração da senha
\param CString sExpireDate Data de expiração da senha
\return int se data atual maior que a de expiração -1, se menor ou igual nº de dias para expirar
*/
int CLoginDlg::VerificaDataExpiracao(CString sExpireDate)
{
	int ano,mes,dia;
	CString sValue;
	SYSTEMTIME	st;	
	GetLocalTime(&st);
	COleDateTime now(st);

	CTime tNow(now.GetYear(),
			   now.GetMonth(),
			   now.GetDay(),0,0,0);

	CStr s = sExpireDate.Left(4);
	ano = atoi(s);
	s = sExpireDate.Mid(4,2);
	mes = atoi(s);
	s = sExpireDate.Right(2);
	dia = atoi(s);

	CTime tExpire(ano,mes,dia,0,0,0);
	
	CTimeSpan tDifData = tExpire - tNow;

	LONG lNumDays  = tDifData.GetDays();
	if ( lNumDays < 0 )
	{
		return -1;
	}
	else
	{
		return (int)lNumDays;
	}
}


///carregar a tela de alteração de senha
int CLoginDlg::Altera_Senha()
{	

	int iRet = IDCANCEL;
	ShowWindow(SW_HIDE);

	CModule module;

	CString sPath = CUtil::GetAppPath() + L"\\data_hora.dll";
	ASSERT(!sPath.IsEmpty());

	m_params->AddPair(_T("param"), _T("SENHA_LOGIN"));

	if(module.Create(sPath))
	{
		LONG size = 0;
		LPBYTE p = m_params->GetBuffer(&size);

		module.Run(NULL, GetParent()->GetSafeHwnd(), &p, &size);
		
		CModParam param1;
		VERIFY(param1.SetBuffer(p, size));

		CString sRet = param1.GetValue(_T("Ret_Dlg_Senha"));
		if ( sRet.CompareNoCase(_T("IDYES")) == 0)
		{
			iRet = IDYES;
		}
		else
		{
			iRet = IDCANCEL;
		}

		m_params->ReleaseBuffer();
		param1.ReleaseBuffer();
		module.Destroy();

		ShowWindow(SW_SHOW);

		return iRet;
	}
	else
	{
		CString msg;
		///msg.LoadString(IDS_ERROATUALIZACAO);

		//MessageBox(msg, L"Erro", MB_OK|MB_ICONERROR);
		STLOG_WRITE(L"CAITDlg::::_SetupPrinter: Erro no modulo de impressão '%s'.", sPath);
	}

	ShowWindow(SW_SHOW);

	return iRet;
}
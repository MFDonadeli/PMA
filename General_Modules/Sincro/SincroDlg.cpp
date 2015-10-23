#include "stdafx.h"
#include "constants.h"
#ifdef _WIN32_WCE
	#include "DIBSectionLite.h"
#endif
#include "ModParam.h"
#include "Sincro.h"
#include "SincroDlg.h"
#include "CppSQLite3.h"
#include "Consultas.h"
#include "Module.h"
#include "ConsultasHttpSender.h"
#include "InfoDlg.h"

#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif

#define ID_TIMER_UPDATE		13

extern HWND g_hWnd;
CMsgWindow* CSincroDlg::m_wnd;
IMPLEMENT_DYNAMIC(CSincroDlg, CDialogEx)

/**
\brief Construtor da classe
\param CppSQLite3DB *pDB Handle de conexão ao banco de dados
\param LPCTSTR szCodigo Código do agente logado
\param CWnd* pParent Parent Window
\return void
*/
CSincroDlg::CSincroDlg(CppSQLite3DB *pDB, LPCTSTR szCodigo, CWnd* pParent /*=NULL*/)
	: CDialogEx(CSincroDlg::IDD, pParent)
{
	m_pDB = pDB;
	m_sCodigo = szCodigo;
}

/**
\brief Destrutor da classe
\param void
\return void
*/
CSincroDlg::~CSincroDlg()
{
}


/**
\brief Chamado pelo framework para troca e validação dos dados do dialog
\param CDataExchange* pDX
\return void
*/
void CSincroDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_list);	
	DDX_Control(pDX, IDC_LIST_OBJ, m_lstObj);
	DDX_Control(pDX, IDC_EDIT_STATUS, m_edtStatus);
	DDX_Control(pDX, IDC_EDIT_STATUS2, m_edtInfo);
}

BEGIN_MESSAGE_MAP(CSincroDlg, CDialogEx)
	ON_WM_SIZE()
	ON_WM_TIMER()
	ON_NOTIFY(NM_CLICK, IDC_LIST1, &CSincroDlg::OnItemClick)	
	ON_COMMAND(ID_FECHAR, &CSincroDlg::OnBnClickedButton1)
	ON_COMMAND(ID_MENU_CRIARENVIO, &CSincroDlg::OnRecreateXML)
	ON_COMMAND(ID_MENU_TENTARREENVIO, &CSincroDlg::OnRetrySend)
	ON_COMMAND(ID_MENU_INFO, &CSincroDlg::OnInfo)
	ON_LBN_SELCHANGE(IDC_LIST_OBJ, &CSincroDlg::OnLbnSelchangeListObj)
END_MESSAGE_MAP()


/**
\brief Chamado na inicialização do dialog
\details Monta elementos da tela e preenche lista através da chamada ao método _FillList()
\param void
\return void
*/
BOOL CSincroDlg::OnInitDialog()
{
	g_hWnd = GetSafeHwnd();
	m_wnd->Hide();

	CDialogEx::OnInitDialog();

#ifdef _WIN32_WCE
	HideSIP();

	if(!m_dlgCommandBar.Create(this))
	{
		AfxMessageBox(IDS_COMMANDBAR_ERROR);
		STLOG_WRITE("CSincroDlg::OnInitDialog() : Failed to create CommandBar");
		EndDialog(IDCANCEL);
		return FALSE;      // fail to create
	}

	if(!m_dlgCommandBar.InsertMenuBar(IDR_MENU_SINC))
	{
		AfxMessageBox(IDS_COMMANDBAR_ERROR);
		STLOG_WRITE("CSincroDlg::OnInitDialog() : Failed to InsertMenuBar CommandBar");
		EndDialog(IDCANCEL);
		return FALSE;      // fail to create
	}

	_CreateBanner(L"Sincronização");

#endif
#if 0
	CRect rectB;
	m_banner.GetClientRect(&rectB);

	VERIFY(m_panel.Create(this, 
				   CRect(0, 
				   rectB.Height(), 
				   rectB.Width(), 
				   rectB.Height() + 20), 
				   (UINT)-1));

	m_panel.SetBkColor(RGB(200, 200, 200));
#endif
	
	CString sContrato = m_params->GetValue(L"contrato");
	CString sNumThreads = m_params->GetValue(L"num_httpsenderthread");

	m_list.SetExtendedStyle(LVS_EX_FULLROWSELECT/*|LVS_EX_GRIDLINES*/);

#ifdef _WIN32_WCE
	m_list.InsertColumn(0, L"Arquivo", LVCFMT_LEFT, DRA::SCALEX(148));	
	m_list.InsertColumn(1, L"Data", LVCFMT_LEFT, DRA::SCALEX(87));	
#else
	m_list.InsertColumn(0, L"Arquivo", LVCFMT_LEFT, 148);	
	m_list.InsertColumn(1, L"Data", LVCFMT_LEFT, 87);	
#endif
	
	//Objetos do combo
	//_GetHttpSenderThreadList();
	
	//Preenche lista
	if(!_FillList())
	{
		m_wnd->Destroy();

		// Avisar o pai que a inicializacao encerrou...
		// Encerra o Wait cursor...
		if(GetParent() != NULL)
		{
			GetParent()->PostMessage(CModParam::WM_MODULE_READY, 
									 0, 
									 (LPARAM) GetSafeHwnd());
		}

		CString msg;
		msg.LoadString(IDS_ERRO_DADOS);

		::MessageBox(GetSafeHwnd(), msg, L"PMA", MB_ICONINFORMATION);
	}

	SetForegroundWindow();  
	// Resize the window over the taskbar area.
	CRect rect;
	rect = CRect(0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
	MoveWindow(&rect, TRUE);
#ifdef _WIN32_WCE
	SHFullScreen(m_hWnd, SHFS_HIDESTARTICON|SHFS_HIDETASKBAR|SHFS_SHOWSIPBUTTON);
#endif

	m_wnd->Destroy();

	// Avisar o pai que a inicializacao encerrou...
	// Encerra o Wait cursor...
	if(GetParent() != NULL)
	{
		GetParent()->PostMessage(CModParam::WM_MODULE_READY, 
								 0, 
								 (LPARAM) GetSafeHwnd());
	}	
	_FullScreen();

	//Timer de atualização da lista
	SetTimer(ID_TIMER_UPDATE, 30000, NULL);

	return TRUE;
}

/**
\brief Chamado pelo framework após o tamanho da dialog ser alterado
\details Reposiciona o listctrl para ocupar a tela toda. É disparado automaticamento quando a mensagem ON_WM_SIZE é enviada à aplicação
\param UINT nType
\param int cx  
\param int cy
\return void
*/
void CSincroDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	// Reposicionar o grid para ocupar a tela toda...
	if(IsWindow(GetSafeHwnd()) && IsWindow(m_list.GetSafeHwnd()))
	{
		// Dados da area de trabalho...
		CRect r;
		VERIFY(SystemParametersInfo(SPI_GETWORKAREA, 0, &r, NULL) == 1);

#ifdef _WIN32_WCE
		// Dados do SIP...
		SIPINFO si = {0};
		si.cbSize = sizeof(si);
		SHSipInfo(SPI_GETSIPINFO, 0, &si, 0);
		BOOL bSIPVisible = si.fdwFlags & SIPF_ON;

		CRect rect = CRect(0, 
			               DRA::SCALEY(56),
						   cx,
						   bSIPVisible ? cy : r.Height()); 

		//m_list.MoveWindow(rect.left, rect.top, rect.Width(), DRA::SCALEY(106));
#else
		CRect rect = CRect(0, 
			               56,
						   cx,
						   cy); 

		m_list.MoveWindow(rect.left, rect.top, rect.Width(), 106);
#endif
		
		/*m_btnCancelar.SetWindowPos(NULL, 
								   rect.left + 5 + rcBtn.Width() + 10,
								   rect.Height() + 2,
								   0,
								   0,
								   SWP_NOZORDER|SWP_NOSIZE);*/
	}
}


/**
\brief Fecha dialog
\param void
\return void
*/
void CSincroDlg::OnBnClickedButton1()
{		
	EndDialog(IDCANCEL);
}

/**
\brief Timer que é dispadado na atualização da lista
\details Executa método _Process()
\param UINT_PTR nIDEvent Nome do evento de atualização ID_TIMER_UPDATE
\return void
*/
void CSincroDlg::OnTimer(UINT_PTR nIDEvent)
{
	//Atualização da lista
	if(nIDEvent == ID_TIMER_UPDATE)
	{		
		if(m_list.GetItemCount() > 0)
		{
			m_list.DeleteAllItems();
			m_edtStatus.SetWindowText(L"");
		}
		_FillList();		
	}

	CDialogEx::OnTimer(nIDEvent);
}


/**
\brief Realiza consulta à base de dados e preenche lista com informações das AITs a ser sincronizadas
\details A consulta à base de dados é feita através do método CConsultas_TEM::GetAITSincro()
\param void
\return void
*/
BOOL CSincroDlg::_FillList()
{
	CString sObjName, sCmbOpt;
	TCITEM tabItem;

	m_nCurSel = m_lstObj.GetCurSel();

	_GetHttpSenderThreadList();

	if(m_nCurSel == LB_ERR)
		m_nCurSel = 0;

	m_lstObj.SetCurSel(m_nCurSel);

	sObjName = m_arrObjs.GetAt(m_nCurSel)->m_sThrdName;
	m_HttpSenderItems.Lookup(sCmbOpt, sObjName);
	
	//Lista de arquivos e seus dados ainda não enviados pela thread HttpSender. Filtrado por nome do objeto (sObjName)
	__CONSULTAHTTPSENDERFILELIST HttpSenderFileList;

	//Pesquisa por todos os arquivos ainda não enviados
	if(!CConsultas::GetHttpSenderFilesNotSent(m_pDB, sObjName, &HttpSenderFileList))
	{
		// SQL Error...
		STLOG_WRITE(L"CSincroDlg::_FillList(): Erro executando CConsultasGerIncid::GetHttpSenderFilesNotSent() para obj: %s", sObjName);

		CString msg;
		msg.LoadString(IDS_ERRO_ARQUIVOS);

		MessageBox(msg, L"Erro", MB_ICONERROR|MB_OK);
		_FullScreen();
		return FALSE;
	}

	POSITION p = HttpSenderFileList.GetHeadPosition();	
	while(p)
	{			
		ConsultaHttpSenderRegs *inc = HttpSenderFileList.GetNext(p);

		//Separa somente nome do arquivo
		CString sOnlyFileName;
		CString sFileName = CString(inc->sFileName);
		sOnlyFileName = sFileName.Right(sFileName.GetLength() - (sFileName.ReverseFind('\\')+1));
		
		int idx = m_list.InsertItem(m_list.GetItemCount(), sOnlyFileName);
		
		CString sData = CString(inc->sDataLog);
		m_list.SetItemText(idx, 1, sData);
		
		m_list.SetRedraw(TRUE);
		m_list.RedrawWindow(NULL, NULL);

		//Alimenta array de resposta do envio p/ posterior exibição em OnItemClick()
		m_ItemStatus.SetAt(sOnlyFileName, CString(inc->sRespServer));
	}		

	//Informações do último envio	
	CString sData, sTime;
	CString sDataHora;
	if(CConsultas::ConsultaLastEnvioHttpSender(m_pDB, sObjName, sData, sTime))
	{
		sDataHora.Format(L"%s %s", sData, sTime);
	}	

	//Preenche informações da Thread
	CString sInfo;
	sInfo.Format(L"-A enviar: %d\r\n-Com erro: %d\r\n-Última execução: %s\r\n-Último envio: %s",
		m_arrObjs.GetAt(m_nCurSel)->m_nToSend, m_arrObjs.GetAt(m_nCurSel)->m_nSendError, m_arrObjs.GetAt(m_nCurSel)->m_TimeStamp, m_arrObjs.GetAt(m_nCurSel)->m_SendTime);

	m_edtInfo.SetWindowText(sInfo);

	return TRUE;
}




void CSincroDlg::OnItemClick(NMHDR *pNMHDR, LRESULT *pResult)
{	
	*pResult = 0;

	m_edtStatus.SetWindowText(L"");

	if(m_list.GetSelectedCount() > 0)
	{
		POSITION p = m_list.GetFirstSelectedItemPosition();
		int selected = m_list.GetNextSelectedItem(p);

		//CString m_FileSelected = m_list.GetItemText(selected, 1);
		CString m_FileSelected = m_list.GetItemText(selected, 0);

		CString m_sText;
		m_ItemStatus.Lookup(m_FileSelected, m_sText);
		
		m_edtStatus.SetWindowText(m_sText);
	}	
}

void CSincroDlg::_GetThreadInfo()
{
	CString ret=L"";
	ThrdInfo* thrdInfo;
	CString sName;
	int iCount = 0;
	int iCountErr = 0;
	DWORD dwExit;

	CStringA sQuery("SELECT * FROM httpsender_info ORDER BY obj_name");

	try
	{
		if(!sQuery.IsEmpty())
		{
			CppSQLite3Query q = CppSQLite3DB::getInstance()->execQuery(sQuery);
			while(!q.eof())
			{
				thrdInfo = new ThrdInfo();

				GetExitCodeThread((HANDLE) q.getIntField(2), &dwExit);
				CConsultasHttpSender::GetGPSJobsCount(CString(q.getStringField(0)), iCount, iCountErr);

				thrdInfo->m_sThrdName = q.getStringField(0);
				thrdInfo->m_nToSend = iCount;
				thrdInfo->m_nSendError = iCountErr;
				thrdInfo->m_TimeStamp = q.getStringField(3);
				thrdInfo->m_SendTime = q.getStringField(4);

				m_arrObjs.Add(thrdInfo);

				q.nextRow();
			}

			q.finalize();

		}
	}
	catch(CppSQLite3Exception e)
	{
		STLOG_WRITE("CConsultas::DoQuery: ERRO executando query", 
					e.errorMessage());

		STLOG_WRITE(sQuery);
	}

}

void CSincroDlg::_GetHttpSenderThreadList()
{
	int iNumThreads;
	CString sNumThreads = m_params->GetValue(L"num_httpsenderthread");
	CStr x = sNumThreads;
	iNumThreads = atoi(x);

	for(int i=0; i<m_arrObjs.GetCount(); i++)
	{
		ThrdInfo* info = m_arrObjs.GetAt(i);
		delete info;
	}
	m_arrObjs.RemoveAll();

	_GetThreadInfo();

	m_lstObj.ResetContent();

	for(int i=0; i < m_arrObjs.GetCount(); i++)
	{
		CString sItem;
		
		sItem.Format(L"%s (%d)", m_arrObjs.GetAt(i)->m_sThrdName, m_arrObjs.GetAt(i)->m_nToSend + m_arrObjs.GetAt(i)->m_nSendError);
		m_lstObj.AddString(sItem);

		/*if(cmbObj.GetCount() == 0)
			cmbObj.SetCurSel(cmbObj.AddString(sList.GetAt(0)));			
		else
			cmbObj.AddString(sList.GetAt(0));		*/
	}

	//m_HttpSenderItems.SetAt(L"LOGIN", L"LOGIN_LOGOUT");
	//cmbObj.AddString(L"LOGIN");
}

void CSincroDlg::OnRecreateXML()
{
	ShowWindow(SW_HIDE);

	CString sObjName, sCmbOpt;
	sObjName = m_arrObjs.GetAt(m_lstObj.GetCurSel())->m_sThrdName;
	//if(sObjName.IsEmpty())
	//{
	//}

	CModule module;
	CString sPath = m_params->GetValue(_T("upd_path"));
	ASSERT(!sPath.IsEmpty());

	m_params->AddPair(_T("codigo"), m_sCodigo);

	for(int i=0; i<m_arrObjs.GetSize(); i++)
	{
		sObjName = m_arrObjs.GetAt(i)->m_sThrdName;
		if(!sObjName.IsEmpty())
		{
			if (sObjName.CompareNoCase(L"XML_AIT") == 0)
			{
				m_params->AddPair(_T("param"), L"XML");
			}
			else if (sObjName.CompareNoCase(L"XML_INCID") == 0)
			{
				m_params->AddPair(_T("param"), L"XML_INCID");
			}	
			else if (sObjName.CompareNoCase(L"XML_EVENTO") == 0)
			{
				m_params->AddPair(_T("param"), L"XML_EVENTO");
			}
			else
			{
				continue;
			}
		}
		else
		{
			continue;
		}

		if(module.Create(sPath))
		{
			LONG size = 0;
			LPBYTE p = m_params->GetBuffer(&size);
			module.Run(NULL, GetParent()->GetSafeHwnd(), &p, &size);
			module.Destroy();

			m_params->ReleaseBuffer();

			ShowWindow(SW_SHOW);
		}
		else
		{
			CString msg;
			msg.LoadString(IDS_ERRO_ATUALIZACAO);

			MessageBox(msg, L"Erro", MB_OK|MB_ICONERROR);
			STLOG_WRITE(L"CLoginDlg::_DoUpdates(): Erro no modulo de atualizacao '%s'.", sPath);
		}
	}

	//if(sObjName.CompareNoCase(L"XML_AIT") == 0)
	{
		sPath.Format(L"%s\\Modules\\Login.dll", CUtil::GetMainAppPath());
		m_params->AddPair(_T("param"), L"XML_LOGIN");

		if(module.Create(sPath))
		{
			LONG size = 0;
			LPBYTE p = m_params->GetBuffer(&size);
			module.Run(NULL, GetParent()->GetSafeHwnd(), &p, &size);
			module.Destroy();

			m_params->ReleaseBuffer();

			ShowWindow(SW_SHOW);
		}
		else
		{
			CString msg;
			msg.LoadString(IDS_ERRO_LOGIN);

			MessageBox(msg, L"Erro", MB_OK|MB_ICONERROR);
			STLOG_WRITE(L"%s(%d): Erro no modulo de login '%s'.", __FUNCTION__ , __LINE__, sPath);
		}
	}


	ShowWindow(SW_SHOW);

	_FillList();
}

void CSincroDlg::OnRetrySend()
{
	CConsultasHttpSender::MarkForResend();
}

void CSincroDlg::OnInfo()
{
	CInfoDlg info;
	info.DoModal();
}

void CSincroDlg::OnLbnSelchangeListObj()
{
	if(m_list.GetItemCount() > 0)
	{
		m_list.DeleteAllItems();
		m_edtStatus.SetWindowText(L"");
	}

	_FillList();

}

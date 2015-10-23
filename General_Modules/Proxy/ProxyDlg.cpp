#include "stdafx.h"
#include "Proxy.h"
#include "ProxyDlg.h"
#include "ModParam.h"
#include "ProxyTable.h"
#include "constants.h"
#include "Utils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern HWND g_hWnd;

CMsgWindow* CProxyDlg::m_wnd;

IMPLEMENT_DYNAMIC(CProxyDlg, CDialogEx)

CProxyDlg::CProxyDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CProxyDlg::IDD, pParent)
	, m_bDiscagem(FALSE)
	, m_bProxy(FALSE)
	, m_sServidor(_T(""))
	, m_sPort(_T(""))
	, m_sUsuario(_T(""))
	, m_sSenha(_T(""))
	, m_pDB(NULL)
{
}

CProxyDlg::~CProxyDlg()
{
}

void CProxyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	//DDX_Check(pDX, IDC_CHECK1, m_bDiscagem);
	DDX_Check(pDX, IDC_CHECK2, m_bProxy);
	DDX_Text(pDX, IDC_EDIT1, m_sServidor);
	DDX_Control(pDX, IDC_EDIT1, m_edtServidor);
	DDX_Text(pDX, IDC_EDIT4, m_sPort);
	DDX_Text(pDX, IDC_EDIT2, m_sUsuario);
	DDX_Control(pDX, IDC_EDIT2, m_edtUsuario);
	DDX_Text(pDX, IDC_EDIT3, m_sSenha);
	DDX_Control(pDX, IDC_EDIT3, m_edtSenha);
	DDX_Control(pDX, IDC_EDIT4, m_edtPorta);
	DDX_Control(pDX, IDC_BUTTON1, m_btnTest);
}

BEGIN_MESSAGE_MAP(CProxyDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CProxyDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON1, &CProxyDlg::OnBnClickedButton1)
	//ON_BN_CLICKED(IDC_CHECK1, &CProxyDlg::OnBnClickedCheck1)
	ON_BN_CLICKED(IDC_CHECK2, &CProxyDlg::OnBnClickedCheck2)
END_MESSAGE_MAP()


// CProxyDlg message handlers
BOOL CProxyDlg::OnInitDialog()
{
	g_hWnd = GetSafeHwnd();
	m_wnd->Show(L"Carregando Dados");

	CDialogEx::OnInitDialog();

#ifdef _WIN32_WCE

	ShowSIP();
	
	if(!m_dlgCommandBar.Create(this))
	{
		STLOG_WRITE("CProxyDlg::OnInitDialog() : Failed to create CommandBar");
		return FALSE;      // fail to create
	}
	
	_CreateBanner(L"Configuração do Proxy");	
#endif

	// Inicializacao vai aqui...
	Init();

	///////////////////////////////////////////////////
	m_wnd->Destroy();

	if(GetParent() != NULL)
		GetParent()->PostMessage(CModParam::WM_MODULE_READY, 0, (LPARAM) GetSafeHwnd());

	_FullScreen();

	return TRUE;
}

void CProxyDlg::OnBnClickedOk()
{
	UpdateData();

	if(m_bProxy)
	{
		if(m_sServidor.IsEmpty())
		{
			MessageBox(L"Digite um servidor", L"Mensagem", MB_ICONERROR|MB_OK);
			m_edtServidor.SetFocus();
			return;
		}
	
		if(m_sServidor.GetLength() > 120)
		{
			MessageBox(L"O servidor pode ter no máximo 120 caractertes", L"Mensagem", MB_ICONERROR|MB_OK);
			m_edtServidor.SetFocus();
			return;
		}

		TCHAR *endp=NULL;
		long value = _tcstol(m_sPort, &endp, 10);
		if(_tcslen(endp) > 0)
		{
			MessageBox(L"Digite um valor numérico", L"Mensagem", MB_ICONERROR|MB_OK);
			m_edtPorta.SetFocus();
			return;
		}

		if(!m_sUsuario.IsEmpty() && m_sUsuario.GetLength()> 20)
		{
			MessageBox(L"O usuario pode ter no máximo 20 caractertes", L"Mensagem", MB_ICONERROR|MB_OK);
			m_edtUsuario.SetFocus();
			return;
		}

		if(!m_sSenha.IsEmpty() && m_sSenha.GetLength()> 20)
		{
			MessageBox(L"A senha pode ter no máximo 20 caractertes", L"Mensagem", MB_ICONERROR|MB_OK);
			m_edtSenha.SetFocus();
			return;
		}
	}

	// Se tiver o banco de dados...
	if(m_pDB != NULL)
	{
		// Gravar os dados...
		CProxyInfo info;
		CProxyTable proxy;
		
		proxy.Init();
		info.bDiscagem = m_bDiscagem;
		info.bProxy	   = m_bProxy;
		info.sServer   = m_sServidor;
		info.nPort	   = _wtol(m_sPort);
		info.sUser	   = m_sUsuario;
		info.sPass	   = m_sSenha;
	
		if(proxy.Count(m_pDB)>0)
		{
			proxy.Update(m_pDB, &info);
			STLOG_WRITE(L"Atualizando informacoes");
		}
		else
		{
			proxy.SetValues((&info));
			proxy.Insert(m_pDB);
			STLOG_WRITE(L"Inserindo informacoes");
		}
		
		STLOG_WRITE(L"m_bDiscagem %ld", m_bDiscagem);
		STLOG_WRITE(L"m_bProxy %ld", m_bProxy);
		STLOG_WRITE(L"m_sServidor %s", m_sServidor);
		STLOG_WRITE(L"m_sPort %s", m_sPort);
		STLOG_WRITE(L"m_sUsuario %s", m_sUsuario);
		STLOG_WRITE(L"m_sSenha %s", m_sSenha);
	}
	
	if(GetParent() != NULL)
		GetParent()->PostMessage(CModParam::WM_CHANGE_PROXY, 0, (LPARAM) GetSafeHwnd());

	OnOK();
}

void CProxyDlg::OnBnClickedButton1() // Testar conf
{
	UpdateData();

	CString sServer   = m_params->GetValue(L"util");
	CString sContrato = m_params->GetValue(L"contrato");

	CProxyInfo info;
	info.bDiscagem = m_bDiscagem;
	info.bProxy	   = m_bProxy;
	info.sServer   = m_sServidor;
	info.nPort	   = _wtol(m_sPort);
	info.sUser	   = m_sUsuario;
	info.sPass	   = m_sSenha;

	if(CUtil::IsOnline(&info, sServer, sContrato))
	{
		STLOG_WRITE("CProxyDlg::OnBnClickedButton1() : Sucesso no teste de conexao");
		MessageBox(L"A conexao foi estabelecida com sucesso", L"Mensagem", MB_OK);
	}
	else
	{
		STLOG_WRITE("CProxyDlg::OnBnClickedButton1() : Erro no teste de conexao");
		MessageBox(L"A conexao não foi estabelecida", L"Mensagem", MB_ICONERROR|MB_OK);
	}
}

void CProxyDlg::Init()
{
	EnableData(FALSE);
	if(m_pDB != NULL)
	{
		// Recuperar os dados...
		CProxyInfo info;
		CProxyTable proxy;
		proxy.Init();
		if(!proxy.Load(m_pDB, &info))
		{
			STLOG_WRITE("CProxyDlg::Init() : Erro carregando dados");
		}
		else
		{
			m_bDiscagem  = info.bDiscagem;
			m_bProxy	 = info.bProxy;
			m_sServidor  = info.sServer;
			m_sPort.Format(L"%ld", info.nPort);
			m_sUsuario	 = info.sUser;
			m_sSenha	 = info.sPass;

			UpdateData(FALSE);

			if(m_bProxy)
				EnableData(TRUE);
		}
	}
}

void CProxyDlg::EnableData(BOOL b)
{
	m_edtServidor.EnableWindow(b);
	m_edtUsuario.EnableWindow(b);
	m_edtSenha.EnableWindow(b);
	m_edtPorta.EnableWindow(b);
	m_btnTest.EnableWindow(b);
}

/*void CProxyDlg::OnBnClickedCheck1()
{
	UpdateData();

	if(m_bDiscagem)
	{
		//connections
		CUtil::ExecuteExternalProgram(L"\\windows\\ctlpnl.exe", L"cplmain.cpl,19", this, FALSE);
	}
}*/

void CProxyDlg::OnBnClickedCheck2()
{
	UpdateData();
	EnableData(m_bProxy);
	if(m_bProxy)
		m_edtServidor.SetFocus();
	else
		_EraseData();
}

void CProxyDlg::_EraseData()
{
	m_sServidor = L"";
	m_sPort = L"";
	m_sUsuario = L"";
	m_sSenha = L"";

	UpdateData(FALSE);
}
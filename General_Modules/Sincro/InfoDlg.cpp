#include "stdafx.h"
#include "Utils.h"
#include "Sincro.h"
#include "InfoDlg.h"

#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNAMIC(CInfoDlg, CDialogEx)
/**
\brief Construtor da classe
\param CppSQLite3DB *pDB Handle de conexão ao banco de dados
\param LPCTSTR szCodigo Código do agente logado
\param CWnd* pParent Parent Window
\return void
*/
CInfoDlg::CInfoDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CInfoDlg::IDD, pParent)
{
	
}

/**
\brief Destrutor da classe
\param void
\return void
*/
CInfoDlg::~CInfoDlg()
{
}


/**
\brief Chamado pelo framework para troca e validação dos dados do dialog
\param CDataExchange* pDX
\return void
*/
void CInfoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TXT_THRDINFO, m_edtInfo);	
	DDX_Control(pDX, IDC_BTN_REFRESH, m_btnUpdate);
}

BEGIN_MESSAGE_MAP(CInfoDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_REFRESH, &CInfoDlg::OnBnClickedAtualizar)
END_MESSAGE_MAP()


/**
\brief Chamado na inicialização do dialog
\details Monta elementos da tela e preenche lista através da chamada ao método _FillList()
\param void
\return void
*/
BOOL CInfoDlg::OnInitDialog()
{

	CDialogEx::OnInitDialog();

#ifdef _WIN32_WCE
	HideSIP();

	if(!m_dlgCommandBar.Create(this))
	{
		AfxMessageBox(IDS_COMMANDBAR_ERROR);
		STLOG_WRITE("CInfoDlg::OnInitDialog() : Failed to create CommandBar");
		EndDialog(IDCANCEL);
		return FALSE;      // fail to create
	}

	_CreateBanner(L"Info");

#endif

	//m_edtInfo.SetWindowText(CUtil::GetThreadInfo());

	SetForegroundWindow();  
	// Resize the window over the taskbar area.
	CRect rect;
	rect = CRect(0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
	MoveWindow(&rect, TRUE);
#ifdef _WIN32_WCE
	SHFullScreen(m_hWnd, SHFS_HIDESTARTICON|SHFS_HIDETASKBAR|SHFS_SHOWSIPBUTTON);
#endif

	_FullScreen();

	return TRUE;
}


/**
\brief Fecha dialog
\param void
\return void
*/
void CInfoDlg::OnBnClickedAtualizar()
{		
	CFile f(L"\\rsend.txt", CFile::modeRead);

	char* szBuffer = new char[(LONG)f.GetLength()+1];
	f.Read(szBuffer, (LONG)f.GetLength());	
	szBuffer[(LONG)f.GetLength()] = 0;

	f.Close();

	m_edtInfo.SetWindowText(CString(szBuffer));
}


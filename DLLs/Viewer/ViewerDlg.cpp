#include "stdafx.h"
#include "Viewer.h"
#include "ViewerDlg.h"
#include "Utils.h"
#include "ModParam.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern HWND g_hWnd;

#ifndef _WIN32_WCE
	CComModule _Module;
	extern __declspec(selectany) CAtlModule* _pAtlModule=&_Module;
#endif

CMsgWindow* CViewerDlg::m_wnd;

IMPLEMENT_DYNAMIC(CViewerDlg, CDialogEx)

CViewerDlg::CViewerDlg(LPCTSTR szURL, LPCTSTR szTitle, CWnd* pParent /*=NULL*/)
	: CDialogEx(CViewerDlg::IDD, pParent)
{
	m_sURL   = szURL;
	m_sTitle = szTitle;
}

CViewerDlg::~CViewerDlg()
{
}

void CViewerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CViewerDlg, CDialogEx)
	ON_WM_SIZE()
	ON_COMMAND(ID_VOLTAR, OnBack)
	ON_COMMAND(ID_STOP, OnStop)
END_MESSAGE_MAP()

BOOL CViewerDlg::OnInitDialog()
{
	g_hWnd = GetSafeHwnd();
	m_wnd = CMsgWindow::getInstance();
	m_wnd->Show(L"Preparando Visualização");

	CDialogEx::OnInitDialog();

	HideSIP();

	// Avisar o pai que a inicializacao encerrou...
	// Encerra o Wait cursor...
	if(GetParent() != NULL)
	{
		GetParent()->PostMessage(CModParam::WM_MODULE_READY, 
								 0, 
								 (LPARAM) GetSafeHwnd());
	}

#ifdef _WIN32_WCE
	if(!m_dlgCommandBar.Create(this))
	{
		AfxMessageBox(IDS_COMMANDBAR_ERROR);
		STLOG_WRITE("CViewerDlg::OnInitDialog() : Failed to create CommandBar");
		EndDialog(IDCANCEL);
		return FALSE;      // fail to create
	}

	if(!m_dlgCommandBar.InsertMenuBar(IDR_MENU_VIEWER))
	{
		AfxMessageBox(IDS_COMMANDBAR_ERROR);
		STLOG_WRITE("CViewerDlg::OnInitDialog() : Failed to create CommandBar 1");
		EndDialog(IDCANCEL);
		return FALSE;      // fail to create
	}
	
	_CreateBanner(m_sTitle);

	if(!m_browser.Create(CRect(0,0,240,290), this, 0x789))
	{
		STLOG_WRITE("CViewerDlg::OnInitDialog() : Failed to create browser control");
		EndDialog(IDCANCEL);
		return FALSE;      // fail to create
	}

	m_browser.Navigate(m_sURL);
#else
	CComPtr<IUnknown> punkCtrl;
    CComVariant v;
	//CAxWindow m_browser;

    // Create the AX host window.
    m_browser.Create ( *this, CRect(0,0,240,320), _T(""), WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN );

    // Create the browser control using its GUID.
    m_browser.CreateControlEx ( L"{8856F961-340A-11D0-A96B-00C04FD705A2}", NULL,
                            NULL, &punkCtrl );

    /*
    NOTE: You could also use the control's ProgID: Shell.Explorer:
    wndIE.CreateControlEx ( L"Shell.Explorer", NULL,
                            NULL, &punkCtrl );
    */

    // Get an IWebBrowser2 interface on the control and navigate to a page.
    pWB2 = punkCtrl;

    if ( pWB2 )
        pWB2->Navigate ( CComBSTR(m_sURL), &v, &v, &v, &v );
#endif

	m_wnd->Destroy();

	_FullScreen();

	return TRUE;
}

void CViewerDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	// Reposicionar o grid para ocupar a tela toda...

	if(IsWindow(GetSafeHwnd()) && IsWindow(m_browser.m_hWnd))
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
						   28,
						   cx,
						   bSIPVisible ? cy : r.Height()); 

		m_browser.MoveWindow(rect.left, rect.top, rect.Width(), rect.Height());
#endif
	}
}


void CViewerDlg::OnBack()
{
#ifdef _WIN32_WCE
	m_browser.Back();
#endif
}

void CViewerDlg::OnStop()
{
#ifdef _WIN32_WCE
	m_browser.Stop();
#endif

}
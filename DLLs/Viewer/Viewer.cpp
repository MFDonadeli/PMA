#include "stdafx.h"
#include "Viewer.h"
#include "ViewerDlg.h"
#include "ModParam.h"
#include "Utils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/***************** Adicionado para funcionar no VS2008 *****************/
CWinApp theApp;

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;
	// initialize MFC and print and error on failure
	if(!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		_tprintf(_T("Fatal Error: MFC initialization failed\n"));
		nRetCode = 1;
	}

	return nRetCode;
}
/***************** Adicionado para funcionar no VS2008 *****************/

HWND g_hWnd = NULL;

// Funcoes padrao para o GetProcAddress funcionar...

extern "C" BOOL PASCAL EXPORT _Load()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return TRUE;
}

extern "C" BOOL PASCAL EXPORT _Unload()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return TRUE;
}

extern "C" BOOL WINAPI EXPORT _Run(HINSTANCE hInst, 
								   HWND hwndParent, 
								   LPBYTE *buffer, 
								   LONG *size)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

#ifdef _WIN32_WCE
	CBrowserCtrl::Init(hInst);
#endif

	CWnd *pWnd = CWnd::FromHandle(hwndParent);

	CModParam param;
	VERIFY(param.SetBuffer(*buffer, *size));

	CWnd *pWnd1 = CWnd::FromHandle(GetTopWindow(NULL));

	CString sURL   = param.GetValue(L"param");
	CString sTitle = param.GetValue(L"text");

	//CViewerDlg::m_wnd->Create(pWnd1);
	//CViewerDlg::m_wnd->Show(L"");
	CViewerDlg dlg(sURL, sTitle, pWnd);
	dlg.m_sServer   = param.GetValue(L"server");
	dlg.m_sContrato = param.GetValue(L"contrato");

	dlg.DoModal();

	// Se nao retorna parametro... OK...
	return FALSE;
}

extern "C" HICON PASCAL EXPORT _GetIcon(UINT nID)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	ASSERT(AfxGetResourceHandle() != NULL);
	return CUtil::LoadResourceIcon(nID, 36);
}

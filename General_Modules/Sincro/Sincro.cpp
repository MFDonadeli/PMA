#include "stdafx.h"
#include "Sincro.h"
#include "Utils.h"
#include "MsgWindow.h"
#include "ModParam.h"
#include "CppSQLite3.h"
#include "CStr.h"
#include "SincroDlg.h"

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

	CWnd *pWnd = CWnd::FromHandle(hwndParent);

	CModParam param;
	VERIFY(param.SetBuffer(*buffer, *size));

	CWnd *pWnd1 = CWnd::FromHandle(GetTopWindow(NULL));

	if(param.GetValue(L"contrato").IsEmpty())
	{
		STLOG_WRITE(L"Tag contrato esta vazia");
		return FALSE;
	}

	CppSQLite3DB *pDB = CppSQLite3DB::getInstance();

	if(!pDB->isOpen())
	{
		pDB = NULL;
		STLOG_WRITE(L"_Run1: Database pointer is null.");
		return FALSE;
	}

	CString sCodigo = param.GetValue(L"codigo");

	CSincroDlg::m_wnd = CMsgWindow::getInstance();
	CSincroDlg::m_wnd->Create(pWnd1);
	CSincroDlg::m_wnd->Show(L"");
	CSincroDlg dlg(pDB, sCodigo, pWnd);
	dlg.m_params = &param;

	//dlg.m_sServer   = param.GetValue(L"server");
	//dlg.m_sContrato = param.GetValue(L"contrato"); 

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

// Data_Hora.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "Exec_Aplic.h"
#include "ModParam.h"
#include "Utils.h"
#include "ExecAplicDlg.h"



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

HWND g_hWnd = NULL;

/***************** Adicionado para funcionar no VS2008 *****************/

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


////extern "C" BOOL WINAPI EXPORT _Run(HINSTANCE hInst, 
////								   HWND hwndParent, 
////								   LPBYTE *buffer, 
////								   LONG *size)
////{
////	AFX_MANAGE_STATE(AfxGetStaticModuleState());
////
////	CWnd *pWnd = CWnd::FromHandle(hwndParent);
////
////	CModParam param;
////	VERIFY(param.SetBuffer(*buffer, *size));
////
////	CWnd *pWnd1 = CWnd::FromHandle(GetTopWindow(NULL));
////
////	////CppSQLite3DB *pDB = CppSQLite3DB::getInstance();
////	////if(!pDB->isOpen())
////	////{
////	////	STLOG_WRITE(L"_Run1: Database pointer is null.");
////	////	//return FALSE;
////	////}
////
////	
////	if(param.GetValue(L"contrato").IsEmpty())
////	{
////		//STLOG_WRITE(L"Tag contrato esta vazia");
////		//return FALSE;
////	}
////
////	CString sParameter	    = param.GetValue(L"param");	
////	int nRet;
////
////	if(sParameter.CompareNoCase(_T("EXEC_APLIC")) == 0)
////	{
////
////		CExecAplicDlg dlg(pWnd);
////		//dlg.m_params = &param;
////		pWnd->PostMessage(CModParam::WM_MODULE_READY, 0, (LPARAM) pWnd->GetSafeHwnd());	
////
////		nRet = dlg.DoModal();
////	}
////	else
////	{
////		pWnd->PostMessage(CModParam::WM_MODULE_READY, 0, (LPARAM) pWnd->GetSafeHwnd());	
////	}
////
////	
////	if(nRet == ID_LOGOUT)
////	{
////		pWnd->PostMessage(CModParam::WM_RETORNO_LOGIN, 0, 0);
////	}
////
////	return FALSE;
////}


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

	////CppSQLite3DB *pDB = CppSQLite3DB::getInstance();
	////// Podemos ter ou nao o database
	////if(!pDB->isOpen())
	////{
	////	pDB = NULL;
	////	STLOG_WRITE(L"_Run1: Database pointer is null.");
	////}

	CExecAplicDlg::m_wnd = CMsgWindow::getInstance();
	CExecAplicDlg::m_wnd->Create(pWnd1);
	CExecAplicDlg::m_wnd->Show(L"");
	CExecAplicDlg dlg(pWnd);
	//dlg.m_pDB = pDB;
	dlg.m_params = &param;
	if(dlg.DoModal() == IDOK)
	{
		// Se for no inicio do programa...
		////if(pDB == NULL)
		////{
			// Retornar dados via parametros...
			////CModParam param1;
			////param1.AddPair(L"DISCAGEM", dlg.m_bDiscagem ? L"TRUE" : L"FALSE");
			////param1.AddPair(L"PROXY",	dlg.m_bProxy ? L"TRUE" : L"FALSE");
			////param1.AddPair(L"SERVER",	dlg.m_sServidor);
			////param1.AddPair(L"PORT",		dlg.m_sPort);
			////param1.AddPair(L"USER",		dlg.m_sUsuario);
			////param1.AddPair(L"PASS",		dlg.m_sSenha);

			delete *buffer;
			////*buffer = param1.GetBuffer(size);
		////}
	}

	return FALSE;
}


extern "C" HICON PASCAL EXPORT _GetIcon(UINT nID)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	ASSERT(AfxGetResourceHandle() != NULL);
	return CUtil::LoadResourceIcon(nID, 36);
}

extern "C" void _ShowWindows(BOOL bShow)
{
}


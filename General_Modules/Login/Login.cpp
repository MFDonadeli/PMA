#include "stdafx.h"
#include "Login.h"
#include "LoginDlg.h"
#include "ContratoDlg.h"
#include "ModParam.h"
#include "CppSQLite3.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

/***************** Adicionado para funcionar no VS2008 *****************/
CWinApp theApp;
/*
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
*/
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

HWND g_hWnd = NULL;

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

	CppSQLite3DB *pDB = CppSQLite3DB::getInstance();
	// Podemos ter ou nao o database
	if(!pDB->isOpen())
	{
		pDB=NULL;
		STLOG_WRITE(L"_Run1: Database pointer is null.");
	}

	CModParam param1;
	BOOL bReturn = FALSE;

	CString sParameter = param.GetValue(L"param");
	if (sParameter.CompareNoCase(_T("XML_LOGIN")) == 0)
	{
		CLoginDlg dlg(pDB, pWnd);
		dlg.m_params = &param;
		dlg.RecreateXML();
		return TRUE;

	}

	if(param.GetValue(L"tela").IsEmpty())
	{
		if(param.GetValue(L"contrato").IsEmpty())
		{
			STLOG_WRITE(L"Tag contrato esta vazia");
			return FALSE;
		}
	
		CLoginDlg dlg(pDB, pWnd);
		dlg.m_params = &param;

		UINT nRet = dlg.DoModal();
		if(nRet == IDOK || nRet == ID_EXIT)
			bReturn = TRUE;

		if(nRet == ID_EXIT)
		{
			param1.AddPair(_T("codigo"), __T("0"));
			param1.AddPair(_T("sair"), __T("TRUE"));
		}
		else if(nRet == IDOK)
		{
			STLOG_WRITE(L"_Run1: Codigo do usuario de login: %s", dlg.GetCodigo());
			param1.AddPair(_T("codigo"), dlg.GetCodigo());
			param1.AddPair(_T("sair"), __T("FALSE"));

			if(dlg.m_bUpdated)
				param1.AddPair(_T("atualizado"), __T("TRUE"));
			else
				param1.AddPair(_T("atualizado"), __T("FALSE"));
		}
	}
	else
	{
		CContratoDlg dlg(pWnd);
		dlg.m_params = &param;
		dlg.m_pDB = pDB;

		UINT nRet = dlg.DoModal();
		if(nRet == IDOK || nRet == ID_EXIT)
			bReturn = TRUE;

		param1.AddPair(L"COD_ORGAO_AUTUADOR", dlg.GetOrgaoAutuador());
		param1.AddPair(L"CONTRATO", dlg.GetContrato());
	}

	//delete *buffer;
	*buffer = param1.GetBuffer(size);

	return bReturn;
}

extern "C" void _ShowWindows(BOOL bShow)
{
	// Esconder ou mostrar todas as janelas atualmente em exibicao...
	if(g_hWnd != NULL)
		ShowWindow(g_hWnd, bShow ? SW_SHOW : SW_HIDE);
}


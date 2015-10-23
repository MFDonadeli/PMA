// Data_Hora.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include "Data_Hora.h"
#include "stdafx.h"
#include "CppSQLite3.h"
#include "ModParam.h"
#include "Utils.h"
#include "Data_HoraDlg.h"
#include "SenhaDlg.h"
#include "PrintersDlg.h"
#include "DlgDraw.h"
#include "DlgWireless.h"
#include "ImprimirDlg.h"
#include "BlueTooth.h"
#include "Mensagens.h"



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

	CppSQLite3DB *pDB = CppSQLite3DB::getInstance();
	if(!pDB->isOpen())
	{
		STLOG_WRITE(L"_Run1: Database pointer is null.");
		//return FALSE;
	}

	
	if(param.GetValue(L"contrato").IsEmpty())
	{
		STLOG_WRITE(L"Tag contrato esta vazia");
		//return FALSE;
	}

	CString sParameter	    = param.GetValue(L"param");	
	int nRet;

	if(sParameter.CompareNoCase(_T("DATAHORA")) == 0)
	{

	//	CData_HoraDlg::m_wnd.Create(pWnd1);
	//	CData_HoraDlg::m_wnd.Show(L"");
		CData_HoraDlg dlg(pDB, pWnd);
		dlg.m_params = &param;

	//	dlg.m_sServer   = param.GetValue(L"server");
	//	dlg.m_sContrato = param.GetValue(L"contrato");

		nRet = dlg.DoModal();
	}
	else if(sParameter.CompareNoCase(_T("SENHA")) == 0 || sParameter.CompareNoCase(_T("SENHA_LOGIN")) == 0)
	{
		CSenhaDlg dlg(pDB, pWnd);
		dlg.m_params = &param;
		nRet = dlg.DoModal();
		CModParam param1;
		if (nRet == IDYES)
		{
			param1.AddPair(_T("Ret_Dlg_Senha"), _T("IDYES"));
		}
		else
		{
			param1.AddPair(_T("Ret_Dlg_Senha"), _T("IDCANCEL"));
		}
		delete *buffer;
		*buffer = param1.GetBuffer(size);

		if(nRet == ID_LOGOUT && sParameter.CompareNoCase(_T("SENHA_LOGIN")) == 0)
		{
			nRet = IDOK;
		}
	}
	else if(sParameter.CompareNoCase(_T("LOC_PRINTERS")) == 0)
	{
		CPrintersDlg dlg(pWnd);
		dlg.m_params = &param;
		nRet = dlg.DoModal();
	}
	else if(sParameter.CompareNoCase(_T("MENSAGEM")) == 0)
	{
		CMensagens dlg(pWnd);
		dlg.m_params = &param;
		nRet = dlg.DoModal();
	}
	else if(sParameter.CompareNoCase(_T("DRAW")) == 0)
	{
		CDlgDraw dlg(pWnd);
		nRet = dlg.DoModal();
	}
	else if(sParameter.CompareNoCase(_T("WIRELESS")) == 0)
	{
		CDlgWireless dlg(pWnd);
		nRet = dlg.DoModal();
	}
	else if(sParameter.CompareNoCase(_T("PRINTERS")) == 0)
	{
		if (CBlueTooth::IsMSBluetoothStack())
		{
			CString sParTest  = param.GetValue(L"printer_teste");	
			CImprimirDlg dlg(pWnd);

			pWnd->PostMessage(CModParam::WM_MODULE_READY, 0, (LPARAM) pWnd->GetSafeHwnd());

			nRet = dlg.DoModal();
		}
		else
		{
			pWnd->PostMessage(CModParam::WM_MODULE_READY, 0, (LPARAM) pWnd->GetSafeHwnd());	
			//IDS_CONFIG_WIDCOMM
			CString Msg;
			Msg.LoadString(IDS_CONFIG_WIDCOMM);
			MessageBox(GetActiveWindow(), Msg, L"Mensagem", MB_ICONINFORMATION|MB_OK);
		}
	}
	
	if(nRet == ID_LOGOUT)
	{
		pWnd->PostMessage(CModParam::WM_RETORNO_LOGIN, 0, 0);
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


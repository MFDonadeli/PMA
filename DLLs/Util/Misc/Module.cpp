// Copyright (c) 2007 Marcos Mori de Siqueira. All Rights Reserved. 
// mori@softfactory.com.br
// Classe que representa o modulo, ela encapsula o carregamento da 
// DLL e os callbacks dos metodos
#include "stdafx.h"
#include "Module.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

CModule::CModule()
{
	m_hModule     = NULL;
	m_lpfnLoad    = NULL;
	m_lpfnUnload  = NULL;
	m_lpfnRun     = NULL;
	m_lpfnGetIcon = NULL;
	m_lpfnSetDbPtr= NULL;
	m_lpfnShowWindows = NULL;
	m_lpfnRun1 = NULL;
	m_lpfnResetPrn = NULL;
}

CModule::~CModule()
{
	Destroy();
}

BOOL CModule::Create(LPCTSTR szModuleName)
{
	if(m_hModule != NULL &&
	   m_lpfnLoad != NULL && m_lpfnUnload != NULL && m_lpfnRun != NULL)
	{
		return TRUE;
	}

	if(m_hModule != NULL)
		Destroy();

	STLOG_WRITE(L"CModule::Create(LPCTSTR): Criando módulo [%s] para execução", szModuleName);

	m_hModule = LoadLibrary(szModuleName);

	if(m_hModule == NULL)
	{
		STLOG_WRITE(L"CModule::Create(LPCTSTR): Erro ao carregar DLL [%s]. Erro: [%d]", szModuleName, GetLastError());
		return FALSE;
	}

#ifdef _WIN32_WCE
	m_lpfnLoad    = (LPFNDLLLOAD)GetProcAddress(m_hModule, _T("_Load"));
	m_lpfnUnload  = (LPFNDLLUNLOAD)GetProcAddress(m_hModule, _T("_Unload"));
	m_lpfnRun     = (LPFNDLLRUN)GetProcAddress(m_hModule, _T("_Run"));
	m_lpfnRun1    = (LPFNDLLRUN1)GetProcAddress(m_hModule, _T("_Run1"));


	// Opcional...
	m_lpfnGetIcon	  = (LPFNDLLGETICON)GetProcAddress(m_hModule, _T("_GetIcon"));
	m_lpfnShowWindows = (LPFNDLLSHOWWINDOWS)GetProcAddress(m_hModule, _T("_ShowWindows"));
	m_lpfnSetDbPtr	  = (LPFNDLLSETDBPTR)GetProcAddress(m_hModule, _T("_SetDbPtr"));
	m_lpfnResetPrn    = (LPFNDLLRESETPRN)GetProcAddress(m_hModule, _T("_ResetPrinter"));
#else
	m_lpfnLoad    = (LPFNDLLLOAD)GetProcAddress(m_hModule, "_Load");
	m_lpfnUnload  = (LPFNDLLUNLOAD)GetProcAddress(m_hModule, "_Unload");
	m_lpfnRun     = (LPFNDLLRUN)GetProcAddress(m_hModule, "_Run");
	m_lpfnRun1    = (LPFNDLLRUN1)GetProcAddress(m_hModule, "_Run1");


	// Opcional...
	m_lpfnGetIcon	  = (LPFNDLLGETICON)GetProcAddress(m_hModule, "_GetIcon");
	m_lpfnShowWindows = (LPFNDLLSHOWWINDOWS)GetProcAddress(m_hModule, "_ShowWindows");
	m_lpfnSetDbPtr	  = (LPFNDLLSETDBPTR)GetProcAddress(m_hModule, "_SetDbPtr");
	m_lpfnResetPrn    = (LPFNDLLRESETPRN)GetProcAddress(m_hModule, "_ResetPrinter");
#endif

	if(m_lpfnLoad == NULL || m_lpfnUnload == NULL || (m_lpfnRun == NULL && m_lpfnRun1 == NULL))
	{
		STLOG_WRITE(_T("Erro carregando os metodos da dll '%s'"), szModuleName);
		return FALSE;
	}

	return _Load();
}

BOOL CModule::_Load()
{
	if(m_lpfnLoad != NULL)
		return m_lpfnLoad();
	return FALSE;
}

BOOL CModule::_Unload()
{
	if(m_lpfnUnload != NULL)
		return m_lpfnUnload();
	return FALSE;
}

BOOL CModule::Run(HINSTANCE hInst, HWND hwndParent, LPBYTE *pParam, LONG *size)
{
	if(m_lpfnRun != NULL)
		return m_lpfnRun(hInst, hwndParent, pParam, size);

	return FALSE;
}

BOOL CModule::Run1(HINSTANCE hInst, HWND hwndParent, LPVOID pDb, LPBYTE *pParam, LONG *size)
{
	if(m_lpfnRun1 != NULL)
		return m_lpfnRun1(hInst, hwndParent, pDb, pParam, size);

	return FALSE;
}

HICON CModule::GetIcon(UINT nResID)
{
	if(m_lpfnGetIcon != NULL)
		return m_lpfnGetIcon(nResID);
	return NULL;
}

void CModule::ShowWindows(BOOL bShow)
{
	if(m_lpfnShowWindows != NULL)
		m_lpfnShowWindows(bShow);
}

void CModule::SetDbPtr(LPVOID pDB)
{
	if(m_lpfnSetDbPtr != NULL)
		m_lpfnSetDbPtr(pDB);
}

void CModule::Destroy()
{
	TCHAR szAux[MAX_PATH];
	memset(&szAux, 0, MAX_PATH);

	GetModuleFileName(m_hModule, szAux, MAX_PATH);

	STLOG_WRITE(L"CModule::Destroy(): Destruindo módulo [%s]", szAux);

	if(m_hModule != NULL)
	{
		VERIFY(_Unload());
		FreeLibrary(m_hModule);
	}

	m_hModule	  = NULL;
	m_lpfnLoad    = NULL;
	m_lpfnUnload  = NULL;
	m_lpfnRun     = NULL;
	m_lpfnGetIcon = NULL;
	m_lpfnShowWindows = NULL;
	m_lpfnSetDbPtr = NULL;
	m_lpfnRun1 = NULL;
	m_lpfnResetPrn = NULL;
}

void CModule::RestartPrinter(LPCTSTR szHandle)
{
	if(m_lpfnResetPrn != NULL)
		m_lpfnResetPrn(szHandle);
}

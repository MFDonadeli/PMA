// Copyright (c) 2007 Marcos Mori de Siqueira. All Rights Reserved. 
// mori@softfactory.com.br
// Classe que representa o gerenciador de hotkeys, agrupa e trata
// as chamadas 
#include "StdAfx.h"

#ifdef _WIN32_WCE

#include "HotKeyMgr.h"
#include <winioctl.h>
#include "Utils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

UINT CHotKeyMgr::WM_HKSHOWWINDOW =
	RegisterWindowMessage(L"CHotKeyMgr::WM_HKSHOWWINDOW");

#define IOCTL_HAL_REBOOT CTL_CODE(FILE_DEVICE_HAL, 15, METHOD_BUFFERED, FILE_ANY_ACCESS)

extern "C" __declspec(dllimport) BOOL KernelIoControl(DWORD dwIoControlCode, LPVOID lpInBuf,
													  DWORD nInBufSize, LPVOID lpOutBuf,
													  DWORD nOutBufSize, LPDWORD lpBytesReturned);

/**
\brief
	Construtor da classe
\details
	Funções executadas neste método:
	- Inicialização de variáveis globais

\param
	void
*/
CHotKeyMgr::CHotKeyMgr(void)
{
	m_hWnd = NULL;
}

/**
\brief
	Destrutor da classe
\details
	Funções executadas neste método:
	- Destruir todos os ponteiros e a lista de HOTKEYS criados

\param
	void
*/
CHotKeyMgr::~CHotKeyMgr(void)
{
	if(m_hWnd != NULL)
	{
		POSITION p = m_hotkeys.GetStartPosition();
		while(p)
		{
			__hkAppInfo *pInfo;
			UINT id;
			m_hotkeys.GetNextAssoc(p, id, pInfo);
			UnregisterHotKey(m_hWnd, id);
			delete pInfo;
		}
		
		m_hotkeys.RemoveAll();
	}
}

/**
\brief
	Inicia o gerenciamento de botões de atalho para uma janela
\param
	HWND hWnd: Janela que irá gerenciar os botões de atalho
\return
	void
*/
void CHotKeyMgr::Init(HWND hWnd)
{
	m_hWnd = hWnd;
}

/**
\brief
	Adiciona o aplicativo a sua tecla de atalho correspondente
\details
	Estas informações estão contidas no arquivo de configuração
\param
	LPCTSTR szAppPath: Caminho do aplicativo a ser vinculado ao botão
\return
	void
*/
void CHotKeyMgr::AddHK(LPCTSTR szAppPath)
{
	if(m_hWnd != NULL)
	{
		int count = m_hotkeys.GetCount();

		__hkAppInfo *pInfo = new __hkAppInfo;
		pInfo->id   = count;
		pInfo->path = szAppPath;
		pInfo->pi.dwProcessId = 0;

		//if(RegisterHotKey(m_hWnd, pInfo->id, MOD_KEYUP|MOD_WIN|MOD_CONTROL|MOD_ALT|MOD_SHIFT, 192 + count))
		if(RegisterHotKey(m_hWnd, pInfo->id, MOD_KEYUP|MOD_WIN, 192 + count))
		{
			STLOG_WRITE(L"Botão registrado com sucesso! %d / %d", pInfo->id, 192 + count);
			m_hotkeys.SetAt(pInfo->id, pInfo);
		}
		else

		{
			STLOG_WRITE(L"Botão %d registrado com erro! %d", pInfo->id, GetLastError());
		}

	}
}

/**
\brief
	Processa a mensagem que é enviada ao apertar o botão de atalho
\details
	Funções executadas neste método:
	- Verificação de qual botão foi pressionada;
	- Execução do aplicativo vinculado a este botão;
		- Se o botão for pressionado duas vezes em seguida, o aplicativo irá fechar.
	- Execução de um som se não há aplicativos vinculados a este botão.

\param WPARAM wParam: Número sequencial que indentificará facilmente qual botão foi pressionado (definido em ordem sequencial)
\param LPARAM lParam: Contém informações sobre o código do botão pressionado
\return
	0L, sempre
*/
LRESULT CHotKeyMgr::OnHotKey(WPARAM wParam, LPARAM lParam)
{
	int id = (int) wParam;
	UINT uVirtKey = (UINT) HIWORD(lParam);
	if((UINT)LOWORD(lParam) & MOD_KEYUP)
	{
//		if(!IsWindowVisible(m_hWnd))
//			return 0L;

		__hkAppInfo *pInfo = NULL;
		if(m_hotkeys.Lookup(id, pInfo))
		{
			if(pInfo->pi.dwProcessId == 0)
			{
				if(!pInfo->path.IsEmpty())
				{
					WIN32_FIND_DATA wfd;

					if(FindFirstFile(pInfo->path, &wfd) == INVALID_HANDLE_VALUE)
					{
						PlaySound(_T("\\windows\\alarm4.wav"), NULL, SND_SYNC | SND_FILENAME);
						return 0L;
					}

					//HWND hwnd = FindWindow(_T("HHTaskBar"),_T(""));
					//ShowWindow(hwnd, SW_SHOW);

					// Esconder todas as janelas...
					SendMessage(m_hWnd, CHotKeyMgr::WM_HKSHOWWINDOW, FALSE, 0);

					CreateProcess(pInfo->path, 
								  _T(""), 
								  NULL, 
								  NULL, 
								  NULL, 
								  CREATE_NEW_CONSOLE, 
								  NULL, 
								  NULL, 
								  NULL, 
								  &pInfo->pi);

					// Esperar encerrar...
					/*while(1)
					{
						if(WaitForSingleObject(pInfo->pi.hProcess, 1000) == WAIT_OBJECT_0)
							break;

						Sleep(100);
					}*/

					//ShowWindow(hwnd, SW_HIDE);

					// Mostrar as janelas...
					//SendMessage(m_hWnd, CHotKeyMgr::WM_HKSHOWWINDOW, TRUE, 0);
					//pInfo->pi.dwProcessId = 0;
				}
				else
				{
					CUtil::ShowPMA();

					PlaySound(_T("\\windows\\alarm4.wav"), NULL, SND_SYNC | SND_FILENAME);
				}
			}
			else
			{
				HWND hwnd = FindWindow(_T("HHTaskBar"),_T(""));
				ShowWindow(hwnd, SW_HIDE);
				SendMessage(m_hWnd, CHotKeyMgr::WM_HKSHOWWINDOW, TRUE, 0);
				TerminateProcess(pInfo->pi.hProcess, 1);
				pInfo->pi.dwProcessId = 0;
			}
		}
		else
		{
			CUtil::ShowPMA();

			PlaySound(_T("\\windows\\alarm4.wav"), NULL, SND_SYNC | SND_FILENAME);
		}
	}

	return 0L;
}

/**
\brief
	Reinicia o equipamento
\param
	void
\return
	void
*/
void CHotKeyMgr::ResetPocket()
{
	KernelIoControl(IOCTL_HAL_REBOOT, NULL, 0, NULL, 0, NULL);
}
#endif
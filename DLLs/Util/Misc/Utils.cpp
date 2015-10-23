// Copyright (c) 2007 Marcos Mori de Siqueira. All Rights Reserved. 
// mori@softfactory.com.br
// Classe que encapsula metodos estaticos de uso geral
#pragma once
#include "stdafx.h"
#include "utils.h"
#include <WINIOCTL.H>
#include <wininet.h>
#include "constants.h"
#include "Http.h"
#include "SimpleRequest.h"
#include "GPass.h"
#ifdef _WIN32_WCE
	#include <connmgr.h>
	#include <connmgr_proxy.h>
	#include "storemgr.h"
	#if _WIN32_WCE != 0x420
	#include <GetDeviceUniqueId.h>
	#include <imaging.h>
	#include <connmgr_status.h>
	#include <snapi.h>
	#endif
	#include <initguid.h>
	#include <imgguids.h>
	#include "MsgWindow.h"
	#include <Tapi.h>
	#include <extapi.h>
	#include <tsp.h>
#endif
#include <wincrypt.h>
#include "CRC_32.h"
#include "strsafe.h"
#include "Registry.h"
#include "XmlCreate.h"
#include <Tlhelp32.h>

CString CUtil::m_sLoggedUser = L"--NENHUM--";
CString CUtil::m_sLastLoggedUser = L"--NENHUM--";
CString CUtil::m_sLoggedUserPerms = L"";
CString CUtil::m_sContrato = L"";
CString CUtil::m_sOnlineURL = L"";
CString CUtil::m_sIdEqpto = L"";
BOOL CUtil::m_bBackupOK = TRUE;
CString CUtil::m_sUrlTxLoginLogout;
CString CUtil::m_sUrlTxAlteraSenha;
CString CUtil::m_sUrlTxInfoTalao;
BOOL CUtil::m_bAutenticando;
BOOL CUtil::m_bOnLine = TRUE;
DWORD CUtil::m_dwMemMessage = 10;
CMap<CString, LPCTSTR, ThrdInfo*, ThrdInfo*> CUtil::m_mapThrdInfo;
STARTUPINFO CUtil::si;
PROCESS_INFORMATION CUtil::pi;
HINSTANCE CUtil::m_hInst = NULL;

CRITICAL_SECTION CUtil::m_uSendLock;

//CCriticalSection CUtil::m_SendLock;
//LPCRITICAL_SECTION CUtil::m_SendLock;
/**
\brief Adquire path da aplicação que esta sendo executada\param void
\return CString
*/
CString CUtil::GetAppPath()
{
	CString s;
	TCHAR szAux[MAX_PATH];
	memset(&szAux, 0, MAX_PATH);
	GetModuleFileName(AfxGetInstanceHandle(), szAux, MAX_PATH);
	s = szAux;

	if(s.GetLength() > 1)
	{
		int pos = s.ReverseFind('\\');
		s = s.Mid(0, pos);
	}

	//TRACE(L"%s\n", s);

	return s;
}

/**
\brief Adquire path principal da aplicação
\param void
\return CString
*/
CString CUtil::GetMainAppPath()
{
	CString sAppPath = GetAppPath();

	int iPos = sAppPath.Find(L"PMA");
	
	return sAppPath.Left(iPos + 3);
}

/**
\brief Adquire path principal dos arquivos .job utilizados pela thread httpsender
\param void
\return CString
*/
CString CUtil::GetJobPath()
{
	// Cria diretório base do httpsender se não existir
#ifdef _WIN32_WCE
	CString sBaseDir = L"\\temp\\httpsender";
#else
	CString sBaseDir = L"c:\\temp\\httpsender";
#endif

	CUtil::CreateDirIfNotExist(sBaseDir);
	return sBaseDir;
}


/**
\brief Adquire path principal do httpsender
\param void
\return CString
*/
void CUtil::CreateDirIfNotExist(LPCTSTR szDirPath)
{		
	if(!CUtil::FileExists(szDirPath))
	{
		if(CreateDirectory(szDirPath, NULL))
			STLOG_WRITE(L"CUtil::CreateDirIfNotExist() : Criado diretório: %s", szDirPath);					
	}	
}


/**
\brief Cria novo estilo de fonte
\param CFont *ft Objeto fonte
\param LPCTSTR szFaceName Nome da nova fonte
\param int size Tamanho da Fonte
\param BOOL bBold Flag Negrito
\param BOOL bItalic Flag Itálico
\param BOOL bUnderline Flag Sublinhado
\return BOOL
*/

BOOL CUtil::CreateFont(CFont *ft, LPCTSTR szFaceName, int size, BOOL bBold, BOOL bItalic, BOOL bUnderline)
{
	LOGFONT	lf;
	long	nHeight;
	ZeroMemory(&lf, sizeof(lf));

	//
	// Create the option font
	//         5 : -8
	//         6 : -9
	//		   7 : -10	
	// Points  8 : -11
	//         9 : -12
	//		  10 : -13
	//		  11 : -14
	//		  12 : -15

	DWORD dwRequired;
	LONG  dwFontSize = DRA::SCALEX(size*(-1));

#ifdef _WIN32_WCE
	//SHGetUIMetrics(SHUIM_FONTSIZE_PIXEL, &dwFontSize, sizeof(dwFontSize), &dwRequired);
#endif

	nHeight = -12; //-11;//(LONG)(-8 * pDC.GetDeviceCaps(LOGPIXELSY) / 72);


	lf.lfHeight			= -dwFontSize;
	lf.lfWidth			= 0;
	lf.lfEscapement		= 0;
	lf.lfOrientation	= 0;
	lf.lfWeight			= bBold ? FW_BOLD : FW_NORMAL;
	lf.lfItalic			= bItalic;
	lf.lfUnderline		= bUnderline;
	lf.lfStrikeOut		= 0;
	lf.lfCharSet		= ANSI_CHARSET;
	lf.lfOutPrecision	= OUT_DEFAULT_PRECIS;
	lf.lfClipPrecision	= CLIP_DEFAULT_PRECIS;
	lf.lfQuality		= DEFAULT_QUALITY;
	lf.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
	lstrcpy(lf.lfFaceName, szFaceName);

	return ft->CreateFontIndirect(&lf);
}

/**
\brief Desabilita auto-complete em edit´s de password
\details Usar no Set e no Kill focus
\param CWnd *pWnd Handle da Janela
\return void
*/
#ifdef _WIN32_WCE
void CUtil::DisableCompletation(CWnd *pWnd) 
{
	SIPINFO info;
	SHSipInfo(SPI_GETSIPINFO, 0, &info, 0);
	info.fdwFlags |= SIPF_DISABLECOMPLETION;
	SHSipInfo(SPI_SETSIPINFO, 0, &info, 0);
	SHSipPreference(pWnd->GetSafeHwnd(), SIP_FORCEDOWN);
	SHSipPreference(pWnd->GetSafeHwnd(), SIP_UP);
}
#endif

/**
\brief Habilita auto-complete  em edit´s de password
\details Usar no Set e no Kill focus
\param CWnd *pWnd Handle da Janela
\return void
*/
#ifdef _WIN32_WCE
void CUtil::EnableCompletation(CWnd *pWnd) 
{
	SIPINFO info;
	SHSipInfo(SPI_GETSIPINFO, 0, &info, 0);
	info.fdwFlags &= ~SIPF_DISABLECOMPLETION;
	SHSipInfo(SPI_SETSIPINFO, 0, &info, 0);
	SHSipPreference(pWnd->GetSafeHwnd(), SIP_FORCEDOWN);
	SHSipPreference(pWnd->GetSafeHwnd(), SIP_UP);
}
#endif

/**
\brief Carrega ícone da aplicação
\param UINT nID Id do ícone
\param int size Tamanho que o ícone será desenhado
\return HICON
*/
HICON CUtil::LoadResourceIcon(UINT nID, int size)
{
	if(AfxGetResourceHandle() != NULL)
	{
		return (HICON) LoadImage(AfxGetResourceHandle(), 
								 MAKEINTRESOURCE(nID), 
								 IMAGE_ICON, 
								 size, 
								 size, 
								 LR_DEFAULTCOLOR);
	}

	return NULL;
}

#if _WIN32_WCE != 0x420 && defined _WIN32_WCE
/**
\brief Carrega uma imagem
\param LPCTSTR szFileName: Nome do arquivo
\param ImgInfo* iInfo: Ponteiro para as informações da foto
\param int sizex: Largura da imagem a ser carregada. O padrão é -1, que indica a imagem em tamanho real
\param int sizey: Largura da imagem a ser carregada. O padrão é -1, que indica a imagem em tamanho real
\return HBITMAP
*/
HBITMAP CUtil::LoadPicture(LPCTSTR szFileName, ImgInfo* iInfo, const HWND hWnd, int iSizex, int iSizey)
{
	IImagingFactory* pImageFactory = 0;
	IImage* pImage = 0;
	HDC hDC;
	//CDC *cDC;
	iInfo->m_photoPath = szFileName;

	//PAINTSTRUCT ps;

	//hDC = CreateCompatibleDC(NULL);
	hDC = GetDC(hWnd);
	//CPaintDC(this);

	//cDC = BeginPaint(&ps);

	//hDC = *cDC;

	CoInitializeEx(0, COINIT_MULTITHREADED);

	HBITMAP hBitmap = 0;
	if (SUCCEEDED(CoCreateInstance(CLSID_ImagingFactory, 0, CLSCTX_INPROC_SERVER, IID_IImagingFactory, (void**)&pImageFactory)))
	{

		ImageInfo imageInfo;

		if (SUCCEEDED(pImageFactory->CreateImageFromFile(szFileName, &pImage)) && SUCCEEDED(pImage->GetImageInfo(&imageInfo)))
		{

			HDC bmpDC = CreateCompatibleDC(hDC);
			if (hBitmap = CreateCompatibleBitmap(hDC, imageInfo.Width, imageInfo.Height))
			{

				HGDIOBJ hOldBitmap = SelectObject(bmpDC, hBitmap);

				iInfo->m_width = imageInfo.Width;
				iInfo->m_height = imageInfo.Height;
				
				if(iSizex!=-1)
				{
					RECT rect = {0, 0, iSizex, iSizey};
					IImage* pImage2 = NULL;
					pImage->GetThumbnail(iSizex, iSizey, &pImage2);
					pImage2->Draw(bmpDC, &rect, 0);
				}
				else
				{
					RECT rect = {0, 0, imageInfo.Width, imageInfo.Height};
					pImage->Draw(bmpDC, &rect, 0);
				}
				SelectObject(bmpDC, hOldBitmap);
			}
			pImage->GetThumbnail(60, 60, &pImage);
			pImage->Release();
			DeleteDC(bmpDC);
		}
		pImageFactory->Release();
	}
	CoUninitialize();
	//EndPaint(&ps);

	return hBitmap;
}
#endif

/**
\brief Registra a classe
\param LPCTSTR szClassOri Classe de origem
\param LPCTSTR szClassNew Nova classe
\param bool &bRegistered Flag de registro
\return bool
*/
bool CUtil::RegisterWndClass(LPCTSTR szClassOri, LPCTSTR szClassNew, bool &bRegistered)
{
	if(bRegistered)
		return true;

	WNDCLASS _wndClassInfo;
	HINSTANCE hInst = AfxGetInstanceHandle();

	if(!::GetClassInfo(hInst, szClassNew, &_wndClassInfo))
	{
		if(szClassOri != NULL)
		{
			if(!::GetClassInfo(hInst, szClassOri, &_wndClassInfo))
			{
				ASSERT(FALSE);
				return FALSE;
			}
		}
		else
		{
			_wndClassInfo.style         = CS_GLOBALCLASS|CS_DBLCLKS;
			_wndClassInfo.lpfnWndProc   = ::DefWindowProc;
			_wndClassInfo.cbClsExtra    = 0;
			_wndClassInfo.cbWndExtra    = 0;
			_wndClassInfo.hInstance     = hInst;
			_wndClassInfo.hIcon         = 0;
			_wndClassInfo.hCursor       = 0;
			_wndClassInfo.hbrBackground = NULL;
			_wndClassInfo.lpszMenuName  = NULL;
		}

		_wndClassInfo.lpszClassName = szClassNew;

		if(!::AfxRegisterClass(&_wndClassInfo))
		{
			ASSERT(FALSE);
			return false;
		}
	}

	bRegistered = true;

	return true;
}

#ifdef _WIN32_WCE
extern "C" 
__declspec(dllimport) 
BOOL KernelIoControl(DWORD dwIoControlCode, LPVOID lpInBuf, DWORD nInBufSize, LPVOID lpOutBuf, DWORD nOutBufSize, LPDWORD lpBytesReturned);

#define IOCTL_HAL_GET_DEVICEID CTL_CODE(FILE_DEVICE_HAL, 21, METHOD_BUFFERED, FILE_ANY_ACCESS)


/**
\brief Adquire número serial do dispositivo, usado como id do talão
\param void
\return CString
*/
CString CUtil::GetSerialNumberFromKernelIoControl()
{
	DWORD dwOutBytes;
	CString strDeviceId;
	int nBuffSize = 4096;
	byte* arrOutBuff = new byte[nBuffSize];
	DWORD dwErr;
	CString strDeviceInfo;

	BOOL bRes = ::KernelIoControl(IOCTL_HAL_GET_DEVICEID, 
									  0, 
									  0, 
									  arrOutBuff, 
									  nBuffSize, 
									  &dwOutBytes);

	dwErr = GetLastError();

	if (bRes)
	{
		for (unsigned int i = 0; i<dwOutBytes; i++)
		{
			CString strNextChar;
			strNextChar.Format(TEXT("%02X"), arrOutBuff[i]);
			strDeviceInfo += strNextChar;
		}
		strDeviceId = 
		strDeviceInfo.Mid(40,2) + 
		strDeviceInfo.Mid(45,9) + 
		strDeviceInfo.Mid(70,6);

		delete [] arrOutBuff;

		return strDeviceId;

	}
	else if(dwErr == ERROR_INSUFFICIENT_BUFFER)
	{	
		nBuffSize = (int)arrOutBuff[0];

		CString msg;
		msg.Format(L"%d", nBuffSize);

		byte* arrOutBuff1 = new byte[nBuffSize];

		bRes = ::KernelIoControl(IOCTL_HAL_GET_DEVICEID, 
									  0, 
									  0, 
									  arrOutBuff1, 
									  nBuffSize, 
									  &dwOutBytes);
		if(bRes)
		{
			for (unsigned int i = 0; i<dwOutBytes; i++) 
			{
				CString strNextChar;
				strNextChar.Format(TEXT("%02X"), arrOutBuff1[i]);
				strDeviceInfo += strNextChar;
			}

			strDeviceId = strDeviceInfo.Mid(40,2) + 
								  strDeviceInfo.Mid(45,9) + 
								  strDeviceInfo.Mid(70,6);

			delete [] arrOutBuff1;
			return strDeviceId;
		}
		
		delete [] arrOutBuff1;
	}

	 delete [] arrOutBuff;

	 
 

	return _T("N/A");
}
#endif

#if _WIN32_WCE >= 0x500
CString CUtil::GetUniqueID()
{
    HRESULT            hr;

	BYTE                g_bDeviceID1[GETDEVICEUNIQUEID_V1_OUTPUT];
	BYTE                g_bDeviceID2[GETDEVICEUNIQUEID_V1_OUTPUT];


	CRC_32 crc;
	ulong val32;
	TCHAR buffer[100];
	TCHAR buf[100];


	CString a, b;

    // Get the first device id

    // Application specific data
    // {8D552BD1-E232-4107-B72D-38B6A4726439}
    const GUID     bApplicationData1  = { 0x8d552bd1, 0xe232, 0x4107, { 0xb7, 0x2d, 0x38, 0xb6, 0xa4, 0x72, 0x64, 0x39 } };
    const DWORD    cbApplicationData1 = sizeof (bApplicationData1);

    DWORD g_cbDeviceID1 = GETDEVICEUNIQUEID_V1_OUTPUT;
	hr = GetDeviceUniqueID (reinterpret_cast<LPBYTE>("PMA_ENGEBRAS"), 
    //hr = GetDeviceUniqueID (reinterpret_cast<LPBYTE>(const_cast<LPGUID>(&bApplicationData1)), 
                             cbApplicationData1, 
                             GETDEVICEUNIQUEID_V1, 
                             g_bDeviceID1, 
                             &g_cbDeviceID1);

	for(int i=0; i<GETDEVICEUNIQUEID_V1_OUTPUT; i++)
	{
		a.AppendFormat(L"%.02x", g_bDeviceID1[i]);	
	}

	val32 = crc.CalcCRC((LPVOID) a.GetBuffer(), (uint) a.GetLength(), 0);
	_ultow(val32, buffer, 16);
	a.ReleaseBuffer();
    
    if (SUCCEEDED (hr))
    {
        // Get a second ID to verify that they are different for
        // different Application data

        // Application specific data
        // {C5BEC46D-2A43-4200-BBB7-5FF14097954D}
        const GUID     bApplicationData2  = { 0xc5bec46d, 0x2a43, 0x4200, { 0xbb, 0xb7, 0x5f, 0xf1, 0x40, 0x97, 0x95, 0x4d } };
        const DWORD    cbApplicationData2 = sizeof (bApplicationData2);

        DWORD g_cbDeviceID2 = GETDEVICEUNIQUEID_V1_OUTPUT;
		hr = GetDeviceUniqueID (reinterpret_cast<LPBYTE>("ID_TALAO"), 
        //hr = GetDeviceUniqueID (reinterpret_cast<LPBYTE>(const_cast<LPGUID>(&bApplicationData2)), 
                                 cbApplicationData2, 
                                 GETDEVICEUNIQUEID_V1, 
                                 g_bDeviceID2, 
                                 &g_cbDeviceID2);
		

		for(int i=0; i<GETDEVICEUNIQUEID_V1_OUTPUT; i++)
		{
			b.AppendFormat(L"%.02x", g_bDeviceID2[i]);	
		}
		val32 = crc.CalcCRC((LPVOID) b.GetBuffer(), (uint) b.GetLength(), 0);
		_ultow(val32, buf, 16);
		b.ReleaseBuffer();
    
    }

	CString c;
	c.Format(L"%s%s", buffer, buf);

    return c.MakeUpper();

}
#endif

/**
\brief Adquire número serial do dispositivo
\deprecated Era usado como id do talão nas versões mais antigas do sistema TE Multas
\param void
\return CString
*/
#ifdef _WIN32_WCE
CString CUtil::GetSerialNumber()
{
	// TODO: Add your implementation code here
	CString m_strCompaqIpaqId, m_strErrorMessage;
	// Start CreateAssetFile.exe
	 PROCESS_INFORMATION pi;
	 if (!::CreateProcess(TEXT("\\windows\\CreateAssetFile.exe"), 
			 NULL, NULL, NULL, FALSE, 0, NULL, NULL, NULL, &pi)) 
	 { 
		  m_strCompaqIpaqId = _T("");
		  m_strErrorMessage += _T("Não foi possível executar o arquivo \\windows\\CreateAssetFile.exe.");
		  //MessageBox(NULL, m_strErrorMessage, _T("Atenção"), MB_OK | MB_ICONEXCLAMATION);
		  return _T("N/A");
	 }
 
	 // Aguarda até CreateAssetFile.exe ser finalizado
	 ::WaitForSingleObject(pi.hProcess, INFINITE);
 
	 // Obtem os dados de cpqAssetData.dat
	 HANDLE hInFile;
	 TCHAR strSN[65]; 
	 DWORD dwBytesRead;   
	 hInFile = CreateFile(TEXT("\\windows\\cpqAssetData.dat"), GENERIC_READ, 
			   FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0); 
 
	 if (hInFile == INVALID_HANDLE_VALUE) { 
		  m_strCompaqIpaqId = _T("");
		  m_strErrorMessage += _T("Não foi possível abrir o arquivo \\windows\\cpqAssetData.dat.");
		  //MessageBox(NULL, m_strErrorMessage, _T("Atenção"), MB_OK | MB_ICONEXCLAMATION);
		  return _T("N/A");
	 } 
 
	 //SetFilePointer(hInFile, 976, NULL, FILE_BEGIN);
	 SetFilePointer(hInFile, 10, NULL, FILE_BEGIN); 
	 memset(strSN, 0, 64 * sizeof(TCHAR)); 
	 ReadFile(hInFile, &strSN, 64, &dwBytesRead, NULL);
	 CloseHandle(hInFile);
	
	 if(wcslen(strSN)==0)
		 return _T("N/A");
 
	return strSN;
}
#endif

/**
\brief Adquire descrição do erro conforme seu código
\param DWORD err Código do erro
\return CString
*/
CString CUtil::ErrorString(DWORD err)
{
	if(err == 0)
		return _T("");

	CString Error;
	LPTSTR s;
	if(::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
					   FORMAT_MESSAGE_FROM_SYSTEM,
					   NULL,
					   err,
					   0,
					   (LPTSTR)&s,
					   0,
					   NULL) == 0)
	{ 
		// Unknown error code %08x (%d)
		CString fmt;
		CString t;
		fmt = "UNKNOWN_ERROR: %d, %d";
		t.Format(fmt, err, LOWORD(err));
		Error = t;
    }
	else
    { 
		LPTSTR p = _tcschr(s, _T('\r'));
		if(p != NULL)
		{
			*p = _T('\0');
        }
     
		Error = s;
		::LocalFree(s);
    }

	return Error;
}

CString CUtil::GetOperadora()
{
	const long TAPI_API_LOW_VERSION = 0x00020000;
	const long TAPI_API_HIGH_VERSION = 0x00020000;
	const long EXT_API_LOW_VERSION = 0x00010000;
	const long EXT_API_HIGH_VERSION = 0x00010000;

	HLINEAPP hLineApp = 0;
	HLINE hLine = 0;
	DWORD dwNumDevs;
	DWORD dwAPIVersion = TAPI_API_HIGH_VERSION;
	LINEINITIALIZEEXPARAMS liep;
	DWORD dwTAPILineDeviceID;
	LINEOPERATOR lOperator;
	const DWORD dwMediaMode = LINEMEDIAMODE_DATAMODEM | LINEMEDIAMODE_INTERACTIVEVOICE;
	DWORD dwExtVersion;

	if (lineInitializeEx(&hLineApp, 0, 0, L"PMA", 
                         &dwNumDevs, &dwAPIVersion, &liep)) 
    {
        goto cleanup;
    }

	DWORD dwReturn = 0xffffffff;
    for(DWORD dwCurrentDevID = 0 ; dwCurrentDevID < dwNumDevs ; dwCurrentDevID++)
    {
        DWORD dwAPIVersion;
        LINEEXTENSIONID LineExtensionID;
        if(0 == lineNegotiateAPIVersion(hLineApp, dwCurrentDevID, 
                                        TAPI_API_LOW_VERSION, TAPI_API_HIGH_VERSION, 
                                        &dwAPIVersion, &LineExtensionID)) 
        {
            LINEDEVCAPS LineDevCaps;
            LineDevCaps.dwTotalSize = sizeof(LineDevCaps);
            if(0 == lineGetDevCaps(hLineApp, dwCurrentDevID, 
                                   dwAPIVersion, 0, &LineDevCaps)) 
            {
                BYTE* pLineDevCapsBytes = new BYTE[LineDevCaps.dwNeededSize];
                if(0 != pLineDevCapsBytes) 
                {
                    LINEDEVCAPS* pLineDevCaps = (LINEDEVCAPS*)pLineDevCapsBytes;
                    pLineDevCaps->dwTotalSize = LineDevCaps.dwNeededSize;
                    if(0 == lineGetDevCaps(hLineApp, dwCurrentDevID, 
                                           dwAPIVersion, 0, pLineDevCaps)) 
                    {
                        if(0 == _tcscmp((TCHAR*)((BYTE*)pLineDevCaps+pLineDevCaps->dwLineNameOffset), 
                                        CELLTSP_LINENAME_STRING)) 
                        {
                            dwReturn = dwCurrentDevID;
                        }
                    }
                    delete[]  pLineDevCapsBytes;
                }
            }
        }
    }
    dwTAPILineDeviceID = dwReturn;

	if (0xffffffff == dwTAPILineDeviceID) 
    {
        goto cleanup;
    }

	if(lineOpen(hLineApp, dwTAPILineDeviceID, 
                &hLine, dwAPIVersion, 0, 0, 
                LINECALLPRIVILEGE_OWNER, dwMediaMode, 0)) 
    {
        goto cleanup;
    }
	
	if (lineNegotiateExtVersion(hLineApp, dwTAPILineDeviceID, 
                                dwAPIVersion, EXT_API_LOW_VERSION, 
                                EXT_API_HIGH_VERSION, &dwExtVersion)) 
    {
        goto cleanup;
    }

	lineGetCurrentOperator(hLine, &lOperator);

cleanup:
	if (hLine) lineClose(hLine);
    if (hLineApp) lineShutdown(hLineApp);

	return lOperator.lpszNumName;

}

BOOL CUtil::SetupConexao(const CString sOperadora)
{
	CString sConnGuid;
	Registry* reg;
	BYTE* bCred;
	int nCredSize;

	if(sOperadora.CompareNoCase(L"Claro") == 0)
	{
		reg = new Registry(HKEY_LOCAL_MACHINE, L"\\Comm\\ConnMgr\\Providers\\{7C4B7A38-5FF7-4bc1-80F6-5DA7870BB1AA}\\Connections\\Claro");
		sConnGuid = L"{1DBDA3B8-1EEF-EF09-6147-DD3E10876CC8}";
		BYTE b[] = {0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,\
					  0x00,0x00,0x00,0x0e,0x66,0x00,0x00,0x10,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x8a,0xc8,0x2d,0xf1,0xb1,0x54,0x48,0xe3,0xa1,0x63,\
					  0xf3,0xa4,0xb9,0xc7,0x3a,0xa7,0x00,0x00,0x00,0x00,0x04,0x80,0x00,0x00,0x10,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x9e,0x7e,0x73,\
					  0x43,0xa0,0xb8,0x35,0x9c,0xfa,0xb1,0x80,0x7c,0xd7,0x45,0x6f,0xef,0x60,0x00,0x00,0x00,0x89,0xc3,0x2c,0xbb,0xb1,0xf2,0x57,0x62,\
					  0xea,0x10,0xb2,0x63,0xf3,0x54,0x38,0x06,0x40,0xf4,0x8b,0x32,0x6d,0xa3,0x9e,0x27,0xbf,0x8e,0xd5,0x96,0x41,0x98,0xc8,0xd8,0xe3,\
					  0x70,0x8e,0xc3,0xec,0xff,0xaa,0xc1,0x6b,0xc5,0x5f,0x76,0xbd,0xcd,0xc7,0x01,0x6a,0xd3,0x7d,0x68,0x25,0xed,0x32,0x24,0x99,0xcb,\
					  0xf7,0xa9,0x35,0xcb,0xa4,0xd7,0x38,0xd6,0x7f,0x94,0x84,0xce,0x7a,0x13,0x74,0x61,0xa4,0xa4,0x47,0x45,0x08,0x85,0x79,0xf5,0xda,\
					  0x3c,0xcc,0x79,0xc0,0x05,0xe6,0x12,0xce,0x77,0xca,0x72,0xb7,0x2e,0x14,0x00,0x00,0x00,0x40,0xb1,0x40,0x3a,0x53,0x0e,0x75,0xfa,\
					  0x23,0xbf,0xfe,0x93,0xab,0xd3,0x77,0xc0,0xa9,0xc0,0x9f,0xa5};
		
		nCredSize = sizeof(b)/sizeof(b[0]);

		bCred = b;
	}
	else if(sOperadora.CompareNoCase(L"Vivo") == 0)
	{
		reg = new Registry(HKEY_LOCAL_MACHINE, L"\\Comm\\ConnMgr\\Providers\\{7C4B7A38-5FF7-4bc1-80F6-5DA7870BB1AA}\\Connections\\Vivo");
		sConnGuid = L"{D72E62DC-2C9F-9D63-AA32-C5BA243852A2}";
		BYTE b[] = {0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,\
					  0x00,0x00,0x00,0x0e,0x66,0x00,0x00,0x10,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x8f,0xb6,0xb6,0xcb,0x57,0x2c,0x66,0xec,0x85,0x4f,\
					  0xfe,0xfc,0x88,0x35,0xbb,0xd3,0x00,0x00,0x00,0x00,0x04,0x80,0x00,0x00,0x10,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x5e,0x3d,0xaa,\
					  0x46,0x1e,0xcb,0xf5,0x7a,0x83,0x05,0xb8,0xcf,0x17,0x8a,0x97,0xbe,0x50,0x00,0x00,0x00,0x2e,0xe2,0x2c,0x7a,0x32,0x60,0xe2,0xfa,\
					  0x37,0xa6,0x41,0xda,0x90,0x55,0x7b,0x8e,0xec,0xb5,0xf5,0x5d,0xfb,0x04,0x8e,0x58,0xe6,0x87,0xaa,0x72,0xb4,0x9f,0xed,0xad,0x96,\
					  0xba,0xae,0x41,0x87,0x43,0x8d,0x19,0x37,0x5a,0xb7,0xc9,0xc2,0xc5,0x13,0xff,0x75,0x12,0x86,0x01,0xb7,0x2a,0x52,0xf1,0xc7,0xe3,\
					  0xca,0x5e,0x8a,0xfc,0xd4,0x62,0xeb,0xc1,0xb8,0x34,0xe5,0x95,0x3f,0x4b,0x82,0x6c,0x8a,0x85,0xaa,0x9c,0xcc,0x12,0x14,0x00,0x00,\
					  0x00,0xcb,0x52,0x2d,0x58,0x9d,0xf8,0xf1,0x22,0xeb,0x45,0xb5,0x07,0xea,0xea,0x7b,0x57,0xa2,0x2d,0xbb,0x05};

		nCredSize = sizeof(b)/sizeof(b[0]);
		bCred = b;
	}

	reg->Open();
	reg->SetValue(L"ConnectionGUID", sConnGuid);
	reg->SetValue(L"NDISGPRSAuthType", (DWORD)1);
	reg->SetValue(L"ReadOnly", (DWORD)0);
	reg->SetValue(L"SecureLevel", (DWORD)0);
	reg->SetValue(L"Secure", (DWORD)0);
	reg->SetValue(L"AlwaysOn", (DWORD)0);
	reg->SetValue(L"RequirePw", (DWORD)1);
	reg->SetValue(L"Enabled", (DWORD)1);
	reg->SetValue(L"EntryType", (DWORD)2);
	reg->SetValue(L"DestId", L"{ADB0B001-10B5-3F39-27C6-9742E785FCD4}");

	reg->Close();

	DWORD dwNumCreds;
	DWORD dwFreeSlots;

	Registry reg2(HKEY_LOCAL_MACHINE, L"\\Comm\\Security\\CredMan\\Creds");
	reg2.Open();
	reg2.GetValue(L"NumCreds", &dwNumCreds);
	reg2.GetValue(L"FreeSlot", &dwFreeSlots);
	dwNumCreds++;
	dwFreeSlots++;
	reg2.SetValue(L"NumCreds", dwNumCreds);
	reg2.SetValue(L"FreeSlot", dwFreeSlots);
	reg2.Close();

	CString sKey;
	sKey.Format(L"\\Comm\\Security\\CredMan\\Creds\\%d", dwNumCreds);
	Registry reg3(HKEY_LOCAL_MACHINE, sKey);
	reg3.SetValue(L"Cred", bCred, nCredSize);
	reg3.Close();

	return TRUE;
}

/**
\brief Qual conexão o equipamento está usando
\param void
\return CString Descrição da conexão
*/
CString CUtil::GetConnectionType()
{
#if _WIN32_WCE != 0x420
	CString sResult;
	CONNMGR_CONNECTION_DETAILED_STATUS* pstatusBuffer = NULL; 
	DWORD dwSize = sizeof(CONNMGR_CONNECTION_DETAILED_STATUS);  
	HRESULT hRet = E_FAIL; 

	hRet = ConnMgrQueryDetailedStatus(pstatusBuffer, &dwSize); 

	if(hRet==(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)))  
	{
		pstatusBuffer=(CONNMGR_CONNECTION_DETAILED_STATUS*)malloc(dwSize);
	        
		hRet=ConnMgrQueryDetailedStatus(pstatusBuffer,&dwSize);    
	}

	if((pstatusBuffer != NULL))   
    {   
		if(pstatusBuffer->dwConnectionStatus == CONNMGR_STATUS_DISCONNECTED)
			sResult = L"Desconectado";
		else
		{
			switch(pstatusBuffer->dwType)
			{
			case CM_CONNTYPE_CELLULAR:
				sResult.Format(L"Celular / %s", CUtil::GetTipoConexao());
				break;
			case CM_CONNTYPE_NIC:
				sResult = L"NIC";
				break;
			case CM_CONNTYPE_BLUETOOTH:
				sResult = L"BlueTooth";
				break;
			case CM_CONNTYPE_UNIMODEM:
				sResult = L"Unimodem";
				break;
			case CM_CONNTYPE_VPN:
				sResult = L"VPN";
				break;
			case CM_CONNTYPE_PROXY:
				sResult = L"Proxy";
				break;
			case CM_CONNTYPE_PC:
				sResult = L"PC";
				break;
			case CM_CONNTYPE_UNKNOWN:
			default:
				sResult = L"Desconhecido";
				break;
			}
		}
    }  

	free(pstatusBuffer);

	return sResult;
#endif
}

/**
\brief Estabelece conexão de internet no dispositivo
\param void
\return void
*/
#ifdef _WIN32_WCE
DWORD CUtil::SetOnLine()
{	
#if _WIN32_WCE != 0x420
	CONNMGR_CONNECTION_DETAILED_STATUS* pstatusBuffer = NULL; 
	DWORD dwSize = sizeof(CONNMGR_CONNECTION_DETAILED_STATUS);  
	HRESULT hRet = E_FAIL; 

	hRet = ConnMgrQueryDetailedStatus(pstatusBuffer, &dwSize); 

	STLOG_WRITE("%s(%d): Online: %d", __FUNCTION__, __LINE__, OnLine());

	if(hRet==(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)))  
	{
		pstatusBuffer=(CONNMGR_CONNECTION_DETAILED_STATUS*)malloc(dwSize);
	        
		hRet=ConnMgrQueryDetailedStatus(pstatusBuffer,&dwSize);    
	}

	if((pstatusBuffer != NULL))   
    {   
		if (CONNMGR_STATUS_DISCONNECTED != pstatusBuffer->dwConnectionStatus && OnLine())
		{
			STLOG_WRITE("%s(%d): Online: %d", __FUNCTION__, __LINE__, OnLine());
			free(pstatusBuffer);
			return CONNMGR_STATUS_CONNECTED;   
		}
    }  

	free(pstatusBuffer);
#endif


	CONNMGR_CONNECTIONINFO ci = {0};
	PROXY_CONFIG pcProxy = {0};
	DWORD dwStatus		 = 0;
	DWORD dwIndex		 = 0;
	HRESULT hr			 = S_OK; 
	HANDLE hConnection	 = NULL;
	HANDLE hOpen		 = NULL;
	LPTSTR pszProxy		 = NULL;
	DWORD dwAccessType   = 0;

	STLOG_WRITE("%s(%d)", __FUNCTION__, __LINE__);

	// Register with the connection manager
	ci.cbSize     = sizeof(CONNMGR_CONNECTIONINFO); 
	ci.dwParams   = CONNMGR_PARAM_GUIDDESTNET; 
	ci.dwFlags    = CONNMGR_FLAG_PROXY_HTTP | 0x40; 
	ci.dwPriority = CONNMGR_PRIORITY_USERINTERACTIVE;

	hr = ConnMgrMapURL(L"http://www.engebras.com.br", &(ci.guidDestNet), &dwIndex);

   //Check hr value.

	hr = ConnMgrEstablishConnectionSync(&ci, &hConnection, 25000, &dwStatus);

	ConnMgrReleaseConnection(hConnection, FALSE);

	STLOG_WRITE("%s(%d): Online: %d", __FUNCTION__, __LINE__, OnLine());

	STLOG_WRITE(L"Status de conexão: %d", dwStatus);

	return dwStatus;

}
#endif

#ifdef _WIN32_WCE
BOOL CUtil::IsOnline()
{
	CSimpleRequest req;

	if(!OnLine())
	{
		if(SetOnLine()!=CONNMGR_STATUS_CONNECTED)
		{
			m_bOnLine = FALSE;
			return FALSE;
		}
	}
		
	m_bOnLine = TRUE;

	return TRUE;
}	
#endif

#ifdef _WIN32_WCE
BOOL CUtil::OnLine()
{
	return m_bOnLine;
}	
#endif

BOOL CUtil::IsOnline(LPCTSTR szURL, LPCTSTR szContrato)
{
#ifdef _WIN32_WCE
	if(!OnLine())
		SetOnLine();
#endif

	if(m_sOnlineURL.IsEmpty())
		m_sOnlineURL = szURL;
	else
	{
		szURL = m_sOnlineURL;
	}

	CSimpleRequest conn;

	/*conn.ResetArguments();
	conn.AddArguments(L"operacao", L"online");
	conn.AddArguments(L"contrato", szContrato);
	conn.AddArguments(L"cd_equipamento", CUtil::GetIDTalao()); */

	if(!conn.Request( szURL, TRUE ))
	{
		m_bOnLine = FALSE;
		return FALSE;
	}
	else
	{
		m_bOnLine = TRUE;
		return TRUE;
	}
}

/**
\brief Converte caracteres ANSI para Unicode
\param const char *szBuffer
\return CString
*/
CString CUtil::ConvertAnsiToUnicode(const char *szBuffer)
{
#ifdef UNDER_CE // WCE

	int len = (int) strlen(szBuffer) + 1;
	WCHAR *ws = new WCHAR[len];
	memset(ws, 0, len);

	// ANSI to UNICODE Conversion ...
	MultiByteToWideChar(CP_ACP, 
						0, 
						szBuffer,
						len, 
						ws,
						len);

	CString s(ws);

	delete [] ws;

	return s;

#else // WIN32 

	CString s(szBuffer);
	return s;

#endif
}

/**
\brief Converte caracteres Unicode para ANSI
\details Nao esqueca de deletar o buffer de retorno
\param const TCHAR *szBuffer
\return char*
*/
char *CUtil::ConvertUnicodeToAnsi(const TCHAR *szBuffer)
{
	int len = (int)_tcslen(szBuffer); 

#ifdef UNDER_CE // WCE

	//char *q = (char *)malloc( len + 1 );]
	char *q = new char[len + 1];
	memset(q, 0, len + 1);

	int sz = WideCharToMultiByte(CP_ACP, 
								 0, 
								 szBuffer, 
								 -1,
								 q, 
								 len, 
								 NULL, 
								 NULL );

	return q;

#else // WIN32

	char *q = (char *)malloc( len + 1 );	
	memset(q, 0, len + 1);
	memcpy(q, szBuffer, len);
	return q;

#endif
}


#ifdef TESTE_IMPRESSORA
/**
\brief Converte string Bluetooth Address para o tipo BT_ADDR
\details Nao esqueca de zerar o conteudo do parametro de retorno pba
\param(in) const WCHAR **pp 
\param(out) BT_ADDR *pba
\return BOOL TRUE se OK FALSE se erro
*/

BOOL CUtil::ConvertStringToBT_ADDR(const WCHAR **pp, BT_ADDR *pba)
{
    int i;   
   
    while (**pp == ' ') 
	{
        ++*pp;
	}
   
    for (i = 0 ; i < 4 ; ++i, ++*pp) {   
        if (! iswxdigit (**pp))   
            return FALSE;   
   
        int c = **pp;   
        if (c >= 'a')   
            c = c - 'a' + 0xa;   
        else if (c >= 'A')   
            c = c - 'A' + 0xa;   
        else c = c - '0';   
   
        if ((c < 0) || (c > 16))   
            return FALSE;   
   
        *pba = *pba * 16 + c;   
    }   
   
    for (i = 0 ; i < 8 ; ++i, ++*pp) {   
        if (! iswxdigit (**pp))   
            return FALSE;   
   
        int c = **pp;   
        if (c >= 'a')   
            c = c - 'a' + 0xa;   
        else if (c >= 'A')   
            c = c - 'A' + 0xa;   
        else c = c - '0';   
   
        if ((c < 0) || (c > 16))   
            return FALSE;   
   
        *pba = *pba * 16 + c;   
    }   
   
    if ((**pp != ' ') && (**pp != '\0'))   
        return FALSE;   
   
    return TRUE;   

}

#endif


/**
\brief Divide uma string em strings
\details Alimenta um array de strings (&list), cada uma como substring de string formada pela divisão dela a partir 
\details do caractere delimitador (seps).
\param const CString &sText String a ser dividida
\param CStringArray &list Array de substrings resultantes da separação
\param int &expectedSize Tamanho de sText
\param const TCHAR* seps Caractere delimitador
\return void
*/
void CUtil::Tokenize(const CString &sText, CStringArray &list, int &expectedSize, const TCHAR* seps/*='|'*/)
{
	list.RemoveAll();
	CString resToken;
	int curPos=0, cnt=0;
	CString s(sText);
	
	if(wcslen(seps)==1)
	{
		//CString sSeps;
		CString sSeps1;
		//sSeps.Format(L"%s", seps);
		sSeps1.Format(L" %s", seps);
		s.Replace(seps, sSeps1);
	}	

	resToken= s.Tokenize(seps,curPos);
	while (resToken != "")
	{
		list.Add(resToken); //Tinha um TRIM()
		resToken= s.Tokenize(seps,curPos);
	}

	expectedSize = list.GetSize();
	//ASSERT(list.GetCount() == expectedSize);
}

/**
\brief Divide uma string em strings
\details Alimenta um array de strings (&list), cada uma como substring de string formada pela divisão dela a partir 
\details do caractere delimitador (seps).
\param const CString &sText String a ser dividida
\param CStringArray &list Array de substrings resultantes da separação
\param int &expectedSize Tamanho de sText
\param const TCHAR* seps Caractere delimitador
\return void
*/
void CUtil::Tokenize(const CString &sText, CStringList &list, int &expectedSize, const TCHAR* seps/*='|'*/)
{
	list.RemoveAll();
	CString resToken;
	int curPos=0, cnt=0;
	CString s(sText);

	if(wcslen(seps)==1)
	{
		//CString sSeps;
		CString sSeps1;
		//sSeps.Format(L"%s", seps);
		sSeps1.Format(L" %s", seps);
		s.Replace(seps, sSeps1);
	}	

	resToken= s.Tokenize(seps,curPos);
	while (resToken != "")
	{
		list.AddTail(resToken);
		resToken= s.Tokenize(seps,curPos);
	}

	expectedSize = list.GetSize();
	//ASSERT(list.GetCount() == expectedSize);
}


/**
\brief Divide uma string em strings
\details Alimenta um array associativo de strings (&list), cada uma como substring de string formada pela divisão dela a partir 
\details do caractere delimitador (seps).
\param const CString &sText String a ser dividida
\param CMapStringToString &list Array de substrings resultantes da separação
\param int &expectedSize Tamanho de sText
\param const TCHAR* seps Caractere delimitador
\return void
*/
void CUtil::Tokenize(const CString &sText, CMapStringToString &list, int &expectedSize, const TCHAR* seps/*='|'*/)
{
	list.RemoveAll();	
	CString resToken;
	int curPos=0, cnt=0;
	CString s(sText);
	
	if(wcslen(seps)==1)
	{
		//CString sSeps;
		CString sSeps1;
		//sSeps.Format(L"%s", seps);
		sSeps1.Format(L" %s", seps);
		s.Replace(seps, sSeps1);
	}	

	resToken= s.Tokenize(seps,curPos);
	while (resToken != "")
	{		
		//contrato=%s|MD5Xml=%s
		CString campo = resToken.Mid(0, resToken.Find(L"="));
		CString valor = resToken.Mid(resToken.Find(L"=")+1, resToken.GetLength());
		campo.Trim();
		valor.Trim();

		list.SetAt(campo, valor);
		resToken= s.Tokenize(seps,curPos);
	}

	expectedSize = list.GetSize();
	//ASSERT(list.GetCount() == expectedSize);
}



/**
\brief Verifica se formato de placa é válido
\details Valida formatos: LLLNNNN, LLNNNN e LLNNN onde L são letras e N são números. 
\param CString& strPlaca Placa a ser validada
\param const BOOL bPadrao Quando TRUE valida formatos citados acima, caso FALSE função retorna TRUE.
\return BOOL
*/
BOOL CUtil::ValidPlaca(CString& strPlaca, const BOOL bPadrao)
{
	if(bPadrao)
	{
		CString strPlacaLetras;
		CString strPlacaNumeros;

		strPlaca.Remove(L'*');
		
		if(strPlaca.Trim().GetLength() == 7)
		{
			strPlacaLetras  = strPlaca.Left(3);
			strPlacaNumeros = strPlaca.Right(4);
		}
		else if(strPlaca.Trim().GetLength() == 6)
		{
			strPlacaLetras  = strPlaca.Left(2);
			strPlacaNumeros = strPlaca.Right(4);
		}
		else if(strPlaca.Trim().GetLength() == 5)
		{
			strPlacaLetras  = strPlaca.Left(2);
			strPlacaNumeros = strPlaca.Right(3);
		}
		else
		{
			return FALSE;
		}

		for(int i = 0; i < strPlacaLetras.GetLength(); i++)
		{
			if(!iswalpha(strPlacaLetras.GetAt(i)))
				return FALSE;
		}

		for(int i = 0; i < strPlacaNumeros.GetLength(); i++)
		{
			if(!iswdigit(strPlacaNumeros.GetAt(i)))
				return FALSE;
		}
	}

	return TRUE;
}

/**
\brief Verifica se dispositivo está on-line
\param CProxyInfo *pProxy Informações do Proxy 
\param LPCTSTR szURL URL de teste de conexão
\param LPCTSTR szContrato Contrato do produto
\return BOOL gerado por ValidatePost() que interpreta o retorno da URL
*/
BOOL CUtil::IsOnline(CProxyInfo *pProxy, LPCTSTR szURL, LPCTSTR szContrato)
{
	CString strResp;
	//CString sURL(szURL);

	CSimpleRequest conn;

	if(m_sOnlineURL.IsEmpty())
		m_sOnlineURL = szURL;
	else
	{
		szURL = m_sOnlineURL;
	}

	if(m_sContrato.IsEmpty())
		m_sContrato = szContrato;
	else
	{
		szContrato = m_sContrato;
	}

	if(pProxy->bProxy)	
		conn.SetProxy(pProxy->sServer, pProxy->nPort, pProxy->sUser, pProxy->sPass);


	//return IsOnline(szURL, szContrato);

	SetOnLine();

	conn.ResetArguments();
	conn.AddArguments(L"operacao", L"online");
	conn.AddArguments(L"contrato", szContrato);
	conn.AddArguments(L"cd_equipamento", CUtil::GetIDTalao()); 

	if(!conn.Request( szURL ))
	{
		///STLOG_WRITE(L"CUtil::IsOnline() : Erro executando http request");
		m_bOnLine = FALSE;
		return FALSE;
	}

	strResp = conn.Response();

	if(ValidatePost(&strResp))
		m_bOnLine = TRUE;
	else
		m_bOnLine = FALSE;

	return m_bOnLine;
}


/**
\brief Verifica se talão é válido para o contrato informado
\param CProxyInfo *pProxy Informações do Proxy 
\param LPCTSTR szURL URL de teste de validaão do talão
\param LPCTSTR szContrato Contrato do produto
\param CString* strResp String de resposta da URL
\return BOOL gerado por ValidatePost() que interpreta o retorno da URL
*/
BOOL CUtil::IsValidTalao(CProxyInfo *pProxy, LPCTSTR szURL, LPCTSTR szContrato, CString* strResp)
{
	CSimpleRequest conn;
	CString sURL(szURL);

	if(pProxy->bProxy)
		conn.SetProxy(pProxy->sServer, pProxy->nPort, pProxy->sUser, pProxy->sPass);
	
	conn.AddArguments(L"operacao", L"valida_talao");
	conn.AddArguments(L"cd_equipamento", CUtil::GetIDTalao());
	conn.AddArguments(L"contrato", szContrato);
	
	if(conn.Request( sURL ))
	{
		*strResp = conn.Response();
		*strResp += L"\n" + CUtil::GetIDTalao();
	}

	return ValidatePost(strResp);
}

/**
\brief Interpreta o retorno de uma URL do sistema
\param CString* strReturn String de resposta da URL
\return BOOL
*/
BOOL CUtil::ValidatePost(CString* strReturn, BOOL bWithErrorCode, int* code)
{

	if(strReturn->GetLength() == 0)
	{
		*strReturn = L"Resposta do servidor vazia";
		return FALSE;
	}

	if(bWithErrorCode)
	{
		int n = 3;
		CStringArray sRespVarList;
		CUtil::Tokenize(*strReturn, sRespVarList, n);

		if(sRespVarList.GetSize() < 3)
		{
			sRespVarList.RemoveAll();
			return FALSE;
		}
		
		//Cod da resposta do server
		CStringA  sCodResp = CStringA(sRespVarList.GetAt(1));
		int nRet = atoi(sCodResp);
		if(code) *code=nRet;

		//Descrição da resposta do server
		*strReturn = sRespVarList.GetAt(2);

		sRespVarList.RemoveAll();
		
		switch(nRet)
		{				
			case 100:
				return TRUE;
			break;

			case 200:
			case 300:
				return FALSE;
			break;

			default: //Se não vier o código de resposta do envio, força reenvio (200)
				nRet = 200;				
				if(code) *code=nRet;
				return FALSE;
			break;
		}
	}
	else
	{
		CString sResult = CString(strReturn->GetAt(0));
		*strReturn = strReturn->Right(strReturn->GetLength()-2);

		if(sResult.Compare(L"1")==0)
		{
			return TRUE;
		}
		
		return FALSE;
	}
}

/**
\brief Retorna mensagem de resposta do servidor
\param CString* strReturn String de resposta da URL
\return BOOL
*/
CString CUtil::GetServerRespostaMsg(CString* strReturn)
{
	if(strReturn->GetLength() == 0)
	{	
		return L"";
	}

	CString sResult = strReturn->Right(strReturn->GetLength()-2);	

	return sResult;
}



/**
\brief Sincroniza data/hora do dispositivo com o servidor
\param CProxyInfo *pProxy Informações do Proxy 
\param LPCTSTR szURL URL que fornece data/hora do servidor
\return BOOL
*/
BOOL CUtil::SetDataHoraServidor(CProxyInfo *pProxy, LPCTSTR szURL, LPCTSTR szContrato)
{
	DeleteCacheIE();
	CSimpleRequest conn;
	CString sHorario;
	//CInetHttp pInetHttp;
	
	CString sURL(szURL);

	if(pProxy)
	{
		if(pProxy->bProxy)
		{
			conn.SetProxy(pProxy->sServer, pProxy->nPort, pProxy->sUser, pProxy->sPass);
		}
			//CHttp::GetInstance().SetProxy(pProxy->sServer, pProxy->nPort, pProxy->sUser, pProxy->sPass);		
	}

	conn.ResetArguments();
	conn.AddArguments(L"contrato", szContrato);
	conn.AddArguments(L"operacao", L"datahora");
	conn.AddArguments(L"cd_equipamento", CUtil::GetIDTalao());
	
	if(conn.Request( sURL ))
	{
		sHorario = conn.Response();
	}

	if(!ValidatePost(&sHorario))
		return FALSE;
	
	CString sHorarioNew;
	SYSTEMTIME dhAtual;

	ZeroMemory(&dhAtual, sizeof(dhAtual));

	for(int i=0;i<=sHorario.GetLength();i++)
	{
		if((int)sHorario.GetAt(i)>=48 && (int)sHorario.GetAt(i)<=57)
			sHorarioNew += sHorario.Mid(i,1);
	}
	
	if(sHorarioNew.Mid(0,4)<L"2007")
	{
		return FALSE;
	}
	else
		dhAtual.wYear = (WORD)_ttoi(sHorarioNew.Mid(0,4));

	
	switch(_ttoi(sHorarioNew.Mid(4,2))){
	case 2:
		if(sHorarioNew.Mid(6,2)>L"29")
		{
			return FALSE;
		}
		break;
	case 4:
	case 6:
	case 9:
	case 11:
		if(sHorarioNew.Mid(6,2)>L"30")
		{
			return FALSE;
		}
		break;
	case 1:
	case 3:
	case 5:
	case 7:
	case 8:
	case 10:
	case 12:
		if(sHorarioNew.Mid(6,2)>L"31")
		{
			return FALSE;
		}
		break;
	default:
		return FALSE;
		break;
	}
	 
	dhAtual.wMonth = (WORD)_ttoi(sHorarioNew.Mid(4,2));
	dhAtual.wDay = (WORD)_ttoi(sHorarioNew.Mid(6,2));

	if(sHorarioNew.Mid(8,2)>L"24")
	{
		return FALSE;
	}
	else
		dhAtual.wHour = (WORD)_ttoi(sHorarioNew.Mid(8,2));

	if(sHorarioNew.Mid(10,2)>L"59")
	{
		return FALSE;
	}
	else
		dhAtual.wMinute = (WORD)_ttoi(sHorarioNew.Mid(10,2));

	if(sHorarioNew.Mid(12,2)>L"59")
	{
		return FALSE;
	}
	else
		dhAtual.wSecond = (WORD)_ttoi(sHorarioNew.Mid(12,2));

	
	/*GetLocalTime(&dhAtual);
	COleDateTime now(dhAtual);
	CString sDate1 = now.Format(_T("%A, %d/%m/%y - %H:%M:%S"));
	AfxMessageBox(sDate1);
	
	COleDateTime odt = COleDateTime::GetCurrentTime();
	CString sDate2 = odt.Format(_T("%A, %d/%m/%y - %H:%M:%S"));
	AfxMessageBox(sDate2);*/

	if(!SetLocalTime(&dhAtual))
	{
		STLOG_WRITE("CUtil::SetDataHoraServidor() : Erro ajustando relogio %ld", GetLastError());
		return FALSE;
	} 

	SetLastAtualizacao(&dhAtual);

	return TRUE;
}

void CUtil::SetLastAtualizacao(LPSYSTEMTIME st)
{
	double d;
	CString sRegTime;
	SystemTimeToVariantTime(st, &d);
	sRegTime.Format(L"%f", d);

	Registry reg(HKEY_LOCAL_MACHINE, L"\\Software\\Engebras\\PMA");

	if(!reg.Open())
	{
		STLOG_WRITE(L"%s(%d): Erro ao abrir o registro: %s", __FUNCTIONW__, __LINE__, reg.GetLastErrorString());
	}
	else
	{
		reg.SetValue(L"LAST_UPDATE", sRegTime);
		STLOG_WRITE(L"%s(%d): Gravando no registry %s", __FUNCTIONW__, __LINE__, sRegTime);
	}
	
}

void CUtil::GetLastAtualizacao(LPSYSTEMTIME st)
{
	double d;
	CString sRegTime;
	
	Registry reg(HKEY_LOCAL_MACHINE, L"\\Software\\Engebras\\PMA");
	
	if(!reg.Open())
	{
		STLOG_WRITE(L"%s(%d): Erro ao abrir o registro: %s", __FUNCTIONW__, __LINE__, reg.GetLastErrorString());
	}
	else
	{
		reg.GetString(L"LAST_UPDATE", sRegTime);
	}

	d = atof(CStringA(sRegTime));
	
	VariantTimeToSystemTime(d, st);
}

BOOL CUtil::CheckAtualizacaoDate()
{
	SYSTEMTIME st_old, st_upd;
	ZeroMemory(&st_old, sizeof(st_old));
	ZeroMemory(&st_upd, sizeof(st_upd));

	CUtil::GetLastAtualizacao(&st_upd);

	CFile fileDate;
	CString sFileName;
	CStr sText;

	GetLocalTime(&st_old);

	CString sMsg;
	int nError;
	//sMsg.Format(L"Não é possível alterar data. A data têm que ter 20 dias de diferença de: %d/%d", st_upd.wDay, st_upd.wMonth);
	if(!CUtil::IsIntervaloDataTimeValid(&st_old, 'D', 20, nError, 'P', &st_upd) && !CUtil::IsIntervaloDataTimeValid(&st_old, 'D', 20, nError, 'F', &st_upd))
	{
		STLOG_WRITE(L"%s(%d): Diferença maior que 20 dias", __FUNCTIONW__, __LINE__);
		//MessageBox(sMsg, L"Mensagem", MB_ICONINFORMATION | MB_OK);
		return FALSE;
	}

	return TRUE;
}

/**
\brief Verifica se arquivo existe
\param LPCTSTR szFile Path do arquivo
\return BOOL
*/
BOOL CUtil::FileExists(LPCTSTR szFile)
{
	WIN32_FIND_DATA wfd;
	HANDLE hFind = FindFirstFile( szFile, &wfd );

	if(hFind != INVALID_HANDLE_VALUE && wcslen(szFile)>0)
	{
		FindClose(hFind);
		return TRUE;
	}

	return FALSE;
}

/**
\brief Move arquivo sobrescrevendo
\param LPCTSTR szPathOrigem  Path do arquivo origem
\param LPCTSTR szPathDestino Path do arquivo destino
\return BOOL
*/
BOOL CUtil::MoveFileOverwrite(LPCTSTR szPathOrigem, LPCTSTR szPathDestino)
{
	if(CUtil::FileExists(szPathDestino))
		DeleteFile(szPathDestino);

	return MoveFileW(szPathOrigem, szPathDestino);	
}

/**
\brief Lista todos os diretórios de dispositivos flash presentes no dispositivo fazendo chamada ao médoto GetFlashCardsInfo()
\param CMapStringToString &strMap Lista de dispositivos flash
\return BOOL
*/
#ifdef _WIN32_WCE
BOOL CUtil::ListFlashCards(CMapStringToString &strMap)
{
	WIN32_FIND_DATA wfd;
	HANDLE h = FindFirstFlashCard(&wfd);
	if(h != INVALID_HANDLE_VALUE)
	{
		do
		{
			strMap.SetAt(wfd.cFileName, L"");

		} while(FindNextFlashCard(h, &wfd));

		GetFlashCardsInfo(strMap);

		return TRUE;
	}

	return FALSE;
}
#endif

#ifdef _WIN32_WCE
CString CUtil::ListStorages()
{	

	TCHAR szMsg[MAX_PATH]=L"";
	TCHAR szStore[MAX_PATH]=L"";
	BOOL res;
	HANDLE hStore;
	HANDLE hPart;
	HANDLE hOStore;
	STOREINFO si;
	memset(&si, 0, sizeof(STOREINFO));
	si.cbSize = sizeof(STOREINFO);

	PARTINFO pi;
	memset(&pi, 0, sizeof(PARTINFO));
	pi.cbSize = sizeof(PARTINFO);

	hStore = FindFirstStore(&si);
	if (hStore!=NULL && hStore!=INVALID_HANDLE_VALUE)
	{
		wcscat(szMsg, L"Storages neste Equipamento: \r\n");
		do
		{
			wsprintf(szStore,L"%s (%s)\r\n",si.szDeviceName,si.szStoreName);
			wcscat(szMsg,szStore);

			hOStore = OpenStore(si.szDeviceName);

			hPart = FindFirstPartition(hOStore, &pi);

			do
			{
				CString sValue, sKey(pi.szVolumeName);
				if(sKey.GetAt(0) == L'\\') sKey.Delete(0, 1);

				wsprintf(szStore, L"\t - %s\r\n", pi.szVolumeName);
				wcscat(szMsg,szStore);
			}
			while(FindNextPartition(hPart, &pi));

			res = FindNextStore(hStore,&si);
		} while (res == TRUE); 
		
	}
	return szMsg;
}
#endif


/**
\brief Adquire informações de dispositivos flash presentes no dispositivo
\param CMapStringToString &strMap Lista de dispositivos flash
\param CString* sInfoType Filto do tipo de dispositivo, por exemplo: "SDMMC Card" ou "SD Memory Card". O default é NULL 
\return void
*/
#ifdef _WIN32_WCE
void CUtil::GetFlashCardsInfo(CMapStringToString &strMap, CString* sInfoType)
{	

	//TCHAR szMsg[MAX_PATH]=L"";
	TCHAR szStore[MAX_PATH]=L"";
	BOOL res;
	HANDLE hStore;
	HANDLE hPart;
	HANDLE hOStore;
	STOREINFO si;
	memset(&si, 0, sizeof(STOREINFO));
	si.cbSize = sizeof(STOREINFO);

	PARTINFO pi;
	memset(&pi, 0, sizeof(PARTINFO));
	pi.cbSize = sizeof(PARTINFO);

	hStore = FindFirstStore(&si);
	if (hStore!=NULL && hStore!=INVALID_HANDLE_VALUE)
	{
		//wcscat(szMsg, L"Storages neste Equipamento: \r\n");
		do
		{
			wsprintf(szStore,L"%s (%s)\r\n",si.szDeviceName,si.szStoreName);
			//wcscat(szMsg,szStore);

			hOStore = OpenStore(si.szDeviceName);

			hPart = FindFirstPartition(hOStore, &pi);

			do
			{
				CString sValue, sKey(pi.szVolumeName);
				if(sKey.GetAt(0) == L'\\') sKey.Delete(0, 1);
				if(strMap.Lookup(sKey,sValue))
					strMap.SetAt(sKey, si.szStoreName);

				if(sInfoType!=NULL)
				{
					if(sInfoType->Compare(si.szStoreName)==0)
					{
						*sInfoType = sKey;
						return;
					}

				}

				wsprintf(szStore, L"\t - %s\r\n", pi.szVolumeName);
				//wcscat(szMsg,szStore);
			}
			while(FindNextPartition(hPart, &pi));

			res = FindNextStore(hStore,&si);
		} while (res == TRUE); 
		
//		STLOG_WRITE(szMsg);
		if(sInfoType != NULL)
			*sInfoType = L"";
		//MessageBox(NULL,szMsg,L"Store Available",MB_SETFOREGROUND);
	}
}
#endif


/**
\brief Adquire diretórios de dispositivos flash do tipo SD Card presentes no dispositivo
\param void
\return CString
*/
#ifdef _WIN32_WCE
CString CUtil::GetSDCardPath() 
{
	CString sValue("SD Memory Card");
	CMapStringToString strMap;

	ListFlashCards(strMap); 
	GetFlashCardsInfo(strMap, &sValue);

	if(sValue.IsEmpty())
	{
		sValue = L"SDMMC Card";
		GetFlashCardsInfo(strMap, &sValue);
	}

	if(sValue.IsEmpty())
	{
		sValue = L"EMULATOR SHARED FOLDER FS";
		GetFlashCardsInfo(strMap, &sValue);
	}


	return sValue;
}
#endif


/**
\brief Adquire path do backup, provável File Store
\param void
\return CString
*/
#ifdef _WIN32_WCE
CString CUtil::GetBackupPath()
{
	CMapStringToString strMap;
	ListFlashCards(strMap);

	POSITION pa;
	CString k, v;

	if(strMap.GetCount() > 3)
		return L"";
	
	pa = strMap.GetStartPosition();

	while(pa)
	{
		strMap.GetNextAssoc(pa, k, v);

		if(v.Trim().CompareNoCase(L"SD Memory Card")==0 ||
			v.Trim().CompareNoCase(L"SDMMC Card")==0 ||
			k.Trim().IsEmpty())
			continue;
		else
			return k;
	}

	return L"";
}
#endif

/**
\brief Obtém versão de uma DLL
\param LPCTSTR szDllName Nome da DLL
\return CString
*/
CString CUtil::GetDLLVersion(LPCTSTR szDllName)
{
	CString s;
	static TCHAR sBackSlash[] = {'\\','\0'};

	DWORD dwVersionDataLen = GetFileVersionInfoSizeW((LPTSTR)szDllName, NULL);
	if(dwVersionDataLen) 
	{
		LPVOID pVersionData = malloc(dwVersionDataLen);
		if(GetFileVersionInfoW((LPTSTR)szDllName, 0, dwVersionDataLen, pVersionData)) 
		{
			LPVOID pVal;
			UINT nValLen;
			if(VerQueryValueW(pVersionData, sBackSlash, &pVal, &nValLen)) 
			{
				if(nValLen == sizeof(VS_FIXEDFILEINFO)) 
				{
					VS_FIXEDFILEINFO *pFixedFileInfo = (VS_FIXEDFILEINFO*)pVal;
					char buffer[1024];
					sprintf(buffer, 
							"%d.%d.%d.%d", 
							HIWORD(pFixedFileInfo->dwFileVersionMS), 
							LOWORD(pFixedFileInfo->dwFileVersionMS),
							HIWORD(pFixedFileInfo->dwFileVersionLS), 
							LOWORD(pFixedFileInfo->dwFileVersionLS));

					s.Format(L"%S", buffer);
				}
			}
		}

		free(pVersionData);
	}

	return s;
}

/**
\brief Obtém nome do fabricante do dispositivo
\param void
\return CString
*/
#ifdef _WIN32_WCE
CString CUtil::GetDeviceManufactureName()
{
	TCHAR wszMachineName[128];

	SystemParametersInfo(SPI_GETOEMINFO, sizeof(wszMachineName), &wszMachineName, 0);
	CString machineName(wszMachineName);
    
	return machineName;
}
#endif


/**
\brief Adquire descrição do erro conforme seu código
\param DWORD dwLastError Código do erro
\return CString
*/
CString CUtil::ErrorMessage(DWORD dwLastError)
{
	LPTSTR szMsg;

	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
      FORMAT_MESSAGE_FROM_SYSTEM,
      NULL,
      dwLastError,
      0,
      (LPTSTR)&szMsg,
      0,
      NULL);

	CString strErrMsg;

	strErrMsg.Format(L"0x%08X - %s", dwLastError, szMsg);

	return strErrMsg;
}


/**
\brief Ajusta data/hora do dispositivo adicionando segundos, minutos, horas ou dias
\param const char flgElementoTempo Elemento de tempo a ser ajustado (D = dias, H = horas, M = minutos, S = segundos)
\param int grandezaTempo Grandeza de tempo a ser somado
\return BOOL
*/
BOOL CUtil::AddDateTime(const char flgElementoTempo, int grandezaTempo)
{	
	SYSTEMTIME	st;	
	GetLocalTime(&st); // retorna certo com fuso-horário! formato system
	CTime now(st);	   // transforma pra formato legivel	
	CTime newTime;
	
	//AfxMessageBox(now.Format(L"%A, %B %d, %Y, %H:%M:%S" ));
	switch(flgElementoTempo){
		case 'D':
		case 'd':
			newTime = now + CTimeSpan(grandezaTempo,0,0,0);
		break;				

		case 'H':
		case 'h':
			newTime = now + CTimeSpan(0,grandezaTempo,0,0);
		break;

		case 'M':
		case 'm':
			newTime = now + CTimeSpan(0,0,grandezaTempo,0);
		break;

		case 'S':
		case 's':
			newTime = now + CTimeSpan(0,0,0,grandezaTempo);
		break;

		default:
			return FALSE;		
	}
	    
	//AfxMessageBox(newTime.Format(L"%A, %B %d, %Y, %H:%M:%S"));

	newTime.GetAsSystemTime(st); // transforma p/ formato system
	if(SetLocalTime(&st))
		return TRUE;
	else
		return FALSE;	
}


#define MAX_PLATFORM  64
/**
\brief Determina se a platform é Smartphone
\param void
\return BOOL
*/
#ifdef _WIN32_WCE
BOOL CUtil::IsSmartphone()
{
    HRESULT hr;
    TCHAR   szPlatform[MAX_PLATFORM] = { 0 };
    BOOL    bResult = FALSE;

    CBR(SystemParametersInfo(SPI_GETPLATFORMTYPE, ARRAYSIZE(szPlatform), szPlatform, 0));

    if (0 == _tcsicmp(szPlatform, TEXT("Smartphone")))
    {
        bResult = TRUE;
    }

_ErrorLabel:
    return bResult;
}
#endif

BOOL CUtil::IsIntervaloDataTimeValid(LPSYSTEMTIME stComp, const char flgElementoTempo, const int grandezaTempo, int &nError, const char flgFuturoPassado, LPSYSTEMTIME lpst)
{
	CString sData;
	sData.Format(L"%d/%d/%d", stComp->wDay, stComp->wMonth, stComp->wYear);

	return IsIntervaloDataTimeValid(sData, flgElementoTempo, grandezaTempo, nError, flgFuturoPassado, lpst);
}

/** 
\brief Determina a quantidade de segundos entre duas datas
\param const SYSTEMTIME &st1 Data 1
\param const SYSTEMTIME &st1 Data 2
\return A diferença em segundos entre st1 e st2. Se st2 for maior que st1 o resultado é negativo.
*/
int CUtil::CalculateSecondsBetween(const SYSTEMTIME &st1, const SYSTEMTIME &st2)
{
	union timeunion
	{
		FILETIME fileTime;
		LARGE_INTEGER ul;
	};

	timeunion ft1;
	timeunion ft2;

	SystemTimeToFileTime(&st1, &ft1.fileTime);
	SystemTimeToFileTime(&st2, &ft2.fileTime);

	return (ft1.ul.QuadPart - ft2.ul.QuadPart) / 10000000;
}

int CUtil::CalculateSecondsNow(const CString &st1)
{
	SYSTEMTIME st;
	SYSTEMTIME st2;	

	//00/00/0000 00:00:00

	st.wDay = _wtoi(st1.Mid(0,2));
	st.wMonth = _wtoi(st1.Mid(3,2));
	st.wYear = _wtoi(st1.Mid(6,4));

	st.wHour = _wtoi(st1.Mid(11,2));
	st.wMinute = _wtoi(st1.Mid(14,2));
	st.wSecond = _wtoi(st1.Mid(17,2));	

	GetLocalTime(&st2);

	return CalculateSecondsBetween(st2, st);
}

/**
\brief Determina se data está dentro de um intervalo válido
\details
	Verifica se está exatamente dentro de um intervalor. 
	Exemplo: Comparando 1s entre data informada e data atual dará true somente se a diferença for realmente um segundo
\param const CString dataComp Data a ser comparada no formato DD/MM/YYYY
\param const char flgElementoTempo Elemento de tempo a ser comparado (D dia,H hora,M minuto ou S segundo)
\param const int  grandezaTempo	Grandeza do elemento de tempo citado acima
\param const char flgFuturoPassado = 'P' Flag de comparação. P passado ou F futuro. P é o default 
\return BOOL FALSE dataComp não é válida p/ o intervalo comparado. TRUE dataComp é válida. 
\todo A parte de comparação de datas futuras necessita ajustes	
*/
BOOL CUtil::IsIntervaloDataTimeValid(const CString dataComp, const char flgElementoTempo, const int grandezaTempo, int &nError, const char flgFuturoPassado/*='P'*/, LPSYSTEMTIME lpst)
{
	nError = 0;
	if(dataComp.IsEmpty())
	{
		nError = -1;
		return FALSE;
	}

	COleDateTime m_dataComp;
	m_dataComp.ParseDateTime(dataComp);

	SYSTEMTIME	st;	
	if(lpst == NULL)
	{
		GetLocalTime(&st); //retorna com fuso-horário, formato system
	}
	else
	{
		st = *lpst;
	}
		
	COleDateTime now(st), newTime;	
	
	
	switch(flgElementoTempo){
		case 'D':
		case 'd':
			newTime = flgFuturoPassado == 'P' ? now - COleDateTimeSpan(grandezaTempo,0,0,0)
											  : now + COleDateTimeSpan(grandezaTempo,0,0,0);			
		break;				

		case 'H':
		case 'h':
			newTime = flgFuturoPassado == 'P' ? now - COleDateTimeSpan(0,grandezaTempo,0,0)
											  : now + COleDateTimeSpan(0,grandezaTempo,0,0);
		break;

		case 'M':
		case 'm':
			newTime = flgFuturoPassado == 'P' ? now - COleDateTimeSpan(0,0,grandezaTempo,0)
											  : now + COleDateTimeSpan(0,0,grandezaTempo,0);
		break;

		case 'S':
		case 's':
			newTime = flgFuturoPassado == 'P' ? now - COleDateTimeSpan(0,0,0,grandezaTempo)
											  : now + COleDateTimeSpan(0,0,0,grandezaTempo);
		break;

		default:
			nError = -2;
			return FALSE;
			break;
	}


	if(flgFuturoPassado == 'P' || flgFuturoPassado == 'p')
	{
		//Se for para comparar dia, despreza parte decimal das variaveis de comparacao
		if(flgElementoTempo == 'D' || flgElementoTempo == 'd')
		{
			if(int(m_dataComp) >= int(newTime) && int(m_dataComp) <= int(now))
				return TRUE;
			else
				return FALSE;
		}
		else
		{
			if(m_dataComp >= newTime && m_dataComp <= now)
				return TRUE;
			else
				return FALSE;
		}
	}
	else if(flgFuturoPassado == 'F' || flgFuturoPassado == 'f')
	{	
		//Se for para comparar dia, despreza parte decimal das variaveis de comparacao
		if(flgElementoTempo == 'D' || flgElementoTempo == 'd')
		{
			if(int(m_dataComp) >= int(now) && int(m_dataComp) <= int(newTime))
				return TRUE;
			else
				return FALSE;	
		}
		else
		{
			if(m_dataComp >= now && m_dataComp <= newTime)
				return TRUE;
			else
				return FALSE;	
		}
	}
	else
	{
		return FALSE;
		nError = -3;
	}
}

/**
\brief Verifica se a string é formada por pontos "." ou ".."
\param const TCHAR* str String a verificar
\return BOOL
*/
BOOL CUtil::IsDots(const TCHAR* str) {
    if(_tcscmp(str,L".") && _tcscmp(str,L"..")) return FALSE;
    return TRUE;
}

/**
\brief Deleta diretórios com seu conteúdo
\param const TCHAR* sPath Path do diretório
\return BOOL
*/
BOOL CUtil::DeleteDirectory(const TCHAR* sPath)
{
	HANDLE hFind; // file handle
	WIN32_FIND_DATA FindFileData;

	TCHAR DirPath[MAX_PATH];
	TCHAR FileName[MAX_PATH];

	_tcscpy(DirPath,sPath);
	_tcscat(DirPath,_T("\\"));
	_tcscpy(FileName,sPath);
	_tcscat(FileName,_T("\\*")); // searching all files

	hFind = FindFirstFile(FileName, &FindFileData); // find the first file
	if( hFind != INVALID_HANDLE_VALUE )
	{
		do
		{
			if( IsDots(FindFileData.cFileName) )
				continue;

			_tcscpy(FileName + _tcslen(DirPath), FindFileData.cFileName);
			if((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				// we have found a directory, recurse
				if( !DeleteDirectory(FileName) )
					break; // directory couldn't be deleted
				}
			else
			{
				if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
				{
					SetFileAttributes(FileName, FILE_ATTRIBUTE_NORMAL);
					//_chmod(FileName, _S_IWRITE); // change read-only file mode					
				}

				if( !DeleteFile(FileName) )
					continue; // file couldn't be deleted
			}

		}while( FindNextFile(hFind,&FindFileData) );
		FindClose(hFind); // closing file handle
	}
	return RemoveDirectory(sPath); // remove the empty (maybe not) directory
}


/**
\brief Realiza pesquisa por arquivos em um determinado diretório
\param const CString sFile Arquivo a ser pesquisado
\param CStringArray *arrFiles Array de aruivos encontrados
\return BOOL
*/
BOOL CUtil::SearchFilesInDir(const CString sFile, CStringArray *arrFiles)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = FindFirstFile(sFile, &FindFileData); // find the first file	

	if( hFind != INVALID_HANDLE_VALUE )
	{
		do
		{			
			if( IsDots(FindFileData.cFileName) )
				continue;
			
			if((!FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				if(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
					SetFileAttributes(FindFileData.cFileName, FILE_ATTRIBUTE_NORMAL);									
			}

			arrFiles->Add(FindFileData.cFileName);
		}
		while( FindNextFile(hFind,&FindFileData) );
	
		FindClose(hFind); // closing file handle

		return TRUE;
	}
	
	return FALSE;
}

CString CUtil::ListFilesInDir(const CString sDir)
{
	CString sReturn;
	WIN32_FIND_DATA ffd;
	LARGE_INTEGER filesize;
	TCHAR szDir[MAX_PATH];
	size_t length_of_arg;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError=0;

	sReturn.Format(L"Estrutura do diretório [%s]\r\n",sDir);

	// Check that the input path plus 2 is not longer than MAX_PATH.

	StringCchLength(sDir, MAX_PATH, &length_of_arg);

	if (length_of_arg > (MAX_PATH - 2))
	{
	  return L"\nDirectory path is too long.\n";
	}

	//_tprintf(TEXT("\nTarget directory is %s\n\n"), sDir);

	// Prepare string for use with FindFile functions.  First, copy the
	// string to a buffer, then append '\*' to the directory name.

	StringCchCopy(szDir, MAX_PATH, sDir);
	StringCchCat(szDir, MAX_PATH, TEXT("\\*"));

	// Find the first file in the directory.

	hFind = FindFirstFile(szDir, &ffd);

	if (INVALID_HANDLE_VALUE == hFind) 
	{
	  return L"Não foi possivel listar os arquivos";
	} 

	// List all the files in the directory with some info about them.

	do
	{
	  if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	  {
		sReturn.AppendFormat(TEXT("  %s   <DIR>\r\n"), ffd.cFileName);
	  }
	  else
	  {
		 filesize.LowPart = ffd.nFileSizeLow;
		 filesize.HighPart = ffd.nFileSizeHigh;
		 sReturn.AppendFormat(TEXT("  %s   %ld bytes\r\n"), ffd.cFileName, filesize.QuadPart);
	  }
	}
	while (FindNextFile(hFind, &ffd) != 0);

	dwError = GetLastError();

	FindClose(hFind);
	
	return sReturn;
}

/**
\brief Deleta cache do Internet Explorer
\param void
\return void
*/
void CUtil::DeleteCacheIE()
{
	#ifdef _WIN32_WCE
		DeleteDirectory(L"\\Windows\\Profiles\\guest\\Temporary Internet Files\\Content.IE5");
	#endif	
}


/**
\brief Obtém a senha para liberação do produto para um determinado contrato
\param LPCTSTR produto Nome do produto
\param LPCTSTR contrato Nome do contrato
\param int type Período de validade da senha
\param int nmrand Semente de geração da senha
\return CString
*/
CString CUtil::GetPassword(LPCTSTR produto, LPCTSTR contrato, int type, int nmrand)
{
	SYSTEMTIME    st;
	char          dat[10];
	CString       sContrato(contrato);
	CString       sProduto(produto);
	char *contr   = new char[sContrato.GetLength() + 1];
	char *prod    = new char[sProduto.GetLength() + 1];
	wcstombs(contr, sContrato, sContrato.GetLength() + 1);
	wcstombs(prod, sProduto, sProduto.GetLength() + 1);

	GetLocalTime(&st);
	
	sprintf(dat, "%04i%02i%02i", st.wYear, st.wMonth, st.wDay);


	GPass   gp;
	CString sGp;
	int     valor = 0;

	gp.setBase(32);
	gp.setContrato(contr);
	gp.setDate(dat);
	gp.setProduto(prod);
	gp.setRand(nmrand);

	if (type == 0)
	{
		valor = (int)st.wDay;
		sGp = gp.getDayPassword(valor).c_str();
	}
	else if (type == 1)
	{
		if (st.wHour < 10)
			valor = 9;
		else if (st.wHour < 19)
			valor = 1019;
		else if (st.wHour < 24)
			valor = 2023;

		sGp = gp.getHourParcialPassword(valor).c_str();
	}
	else if (type == 2)
	{
		valor = st.wHour;

		if (valor % 2)
			valor--;

		valor = valor * 100 + valor + 1;
		sGp = gp.getHourDoublePassword(valor).c_str();
	}
	else if (type == 3)
	{
		valor = st.wHour;
		sGp = gp.getHourPassword(valor).c_str();
	}

	return sGp;
}


/**
\brief Obtém path conforme nome de variável recebida. Esse método é utilizado no parse das variáveis do XML
\param CString &sPath Nome de variável
\return BOOL
*/
BOOL CUtil::GetPathFromVariable(CString &sPath)
{
	BOOL bRet = TRUE;

	if(sPath.Find(_T("$path$")) >= 0)
		sPath.Replace(_T("$path$"), GetMainAppPath());
	else if(sPath.Find(_T("$sdcard$")) >= 0)
	{
#ifdef _WIN32_WCE
		CString s = GetSDCardPath();
#else
		CString s = L"";
#endif
		if(!s.IsEmpty())
			sPath.Replace(_T("$sdcard$"), s);
		else
		{
			STLOG_WRITE("CUtil::GetPathFromVariable - Erro tentando recuperar o path para o SDCard");
			bRet = FALSE;
		}
	}
	else if(sPath.Find(_T("$filestore$")) >= 0)
	{
		// Tenta recuperar o path
#ifdef _WIN32_WCE
		CString s = GetBackupPath();
#else
		CString s = GetAppPath();
#endif
		if(!s.IsEmpty())
			sPath.Replace(_T("$filestore$"), s);
		else
		{
			// Se deu erro, nao parar, vamos substituir o path pelo padrao...
			STLOG_WRITE("CUtil::GetPathFromVariable - Erro tentando recuperar o path para o FileStore");
			sPath.Replace(_T("$filestore$"), GetMainAppPath());

			bRet = FALSE;
		}
	}
	else if(sPath.Find(_T("$jobdir$")) >= 0)
	{
		// Tenta recuperar o path
		CString s = GetJobPath();
		if(!s.IsEmpty())
			sPath.Replace(_T("$jobdir$"), s);
		else
		{
			// Se deu erro, nao parar, vamos substituir o path pelo padrao...
			STLOG_WRITE("CUtil::GetPathFromVariable - Erro tentando recuperar o path para o diretório de JOBs");
			sPath.Replace(_T("$jobdir$"), GetAppPath());

			bRet = FALSE;
		}
	}

	return bRet;
}

/**
\brief Retira todos os acentos e caracteres especiais de uma string
\param CString str String de transformação
\return CString
*/
CString CUtil::RetiraAcentos(CString str)
{	
	str.Replace(L"á", L"a");
	str.Replace(L"à", L"a");
	str.Replace(L"â", L"a");
	str.Replace(L"ä", L"a");
	str.Replace(L"ã", L"a");

    str.Replace(L"è", L"e");
	str.Replace(L"é", L"e");
	str.Replace(L"ê", L"e");
	str.Replace(L"ë", L"e");

    str.Replace(L"ì", L"i");
	str.Replace(L"í", L"i");
	str.Replace(L"î", L"i");
	str.Replace(L"ï", L"i");

    str.Replace(L"ò", L"o");
	str.Replace(L"ó", L"o");
	str.Replace(L"ô", L"o");
	str.Replace(L"ö", L"o");
	str.Replace(L"õ", L"o");

    str.Replace(L"ù", L"u");
	str.Replace(L"ú", L"u");
	str.Replace(L"û", L"u");
	str.Replace(L"ü", L"u");
	
	str.Replace(L"À", L"A");
	str.Replace(L"Á", L"A");
	str.Replace(L"Â", L"A");
	str.Replace(L"Ä", L"A");
	str.Replace(L"Ã", L"A");

    str.Replace(L"È", L"E");
	str.Replace(L"É", L"E");
	str.Replace(L"Ê", L"E");
	str.Replace(L"Ë", L"E");

	str.Replace(L"Ì", L"I");
	str.Replace(L"Í", L"I");
	str.Replace(L"Î", L"I");
	str.Replace(L"Ï", L"I");
    
    str.Replace(L"Ò", L"O");
	str.Replace(L"Ó", L"O");
	str.Replace(L"Ô", L"O");
	str.Replace(L"Ö", L"O");
	str.Replace(L"Õ", L"O");

	str.Replace(L"Ù", L"U");
	str.Replace(L"Ú", L"U");
	str.Replace(L"Û", L"U");
	str.Replace(L"Ü", L"U");
    
    str.Replace(L"ç", L"c");
	str.Replace(L"Ç", L"C");
    
    str.Replace(L"ñ", L"n");
	str.Replace(L"Ñ", L"N");

    str.Replace(L"´", L"");
	str.Replace(L"`", L"");
	str.Replace(L"¨", L"");
	str.Replace(L"^", L"");
	str.Replace(L"~", L"");
	str.Replace(L"\\", L"");
     
    return str;
}

/**
\brief Valida um CPF
\param const CString sCPF CPF a ser validado
\return BOOL
*/
BOOL CUtil::ValidaCPF(const CString sCPF)
{	
	if (sCPF.GetLength() != 11)
		return FALSE;

	BOOL bIguais = TRUE;

	for(int i=1; i<sCPF.GetLength(); i++)
	{
		if(sCPF[0] != sCPF[i])
			bIguais = FALSE;
	}
    
	if (bIguais)
	{
		return FALSE;		
	}	
	
	int sum=0, dig1=0, dig2=0, i;

	for(i=0; i<=8; i++)
		sum += (11 - (i + 1)) * _wtoi(sCPF.Mid(i,1));		
	
	sum = sum%11;

    if (sum < 2) 
		dig1 = 0;
    else 
		dig1 = 11-sum;
	
    if (dig1 == _wtoi(sCPF.Mid(9,1)))
	{
		sum = 0;
		for(i=0; i<=9; i++)
			sum += (11 - i) * _wtoi(sCPF.Mid(i,1));		
       
        sum = sum%11;

        if (sum < 2)
			dig2 = 0;
        else 
			dig2 = 11-sum;
        
        if (dig2 == _wtoi(sCPF.Mid(10,1)))
            return TRUE;        
    } 
	
	return FALSE;   
}

/**
\brief Valida um CNH
\param const CString sCNH CNH a ser validado
\return BOOL
*/
BOOL CUtil::ValidaCNH(CString &sCNH)
{	
	int sum=0, tot=0, dig=0, i;

	if (sCNH.GetLength() > 11)
	{
		return FALSE;
	}

	BOOL bIguais = TRUE;

	for(i=1; i<sCNH.GetLength(); i++)
	{
		if(sCNH[0] != sCNH[i])
			bIguais = FALSE;
	}

	if (bIguais)
	{
		return FALSE;		
	}	

	CString sTmp;

	for(i=0; i<11-sCNH.GetLength(); i++)
	{
		sTmp+='0';
	}
	sCNH = sTmp + sCNH;
    

	for(i=0; i<sCNH.GetLength()-2; i++)
		sum += (_wtoi(sCNH.Mid(i,1)) * (i + 2));
	
	tot = (sum/11) * 11;

    if (sum-tot < 2) 
		dig = 0;
    else 
		dig = 11-(sum-tot);
	
    if (dig == _wtoi(sCNH.Mid(9,1)))
	{
		return TRUE;        
    } 
	
	return FALSE;   
}

/**
\brief Valida um CNH do padrão antigo
\param const CString sCNH CNH a ser validado
\return BOOL
*/
BOOL CUtil::ValidaCNHOld(CString &sCNH)
{	
	int sum=0, tot=0, dig=0, i;

	if (sCNH.GetLength() > 9)
	{
		return FALSE;
	}

	BOOL bIguais = TRUE;

	for(i=1; i<sCNH.GetLength(); i++)
	{
		if(sCNH[0] != sCNH[i])
			bIguais = FALSE;
	}

	if (bIguais)
	{
		return FALSE;		
	}	

	CString sTmp;

	for(i=0; i<9-sCNH.GetLength(); i++)
	{
		sTmp+='0';
	}
	sCNH = sTmp + sCNH;
    
	

	for(i=0; i<sCNH.GetLength()-1; i++)
		sum += (_wtoi(sCNH.Mid(i,1)) * (i + 2));
	
	tot = sum % 11;

	if(tot == 10)
		tot = 0;
	
    if (tot == _wtoi(sCNH.Mid(8,1)))
	{
		return TRUE;        
    } 
	
	return FALSE;    
}

/**
\brief Valida um CNPJ
\param const CString sCNPJ CNPJ a ser validado
\return BOOL
*/
BOOL CUtil::ValidaCNPJ(const CString sCNPJ)
{
	if (sCNPJ.GetLength() != 14)
		return FALSE;

	if (sCNPJ.Compare(L"00000000000000") == 0) 		
		return FALSE;		
	   
	int sum=0, dig1=0, dig2=0, i;

	for(i=0; i<=11; i++)
	{
		if(i<=3)
			sum += (5 - i) * _wtoi(sCNPJ.Mid(i,1));		
		else
			sum += (13 - i) * _wtoi(sCNPJ.Mid(i,1));		
	}
	
	sum = sum%11;

    if (sum < 2) 
		dig1 = 0;
    else 
		dig1 = 11-sum;
	
    if (dig1 == _wtoi(sCNPJ.Mid(12,1)))
	{
		sum = 0;
		for(i=0; i<=12; i++)
		{
			if(i<=4)
				sum += (6 - i) * _wtoi(sCNPJ.Mid(i,1));		
			else
				sum += (14 - i) * _wtoi(sCNPJ.Mid(i,1));		
		}		
       
        sum = sum%11;

        if (sum < 2)
			dig2 = 0;
        else 
			dig2 = 11-sum;
        
        if (dig2 == _wtoi(sCNPJ.Mid(13,1)))
            return TRUE;        
    } 	
	return FALSE;                
}

/**
\brief Termina a execução de um programa externo, se este estiver sendo executado
\param LPCTSTR szProcessName Programa a ser pesquisado
\return void
*/
void CUtil::TerminateProcessIfRunning(LPCTSTR szProcessName)
{
	/*CString sName(szProcessName);
	sName = sName.Right(sName.GetLength() - sName.ReverseFind(L'\\') - 1);
	
	static int index = 0;
	HANDLE snapHand;
	PROCESSENTRY32 procEntry;
	procEntry.dwSize = sizeof(PROCESSENTRY32);
	snapHand = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS | TH32CS_SNAPNOHEAPS,0);

	if(INVALID_HANDLE_VALUE == snapHand)
		return;

	if(!Process32First(snapHand,&procEntry))
	{
		return;
	}

	procEntry.dwSize = sizeof(procEntry);
	do
	{
		if(wcscmp(procEntry.szExeFile, sName)==0)
		{
			HANDLE h = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procEntry.th32ProcessID);
			TerminateProcess(h, 0);
			index = GetLastError();
			break;
		}
	}while(Process32Next(snapHand,&procEntry));*/
}

/**
\brief Cria processo de execução de programa externo
\param LPCTSTR szFile Arquivo executável
\param LPCTSTR szParam Parâmetros para execução do programa
\param CWnd* pWnd Handle da janela
\param BOOL bHideTaskbar Controla se a TaskBar deve ser escondida
\param BOOL bWait Espera o processo terminar
\return void
*/
void CUtil::ExecuteExternalProgram(LPCTSTR szFile, LPCTSTR szParam, CWnd* pWnd, BOOL bHideTaskbar, BOOL bWait)
{
	CString ss(szFile);

	CloseExternalProcess();

	if(CUtil::FileExists(ss))
	{
#ifdef _WIN32_WCE
		CWnd *pWnd = pWnd->FindWindow(_T("HHTaskBar"),_T(""));
		//pWnd->ShowWindow(SW_SHOW);
		
		if(pWnd != NULL && !bHideTaskbar)
		{
			pWnd->EnableWindow(TRUE);
			pWnd->Invalidate();
			pWnd->UpdateWindow();
		}
#endif

		if(!CreateProcessW(ss, 
#ifdef _WIN32_WCE
					   szParam,
#else
						(LPWSTR)szParam,
#endif
					   NULL, 
					   NULL, 
					   NULL, 
					   CREATE_NEW_CONSOLE, 
					   NULL, 
					   NULL, 
					   &si, 
					   &pi))
		{
			TCHAR buff[256];
			LoadString(m_hInst, IDS_ERROR_EXEC_APP, buff, 256);	
			MessageBox(pWnd->m_hWnd, buff, L"Mensagem", MB_ICONERROR|MB_OK);
			
			if(pWnd != NULL && !bHideTaskbar)
				pWnd->ShowWindow(SW_HIDE);

			return;
		}

		if(bWait)
		{
			// Wait until child process exits.
			 WaitForSingleObject( pi.hProcess, INFINITE );
		}

		// NOTA: Nao podemos esperar o processo encerrar porque muitos
		// ao clicar OK ficam minimizados e nao retornam nunca...

		
	}
}


/**
\brief Cria processo de execução de programa externo
\param LPCTSTR szFile Arquivo executável
\param LPCTSTR szParam Parâmetros para execução do programa
\param CWnd* pWnd Handle da janela
\param BOOL bHideTaskbar Controla se a TaskBar deve ser escondida
\param BOOL bWait Espera o processo terminar
\return void
*/
void CUtil::ExecuteExternalShortCut(LPCTSTR szFile, LPCTSTR szParam, CWnd* pWnd, BOOL bHideTaskbar, BOOL bWait)
{
	CString ss(szFile);

	CloseExternalProcess();

	SHELLEXECUTEINFO ShExecInfo;
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    ShExecInfo.fMask = NULL;
    ShExecInfo.hwnd = NULL;
    ShExecInfo.lpVerb = L"open"; //NULL;
    ShExecInfo.lpFile = ss; //pszParseName;
    ShExecInfo.lpParameters = NULL;
    ShExecInfo.lpDirectory = NULL;
    ShExecInfo.nShow = SW_MAXIMIZE;
    ShExecInfo.hInstApp = NULL;

    

	if(CUtil::FileExists(ss))
	{
#ifdef _WIN32_WCE
		CWnd *pWnd = pWnd->FindWindow(_T("HHTaskBar"),_T(""));
		//pWnd->ShowWindow(SW_SHOW);
		
		if(pWnd != NULL && !bHideTaskbar)
		{
			pWnd->EnableWindow(TRUE);
			pWnd->Invalidate();
			pWnd->UpdateWindow();
		}
#endif
		if (!ShellExecuteEx(&ShExecInfo))
		//if(!CreateProcessW(ss, 
////#ifdef _WIN32_WCE
////					   szParam,
////#else
////						(LPWSTR)szParam,
////#endif
////					   NULL, 
////					   NULL, 
////					   NULL, 
////					   CREATE_NEW_CONSOLE, 
////					   NULL, 
////					   NULL, 
////					   &si, 
////					   &pi))
		{
			TCHAR buff[256];
			LoadString(m_hInst, IDS_ERROR_EXEC_APP, buff, 256);	
			MessageBox(pWnd->m_hWnd, buff, L"Mensagem", MB_ICONERROR|MB_OK);
			
			if(pWnd != NULL && !bHideTaskbar)
				pWnd->ShowWindow(SW_HIDE);

			return;
		}

		pi.hProcess = ShExecInfo.hProcess;

		if(bWait)
		{
			// Wait until child process exits.
			 WaitForSingleObject( pi.hProcess, INFINITE );
		}

		// NOTA: Nao podemos esperar o processo encerrar porque muitos
		// ao clicar OK ficam minimizados e nao retornam nunca...

		
	}
}

/**
\brief Verifica se há programas em execução iniciados pela função ExecuteExternalProgram
\return void
*/
BOOL CUtil::IsProcessRunning()
{
	if(pi.hProcess != NULL)
	{
		return TRUE;
	}

	return FALSE;
}

/**
\brief Fecha programas em execução iniciados pela função ExecuteExternalProgram
\details Esta função também inicia informações para a função ExecuteExternalProgram
\return void
*/
void CUtil::CloseExternalProcess()
{
	if(pi.hProcess != NULL)
	{
		TerminateProcess(pi.hProcess, 0);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}

	ZeroMemory(&si, sizeof(STARTUPINFO));
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
	si.cb = sizeof(STARTUPINFO);
}

/**
\brief Informa se string é composta por números
\param const CString &s String a ser validada
\return BOOL
*/
BOOL CUtil::IsNumeric(const CString &s)
{		
	for(int i = 0; i < s.GetLength(); i++)
	{
		if(!_istdigit(s.GetAt(i)))
			return FALSE;
	}
	return TRUE;
}

/**
\brief Obtém a quantidade de memória disponível
\param void
\return DWORD
*/
DWORD CUtil::GetAvailableMemory()
{
	/*HINSTANCE hCoreDll = LoadLibrary(_T("coredll.dll")); 
	GetSystemMemoryDivisionProc procGet =
		(GetSystemMemoryDivisionProc)GetProcAddress( hCoreDll,_T("GetSystemMemoryDivision")); 
	
	DWORD dwStoragePages; 
	DWORD dwRamPages;
	DWORD dwPageSize;
	DWORD dwMemSize;

	if(!procGet(&dwStoragePages, &dwRamPages, &dwPageSize))
		dwMemSize = -1;
	else
		dwMemSize = (dwStoragePages * dwPageSize) / (1024 * 1024);
	
	FreeLibrary(hCoreDll); */

	MEMORYSTATUS ms;
	ms.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&ms);


	/*STLOG_WRITE("Global Memory Status: \r\n ->Lenght: %d \r\n ->MemoryLoad: %d \r\n ->TotalPhys: %d \r\n ->AvailPhys: %d \r\n ->TotalPageSize: %d \r\n ->AvailPageSize: %d \r\n ->TotalVirtual: %d \r\n ->AvailVirtual: %d \r\n",
		ms.dwLength/1024, ms.dwMemoryLoad/1024, ms.dwTotalPhys/1024, ms.dwAvailPhys/1024, ms.dwTotalPageFile/1024, ms.dwAvailPageFile/1024, ms.dwTotalVirtual/1024, ms.dwAvailVirtual/1024);*/

	/*if( (ms.dwAvailVirtual/1024) < (m_dwMemMessage*1024) )
	{
		m_dwMemMessage--;
		::MessageBox(GetForegroundWindow(), L"A memória do dispositivo está muito baixa. Recomenda-se reiniciar o equipamento.", L"Atenção!", MB_ICONEXCLAMATION|MB_OK);
	}*/
	
	//return dwMemSize;
	return ms.dwAvailVirtual/1024;
}


BOOL CUtil::MakeHash(const char * data, CString sAlgid, CString * sHash)
{
	HCRYPTPROV hProv = 0;
	HCRYPTHASH hHash = 0;
	BYTE *pbHash = NULL;
	DWORD dwHashLen;

	BYTE * pbBuffer = NULL;
	DWORD dwCount;
	DWORD i;
	unsigned long bufLen = 0;

	// Get a handle to the default provider using CryptAcquireContext
	if(!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
		STLOG_WRITE("CUtil::MakeHash() Erro %x durante CryptAcquireContext!", GetLastError());
		return FALSE;
	}
	
	// Create a hash object
	if(sAlgid.CompareNoCase(L"SHA1") == 0)
	{	
		if(!CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash)) {		
			STLOG_WRITE("CUtil::MakeHash() Erro %x durante CryptBeginHash!", GetLastError());
			return FALSE;
		}
	}
	else if(sAlgid.CompareNoCase(L"MD5") == 0)
	{
		if(!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash)) {
			STLOG_WRITE("CUtil::MakeHash() Erro %x durante CryptBeginHash!", GetLastError());
			return FALSE;
		}
	}

	// Calcula o tamanho do buffer de dados
	bufLen = strlen(data);
	
	pbBuffer = (BYTE*)malloc(bufLen +1);
	memset(pbBuffer, 0, bufLen +1);

	// Fill the buffer with data
	for(i = 0 ; i < bufLen ; i++) {
		pbBuffer[i] = (BYTE)data[i];
	}

	// Put the hash in buffer
	if(!CryptHashData(hHash, pbBuffer, bufLen, 0)) {
		STLOG_WRITE("CUtil::MakeHash() Erro %x durante CryptHashData!", GetLastError());
		return FALSE;
	}

	// Read the hash value size and allocate memory
	dwCount = sizeof(DWORD);
	
	if(!CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE *)&dwHashLen, &dwCount, 0)) {
		STLOG_WRITE("CUtil::MakeHash() Erro %x durante leitura do tamanho do hash!", GetLastError());
		return FALSE;
	}
	
	if((pbHash = (unsigned char*)malloc(dwHashLen)) == NULL) {
		STLOG_WRITE("CUtil::MakeHash() Erro alocando memória para hash!");
		return FALSE;
	}

	memset(pbHash, 0, dwHashLen);

	// Read the hash value.
	if(!CryptGetHashParam(hHash, HP_HASHVAL, pbHash, &dwHashLen, 0)) {
		STLOG_WRITE("CUtil::MakeHash() Erro %x durante leitura do valor do hash!", GetLastError());
		return FALSE;
	}	
	
	for(int i=0; i<dwHashLen; i++)
	{				
		CString temp;
		temp.Format(L"%.02x", pbHash[i]);		
		sHash->Append(temp);
	}	

	// Destroy the hash object
	if(hHash) CryptDestroyHash(hHash);
	
	// Free the CSP handle
	if(hProv) CryptReleaseContext(hProv,0);

	return TRUE;
}


void CUtil::WriteHashFile(const char * filename, unsigned char * data, long length)
{
	FILE * fp = fopen(filename, "w+b");
	if (fp) {
		for(int i=0; i<length; i++)
		{
			fprintf(fp, "%.02x", data[i]);
		}		
		fclose(fp);
	}
}


#ifdef _SIMPLE_REQUEST
//#define TRACING
//#define TRACING
/**
\brief Envia arquivo (como anexo) via post multipart
\details Utiliza classe CHttp
\param LPCTSTR szHttpVarFileName Nome da variável post referente ao arquivo
\param LPCTSTR szPathFile Arquivo a ser enviado
\param LPCTSTR szUrl URL para onde o arquivo será enviado
\param CMapStringToString *strMapVarValue Lista de variáveis a ser enviadas via post junto ao arquivo
\return BOOL TRUE: Envio OK / FALSE: Falha no envio
*/
BOOL CUtil::HttpSendFile(LPCTSTR szHttpVarFileName, LPCTSTR szPathFile, LPCTSTR szUrl, CMapStringToString *strMapVarValue, int &iServerRespCod, CString *sServerResp, CProxyInfo* proxy, BOOL bWithErrorCode)
{

	/*CSendFile a;
	BOOL b = a.Send(szHttpVarFileName, szPathFile, szUrl, strMapVarValue, iServerRespCod, sServerResp, bWithErrorCode);
	STLOG_WRITE("%s(%d) Enviou...", __FUNCTION__, __LINE__);
	return b;*/
	#ifdef TRACING
	STLOG_WRITE(L"%S(%d): Arquivo: %s URL: %s", __FUNCTION__, __LINE__, szPathFile, szUrl);
	STLOG_WRITE("CUtil::HttpSendFile -> antes -> Memoria Disponivel: [%d]",  CUtil::GetAvailableMemory());
	#endif
	//Se arquivo a ser enviado não existir...
	if(!CUtil::FileExists(szPathFile))
	{
		#ifdef TRACING
		STLOG_WRITE(L"CUtil::HttpSendFile Arquivo não existe [%s]", szPathFile);
		#endif
		iServerRespCod = 300;
		*sServerResp = L"Arquivo não existe";

		return FALSE;
	}	

	//STLOG_WRITE("%s(%d): A. Memoria Disponivel: [%d]", __FUNCTION__, __LINE__, CUtil::GetAvailableMemory());
	
	//CHttp conn;
	//STLOG_WRITE(L"**************** Tentando LOCK de: %s", szPathFile);
	//m_SendLock.Lock();
	//InitializeCriticalSection(m_SendLock);
	//STLOG_WRITE(L"**************** LOCK enviando: %s", szPathFile);


	/////CHttp::GetInstance().ResetArguments();
	/////CHttp::GetInstance().AddArguments(szHttpVarFileName, szPathFile, CHttp::ContentType::Binary);

	CSimpleRequest* sr = new CSimpleRequest();
	sr->ResetArguments();

	if(proxy)
		sr->SetProxy(proxy->sServer, proxy->nPort, proxy->sUser, proxy->sPass);

	CString tipoArq(szHttpVarFileName);
	if (tipoArq.CompareNoCase(L"XML") == 0)
	{
		sr->AddArguments(szHttpVarFileName, szPathFile, TRUE);
	}
	else
	{
		sr->Setm_bFile(TRUE);
	}

	POSITION p = strMapVarValue->GetStartPosition();	
	while(p)
	{		
		CString sKey, sValue;
		
		strMapVarValue->GetNextAssoc(p, sKey, sValue);
		//CHttp::GetInstance().AddArguments(sKey.Trim(), sValue.Trim());
		sr->AddArguments(sKey.Trim(), sValue.Trim());
	}
	if (tipoArq.CompareNoCase(L"XML") != 0)
	{
		sr->AddArguments(szHttpVarFileName, szPathFile, TRUE);
	}

#ifdef _WIN32_WCE
	//STLOG_WRITE("%s(%d): B. Memoria Disponivel: [%d]", __FUNCTION__, __LINE__, CUtil::GetAvailableMemory());
	//STLOG_WRITE(L"CUtil::HttpSendFile Enviando %s para %s", szPathFile, szUrl);
	//STLOG_WRITE(L"CUtil::HttpSendFile Qualidade do sinal de celular: %s Conexão: [%s]", CUtil::GetQualidadeSinal(), CUtil::GetTipoConexao());
#endif

	//EnterCriticalSection(m_SendLock);
	//if(CHttp::GetInstance().Request(szUrl, CHttp::RequestPostMethod, szPathFile))
	if(sr->Request(szUrl))
	{
		//*sServerResp = CHttp::GetInstance().Response();	
		*sServerResp = sr->Response();
		#ifdef TRACING
		STLOG_WRITE(L"CUtil::HttpSendFile Resposta de envio do arquivo %s: %s", szPathFile, *sServerResp);
		#endif
	}

	delete sr;
	sr = 0;
	//LeaveCriticalSection(m_SendLock);
	
	//Se resposta do servidor for vazia...
	if(sServerResp->IsEmpty())
	{
		iServerRespCod = 200; //Reenvio
		#ifdef TRACING
		STLOG_WRITE(L"CUtil::HttpSendFile URL retornou resposta vazia! Retornei 200 p/ reenvio[%s]", szUrl);
		STLOG_WRITE("CUtil::HttpSendFile -> depois -> Memoria Disponivel: [%d]",  CUtil::GetAvailableMemory());
		#endif
		return FALSE;
	}	
	
	//DeleteCriticalSection(m_SendLock);
	//m_SendLock.Unlock();
	//STLOG_WRITE(L"**************** UNLOCK enviando: %s", szPathFile);

	BOOL bRetValidate = CUtil::ValidatePost(sServerResp, TRUE, &iServerRespCod);
	
	sServerResp->Replace(L"\"",L"");
	sServerResp->Replace(L"'",L"");

	//STLOG_WRITE("%s(%d): C. Memoria Disponivel: [%d]", __FUNCTION__, __LINE__, CUtil::GetAvailableMemory());
	#ifdef TRACING
	STLOG_WRITE("CUtil::HttpSendFile -> depois -> Memoria Disponivel: [%d]",  CUtil::GetAvailableMemory());
	#endif
	return bRetValidate;	
	//return FALSE;
	//return CUtil::ValidatePost(sServerResp, TRUE, &iServerRespCod);
}

#else //(_SIMPLE_REQUEST)

/**
\brief Envia arquivo (como anexo) via post multipart
\details Utiliza classe CHttp
\param LPCTSTR szHttpVarFileName Nome da variável post referente ao arquivo
\param LPCTSTR szPathFile Arquivo a ser enviado
\param LPCTSTR szUrl URL para onde o arquivo será enviado
\param CMapStringToString *strMapVarValue Lista de variáveis a ser enviadas via post junto ao arquivo
\return BOOL TRUE: Envio OK / FALSE: Falha no envio
*/
BOOL CUtil::HttpSendFile(LPCTSTR szHttpVarFileName, LPCTSTR szPathFile, LPCTSTR szUrl, CMapStringToString *strMapVarValue, int &iServerRespCod, CString *sServerResp, BOOL bWithErrorCode)
{

	/*CSendFile a;
	BOOL b = a.Send(szHttpVarFileName, szPathFile, szUrl, strMapVarValue, iServerRespCod, sServerResp, bWithErrorCode);
	STLOG_WRITE("%s(%d) Enviou...", __FUNCTION__, __LINE__);
	return b;*/

	//Se arquivo a ser enviado não existir...
	if(!CUtil::FileExists(szPathFile))
	{
		STLOG_WRITE(L"CUtil::HttpSendFile Arquivo não existe [%s]", szPathFile);										
		return FALSE;
	}	

	//STLOG_WRITE("%s(%d): A. Memoria Disponivel: [%d]", __FUNCTION__, __LINE__, CUtil::GetAvailableMemory());
	
	//CHttp conn;
	//STLOG_WRITE(L"**************** Tentando LOCK de: %s", szPathFile);
	//m_SendLock.Lock();
	//InitializeCriticalSection(m_SendLock);
	//STLOG_WRITE(L"**************** LOCK enviando: %s", szPathFile);

	CHttp::GetInstance().ResetArguments();
	CHttp::GetInstance().AddArguments(szHttpVarFileName, szPathFile, CHttp::ContentType::Binary);

	//CSimpleRequest* sr = new CSimpleRequest();
	//sr->ResetArguments();
	//sr->AddArguments(szHttpVarFileName, szPathFile, TRUE);

	POSITION p = strMapVarValue->GetStartPosition();	
	while(p)
	{		
		CString sKey, sValue;
		
		strMapVarValue->GetNextAssoc(p, sKey, sValue);
		CHttp::GetInstance().AddArguments(sKey.Trim(), sValue.Trim());
		//sr->AddArguments(sKey.Trim(), sValue.Trim());
	}
#ifdef _WIN32_WCE
	//STLOG_WRITE("%s(%d): B. Memoria Disponivel: [%d]", __FUNCTION__, __LINE__, CUtil::GetAvailableMemory());
	//STLOG_WRITE(L"CUtil::HttpSendFile Enviando %s para %s", szPathFile, szUrl);
	//STLOG_WRITE(L"CUtil::HttpSendFile Qualidade do sinal de celular: %s Conexão: [%s]", CUtil::GetQualidadeSinal(), CUtil::GetTipoConexao());
#endif

	//EnterCriticalSection(m_SendLock);
	if(CHttp::GetInstance().Request(szUrl, CHttp::RequestPostMethod, szPathFile))
	//if(sr->Request(szUrl))
	{
		*sServerResp = CHttp::GetInstance().Response();	
		//*sServerResp = sr->Response();
		STLOG_WRITE(L"CUtil::HttpSendFile Resposta de envio do arquivo %s: %s", szPathFile, *sServerResp);
	}

	//delete sr;
	//sr = 0;
	//LeaveCriticalSection(m_SendLock);
	
	//Se resposta do servidor for vazia...
	if(sServerResp->IsEmpty())
	{
		iServerRespCod = 200; //Reenvio
		STLOG_WRITE(L"CUtil::HttpSendFile URL retornou resposta vazia! Retornei 200 p/ reenvio[%s]", szUrl);
		return FALSE;
	}	
	
	//DeleteCriticalSection(m_SendLock);
	//m_SendLock.Unlock();
	//STLOG_WRITE(L"**************** UNLOCK enviando: %s", szPathFile);

	BOOL bRetValidate = CUtil::ValidatePost(sServerResp, TRUE, &iServerRespCod);
	
	sServerResp->Replace(L"\"",L"");
	sServerResp->Replace(L"'",L"");

	//STLOG_WRITE("%s(%d): C. Memoria Disponivel: [%d]", __FUNCTION__, __LINE__, CUtil::GetAvailableMemory());
			
	return bRetValidate;	
	//return FALSE;
	//return CUtil::ValidatePost(sServerResp, TRUE, &iServerRespCod);
}


#endif //_SIMPLE_REQUEST

/**
\brief Habilita a taskbar do Windows Mobile (a do Menu Iniciar)
\param BOOL bEnable Flag que indica se é para habilitar ou não a TASKBAR
\return void
*/
#ifdef _WIN32_WCE
void CUtil::EnableTaskBar(BOOL bEnable)
{
	CWnd *pWnd = pWnd->FindWindow(_T("HHTaskBar"),_T(""));
	bEnable ? pWnd->ShowWindow(SW_SHOW) : pWnd->ShowWindow(SW_HIDE);
	
	if(pWnd != NULL)
	{
		pWnd->EnableWindow(bEnable);
		pWnd->Invalidate();
		pWnd->UpdateWindow();
	}
}

/**
\brief Indica a qualidade do sinal do celular em porcentagem
\return CString A qualidade do sinal do celular em porcentagem
*/
CString CUtil::GetQualidadeSinal()
{
	CString sRet;
#if _WIN32_WCE > 0x420
	DWORD dwSignal = 0;
	Registry reg(SN_CELLSYSTEMCONNECTED_ROOT, SN_CELLSYSTEMCONNECTED_PATH);
	reg.Open();
	reg.GetValue(L"Signal Strength Raw", &dwSignal);
	sRet.Format(L"%d", dwSignal);
	reg.Close();
#endif

	return sRet;
}

/**
\brief Indica qual o tipo de conexão celular está ativo no momento
\return CString O tipo de conexão celular que está ativo no momento ou Desconhecido se não existe conexão via celular
*/
CString CUtil::GetTipoConexao()
{
	DWORD dwConn;
	CString sRet = L"Desconhecido";
#if _WIN32_WCE > 0x420
	Registry reg(SN_CELLSYSTEMCONNECTED_ROOT, SN_CELLSYSTEMCONNECTED_PATH);
	reg.Open();
	reg.GetValue(SN_CELLSYSTEMCONNECTED_VALUE, &dwConn);

	switch(dwConn)
	{
		case SN_CELLSYSTEMCONNECTED_GPRS_BITMASK:
			sRet = L"GPRS";
		break;
		case SN_CELLSYSTEMCONNECTED_1XRTT_BITMASK:
			sRet = L"1XRTT";
		break;
		case SN_CELLSYSTEMCONNECTED_1XEVDO_BITMASK:
			sRet = L"EVDO";
		break;
		case SN_CELLSYSTEMCONNECTED_EDGE_BITMASK:
			sRet = L"EDGE";
		break;
		case SN_CELLSYSTEMCONNECTED_UMTS_BITMASK:
			sRet = L"UMTS";
		break;
		case SN_CELLSYSTEMCONNECTED_EVDV_BITMASK:
			sRet = L"EVDV";
		break;
		case SN_CELLSYSTEMCONNECTED_HSDPA_BITMASK:
			sRet = L"HSDPA";
		break;
	}

	reg.Close();
#endif

	return sRet;
}

#endif

/**
\brief Retorna o timestamp atual
\return CString o timestamp atual
*/
CString CUtil::GetCurrentTimeStamp()
{
	CString sTimeStamp;

	__time64_t ts;
	_time64(&ts);

	sTimeStamp.Format(L"%ld", ts);

	return sTimeStamp;
}

/**
\brief Retorna o usuário logado no sistema atualmente
\return CString o usuário logado no sistema
*/
CString CUtil::GetLoggedUser()
{
	return m_sLoggedUser;
}

/**
\brief Retorna o usuário logado no sistema atualmente
\return CString o usuário logado no sistema
*/
CString CUtil::GetLoggedUserPerms()
{
	return m_sLoggedUserPerms;
}

void CUtil::SetLoggedUser(LPCTSTR szUser)
{
	m_sLoggedUser = szUser;

	Registry reg(HKEY_LOCAL_MACHINE, REG_KEY_APP_PARAMS);
	reg.Open();

	reg.SetValue(L"LoggedUser", m_sLoggedUser);

	reg.Close();
}

/**
\brief Inicia a calibragem da tela
\return void
*/
void CUtil::CalibrateScreen()
{
	typedef BOOL (WINAPI* PTouchCalibrate)(void);

	HMODULE hCoreDLL = LoadLibrary(_T("coredll.dll"));
	PTouchCalibrate pTouchCalibrate = (PTouchCalibrate)GetProcAddress(hCoreDLL, _T("TouchCalibrate"));

	if (pTouchCalibrate)
		pTouchCalibrate();
}

/**
\brief Faz o backup do banco de dados atual
\detail O backup vai tentar ser gravado no caminho BackupPath, caso dê algo errado o sistema tentará gravá-lo no Cartão de Memória, caso dê errado retorna FALSE
\param CString DBPath o Path do banco a ser feito o backup
\param CString BackupPath o Path do backup
\param BOOL tstFalha Flag para esta função retornar FALSE (Padrão FALSE, não influencia no resultado final)
\return BOOL TRUE se o backup for feito com sucesso
*/
BOOL CUtil::doBackup(CString DBPath, CString BackupPath, BOOL tstFalha )
{
#ifdef _WIN32_WCE

	DWORD dwError;
	BOOL  bCopy = TRUE;

	CMsgWindow* wnd;
	wnd = CMsgWindow::getInstance();
	wnd->Show(L"Atualizando Backup...");

	//bRunning = true;

	CString sSDBackupPath;
	sSDBackupPath.Format(L"%s\\%s", GetSDCardPath(), GetIDTalao());
	
	STLOG_WRITE("CUtil::doBackup() : Execucao do backup");

	// Recuperar o diretorio do banco de dados
	//CString sDBPath = theApp.GetInitInfo()->GetDBPath();
	CString sDBPath = DBPath;
	if(sDBPath.IsEmpty())
	{
		wnd->Destroy();
		//bRunning = false;
		STLOG_WRITE("CUtil::doBackup() : DBPath no PMA.XML esta invalido");
		return FALSE;
	}

	//CString sPath = theApp.GetInitInfo()->GetBackupPath();
	CString sPath = BackupPath;
	if(!sPath.IsEmpty())
	{
		CFileStatus fsDB, fsBackup;
		
		CString sPathOld = L"\\Temp\\Backup.db.tmp";
		BOOL bBackupOfBackup = FALSE;

		/*CFile::GetStatus(sDBPath, fsDB);
		CFile::GetStatus(sPath, fsBackup);*/

		//if(fsDB.m_mtime == fsBackup.m_mtime)
		//{
		//	wnd->Destroy();
		//	//bRunning = false;
		//	STLOG_WRITE("CUtil::doBackup() : Arquivo de backup idêntico ao original");
		//	if (tstFalha) {return FALSE;} //////////teste retirar
		//	return TRUE;
		//}		

		// Se existir um backup, vamos criar um backup dele...
		if(CUtil::FileExists(sPath))
		{
			if(FileExists(sPathOld))
				DeleteFile(sPathOld);

			if(!MoveFileW(sPath, sPathOld))
			{
				STLOG_WRITE("CUtil::doBackup() : Erro criando backup do backup. Motivo: %d", GetLastError());
				wnd->Destroy();
				return FALSE;
			}
			else
			{
				bBackupOfBackup = TRUE;

				ULARGE_INTEGER nFreeToCaller;
				ULARGE_INTEGER nTotal;
				ULARGE_INTEGER nFree;

				GetDiskFreeSpaceEx(BackupPath, &nFreeToCaller, &nTotal, &nFree);

				if(fsDB.m_size > nFree.LowPart)
					bCopy = FALSE;
			}
		}

		//Se puder copiar, copie
		if(bCopy)
			bCopy = CopyFile(sDBPath, sPath, FALSE);
		else
		{
			STLOG_WRITE("%s(%d): Backup maior que o espaço disponível no diretório de backup.", __FUNCTION__, __LINE__);
			dwError = 112;
		}

		//Se não conseguiu fazer a cópia
		if(!bCopy)
		{
			dwError = GetLastError();
			//Tenta copiar no SD Card
			if(!GetSDCardPath().IsEmpty())
			{
				CreateDirectory(sSDBackupPath, NULL);
				sSDBackupPath += "\\backup.db";

				if(CopyFile(sDBPath, sSDBackupPath, FALSE))
				{
					STLOG_WRITE("CUtil::doBackup() : backup ok... removendo o tmp");
					// Se tudo der certo, deletar o tmp...
					DeleteFile(sPathOld);
					wnd->Destroy();
				

					TCHAR buff[256];
					LoadString(m_hInst, IDS_BACKUP_LOCAL_PADRAO, buff, 256);	
					MessageBox(GetActiveWindow(), buff, L"Mensagem", MB_ICONERROR|MB_OK);
					return TRUE;
				}
			}

			if(dwError == 112)
			{
				wnd->Destroy();

				MEMORY_BASIC_INFORMATION mbi;
				static int dummy;
				VirtualQuery( &dummy, &mbi, sizeof(mbi) );

				TCHAR buff[256];
				LoadString(m_hInst, IDS_BACKUP_LOCAL_PADRAO, buff, 256);	
				MessageBox(GetActiveWindow(), buff, L"Mensagem", MB_ICONERROR|MB_OK);

				return FALSE;
			}			

			STLOG_WRITE("CUtil::doBackup() : Erro copiando arquivo backup %ld", dwError);
			DeleteFile(sPath); // Deleta o arquivo existente...

			if(bBackupOfBackup) // Se fizemos uma copia...
			{
				// Retorna o ultimo backup valido...
				if(!MoveFileW(sPathOld, sPath))
				{
					STLOG_WRITE("CUtil::doBackup() : Erro renomeando backup do backup");
					wnd->Destroy();
					return FALSE;
				}
			}
		}
		else
		{
			STLOG_WRITE("CUtil::doBackup() : backup ok... removendo o tmp");
			// Se tudo der certo, deletar o tmp...
			DeleteFile(sPathOld);
		}
	}
	else
	{
		STLOG_WRITE("CUtil::doBackup() : Erro recuperando o caminho de backup");
		wnd->Destroy();
		return FALSE;
	}

	wnd->Destroy();
	//bRunning = false;


#endif
	if (tstFalha) {return FALSE;}  //////////teste retirar

	return TRUE;
}

/**
\brief Retorna a data ou a hora atual em formato CString
\param const CString& sDateOrTime Indica o que retornar: "DATA" retorna a data. "HORA" retorna a hora.
\return CString a data ou a hora atual.
*/
CString CUtil::GetCurrentDateTime(const CString& sDateOrTime)
{
	SYSTEMTIME st;
	ZeroMemory(&st, sizeof(st));

	CString sRet = L"";

	GetLocalTime(&st);

	if(sDateOrTime.CompareNoCase(L"DATA")==0)
	{
		sRet.Format(L"%d-%02d-%02d", st.wYear, st.wMonth, st.wDay);
	}
	else if(sDateOrTime.CompareNoCase(L"DATA_COMUM")==0)
	{
		sRet.Format(L"%02d/%02d/%d", st.wDay, st.wMonth, st.wYear);
	}
	else if(sDateOrTime.CompareNoCase(L"HORA")==0)
	{
		sRet.Format(L"%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);
	}
	else if(sDateOrTime.CompareNoCase(L"DATA_HORA")==0)
	{
		sRet.Format(L"%02d/%02d/%d %02d:%02d:%02d", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);
	}

	return sRet;
}


//////////

/**
\brief Executa envio de arquivo XML.
\param LPVOID lpParameter 
\return UINT
*/
int CUtil::SendArqXml(LPVOID lpParameter, CString szPathFile, LPCTSTR szURLDestino, CString * sRespWeb, CProxyInfo* proxy)
{

	ConsultaJobs2Send  Job2Send;
	CString sContrato = CUtil::m_sContrato;
	CString sVarsList ;
	//sVarsList.AppendFormat(L"contrato=%s", sContrato);

	Job2Send.iId = 0;
	/** \brief Variável de controle da web para envio de arquivos*/ 
	Job2Send.sFileWebExt = L"";
	/** \brief Path do arquivo a ser enviado*/ 
	Job2Send.sFilePath = szPathFile;
	/** \brief Lista das variaveis post que serão transmitidas junto com o arquivo*/ 
	Job2Send.sVarList = sVarsList;
	/** \brief Ação a ser executada após a transmissão com sucesso do arquivo*/ 
	Job2Send.sOnTXOkAction = L"";
	/** \brief Tabela a ser manipulada após a transmissão com sucesso do arquivo*/
	Job2Send.sOnTXOkTable = L"";
	/** \brief Chave a ser manipulada após a transmissão com sucesso do arquivo*/
	Job2Send.sOnTXOkKeyName = L"";
	/** \brief Valor da chave a ser manipulada após a transmissão com sucesso do arquivo*/
	Job2Send.sOnTXOkKeyValue = L"";

	
	//Lista de jobs ainda não enviados pela thread HttpSender. Filtrado por nome do objeto (sObjName)
	__CONSULTAHTTPSENDERJOBSLIST HttpSenderJobList;


	///STLOG_WRITE(L"[%s] CUtil::SendArqXml: Enviando Registro Login_Logout", szPathFile);	
#ifdef _WIN32_WCE
	//Se off-line nao faz nada...
	if(!CUtil::IsOnline())
	{
		//STLOG_WRITE(L"[%s] CHttpSenderThread::HttpSenderThreadProc: Talao Off-Line", p_HttpSenderDados->sThreadName);			
		return -1; //off line
	}
#endif

		
	ConsultaJobs2Send *inc = & Job2Send;

	CString sVarFileName	 = L"XML";//CString(inc->sFileWebExt);
	CString sPathFile		 = CString(inc->sFilePath);
	CString sPostVarsList	 = CString(inc->sVarList);
	CString sOnTXOkAction	 = CString(inc->sOnTXOkAction);							

	//Separa lista de variavel=valor (sPostVarsList)
	int n = 3;
	CMapStringToString sList;				
	CUtil::Tokenize(sPostVarsList, sList, n); 				

	//Envia arquivo...
	CString sResp;
	int iRespCod = 0;
	int iRet = 0;
	//Lock do sender...
#ifndef _SIMPLE_REQUEST
	EnterCriticalSection(&m_uSendLock);			
#endif		
	//Se envio OK...
	if(CUtil::HttpSendFile(sVarFileName, sPathFile, szURLDestino, &sList, iRespCod, &sResp, proxy, TRUE))
	{					
		//Executa função apos TX=OK, geralmente atualiza status de transmissão do registro na tabela do obj de envio				
		//if(_ExecFuncOnTxOk(sOnTXOkAction, inc))
		//{					
		//		DeleteFile(sPathFile);										
		//}
		//else //Erro ao atualizar status de transmissao
		//{	
		//	//	CConsultas::GravaHttpSenderLog(m_pDB, p_HttpSenderDados->sThreadName, sPathFile, L"Erro na atualização de status de transmissão!", iRespCod);											
		//}
		DeleteFile(sPathFile);
		iRet = iRespCod;
	}
	//Erro no envio...
	else
	{					
		//STLOG_WRITE(L"[%s] CHttpSenderThread::HttpSenderThreadProc Falha no envio do arquivo [%s]", p_HttpSenderDados->sThreadName, sPathFile);					
		//STLOG_WRITE(L"[%s] CHttpSenderThread::HttpSenderThreadProc Resposta do servidor: [%d] %s", p_HttpSenderDados->sThreadName, iRespCod, sResp);					

		//Trata códigos de erro...
		switch(iRespCod)
		{
			/***** Erros que implicam reenvio *****/						
			case 200:
				//Volta o status do job para N
				//campoValorSetList.SetAt(L"status", L"'N'");
				//if(!CConsultas::Update(m_pDB, L"httpsender_job", L"id", sId, &campoValorSetList))
				//{
				//	STLOG_WRITE(L"[%s] CHttpSenderThread::CreateHttpSenderThread() Erro ao marcar job p/ reenvio", p_HttpSenderDados->sThreadName);
				//}

				////e incrementa núm de tentativa de envio com falha
				//iNumTentativaComErro++;
				//LeaveCriticalSection(&m_uSendLock);
				iRet = iRespCod;
			break;

			/***** Erros fatais *****/
			case 300:						
				//Marca o job como erro (status=E) e não envia mais
				//campoValorSetList.SetAt(L"status", L"'E'");
				//if(!CConsultas::Update(m_pDB, L"httpsender_job", L"id", sId, &campoValorSetList))
				//{
				//	STLOG_WRITE(L"[%s] CHttpSenderThread::CreateHttpSenderThread() Erro ao marcar job com erro fatal", p_HttpSenderDados->sThreadName);
				//}
				//LeaveCriticalSection(&m_uSendLock);
				iRet = iRespCod;
			break;

			/***** Erro inválido ou resposta vazia *****/
			default:
				//Qualquer outro valor de iRespCod, o job fica com status=T e só será marcado como novo (N) 
				//ao entrar no sistema novamente, na criação da thread (CreateHttpSenderThread)
				
				iRet = iRespCod;
				//LeaveCriticalSection(&m_uSendLock);
			break;
		}

		//Grava log de tentativa de envio
		if(sResp.IsEmpty() || sResp.GetLength() > 300)
			sResp = L"Erro ao enviar arquivo!";

		//	CConsultas::GravaHttpSenderLog(m_pDB, p_HttpSenderDados->sThreadName, sPathFile, sResp, iRespCod);									
							
		//break;
		//continue;
	}
#ifndef _SIMPLE_REQUEST
	LeaveCriticalSection(&m_uSendLock);
#endif
	*sRespWeb = sResp;
	return iRet;
}
			


/**
\brief verifica se o Agente está logado em outro equipamento
\detail a consulta será enviada para o servidor Web 
\param CString *sResp ponteiro para a string de resposta
\return BOOL TRUE se sim, FALSE se não ou não conseguiu verificar
*/
BOOL CUtil::VerificaAgenteLogado( CString *sResp, CProxyInfo* proxy)
{
	BOOL bResp = FALSE;

	if (OnLine())
	{
	
		CString sContrato = CUtil::m_sContrato;
		//CString m_sSerial = CUtil::GetIDTalao();
		//CString m_sCodigo = CUtil::GetLoggedUser();
		CString m_sData = CUtil::GetCurrentDateTime(L"DATA");
		CString m_sHora = CUtil::GetCurrentDateTime(L"HORA");

		//CString logado = GetLoggedUser();
		/*** Início construção do XML de envio de reg LOGIN_LOGOUT ***/
		CXmlCreate xml;	
		xml.OpenRootTag(L"reg_login_logout");
		xml.AddElement(L"id_talao", GetIDTalao());
		xml.AddElement(L"id_agente", GetLoggedUser());	
		xml.AddElement(L"contrato", sContrato);		
		xml.AddElement(L"operacao", L"CHECK");
		xml.AddElement(L"datahora", m_sData + L" " + m_sHora);
		xml.CloseRootTag(L"reg_login_logout");
		/*** Final da construção do XML ***/		

		//Valida estrutura do XML criado
		if(xml.ValidateXml())
		{	
			//Cria diretório XML_LOGIN do httpsender se não existir
			CString sBaseDir = CUtil::GetMainAppPath() + L"\\XML_LOGIN";	
			CUtil::CreateDirIfNotExist(sBaseDir);



			CString sTimeStamp = CUtil::GetCurrentTimeStamp();
			CString sPathXmlFile;
			sPathXmlFile.Format(L"%s\\LOGIN_LOGOUT_%s.xml",sBaseDir, sTimeStamp);
			xml.CreateXmlFile(sPathXmlFile);

			CString sVarsList ;
			sVarsList.AppendFormat(L"contrato=%s", sContrato);
		
			
			int iRetWeb = CUtil::SendArqXml(LPVOID(NULL) , sPathXmlFile, m_sUrlTxLoginLogout, sResp, proxy);
			if (iRetWeb == 100)
			{
				bResp = FALSE;
			}
			else
			{
				if (iRetWeb == 300) // agente logado em outro equipamento
				{
					bResp = TRUE;
				}
				else     // outros erros ou off line - indefinido
				{
					bResp = FALSE;
				}
			}
			DeleteFile(sPathXmlFile);
		}
	}
	else
	{
		bResp = FALSE; //off line
	}
	return bResp;
}


/**
\brief Coloca o PMA como janela corrente
\return void
*/
void CUtil::ShowPMA()
{
	HWND hWnd = ::FindWindow(_T("Dialog"), _T("PMA"));
	if(NULL != hWnd)
	{
		CloseExternalProcess();
		::SetForegroundWindow(hWnd);
	}
}


#ifdef _SIMPLE_REQUEST
/**
\brief Pesquisa endereço a partir da posição de GPS
\retorna lista de endereços em lstResp
\return BOOL
*/
BOOL CUtil::GetAddressByCoord(CString sContrato, CString sURL, CString sCodigoAgente, CString sLatitude, CString sLongitude, CStringList & lstResp)
{
	if(sURL.IsEmpty())
	{
		return FALSE;
	}
	if((sLatitude.IsEmpty())||sLongitude.IsEmpty())
	{
		return FALSE;
	}
	if((sLatitude.CompareNoCase(L"0.000000") == 0) && (sLongitude.CompareNoCase(L"0.000000") == 0))
	{
		return FALSE;
	}


	CMsgWindow* wnd;
	wnd = CMsgWindow::getInstance();

	wnd->Show(L"Verificando Conexão...");


	CString sIdTalao  = GetIDTalao();
	
	if(CUtil::OnLine())
	{
		if(sURL.IsEmpty())
		{
			return FALSE;
		}

		CSimpleRequest* sr = new CSimpleRequest();
		sr->ResetArguments();
		sr->AddArguments(L"contrato", sContrato);
		sr->AddArguments(L"cd_equipamento", sIdTalao);
		sr->AddArguments(L"cd_agente", sCodigoAgente);
		//sr->AddArguments(L"latitude", sLatitude);
		//sr->AddArguments(L"longitude", sLongitude);
		sr->AddArguments(L"lat", sLatitude);
		sr->AddArguments(L"long", sLongitude);

		wnd->Show(L"Pesquisando Endereço...");	

		CString sResposta;
		if(sr->Request(sURL))
		{
			///sResposta = CHttp::GetInstance().Response();
			sResposta = sr->Response();
			////LeaveCriticalSection(&CUtil::m_uSendLock);
			CString sTmp = sResposta;
			if(!CUtil::ValidatePost(&sResposta))
			{
				wnd->Destroy();
				return FALSE;
			}
			else
			{
				//if(m_LocalXmlParser.LoadXML(sTmp))
				//	return TRUE;
				TCHAR sSep = '#';
				int n = 3;
				sTmp.Replace(L"|",L"#");
				CStringArray sRespVarList;
				CUtil::Tokenize(sTmp, sRespVarList, n, &sSep);

				if(sRespVarList.GetSize() < 4)
				{
					sRespVarList.RemoveAll();
					STLOG_WRITE("CIncidentePage2::GetAddressByCoord() - Erro carregando xml da lista de enderecos [%S]",sResposta);				
					wnd->Destroy();
					return FALSE;
				}
				else
				{
					//resposta do server
					CString  sEstado = CString(sRespVarList.GetAt(2));
					CString  sCidade = CString(sRespVarList.GetAt(3));
					CString  sEndereco = L"";
					if (sRespVarList.GetSize() > 4)
					{
						sEndereco = CString(sRespVarList.GetAt(4));
					}
					CString sLocal = sEndereco + L" - " + sCidade +  L" - " + sEstado;

					lstResp.RemoveAll();
					lstResp.AddTail(sEndereco);
					lstResp.AddTail(sCidade);
					lstResp.AddTail(sEstado);
					sRespVarList.RemoveAll();

					if (sEndereco.IsEmpty())
					{
						wnd->Destroy();
						return FALSE;
					}
					wnd->Destroy();
					return TRUE;
				}
			}

		}
		else
		{
			////LeaveCriticalSection(&CUtil::m_uSendLock);
			STLOG_WRITE("CUtil::GetAddressByCoord() - Erro pesquisando Endereco");
			wnd->Destroy();
			return FALSE;
		}
		delete sr;
		sr = 0;
	}
	else
	{
		STLOG_WRITE(L"CUtil::GetAddressByCoord(): Talão não está on-line. Impossível pesquisar endereco");
	}

	wnd->Destroy();
	return FALSE;

}

#else   //(_SIMPLE_REQUEST)
/**
\brief Pesquisa endereço a partir da posição de GPS
\retorna lista de endereços em lstResp
\return BOOL
*/
BOOL CUtil::GetAddressByCoord(CString sContrato, CString sURL, CString sCodigoAgente, CString sLatitude, CString sLongitude, CStringList & lstResp)
{
	if(sURL.IsEmpty())
	{
		return FALSE;
	}
	if((sLatitude.IsEmpty())||sLongitude.IsEmpty())
	{
		return FALSE;
	}
	if((sLatitude.CompareNoCase(L"0.000000") == 0) && (sLongitude.CompareNoCase(L"0.000000") == 0))
	{
		return FALSE;
	}

	CMsgWindow* wnd;
	wnd = CMsgWindow::getInstance();

	wnd->Show(L"Verificando Conexão...");

	CString sIdTalao  = GetIDTalao();
	
	if(CUtil::OnLine())
	{
		if(sURL.IsEmpty())
		{
			return FALSE;
		}

		wnd->Show(L"Pesquisando Endereço...");	
		EnterCriticalSection(&CUtil::m_uSendLock);
		CHttp::GetInstance().ResetArguments();
		CHttp::GetInstance().AddArguments(L"contrato", sContrato);										
		CHttp::GetInstance().AddArguments(L"cd_equipamento", sIdTalao);	
		CHttp::GetInstance().AddArguments(L"cd_agente", sCodigoAgente);
		////CHttp::GetInstance().AddArguments(L"latitude", sLatitude);
		////CHttp::GetInstance().AddArguments(L"longitude", sLongitude);
		CHttp::GetInstance().AddArguments(L"lat", sLatitude);
		CHttp::GetInstance().AddArguments(L"long", sLongitude);

		CString sResposta;
		if(CHttp::GetInstance().Request( sURL ))
		{
			sResposta = CHttp::GetInstance().Response();	
			LeaveCriticalSection(&CUtil::m_uSendLock);
			CString sTmp = sResposta;
			if(!CUtil::ValidatePost(&sResposta))
			{
				wnd->Destroy();
				return FALSE;
			}
			else
			{
				//if(m_LocalXmlParser.LoadXML(sTmp))
				//	return TRUE;
				TCHAR sSep = '#';
				int n = 3;
				sTmp.Replace(L"|",L"#");
				CStringArray sRespVarList;
				CUtil::Tokenize(sTmp, sRespVarList, n, &sSep);

				if(sRespVarList.GetSize() < 4)
				{
					sRespVarList.RemoveAll();
					STLOG_WRITE("CIncidentePage2::GetAddressByCoord() - Erro carregando xml da lista de enderecos [%S]",sResposta);				
					wnd->Destroy();
					return FALSE;
				}
				else
				{
					//resposta do server
					CString  sEstado = CString(sRespVarList.GetAt(2));
					CString  sCidade = CString(sRespVarList.GetAt(3));
					CString  sEndereco = L"";
					if (sRespVarList.GetSize() > 4)
					{
						sEndereco = CString(sRespVarList.GetAt(4));
					}
					CString sLocal = sEndereco + L" - " + sCidade +  L" - " + sEstado;

					lstResp.RemoveAll();
					lstResp.AddTail(sEndereco);
					lstResp.AddTail(sCidade);
					lstResp.AddTail(sEstado);
					sRespVarList.RemoveAll();
					if (sEndereco.IsEmpty())
					wnd->Destroy();
					if (sEndereco.IsEmpty())
					{
						return FALSE;
					}
					return TRUE;
				}
			}

		}
		else
		{
			LeaveCriticalSection(&CUtil::m_uSendLock);
			STLOG_WRITE("CUtil::GetAddressByCoord() - CHttp::GetInstance().Request - Erro pesquisando Endereco");
			wnd->Destroy();
			return FALSE;
		}
	}
	else
	{
		STLOG_WRITE(L"CUtil::GetAddressByCoord(): Talão não está on-line. Impossível pesquisar endereco");
	}

	wnd->Destroy();
	return FALSE;

}

#endif //(_SIMPLE_REQUEST)

BOOL CUtil::InitCriticalSection()
{
	//InitializeCriticalSection(&CUtil::m_uSendLock);
	InitializeCriticalSection(&m_uSendLock);
	return TRUE;
}

void CUtil::SetThreadInfo(CString& sThread, int& nExecTime, DWORD dwThreadId, HANDLE hThread, CString& sTimeStamp, const CString sSendTime)
{
	ThrdInfo* thrdInfo=NULL;
	m_mapThrdInfo.Lookup(sThread, thrdInfo);

	if(thrdInfo)
	{
		hThread = thrdInfo->m_ThrdHandle;
		delete thrdInfo;
	}

	thrdInfo = new ThrdInfo();
	thrdInfo->m_curThrdTime = nExecTime;
	thrdInfo->m_ThrdHandle = hThread;
	thrdInfo->m_thrdId = dwThreadId;
	thrdInfo->m_TimeStamp = sTimeStamp;
	thrdInfo->m_sThrdName = sSendTime;

	m_mapThrdInfo.SetAt(sThread, thrdInfo);

}

void CUtil::GetThreadInfo(CArray<ThrdInfo*, ThrdInfo*> *retArr)
{
	CString ret=L"";
	ThrdInfo* thrdInfo;
	CString sName;
	int iCount = 0;
	int iCountErr = 0;
	DWORD dwExit;

	POSITION p = m_mapThrdInfo.GetStartPosition();
	
	while(p)
	{
		m_mapThrdInfo.GetNextAssoc(p, sName, thrdInfo);

		if(thrdInfo)
		{
			GetExitCodeThread(thrdInfo->m_ThrdHandle, &dwExit);
			CConsultasHttpSender::GetGPSJobsCount(sName, iCount, iCountErr);

			thrdInfo->m_sThrdName = sName;
			thrdInfo->m_nToSend = iCount;
			thrdInfo->m_nSendError = iCountErr;

			retArr->Add(thrdInfo);
		}
	}

}

/**
\brief Preenche Id do Talão para transmissão de arquivos de outros equipamentos
\param void
\return CString
*/
CString CUtil::SetIDTalao(LPCTSTR szID)
{
	if(CString(szID).IsEmpty())
		return m_sIdEqpto;

	m_sIdEqpto = szID;

	return m_sIdEqpto;
}

#define EQPTO
/**
\brief Adquire Id do Talão utilizando o método GetSerialNumberFromKernelIoControl()	
\param void
\return CString
*/
CString CUtil::GetIDTalao()
{
#ifdef _WIN32_WCE
#if _WIN32_WCE <= 0x420
	return GetSerialNumberFromKernelIoControl();
#else	
	return GetUniqueID();
#endif
#ifdef EQPTO
	return SetIDTalao(L"");
#endif
#else
	return _T("EB9EAE0290F724FB");
#endif
}	



/**
\brief completa com zeros a esquerda uma string
\param const CString string a ser completada
\param int tam Tamanho da string desejada
\return CString resultado da transformação
*/
CString CUtil::ZeroLeft(CString &sInput, int tam)
{	

	CString sTemp = _T("");

	int numZeros = tam - sInput.GetLength();

	for (int i = 0; i < numZeros; i++)
	{
		sTemp += _T("0");
	}
	sTemp += sInput;
	
	return sTemp;   
}



/**
\brief Converte string Bluetooth Address para o tipo BT_ADDR
\details Nao esqueca de zerar o conteudo do parametro de retorno pba
\param(in) const WCHAR **pp 
\param(out) BT_ADDR *pba
\return BOOL TRUE se OK FALSE se erro
*/

BOOL CUtil::ConvertStringToBT_ADDR(const WCHAR **pp, BT_ADDR *pba)
{
    int i;   
   
    while (**pp == ' ') 
	{
        ++*pp;
	}
   
    for (i = 0 ; i < 4 ; ++i, ++*pp) {   
        if (! iswxdigit (**pp))   
            return FALSE;   
   
        int c = **pp;   
        if (c >= 'a')   
            c = c - 'a' + 0xa;   
        else if (c >= 'A')   
            c = c - 'A' + 0xa;   
        else c = c - '0';   
   
        if ((c < 0) || (c > 16))   
            return FALSE;   
   
        *pba = *pba * 16 + c;   
    }   
   
    for (i = 0 ; i < 8 ; ++i, ++*pp) {   
        if (! iswxdigit (**pp))   
            return FALSE;   
   
        int c = **pp;   
        if (c >= 'a')   
            c = c - 'a' + 0xa;   
        else if (c >= 'A')   
            c = c - 'A' + 0xa;   
        else c = c - '0';   
   
        if ((c < 0) || (c > 16))   
            return FALSE;   
   
        *pba = *pba * 16 + c;   
    }   
   
    if ((**pp != ' ') && (**pp != '\0'))   
        return FALSE;   
   
    return TRUE;   

}

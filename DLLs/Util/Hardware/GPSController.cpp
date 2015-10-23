// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//

#include "stdafx.h"

#if defined _WIN32_WCE

#include "GPSController.h"
#include "XmlCreate.h"
#include "utils.h"
#include "FastTemplateCls.h"
#include "registry.h"
#include "ConsultasHttpSender.h"

#define MAX_WAIT    5000
#define MAX_AGE     3000
#define GPS_CONTROLLER_EVENT_COUNT 3

HANDLE CGPSController::s_hGPS_Device = NULL;
HANDLE CGPSController::s_hNewLocationData = NULL;
HANDLE CGPSController::s_hDeviceStateChange = NULL;
HANDLE CGPSController::s_hExitThread = NULL;
CEvent CGPSController::m_evStop;

//Informações específicas da thread
CString CGPSController::m_Contrato = L"";
CString CGPSController::m_CodAgente = L"";
CString CGPSController::m_IdTalao = CUtil::GetIDTalao();
CString CGPSController::m_sDateLastJob = L"START";
//SYSTEMTIME CGPSController::m_stDateLastJob = {0, 0, 0, 0, 0, 0, 0, 0};

//Habilita o rastreamento. Criação de jobs continuo do AVL
BOOL	CGPSController::m_bEnableAVL = TRUE;

//Por quantos minutos a criação do último job será válido 
//caso as condições de criação de um novo não sejam atendidas? 
int		CGPSController::m_iExpirationTimeWhenMoving = 30;

//Por quantos minutos a criação do último job será válido 
//caso as condições de criação de um novo não sejam atendidas? 
int		CGPSController::m_iExpirationTimeWhenStatic = 60*3;

//Qual distância entre o último ponto e o atual para ser considerado novo ponto válido
int		CGPSController::m_iBetweenPointsDistance = 100;

//Latitude e longitude atual
double CGPSController::m_dblCurrentLatitude;
double CGPSController::m_dblCurrentLongitude;

//Última latitude e longitude válida
double CGPSController::m_dblLastLatitude;
double CGPSController::m_dblLastLongitude;

// **************************************************************************
// Function Name: CGPSController:CGPSController
// 
// Purpose: Initialize the non static member variables of CGPSController
//
// Arguments:
//    None
//
// Return Values:
//    None
//
// Side effects:  
//    None
// 
// Description:  
//    See purpose
// **************************************************************************
CGPSController::CGPSController() : m_hThread(NULL), m_dwThreadID(0), m_bHasGPS(TRUE)
{
	m_pDIBSection = new CDIBSectionLite();	
}

CGPSController::~CGPSController()
{
	if(m_pDIBSection != NULL)
	{
		delete m_pDIBSection;
		m_pDIBSection = NULL;
	}
}

BEGIN_MESSAGE_MAP(CGPSController, CWnd)	
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_LBUTTONDOWN()
	ON_WM_TIMER()
END_MESSAGE_MAP()


void CGPSController::OnTimer(UINT_PTR nIDEvent)
{
	if(nIDEvent == 1)
	{			
		/*if(IsWindow(m_GPSWnd.GetSafeHwnd()))
		{	
			KillTimer(1);
			m_GPSWnd.Destroy();
			_setDadosMonitoramento();
		}
		else
		{
			KillTimer(1);
		}
		InvalidateRect(CRect(0,0,240,320), TRUE);*/

		//Alimenta dados p/ monitoramento do GPS
		_setDadosMonitoramento();		
	}
}	


BOOL CGPSController::SetGPSIconArea(CWnd *pParentWnd, const RECT &rect, UINT nID)
{		
	CRect r;
	if(SystemParametersInfo(SPI_GETWORKAREA, 0, &r, NULL))
	{
		if(r.Width() == 240)
		{
			szIconGreen = L"\\simbolos\\icon_gps_green.bmp";
			szIconRed = L"\\simbolos\\icon_gps_red.bmp";
			if(!m_bHasGPS)
				szIconGreen = szIconRed = L"\\simbolos\\icon_gps_gray.bmp";
		}
		else
		{
			szIconGreen = L"\\simbolos\\icon_gps_green_big.bmp";
			szIconRed = L"\\simbolos\\icon_gps_red_big.bmp";
			if(!m_bHasGPS)
				szIconGreen = szIconRed = L"\\simbolos\\icon_gps_gray_big.bmp";
		}
	}	

	return CWnd::Create(NULL, NULL, WS_CHILD, rect, pParentWnd, nID);
}


BOOL CGPSController::OnEraseBkgnd(CDC *pDC)
{
	CRect rect;
	GetClientRect(&rect);	
	pDC->PatBlt(rect.left, rect.top, rect.Width(), rect.Height(), PATCOPY);	

	return TRUE;
}

void CGPSController::OnPaint()
{
	if(m_sIconPath.IsEmpty())
		m_sIconPath = CUtil::GetMainAppPath() + szIconRed;
	
	HBITMAP hBmp = SHLoadImageFile(m_sIconPath);
	if(!m_pDIBSection->SetBitmap(hBmp))
	{
		delete m_pDIBSection;
		m_pDIBSection = NULL;	
	}	

	CPaintDC dc(this);	
	if(m_pDIBSection != NULL)
		m_pDIBSection->Draw(&dc, CPoint(0, 0));
}


// **************************************************************************
// Function Name: CGPSController:GPSThreadProc
// 
// Purpose: Provide an Asynchronous mechanism to query for GPS data
//
// Arguments:
//    __opt LPVOID lpParameter: This parameter is not used.  It is required to 
//                        enable GPSThreadProc to be started as a thread
//
// Return Values:
//    None: it always return 0.
//
// Side effects:  
//    The thread got to be instructed to unload before CGPSController object
//    is deleted.
// 
// Description:  
//    GPSThreadProc asynchronously receive notifcations events from the GPS
//    intermediate driver whenever the GPS hardware status changes or new
//    location information is available
// **************************************************************************
#if _WIN32_WCE != 0x420 && defined _WIN32_WCE
UINT CGPSController::GPSThreadProc(__opt LPVOID lpParameter)
{
	CGPSController *p_GPSController = reinterpret_cast<CGPSController*>(lpParameter);
    DWORD dwRet = 0;
    GPS_POSITION gps_Position = {0};
    GPS_DEVICE gps_Device = {0};
    HANDLE gpsHandles[GPS_CONTROLLER_EVENT_COUNT] = {s_hNewLocationData, 
        s_hDeviceStateChange,
        s_hExitThread
        };

    gps_Position.dwSize = sizeof(gps_Position);
    gps_Position.dwVersion = GPS_VERSION_1;

    gps_Device.dwVersion = GPS_VERSION_1;
    gps_Device.dwSize = sizeof(gps_Device);

    do
    {
        dwRet = WaitForMultipleObjects(GPS_CONTROLLER_EVENT_COUNT,
            gpsHandles,
            FALSE,
            INFINITE);
        if (dwRet == WAIT_OBJECT_0)
        {
            dwRet = GPSGetPosition(s_hGPS_Device,
                &gps_Position,
                MAX_AGE,
                0);
            if (ERROR_SUCCESS != dwRet)
            {
                continue;
            }
            else
            {                
				p_GPSController->SetGPSPosition(gps_Position);				
            }
        }
        else if (dwRet == WAIT_OBJECT_0 + 1)
        {
            dwRet = GPSGetDeviceState(&gps_Device);			
            if (ERROR_SUCCESS != dwRet)
            {
                continue;
            }
            else
            {             
				p_GPSController->SetGPSDeviceInfo(gps_Device);
            }			
        }
        else if (dwRet == WAIT_OBJECT_0 + 2)
        {
            break;
        }
        else
        {
			////STLOG_WRITE("Erro no retorno da espera dos objetos: [%d], retornado [%d]", GetLastError(), dwRet);
        }
    }while(1);//while(WaitForSingleObject(m_evStop, 1000) != WAIT_OBJECT_0);

    return 0;
}
#endif

// **************************************************************************
// Function Name: CGPSController:UnloadThread
// 
// Purpose: Instructs GPSThreadProc to unload and wait for the thread to exit
//
// Arguments:
//    None
//
// Return Values:
//    HRESULT: S_OK on success 
//             An appropriate error value on failure
//
// Side effects:  
//    It unloads GPSThreadProc
// 
// Description:  
//    GPSThreadProc asynchronously receive notifcations events from the GPS
//    intermediate driver whenever the GPS hardware status changes or new
//    location information is available
// **************************************************************************
HRESULT CGPSController::UnloadThread()
{
    HRESULT hr = E_FAIL;
#if _WIN32_WCE != 0x420 && defined _WIN32_WCE
    BOOL bRet = FALSE;
    DWORD dwRet = 0;

    // Instruct the thread to unload
    // There is not much one can do if set event failed
    // Ignore the failure and continue unloading
    bRet = SetEvent(s_hExitThread);
    if (!bRet)
    {
        dwRet = GetLastError();
        hr = HRESULT_FROM_WIN32(dwRet);
        goto Exit;
    }

    dwRet = WaitForSingleObject(m_hThread, MAX_WAIT);
    if(WAIT_OBJECT_0 != dwRet)
    {
        dwRet = GetLastError();
        hr = HRESULT_FROM_WIN32(dwRet);
        goto Exit;
    }

    hr = S_OK;

Exit:
    if (s_hNewLocationData)
    {
        CloseHandle(s_hNewLocationData);
        s_hNewLocationData = NULL;
    }

    if (s_hDeviceStateChange)
    {
        CloseHandle(s_hDeviceStateChange);
        s_hDeviceStateChange = NULL;
    }

    if (s_hExitThread)
    {
        CloseHandle(s_hExitThread);
        s_hExitThread = NULL;
    }

    if (m_hThread)
    {
        CloseHandle(m_hThread);
        m_hThread = NULL;
    }
#endif
    return hr;
}

// **************************************************************************
// Function Name: CGPSController:LoadThread
// 
// Purpose: Initialize the variables to be used by GPSThreadProc then 
//          create the thread
//
// Arguments:
//    None
//
// Return Values:
//    HRESULT: S_OK on success
//             An appropriate error value on failure
//              
//
// Side effects:  
//    It loads the GPSThreadProc
// 
// Description:  
//    LoadThread intializes 3 events before creating GPSThreadProc.
//    s_hNewLocationData    :   This event is set by the GPS Intermdiate 
//                              driver whenever new location information is
//                              available
//    s_hDeviceStateChange  :   This event is set by the GPS Intermediate
//                              driver whenever the GPS hardware state 
//                              changes
//    s_hExitThread         :   Used by CGPSController to instruct 
//                              GPSThreadProc to unload
// **************************************************************************
HRESULT CGPSController::LoadThread()
{
#if _WIN32_WCE != 0x420 && defined _WIN32_WCE
    HRESULT hr = E_FAIL;
    DWORD dwRet = 0;

    s_hNewLocationData = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!s_hNewLocationData)
    {
        dwRet = GetLastError();
        hr = HRESULT_FROM_WIN32(dwRet);
        goto Exit;
    }

    s_hDeviceStateChange = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!s_hDeviceStateChange)
    {
        dwRet = GetLastError();
        hr = HRESULT_FROM_WIN32(dwRet);
        goto Exit;
    }

    s_hExitThread = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!s_hExitThread)
    {
        dwRet = GetLastError();
        hr = HRESULT_FROM_WIN32(dwRet);
        goto Exit;
    }

    //m_hThread = CreateThread(NULL, NULL, GPSThreadProc, NULL, NULL, &m_dwThreadID);	
	m_hThread = AfxBeginThread(GPSThreadProc, this);
	
    if (!m_hThread)
    {
        dwRet = GetLastError();
        hr = HRESULT_FROM_WIN32(dwRet);
        goto Exit;
    }

	STLOG_WRITE("CGPSController::LoadThread() Thread do GPS iniciada!");
	STLOG_WRITE("CGPSController::LoadThread() m_iBetweenPointsDistance = %d | m_iExpirationTimeWhenMoving = %d | m_iExpirationTimeWhenStatic = %d", m_iBetweenPointsDistance, m_iExpirationTimeWhenMoving, m_iExpirationTimeWhenStatic);

    hr = S_OK;

Exit:
    if (FAILED(hr))
    {
		STLOG_WRITE("CGPSController::LoadThread() Erro na inicializaçõa da thread do GPS!");

        if (s_hNewLocationData)
        {
            CloseHandle(s_hNewLocationData);
            s_hNewLocationData = NULL;
        }

        if (s_hDeviceStateChange)
        {
            CloseHandle(s_hDeviceStateChange);
            s_hDeviceStateChange = NULL;
        }

        if (s_hExitThread)
        {
            CloseHandle(s_hExitThread);
            s_hExitThread = NULL;
        }

        // tmphThread is NULL & the thread was not created
    }
    return hr;
#else
	return 0;
#endif
}

// **************************************************************************
// Function Name: CGPSController:InitDevice
// 
// Purpose: Setup CGPSContoller internal state then initialize the GPS 
//          Intermediate driver
//
// Arguments:
//    __in IGPSSink *pGPSSink : Call back interface
//
// Return Values:
//    HRESULT: S_OK on success
//             An appropriate error value on failure
//              
//
// Side effects:  
//    If InitDevice succeeds, then UninitDevice is to be called before 
//    detroying the CGPSController object
// 
// Description:
//    See purpose
//    
// **************************************************************************
HRESULT CGPSController::InitDevice()
{
    HRESULT hr = E_FAIL;
#if _WIN32_WCE != 0x420 && defined _WIN32_WCE
    DWORD	dwRet = 0;
    
    if (s_hGPS_Device)
    {
        // InitDevice is to be called only once
        // This is best protected by implemeneting
        // an instantitor function and having the construtor set to become
        // private.
        hr = E_INVALIDARG;
        goto Exit;
    }

    hr = LoadThread();
    if (FAILED(hr))
    {
       goto Exit;
    }

    s_hGPS_Device = GPSOpenDevice(s_hNewLocationData,
        s_hDeviceStateChange,
        NULL,
        NULL);
    if (!s_hGPS_Device)
    {
        dwRet = GetLastError();
        hr = HRESULT_FROM_WIN32(dwRet);
        goto Exit;
    }

    hr = S_OK;

	SetTimer(1, 5000, NULL);

Exit:
    if (FAILED(hr))
    {
		STLOG_WRITE("CGPSController::InitDevice() Erro inicializando GPS device!");
        UninitDevice();
    }
#endif
    return hr;
}

// **************************************************************************
// Function Name: CGPSController:UninitDevice
// 
// Purpose: clear CGPSContoller internal state and uninitialize the GPS 
//          Intermediate driver handle
//
// Arguments:
//    None
//
// Return Values:
//    HRESULT: S_OK on success
//             E_INVALIDARG if the Intermediate Driver is not initialized
//             Otherwise, an appropriate error value on failure
//              
//
// Side effects:  
//    Unload the thread and release all resources owned by CGPSController
// 
// Description:
//    See purpose
//    
// **************************************************************************
HRESULT CGPSController::UninitDevice()
{
    HRESULT hr = E_FAIL;
#if _WIN32_WCE != 0x420 && defined _WIN32_WCE
    DWORD dwRet = 0;

    if (!s_hGPS_Device)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    // Attempt to unload the thread
    // There is nothing much that one can do if it fails
    hr = UnloadThread();

    // Attempt to unload the thread
    // there is nothing much that one can do if it fails
    dwRet = GPSCloseDevice(s_hGPS_Device);
    s_hGPS_Device = NULL;

    // return the origin of the failure if any
    if (FAILED(hr))
    {
       goto Exit;
    }
    else if (ERROR_SUCCESS != dwRet)
    {
        dwRet = GetLastError();
        hr = HRESULT_FROM_WIN32(dwRet);
        goto Exit;
    }

    hr = S_OK;
#endif
Exit:
    return hr;
}

#if _WIN32_WCE != 0x420 && defined _WIN32_WCE
HRESULT CGPSController::SetGPSDeviceInfo(GPS_DEVICE gps_Device)
{ 
	m_gps_Device = gps_Device;
	return S_OK;
}
#endif


double CGPSController::_calculaDistanciaMetros(double &dLat1, double &dLon1, double &dLat2, double &dLon2)
{
	//calculate radian distance, assume dLat1, dLon1, dLat2, dLon2 are in degrees
	double PI = 3.14159265359;
	double a = PI / 180;	

	//convert to radian
	//dLat1 = dLat1 * a; 
	//dLon1 = dLon1 * a;	
	
	//dLat2 = dLat2 * a;
	//dLon2 = dLon2 * a;
	
	double t1 = sin(dLat1 * a) * sin(dLat2 * a);
    double t2 = cos(dLat1 * a) * cos(dLat2 * a);
    double t3 = cos((dLon1 * a) - (dLon2 * a));
    double t4 = t2 * t3;
    double t5 = t1 + t4;
    double rad_dist = atan(-t5/sqrt(-t5 * t5 +1)) + 2 * atan(1);

	double mi = rad_dist * 3437.74677 * 1.1508;
	double km = mi * 1.6093470878864446;
	double meter = km * 1000;	

	return meter;
}

void CGPSController::_setGPSIcon(DWORD nSatelites)
{
	CString sCurrent = m_sIconPath;
	
	if(nSatelites > 2)
		m_sIconPath = CUtil::GetMainAppPath() + szIconGreen;	
	else
		m_sIconPath = CUtil::GetMainAppPath() + szIconRed;	
	
	if(sCurrent.CompareNoCase(m_sIconPath)!=0)
	{
		RedrawWindow();
	}
}

#if _WIN32_WCE != 0x420 && defined _WIN32_WCE
HRESULT CGPSController::SetGPSPosition(GPS_POSITION gps_Position)
{	
	//Atualiza status do ícone conforme número de satélites visíveis
	_setGPSIcon(gps_Position.dwSatelliteCount);
	m_gps_Position = gps_Position;

	int nErrorDateDiff;

	//Armazena lat,long atual
	_setCurrentLatLong(m_gps_Position.dblLatitude, m_gps_Position.dblLongitude);	

	//Calcula distancia entre a última coordenada enviada e a atual
	double dDistancia = _calculaDistanciaMetros(m_dblLastLatitude, m_dblLastLongitude, 
												m_dblCurrentLatitude, m_dblCurrentLongitude);
	//Data e hora atual...	
	GetLocalTime(&st);
	m_sDataHora.Format(L"%02d/%02d/%4d %02d:%02d:%02d", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);

	//Guarda velocidade em km/h...
	float fSpeed = gps_Position.flSpeed * 1.852;
	
	/********************************************************************/
	/*Cria jobs para envio de coordenadas GPS se:						*/
	/*																	*/
	/*  Condições Gerais:												*/
	/*																	*/
	/*	A - Num. Satelites visiveis maior que 2; 						*/
	/*																	*/
	/*	B - Latitude e Longitude válidas _IsLatLongValid();				*/
	/*																	*/
	/*  Condições Específicas:											*/
	/*																	*/
	/*	A - m_sDateLastJob = "START", ou seja, na primeira execução da  */
	/*		thread;														*/
	/*																	*/
	/*	B - Data de criação do último job (m_sDateLastJob) deve ser uma	*/
	/*	    data válida && m_sDateLastJob >	data/hora atual em			*/
	/*		m_iExpirationTimeWhenMoving segundos && distância entre		*/
	/*		pontos maior que m_iBetweenPointsDistance;					*/
	/*																	*/
	/*	C - Data de criação do último job (m_sDateLastJob) válida &&	*/
	/*		m_sDateLastJob expirada, ou seja, m_sDateLastJob > data/hora*/
	/*		atual em m_iExpirationTimeWhenStatic segundos.				*/ 
	/*																	*/			
	/********************************************************************/
	
	/*if((gps_Position.dwSatelliteCount > 2 && dDistancia > m_iBetweenPointsDistance) ||
	   (!m_sDateLastJob.IsEmpty() && m_sDateLastJob.CompareNoCase(L"00/00/0000 00:00:00") != 0 && !CUtil::IsIntervaloDataTimeValid(m_sDateLastJob, 'm', m_iExpirationTimeWhenStatic)) && 
	   (m_dblCurrentLatitude != 0.000000 && m_dblCurrentLongitude != 0.000000))*/		

	///STLOG_WRITE("_________________TESTE: Sats: [%d], PDOP: [%f], HDOP: [%f]", gps_Position.dwSatelliteCount, gps_Position.flPositionDilutionOfPrecision,
	///	gps_Position.flHorizontalDilutionOfPrecision);

	if(gps_Position.dwSatelliteCount > 2 && _IsLatLongValid(m_dblCurrentLatitude, m_dblCurrentLongitude))
	{ 
		if(m_sDateLastJob.CompareNoCase(L"START") == 0  || 
			((CUtil::CalculateSecondsNow(m_sDateLastJob) > m_iExpirationTimeWhenMoving) && dDistancia > m_iBetweenPointsDistance) ||
			CUtil::CalculateSecondsNow(m_sDateLastJob) > m_iExpirationTimeWhenStatic)
		  //(_IsDateLastJobValid(m_sDateLastJob) && !CUtil::IsIntervaloDataTimeValid(m_sDateLastJob, 's', m_iExpirationTimeWhenMoving, nErrorDateDiff) && dDistancia > m_iBetweenPointsDistance) ||
		  //(_IsDateLastJobValid(m_sDateLastJob) && !CUtil::IsIntervaloDataTimeValid(m_sDateLastJob, 's', m_iExpirationTimeWhenStatic, nErrorDateDiff)))
		{
					
			/*STLOG_WRITE("CGPSController::SetGPSPosition(): Nº Sat visíveis: %d | Distância entre A [%f %f] e B [%f %f] é de %f metros", gps_Position.dwSatelliteCount, 
																																		m_dblLastLatitude, 
																																		m_dblLastLongitude, 
																																		m_dblCurrentLatitude, 
																																		m_dblCurrentLongitude,
																																		dDistancia);*/

			//STLOG_WRITE("%s(%d): A. Memoria Disponivel: [%d]. Lat/Long [%f/%f]", __FUNCTION__, __LINE__, CUtil::GetAvailableMemory(), m_dblCurrentLatitude, m_dblCurrentLongitude);

			//STLOG_WRITE(L"CGPSController::SetGPSPosition(): Condição de envio verdadeira! LastJob_TIME: %s | NEW_TIME: %s | Distancia: %f metros | Vel: %f km/h", m_sDateLastJob, m_sDataHora, dDistancia, fSpeed);
			//STLOG_WRITE(L"CGPSController::SetGPSPosition(): Condição de envio verdadeira! LastJob_TIME: %s | ST: %d:%d:%d", m_sDateLastJob, m_stDateLastJob.wHour, m_stDateLastJob.wMinute, m_stDateLastJob.wSecond);

			//Guarda última lat/lon válida...
			m_dblLastLatitude = m_dblCurrentLatitude;
			m_dblLastLongitude = m_dblCurrentLongitude;
			
			//Se o rastreamento (AVL) estiver habilitado, cria job de gps...
			//Se o rastreamento (AVL) estiver desabilitado, cria job apenas com a expiração do tempo do Static...

			int secs = CUtil::CalculateSecondsNow(m_sDateLastJob); 
			BOOL b =  (secs >= m_iExpirationTimeWhenStatic);
			//BOOL c =  (secs >= m_iExpirationTimeWhenMoving);

			///STLOG_WRITE("%s(%d): AVL: [%d] StaticExpTime: [%d] IsExpired: [%d] Diff: [%d]", __FUNCTION__, __LINE__, m_bEnableAVL, m_iExpirationTimeWhenStatic, b, secs);
			//STLOG_WRITE("%s(%d): AVL: [%d] MovingExpTime: [%d] IsExpired: [%d] Diff: [%d] Dist: [%d]", __FUNCTION__, __LINE__, m_bEnableAVL, m_iExpirationTimeWhenMoving, c, secs, dDistancia);

			if(m_bEnableAVL || b)
			{
				/*** Início construção do XML de envio do GPS ***/
				CXmlCreate xml;	
				xml.OpenRootTag(L"gps");
					//xml.AddElement(L"cd_equipamento", m_IdTalao);		
					xml.AddElement(L"id_talao", m_IdTalao);
					xml.AddElement(L"id_agente", CUtil::GetLoggedUser());	
					xml.AddElement(L"contrato", m_Contrato); /*L"contrato");*/ 		//-------> TROCAR
					xml.AddElement(L"sat", (int)gps_Position.dwSatelliteCount);
					xml.AddElement(L"latitude", m_dblCurrentLatitude);
					xml.AddElement(L"longitude", m_dblCurrentLongitude);
					xml.AddElement(L"velocidade", fSpeed);		
					xml.AddElement(L"datahora", m_sDataHora);
				xml.CloseRootTag(L"gps");
				/*** Final da construção do XML ***/		

				//Valida estrutura do XML criado
				if(xml.ValidateXml())
				{	
					//Cria diretório GPS do httpsender se não existir
					CString sBaseDir = CUtil::GetMainAppPath() + L"\\GPS";	
					CUtil::CreateDirIfNotExist(sBaseDir);

					CString sTimeStamp = CUtil::GetCurrentTimeStamp();

					//Cria arquivo XML físico
					CString sPathXmlFile;
					sPathXmlFile.Format(L"%s\\GPS_%s.xml",sBaseDir, sTimeStamp);
					xml.CreateXmlFile(sPathXmlFile);

					//Vars enviadas por http-post
					CString sVarsList;
					sVarsList.AppendFormat(L"contrato=%s", m_Contrato);

					//Cria registro do job de envio	do XML
					//O szObjName deve ser o mesmo do nome descrito no atributo name da tag httpsenderthread do pma.xml
					if(CConsultasHttpSender::GravaHttpSenderJob(CppSQLite3DB::getInstance(), L"XML_GPS", sPathXmlFile, L"XML", sVarsList, L"", L"", L"", L""))
					{
						//STLOG_WRITE(L"CAitSend::Send() Inserido registro job de envio de GPS");
						//m_stDateLastJob = st;
						m_sDateLastJob = m_sDataHora;
					}
					else
					{
						//STLOG_WRITE(L"CAitSend::Send() Falha ao inserir registro Job de envio de GPS");
					}								
				}
			}	

			
		}
		else
		{
			/*** Atenção ao deixar essas linhas descomentadas. Ocorre perda de performance no sistema devido ao tamanho do arquivo de log ***/
			/*STLOG_WRITE(L"CGPSController::SetGPSPosition(): Nenhuma condição de envio verdadeira! LastJob_TIME: %s | NEW_TIME: %s | Distancia: %f metros | Vel: %f km/h", m_sDateLastJob, m_sDataHora, dDistancia, fSpeed);
			STLOG_WRITE("CGPSController::SetGPSPosition() Distância entre ponto A [%f %f] e B [%f %f] é %f metros", m_dblLastLatitude, 
																											    m_dblLastLongitude, 
																												m_dblCurrentLatitude, 
																												m_dblCurrentLongitude,
																												dDistancia);*/
		}
	}
	else
	{
		/*** Atenção ao deixar essas linhas descomentadas. Ocorre perda de performance no sistema devido ao tamanho do arquivo de log ***/
		//STLOG_WRITE("CGPSController::SetGPSPosition() Número de satélites visíveis: %d", gps_Position.dwSatelliteCount);		
	}

	//STLOG_WRITE("%s(%d): 2. Memoria Disponivel: [%d]", __FUNCTION__, __LINE__, CUtil::GetAvailableMemory());
	return S_OK;
}
#endif

void CGPSController::OnLButtonDown(UINT nFlags, CPoint point)
{	
	CRect r;
	VERIFY(SystemParametersInfo(SPI_GETWORKAREA, 0, &r, NULL) == 1);	

	m_GPSWnd.HasGPS(m_bHasGPS);
	m_GPSWnd.Create(this);

	CWnd::OnLButtonDown(nFlags, point);		
}

void CGPSController::_setDadosMonitoramento()
{		
	CString sValue;
	SYSTEMTIME	st;	
	GetLocalTime(&st);
	COleDateTime now(st);

	CTime tNow(now.GetYear(),
			   now.GetMonth(),
			   now.GetDay(),
			   now.GetHour(),
			   now.GetMinute(),
			   now.GetSecond());


	/************** Dados do GPS **************/
#if _WIN32_WCE != 0x420 && defined _WIN32_WCE
	int nSats = (int)m_gps_Position.dwSatelliteCount;
	nSats = nSats > 12 ? 0 : nSats;
	
	m_sGPStruct.sSatelliteCount = nSats;
	sValue = L"";			

	if(!m_sDateLastJob.IsEmpty() && m_sDateLastJob.CompareNoCase(L"START") != 0)
	{
		COleDateTime m_dtLastCoord;
		m_dtLastCoord.ParseDateTime(m_sDateLastJob);		
		
		CTime tGPS(m_dtLastCoord.GetYear(),
				   m_dtLastCoord.GetMonth(),
				   m_dtLastCoord.GetDay(),
				   m_dtLastCoord.GetHour(),
				   m_dtLastCoord.GetMinute(),
				   m_dtLastCoord.GetSecond());
        
		// Calculate time span.
        CTimeSpan tLastCoord = tNow - tGPS;
        LONG lNumDays  = tLastCoord.GetDays();
		LONG lNumHoras = tLastCoord.GetHours();
		LONG lNumMin   = tLastCoord.GetMinutes();
		LONG lNumSec   = tLastCoord.GetSeconds();
		
		if(lNumDays != 0)
		{
			if(lNumDays > 1)
				sValue.Format(L"%ld dias", lNumDays);
			else
				sValue.Format(L"%ld dia", lNumDays);			
		}	
		else if(lNumHoras != 0)
			sValue.Format(L"%ld h", lNumHoras);
		else if(lNumMin != 0)
			sValue.Format(L"%ld min", lNumMin);
		else if(lNumSec != 0)
			sValue.Format(L"%ld s", lNumSec);
		else if(sValue.IsEmpty())
			sValue = L"0 s";
		
		m_sGPStruct.sDateLastJob = sValue;		
	}
	else
	{
		m_sGPStruct.sDateLastJob = L"--";
		
	}

	sValue = L"";
#endif

	/************** Dados do Celular **************/

	//Recupera data/hora do último TX no registro do windows
	CString sDateLastTX;
	TCHAR szBuffer[512];
	DWORD dwLen = sizeof(szBuffer) * sizeof(TCHAR);

	Registry reg(HKEY_LOCAL_MACHINE, L"SOFTWARE\\GPS\\LastSend");
	reg.GetValue(L"SOFTWARE\\GPS\\LastSend", szBuffer, &dwLen);		
	reg.Close();

	sDateLastTX = szBuffer;

	if(sDateLastTX.GetLength() == 19)
	{
		COleDateTime m_dtLastTX;
		m_dtLastTX.ParseDateTime(sDateLastTX);		
		
		CTime tCelTX(m_dtLastTX.GetYear(),
				 m_dtLastTX.GetMonth(),
				 m_dtLastTX.GetDay(),
				 m_dtLastTX.GetHour(),
				 m_dtLastTX.GetMinute(),
				 m_dtLastTX.GetSecond());
        
		// Calculate time span.
        CTimeSpan tLastTX = tNow - tCelTX;
        LONG lNumDays  = tLastTX.GetDays();
		LONG lNumHoras = tLastTX.GetHours();
		LONG lNumMin   = tLastTX.GetMinutes();
		LONG lNumSec   = tLastTX.GetSeconds();
		
		if(lNumDays != 0)
		{
			if(lNumDays > 1)
				sValue.Format(L"%ld dias", lNumDays);
			else
				sValue.Format(L"%ld dia", lNumDays);
		}
		else if(lNumHoras != 0)
			sValue.Format(L"%ld h", lNumHoras);
		else if(lNumMin != 0)
			sValue.Format(L"%ld min", lNumMin);
		else if(lNumSec != 0)
			sValue.Format(L"%ld s", lNumSec);
		else if(sValue.IsEmpty())
			sValue = L"0 s";
		
		m_sGPStruct.sDateLastTX = sValue;
	}
	else
	{		
		m_sGPStruct.sDateLastTX = L"--";	
	}

	int iJobCount = 0;
	int iJobError = 0;
	sValue = L"";

	if(!CConsultasHttpSender::GetGPSJobsCount(L"XML_GPS", iJobCount, iJobError))
	{
		STLOG_WRITE("CGPSController::_setDadosMonitoramento() Erro recuperando núm. jobs GPS pendentes!");
	}

	sValue.Format(L"%d", iJobCount);
	m_sGPStruct.sNumJobsPendentes = sValue;	
	m_GPSWnd.UpdateStructGPSData(m_sGPStruct);
}


void CGPSController::_setCurrentLatLong(double &dbLat, double &dbLon)
{	
	m_dblCurrentLatitude  = dbLat;
	m_dblCurrentLongitude = dbLon;
	//STLOG_WRITE("%s(%d): Coordenada: [%f] [%f]", __FUNCTION__, __LINE__, dbLat, dbLon);
}

BOOL CGPSController::getCurrentLatLong(double &dbLat, double &dbLon)
{
	dbLat = m_dblCurrentLatitude;
	dbLon = m_dblCurrentLongitude;
	//STLOG_WRITE("%s(%d): Coordenada: [%f] [%f]", __FUNCTION__, __LINE__, dbLat, dbLon);

	if(_IsLatLongValid(dbLat, dbLon))
	{
		return TRUE;
	}

	return FALSE;
}

BOOL CGPSController::getLastLatLongValid(double &dbLat, double &dbLon)
{	
	if(_IsDateLastJobValid(m_sDateLastJob) && m_sDateLastJob.CompareNoCase(L"START") != 0)
	{
		dbLat = m_dblLastLatitude;
		dbLon = m_dblLastLongitude;

		return TRUE;
	}

	return FALSE;
}

BOOL CGPSController::_IsLatLongValid(double &dbLat, double &dbLon)
{	
	if((dbLat != 0.000000 && dbLon != 0.000000) && (dbLat != 0.0 && dbLon != 0.0))
		return TRUE;

	return FALSE;
}

BOOL CGPSController::_IsDateLastJobValid(CString &sDate)
{	
	if(!sDate.IsEmpty() && sDate.CompareNoCase(L"00/00/0000 00:00:00") != 0)
		return TRUE;

	return FALSE;
}

void CGPSController::VerificaTelaGPS(void)
{
	if(!m_GPSWnd)
		return;

	if (m_GPSWnd.IsWindowVisible())
	{
		m_GPSWnd.Destroy();
	}
}

#endif
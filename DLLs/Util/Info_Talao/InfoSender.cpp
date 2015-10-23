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


#include "InfoSender.h"
#include "XmlCreate.h"
#include "utils.h"
#include "FastTemplateCls.h"
#include "registry.h"
#include "ConsultasHttpSender.h"
#include "GPSController.h"
#include "PowerMonCtrl.h"

#define MAX_WAIT    5000
#define MAX_AGE     3000
#define INFO_SENDER_EVENT_COUNT 3

///extern CInfoDataWnd m_InfoWnd;
HANDLE CInfoSender::s_hGPS_Device = NULL;
HANDLE CInfoSender::s_hNewLocationData = NULL;
HANDLE CInfoSender::s_hDeviceStateChange = NULL;
HANDLE CInfoSender::s_hExitThread = NULL;
CEvent CInfoSender::m_evStop;
int CInfoSender::m_iTotMin = 0;

//Informações específicas da thread
CString CInfoSender::m_Contrato = L"";
CString CInfoSender::m_CodAgente = L"";
CString CInfoSender::m_VersaoSist = L"";
CString CInfoSender::m_IdTalao = CUtil::GetIDTalao();
///CString CInfoSender::m_sDateLastJob = L"START";

//Latitude e longitude atual
double CInfoSender::m_dblCurrentLatitude;
double CInfoSender::m_dblCurrentLongitude;

//Última latitude e longitude válida
double CInfoSender::m_dblLastLatitude;
double CInfoSender::m_dblLastLongitude;

// **************************************************************************
// Function Name: CInfoSender:CInfoSender
// 
// Purpose: Initialize the non static member variables of CInfoSender
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
CInfoSender::CInfoSender() : m_hThread(NULL), m_dwThreadID(0), m_bHasGPS(TRUE)
{
}

CInfoSender::~CInfoSender()
{
}

BEGIN_MESSAGE_MAP(CInfoSender, CWnd)	
	ON_WM_TIMER()
END_MESSAGE_MAP()


void CInfoSender::OnTimer(UINT_PTR nIDEvent)
{
	if(nIDEvent == 1)
	{
		BOOL bRet = FALSE;
		bRet = SetEvent(s_hDeviceStateChange);
	}
}	




// **************************************************************************
// Function Name: CInfoSender:SituacaoThreadProc
// 
// Purpose: Provide an Asynchronous mechanism to query for GPS data
//
// Arguments:
//    __opt LPVOID lpParameter: This parameter is not used.  It is required to 
//                        enable SituacaoThreadProc to be started as a thread
//
// Return Values:
//    None: it always return 0.
//
// Side effects:  
//    The thread got to be instructed to unload before CInfoSender object
//    is deleted.
// 
// Description:  
//    SituacaoThreadProc asynchronously receive notifcations events from the GPS
//    intermediate driver whenever the GPS hardware status changes or new
//    location information is available
// **************************************************************************
#if _WIN32_WCE != 0x420 && defined _WIN32_WCE
UINT CInfoSender::SituacaoThreadProc(__opt LPVOID lpParameter)
{
	CInfoSender *p_InfoSender = reinterpret_cast<CInfoSender*>(lpParameter);
    DWORD dwRet = 0;
    GPS_POSITION gps_Position = {0};
    GPS_DEVICE gps_Device = {0};
    HANDLE gpsHandles[INFO_SENDER_EVENT_COUNT] = {s_hNewLocationData, 
        s_hDeviceStateChange,
        s_hExitThread
        };


    do
    {
        dwRet = WaitForMultipleObjects(INFO_SENDER_EVENT_COUNT,
            gpsHandles,
            FALSE,
            INFINITE);
        if (dwRet == WAIT_OBJECT_0)
        {
			p_InfoSender->SetInfoData();				
        }
        else if (dwRet == WAIT_OBJECT_0 + 1)
        {
			p_InfoSender->SetInfoData();
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
// Function Name: CInfoSender:UnloadThread
// 
// Purpose: Instructs SituacaoThreadProc to unload and wait for the thread to exit
//
// Arguments:
//    None
//
// Return Values:
//    HRESULT: S_OK on success 
//             An appropriate error value on failure
//
// Side effects:  
//    It unloads SituacaoThreadProc
// 
// Description:  
//    SituacaoThreadProc asynchronously receive notifcations events from the GPS
//    intermediate driver whenever the GPS hardware status changes or new
//    location information is available
// **************************************************************************
HRESULT CInfoSender::UnloadThread()
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
// Function Name: CInfoSender:LoadThread
// 
// Purpose: Initialize the variables to be used by SituacaoThreadProc then 
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
//    It loads the SituacaoThreadProc
// 
// Description:  
//    LoadThread intializes 3 events before creating SituacaoThreadProc.
//    s_hNewLocationData    :   This event is set by the GPS Intermdiate 
//                              driver whenever new location information is
//                              available
//    s_hDeviceStateChange  :   This event is set by the GPS Intermediate
//                              driver whenever the GPS hardware state 
//                              changes
//    s_hExitThread         :   Used by CInfoSender to instruct 
//                              SituacaoThreadProc to unload
// **************************************************************************
HRESULT CInfoSender::LoadThread()
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

    //m_hThread = CreateThread(NULL, NULL, SituacaoThreadProc, NULL, NULL, &m_dwThreadID);	
	m_hThread = AfxBeginThread(SituacaoThreadProc, this);
	
    if (!m_hThread)
    {
        dwRet = GetLastError();
        hr = HRESULT_FROM_WIN32(dwRet);
        goto Exit;
    }

	STLOG_WRITE("CInfoSender::LoadThread() Thread de Situação iniciada!");

    hr = S_OK;

Exit:
    if (FAILED(hr))
    {
		STLOG_WRITE("CInfoSender::LoadThread() Erro na inicialização da thread de Situação!");

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
// Function Name: CInfoSender:InitDevice
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
//    detroying the CInfoSender object
// 
// Description:
//    See purpose
//    
// **************************************************************************
HRESULT CInfoSender::InitDevice()
{
    HRESULT hr = E_FAIL;
#if _WIN32_WCE != 0x420 && defined _WIN32_WCE
    DWORD	dwRet = 0;
 
	Create(NULL);
    hr = LoadThread();
    if (FAILED(hr))
    {
       goto Exit;
    }


    hr = S_OK;

	int iTime = m_iTotMin;
	if (iTime < 1)
	{
		iTime = 10;
	}
	DWORD dwTimeut = iTime*60000;

	SetTimer(1, dwTimeut, NULL); /// 10 minutos
	///SetTimer(1, 60000, NULL);    ///teste 1minuto
	///OnTimer(1);

Exit:
    if (FAILED(hr))
    {
		STLOG_WRITE("CInfoSender::InitDevice() Erro(2) inicializando  Envio Info!!");
        UninitDevice();
    }
#endif


    return hr;
}

// **************************************************************************
// Function Name: CInfoSender:UninitDevice
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
//    Unload the thread and release all resources owned by CInfoSender
// 
// Description:
//    See purpose
//    
// **************************************************************************
HRESULT CInfoSender::UninitDevice()
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



HRESULT CInfoSender::SetInfoData()
{	

	//Data e hora atual...	
	GetLocalTime(&st);
	m_sDataHora.Format(L"%4d-%02d-%02d %02d:%02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

	CString sVersao   = m_VersaoSist;	

	/////int secs = CUtil::CalculateSecondsNow(m_sDateLastJob); 
	double dbLatitude = -1.0, dbLongitude = -1.0;
	CString sLat = L"";
	CString sLong = L"";
	#if _WIN32_WCE != 0x420 && defined _WIN32_WCE
		if (m_bHasGPS)
		{
			if (CGPSController::getCurrentLatLong(dbLatitude, dbLongitude))
			{
				sLat.Format(L"%f", dbLatitude);
				sLong.Format(L"%f", dbLongitude);
			}
		}
	#endif

	///espaço em disco
	CString sDiskFree;
	CString sMainDir = CUtil::GetMainAppPath();	
	ULARGE_INTEGER nFreeToCaller;
	ULARGE_INTEGER nTotal;
	ULARGE_INTEGER nFree;
	ULONGLONG nFreePercent;

	GetDiskFreeSpaceEx(sMainDir, &nFreeToCaller, &nTotal, &nFree);
	nFreePercent = (nFree.QuadPart*100)/nTotal.QuadPart;
	sDiskFree.Format(L"%ld",nFreePercent);

	///agente logado
	CString sAgenteLogado = CUtil::GetLoggedUser();
	if (sAgenteLogado.CompareNoCase(L"--NENHUM--") == 0)
	{
		sAgenteLogado = L"";
	}
	///Total de AIT não transmitidos:
	CString sTotalNaoTransmitido = L"";
	TotalAitNaoTransmitido(CppSQLite3DB::getInstance(), sTotalNaoTransmitido);

	///Ultima Transmissão:
	CString sUltimaTransmissao = L"";
	SYSTEMTIME stut;
	if (GetLastDataSent(&stut))
	{
		sUltimaTransmissao.Format(L"%04d-%02d-%02d %02d:%02d:%02d", stut.wYear, stut.wMonth, stut.wDay, stut.wHour, stut.wMinute, stut.wSecond);
	}

	///Ultima Atualização:
	SYSTEMTIME  st_upd;
	ZeroMemory(&st_upd, sizeof(st_upd));

	CUtil::GetLastAtualizacao(&st_upd);
	CString sDataAtu;
	sDataAtu.Format(L"%04d-%02d-%02d %02d:%02d:%02d", st_upd.wYear, st_upd.wMonth, st_upd.wDay, st_upd.wHour, st_upd.wMinute, st_upd.wSecond);

	///Ultimo AIT transmitido:
	CString sUltimoAitTransmitido = L"";
	SerieUltimoAitTransmitido(CppSQLite3DB::getInstance(), sUltimoAitTransmitido);


	/*** Início construção do XML de envio do GPS ***/
	CXmlCreate xml;	
	xml.OpenRootTag(L"SIT_TEM");
		xml.AddElement(L"contrato", m_Contrato);
		///xml.AddElement(L"versao", sVersao);
		xml.AddElement(L"id_talao", m_IdTalao);
		xml.AddElement(L"latitude", sLat);
		xml.AddElement(L"longitude", sLong);
		xml.AddElement(L"espaco_disco", sDiskFree );
		xml.AddElement(L"nivel_bateria", CPowerMonCtrl::GetBatteryPerc() );
		xml.AddElement(L"nivel_sinal_cel",CUtil::GetQualidadeSinal() );
		xml.AddElement(L"datahora", m_sDataHora);
		xml.AddElement(L"versao_sistema", sVersao);
		xml.AddElement(L"agente_logado", sAgenteLogado);
		xml.AddElement(L"qtd_ait_preso", sTotalNaoTransmitido );
		xml.AddElement(L"data_hora_trans_ait", sUltimaTransmissao );
		xml.AddElement(L"data_hora_atua_ae", sDataAtu );
		xml.AddElement(L"ult_ait_trans", sUltimoAitTransmitido );
	xml.CloseRootTag(L"SIT_TEM");
	/*** Final da construção do XML ***/		

	//Valida estrutura do XML criado
	if(xml.ValidateXml())
	{	
		//Cria diretório GPS do httpsender se não existir
		CString sBaseDir = CUtil::GetMainAppPath() + L"\\INFO";	
		CUtil::CreateDirIfNotExist(sBaseDir);

		CString sTimeStamp = CUtil::GetCurrentTimeStamp();

		//Cria arquivo XML físico
		CString sPathXmlFile;
		sPathXmlFile.Format(L"%s\\INFO_%s.xml",sBaseDir, sTimeStamp);
		xml.CreateXmlFile(sPathXmlFile);

		//Vars enviadas por http-post
		CString sVarsList;
		///sVarsList.AppendFormat(L"contrato=%s", m_Contrato);
		sVarsList.AppendFormat(L"contrato=%s|cd_equipamento=%s|agente=%s", m_Contrato, CUtil::GetIDTalao(), sAgenteLogado);
		//Cria registro do job de envio	do XML
		//O szObjName deve ser o mesmo do nome descrito no atributo name da tag httpsenderthread do pma.xml
		if(CConsultasHttpSender::GravaHttpSenderJob(CppSQLite3DB::getInstance(), L"XML_INFO", sPathXmlFile, L"XML", sVarsList, L"", L"", L"", L""))
		{
			//STLOG_WRITE(L"CAitSend::Send() Inserido registro job de envio de INFO");
			//m_stDateLastJob = st;
			///m_sDateLastJob = m_sDataHora;
		}
		else
		{
			//STLOG_WRITE(L"CAitSend::Send() Falha ao inserir registro Job de envio de GPS");
		}								
	}

	return S_OK;
}



BOOL CInfoSender::Create(CWnd *pParentWnd)
{
	BOOL b = CreateEx(WS_EX_TRANSPARENT,
				      AfxRegisterWndClass(0), 
					  NULL, 
					  WS_ICONIC, 
					  CRect(0, 0, GetSystemMetrics(SM_CXFULLSCREEN), GetSystemMetrics(SM_CYFULLSCREEN)+DRA::SCALEY(26)),
					  pParentWnd, 
					  0x556);	

	if(b)
	{
	
		ShowWindow(SW_HIDE);
		Invalidate();
		return TRUE;
	}

	return FALSE;
}


/**
\brief Consulta a data da ultima tentativa de transmissão
\param LPSYSTEMTIME st Ponteiro que receberá a data
\exception Erro de execução da query
\return BOOL
*/
BOOL CInfoSender::GetLastDataSent(LPSYSTEMTIME st)
{
	CStringA sQuery;

	try
	{
		sQuery.Format("SELECT strftime('%%d/%%m/%%Y', max(dt_log)), time_log FROM httpsender_log WHERE status <> 0 and obj_name = 'XML_AIT'");
		if(!sQuery.IsEmpty())
		{
			CppSQLite3Query q = CppSQLite3DB::getInstance()->execQuery(sQuery);
			if(!q.eof())
			{
				if(strlen(q.getStringField(0)) == 0)
					return FALSE;

				CStringArray strArr;
				int size = 3;
				CUtil::Tokenize(CString(q.getStringField(0)), strArr, size, L"/");
				//data
				st->wDay = _wtoi(strArr[0]);
				st->wMonth = _wtoi(strArr[1]);
				st->wYear = _wtoi(strArr[2]);

				//hora
				strArr.RemoveAll();
				CUtil::Tokenize(CString(q.getStringField(1)), strArr, size, L":");
				st->wHour = _wtoi(strArr[0]);
				st->wMinute = _wtoi(strArr[1]);
				st->wSecond = _wtoi(strArr[2]);

			}

			q.finalize();
		}
	}
	catch(CppSQLite3Exception e)
	{
		STLOG_WRITE("CConsultas_TEM::GetAITSincro: ERRO executando query", 
					e.errorMessage());

		STLOG_WRITE(sQuery);

		return FALSE;
	}

	return TRUE;

}


/**
\brief Consulta Total de AIT não transmitidos
\param CppSQLite3DB *pDB Handle de conexão do banco de dados
\param CString &sTotalNaoTransmit total de autuações não enviadas
\exception Erro de execução da query
\return BOOL
*/
BOOL CInfoSender::TotalAitNaoTransmitido(CppSQLite3DB *pDB, CString &sTotalNaoTransmit)
{
	CStringA sQuery;

	try
	{
		sQuery.Format("SELECT count(serie) from aiip WHERE transmissao = 0;");
		if(!sQuery.IsEmpty())
		{
			CppSQLite3Query q = pDB->execQuery(sQuery);
			if(!q.eof())
			{
				CString sdt(q.getStringField(0));

				if(sdt.IsEmpty())
					return FALSE;
				sTotalNaoTransmit = sdt;
			}

			q.finalize();
			return TRUE;
		}
	}
	catch(CppSQLite3Exception e)
	{
		STLOG_WRITE("CConsultas_TEM::TotalAitNaoTransmitido: ERRO executando query", e.errorMessage());
		STLOG_WRITE(sQuery);
	}

	return FALSE;
}

/**
\brief Consulta serie do ultimo AIT transmitido
\param CppSQLite3DB *pDB Handle de conexão do banco de dados
\param CString &sSerieUltimoAitTrans serie da última autuação enviada
\exception Erro de execução da query
\return BOOL
*/
BOOL CInfoSender::SerieUltimoAitTransmitido(CppSQLite3DB *pDB, CString &sSerieUltimoAitTrans)
{
	CStringA sQuery;

	try
	{
		sQuery.Format("SELECT serie, strftime('%%d/%%m/%%Y',max(data)) from aiip WHERE transmissao = 1;");
		if(!sQuery.IsEmpty())
		{
			CppSQLite3Query q = pDB->execQuery(sQuery);
			if(!q.eof())
			{
				CString sdt(q.getStringField(0));

				if(sdt.IsEmpty())
					return FALSE;
				sSerieUltimoAitTrans = sdt;
			}

			q.finalize();
			return TRUE;
		}
	}
	catch(CppSQLite3Exception e)
	{
		STLOG_WRITE("CConsultas_TEM::DataPerSerieUltimoAitTransmitido: ERRO executando query", e.errorMessage());
		STLOG_WRITE(sQuery);
	}

	return FALSE;
}

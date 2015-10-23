//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
// Modo de criacao da camera via DirectShow API
//
#include "stdafx.h"

#include "graphmanager.h"
#include "CPropertyBag.h"

#define CHK( x ) do{ if( FAILED( hr = ( x )) && hr != VFW_S_NOPREVIEWPIN ) { goto Cleanup; }} while( FALSE );
#define ERR( x ) do{ hr = x; goto Cleanup; } while( FALSE );

#if _WIN32_WCE >= 0x500

CGraphManager::CGraphManager()
{
	m_fGraphBuilt = FALSE;
	ZeroMemory( m_handle, sizeof( m_handle ));
	m_hwnd = 0;
	m_dwThreadId = 0;
	m_hThread = NULL;
	m_hCommandCompleted = NULL;
	m_currentCommand = COMMAND_NOCOMMAND;

	m_rectPreview.top    = 0;
	m_rectPreview.left   = 0;
	m_rectPreview.right  = 120;
	m_rectPreview.bottom = 160;
	m_szFileName = NULL;

    // Launch COM
    CoInitialize( NULL );
}


CGraphManager::~CGraphManager()
{
	ShutDown();

    if( m_handle[0] )
    {
        CloseHandle( m_handle[0] );
		m_handle[0] = NULL;
    }

    if( m_handle[1] )
    {
        CloseHandle( m_handle[1] );
		m_handle[1] = NULL;
    }

	CoUninitialize();
}


HRESULT CGraphManager::Init(HWND hwnd, STILLSIZES stillSize, const RECT *rectPreview)
{
    HRESULT hr = S_OK;

	m_nStillSize = stillSize;
	m_hwnd = hwnd;
	
	if(rectPreview != NULL)
	{
		m_rectPreview.top    = rectPreview->top;
		m_rectPreview.left   = rectPreview->left;
		m_rectPreview.bottom = rectPreview->bottom;
		m_rectPreview.right  = rectPreview->right;
	}

    // Create the event that will signal the thread for commands
    m_handle[0] = CreateEvent( NULL, FALSE, FALSE, NULL );
    if( m_handle[0] == NULL )
    {
        ERR( HRESULT_FROM_WIN32( GetLastError() ));
    }
    m_handle[1] = 0;

    // Create the event to sync on to wait for the command to be executed
    m_hCommandCompleted = CreateEvent( NULL, FALSE, FALSE, NULL );
    if( m_hCommandCompleted == NULL )
    {
        ERR( HRESULT_FROM_WIN32( GetLastError() ));
    }

    // CCreate the thread that will run the filtergraph. 
    // The filtergraph is runing on a background thread to prevent any window message
    // reentrancy issue. 
    m_hThread = CreateThread( NULL, 0, CGraphManager::ThreadProc, this, 0, &m_dwThreadId );
    if( m_hThread == NULL )
    {
        ERR( HRESULT_FROM_WIN32( GetLastError() ));
    }


Cleanup:
    return hr;
}

HRESULT CGraphManager::BuildCaptureGraph()
{
    // The Graph is built on a separate thread to 
    // prevent reentrancy issues. 
    m_currentCommand = COMMAND_BUILDGRAPH;
    SetEvent( m_handle[0] );
    WaitForSingleObject( m_hCommandCompleted, INFINITE );

    return S_OK;
}


HRESULT CGraphManager::RunCaptureGraph()
{
    // Unlike the other operations, running the graph
    // has to happen from the UI thread. 
    return RunCaptureGraphInternal();
}


HRESULT CGraphManager::StartRecordVideo()
{
    m_currentCommand = COMMAND_STARTCAPTURE;
    SetEvent( m_handle[0] );
    WaitForSingleObject( m_hCommandCompleted, INFINITE );

    return S_OK;
}


HRESULT CGraphManager::StopRecordVideo()
{
    m_currentCommand = COMMAND_STOPCAPTURE;
    SetEvent( m_handle[0] );
    WaitForSingleObject( m_hCommandCompleted, INFINITE );

    return S_OK;
}


HRESULT CGraphManager::CaptureStillImage(LPCTSTR szFileName)
{
    m_currentCommand = COMMAND_STILLIMAGE;
	m_szFileName     = szFileName;

    SetEvent( m_handle[0] );
    WaitForSingleObject( m_hCommandCompleted, INFINITE );

    return S_OK;
}


HRESULT CGraphManager::ShutDown()
{
    m_currentCommand = COMMAND_SHUTDOWN;
    SetEvent( m_handle[0] );
    WaitForSingleObject( m_hThread, INFINITE );

    return S_OK;
}

DWORD WINAPI CGraphManager::ThreadProc( LPVOID lpParameter )
{
    HRESULT       hr = S_OK;
    DWORD         dwReturnValue;
    CGraphManager *pThis = (CGraphManager*) lpParameter;
    GRAPHCOMMANDS command = COMMAND_NOCOMMAND;

    if( pThis == NULL )
        return 0;

    while(( command != COMMAND_SHUTDOWN ) && ( hr != S_FALSE ))
    {
        dwReturnValue = WaitForMultipleObjects( 2, pThis->m_handle, FALSE, INFINITE );
        switch( dwReturnValue )
        {
            case WAIT_OBJECT_0:
                command = pThis->m_currentCommand;
                pThis->ProcessCommand();
                break;

            case WAIT_OBJECT_0 + 1:
                pThis->ProcessDShowEvent();
                break;

            default:
                break;
        }
    };

    return 0;
}


HRESULT CGraphManager::ProcessCommand()
{
    HRESULT hr = S_OK;
    
    switch( m_currentCommand )
    {
        case COMMAND_BUILDGRAPH:
            hr = CreateCaptureGraphInternal();
            SetEvent( m_hCommandCompleted );
            break;

        case COMMAND_RUNGRAPH:
            hr = RunCaptureGraphInternal();
            SetEvent( m_hCommandCompleted );
            break;

        case COMMAND_STARTCAPTURE:
            hr = StartCaptureVideoInternal();
            SetEvent( m_hCommandCompleted );
            break;

        case COMMAND_STOPCAPTURE:
            hr = StopCaptureVideoInternal();
            SetEvent( m_hCommandCompleted );
            break;
        
		case COMMAND_STILLIMAGE:
			hr = CaptureStillImageInternal();
            SetEvent( m_hCommandCompleted );
            break;

        case COMMAND_SHUTDOWN:
            hr = S_FALSE;
            break;

        default:
            break;
    }

    return hr;
}


HRESULT CGraphManager::ProcessDShowEvent()
{
    HRESULT hr = S_OK;
    long    lEventCode, lParam1, lParam2;

    CComPtr<IMediaEvent> pMediaEvent;
    CComPtr<IGraphBuilder> pFilterGraph;
    CComPtr<IMediaControl> pMediaControl;

    if( m_pCaptureGraphBuilder == NULL )
        ERR( E_FAIL );

    CHK( m_pCaptureGraphBuilder->GetFiltergraph( &pFilterGraph ));
    CHK( pFilterGraph->QueryInterface( &pMediaEvent ));
    CHK( pMediaEvent->GetEvent( &lEventCode, &lParam1, &lParam2, 0 ));

    if( lEventCode == EC_STREAM_CONTROL_STOPPED )   // We have received a control stream stop event
    {
        NotifyMessage( MESSAGE_ENDRECORDING, L"Recording done ..." );

        // To close the file, get the IMediaControl interface, stop and 
        // restart the graph
        CHK( pFilterGraph->QueryInterface( &pMediaControl ));
        if( pMediaControl == NULL )
        {
            ERR( E_POINTER );
        }         

        CHK( pMediaControl->Stop());
        CHK( pMediaControl->Run());

        NotifyMessage( MESSAGE_FILECAPTURED, L"File captured ..." );
    }
    else if( lEventCode == EC_CAP_FILE_COMPLETED )
    {
        NotifyMessage( MESSAGE_FILECAPTURED, L"File captured ..." );
    }


    CHK( pMediaEvent->FreeEventParams( lEventCode, lParam1, lParam2 ));

Cleanup:
    return S_OK;
}


HRESULT CGraphManager::GetFirstCameraDriver( WCHAR *pwzName )
{
	HRESULT hr = S_OK;
	HANDLE	handle = NULL;
	DEVMGR_DEVICE_INFORMATION di;
	GUID guidCamera = { 0xCB998A05, 0x122C, 0x4166, 0x84, 0x6A, 0x93, 0x3E, 0x4D, 0x7E, 0x3C, 0x86 };
	// Note about the above: The driver material doesn't ship as part of the SDK. This GUID is hardcoded
	// here to be able to enumerate the camera drivers and pass the name of the driver to the video capture filter

	if( pwzName == NULL )
		return E_POINTER;

	di.dwSize = sizeof(di);

	handle = FindFirstDevice( DeviceSearchByGuid, &guidCamera, &di );
	if(( handle == NULL ) || ( di.hDevice == NULL ))
	{
		ERR( HRESULT_FROM_WIN32( GetLastError() ));
	}

	StringCchCopy( pwzName, MAX_PATH, di.szLegacyName );

Cleanup:
	FindClose( handle );
	return hr;
}

HRESULT CGraphManager::CreateCaptureGraphInternal()
{
    HRESULT       hr = S_OK;
    CComVariant   varCamName;
    CPropertyBag  PropBag;
    OAEVENT       oaEvent;
	WCHAR	      wzDeviceName[ MAX_PATH + 1 ];

	HCURSOR hOldCursor = GetCursor();
	SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(IDC_WAIT))); 

    CComPtr<IMediaEvent>            pMediaEvent;
    CComPtr<IGraphBuilder>          pFilterGraph;
    CComPtr<IBaseFilter>            pVideoEncoder;
    CComPtr<IBaseFilter>            pASFMultiplexer;
	CComPtr<IFileSinkFilter>        pFileSinkFilter;
    CComPtr<IPersistPropertyBag>    pPropertyBag;
    CComPtr<IDMOWrapperFilter>      pWrapperFilter;
    CComPtr<IBaseFilter>			pImageSinkFilter;

    //
    // Create the capture graph builder and register the filtergraph manager. 
    //
    CHK( m_pCaptureGraphBuilder.CoCreateInstance( CLSID_CaptureGraphBuilder ));
    CHK( pFilterGraph.CoCreateInstance( CLSID_FilterGraph ));
    CHK( m_pCaptureGraphBuilder->SetFiltergraph( pFilterGraph ));

	//
    // Create and initialize the video capture filter
    //
    CHK( m_pVideoCaptureFilter.CoCreateInstance( CLSID_VideoCapture ));
    CHK( m_pVideoCaptureFilter.QueryInterface( &pPropertyBag ));

    // We are loading the driver CAM1 in the video capture filter. 
	CHK( GetFirstCameraDriver( wzDeviceName ));
    varCamName = wzDeviceName;
    if( varCamName.vt != VT_BSTR )
        ERR( E_OUTOFMEMORY );

    CHK( PropBag.Write( L"VCapName", &varCamName ));   
    CHK( pPropertyBag->Load( &PropBag, NULL ));

    // Everything succeeded, the video capture filter is added to the filtergraph
    CHK( pFilterGraph->AddFilter( m_pVideoCaptureFilter, L"Video Capture Filter Source" ));

    // 
    // Third step: Create the video encoder DMO, load the WMV9 encoder, and 
    // add it to the graph
    //

    // Create the video encoder
    CHK( pVideoEncoder.CoCreateInstance( CLSID_DMOWrapperFilter ));
    CHK( pVideoEncoder.QueryInterface( &pWrapperFilter ));

    // Load the WMV9 DMO
    CHK( pWrapperFilter->Init( CLSID_CWMV9EncMediaObject, DMOCATEGORY_VIDEO_ENCODER ));

    // Everything succeeded, let's add the encoder to the graph
    CHK( pFilterGraph->AddFilter( pVideoEncoder, L"WMV9 DMO Encoder" ));

    //
    // Create the ASF multiplexer and add it to the graph
    //
    CHK( m_pCaptureGraphBuilder->SetOutputFileName(&MEDIASUBTYPE_Asf, 
												   L"\\video1.asf", 
												   &pASFMultiplexer, 
												   &pFileSinkFilter ));

	// 
	// Adiciona o preview...
	//
	CHK( AddPreview() );

    //
    // Connect the video capture filter, the encoder and the multiplexer together
    // 
    CHK( m_pCaptureGraphBuilder->RenderStream(&PIN_CATEGORY_CAPTURE, 
											  &MEDIATYPE_Video, 
											  m_pVideoCaptureFilter, 
											  pVideoEncoder, 
											  pASFMultiplexer));

	//
	// Faz o setup da captura da imagem... 
	//
	CHK( SetupStillImage() );

	//
	// Create the still image filter, and connect it to the video capture filter
	//
	CHK( pImageSinkFilter.CoCreateInstance( CLSID_IMGSinkFilter ));

	CHK( pFilterGraph->AddFilter( pImageSinkFilter, L"Still image filter" ));

	CHK( m_pCaptureGraphBuilder->RenderStream( &PIN_CATEGORY_STILL, 
											   &MEDIATYPE_Video, 
											   m_pVideoCaptureFilter, 
											   NULL, 
											   pImageSinkFilter ));

	CHK( pImageSinkFilter.QueryInterface( &m_pImageSinkFilter ));

    //
    // Prevent the data from flowing into the capture stream
    //
    CHK( m_pCaptureGraphBuilder->ControlStream( &PIN_CATEGORY_CAPTURE, 
												&MEDIATYPE_Video, 
												m_pVideoCaptureFilter, 
												0, 0 , 0, 0 ));

    //
    // Let's get the handle for DShow events. The main loop will listen to both notifications from 
    // the UI thread and for DShow notifications
    //
    CHK( pFilterGraph->QueryInterface( IID_IMediaEvent, (void**) &pMediaEvent ));
    CHK( pMediaEvent->GetEventHandle( &oaEvent ));

    m_handle[1] = (HANDLE) oaEvent;

    m_fGraphBuilt = TRUE;

	SetCursor(hOldCursor); 

Cleanup:
	if( FAILED( hr ) )
	{
		NotifyMessage( MESSAGE_ERROR, L"Builing the graph failed" );
	}
    return hr;
}


HRESULT CGraphManager::RunCaptureGraphInternal()
{
    HRESULT hr = S_OK;

    CComPtr<IGraphBuilder> pGraphBuilder;
    CComPtr<IMediaControl> pMediaControl;

    // Let's make sure that the graph has been initialized
    if(( m_pCaptureGraphBuilder == NULL ) || ( m_fGraphBuilt == FALSE ))
        ERR( E_FAIL );

    // Retrieve the filtergraph off the capture graph builder
    CHK( m_pCaptureGraphBuilder->GetFiltergraph( &pGraphBuilder ));

    // Get the media control interface, and run the graph
    CHK( pGraphBuilder->QueryInterface( &pMediaControl ));

    CHK( pMediaControl->Run());
    CHK( NotifyMessage( MESSAGE_INFO, L"The Graph is running" ));

Cleanup:
	if( FAILED( hr ))
	{
		NotifyMessage( MESSAGE_ERROR, L"Runing the capture graph failed" );
	}

    return hr;
}


HRESULT CGraphManager::StartCaptureVideoInternal()
{
    HRESULT  hr = S_OK;
    LONGLONG dwStart = 0, dwEnd = 0;
    WORD     wStartCookie = 1, wEndCookie = 2;
    WORD     wStartCookie1 = 1, wEndCookie1 = 2;

    CComPtr<IGraphBuilder>  pGraphBuilder;

    // Make sure that the capture graph builder is present
    if(( m_pCaptureGraphBuilder == NULL ) || ( m_fGraphBuilt == FALSE ))
        ERR( E_FAIL );

    // Let's retrieve the event signaled by the filtergraph
    CHK( m_pCaptureGraphBuilder->GetFiltergraph( &pGraphBuilder ));

    // To start the video capture, call ICaptureGraphBuilder2::ControlStream. 
    dwStart=0;
    dwEnd=MAXLONGLONG;
    CHK( m_pCaptureGraphBuilder->ControlStream( &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, NULL, &dwStart, &dwEnd, wStartCookie, wEndCookie ));

    CHK( NotifyMessage( MESSAGE_INFO, L"Capturing...." ));

Cleanup:
	if( FAILED( hr ))
	{
		NotifyMessage( MESSAGE_ERROR, L"Starting the video recording failed" );
	}
    return hr;
}


HRESULT CGraphManager::StopCaptureVideoInternal()
{
    HRESULT   hr = S_OK;
    LONGLONG  dwStart = 0, dwEnd = 0;
    WORD      wStartCookie = 1, wEndCookie = 2;


    CComPtr<IGraphBuilder> pFilterGraph;
    CComPtr<IMediaSeeking> pMediaSeeking;

    CHK( m_pCaptureGraphBuilder->GetFiltergraph( &pFilterGraph ));
    CHK( pFilterGraph->QueryInterface( &pMediaSeeking ));

    // Stop the capture of the graph
    dwStart = 0;
    CHK( pMediaSeeking->GetCurrentPosition( &dwEnd ));

    CHK( m_pCaptureGraphBuilder->ControlStream( &PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, NULL, &dwStart, &dwEnd, wStartCookie, wEndCookie ));

    // The filtergraph will fire a EC_CONTROLSTREAM_STOPPED event once the capture of the video is completed
    // The event will be caugth in the main loop and processed in the ProcessDshowEvents function
    CHK( NotifyMessage( MESSAGE_INFO, L"Encoding file ..." ));

Cleanup:
	if( FAILED( hr ))
	{
		NotifyMessage( MESSAGE_ERROR, L"Stopping the video recording failed" );
	}
    return hr;
}


HRESULT CGraphManager::CaptureStillImageInternal()
{
	HRESULT hr = S_OK;
	CComPtr<IFileSinkFilter> pFileSink;
	CComPtr<IUnknown>		 pUnkCaptureFilter;
	CComPtr<IPin>			 pStillPin;
	CComPtr<IAMVideoControl> pVideoControl;

    if(( m_pCaptureGraphBuilder == NULL ) || ( m_fGraphBuilt == FALSE ))
        ERR( E_FAIL );

	CHK( m_pImageSinkFilter.QueryInterface( &pFileSink ));

	CHK( pFileSink->SetFileName(m_szFileName, NULL));

	CHK( m_pVideoCaptureFilter.QueryInterface( &pUnkCaptureFilter ));

	CHK( m_pCaptureGraphBuilder->FindPin( pUnkCaptureFilter, 
										  PINDIR_OUTPUT, 
										  &PIN_CATEGORY_STILL, 
										  &MEDIATYPE_Video, 
										  FALSE, 
										  0, 
										  &pStillPin ));

	CHK( m_pVideoCaptureFilter.QueryInterface( &pVideoControl ));
	CHK( pVideoControl->SetMode( pStillPin, VideoControlFlag_Trigger ));

Cleanup:
	if( FAILED( hr ))
	{
		NotifyMessage( MESSAGE_ERROR, L"Capturing a still image failed" );
	}
    return hr;
}


HRESULT CGraphManager::NotifyMessage( DSHOW_MESSAGE message, WCHAR *wzText )
{
    HRESULT hr = S_OK;
    Message *pMessage;
    WCHAR   *wzString;

    if(( wzText == NULL ) || ( *wzText == NULL ))
    {
        ERR( E_POINTER );
    }

    if( m_hwnd == NULL )
    {
        return S_FALSE;
    }

    pMessage = (Message*) LocalAlloc( LMEM_ZEROINIT, sizeof( Message ));
    if( pMessage == NULL )
    {
        ERR( E_OUTOFMEMORY );
    }

    wzString = (WCHAR*) LocalAlloc( LMEM_ZEROINIT, ( wcslen( wzText ) + 1 ) * sizeof( WCHAR ));
    if( wzString == NULL )
    {
        ERR( E_OUTOFMEMORY );
    }

    StringCchCopy( wzString, wcslen( wzText ) + 1, wzText );

    pMessage->wzMessage = wzString;
    pMessage->dwMessage = message;

    PostMessage( m_hwnd, WM_USER, 0, (LPARAM) pMessage );

	return hr;

Cleanup:
    return hr;
}

HRESULT CGraphManager::AddPreview()
{
	HRESULT hr = S_OK;
	CComPtr<IVideoWindow>  pVideoWindow;

	CHK( m_pCaptureGraphBuilder->RenderStream(&PIN_CATEGORY_PREVIEW, 
											  &MEDIATYPE_Video, 
											  m_pVideoCaptureFilter, 
											  NULL, 
											  NULL));

	CHK( m_pCaptureGraphBuilder->FindInterface(NULL,
											   &PIN_CATEGORY_PREVIEW,
											   m_pVideoCaptureFilter,
											   IID_IVideoWindow, 
											   (void **)&pVideoWindow));

	pVideoWindow->put_Owner((OAHWND)m_hwnd);
	pVideoWindow->put_WindowStyle(WS_CHILD|WS_CLIPSIBLINGS);

	pVideoWindow->SetWindowPosition(m_rectPreview.left, 
									m_rectPreview.top, 
									m_rectPreview.right-m_rectPreview.left, 
									m_rectPreview.bottom-m_rectPreview.top);

	pVideoWindow->put_Visible(OATRUE);
	pVideoWindow->put_Width (m_rectPreview.right-m_rectPreview.left);
	pVideoWindow->put_Height(m_rectPreview.bottom-m_rectPreview.top);

	//pVideoWindow->put_MessageDrain((OAHWND)m_hwnd);

Cleanup:
	if( FAILED( hr ))
	{
		NotifyMessage( MESSAGE_ERROR, L"Capturing a still image failed" );
	}
    return hr;
}

HRESULT CGraphManager::SetupStillImage()
{
	HRESULT hr = S_OK;
	CComPtr<IAMStreamConfig> pStillStreamConfig;

	CHK(m_pCaptureGraphBuilder->FindInterface(&PIN_CATEGORY_STILL,
											  &MEDIATYPE_Video,
											  m_pVideoCaptureFilter, 
											  IID_IAMStreamConfig,
											  (void**)&pStillStreamConfig));

	AM_MEDIA_TYPE *pType = NULL, *pType1 = NULL;
	int iCount=0, iSize=0;
	VIDEO_STREAM_CONFIG_CAPS caps;

	pStillStreamConfig->GetNumberOfCapabilities(&iCount, &iSize);
	int i = 0;
	for(i = 0; i < iCount; i++)
	{
		// GetStreamCaps allocates the AM_MEDIA_TYPE, which must be deleted by using DeleteMediaType
		if(pStillStreamConfig->GetStreamCaps(i, &pType, (BYTE*)&caps) == S_OK )
		{
			VIDEOINFOHEADER *fVideoHeader = (VIDEOINFOHEADER*)pType->pbFormat;
			if(fVideoHeader->bmiHeader.biWidth == 120  && 
			   fVideoHeader->bmiHeader.biHeight == 160 &&
			   m_nStillSize == STILLSIZE_120X160)
			{
				CHK(pStillStreamConfig->SetFormat(pType));
				DeleteMediaType(pType);
				break;
			}
			else if(fVideoHeader->bmiHeader.biWidth == 240  && 
					fVideoHeader->bmiHeader.biHeight == 320 &&
					m_nStillSize == STILLSIZE_240X320)
			{
				CHK(pStillStreamConfig->SetFormat(pType));
				DeleteMediaType(pType);
				break;
			}
			else if(fVideoHeader->bmiHeader.biWidth == 480  && 
					fVideoHeader->bmiHeader.biHeight == 640 &&
					m_nStillSize == STILLSIZE_480X640)
			{
				CHK(pStillStreamConfig->SetFormat(pType));
				DeleteMediaType(pType);
				break;
			}
			else
				DeleteMediaType(pType);
		}
	}

Cleanup:
	if( FAILED( hr ))
	{
		NotifyMessage( MESSAGE_ERROR, L"Capturing a still image failed" );
	}
    return hr;
}

#endif

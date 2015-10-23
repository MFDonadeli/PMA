// Copyright (c) 2007 Marcos Mori de Siqueira. All Rights Reserved. 
// mori@softfactory.com.br
// Classe que representa o monitor de bateria, provendo uma representacao
// grafica do status da bateria e disparando mensagens informando este
// status
#include "stdafx.h"
#ifdef _WIN32_WCE

#include <pm.h>
#include <msgqueue.h>
#include "PowerMonCtrl.h"
#include "Utils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define BAT_FRONT_HEIGHT	7
#define BAT_FRONT_WIDTH		3
#define STATUS_HEIGHT		6

UINT CPowerMonCtrl::WM_BATMON_ALERT = 
	RegisterWindowMessage(L"CPowerMonCtrl::WM_BATMON_ALERT");

CString CPowerMonCtrl::m_sPerc = L"--";

IMPLEMENT_DYNAMIC(CPowerMonCtrl, CWnd)

CPowerMonCtrl::CPowerMonCtrl()
	: m_pThread(NULL)
{
	m_nPerc = 0;
	m_nPercAlert = 30;
}

CPowerMonCtrl::~CPowerMonCtrl()
{
	Stop();
}

BEGIN_MESSAGE_MAP(CPowerMonCtrl, CWnd)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
END_MESSAGE_MAP()

BOOL CPowerMonCtrl::Create(CWnd *pParentWnd, const RECT &rect, UINT nID)
{
	CUtil::CreateFont(&m_font, L"Verdana", -7, FALSE);
	return CWnd::Create(NULL, NULL, WS_CHILD, rect, pParentWnd, nID);
}

BOOL CPowerMonCtrl::Start(int nPercAlert)
{
	m_nPercAlert = nPercAlert;

	m_pThread = AfxBeginThread(_ThreadProc, this);
	if(m_pThread != NULL)
		return FALSE;

	return TRUE;
}

void CPowerMonCtrl::Stop()
{
	if(m_pThread != NULL)
	{
		m_evStop.SetEvent();
		WaitForSingleObject(m_evStopped, INFINITE);
		m_pThread = NULL;
	}
}

UINT CPowerMonCtrl::_ThreadProc(LPVOID pParam)
{
	CPowerMonCtrl *p = reinterpret_cast<CPowerMonCtrl *>(pParam);
	ASSERT(p != NULL);
	if(p != NULL)
		return p->ThreadProc();

	return 0xDEAD;
}

UINT CPowerMonCtrl::ThreadProc()
{
    // size of a POWER_BROADCAST message
    DWORD cbPowerMsgSize = sizeof POWER_BROADCAST + (MAX_PATH * sizeof TCHAR);

    // Initialize our MSGQUEUEOPTIONS structure
    MSGQUEUEOPTIONS mqo;
    mqo.dwSize = sizeof(MSGQUEUEOPTIONS); 
    mqo.dwFlags = MSGQUEUE_NOPRECOMMIT;
    mqo.dwMaxMessages = 4;
    mqo.cbMaxMessage = cbPowerMsgSize;
    mqo.bReadAccess = TRUE;              

    // Create a message queue to receive power notifications
    HANDLE hPowerMsgQ = CreateMsgQueue(NULL, &mqo);
    if(NULL == hPowerMsgQ) 
    {
		m_pThread = NULL;
        //TRACE(L"CreateMsgQueue failed: %x\n", GetLastError());
        return 0L;
    }

    // Request power notifications 
    HANDLE hPowerNotifications = RequestPowerNotifications(hPowerMsgQ,
                                                           PBT_TRANSITION | 
                                                           PBT_RESUME | 
                                                           PBT_POWERINFOCHANGE);
    if(NULL == hPowerNotifications) 
    {
		if(hPowerMsgQ)
			CloseMsgQueue(hPowerMsgQ);

		m_pThread = NULL;
        //TRACE(L"RequestPowerNotifications failed: %x\n", GetLastError());
        return 0L;;
    }

    HANDLE rgHandles[2] = {0};
    rgHandles[0] = hPowerMsgQ;
    rgHandles[1] = m_evStop;

    // Wait for a power notification or for the app to exit
    while(WaitForMultipleObjects(2, rgHandles, FALSE, INFINITE) == WAIT_OBJECT_0)
    {
        DWORD cbRead;
        DWORD dwFlags;
        POWER_BROADCAST *ppb = (POWER_BROADCAST*) new BYTE[cbPowerMsgSize];

        // loop through in case there is more than 1 msg 
        while(ReadMsgQueue(hPowerMsgQ, ppb, cbPowerMsgSize, &cbRead, 0, &dwFlags))
        {
            switch (ppb->Message)
            {
                case PBT_TRANSITION:
                    //TRACE(L"Power Notification Message: PBT_TRANSITION\n");
                    //TRACE(L"Flags: %lx", ppb->Flags);
                    //TRACE(L"Length: %d", ppb->Length);
                    //if (ppb->Length)
                    //    TRACE(L"SystemPowerState: %s\n", ppb->SystemPowerState);
                    break;

                case PBT_RESUME:
                    //TRACE(L"Power Notification Message: PBT_RESUME\n");
                    break;

                case PBT_POWERINFOCHANGE:
                {
                    //TRACE(L"Power Notification Message: PBT_POWERINFOCHANGE\n");

                    // PBT_POWERINFOCHANGE message embeds a 
                    // POWER_BROADCAST_POWER_INFO structure into the 
                    // SystemPowerState field
                    PPOWER_BROADCAST_POWER_INFO ppbpi =
                        (PPOWER_BROADCAST_POWER_INFO) ppb->SystemPowerState;
                    if (ppbpi) 
                    {
						m_nPerc = ppbpi->bBatteryLifePercent;

						m_sPerc.Format(L"%d", m_nPerc);

						if(GetParent() != NULL)
						{
							if(m_nPerc <= m_nPercAlert)
								GetParent()->PostMessage(CPowerMonCtrl::WM_BATMON_ALERT, m_nPerc);
						}
						/*
                        TRACE(L"Length: %d", ppb->Length);
                        TRACE(L"BatteryLifeTime = %d\n",ppbpi->dwBatteryLifeTime);
                        TRACE(L"BatterFullLifeTime = %d\n", ppbpi->dwBatteryFullLifeTime);
                        TRACE(L"BackupBatteryLifeTime = %d\n", ppbpi->dwBackupBatteryLifeTime);
                        TRACE(L"BackupBatteryFullLifeTime = %d\n", ppbpi->dwBackupBatteryFullLifeTime);
                        TRACE(L"ACLineStatus = %d\n",ppbpi->bACLineStatus);
                        TRACE(L"BatteryFlag = %d\n",ppbpi->bBatteryFlag);
                        TRACE(L"BatteryLifePercent = %d\n", ppbpi->bBatteryLifePercent);
                        TRACE(L"BackupBatteryFlag = %d\n", ppbpi->bBackupBatteryFlag);
                        TRACE(L"BackupBatteryLifePercent = %d\n", ppbpi->bBackupBatteryLifePercent);
						*/
                    }
                    break;
                }

                default:
                    break;
            }

            _UpdatePowerState();
        }

        delete ppb;
	}

    if(hPowerNotifications)
        StopPowerNotifications(hPowerNotifications);

    if(hPowerMsgQ)
        CloseMsgQueue(hPowerMsgQ);

	m_evStopped.SetEvent();

	return 0xDEAD;
}

void CPowerMonCtrl::_UpdatePowerState() 
{
    TCHAR szState[MAX_PATH];
    DWORD dwState;

	if(ERROR_SUCCESS == GetSystemPowerState(szState, MAX_PATH, &dwState)) 
    {
		m_sStatus = szState;
		Invalidate();
		UpdateWindow();
    }
}

BOOL CPowerMonCtrl::OnEraseBkgnd(CDC *pDC)
{
	return TRUE;
}

void CPowerMonCtrl::OnPaint()
{
	CPaintDC dc(this);

	COLORREF crBack   = RGB(255, 255, 255);
	COLORREF crBorder = RGB(100, 100, 100);

	if(m_nPerc <= m_nPercAlert)
		crBorder = RGB(255, 0, 0);

	CRect r;
	GetClientRect(&r);
	dc.FillSolidRect(r, crBack);

	CPen pen[2];
	pen[0].CreatePen(PS_SOLID, 1, crBorder);
	pen[1].CreatePen(PS_SOLID, 1, crBack);

	// Desenha a ponta da bateria ...
	int a = (r.Height() - BAT_FRONT_HEIGHT) / 2;

	CPen *pOldPen = dc.SelectObject(&pen[0]);
	dc.MoveTo(0, a);
	dc.LineTo(0, a+BAT_FRONT_HEIGHT);
	dc.LineTo(BAT_FRONT_WIDTH, a+BAT_FRONT_HEIGHT);
	dc.MoveTo(0, a);
	dc.LineTo(BAT_FRONT_WIDTH, a);
	dc.SelectObject(pOldPen);

	// Desenha o corpo da bateria ...
	r.left += BAT_FRONT_WIDTH;
	dc.Draw3dRect(r, crBorder, crBorder);

	// Apaga um pedaco do corpo na juncao com a ponta ...
	pOldPen = dc.SelectObject(&pen[1]);
	dc.MoveTo(BAT_FRONT_WIDTH, a+1);
	dc.LineTo(BAT_FRONT_WIDTH, a+BAT_FRONT_HEIGHT);
	dc.SelectObject(pOldPen);

	// Calcular os retangulos ...
	CRect rcText(r.left  + 1, 
				 r.top, 
				 r.Width() + BAT_FRONT_WIDTH, 
				 r.Height() - STATUS_HEIGHT);

	CRect rcImage(r.left + 2, 
				  r.Height() - STATUS_HEIGHT, 
				  r.Width() + BAT_FRONT_WIDTH - 2, 
				  r.Height() - 2);

	CString s;
	s.Format(_T("%d %%"), m_nPerc);

	CFont *pOldFont = dc.SelectObject(&m_font);

	dc.SetBkMode(TRANSPARENT);
	dc.DrawText(s, rcText, DT_SINGLELINE|DT_VCENTER|DT_CENTER);

	dc.SelectObject(pOldFont);

	if(m_nPerc > m_nPercAlert)
		dc.FillSolidRect(rcImage, RGB(0,255,0));
	else
		dc.FillSolidRect(rcImage, RGB(255,0,0));
}
#endif
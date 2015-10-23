#include "StdAfx.h"
#ifdef _WIN32_WCE
#include "GPSDataWnd.h"
#include "Utils.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CGPSDataWnd, CWnd)

CGPSDataWnd::CGPSDataWnd(void)
{	
	m_fnt1.CreatePointFont(70, L"Tahoma");
	m_fnt2.CreatePointFont(100, L"Tahoma Bold");
	m_fnt3.CreatePointFont(210, L"Tahoma Bold");
	m_bHasGPS = TRUE;
}

CGPSDataWnd::~CGPSDataWnd(void)
{
}

BEGIN_MESSAGE_MAP(CGPSDataWnd, CWnd)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_TIMER()
END_MESSAGE_MAP()

void CGPSDataWnd::OnTimer(UINT_PTR nIDEvent)
{	
	if(nIDEvent == 1)
	{
		RedrawWindow();		
	}
}	


BOOL CGPSDataWnd::Create(CWnd *pParentWnd)
{
	BOOL b = CreateEx(WS_EX_TOPMOST,
				      AfxRegisterWndClass(0), 
					  NULL, 
					  WS_POPUP, 
					  CRect(0, 0, GetSystemMetrics(SM_CXFULLSCREEN), GetSystemMetrics(SM_CYFULLSCREEN)+DRA::SCALEY(26)),
					  pParentWnd, 
					  0x556);	

	if(b)
	{
		SetWindowPos(NULL, 
					 0,
					 0,
					 GetSystemMetrics(SM_CXFULLSCREEN), 
					 GetSystemMetrics(SM_CYFULLSCREEN) + DRA::SCALEY(26), 				 
					 SWP_SHOWWINDOW|SWP_NOZORDER|SWP_FRAMECHANGED);

	
		
		
		SetCapture();
		Invalidate();
		UpdateWindow();

		SetTimer(1, 5000, NULL);

		return TRUE;
	}

	return FALSE;
}

void CGPSDataWnd::Destroy()
{
	ReleaseCapture();
	if(IsWindow(GetSafeHwnd()))
		DestroyWindow();
}

void CGPSDataWnd::Hide()
{
	if(IsWindow(GetSafeHwnd()))
	{
		ReleaseCapture();
		ShowWindow(SW_HIDE);
	}
}

void CGPSDataWnd::UpdateStructGPSData(sGPSInfo m_sGPSInfo)
{
	m_sGPStruct = m_sGPSInfo;
}

BOOL CGPSDataWnd::OnEraseBkgnd(CDC* pDC)
{
	CRect rect;
	GetClientRect(&rect);
	pDC->FillSolidRect(rect, RGB(255, 255, 255));
	/*pDC->Draw3dRect(rect, RGB(0,0,0), RGB(0,0,0));*/
	return TRUE;
}

void CGPSDataWnd::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	CFont *pOldFont;
	CBrush *pOldBrush;
	CPen *pOldPen;

	COLORREF ColorRed(RGB(255,0,0));
	COLORREF ColorGreen(RGB(0,128,0));

	CBrush redBrush;
	CBrush greenBrush;

	CPen transpPen;

	redBrush.CreateSolidBrush(ColorRed);
	greenBrush.CreateSolidBrush(ColorGreen);

	transpPen.CreatePen(PS_NULL, 0, RGB(255,255,255));

	CString sValue;
	
	/**** Data e Hora ****/
	GetLocalTime(&st);
	m_sDataHora.Format(L"%02d/%02d/%4d %02d:%02d:%02d", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);

	pOldFont = dc.SelectObject(&m_fnt2);
	dc.Rectangle(DRA::SCALEX(0),DRA::SCALEY(0),DRA::SCALEX(240),DRA::SCALEY(26));
	dc.DrawText(m_sDataHora, CRect(DRA::SCALEX(0),DRA::SCALEY(0),DRA::SCALEX(240),DRA::SCALEY(26)), DT_VCENTER | DT_CENTER);
	/*********************/
	if(m_bHasGPS)
	{
		/**** Informações do GPS ****/
		dc.Rectangle(DRA::SCALEX(0),DRA::SCALEY(26),DRA::SCALEX(240),DRA::SCALEY(48));
		dc.DrawText(L"Informações do GPS", CRect(DRA::SCALEX(0),DRA::SCALEY(26),DRA::SCALEX(240),DRA::SCALEY(48)), DT_VCENTER | DT_CENTER);
		/***************************/

		/**** Quadros Satelites / Qual. Sinal / Ultim. Coord. ****/
		int nSats = m_sGPStruct.sSatelliteCount;
		nSats = nSats > 12 ? 0 : nSats;
		if(nSats > 2)
		{
			pOldBrush = dc.SelectObject(&greenBrush);
			sValue = L"Boa";		
		}
		else
		{
			pOldBrush = dc.SelectObject(&redBrush);
			sValue = L"Ruim";		
		}

		CString sSatCount;
		sSatCount.Format(L"%d", nSats);
		
		pOldPen = dc.SelectObject(&transpPen);
		dc.Rectangle(DRA::SCALEX(0),DRA::SCALEY(48),DRA::SCALEX(80),DRA::SCALEY(120));
		dc.Rectangle(DRA::SCALEX(80),DRA::SCALEY(48),DRA::SCALEX(160),DRA::SCALEY(120));
		dc.SelectObject(pOldBrush);
		dc.Rectangle(DRA::SCALEX(160),DRA::SCALEY(48),DRA::SCALEX(240),DRA::SCALEY(120));
		
		dc.SelectObject(&m_fnt1);
		//pOldBrush = dc.SelectObject(&redBrush);
		dc.SetBkMode(TRANSPARENT);
		dc.SetTextColor(RGB(255,255,255));		

		dc.DrawText(L"Nº satélites:", CRect(DRA::SCALEX(0),DRA::SCALEY(51),DRA::SCALEX(80),DRA::SCALEY(60)), DT_VCENTER | DT_CENTER);
		dc.DrawText(L"Qualidade Sinal:", CRect(DRA::SCALEX(80),DRA::SCALEY(51),DRA::SCALEX(160),DRA::SCALEY(60)), DT_VCENTER | DT_CENTER);
		dc.SetTextColor(RGB(0,0,0));
		dc.DrawText(L"Última coord a:", CRect(DRA::SCALEX(160),DRA::SCALEY(51),DRA::SCALEX(240),DRA::SCALEY(60)), DT_VCENTER | DT_CENTER);

		dc.SelectObject(&m_fnt3);
		dc.SetTextColor(RGB(255,255,255));
		dc.DrawText(sSatCount, CRect(DRA::SCALEX(0), DRA::SCALEY(61), DRA::SCALEX(80), DRA::SCALEY(120)), DT_VCENTER | DT_CENTER);
		dc.DrawText(sValue, CRect(DRA::SCALEX(80), DRA::SCALEY(61), DRA::SCALEX(160), DRA::SCALEY(120)), DT_VCENTER | DT_CENTER);
		dc.SetTextColor(RGB(0,0,0));
		dc.DrawText(m_sGPStruct.sDateLastJob, CRect(DRA::SCALEX(160), DRA::SCALEY(61), DRA::SCALEX(240), DRA::SCALEY(120)), DT_VCENTER | DT_CENTER);
		dc.SelectObject(pOldBrush);
		dc.SelectObject(pOldPen);
		/********************************************************/


		/**** Informações do Celular ****/
		dc.SelectObject(&m_fnt2);
		dc.Rectangle(DRA::SCALEX(0),DRA::SCALEY(120),DRA::SCALEX(240),DRA::SCALEY(142));
		dc.DrawText(L"Informações do Celular", CRect(DRA::SCALEX(0),DRA::SCALEY(120),DRA::SCALEX(240),DRA::SCALEY(142)), DT_VCENTER | DT_CENTER);
		/*******************************/

		/**** Quadros Status Conexão / Nivel do Sinal ****/
		if(CUtil::OnLine())
		{
			dc.SelectObject(&greenBrush);
			sValue = L"On-Line";
		}
		else
		{
			dc.SelectObject(&redBrush);
			sValue = L"Off-Line";
		}

		dc.SelectObject(&transpPen);
		dc.Rectangle(DRA::SCALEX(0),DRA::SCALEY(142),DRA::SCALEX(100),DRA::SCALEY(214));
		dc.Rectangle(DRA::SCALEX(100),DRA::SCALEY(142),DRA::SCALEX(240),DRA::SCALEY(214));

		dc.SelectObject(&m_fnt1);
		dc.SetTextColor(RGB(255,255,255));
		dc.DrawText(L"Status da conexão:", CRect(DRA::SCALEX(0), DRA::SCALEY(145), DRA::SCALEX(120), DRA::SCALEY(154)), DT_VCENTER | DT_CENTER);
		dc.DrawText(L"Nível de sinal:", CRect(DRA::SCALEX(120), DRA::SCALEY(145), DRA::SCALEX(240), DRA::SCALEY(154)), DT_VCENTER | DT_CENTER);		

		dc.SelectObject(&m_fnt3);
		dc.DrawText(sValue, CRect(DRA::SCALEX(0),DRA::SCALEY(155),DRA::SCALEX(120),DRA::SCALEY(214)), DT_VCENTER | DT_CENTER);
		dc.DrawText(CUtil::GetQualidadeSinal() + L" %", CRect(DRA::SCALEX(120),DRA::SCALEY(155),DRA::SCALEX(240),DRA::SCALEY(214)), DT_VCENTER | DT_CENTER);	

		dc.SetTextColor(RGB(0,0,0));
		dc.SelectObject(pOldBrush);
		/************************************************/

		/**** Quadros Ultimo Envio / Jobs Pendentes ****/
		dc.Rectangle(DRA::SCALEX(0),DRA::SCALEY(214),DRA::SCALEX(80),DRA::SCALEY(286));
		dc.Rectangle(DRA::SCALEX(80),DRA::SCALEY(214),DRA::SCALEX(240),DRA::SCALEY(286));

		dc.SelectObject(&m_fnt1);
		dc.DrawText(L"Último envio a:", CRect(DRA::SCALEX(0), DRA::SCALEY(217), DRA::SCALEX(150), DRA::SCALEY(226)), DT_VCENTER | DT_CENTER);
		dc.DrawText(L"Jobs Pendentes:", CRect(DRA::SCALEX(150), DRA::SCALEY(217), DRA::SCALEX(240), DRA::SCALEY(226)), DT_VCENTER | DT_CENTER);

		dc.SelectObject(&m_fnt3);
		dc.DrawText(m_sGPStruct.sDateLastTX, CRect(DRA::SCALEX(0), DRA::SCALEY(227), DRA::SCALEX(150), DRA::SCALEY(286)), DT_VCENTER | DT_CENTER | DT_WORDBREAK);
		dc.DrawText(m_sGPStruct.sNumJobsPendentes, CRect(DRA::SCALEX(150), DRA::SCALEY(227), DRA::SCALEX(240), DRA::SCALEY(286)), DT_VCENTER | DT_CENTER);

		dc.SelectObject(pOldFont);
		dc.SelectObject(pOldBrush);
		dc.SelectObject(pOldPen);
		/*********************************************/
	}
	else
	{
		/**** Informações do GPS ****/
		dc.Rectangle(DRA::SCALEX(0),DRA::SCALEY(26),DRA::SCALEX(240),DRA::SCALEY(48));
		dc.DrawText(L"Informações da Conexão", CRect(DRA::SCALEX(0),DRA::SCALEY(26),DRA::SCALEX(240),DRA::SCALEY(48)), DT_VCENTER | DT_CENTER);
		/***************************/

		/**** Quadros Satelites / Qual. Sinal / Ultim. Coord. ****/
		int nSats = m_sGPStruct.sSatelliteCount;
		nSats = nSats > 12 ? 0 : nSats;
		if(CUtil::OnLine())
		{
			pOldBrush = dc.SelectObject(&greenBrush);
			sValue = L"On-Line";		
		}
		else
		{
			pOldBrush = dc.SelectObject(&redBrush);
			sValue = L"Off-Line";		
		}

		CString sSatCount;
		sSatCount.Format(L"%d", nSats);
		
		pOldPen = dc.SelectObject(&transpPen);
		dc.Rectangle(DRA::SCALEX(0),DRA::SCALEY(48),DRA::SCALEX(240),DRA::SCALEY(120));

		dc.SelectObject(&m_fnt1);
		//pOldBrush = dc.SelectObject(&redBrush);
		dc.SetBkMode(TRANSPARENT);
		dc.SetTextColor(RGB(255,255,255));		

		dc.DrawText(L"Status da Conexão", CRect(DRA::SCALEX(0),DRA::SCALEY(51),DRA::SCALEX(240),DRA::SCALEY(60)), DT_VCENTER | DT_CENTER);
		dc.SelectObject(&m_fnt3);
		dc.DrawText(sValue, CRect(DRA::SCALEX(0), DRA::SCALEY(61), DRA::SCALEX(240), DRA::SCALEY(120)), DT_VCENTER | DT_CENTER);
		dc.SelectObject(pOldBrush);
		dc.SelectObject(pOldPen);
		/********************************************************/
	}


}

void CGPSDataWnd::OnLButtonDown(UINT nFlags, CPoint point)
{	
	KillTimer(1);
	Destroy();	
	CWnd::OnLButtonDown(nFlags, point);	
}
#endif
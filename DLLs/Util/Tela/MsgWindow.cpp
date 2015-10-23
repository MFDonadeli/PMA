// Copyright (c) 2007 Marcos Mori de Siqueira. All Rights Reserved. 
// mori@softfactory.com.br
// Classe que representa a janela de status da aplicacao, suportando texto
// e progressbar
#include "stdafx.h"
#include "MsgWindow.h"
#include "Utils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CMsgWindow, CWnd)

CMsgWindow* CMsgWindow::m_pInstance = NULL;

CMsgWindow::CMsgWindow()
{
	m_bNotSimpleWnd = FALSE;
}

CMsgWindow::~CMsgWindow()
{

}

BEGIN_MESSAGE_MAP(CMsgWindow, CWnd)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

BOOL CMsgWindow::Create(CWnd *pParentWnd)
{
#ifdef _WIN32_WCE

	if(IsWindow(GetSafeHwnd()))
		return TRUE;

	BOOL b = CreateEx(WS_EX_TOPMOST,
				      AfxRegisterWndClass(0), 
					  NULL, 
					  WS_POPUP, 
					  CRect(0, 0, DRA::SCALEX(160), DRA::SCALEY(40)),
					  pParentWnd, 
					  0x556);
	if(b)
	{
		CRect r(DRA::SCALEX(1), DRA::SCALEY(30), DRA::SCALEX(159), DRA::SCALEY(39));
		b = m_progress.Create(WS_CHILD|PBS_SMOOTH, r, this, 0xABD);
	}

	return b;
#endif
//#else
//	BOOL b = CreateEx(WS_EX_TOPMOST,
//				      AfxRegisterWndClass(0), 
//					  NULL, 
//					  WS_POPUP, 
//					  CRect(0, 0, 160, 40),
//					  pParentWnd, 
//					  0x556);
//	if(b)
//	{
//		CRect r(1, 30, 159, 39);
//		b = m_progress.Create(WS_CHILD|PBS_SMOOTH, r, this, 0xABD);
//	}
//
//	return b;
//#endif
	return TRUE;
}

void CMsgWindow::Show()
{
#ifdef _WIN32_WCE
	SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_SHOWWINDOW);
	//SetCapture();
	Invalidate();
	UpdateWindow();
#endif
}

void CMsgWindow::Show(LPCTSTR szText)
{
#ifdef _WIN32_WCE
	m_sText = szText;	

	m_bNotSimpleWnd = FALSE;

	if(!IsWindow(GetSafeHwnd()))
	{
		if(!Create(NULL))
		{
			STLOG_WRITE("%s(%d): Falha criando janela de mensagem", __FUNCTION__, __LINE__);
			return;
		}
	}

	CRect r;		
	r.left   = DRA::SCALEX(0);
	r.top    = DRA::SCALEY(0);
	r.right  = DRA::SCALEX(160);
	r.bottom = DRA::SCALEY(40);	
	
	int top =  (GetSystemMetrics(SM_CYFULLSCREEN) - r.Height()) / 2;
	int left = (GetSystemMetrics(SM_CXFULLSCREEN) - r.Width())  / 2;

	SetWindowPos(&wndTopMost, 
				 left, 
				 top, 
				 r.Width(), 
				 r.Height(), 
				 SWP_SHOWWINDOW|SWP_NOZORDER|SWP_FRAMECHANGED);

//	//SetCapture();
	Invalidate();
	UpdateWindow();
#endif
}

void CMsgWindow::ShowExpand(LPCTSTR szText)
{
	m_bNotSimpleWnd = TRUE;

	m_sText = szText;

	if(!IsWindow(GetSafeHwnd()))
		Create(NULL);

	CRect r;		
	r.left   = DRA::SCALEX(0);
	r.top    = DRA::SCALEY(0);
	r.right  = DRA::SCALEX(160);
	r.bottom = DRA::SCALEY(40);	

	CSize size = GetDC()->GetTextExtent(szText, wcslen(szText));
	int tam = size.cx / r.right;

	r.bottom = size.cy * (tam + 1);

	int top =  (GetSystemMetrics(SM_CYFULLSCREEN) - r.Height()) / 2;
	int left = (GetSystemMetrics(SM_CXFULLSCREEN) - r.Width())  / 2;

	SetWindowPos(NULL, 
				 left, 
				 top, 
				 r.Width(), 
				 r.Height(), 
				 SWP_SHOWWINDOW|SWP_NOZORDER|SWP_FRAMECHANGED);

	//SetCapture();
	Invalidate();
	UpdateWindow();

}

void CMsgWindow::Show(LPCTSTR szText, CRect r)
{
#ifdef _WIN32_WCE
	m_sText = szText;

	m_bNotSimpleWnd = TRUE;

	if(!IsWindow(GetSafeHwnd()))
		Create(NULL);

	//btClose.Create(L"X", WS_VISIBLE, CRect(r.Width()-DRA::SCALEX(15), 0, r.Height()-DRA::SCALEY(15), 0), this, 0x123);

	int top =  (GetSystemMetrics(SM_CYFULLSCREEN) - r.Height()) / 2;
	int left = (GetSystemMetrics(SM_CXFULLSCREEN) - r.Width())  / 2;

	SetWindowPos(NULL, 
				 left, 
				 top, 
				 r.Width(), 
				 r.Height(), 
				 SWP_SHOWWINDOW|SWP_NOZORDER|SWP_FRAMECHANGED);

	//SetCapture();
	Invalidate();
	UpdateWindow();
#endif
}

void CMsgWindow::Destroy()
{
#ifdef _WIN32_WCE
	//ReleaseCapture();
	Hide();
	/*if(IsWindow(GetSafeHwnd()))
		DestroyWindow();*/
#endif
}

void CMsgWindow::Hide()
{
#ifdef _WIN32_WCE
	if(IsWindow(GetSafeHwnd()))
	{
		//ReleaseCapture();
		ShowWindow(SW_HIDE);
	}
#endif
}

BOOL CMsgWindow::OnEraseBkgnd(CDC* pDC)
{
#ifdef _WIN32_WCE	
	CRect rect;
	GetClientRect(&rect);
	pDC->FillSolidRect(rect, RGB(240, 240, 240));
	pDC->Draw3dRect(rect, RGB(0,0,0), RGB(0,0,0));
#endif
	return TRUE;

}

void CMsgWindow::OnPaint()
{
#ifdef _WIN32_WCE
	CPaintDC dc(this);

	CRect r;
	GetClientRect(&r);

	// Excluir a borda...
	r.DeflateRect(1,1);

	// descontar o progress...
	if(m_progress.IsWindowVisible())
	{
		CRect r(DRA::SCALEX(1), DRA::SCALEY(30), DRA::SCALEX(159), DRA::SCALEY(39));
		dc.ExcludeClipRect(r);
		r.bottom -= 10;
	}

	// Desenhar o texto...
	dc.SetBkMode(TRANSPARENT);
	dc.SetTextColor(RGB(0,0,0));
	CFont ft;
	CUtil::CreateFont(&ft, _T("MS Sans Serif"), -11, FALSE);
	CFont *pOldFont = dc.SelectObject(&ft);
	
	if(m_bNotSimpleWnd == TRUE)
		dc.DrawText(m_sText, r, DT_LEFT | DT_WORDBREAK);		
	else
		dc.DrawText(m_sText, r, DT_SINGLELINE|DT_VCENTER|DT_CENTER); //Só msg window simples		

	dc.SelectObject(pOldFont);
#endif
}

void CMsgWindow::UpdateText(LPCTSTR szText)
{
#ifdef _WIN32_WCE
	m_sText = szText;
	Invalidate();
	UpdateWindow();
#endif
}

void CMsgWindow::SetMax(int max)
{
#ifdef _WIN32_WCE
	m_progress.SetRange(0, max);
	m_progress.SetStep(1);
#endif
}

void CMsgWindow::StepIt()
{
#ifdef _WIN32_WCE
	m_progress.StepIt();
#endif
}

void CMsgWindow::Reset()
{
#ifdef _WIN32_WCE
	m_progress.SetPos(0);
#endif
}

void CMsgWindow::ShowProgress(BOOL bVisible)
{
#ifdef _WIN32_WCE
	m_progress.ShowWindow(bVisible ? SW_SHOW : SW_HIDE);

	if(bVisible)
	{
		Invalidate();
		UpdateWindow();
	}
#endif
}



void CMsgWindow::OnLButtonDown(UINT nFlags, CPoint point)
{	
#ifdef _WIN32_WCE
	Destroy();
	CWnd::OnLButtonDown(nFlags, point);	
#endif
}

BOOL CMsgWindow::DestroyWindow()
{
	// TODO: Add your specialized code here and/or call the base class

	return CWnd::DestroyWindow();
}

CMsgWindow* CMsgWindow::getInstance()
{
	//static CMsgWindow* m_pInstance;
	if(!m_pInstance)
	{
		m_pInstance = new CMsgWindow();
	}

	return m_pInstance;
}
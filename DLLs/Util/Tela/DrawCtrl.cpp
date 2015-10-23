// DrawCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "DrawCtrl.h"
#include "Utils.h"
#include <atlimage.h>

// CDrawCtrl

IMPLEMENT_DYNAMIC(CDrawCtrl, CStatic)

CDrawCtrl::CDrawCtrl()
{
	m_pen.CreatePen(PS_SOLID, 2, RGB(0,0,0)); // solid black
	m_bErase = TRUE;
}

CDrawCtrl::~CDrawCtrl()
{
	_ErasePoints();
}


BEGIN_MESSAGE_MAP(CDrawCtrl, CStatic)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_PAINT()
	ON_WM_RBUTTONUP()
	ON_COMMAND(1000, OnLimpar)
	ON_COMMAND(1001, OnDesfazer)
END_MESSAGE_MAP()



// CDrawCtrl message handlers



void CDrawCtrl::OnLButtonDown(UINT nFlags, CPoint point)
{
#ifdef _WIN32_WCE
	if(SHRecognizeGesture(point))
	{
		OnRButtonUp(nFlags, point);
	}
#endif

	CClientDC dc(this);
	dc.DPtoLP(&point);

	m_pDraw = new CDesenho();
	
	m_pDraw->m_pointArray.Add(point);
	
	SetCapture();
	m_ptPrev = point;

	//CStatic::OnLButtonDown(nFlags, point);
}

void CDrawCtrl::OnLButtonUp(UINT nFlags, CPoint point)
{
	CRect r;
	GetWindowRect(r);
	ScreenToClient(r);

	/*if(!r.PtInRect(point))
	{
		ReleaseCapture();
		return;
	}*/

	CClientDC dc(this);

	dc.DPtoLP(&point);

	CPen* oldPen = dc.SelectObject(&m_pen);
	LOGPEN lp;
	m_pen.GetLogPen(&lp);
	m_pDraw->cr = lp.lopnColor;

	if(r.PtInRect(point))
	{
		dc.MoveTo(m_ptPrev);
		dc.LineTo(point);
		dc.SelectObject(oldPen);
		m_pDraw->m_pointArray.Add(point);
	}

	m_arrayDesenho.Add(m_pDraw);

	ReleaseCapture();

	//CStatic::OnLButtonUp(nFlags, point);
}

void CDrawCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	if(GetCapture() != this)
		return;

	CRect r;
	GetWindowRect(r);
	ScreenToClient(r);

	if(!r.PtInRect(point))
		return;

	CClientDC dc(this);
	dc.DPtoLP(&point);

	m_pDraw->m_pointArray.Add(point);

	CPen* oldPen = dc.SelectObject(&m_pen);
	dc.MoveTo(m_ptPrev);
	dc.LineTo(point);
	dc.SelectObject(oldPen);
	m_ptPrev = point;

	//CStatic::OnMouseMove(nFlags, point);
}

void CDrawCtrl::ChangePen(COLORREF cr)
{
	m_pen.DeleteObject();
	m_pen.CreatePen(PS_SOLID, 2, cr);
}

void CDrawCtrl::_ErasePoints()
{
	for(int i = 0; i<m_arrayDesenho.GetSize(); i++)
	{
		m_pDraw = m_arrayDesenho[i];
		m_pDraw->m_pointArray.RemoveAll();
		delete m_pDraw;
	}

	m_arrayDesenho.RemoveAll();
}

void CDrawCtrl::OnPaint()
{

	CPaintDC dc(this); // device context for painting

	CRect rect;
	GetClientRect(rect);
	dc.FillSolidRect(&rect, RGB(255,255,255));

	if(m_bErase)
	{
		m_bErase = FALSE;
		_ErasePoints();
		return;
	}
	
	if(!m_arrayDesenho.IsEmpty())
	{
		CPen* oldPen = dc.SelectObject(&m_pen);
		for(int i=0; i<m_arrayDesenho.GetSize(); i++)
		{
			m_pDraw = m_arrayDesenho[i];
			dc.MoveTo(m_pDraw->m_pointArray[0]);
			ChangePen(m_pDraw->cr);
			dc.SelectObject(m_pen);
			for(int j=1; j<m_pDraw->m_pointArray.GetSize(); j++)
			{
				dc.LineTo(m_pDraw->m_pointArray[j]);
			}
		}
		dc.SelectObject(oldPen);
	}
	
}

void CDrawCtrl::SaveImage()
{
	CDC* dc;

	dc = GetDC();

	CString sFileName;
	sFileName.Format(L"\\temp\\Draw_%s.bmp", CUtil::GetCurrentTimeStamp());

	CDC memDC;
	memDC.CreateCompatibleDC(dc);

	BYTE *lpBitmapBits = NULL;

	CBitmap bm;
	CRect r;
	GetClientRect(r);

	BITMAPINFO bi;
	ZeroMemory(&bi, sizeof(BITMAPINFO));
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = r.Width();
	bi.bmiHeader.biHeight = r.Height();
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 24;

	HBITMAP bitmap = CreateDIBSection(memDC.GetSafeHdc(), &bi, DIB_RGB_COLORS, (LPVOID*)&lpBitmapBits, NULL, 0);
	HGDIOBJ oldbmp = SelectObject(memDC.GetSafeHdc(), bitmap);

	BitBlt(memDC.GetSafeHdc(), 0, 0, r.Width(), r.Height(), dc->GetSafeHdc(), 0, 0, SRCCOPY);

	BITMAPFILEHEADER bh;
	ZeroMemory(&bh, sizeof(BITMAPFILEHEADER));
	bh.bfType = 0x4d42; //bitmap 
	bh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	bh.bfSize = bh.bfOffBits + ((r.Width()*r.Height())*3);

	CFile file;
	if(file.Open(sFileName, CFile::modeCreate | CFile::modeWrite))
	{ 
		file.Write(&bh, sizeof(BITMAPFILEHEADER));
		file.Write(&(bi.bmiHeader), sizeof(BITMAPINFOHEADER));
		file.Write(lpBitmapBits, 3 * r.Width() * r.Height());
		file.Close();
	}

	SelectObject(memDC.GetSafeHdc(), oldbmp);
	DeleteObject(bitmap);
	memDC.DeleteDC();
	dc->DeleteDC();
#ifndef _WIN32_WCE

	CImage img;
	img.Attach((HBITMAP)bm.m_hObject);
	img.Save(L"c:\\temp\\teste.jpg", Gdiplus::ImageFormatJPEG);

#endif

}
void CDrawCtrl::OnRButtonUp(UINT nFlags, CPoint point)
{
	CMenu popupMenu;

	popupMenu.CreatePopupMenu();
	popupMenu.AppendMenu(MF_STRING, 1000, _T("Limpar"));
	popupMenu.AppendMenu(MF_STRING, 1001, _T("Desfazer"));

	ClientToScreen(&point);
	popupMenu.TrackPopupMenu(0, point.x, point.y, this, NULL);
}

void CDrawCtrl::OnLimpar()
{
	m_bErase = TRUE;
	Invalidate();
}

void CDrawCtrl::OnDesfazer()
{
	int a = m_arrayDesenho.GetUpperBound();

	m_arrayDesenho.RemoveAt(a);
	Invalidate();
}
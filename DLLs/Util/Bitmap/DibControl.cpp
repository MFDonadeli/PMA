#include "stdafx.h"
#ifdef _WIN32_WCE

#include "DibControl.h"
#include "Utils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDibControl
CDibControl::CDibControl()
{
	m_bStretch = FALSE;
	m_color = RGB(0,0,128);
	m_nPos = 5;
}

CDibControl::~CDibControl()
{
}

BEGIN_MESSAGE_MAP(CDibControl, CWnd)
	//{{AFX_MSG_MAP(CDibControl)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CDibControl message handlers

void CDibControl::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

    if(m_DIBSection.GetSafeHandle())
	{
		CRect rect;
        GetClientRect(rect);

		if(!m_bStretch)
		{
			int top = abs((rect.Height() - m_DIBSection.GetHeight()) / 2);
			int left = abs((rect.Width() - m_DIBSection.GetWidth()) / 2);

			m_DIBSection.Draw(&dc, CPoint(left, top));
		}
		else
		{
            m_DIBSection.Stretch(&dc, CPoint(0,0), rect.Size());
		}

		if(!m_sText.IsEmpty())
		{
			dc.SetTextColor(m_color);
			rect.left += m_nPos;
			dc.SetBkMode(TRANSPARENT);
			CFont *pOldFont = dc.SelectObject(&m_font);
			dc.DrawText(m_sText, m_sText.GetLength(), rect, DT_SINGLELINE|DT_VCENTER);
			dc.SelectObject(pOldFont);
		}
	}
}

BOOL CDibControl::OnEraseBkgnd(CDC* pDC) 
{
    if(m_bStretch)
        return TRUE;

	HBRUSH hBrush = ::CreateSolidBrush(RGB(255,255,255));
    CRect ClientRect, ImageRect;
    GetClientRect(ClientRect);
	
	int top  = abs((ClientRect.Height() - m_DIBSection.GetHeight()) / 2);
	int left = abs((ClientRect.Width() - m_DIBSection.GetWidth()) / 2);

    ImageRect.SetRect(left,
					  top,
					  left + m_DIBSection.GetWidth(), 
					  top + m_DIBSection.GetHeight());

    ::FillRect(pDC->GetSafeHdc(), 
               CRect(0,
					 0, 
					 ClientRect.right, 
					 ImageRect.top), 
					 hBrush);

    ::FillRect(pDC->GetSafeHdc(), 
               CRect(0,
					 0, 
					 ImageRect.left, 
					 ClientRect.bottom), 
					 hBrush);

    ::FillRect(pDC->GetSafeHdc(), 
               CRect(ImageRect.right,
					 0, 
					 ClientRect.right, 
					 ImageRect.bottom), 
					 hBrush);

    ::FillRect(pDC->GetSafeHdc(), 
               CRect(0, 
					 ImageRect.bottom, 
					 ClientRect.right, 
					 ClientRect.bottom), 
					 hBrush);

    ::DeleteObject(hBrush);

	return TRUE;
}

BOOL CDibControl::Load(LPCTSTR szFileName)
{
	return m_DIBSection.Load(szFileName);
}

void CDibControl::SetStretch(BOOL b)
{
	m_bStretch = b;
	Invalidate();
}

BOOL CDibControl::Load(UINT nIDRes)
{
	return m_DIBSection.SetBitmap(nIDRes);
}

BOOL CDibControl::Create(const RECT &rect, CWnd *pParentWnd, UINT nID, BOOL bVisible/*=TRUE*/)
{
	if(m_font.GetSafeHandle() == NULL)
		CUtil::CreateFont(&m_font, L"Verdana", -11, TRUE);

	DWORD dwFlags = WS_CHILD;
	if(bVisible) 
		dwFlags |= WS_VISIBLE;

	return CWnd::Create(NULL, NULL, dwFlags, rect, pParentWnd, nID);
}

void CDibControl::SetText(LPCTSTR szText) 
{
	m_sText = szText; 
	Invalidate();
}

BOOL CDibControl::SetBitmap(HBITMAP hBitmap)
{
	return m_DIBSection.SetBitmap(hBitmap);
}


BOOL CDibControl::LoadJPG(LPCTSTR szFile)
{
	HBITMAP hBmp = SHLoadImageFile(szFile);
	if(hBmp != NULL && SetBitmap(hBmp))
	{
		Invalidate();
		return TRUE;
	}

	return FALSE;
}


#endif
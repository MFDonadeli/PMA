#include "stdafx.h"
#include "TabCtrlEx.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

bool CTabCtrlEx::g_bTabCtrlExWndClassRegistered = false;

CTabCtrlEx::CTabCtrlEx()
{
	VERIFY(_RegisterWndClass());
	m_bInitialized = FALSE;
}

CTabCtrlEx::~CTabCtrlEx()
{
}

BEGIN_MESSAGE_MAP(CTabCtrlEx, CTabCtrl)
	//{{AFX_MSG_MAP(CTabCtrlEx)
	ON_WM_ERASEBKGND()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CTabCtrlEx::Create(DWORD dwStyle, const RECT &rect, CWnd *pParentWnd, UINT nID)
{
	if(!_RegisterWndClass())
		return FALSE;

	return /*CTabCtrl*/CWnd::CreateEx(0, 
							  __TABCTRLEX_WND_CLASS_NAME, 
							  NULL, 
							  dwStyle, 
							  rect, 
							  pParentWnd, 
							  nID);
}


BOOL CTabCtrlEx::OnEraseBkgnd(CDC* pDC) 
{
	Default();
	CRect r;
	GetClientRect(&r);
	r.bottom -= 20;
	pDC->FillSolidRect(&r, RGB(255,255,255));
	return TRUE;
}

void CTabCtrlEx::SetWCELookAndFeel() 
{
	if(!m_bInitialized)
	{
		m_bInitialized = TRUE;
		SendMessage(CCM_SETVERSION, COMCTL32_VERSION, 0);
	}
}

bool CTabCtrlEx::_RegisterWndClass()
{
	if(g_bTabCtrlExWndClassRegistered)
		return true;

	WNDCLASS _wndClassInfo;
	HINSTANCE hInst = AfxGetInstanceHandle();

	if(!::GetClassInfo(hInst, __TABCTRLEX_WND_CLASS_NAME, &_wndClassInfo))
	{
		if(!::GetClassInfo(hInst, _T("SysTabControl32"), &_wndClassInfo))
			return FALSE;

		_wndClassInfo.lpszClassName = __TABCTRLEX_WND_CLASS_NAME;

		if(!::AfxRegisterClass(&_wndClassInfo))
		{
			ASSERT(FALSE);
			return false;
		}
	}

	g_bTabCtrlExWndClassRegistered = true;

	return true;
}


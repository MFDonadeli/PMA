// DlgDraw.cpp : implementation file
//

#include "stdafx.h"
#include "Data_Hora.h"
#include "DlgDraw.h"
#include "ModParam.h"


// CDlgDraw dialog

IMPLEMENT_DYNAMIC(CDlgDraw, CDialogEx)

CDlgDraw::CDlgDraw(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDlgDraw::IDD, pParent)
{

}

CDlgDraw::~CDlgDraw()
{
}

void CDlgDraw::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CDlgDraw, CDialog)
	ON_COMMAND(ID_MENU_SALVAR, OnSalvar)
END_MESSAGE_MAP()


// CDlgDraw message handlers

BOOL CDlgDraw::OnInitDialog()
{
	CDialogEx::OnInitDialog();

#ifdef _WIN32_WCE
	if (!m_dlgCommandBar.Create(this) ||
	    !m_dlgCommandBar.InsertMenuBar(IDR_MENU3))
	{
		STLOG_WRITE("CDlgDraw::OnInitDialog() : Failed to create CommandBar");
		return FALSE;      // fail to create
	}

	_CreateBanner(L"Assinatura");	
#endif

	m_drwCtrl = new CDrawCtrl();
	m_drwCtrl->Create(L"", SS_NOTIFY | WS_BORDER, 
		CRect(DRA::SCALEX(5), DRA::SCALEY(5), DRA::SCALEX(200), DRA::SCALEY(300)), this, 1050);
	m_drwCtrl->ShowWindow(SW_SHOW);

	_FullScreen();

	if(GetParent() != NULL)
		GetParent()->PostMessage(CModParam::WM_MODULE_READY, 0, (LPARAM) GetSafeHwnd());

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CDlgDraw::OnSalvar()
{
	m_drwCtrl->SaveImage();
}
// GetPinDlg.cpp : implementation file
//

#include "stdafx.h"
#include "GetPinDlg.h"
#include "ExtDllState.h"


// CGetPinDlg dialog

IMPLEMENT_DYNAMIC(CGetPinDlg, CDialog)

CGetPinDlg::CGetPinDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGetPinDlg::IDD, pParent)
	, sPin(_T(""))
	, sPinConfirm(_T(""))
{

}

CGetPinDlg::~CGetPinDlg()
{
}

void CGetPinDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_PIN, edt_Pin);
	DDX_Control(pDX, IDC_EDIT_PIN_CONF, edt_Pin_Confirm);
	DDX_Text(pDX, IDC_EDIT_PIN, sPin);
	DDV_MaxChars(pDX, sPin, 16);
	DDX_Text(pDX, IDC_EDIT_PIN_CONF, sPinConfirm);
	DDV_MaxChars(pDX, sPinConfirm, 16);
}


BEGIN_MESSAGE_MAP(CGetPinDlg, CDialog)
	ON_BN_CLICKED(IDC_BUTTON1, &CGetPinDlg::OnBnClickedButton1)
	ON_WM_SETFOCUS()
END_MESSAGE_MAP()


// CGetPinDlg message handlers

void CGetPinDlg::OnBnClickedButton1()
{
	UpdateData();
	if(sPin.IsEmpty() || sPinConfirm.IsEmpty())
	{
		MessageBox(L"Todos os campos são de preenchimento obrigatório", L"Erro", MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	if (!sPin.Compare(sPinConfirm)==0)
	{
		MessageBox(L"PIN e a confirmação estão diferentes. Digite Novamente.!", L"Erro", MB_OK | MB_ICONEXCLAMATION);
		///EndDialog( IDCANCEL );
		return;
	}
	//szPIN = sPin;
	HideSIP();
	EndDialog( IDOK );
}

void CGetPinDlg::OnSetFocus(CWnd* pOldWnd)
{
	CDialog::OnSetFocus(pOldWnd);

	edt_Pin.SetFocus();
}

BOOL CGetPinDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	ShowSIP();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CGetPinDlg::HideSIP()
{
	//m_bSipVisible = FALSE;
	SHSipPreference((this->GetParent())->GetSafeHwnd(), SIP_FORCEDOWN);

}

void CGetPinDlg::ShowSIP()
{
	//m_bSipVisible = TRUE;
	SHSipPreference((this->GetParent())->GetSafeHwnd(), SIP_UP);
}


INT_PTR CGetPinDlg::DoModal()	
{
	CEXTDLLState State;

	return CDialog::DoModal();
}
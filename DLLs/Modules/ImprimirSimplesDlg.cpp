// Dlg.cpp : implementation file
//

#include "stdafx.h"
#include "ImprimirSimplesDlg.h"
#include "ExtDllState.h"
#include "ImprimirDlg.h"
#include "BlueTooth.h"
#include "BlueToothSearchDlg.h"
#include "GetPinDlg.h"
#include "ModParam.h"

// CDlg dialog

IMPLEMENT_DYNAMIC(CImprimirSimplesDlg, CDialog)

CImprimirSimplesDlg::CImprimirSimplesDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CImprimirSimplesDlg::IDD, pParent)
{

}

CImprimirSimplesDlg::~CImprimirSimplesDlg()
{
}

void CImprimirSimplesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, cmboImpPadrao);
}


BEGIN_MESSAGE_MAP(CImprimirSimplesDlg, CDialog)
	ON_BN_CLICKED(IDC_PAGE_IMP_BTN_PRINT, &CImprimirSimplesDlg::OnBnClickedPageImpBtnPrint)
	ON_BN_CLICKED(IDC_PROSSEGUIR, &CImprimirSimplesDlg::OnBnClickedProsseguir)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON_CONFIGURAR, &CImprimirSimplesDlg::OnBnClickedButtonConfig)
END_MESSAGE_MAP()


// CDlg message handlers

INT_PTR CImprimirSimplesDlg::DoModal()	
{
	CEXTDLLState State;

	return CDialog::DoModal();
}

BOOL CImprimirSimplesDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	ShowPrinters();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CImprimirSimplesDlg::OnBnClickedButtonConfig()
{
	CImprimirDlg dlg(this);
	dlg.DoModal();
	ShowPrinters();
}

void CImprimirSimplesDlg::ShowPrinters()
{
	CBTPrinter print;


	CString m_sPrinterName = print.GetPrinterName();
	CStringList lPrinters;
	
	lPrinters.RemoveAll();
	print.GetPrintersList(lPrinters);

	int idx;
	int idx_Padrao;

	
	cmboImpPadrao.ResetContent();
	POSITION p = lPrinters.GetHeadPosition();
	while(p)
	{
		CString sPname = lPrinters.GetNext(p);
		if (sPname.CompareNoCase(m_sPrinterName) == 0)
		{
			sPname = L"•" + m_sPrinterName;
			idx = cmboImpPadrao.AddString(sPname);
			idx_Padrao = idx;
		}
		else
		{
			sPname = L" " + sPname;
			idx = cmboImpPadrao.AddString(sPname);
		}
		///cmboImpPadrao.SetItemData(idx, 0);
	}

	cmboImpPadrao.SetCurSel(idx_Padrao);
	cmboImpPadrao.UpdateData();
	cmboImpPadrao.UpdateWindow();

}

void CImprimirSimplesDlg::OnBnClickedPageImpBtnPrint()
{
	int idx;
	CString sPrName = L"";
	idx = cmboImpPadrao.GetCurSel();
	if(idx != CB_ERR)
	{
		cmboImpPadrao.GetLBText(idx, sPrName);
		sPrName.TrimLeft(L" ");
		sPrName.TrimLeft(L"•");
	}
	m_params->AddPair(_T("Selected_Printer_Name"), sPrName);
	EndDialog(IDYES);

}

void CImprimirSimplesDlg::OnBnClickedProsseguir()
{
	EndDialog(IDCANCEL);
}
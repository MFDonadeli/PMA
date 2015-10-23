// Dlg.cpp : implementation file
//

#include "stdafx.h"
#include "Dlg.h"
#include "ExtDllState.h"
#include "ImprimirDlg.h"
#include "BlueTooth.h"
#include "BlueToothSearchDlg.h"
#include "GetPinDlg.h"

// CDlg dialog

IMPLEMENT_DYNAMIC(CDlg, CDialogEx)

CDlg::CDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDlg::IDD, pParent)
{
	m_sNomeImpressoraSelecionada = L"";
}

CDlg::~CDlg()
{
}

void CDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, cmboImpPadrao);
}


BEGIN_MESSAGE_MAP(CDlg, CDialogEx)
	ON_BN_CLICKED(IDC_PAGE_IMP_BTN_PRINT, &CDlg::OnBnClickedPageImpBtnPrint)
	ON_BN_CLICKED(IDC_PROSSEGUIR, &CDlg::OnBnClickedProsseguir)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON_CONFIGURAR, &CDlg::OnBnClickedButtonConfig)
END_MESSAGE_MAP()


// CDlg message handlers

INT_PTR CDlg::DoModal()	
{
	CEXTDLLState State;

	return CDialogEx::DoModal();
}

BOOL CDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

#ifdef _WIN32_WCE
	HideSIP();

	if (!m_dlgCommandBar.Create(this)/* ||
	    !m_dlgCommandBar.InsertMenuBar(IDR_MENU3)*/)
	{
		STLOG_WRITE("CAITDlg::OnInitDialog() : Failed to create CommandBar");
		return FALSE;      // fail to create
	}

	_CreateBanner(L"Imprimir");	

	////UINT nStyle = m_dlgCommandBar.GetButtonStyle(0);
	////nStyle |= TBBS_DISABLED; 
	////m_dlgCommandBar.SetButtonStyle(0, nStyle);
#endif

	_FullScreen();

	ShowPrinters();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CDlg::OnBnClickedButtonConfig()
{
	CImprimirDlg dlg(this);
	dlg.DoModal();
	ShowPrinters();
}

void CDlg::ShowPrinters()
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

void CDlg::OnBnClickedPageImpBtnPrint()
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
	else
	{
		//CString msg;
		//msg.LoadString(IDS_IMPRESSORANSELECIONADA);
		MessageBox( L"Selecione uma impressora!.", L"Mensagem", MB_ICONINFORMATION|MB_OK);
		return;
	}
	m_sNomeImpressoraSelecionada = sPrName;
	EndDialog(IDYES);
}

void CDlg::OnBnClickedProsseguir()
{
	EndDialog(IDCANCEL);
}
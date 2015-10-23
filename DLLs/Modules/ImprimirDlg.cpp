// ImprimirDlg.cpp : implementation file
//

#include "stdafx.h"
//#include "AIT.h"
#include "ImprimirDlg.h"
#include "ModParam.h"
#include "Utils.h"
#include "BlueTooth.h"
#include "BlueToothSearchDlg.h"
#include "GetPinDlg.h"
#include "ExtDllState.h"


extern HWND g_hWnd;

// CImprimirDlg dialog

IMPLEMENT_DYNAMIC(CImprimirDlg, CDialogEx)

CImprimirDlg::CImprimirDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CImprimirDlg::IDD, pParent)
{
	m_bTeste = TRUE;
}

CImprimirDlg::~CImprimirDlg()
{
}

void CImprimirDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, cmboImpPadrao);
}


BEGIN_MESSAGE_MAP(CImprimirDlg, CDialogEx)
	ON_BN_CLICKED(IDC_PAGE_IMP_BTN_PRINT, &CImprimirDlg::OnBnClickedPageImpBtnPrint)
	ON_BN_CLICKED(IDC_PROSSEGUIR, &CImprimirDlg::OnBnClickedProsseguir)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_PAGE_IMP_BTN_LOC, &CImprimirDlg::OnBnClickedPageImpBtnLoc)
	ON_BN_CLICKED(IDC_BUTTON_DEFINIR, &CImprimirDlg::OnBnClickedButtonDefinir)
	ON_BN_CLICKED(IDC_BUTTON_EXCLUIR, &CImprimirDlg::OnBnClickedButtonExcluir)
END_MESSAGE_MAP()


// CImprimirDlg message handlers


/**
\brief Captura mensagem do dialog
\details Trata evento KeyDown + VK_RETURN (Enter do teclado) chamando OnOK()
\param void
\return void
*/
BOOL CImprimirDlg::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message == WM_KEYDOWN)
	{
		if(pMsg->wParam == VK_RETURN)
		{
			OnOK();
			return TRUE;
		}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}

void CImprimirDlg::OnBnClickedPageImpBtnPrint()
{
	if (m_bTeste)
	{
		ImprimeTeste();
	}
	else
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
}

void CImprimirDlg::OnBnClickedProsseguir()
{
	EndDialog(IDCANCEL);
}

BOOL CImprimirDlg::OnInitDialog()
{

	///g_hWnd = GetSafeHwnd();

	CDialogEx::OnInitDialog();

	m_bTeste = TRUE;


#ifdef _WIN32_WCE
	HideSIP();

	if (!m_dlgCommandBar.Create(this)/* ||
	    !m_dlgCommandBar.InsertMenuBar(IDR_MENU3)*/)
	{
		STLOG_WRITE("CAITDlg::OnInitDialog() : Failed to create CommandBar");
		return FALSE;      // fail to create
	}

	_CreateBanner(L"Configurar impressora");	

	////UINT nStyle = m_dlgCommandBar.GetButtonStyle(0);
	////nStyle |= TBBS_DISABLED; 
	////m_dlgCommandBar.SetButtonStyle(0, nStyle);
#endif

	_FullScreen();

	/*if(GetParent() != NULL)
		GetParent()->PostMessage(CModParam::WM_MODULE_READY, 0, (LPARAM) GetSafeHwnd());*/

	//ShowPrinterDefault();
	ShowPrinters();

	if (m_bTeste)
	{
		(GetDlgItem(IDC_PAGE_IMP_BTN_PRINT))->SetWindowTextW(L"Imprimir Teste");
		(GetDlgItem(IDC_PROSSEGUIR))->SetWindowTextW(L"Sair");
	}
	else
	{
		(GetDlgItem(IDC_PAGE_IMP_BTN_PRINT))->SetWindowTextW(L"Imprimir Autuação");
		(GetDlgItem(IDC_PROSSEGUIR))->SetWindowTextW(L"Continuar");
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CImprimirDlg::OnTimer(UINT_PTR nIDEvent)
{

	CDialogEx::OnTimer(nIDEvent);
}

void CImprimirDlg::OnBnClickedPageImpBtnLoc()
{

	///localizar impressoras
	CBTPrinter printer;
	CBlueToothSearchDlg search;

	if (search.DoModal() == IDOK)
	{
		//if (!printer.ExistePIN())
		//{
		//	////set PIN

		//	CGetPinDlg gp;
		//	gp.DoModal();
		//	
		//	printer.SavePIN(gp.sPin);

		//}

		ShowPrinters();
	}
}


BOOL CImprimirDlg::ImprimeTeste()
{
	///Imprimir o TESTE

	CBTPrinter m_printer;

	int idx;
	CString sPrName = L"";
	idx = cmboImpPadrao.GetCurSel();
	if(idx != CB_ERR)
	{
		cmboImpPadrao.GetLBText(idx, sPrName);
		sPrName.TrimLeft(L" ");
		sPrName.TrimLeft(L"•");
	}
	if (sPrName.IsEmpty())
	{
		return FALSE;
	}
	m_printer.m_sPrinterName = sPrName;

	///CString s = L"______________________________\r\n\r\n";
	CString s = L"______________________________\r\n";  

	char *init = new char[2];
	init[0]=0x0A;  //avança 1 linha
	init[1]=0x00;

	CString sInit = CUtil::ConvertAnsiToUnicode(init);
	s += sInit + L"\r\n";


	CString sPort = L"";
	int iCom = -1;
	BOOL printerOK = FALSE;
	CString sAddr;
	///BT_ADDR m_ba = m_printer.GetDefaultPrinterAddr(sAddr);
	BT_ADDR m_ba = m_printer.GetSavedPrinterAddr(sPrName);
	CString sPin = m_printer.GetSavedPrinterPin(sPrName);
	m_printer.SavePIN(sPin,sPrName);

	iCom = m_printer.RegisterBTDevice(m_ba);
	if (iCom > -1)
	{
		sPort.Format(L"COM%d:", iCom);
		///m_printer.EraseRegAuthKey(sPrName);
		if (m_printer.Print(this, sPort, s))
		{
			printerOK = TRUE;
		}
		else
		{
			m_printer.EraseRegAuthKey(sPrName);
			if (m_printer.Print(this, sPort, s))
			{
				printerOK = TRUE;
			}

		}
		DeregisterDevice (m_printer.m_hDev);
		m_printer.EraseRegAuthKey(sPrName);
	}
	return printerOK;
}


void CImprimirDlg::ShowPrinterDefault()
{
	CBTPrinter print;



	CString m_sPrinterName = print.GetPrinterName();

	int idx;
	idx = cmboImpPadrao.AddString(L"•" + m_sPrinterName);
	cmboImpPadrao.SetItemData(idx, 0);

	cmboImpPadrao.SetCurSel(0);
	cmboImpPadrao.UpdateData();
	cmboImpPadrao.UpdateWindow();

}

void CImprimirDlg::OnBnClickedButtonDefinir()
{
	CBTPrinter pr;

	cmboImpPadrao.UpdateData();

	int idx;
	CString sPrName = L"";
	idx = cmboImpPadrao.GetCurSel();
	if(idx != CB_ERR)
	{
		cmboImpPadrao.GetLBText(idx, sPrName);
		sPrName.TrimLeft(L" ");
		sPrName.TrimLeft(L"•");
	}
	if (!sPrName.IsEmpty())
	{
		CString pAddress =	pr.GetPrinterAddress(sPrName);
	
		pr.SaveDefaultPrinter(sPrName, pAddress);
	}
	ShowPrinters();
}

void CImprimirDlg::OnBnClickedButtonExcluir()
{
	CBTPrinter print;

	cmboImpPadrao.UpdateData();

	int idx;
	CString sPrName = L"";
	idx = cmboImpPadrao.GetCurSel();
	if(idx != CB_ERR)
	{
		cmboImpPadrao.GetLBText(idx, sPrName);
		sPrName.TrimLeft(L" ");
		sPrName.TrimLeft(L"•");
	}
	if (!sPrName.IsEmpty())
	{
		sPrName.TrimLeft(L"•");
		sPrName.TrimLeft(L" ");
		if (print.ExcluiPrinter(sPrName))
		{
			CString msg;
			msg.LoadString(IDS_IMPRESSORA_EXCLUIDA);
			MessageBox(msg, L"Mensagem", MB_ICONINFORMATION|MB_OK);
		}
		else
		{
			CString msg;
			msg.LoadString(IDS_IMPRESSORA_N_EXCLUIDA);
			MessageBox(msg, L"Mensagem", MB_ICONERROR|MB_OK);
		}
	}
	ShowPrinters();
	
}


void CImprimirDlg::ShowPrinters()
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

INT_PTR CImprimirDlg::DoModal()	
{
	CEXTDLLState State;

	return CDialog::DoModal();
}
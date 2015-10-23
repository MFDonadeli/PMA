// Data_HoraDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Data_Hora.h"
#include "PrintersDlg.h"
#include "Registry.h"
#include "ModParam.h"
#include "Utils.h"
#include "SimpleRequest.h"
#include "CStr.h"
#include "BlueTooth.h"
#ifdef TESTE_IMPRESSORA
#include "GetPinDlg.h"
#endif

// CPrintersDlg dialog

IMPLEMENT_DYNAMIC(CPrintersDlg, CDialogEx)

CPrintersDlg::CPrintersDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CPrintersDlg::IDD, pParent)
{

}

CPrintersDlg::~CPrintersDlg()
{
}

void CPrintersDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_PRINTERS, m_lstPrinters);
}


BEGIN_MESSAGE_MAP(CPrintersDlg, CDialogEx)
	ON_COMMAND(ID_ALTERAR, &CPrintersDlg::OnAlterar)
END_MESSAGE_MAP()


// CPrintersDlg message handlers

BOOL CPrintersDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

#ifdef _WIN32_WCE
	if (!m_dlgCommandBar.Create(this) ||
	    !m_dlgCommandBar.InsertMenuBar(IDR_MENU2))
	{
		STLOG_WRITE("CConsLoteDlg::OnInitDialog() : Failed to create CommandBar");
		return FALSE;      // fail to create
	}

	_CreateBanner(L"Alterar Senha");	
#endif

	_FullScreen();

	m_lstPrinters.InsertColumn(0, L"",LVS_ALIGNLEFT, DRA::SCALEX(10)); 
	m_lstPrinters.InsertColumn(1, L"",LVS_ALIGNLEFT, DRA::SCALEX(100)); 

	m_lstPrinters.SetExtendedStyle(LVS_EX_FULLROWSELECT);

	_FillList();

	if(GetParent() != NULL)
		GetParent()->PostMessage(CModParam::WM_MODULE_READY, 0, (LPARAM) GetSafeHwnd());

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CPrintersDlg::_FillList()
{
	CBTPrinter btPrint;
	CStringArray m_arrPrint;
	CString sDefPrint;

	btPrint.GetPrinters(&m_arrPrint);

	Registry reg(HKEY_CURRENT_USER, BT_REG_KEY_APP);

	if(!reg.Open())
	{
		STLOG_WRITE("%s(%d): Open registry: %s", __FUNCTION__, __LINE__,
					reg.GetLastErrorString());
	}
	else
	{
		if(!reg.GetString(L"DefaultPrinter", sDefPrint))
		{
			STLOG_WRITE("%s(%d): Read Registry: %s", __FUNCTION__, __LINE__,
						reg.GetLastErrorString());
		}

		reg.Close();
	}	

	LVITEM lv;

	int idx;
	for(int i=0; i<m_arrPrint.GetCount(); i++)
	{
		//m_lstPrinters.AddString(m_arrPrint[i]);
		if(m_arrPrint[i].CompareNoCase(sDefPrint)==0)
			idx = m_lstPrinters.InsertItem(m_lstPrinters.GetItemCount(), L"•");
		else
			idx = m_lstPrinters.InsertItem(m_lstPrinters.GetItemCount(), L"");

		m_lstPrinters.SetItemText(idx, 1, m_arrPrint[i]);
	}

}

void CPrintersDlg::OnAlterar()
{	
	CString sPrinter;

	POSITION p = m_lstPrinters.GetFirstSelectedItemPosition();
	int selected = m_lstPrinters.GetNextSelectedItem(p);

	sPrinter = m_lstPrinters.GetItemText(selected, 1);

	if(m_lstPrinters.GetSelectedCount() == 0)
	{
		CString msg;
		msg.LoadString(IDS_IMPRESSORA_PADRAO);

		MessageBox(msg, L"Mensagem", MB_OK | MB_ICONINFORMATION);
		return;
	}

	Registry reg(HKEY_CURRENT_USER, BT_REG_KEY_APP);
	if(!reg.Open())
	{
		STLOG_WRITE("%s(%d): Open registry: %s", __FUNCTION__, __LINE__,
					reg.GetLastErrorString());

		CString msg;
		msg.LoadString(IDS_ERRO_IMPRESSORA_PADRAO);

		MessageBox(msg, L"Erro", MB_OK | MB_ICONERROR);
	}
	else
	{
		if(!reg.SetValue(L"DefaultPrinter", sPrinter))
		{
			STLOG_WRITE("%s(%d): read registry: %s", __FUNCTION__, __LINE__,
						reg.GetLastErrorString());
			
			CString msg;
			msg.LoadString(IDS_ERRO_IMPRESSORA_PADRAO);

			MessageBox(msg, L"Erro", MB_OK | MB_ICONERROR);
		}
		else
		{
#ifdef TESTE_IMPRESSORA
			CBTPrinter pp;
			pp.SaveDefaultPrinterAddr();

			CGetPinDlg gp;
			gp.DoModal();
			
			pp.SavePIN(gp.sPin);

#endif
			CString msg;
			msg.LoadString(IDS_IMPRESSORA_PADRAO_OK);

			MessageBox(msg, L"Mensagem", MB_OK | MB_ICONINFORMATION);
		}

		reg.Close();
	}

	EndDialog(IDOK);
	
}

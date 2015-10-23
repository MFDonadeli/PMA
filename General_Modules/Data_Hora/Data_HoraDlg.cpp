// Data_HoraDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Data_Hora.h"
#include "Data_HoraDlg.h"
#include "CppSQLite3.h"
#include "ModParam.h"
#include "Utils.h"
#include "CStr.h"


// CData_HoraDlg dialog

IMPLEMENT_DYNAMIC(CData_HoraDlg, CDialogEx)

CData_HoraDlg::CData_HoraDlg(CppSQLite3DB *pDB, CWnd* pParent /*=NULL*/)
	: CDialogEx(CData_HoraDlg::IDD, pParent)
{
	m_pDB = pDB;
}

CData_HoraDlg::~CData_HoraDlg()
{
}

void CData_HoraDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PAGE1_DATE, m_dtData);
	DDX_Control(pDX, IDC_PAGE1_TIME, m_tmHora);
	DDX_Control(pDX, IDC_EDT_AGENTE, m_txtMotivo);
}


BEGIN_MESSAGE_MAP(CData_HoraDlg, CDialogEx)
	ON_COMMAND(ID_ALTERAR, &CData_HoraDlg::OnAlterar)
END_MESSAGE_MAP()


// CData_HoraDlg message handlers

BOOL CData_HoraDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

#ifdef _WIN32_WCE
	if (!m_dlgCommandBar.Create(this) ||
	    !m_dlgCommandBar.InsertMenuBar(IDR_MENU2))
	{
		STLOG_WRITE("CConsLoteDlg::OnInitDialog() : Failed to create CommandBar");
		return FALSE;      // fail to create
	}

	_CreateBanner(L"Alterar Data/Hora");	
#endif

	_FullScreen();

	if(GetParent() != NULL)
		GetParent()->PostMessage(CModParam::WM_MODULE_READY, 0, (LPARAM) GetSafeHwnd());

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CData_HoraDlg::OnAlterar()
{
	CString sMsg;
	sMsg.LoadString(IDS_CONFALTERA);
	int nErrorDateDiff;

	CString sValue;
	m_txtMotivo.GetWindowText(sValue);

	if(sValue.Trim().IsEmpty())
	{
		MessageBox(L"A inseração do motivo é obrigatória", L"Atenção", MB_ICONINFORMATION);
		return;
	}

	if(MessageBox(sMsg, L"Confirmação", MB_YESNO | MB_ICONQUESTION) == IDYES)
	{
		SYSTEMTIME st_old, st_new, st_upd;
		ZeroMemory(&st_old, sizeof(st_old));
		ZeroMemory(&st_new, sizeof(st_new));
		ZeroMemory(&st_upd, sizeof(st_upd));

		//CUtil::GetLastAtualizacao(&st_upd);

		CFile fileDate;
		CString sFileName;
		CStr sText;

		GetLocalTime(&st_old);

		//m_dtData.GetWindowText(sValue);

		CTime cTime;

		m_dtData.GetTime(cTime);

		st_new.wDay = cTime.GetDay();
		st_new.wMonth = cTime.GetMonth();
		st_new.wYear = cTime.GetYear();

		//m_tmHora.GetWindowText(sValue);
		m_tmHora.GetTime(cTime);

		st_new.wHour = cTime.GetHour();
		st_new.wMinute = cTime.GetMinute();
		st_new.wSecond = cTime.GetSecond();

		CString sMsg;
		sMsg.Format(L"Não é possível alterar data. A data têm que ter 20 dias de diferença de: %d/%d", st_upd.wDay, st_upd.wMonth);
		if(!CUtil::IsIntervaloDataTimeValid(&st_new, 'D', 20, nErrorDateDiff, 'P', &st_upd) && !CUtil::IsIntervaloDataTimeValid(&st_new, 'D', 20, nErrorDateDiff, 'F', &st_upd))
		{
			STLOG_WRITE("%s(%d): Diferença maior que 20 dias", __FUNCTION__, __LINE__);
			MessageBox(sMsg, L"Mensagem", MB_ICONINFORMATION | MB_OK);
			return;
		}

		if(!SetLocalTime(&st_new))
		{
			STLOG_WRITE(L"CData_HoraDlg::OnAlterar(): Erro ao alterar data/hora");
			MessageBox(L"Erro ao alterar Data/Hora");
			return;
		}

		ZeroMemory(&st_new, sizeof(st_new));
		GetLocalTime(&st_new);


#if _WIN32_WCE > 0x420
		sFileName.Format(L"%s\\Data_Hora_log.txt", CUtil::GetMainAppPath());
#else
		sFileName.Format(L"%s\\Data_Hora_log.txt", CUtil::GetBackupPath());
#endif
		fileDate.Open(sFileName, CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite);
		fileDate.SeekToEnd();

		sText.Format("Agente [%S]. De [%02d/%02d/%04d %02d:%02d] para [%02d/%02d/%04d %02d:%02d]. Motivo: %S \r\n",
			m_params->GetValue(L"codigo"), st_old.wDay, st_old.wMonth, st_old.wYear, st_old.wHour, st_old.wMinute,
			st_new.wDay, st_new.wMonth, st_new.wYear, st_new.wHour, st_new.wMinute, sValue);

		fileDate.Write(sText, sText.GetLength());

		fileDate.Close();
	}
	
	EndDialog(IDOK);

}

// DlgWireless.cpp : implementation file
//

#include "stdafx.h"
#include "Data_Hora.h"
#include "DlgWireless.h"


// DlgWireless dialog

IMPLEMENT_DYNAMIC(CDlgWireless, CDialogEx)

CDlgWireless::CDlgWireless(CWnd* pParent /*=NULL*/)
	: CDialogEx(CDlgWireless::IDD, pParent)
{

}

CDlgWireless::~CDlgWireless()
{
}

void CDlgWireless::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LBL_CONEXAO, m_lblConexao);
	DDX_Control(pDX, IDC_BTN_CONEXAO, m_btnConexao);
	DDX_Control(pDX, IDC_LBL_OPERADORA, m_lblOperadora);
	DDX_Control(pDX, IDC_LBL_SINAL, m_lblSinal);
	DDX_Control(pDX, IDC_BTN_CONFIGURAR, m_btnConfigurarCelular);
	DDX_Control(pDX, IDC_LBL_SITUACAO, m_lblSituacaoWiFi);
	DDX_Control(pDX, IDC_BTN_CONECTAR_WIFI, m_btnLigarWiFi);
	DDX_Control(pDX, IDC_BTN_CONFIGURAR_WIFI, m_btnConfigurarWiFi);
}


BEGIN_MESSAGE_MAP(CDlgWireless, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_CONEXAO, &CDlgWireless::OnBnClickedBtnConexao)
	ON_BN_CLICKED(IDC_BTN_CONFIGURAR, &CDlgWireless::OnBnClickedBtnConfigurar)
	ON_BN_CLICKED(IDC_BTN_CONECTAR_WIFI, &CDlgWireless::OnBnClickedBtnConectarWifi)
	ON_BN_CLICKED(IDC_BTN_CONFIGURAR_WIFI, &CDlgWireless::OnBnClickedBtnConfigurarWifi)
END_MESSAGE_MAP()


// DlgWireless message handlers

void CDlgWireless::OnBnClickedBtnConexao()
{
	// TODO: Add your control notification handler code here
}

void CDlgWireless::OnBnClickedBtnConfigurar()
{
	// TODO: Add your control notification handler code here
}

void CDlgWireless::OnBnClickedBtnConectarWifi()
{
	// TODO: Add your control notification handler code here
}

void CDlgWireless::OnBnClickedBtnConfigurarWifi()
{
	// TODO: Add your control notification handler code here
}

BOOL CDlgWireless::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_lblConexao.SetWindowText(CUtil::GetTipoConexao());
	m_lblOperadora.SetWindowText(CUtil::GetOperadora());

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

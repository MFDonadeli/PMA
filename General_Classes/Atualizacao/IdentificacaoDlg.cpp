#include "stdafx.h"
#include "IdentificacaoDlg.h"
#include "Utils.h"
#include "Consultas_TEM.h"
#include "CppSQLite3.h"
#include "ModParam.h"

// CIdentificacaoDlg dialog

IMPLEMENT_DYNAMIC(CIdentificacaoDlg, CDialogEx)

/**
\brief Construtor da classe
\details
	Funções executadas neste módulo:
	- Inicia as variáveis globais desta classe
	- Cria a fonte a ser utilizada nesta tela
	- Muda a cor da janela de fundo

\param CppSQLite3DB *pDB: Ponteiro para o banco de dados
\param LPCTSTR szCodigo: Código do agente
\param CWnd* pParent: Ponteiro para a janela que será pai desta
*/
CIdentificacaoDlg::CIdentificacaoDlg(CppSQLite3DB *pDB, LPCTSTR szCodigo, CWnd* pParent /*=NULL*/)
	: CDialogEx(CIdentificacaoDlg::IDD, pParent)
{
	m_sAgente = szCodigo;
	m_pDB     = pDB;

	CUtil::CreateFont(&m_font, L"Verdana", -11, TRUE);

	// Mudar a cor do fundo...
	SetBackColor(RGB(173, 199, 222)); 
}

/**
\brief Destrutor da classe
*/
CIdentificacaoDlg::~CIdentificacaoDlg()
{
}

void CIdentificacaoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_STATIC_NOME, m_lblNome);
	DDX_Control(pDX, IDC_STATIC_CAPTION, m_lblCaption);
}

BEGIN_MESSAGE_MAP(CIdentificacaoDlg, CDialogEx)
END_MESSAGE_MAP()


// CIdentificacaoDlg message handlers
/**
\brief Início desta janela.
\details
	Funções executadas neste módulo:
	- Configurar a janela para se adequar o padrão do PMA:
		- Configuração da barra de comando (barra de menu), na parte inferior da tela;
		- Criar o banner da parte superior;
		- Configurar tela cheia;
		- Configurar a fonte dos labels;
		- Escrever os dados na tela.
	- Construir as abas exibidas
	- Iniciar o preenchimento da lista de módulos

\param void
\return TRUE se a execução ocorrer com sucesso
*/
BOOL CIdentificacaoDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

#ifdef _WIN32_WCE
	HideSIP();

	if (!m_dlgCommandBar.Create(this) ||
	    !m_dlgCommandBar.InsertMenuBar(IDR_OKCANCEL))
	{
		AfxMessageBox(L"Erro ao criar tela de Identificação");
		_FullScreen();
		STLOG_WRITE("CIdentificacaoDlg::OnInitDialog() : Failed to create CommandBar");
		return FALSE;      // fail to create
	}

	_CreateBanner(IDB_HEADER, FALSE, BANNER_HEIGHT, L"Talão Eletrônico de Multas", BANNER_TEXT_LEFT);
	m_banner.ShowWindow(SW_SHOW);
#endif

	m_lblCaption.SetFont(&m_font);
	m_lblNome.SetFont(&m_font);

	CString sTexto;
	sTexto = L"Nome: %s \r\n\r\nAgente: %s\r\n\r\nEntidade: %s\r\n\r\nDefinição: %s";

	if(CConsultas_TEM::GetAgenteInfo(m_pDB, 
							     m_sAgente, 
							     sTexto, FALSE))
	{		
		m_lblNome.SetWindowText(sTexto);
	}

	_FullScreen();

	return TRUE;
}

/**
\brief Fecha esta janela com a opção cancelar
\details Envia a janela pai (PMADlg) a mensagem WM_RETORNO_LOGIN que resultará na chamada do método DoLogout()
\param void
\return void
*/
void CIdentificacaoDlg::OnCancel()
{
	//TODO: Quando o agente logado não existir o equipamento está travando
	//Envia msg WM_RETORNO_LOGIN
	if(GetParent() != NULL)
		GetParent()->PostMessage(CModParam::WM_RETORNO_LOGIN, 0, 0);
	
	EndDialog(IDCANCEL);
}

/**
\brief Fecha esta janela com a opção OK
\param void
\return void
*/
void CIdentificacaoDlg::OnOK()
{
	EndDialog(IDOK);
}


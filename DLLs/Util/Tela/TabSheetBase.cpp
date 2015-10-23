#include "StdAfx.h"
#include "TabSheetBase.h"


IMPLEMENT_DYNAMIC(CTabSheetBase, CDialog);

/**
\brief 
	Construtor da classe
\details
	Fun��es executadas neste m�dulo:
	- Inicializa��o de vari�veis globais

\param 
	UINT nResID: ID do form que esta classe usar�
	CMultiDlgBase* pParent: Ponteiro para a janela que ser� pai desta
*/
CTabSheetBase::CTabSheetBase(UINT nResID, CMultiDlgBase* pParent)
	: CDialog(nResID, (CWnd *)pParent)
{
	m_pParent = pParent;
	m_bSipVisible = FALSE;
	m_bFromRcGen = FALSE;
}

/**
\brief 
	Destrutor da classe
*/
CTabSheetBase::~CTabSheetBase(void)
{
}

/**
\brief 
	Configura as trocas e valida��es dos campos desta janela (Uni�o de controles e vari�veis)
\param 
	CDataExchange* pDx: Ponteiro para a classe que faz essa troca
\return 
	void
*/
void CTabSheetBase::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CTabSheetBase, CDialog)
	ON_WM_SETTINGCHANGE()
	ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()


//void CTabSheetBase::ProcessShowWindow(BOOL bVisible)
//{
//}

/**
\brief
	In�cio desta janela.
\details
	Fun��es executadas neste m�dulo:
	- Indicar que a p�gina n�o ser� tela cheia (e sim a janela pai desta p�gina)
	- Modificar o estilo da janela para adequar aos padr�es do aplicativo

\return 
	TRUE se a execu��o ocorrer com sucesso
*/
BOOL CTabSheetBase::OnInitDialog() 
{
	//m_bFullScreen = FALSE; //VERIFICAR SE ESTE COMENTARIO REFLETE NO SISTEMA DO POCKET

	CDialog::OnInitDialog();

#ifdef _WIN32_WCE

	SHINITDLGINFO shidi;
	shidi.hDlg    = m_hWnd;
	shidi.dwMask  = SHIDIM_FLAGS;
	shidi.dwFlags = SHIDIF_SIZEDLGFULLSCREEN;
	::SHInitDialog(&shidi);

	::SHDoneButton(m_hWnd, SHDB_HIDE);
	ModifyStyle(0, WS_NONAVDONEBUTTON, SWP_NOSIZE);

#endif

	return TRUE;
}

/**
\brief 
	Processa qualquer comando recebido pela p�gina, que n�o foi processado por CMultiDlgBase
\param 
	WPARAM wParam: Par�metro da mensagem
	LPARAM lParam: Par�metro da mensagem
\return
	TRUE se a execu��o ocorrer com sucesso
*/
BOOL CTabSheetBase::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	// Se processar... encerrar...
	if(CDialog::OnCommand(wParam, lParam))
		return TRUE;

	return TRUE;
}

/**
\brief
	Mostra o painel de entrada de dados (teclado virtual)
\param
	void
\return 
	void
*/
void CTabSheetBase::ShowSIP()
{
#ifdef _WIN32_WCE
	m_bSipVisible = TRUE;
	SHSipPreference(GetSafeHwnd(), SIP_UP);
#endif
}

/**
\brief
	Esconde o painel de entrada de dados (teclado virtual)
\param
	void
\return 
	void
*/
void CTabSheetBase::HideSIP()
{
#ifdef _WIN32_WCE
	m_bSipVisible = FALSE;
	SHSipPreference(GetSafeHwnd(), SIP_FORCEDOWN);
#endif
}

/**
\details
*****AVISO******
Configura a p�gina como tela cheia, por�m esta fun��o � executada de modo errado.
*/
void CTabSheetBase::_FullScreen1()
{
#ifdef _WIN32_WCE
	// Descontar o tamanho do SIP...
	CRect rect;
	if(SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, NULL) == 1)
		MoveWindow(CRect(0, 0, rect.Width(), rect.Height()- 80), TRUE);

	SHFullScreen(m_hWnd, SHFS_HIDESTARTICON|SHFS_HIDETASKBAR);
#endif
}

/**
\brief
	M�todo chamado quando � mudado algum par�metro de sistema na janela
\details
	Fun��es executadas por este m�todo:
	- Manter a janela no lugar ao chamar/esconder o teclado virtual

\param
	UINT uFlags: Par�metro do sistema a ser mudado
	LPCTSTR lpszSection: Nome da sess�o a ser modificada
*/
void CTabSheetBase::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CDialog::OnSettingChange(uFlags, lpszSection);

	BOOL bSIPVisible = FALSE;

#ifdef _WIN32_WCE
	SIPINFO si = {0};
	si.cbSize = sizeof(si);
	SHSipInfo(SPI_GETSIPINFO, 0, &si, 0);
	bSIPVisible = si.fdwFlags & SIPF_ON;

	// Se houver mudancas no SIP, vamos setar Fullscreen novamente...
	if(!m_bSipVisible && bSIPVisible)
	{
		m_bSipVisible = TRUE;

		// Descontar o tamanho do SIP...
		CRect rect;
		if(SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, NULL) == 1)
			MoveWindow(CRect(0, 0, rect.Width(), rect.Height()- 80), TRUE);

		SHFullScreen(m_hWnd, SHFS_HIDESTARTICON|SHFS_HIDETASKBAR);
	}
	else if(m_bSipVisible && !bSIPVisible)
	{
		// Fullscreen novamente...
		m_bSipVisible = FALSE;
		_FullScreen();
	}
#endif
}

/**
\brief
	Configura o aplicativo para preencher toda a tela do equipamento
\details
	*********ATENCAO**********
	Provavelmente tem o mesmo problema de _FullScreen1(), por isso est� sem uso
\param
	void
\return
	void
*/
void CTabSheetBase::_FullScreen()
{
#ifdef _WIN32_WCE
	SetForegroundWindow();  
	// Resize the window over the taskbar area.
	CRect rect = CRect(0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
	MoveWindow(&rect, TRUE);
	SHFullScreen(m_hWnd, SHFS_HIDESTARTICON|SHFS_HIDETASKBAR);
#endif
}

/**
\brief
	Indica se o teclado virtual est� ou n�o vis�vel
\param
	void
\return
	TRUE se o teclado virtual est� vis�vel
*/
BOOL CTabSheetBase::IsSIPVisible()
{
#ifdef _WIN32_WCE
	SIPINFO si = {0};
	si.cbSize = sizeof(si);
	SHSipInfo(SPI_GETSIPINFO, 0, &si, 0);
	return si.fdwFlags & SIPF_ON;
#else
	return FALSE;
#endif
}

/**
\brief
	M�todo usado para mostrar/esconder p�gina atual. Sem uso.
\param
	BOOL bShow: Indica se a janela est� sendo mostrada ou escondida
	UINT nStatus: Status da janela
\return
	void
*/
void CTabSheetBase::OnShowWindow(BOOL bShow, UINT nStatus)
{
}

/**
\brief
	Fecha a janela atual.
\details
	Este m�todo � uma sobreposi��o declarada para que a janela n�o feche ao apertar
	a tecla ENTER em algum controle da p�gina atual.
\param
	void
\return
	void
*/
void CTabSheetBase::OnOK()
{
	// TODO: Add your specialized code here and/or call the base class

	//CDialog::OnOK();
}

BOOL CTabSheetBase::Validate(CString &sErrMsg, BOOL bFromCance)
{
	return FALSE;
}

#include "StdAfx.h"
#include "PanelButton.h"
#include "Utils.h"

#ifdef _WIN32_WCE


#define ID_BTN_UP	0x4488
#define ID_BTN_DN	0x4499
#define ID_BTN_CAM	0x4477

UINT CPanelButton::WM_PANELBUTTON_MSG = 
	RegisterWindowMessage(L"CPanelButton::WM_PANELBUTTON_MSG");

/**
\brief Construtor da classe
*/
CPanelButton::CPanelButton(void)
{	
}

/**
\brief Destrutor da classe
*/
CPanelButton::~CPanelButton(void)
{
}

/**
\brief
	Cria um novo painel que irá conter o título da janela (página) e os botões para mudar de página
\details
	Funções executadas neste método
	- Criar e posicionar os botões

	Este painel é um controle CStatic

\param CWnd* pParentWnd: Ponteiro para a janela pai deste painel
\param UINT nIDUp: ID do botão de subida de tela
\param UINT nIDDown: ID do botão de descida de tela
\param const RECT &rect: Retângulo que indica tamanho do painel (desenho exato)
\param UINT nID: ID da janela (painel)
\return TRUE se a criação do controle ocorrer com sucesso
*/
BOOL CPanelButton::Create(CWnd* pParentWnd, UINT nButtons[], const RECT &rect, UINT nID)
{
	if(CStatic::CreateEx(0,
						 NULL,
						 NULL, 
						 WS_CHILD, 
						 rect,
						 pParentWnd, 
						 nID))
	{
		CRect r;
		pParentWnd->GetClientRect(&r);

		int left3 = r.Width() - DRA::SCALEX(60);
		int left1 = r.Width() - DRA::SCALEX(40);
		int left2 = r.Width() - DRA::SCALEX(20);

		if(nButtons)
		{
			if(nButtons[0] != NULL)
			{
				m_btns[0].Create(L"", WS_CHILD|WS_VISIBLE, CRect(left1-1, DRA::SCALEY(5), left1 + DRA::SCALEX(16), DRA::SCALEY(21)), this, ID_BTN_UP);
				m_btns[0].SetBKColor(RGB(255, 255, 255), FALSE);
				m_btns[0].SetIconEx(nButtons[0], CSize(DRA::SCALEX(16), DRA::SCALEY(16)));
				m_btns[0].DrawBorder(FALSE);
				m_btns[0].ModifyStyle(WS_BORDER, 0);
			}

			if(nButtons[1] != NULL)
			{
				m_btns[1].Create(L"", WS_CHILD|WS_VISIBLE, CRect(left2-1, DRA::SCALEY(5), left2 + DRA::SCALEX(16), DRA::SCALEY(21)), this, ID_BTN_DN);
				m_btns[1].SetBKColor(RGB(255, 255, 255), FALSE);
				m_btns[1].SetIconEx(nButtons[1], CSize(DRA::SCALEX(16), DRA::SCALEY(16)));
				m_btns[1].DrawBorder(FALSE);
				m_btns[1].ModifyStyle(WS_BORDER, 0);
			}

			if(nButtons[2] != NULL)
			{

				m_btns[2].Create(L"", WS_CHILD|WS_VISIBLE, CRect(left3-1, DRA::SCALEY(5), left3 + DRA::SCALEX(16), DRA::SCALEY(21)), this, ID_BTN_CAM); 
				m_btns[2].SetBKColor(RGB(255, 255, 255), FALSE);
				m_btns[2].SetIconEx(nButtons[2], CSize(DRA::SCALEX(16), DRA::SCALEY(16)));
				m_btns[2].DrawBorder(FALSE);
				m_btns[2].ModifyStyle(WS_BORDER, 0);		

			}
		}

		return TRUE;
	}

	return FALSE;
}

BEGIN_MESSAGE_MAP(CPanelButton, CCaptionPanel)
	ON_WM_SIZE()
	ON_BN_CLICKED(ID_BTN_UP, OnUp)
	ON_BN_CLICKED(ID_BTN_DN, OnDown)
	ON_BN_CLICKED(ID_BTN_CAM, OnCam)
	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()

/**
\brief Posiciona a janela, de acordo com o tamanho
\details
	Funcoes executadas neste metodo
	- Posicionar os botoes

\param UINT nType: Tipo de posicionamento requerido
\param int cx: Largura da tela do equipamento
\param int cy: Altura da tela do equipamento
\return void
*/
void CPanelButton::OnSize(UINT nType, int cx, int cy)
{
	CStatic::OnSize(nType, cx, cy);
	
	if(IsWindow(GetSafeHwnd()))
	{
		if(IsWindow(m_btns[0].GetSafeHwnd()) && m_btns[0].IsWindowVisible())
			m_btns[0].SetWindowPos(NULL, cx - DRA::SCALEX(40), DRA::SCALEY(2), 0, 0, SWP_NOZORDER|SWP_NOSIZE);

		if(IsWindow(m_btns[1].GetSafeHwnd()) && m_btns[1].IsWindowVisible())
			m_btns[1].SetWindowPos(NULL, cx - DRA::SCALEX(20), DRA::SCALEY(2), 0, 0, SWP_NOZORDER|SWP_NOSIZE);
		
		if(IsWindow(m_btns[2].GetSafeHwnd()) && m_btns[2].IsWindowVisible())
			m_btns[2].SetWindowPos(NULL, cx - DRA::SCALEX(60), DRA::SCALEY(2), 0, 0, SWP_NOZORDER|SWP_NOSIZE);
	}
}

/**
\brief
	Envia uma mensagem a janela pai para processar o clique no botão de volta de página
\param void
\return
	void
*/
void CPanelButton::OnUp()
{
	GetParent()->SendMessage(CPanelButton::WM_PANELBUTTON_MSG, 0, 0);
}

/**
\brief
	Envia uma mensagem a janela pai para processar o clique no botão de próxima página
\param void
\return
	void
*/
void CPanelButton::OnDown()
{
	GetParent()->SendMessage(CPanelButton::WM_PANELBUTTON_MSG, 1, 0);
}

/**
\brief
	Envia uma mensagem a janela pai para processar o clique no botão de câmera
\param void
\return
	void
*/
void CPanelButton::OnCam()
{
	EnableCameraBtn(0);
	GetParent()->SendMessage(CPanelButton::WM_PANELBUTTON_MSG, 2, 0);	
}

/**
\brief
	Processa a mensagem ao movimento do toque na tela (arrasto da stylus)
\details
	Funcoes executadas neste metodo
	- Muda de página de acordo com o movimento feito na tela
\param UINT nFlags: Flags que indica se houve um pressionamento comum ou combinado na tela (tecla CTRL por exemplo)
\param CPoint point: Posição corrente na tela
\return
	void
*/
void CPanelButton::OnMouseMove(UINT nFlags, CPoint point)
{
	POINT pt[64];
	UINT ppr;
	int a;

	GetMouseMovePoints(pt, 64, &ppr);

	if(ppr>1)
	{
		a = pt[ppr-1].x - pt[0].x;

		if(a < 0)
		{
			STLOG_WRITE(L"Moveu pra tras [%d]", a);
			OnUp();
		}
		else
		{
			STLOG_WRITE(L"Moveu pra frente [%d]", a);
			OnDown();
		}
	}

	//CDialog::OnMouseMove(nFlags, point);
	
}

/**
\brief
	Controla exibição do botão da câmera
\param BOOL
\return
	void
*/
void CPanelButton::ShowCameraBtn(BOOL bShowCameraButton)
{		
	if(bShowCameraButton)
		m_btns[2].ShowWindow(SW_SHOW);		
	else
		m_btns[2].ShowWindow(SW_HIDE);
}

void CPanelButton::EnableCameraBtn(BOOL bEnableCameraButton)
{		
	m_btns[2].EnableWindow(bEnableCameraButton);
}
#endif
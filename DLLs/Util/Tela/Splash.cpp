// Copyright (c) 2007 Marcos Mori de Siqueira. All Rights Reserved. 
// mori@softfactory.com.br
// Classe que representa o splash screen da aplicacao
#include "StdAfx.h"
#include "Splash.h"
#include "Utils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CSplash, CWnd)


/**
\brief Construtor da classe
\param void
\return void
*/
CSplash::CSplash(void)
#ifdef _WIN32_WCE
	: m_pDIBSection(NULL)
#endif
{
}


/**
\brief Destrutor da classe
\details Deleta instância m_pDIBSection
\param void
\return void
*/
CSplash::~CSplash(void)
{
#ifdef _WIN32_WCE
	if(m_pDIBSection != NULL)
	{
		delete m_pDIBSection;
		m_pDIBSection = NULL;
	}
#endif
}

BEGIN_MESSAGE_MAP(CSplash, CWnd)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
END_MESSAGE_MAP()



/**
\brief Cria os elementos da tela splash, carregando a imagem e configurando seu tamanho 
\param CWnd *pParentWnd Ponteiro pra janela pai
\param LPCTSTR szFile Arquivo da imagem 
\param int cx Largura da imagem
\param int cy Altura da imagem
\return BOOL
*/
BOOL CSplash::Create(CWnd *pParentWnd, LPCTSTR szFile, int cx, int cy)
{
	//cx = 480;
	//cy = 640;

	BOOL b = CreateEx(WS_EX_TOPMOST,
				      AfxRegisterWndClass(0), 
					  NULL, 
					  WS_POPUP, 
					  CRect(0, 0, cx, cy),
					  pParentWnd, 
					  0x558);

	if(b) 
	{ 
		// Criamos a imagem com new para poder 
		// deletar o mais breve possivel...
		//HBITMAP hBmp = SHLoadImageFile(L"\\imgs\\Splash.png");
#ifdef _WIN32_WCE
		HBITMAP hBmp = SHLoadImageFile(szFile);
		m_pDIBSection = new CDIBSectionLite();
		if(!m_pDIBSection->SetBitmap(hBmp/*szFile*/))
		{
			delete m_pDIBSection;
			m_pDIBSection = NULL;
			b = FALSE;
		}
#endif
	}

	return b;
}



/**
\brief Exibe tela splash criada
\param DWORD dwSeconds Tempo de exibição em segundos
\return void
*/
void CSplash::Show(DWORD dwSeconds)
{
#ifdef _WIN32_WCE
	HWND hWndTaskBar = ::FindWindow(TEXT("HHTaskBar"), NULL);
	if(hWndTaskBar != NULL)
		::ShowWindow(hWndTaskBar, SW_HIDE);
#endif

	CRect r;
	GetClientRect(&r);

	int top  = (GetSystemMetrics(SM_CYSCREEN) - r.Height() ) / 2;
	int left = (GetSystemMetrics(SM_CXSCREEN) - r.Width() ) / 2;

	SetWindowPos(&wndTopMost,
				 left, 
				 top, 
				 r.Width(), 
				 r.Height(), 
				 SWP_SHOWWINDOW|SWP_FRAMECHANGED);

	SetCapture();
	Invalidate();
	UpdateWindow();

	Sleep(dwSeconds * 1000);
}


/**
\brief Destroi elementos da tela e suas instâncias
\param void
\return void
*/
void CSplash::Destroy()
{
	if(GetSafeHwnd() != NULL)
	{
#ifdef _WIN32_WCE
		if(m_pDIBSection != NULL)
		{
			delete m_pDIBSection;
			m_pDIBSection = NULL;
		}
#endif

		Hide();
		DestroyWindow();
	}
}

/**
\brief Esconde tela splash
\param void
\return void
*/
void CSplash::Hide()
{
	ReleaseCapture();
	if(::IsWindow(this->m_hWnd))
		ShowWindow(SW_HIDE);
}


BOOL CSplash::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

/**
\brief Executado quando a tela é desenhada
\param void
\return void
*/
void CSplash::OnPaint()
{
	CPaintDC dc(this);
#ifdef _WIN32_WCE
	if(m_pDIBSection != NULL)
		m_pDIBSection->Draw(&dc, CPoint(0, 0));
#endif
}

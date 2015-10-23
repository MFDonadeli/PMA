// PhotoViewDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Camera.h"
#include "PhotoViewDlg.h"
#include "Utils.h"


// CPhotoViewDlg dialog

IMPLEMENT_DYNAMIC(CPhotoViewDlg, CDialogEx)

CPhotoViewDlg::CPhotoViewDlg(CString sImage, BOOL bFullScreen, CWnd* pParent /*=NULL*/)
	: CDialogEx(CPhotoViewDlg::IDD, pParent)
{
	m_sImage = sImage;
	m_bFullScreen = bFullScreen;
	//m_ListPhotos.ShowWindow(SW_HIDE);
}


CPhotoViewDlg::~CPhotoViewDlg()
{
	delete m_pDIBSection;
}

void CPhotoViewDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_PHOTOS, m_ListPhotos);
}


BEGIN_MESSAGE_MAP(CPhotoViewDlg, CDialogEx)
	ON_COMMAND(ID_EXCLUIR, CPhotoViewDlg::OnExcluir)
	ON_COMMAND(ID_SAIR, CPhotoViewDlg::OnSair)
	ON_LBN_SELCHANGE(IDC_LIST_FOTOS, &CPhotoViewDlg::OnLbnSelchangeListFotos)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_PHOTOS, &CPhotoViewDlg::OnLvnItemchangedListPhotos)
	ON_WM_TIMER()
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()


// CPhotoViewDlg message handlers

BOOL CPhotoViewDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	if(!m_dlgCommandBar.Create(this) || !m_dlgCommandBar.InsertMenuBar(ID_MENU_VIEW))
	{
		//AfxMessageBox(IDS_COMMANDBAR_ERROR);
		//STLOG_WRITE("CSincroDlg::OnInitDialog() : Failed to create CommandBar");
		EndDialog(IDCANCEL);
		return FALSE;      // fail to create
	}

	if(!m_bFullScreen)
	{
		_CreateBanner(L"Visualização");		
	}

	SetForegroundWindow();

	_FullScreen();

//	CWnd *pWnd = GetDlgItem(IDC_BUTTON3);
	

	/*if(m_bFullScreen)
	{
		m_ListPhotos.ShowWindow(SW_HIDE);
	}
	else
	{

		DWORD a = GetTickCount();

		m_ImgView.ShowWindow(SW_HIDE);

		_FillList();
		
	}*/


	SetTimer(1, 1000, NULL);



	


	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CPhotoViewDlg::OnExcluir()
{
	if(DeleteFile(m_sImage))
		MessageBox(L"Arquivo excluído!",L"Mensagem", MB_ICONINFORMATION|MB_OK);
	else
		MessageBox(L"Falha ao excluir o arquivo",L"Mensagem", MB_ICONINFORMATION|MB_OK);

//	_FillList();
	SetTimer(2, 500, NULL);
}

void CPhotoViewDlg::OnSair()
{
	EndDialog(IDOK);
}

void CPhotoViewDlg::_FillList()
{
	// Adicionar o string de busca...
	CString sPath1 = L"\\temp\\*.jpg";

	if(!m_sImagePat.IsEmpty())
	{
		sPath1.Format(L"\\temp\\%s_", m_sImagePat);
	}

	m_ListPhotos.DeleteAllItems();
	m_ListPhotos.CreateColumn();
	m_ListPhotos.Add(sPath1);

	//// Varrer o diretorio...
	//WIN32_FIND_DATA wfd;
	//HANDLE hSearch = FindFirstFileW(sPath1, &wfd);
	//if(hSearch != INVALID_HANDLE_VALUE)
	//{
	//	m_sImage = wfd.cFileName;
	//	// Recuperar o item...
	//	do
	//	{
	//		CString sFile = wfd.cFileName;
	//		
	//		m_ListPhotos.AddItem(sFile);
	//	}
	//	while(FindNextFileW(hSearch, &wfd));

	//	FindClose(hSearch);
	//}
}

void CPhotoViewDlg::OnLbnSelchangeListFotos()
{
	/*CRect r1;

	CString sPhotoName;
	int iCurSel = m_ListPhotos.GetCurSel();
	m_ListPhotos.GetText(iCurSel, sPhotoName);

	m_sImage.Format(L"\\temp\\%s", sPhotoName);

	STLOG_WRITE(L"CPhotoViewDlg::OnLbnSelchangeListFotos(): Showing photo [%s]",m_sImage);

	m_ImgView.LoadJPG(m_sImage);

	m_ImgView.Invalidate();
	m_ImgView.RedrawWindow();

	UpdateWindow();*/

}

void CPhotoViewDlg::OnLvnItemchangedListPhotos(NMHDR *pNMHDR, LRESULT *pResult)
{
	m_ImgView.ShowWindow(SW_SHOW);

	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	ImgInfo* iImg;
	ImgInfo* iImg2 = new ImgInfo();

	if(pNMLV->uNewState!=3)
		return;
	// TODO: Add your control notification handler code here

	CRect r1;
	int n=3;

	iImg = m_ListPhotos.pImgInfo[pNMLV->iItem];
	//CString sText = m_ListPhotos.GetItemText(pNMLV->iItem, 1);
	//CStringArray sArray;
	//CUtil::Tokenize(sText, sArray, n, L";");
	//m_sImage = sArray[0];
	m_sImage = iImg->m_photoPath;

	RECT r, r2;
	SystemParametersInfo(SPI_GETWORKAREA, 0, &r, NULL);

	m_ListPhotos.GetWindowRect(&r2);

	float x_foto, y_foto;
	x_foto = iImg->m_width;
	y_foto = iImg->m_height;
	//x_foto = _wtoi(sArray[1]);
	//y_foto = _wtoi(sArray[2]);

	float x_livre, y_livre;
	x_livre = r.right;
	y_livre = r2.top;

	int dx_foto, dy_foto;

	if(x_foto > y_foto)
	{//deitado
		dx_foto = x_livre;
		dy_foto = (x_livre/x_foto) * y_foto;
		STLOG_WRITE(L"Retangulo: [%d][%d][%d][%d] e [%d][%d]", 0, ((y_livre/2)-(dy_foto/2)), x_livre, ((y_livre/2)+(dy_foto/2)), dx_foto, dy_foto);
		r1.SetRect(0, ((y_livre/2)-(dy_foto/2)), x_livre, ((y_livre/2)+(dy_foto/2)));
	}
	else
	{
		dx_foto = (y_livre / y_foto) * x_foto;
		dy_foto = y_livre;

		STLOG_WRITE(L"Retangulo: [%d][%d][%d][%d] e [%d][%d]", ((x_livre/2)-(dx_foto/2)),0,((x_livre/2)+(dx_foto/2)), y_livre, dx_foto, dy_foto);
		r1.SetRect(((x_livre/2)-(dx_foto/2)),0,((x_livre/2)+(dx_foto/2)), y_livre);
	}


	ScreenToClient(r1);

	m_ImgView.MoveWindow(r1);
	
	m_ImgView.SetBitmap(CUtil::LoadPicture(iImg->m_photoPath, iImg2, m_hWnd));
	//m_ImgView.LoadJPG(iImg->m_photoPath);

	m_sImage = iImg->m_photoPath;

	m_ImgView.Invalidate();
	m_ImgView.RedrawWindow();

	UpdateWindow();

	delete iImg2;

	*pResult = 0;

}

void CPhotoViewDlg::SetImagePattern(LPCTSTR szImage)
{
	m_sImagePat = szImage;
}

void CPhotoViewDlg::OnTimer(UINT_PTR nIDEvent)
{
	if(nIDEvent == 1)
	{
		/*CRect r1;
		GetClientRect(&r1);

		m_ImgView.Create(r1, this, 0x67);
		m_ImgView.SetStretch(TRUE);
		ScreenToClient(&r1);*/

		RedrawWindow();
	}
	else if(nIDEvent == 2)
	{
		OnSair();
	}
}

BOOL CPhotoViewDlg::OnEraseBkgnd(CDC *pDC)
{
	CRect rect;
	GetClientRect(&rect);	
	pDC->PatBlt(rect.left, rect.top, rect.Width(), rect.Height(), PATCOPY);	

	return TRUE;
}

void CPhotoViewDlg::OnPaint()
{
	
	HBITMAP hBmp = SHLoadImageFile(m_sImage);
	if(!m_pDIBSection->SetBitmap(hBmp))
	{
		delete m_pDIBSection;
		m_pDIBSection = NULL;	
	}	

	CPaintDC dc(this);	
	if(m_pDIBSection != NULL)
		m_pDIBSection->Draw(&dc, CPoint(0, 0));
}
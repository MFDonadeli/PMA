// ListImageCtrl.cpp : implementation file
//

#include "stdafx.h"


#if _WIN32_WCE != 0x420 && defined _WIN32_WCE
//#include "ListImage.h"
#include "ListImageCtrl.h"
#include "Utils.h"
//#include "VOIMAGE.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#define SCALEFORTHUMB 50
#define THUMWIDTH 65
#define THUMHEIGHT 65

CString ConvertDoubleToString(double dvTmp);
void DoEvents(void);
/////////////////////////////////////////////////////////////////////////////
// CListImageCtrl

CListImageCtrl::CListImageCtrl()
{
	
}

CListImageCtrl::~CListImageCtrl()
{
	for(int i=0; i<pImgInfo.GetSize();i++)
	{
		ImgInfo* iInfo = pImgInfo[i];
		delete iInfo;
	}
	pImgInfo.RemoveAll();
}


BEGIN_MESSAGE_MAP(CListImageCtrl, CListCtrl)
	//{{AFX_MSG_MAP(CListImageCtrl)
//	ON_WM_DROPFILES()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CListImageCtrl message handlers
// This funtion is used to load the Window dropped files into the listview
HBITMAP CListImageCtrl::LoadPicture(CString strOriginalImgPath)
{
	HBITMAP hThumb;
	CDC memdc;
	m_hBMP = SHLoadImageFile (strOriginalImgPath);
	HBITMAP hBMP = m_hBMP;
	if (hBMP != NULL)
	{
		BITMAP bm;
		GetObject(hBMP, sizeof(bm), &bm);
		m_width = bm.bmWidth;
		m_height = bm.bmHeight;

		HDC hdcSrc = CreateCompatibleDC(::GetDC(this->m_hWnd));
		HBITMAP hOldBmp = (HBITMAP)SelectObject(hdcSrc, hBMP);
		hThumb = CreateCompatibleBitmap(hdcSrc, THUMWIDTH, THUMHEIGHT);
		memdc.CreateCompatibleDC(CDC::FromHandle(hdcSrc));
		HBITMAP hOldBmp2 = (HBITMAP)SelectObject(memdc, hThumb);
		StretchBlt(memdc, 0, 0, THUMWIDTH, THUMHEIGHT, hdcSrc, 0,0, bm.bmWidth, bm.bmHeight, SRCCOPY);
		//SelectObject(hdcSrc, hOldBmp);
		//SelectObject(memdc, hOldBmp2);
		DeleteObject(hBMP);
	}
	else
		return NULL;

	return hThumb;
}

void CListImageCtrl::Add(CString sPath) 
{
	//WORD wNumFilesDropped = DragQueryFile(hDropInfo, -1, NULL, 0);

	CString firstFile=L"";
	CFile Cf;
	DWORD TmpVal;
	TCHAR szText[MAX_PATH];
	int kk=0;
//	int tTot=(int)wNumFilesDropped;
	int m_TotCount;
	//CVOImage image;

	/*if(sPath.GetAt(sPath.GetLength()-1) != '\\')
		sPath += L"\\";*/

	// Adicionar o string de busca...
	CString sPath1 = sPath + L"*.jpg";
	sPath = L"\\temp\\";

	// Varrer o diretorio...
	WIN32_FIND_DATA wfd;
	HANDLE hSearch = FindFirstFileW(sPath1, &wfd);

	if(hSearch != INVALID_HANDLE_VALUE)
	{
		// Recuperar o item...
		do
		{

			kk++;
			CString nFileText;
			CString nPath;
			CString pItemText;
			pItemText.Format(L"%s%s", sPath, wfd.cFileName);
			pItemText.MakeUpper();
			//CString pItemText=npszFile;
	//		CFile Cf;


			//CVOImage::g_iScale = 10;
			//image.Load(::GetDC(this->m_hWnd),pItemText);

			int i=pItemText.ReverseFind('\\');

			nFileText=pItemText.Mid(i+1);
			nPath=pItemText.Left(i+1);
			i=nFileText.Find(L"%");
			if (i==-1)
			{
				i=pItemText.Find(_T(".JPG"),0);

				if (i!=-1)
				{
					//Creating thumbnail image for the file
					//HBITMAP bitm=LoadPicture1(pItemText, TRUE);
					ImgInfo* pImg = new ImgInfo();

					HBITMAP bitm = CUtil::LoadPicture(pItemText, pImg, m_hWnd, THUMWIDTH, THUMHEIGHT);

					
					//pImg->m_height = m_height;
					//pImg->m_width = m_width;
					//pImg->m_photoPath = pItemText;
					//pImg->hBmp = m_hBMP;
					//pImg->hDC = hDC;

					pImgInfo.Add(pImg);

					//HBITMAP bitm = image.m_hbitmap;
					//bitm = SHLoadImageFile(pItemText);
					if (bitm!=NULL)
					{
						//List item index
						m_TotCount=GetItemCount();
						//Adding Bitmap to the Imagelist
						CBitmap*    pImage = NULL;	
						pImage = new CBitmap();		 
						pImage->Attach(bitm);

						int imgP=m_imageList.Add(pImage,RGB(0,0,0));
						//Link to the added listview item 
						InsertItem(m_TotCount,L"",imgP);

						//SetItemText(m_TotCount, 0, m_sImgSize);
						//CString n = GetItemText(m_TotCount, 0);
					}
				}
			}
		}
		while(FindNextFileW(hSearch, &wfd));

		FindClose(hSearch);
	}

		// clean up
//		LocalFree(npszFile);
	

	// Free the memory block containing the dropped-file information
//	DragFinish(hDropInfo);

	// if this was a shortcut, we need to expand it to the target path
	int result=GetItemCount();
	//SetItemState(result, LVIS_SELECTED | 
    //LVIS_FOCUSED | LVIS_ACTIVATING, LVIS_SELECTED | LVIS_FOCUSED); 
	//m_ProgList.SetActiveItem(k);
	//m_ProgList.SetFocus();
	result--;
	EnsureVisible(result, TRUE);
	SetFocus();
	SetRedraw(TRUE);
	RedrawWindow(NULL,NULL);
	
//	CListCtrl::OnDropFiles(hDropInfo);
}

void CListImageCtrl::AddItem(CString sPath) 
{
	CString firstFile=L"";
	CFile Cf;
	DWORD TmpVal;
	TCHAR szText[MAX_PATH];
	int kk=0;
//	int tTot=(int)wNumFilesDropped;
	int m_TotCount;
	//CVOImage image;
		
	kk++;

	CString nFileText;
	CString nPath;
	CString pItemText;
	pItemText = sPath;
	pItemText.MakeUpper();	

	int i=pItemText.ReverseFind('\\');

	nFileText=pItemText.Mid(i+1);
	nPath=pItemText.Left(i+1);
	i=nFileText.Find(L"%");
	if (i==-1)
	{
		i=pItemText.Find(_T(".PNG"),0);
		if(i==-1) i=pItemText.Find(_T(".JPG"),0);

		if (i!=-1)
		{
			ImgInfo* pImg = new ImgInfo();

			HBITMAP bitm = CUtil::LoadPicture(pItemText, pImg, m_hWnd, THUMWIDTH, THUMHEIGHT);
			pImgInfo.Add(pImg);


			if (bitm!=NULL)
			{
				//List item index
				m_TotCount=GetItemCount();
				//Adding Bitmap to the Imagelist
				CBitmap*    pImage = NULL;	
				pImage = new CBitmap();		 
				pImage->Attach(bitm);

				int imgP=m_imageList.Add(pImage,RGB(0,0,0));
				//Link to the added listview item 
				//InsertItem(m_TotCount,pItemText,imgP);


			}
		}
	}


	// if this was a shortcut, we need to expand it to the target path
	int result=GetItemCount();
	
	result--;
	EnsureVisible(result, TRUE);
	SetFocus();
	SetRedraw(TRUE);
	RedrawWindow(NULL,NULL);
	
//	CListCtrl::OnDropFiles(hDropInfo);
}


CString ConvertDoubleToString(double dvTmp)
{
	CString strValue,strInt, strDecimal;
	int decimal,sign;
	double dValue = dvTmp;
	strValue = _fcvt(dValue,6,&decimal,&sign); 
		// Now decimal contains 1 because there is 
		// only one digit before the .

	strInt = strValue.Left(decimal); // strInt contains 4
	strDecimal = strValue.Mid(decimal,2); // strDecimal contains 125

	CString strFinalVal;
	strFinalVal.Format(L"%s.%s",strInt,strDecimal); 
	return strFinalVal;
}

void CListImageCtrl::CreateColumn()
{
	InsertColumn(0,_T("Filename"),LVCFMT_LEFT,125,-1);
	InsertColumn(1,_T("Path"),LVCFMT_LEFT,125,-1);
	InsertColumn(2,_T("Size"),LVCFMT_LEFT,75,-1);

#if (_WIN32_WCE >= 0x500)
	HIMAGELIST hScreens = ImageList_Create(THUMWIDTH, THUMHEIGHT, ILC_COLOR32, 0, 1);
#else
	HIMAGELIST hScreens = ImageList_Create(THUMWIDTH, THUMHEIGHT, ILC_COLOR, 0, 1);
#endif
	m_imageList.Attach(hScreens);

	SetImageList(&m_imageList, LVSIL_NORMAL);
	SetImageList(&m_imageList, LVSIL_SMALL);
}

// this function is used to enable the system messages
// this is mainly used to display the multiple images dropped on the list control

void DoEvents(void)
{
    MSG Symsg;
    
    while(PeekMessage(&Symsg,NULL,0,0,PM_REMOVE))
    {
    TranslateMessage(&Symsg);
    DispatchMessage(&Symsg);
    }
}
#endif
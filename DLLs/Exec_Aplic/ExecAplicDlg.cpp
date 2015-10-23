// ExecAplicDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Exec_Aplic.h"
#include "ExecAplicDlg.h"
#include "Utils.h"
#include "ModParam.h"
#include "XmlCreate.h"
#include "ModuleInfo.h"


extern HWND g_hWnd;
CMsgWindow* CExecAplicDlg::m_wnd;


// CExecAplicDlg dialog

IMPLEMENT_DYNAMIC(CExecAplicDlg, CDialogEx)


CExecAplicDlg::CExecAplicDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CExecAplicDlg::IDD, pParent)
{

}

CExecAplicDlg::~CExecAplicDlg()
{
}

void CExecAplicDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, ID_EXEC_APLIC_LST_ALL, m_lstAll);
	DDX_Control(pDX, ID_EXEC_APLIC_LST_SELECTED, m_lstSelecteds);
}


BEGIN_MESSAGE_MAP(CExecAplicDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_SAIR, &CExecAplicDlg::OnBnClickedButtonSair)
	ON_BN_CLICKED(IDC_BUTTON_EXECUTE_1, &CExecAplicDlg::OnBnClickedButtonExecute1)
	///ID_FECHAR_APLIC
	ON_COMMAND(ID_CONCLUIR_APLIC, OnSair)
	ON_COMMAND(ID_CANCELAR_APLIC, OnCancel)
	ON_BN_CLICKED(IDC_BUTTON_INCLUI, &CExecAplicDlg::OnBnClickedButtonInclui)
	ON_BN_CLICKED(IDC_BUTTON_EXCLUI, &CExecAplicDlg::OnBnClickedButtonExclui)
END_MESSAGE_MAP()


// CExecAplicDlg message handlers

void CExecAplicDlg::OnBnClickedButtonSair()
{
	OnSair();
}

void CExecAplicDlg::OnSair()
{
	CreateXMLAplics();
	EndDialog(IDOK);
}

BOOL CExecAplicDlg::OnInitDialog()
{
	g_hWnd = GetSafeHwnd();
	m_wnd->Show(L"Carregando Dados");

	CDialogEx::OnInitDialog();

#ifdef _WIN32_WCE

	if (!m_dlgCommandBar.Create(this) || !m_dlgCommandBar.InsertMenuBar(IDR_MENU_APLIC))
	{
		///STLOG_WRITE("CInfoAitDlg::OnInitDialog() : Failed to create CommandBar");
		return FALSE;      // fail to create
	}


	HideSIP();
	
	
	_CreateBanner(L"Aplicativos Externos");	
#endif


	m_wnd->Destroy();

	if(GetParent() != NULL)
		GetParent()->PostMessage(CModParam::WM_MODULE_READY, 0, (LPARAM) GetSafeHwnd());

	_FullScreen();
	m_lstAll.InsertColumn(0, L"Aplicativos", LVCFMT_LEFT, DRA::SCALEX(200));
	m_lstSelecteds.InsertColumn(0, L"Aplicativos configurados", LVCFMT_LEFT, DRA::SCALEX(200));
	_FillList();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}


/**
\brief Pesquisa atalhos para aplicativos na pata '\windows\Start Menu\' e preenche lista com os nomes dos atalhos
\details 
\param void
\return BOOL
*/
BOOL CExecAplicDlg::_FillList()
{
	m_lstAll.DeleteAllItems();
	m_lstAtalhos.RemoveAll();
	TCHAR szFolder[MAX_PATH];
	CString sPath = L"";

	SHGetSpecialFolderPath(GetSafeHwnd(), szFolder, CSIDL_PROGRAMS, FALSE);
	sPath.Format(L"%s", szFolder);
	ListaAtalhos(sPath);

	int pos = sPath.ReverseFind('\\');
	sPath = sPath.Mid(0, pos);

	ListaAtalhos(sPath);
	POSITION p = m_lstAtalhos.GetHeadPosition();
	while(p)
	{			
		CTwoFieldsApl inc = m_lstAtalhos.GetNext(p);	
		int idx = m_lstAll.InsertItem(m_lstAll.GetItemCount(), inc.sFileName);						
		m_lstAll.SetItemData(idx, idx);

		m_lstAll.SetRedraw(TRUE);
		m_lstAll.RedrawWindow(NULL, NULL);
		
	}

	LoadXMLAplics();

	POSITION ps = m_lstAtalhosSel.GetHeadPosition();
	while(ps)
	{			
		CTwoFieldsApl inc = m_lstAtalhosSel.GetNext(ps);	
		int idx = m_lstSelecteds.InsertItem(m_lstSelecteds.GetItemCount(), inc.sFileName);						
		m_lstSelecteds.SetItemData(idx, idx);

		ExcluiItemListaTotal(inc.sFileName);

		m_lstSelecteds.SetRedraw(TRUE);
		m_lstSelecteds.RedrawWindow(NULL, NULL);
		
	}


	return TRUE;
}




void CExecAplicDlg::OnBnClickedButtonExecute1()
{

}

void CExecAplicDlg::OnBnClickedButtonInclui()
{
	long lData = 0;
	CString teste = L"";

	POSITION p = m_lstAll.GetFirstSelectedItemPosition();
	int iSelected = m_lstAll.GetNextSelectedItem(p);
	if ( iSelected != CB_ERR )
	{
		teste = m_lstAll.GetItemText(iSelected,0);
		//lData = m_cmbSubTipo.GetItemData(iSelected);
		BOOL jaIncluido = FALSE;
		if (!teste.IsEmpty())
		{
			for (int i=0; i < m_lstSelecteds.GetItemCount(); i++)
			{
				if (teste.CompareNoCase(m_lstSelecteds.GetItemText(i,0)) == 0)
				{
					jaIncluido = TRUE;

					////MessageBox(L"Atalho já inserido.",
					////		   L"Atenção",
					////		   MB_ICONERROR|MB_OK);
					break;
				}
			}
		}
		if (!jaIncluido)
		{
			int idx = m_lstSelecteds.InsertItem(m_lstSelecteds.GetItemCount(), teste);						
			///m_lstSelecteds.SetItemText(idx, 0, teste);
			m_lstSelecteds.SetItemData(idx, idx);
			POSITION pt = m_lstAtalhos.FindIndex(iSelected);
			CTwoFieldsApl item = m_lstAtalhos.GetAt(pt);

			m_lstAtalhosSel.AddTail(item);

			ExcluiItemListaTotal(teste);
		}
	}
}

void CExecAplicDlg::OnBnClickedButtonExclui()
{
	POSITION pos = m_lstSelecteds.GetFirstSelectedItemPosition();
	if (pos != NULL)
	{
		////if (MessageBox(L"Tem certeza que deseja excluir a característica selecionada",
		////		   L"Incidente",
		////		   MB_ICONQUESTION|MB_YESNO) == IDYES)
		{
			int nItem = m_lstSelecteds.GetNextSelectedItem(pos);
			CString sAtalho = m_lstSelecteds.GetItemText(nItem,0);
			m_lstSelecteds.DeleteItem(nItem);
		
			int idxAll = m_lstAll.InsertItem(m_lstAll.GetItemCount(), sAtalho);						
			///m_lstAll.SetItemText(idxAll, 0, sAtalho);
			m_lstAll.SetItemData(idxAll, idxAll);


			POSITION px = m_lstAtalhosSel.FindIndex(nItem);

			CTwoFieldsApl itemAll = m_lstAtalhosSel.GetAt(px);
			m_lstAtalhos.SetAt(px, itemAll);

			m_lstAtalhosSel.RemoveAt(px);
			///InformaTotal();

		}

	}	

}


void CExecAplicDlg::CreateXMLAplics()
{
	/*** Início construção do XML de envio de reg LOGIN_LOGOUT ***/
	CXmlCreate xml;
	CString sBlockXmlAplic = L"<?xml version=\"1.0\" encoding=\"ISO-8859-1\" standalone=\"yes\" ?>";
	sBlockXmlAplic += L"<modules>";
	int iId = 17400;
	POSITION p = m_lstAtalhosSel.GetHeadPosition();
	while (p)
	{
		CTwoFieldsApl item = m_lstAtalhosSel.GetNext(p);
		//item.
		CString sFullPath = item.sPath;
		sBlockXmlAplic.AppendFormat(L"<module path=\"%s\" text=\"%s\" id=\"%d\" />", sFullPath,item.sFileName,iId--);
		//sBlockXmlAplic += L"<module path=\"\\Windows\\iexplore.exe\" text=\"Internet Explorer\" id=\"17340\" />";

	}
	sBlockXmlAplic += L"</modules>";
	///xml.EnableDoctypeDeclHandler();
	//////xml.OpenRootTag(L"modules");
	//////xml.AddElement(L"module", L"path=\"\\Windows\\iexplore.exe\" text=\"Internet Explorer\" id=\"17340\" />");
	//////xml.CloseRootTag(L"modules");
	xml.SetWithHeader(FALSE);
	xml.DeleteAllItems();
	xml.SetXMLStruct(sBlockXmlAplic);
	/*** Final da construção do XML ***/		

	//Valida estrutura do XML criado
	if(1/*xml.ValidateXml()*/)
	{	
		//Cria diretório XML_LOGIN do httpsender se não existir
		CString sBaseDir = CUtil::GetMainAppPath() ;	
		//CUtil::CreateDirIfNotExist(sBaseDir);



		//CString sTimeStamp = CUtil::GetCurrentTimeStamp();
		CString sPathXmlFile;
		sPathXmlFile.Format(L"%s\\confaplic.xml",sBaseDir);
		xml.CreateXmlFile(sPathXmlFile, FALSE);

	}
}


BOOL CExecAplicDlg::LoadXMLAplics()
{
	CModuleInfo		modules;
	CString s;
	s.Format(_T("%s\\confaplic.xml"), CUtil::GetMainAppPath());

	__MenuItem *pParentItem = new __MenuItem();
	// Fazer o parse e carregar os modulos para o menu...
	modules.SetCurrentItem( pParentItem );
	if(!modules.LoadXML(s, TRUE))
	{
		///CPMADlg::m_pSplash.Hide();
		///_ShowError(L"Erro Carregando aplicação.");
		//STLOG_WRITE("Erro Carregando dados de aplicativos externos. Processamento do XML");
		return FALSE;
	}
	///theApp._LoadIcons(pParentItem);

	POSITION p = pParentItem->m_children.GetHeadPosition();
	while(p)
	{
		__MenuItem *pItem = pParentItem->m_children.GetNext(p);
		if(pItem != NULL)
		{
			if(pItem->nType == __MenuItem::TYPE_ITEM)
			{
				CTwoFieldsApl apl;
				apl.sFileName = pItem->sText;
				apl.sPath = pItem->sModulePath;

				m_lstAtalhosSel.AddTail(apl);
				///int idx = m_lstSelecteds.InsertItem(m_lstSelecteds.GetItemCount(), teste);
			}
		}
	}
	return TRUE;
}


BOOL CExecAplicDlg::ExcluiItemListaTotal(CString sAtalho)
{
	LVFINDINFO fInfo;
	fInfo.psz = sAtalho;
	fInfo.flags = LVFI_STRING ;
	int idxAll = m_lstAll.FindItem(&fInfo);
	POSITION pa = m_lstAtalhos.FindIndex(idxAll);
	m_lstAtalhos.RemoveAt(pa);
	m_lstAll.DeleteItem(idxAll);
	return TRUE;
}

BOOL CExecAplicDlg::ListaAtalhos(CString sPath)
{
	CTwoFieldsApl item_lst;
	WIN32_FIND_DATA FindData;
	HANDLE hFind;
	hFind = FindFirstFile(sPath + L"\\*.lnk", &FindData);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		DWORD erro = GetLastError();
		return FALSE;
	}
	CString sFileName = FindData.cFileName;
	CString sFullPath = sPath + L"\\" + sFileName;
	sFileName.Replace(L".lnk", L"");
	item_lst.sFileName = sFileName;
	item_lst.sPath = sFullPath;
	m_lstAtalhos.AddTail(item_lst);

	while (FindNextFile(hFind, &FindData))
	{     
		sFileName = FindData.cFileName;
		sFullPath.Empty();
		sFullPath = sPath + L"\\" + sFileName;
		sFileName.Replace(L".lnk", L"");
		item_lst.sFileName = sFileName;
		item_lst.sPath = sFullPath;
		m_lstAtalhos.AddTail(item_lst);
	}           
	
	FindClose(hFind); 
	return TRUE;

}



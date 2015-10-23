// Mensagens.cpp : implementation file
//

#include "stdafx.h"
#include "Data_Hora.h"
#include "Mensagens.h"
#include "Utils.h"


// CMensagens dialog

IMPLEMENT_DYNAMIC(CMensagens, CDialogEx)

CMensagens::CMensagens(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMensagens::IDD, pParent)
{
	m_sFillArray = NULL;
}

CMensagens::~CMensagens()
{
}

void CMensagens::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MSG, m_txtConteudo);
	DDX_Control(pDX, IDC_BNTODOS, m_Todos);
	DDX_Control(pDX, IDC_BNAGENTE, m_Agente);
	DDX_Control(pDX, IDC_BNEQUIP, m_Equip);
	DDX_Control(pDX, IDC_GRID, m_txtLista);
}


BEGIN_MESSAGE_MAP(CMensagens, CDialog)
	ON_COMMAND(ID_SAIR, &CMensagens::OnSair)
	ON_COMMAND(ID_M_LIDA, &CMensagens::OnMarcaLida)
	ON_COMMAND(ID_M_NLIDA, &CMensagens::OnMarcaNaoLida)
	ON_COMMAND(ID_M_EXCLUIR, &CMensagens::OnExcluir)
	ON_BN_CLICKED(IDC_BNTODOS, &CMensagens::OnBnClickedBntodos)
	ON_BN_CLICKED(IDC_BNAGENTE, &CMensagens::OnBnClickedBnagente)
	ON_BN_CLICKED(IDC_BNEQUIP, &CMensagens::OnBnClickedBnequip)
	ON_NOTIFY(NM_CLICK, 0x500, &CMensagens::OnGridClick)
END_MESSAGE_MAP()

// OnInitDialog Funcion

BOOL CMensagens::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	_CreateBanner(L"Mensagens");

	if (!m_dlgMenuBar.Create(this) ||
		!m_dlgMenuBar.InsertMenuBar(IDR_MENU4))
	{
		STLOG_WRITE("CMensagensDlg::OnInitDialog() : Failed to create MenuBar");
		return FALSE;      // fail to create
	}

	// TODO:  Add extra initialization here

	m_txtLista.GetWindowRect(r1);

	m_Msggrid.Create(r1, this, 0x500);
	m_Msggrid.ShowWindow(SW_SHOW);

	// Desenha Grid

	CMensagens::CriaGrid();
	CMensagens::OnBnClickedBntodos();

	if(GetParent() != NULL)
	{
		GetParent()->PostMessage(CModParam::WM_MODULE_READY, 
								 0, 
								 (LPARAM) GetSafeHwnd());
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE

}

	// Cria Grid

void CMensagens::CriaGrid()
{

	if(m_Msggrid.GetCell(1,1))
		m_Msggrid.DeleteAllItems();

	m_Msggrid.SetFixedColumnCount(0);
	m_Msggrid.SetFixedRowCount(1);
	m_Msggrid.SetColumnCount(2);
	m_Msggrid.SetColumnWidth(0, DRA::SCALEX(75));
	m_Msggrid.SetColumnWidth(1, DRA::SCALEX(160));
	m_Msggrid.SetEditable(FALSE);
	m_Msggrid.GetCell(0,0)->SetText(L"Para");
	m_Msggrid.GetCell(0,1)->SetText(L"Data");

}

// CMensagens message handlers

void CMensagens::OnSair()
{
	EndDialog(IDOK);
}

void CMensagens::OnMarcaLida()
{
	
	font = m_Msggrid.GetFont();
	font->GetLogFont(&logfont);
	logfont.lfWeight=FW_LIGHT;

	CCellRange crange;
	int id,i;
	CStringA data;

	//font = m_Msggrid.GetFont();
	//font->GetLogFont(&logfont);
	//logfont.lfWeight=FW_BOLD;

	crange = m_Msggrid.GetSelectedCellRange();
	
	if(crange.GetMaxRow() >= 1)
	{
		for(i = crange.GetMinRow(); i <= crange.GetMaxRow(); i++)
		{
			id = m_Msggrid.GetCell(i,0)->GetData();
			data = CUtil::GetCurrentDateTime(L"DATA_HORA");

			CStringA sQuery;
			sQuery.Format("SELECT [data_hora_vis], [status] from mensagem where [codigo] = %d",id);
			try
			{
				q = CppSQLite3DB::getInstance()->execQuery(sQuery);
			}
			catch(CppSQLite3Exception e)
			{
				STLOG_WRITE("%s(%d): Erro executando query. Motivo: %s", __FUNCTION__, __LINE__, e.errorMessage());
				//return;
			}
			
			CTwoFields* fields;

			fields = new CTwoFields(q.getStringField(0), q.getStringField(1));

			if(fields->sFieldTwo == "0")
			{
				sQuery.Format("UPDATE mensagem SET [data_hora_vis] = '%s', [status] = 1 where [codigo] = %d",data,id);

				try
				{
					q = CppSQLite3DB::getInstance()->execQuery(sQuery);
				}
				catch(CppSQLite3Exception e)
				{
					STLOG_WRITE("%s(%d): Erro executando query. Motivo: %s", __FUNCTION__, __LINE__, e.errorMessage());
					//return;
				}
			}
			m_Msggrid.GetCell(i,0)->SetFont(&logfont);
			m_Msggrid.GetCell(i,1)->SetFont(&logfont);
		}
		m_Msggrid.Refresh();
	}
	else
		MessageBox(L"Selecione uma mensagem",L"Erro",MB_OK);

}

void CMensagens::OnMarcaNaoLida()
{

	font = m_Msggrid.GetFont();
	font->GetLogFont(&logfont);
	logfont.lfWeight=FW_BOLD;

	CCellRange crange;
	int id,i;

	crange = m_Msggrid.GetSelectedCellRange();
	
	if(crange.GetMaxRow() >= 1)
	{
		for(i = crange.GetMinRow(); i <= crange.GetMaxRow(); i++)
		{
			id = m_Msggrid.GetCell(i,0)->GetData();

			CStringA sQuery;
			sQuery.Format("SELECT [data_hora_vis], [status] from mensagem where [codigo] = %d",id);
			try
			{
				q = CppSQLite3DB::getInstance()->execQuery(sQuery);
			}
			catch(CppSQLite3Exception e)
			{
				STLOG_WRITE("%s(%d): Erro executando query. Motivo: %s", __FUNCTION__, __LINE__, e.errorMessage());
				//return;
			}
			
			CTwoFields* fields;

			fields = new CTwoFields(q.getStringField(0), q.getStringField(1));

			if(fields->sFieldTwo == "1")
			{
				sQuery.Format("UPDATE mensagem SET [data_hora_vis] = '', [status] = 0 where [codigo] = %d",id);

				try
				{
					q = CppSQLite3DB::getInstance()->execQuery(sQuery);
				}
				catch(CppSQLite3Exception e)
				{
					STLOG_WRITE("%s(%d): Erro executando query. Motivo: %s", __FUNCTION__, __LINE__, e.errorMessage());
					//return;
				}
			}
			m_Msggrid.GetCell(i,0)->SetFont(&logfont);
			m_Msggrid.GetCell(i,1)->SetFont(&logfont);
		}
		m_Msggrid.Refresh();
	}
	else
		MessageBox(L"Selecione uma mensagem",L"Erro",MB_OK);

}

void CMensagens::OnExcluir()
{

	CCellRange crange;
	int id,i,res;
	CStringA sQuery;

	crange = m_Msggrid.GetSelectedCellRange();

	if(crange.GetMaxRow() >= 1)
		res = MessageBox(L"Deseja excluir as mensagens selecionadas?",L"Excluir mensagem?",MB_YESNO);
	else
		MessageBox(L"Selecione uma mensagem",L"Erro",MB_OK);
	if(res == IDYES)
	{
		for(i = crange.GetMinRow(); i <= crange.GetMaxRow(); i++)
		{
			id = m_Msggrid.GetCell(i,0)->GetData();

			sQuery.Format("DELETE FROM mensagem where [codigo] = %d",id);

			try
			{
				q = CppSQLite3DB::getInstance()->execQuery(sQuery);
			}
			catch(CppSQLite3Exception e)
			{
				STLOG_WRITE("%s(%d): Erro executando query. Motivo: %s", __FUNCTION__, __LINE__, e.errorMessage());
				//return;
			}
		}
		for(i = crange.GetMinRow(); i <= crange.GetMaxRow(); i++)
		{
			m_Msggrid.DeleteRow(crange.GetMinRow());
		}
		m_Msggrid.Refresh();
	}

	SetDlgItemText(IDC_MSG,L"");

}

	// Botão TODOS

void CMensagens::OnBnClickedBntodos()
{

	CMensagens::CriaGrid();

	font = m_Msggrid.GetFont();
	font->GetLogFont(&logfont);
	logfont.lfWeight=FW_BOLD;

	m_sFillArray = new __2FIELDS();

	CStringA sQuery;
	sQuery.Format("SELECT [codigo], [id_talao], [cd_agente], [data_hora_cad], [status] FROM mensagem");

	try
	{
		q = CppSQLite3DB::getInstance()->execQuery(sQuery);
	}
	catch(CppSQLite3Exception e)
	{
		STLOG_WRITE("%s(%d): Erro executando query. Motivo: %s", __FUNCTION__, __LINE__, e.errorMessage());
		//return;
	}

	CTwoFields* fields;
	int row = 0;
	CString sCampo, sData, sHora;

	while(!q.eof())
	{
		fields = new CTwoFields(q.getStringField(2), q.getStringField(3));
		if(fields->sFieldOne == "")
		{
			fields = new CTwoFields(q.getStringField(1), q.getStringField(3));
			sCampo = CString(fields->sFieldOne);
			row = m_Msggrid.InsertRow(sCampo);
		}
		else
		{
			sCampo = CString(fields->sFieldOne);
			if(sCampo == CUtil::GetLoggedUser())
			{
				row = m_Msggrid.InsertRow(sCampo);
			}
			else
				row = 0;
		}
		if(row != 0)
		{
			sCampo = CString(fields->sFieldTwo);
			sData = sCampo.Left(10);
			sHora = sCampo.Right(8);
			sCampo.Format(sData.Right(2) + sData.Mid(4,4) + sData.Left(4) + _T(" ") + sHora);
			m_Msggrid.SetItemText(row,1,sCampo);

			fields = new CTwoFields(q.getStringField(0), q.getStringField(4));
			m_Msggrid.GetCell(row, 0)->SetData(atoi(fields->sFieldOne));
			//m_Msggrid.GetCell(row, 1)->SetFormat(DT_WORDBREAK);	// Exibe apenas data

			if(fields->sFieldTwo == "0")
			{
				m_Msggrid.GetCell(row,0)->SetFont(&logfont);
				m_Msggrid.GetCell(row,1)->SetFont(&logfont);
			}

			m_sFillArray->AddTail(fields);
		}

		q.nextRow();
	}

	q.finalize();

	m_Msggrid.SetGridLines();

	CConsultas::Destroy2FIELDS(m_sFillArray);
	m_sFillArray = NULL;

	SetDlgItemText(IDC_MSG,L"");

}

	// Botão AGENTE

void CMensagens::OnBnClickedBnagente()
{

	CMensagens::CriaGrid();

	font = m_Msggrid.GetFont();
	font->GetLogFont(&logfont);
	logfont.lfWeight=FW_BOLD;

	m_sFillArray = new __2FIELDS();

	CStringA sQuery;
	sQuery.Format("SELECT [codigo], [cd_agente], [data_hora_cad], [status] FROM mensagem");

	try
	{
		q = CppSQLite3DB::getInstance()->execQuery(sQuery);
	}
	catch(CppSQLite3Exception e)
	{
		STLOG_WRITE("%s(%d): Erro executando query. Motivo: %s", __FUNCTION__, __LINE__, e.errorMessage());
		//return;
	}

	CTwoFields* fields;
	int row = 0;
	CString sCampo, sData, sHora;

	while(!q.eof())
	{
		fields = new CTwoFields(q.getStringField(1), q.getStringField(2));
		sCampo = CString(fields->sFieldOne);
		if(sCampo == CUtil::GetLoggedUser())
			row = m_Msggrid.InsertRow(sCampo);
		else
			row = 0;

		if(row != 0)
		{
			sCampo = CString(fields->sFieldTwo);
			sData = sCampo.Left(10);
			sHora = sCampo.Right(8);
			sCampo.Format(sData.Right(2) + sData.Mid(4,4) + sData.Left(4) + _T(" ") + sHora);
			m_Msggrid.SetItemText(row,1,sCampo);
			m_Msggrid.SetItemText(row,1,sCampo);

			fields = new CTwoFields(q.getStringField(0), q.getStringField(3));
			m_Msggrid.GetCell(row, 0)->SetData(atoi(fields->sFieldOne));
			//m_Msggrid.GetCell(row, 1)->SetFormat(DT_WORDBREAK);	// Exibe apenas data

			if(fields->sFieldTwo == "0")
			{
				m_Msggrid.GetCell(row,0)->SetFont(&logfont);
				m_Msggrid.GetCell(row,1)->SetFont(&logfont);
			}

			m_sFillArray->AddTail(fields);

		}

			q.nextRow();

	}

	q.finalize();

	m_Msggrid.SetGridLines();

	CConsultas::Destroy2FIELDS(m_sFillArray);
	m_sFillArray = NULL;

	SetDlgItemText(IDC_MSG,L"");

}

	// Botão EQUIPAMENTO

void CMensagens::OnBnClickedBnequip()
{

	CMensagens::CriaGrid();

	font = m_Msggrid.GetFont();
	font->GetLogFont(&logfont);
	logfont.lfWeight=FW_BOLD;

	m_sFillArray = new __2FIELDS();

	CStringA sQuery;
	sQuery.Format("SELECT [codigo], [id_talao], [cd_agente], [data_hora_cad], [status] FROM mensagem");

	try
	{
		q = CppSQLite3DB::getInstance()->execQuery(sQuery);
	}
	catch(CppSQLite3Exception e)
	{
		STLOG_WRITE("%s(%d): Erro executando query. Motivo: %s", __FUNCTION__, __LINE__, e.errorMessage());
		//return;
	}

	CTwoFields* fields;
	int row = 0;
	CString sCampo, sData, sHora;

	while(!q.eof())
	{
		fields = new CTwoFields(q.getStringField(2), q.getStringField(3));
		sCampo = CString(fields->sFieldOne);
		if(sCampo == "")
		{
			fields = new CTwoFields(q.getStringField(1), q.getStringField(3));
			sCampo = CString(fields->sFieldOne);

			row = m_Msggrid.InsertRow(sCampo);
			
			sCampo = CString(fields->sFieldTwo);
			sData = sCampo.Left(10);
			sHora = sCampo.Right(8);
			sCampo.Format(sData.Right(2) + sData.Mid(4,4) + sData.Left(4) + _T(" ") + sHora);
			m_Msggrid.SetItemText(row,1,sCampo);
			m_Msggrid.SetItemText(row,1,sCampo);

			fields = new CTwoFields(q.getStringField(0), q.getStringField(4));
			m_Msggrid.GetCell(row, 0)->SetData(atoi(fields->sFieldOne));
			//m_Msggrid.GetCell(row, 1)->SetFormat(DT_WORDBREAK);	// Exibe apenas data

			if(fields->sFieldTwo == "0")
			{
				m_Msggrid.GetCell(row,0)->SetFont(&logfont);
				m_Msggrid.GetCell(row,1)->SetFont(&logfont);
			}

			m_sFillArray->AddTail(fields);

		}

		q.nextRow();

	}

	q.finalize();

	m_Msggrid.SetGridLines();

	CConsultas::Destroy2FIELDS(m_sFillArray);
	m_sFillArray = NULL;

	SetDlgItemText(IDC_MSG,L"");

}

	// On Grid Click

void CMensagens::OnGridClick(NMHDR *pNotifyStruct, LRESULT* pResult)
{

	CCellID i;
	int id;
	CString mensagem;
	CStringA data;

	font = m_Msggrid.GetFont();
	font->GetLogFont(&logfont);
	logfont.lfWeight=FW_LIGHT;

	// Localiza a mensagem selecionada

	i = m_Msggrid.GetFocusCell();
	id = m_Msggrid.GetCell(i.row,0)->GetData();

	// Executa query

	CStringA sQuery;
	sQuery.Format("SELECT [descricao], [status] FROM mensagem where [codigo] = %d",id);

	try
	{
		q = CppSQLite3DB::getInstance()->execQuery(sQuery);
	}
	catch(CppSQLite3Exception e)
	{
		STLOG_WRITE("%s(%d): Erro executando query. Motivo: %s", __FUNCTION__, __LINE__, e.errorMessage());
		//return;
	}

	// Grava dados da mensagem nas variáveis

	CTwoFields* fields;

	fields = new CTwoFields(q.getStringField(0), q.getStringField(1));
	mensagem = CString(fields->sFieldOne);

	// Exibe a mensagem

	SetDlgItemTextW(IDC_MSG,mensagem);

	if(fields->sFieldTwo == "0")
	{

		data = CUtil::GetCurrentDateTime(L"DATA_HORA");
		sQuery.Format("UPDATE mensagem SET [data_hora_vis] = \"%s\", [status] = 1 where [codigo] = %d",data,id);

		try
		{
			q = CppSQLite3DB::getInstance()->execQuery(sQuery);
		}
		catch(CppSQLite3Exception e)
		{
			STLOG_WRITE("%s(%d): Erro executando query. Motivo: %s", __FUNCTION__, __LINE__, e.errorMessage());
			//return;
		}
		
		m_Msggrid.GetCell(i.row,0)->SetFont(&logfont);
		m_Msggrid.GetCell(i.row,1)->SetFont(&logfont);
		m_Msggrid.Refresh();

	}

}

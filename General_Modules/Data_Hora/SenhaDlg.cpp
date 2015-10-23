// Data_HoraDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Data_Hora.h"
#include "SenhaDlg.h"
#include "CppSQLite3.h"
#include "ModParam.h"
#include "Utils.h"
#include "SimpleRequest.h"
#include "CStr.h"
#include "XmlCreate.h"


// CSenhaDlg dialog

IMPLEMENT_DYNAMIC(CSenhaDlg, CDialogEx)

CSenhaDlg::CSenhaDlg(CppSQLite3DB *pDB, CWnd* pParent /*=NULL*/)
	: CDialogEx(CSenhaDlg::IDD, pParent)
{
	m_pDB = pDB;
}

CSenhaDlg::~CSenhaDlg()
{
}

void CSenhaDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TXT_ANTIGA, m_txtSenhaAntiga);
	DDX_Control(pDX, IDC_TXT_NOVA, m_txtSenhaNova);
	DDX_Control(pDX, IDC_TXT_CONFIRMA, m_txtSenhaConfirma);
}


BEGIN_MESSAGE_MAP(CSenhaDlg, CDialogEx)
	ON_COMMAND(ID_ALTERAR, &CSenhaDlg::OnAlterar)
END_MESSAGE_MAP()


// CSenhaDlg message handlers

BOOL CSenhaDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

#ifdef _WIN32_WCE
	if (!m_dlgCommandBar.Create(this) ||
	    !m_dlgCommandBar.InsertMenuBar(IDR_MENU2))
	{
		STLOG_WRITE("CConsLoteDlg::OnInitDialog() : Failed to create CommandBar");
		return FALSE;      // fail to create
	}

	_CreateBanner(L"Alterar Senha");	
#endif

	_FullScreen();

	if(GetParent() != NULL)
		GetParent()->PostMessage(CModParam::WM_MODULE_READY, 0, (LPARAM) GetSafeHwnd());

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CSenhaDlg::OnAlterar()
{
	
	CString sAntiga, sNova, sConfirma;

	m_txtSenhaAntiga.GetWindowText(sAntiga);
	m_txtSenhaNova.GetWindowText(sNova);
	m_txtSenhaConfirma.GetWindowText(sConfirma);

	if(sAntiga.IsEmpty() || sNova.IsEmpty() || sConfirma.IsEmpty())
	{
		MessageBox(L"Todos os campos são de preenchimento obrigatório", L"Erro", MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	if(!_ValidaSenha(sAntiga))
	{
		MessageBox(L"A senha antiga digitada está incorreta", L"Erro", MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	if(sAntiga.CompareNoCase(sNova)==0)
	{
		MessageBox(L"Senha nova deve ser diferente da senha antiga", L"Erro", MB_OK | MB_ICONEXCLAMATION);
		return;
	}

	if(sNova.CompareNoCase(sConfirma)!=0)
	{	
		MessageBox(L"Senha nova e a confirmação estão diferentes!", L"Erro", MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	CString sContrato = m_params->GetValue(L"contrato");
	CString sIDTalao = CUtil::GetIDTalao();
	CString sAgente = m_params->GetValue(L"codigo");

	CString m_sData = CUtil::GetCurrentDateTime(L"DATA");
	/*** Início construção do XML de alteração de SENHA ***/
	CXmlCreate xml;	
	xml.OpenRootTag(L"senha");
	xml.AddElement(L"contrato", sContrato);
	xml.AddElement(L"id_talao", sIDTalao);
	xml.AddElement(L"agente", sAgente);
	xml.AddElement(L"senha_atual", sAntiga);
	xml.AddElement(L"senha_nova", sNova);
	xml.AddElement(L"data_alteracao_senha", m_sData);
	xml.CloseRootTag(L"senha");
	/*** Final da construção do XML ***/		

	//Valida estrutura do XML criado
	if(xml.ValidateXml())
	{
		//Cria diretório XML_LOGIN do httpsender se não existir
		CString sBaseDir = CUtil::GetMainAppPath() + L"\\SENHA_XML";	
		CUtil::CreateDirIfNotExist(sBaseDir);



		CString sTimeStamp = CUtil::GetCurrentTimeStamp();
		CString sPathXmlFile;
		//sPathXmlFile.Format(L"LOGIN_LOGOUT_%s.xml", sTimeStamp);
		sPathXmlFile.Format(L"%s\\SENHA_%s.xml",sBaseDir, sTimeStamp);
		xml.CreateXmlFile(sPathXmlFile);

		CString sLastId;
		///sLastId.Format(L"%d",m_iLastId);
		////CString sVarsList ;
		////sVarsList.AppendFormat(L"contrato=%s", sContrato);

		//Cria hash do arquivo XML	
		CString sXmlFile = xml.GetXml();		
		CString sXmlMD5;
		if(!CUtil::MakeHash(CStr(sXmlFile), L"MD5", &sXmlMD5))
		{
			STLOG_WRITE(L"CSenhaDlg::OnAlterar() Erro calculando hash do arquivo XML!");
		}	

		//Vars http-post
		CString sVarsList;
		sVarsList.AppendFormat(L"contrato=%s|cd_equipamento=%s|MD5Xml=%s", sContrato, sIDTalao, sXmlMD5);

		{
			///if(CConsultasHttpSender::GravaHttpSenderJob(CppSQLite3DB::getInstance(), L"LOGIN_LOGOUT", sPathXmlFile, L"XML", sVarsList, L"UPDATE_TRANSMISSAO", L"LOG_AGENTE", L"Id", sLastId))
			///if(CConsultasHttpSender::GravaHttpSenderJob(CppSQLite3DB::getInstance(), L"SENHA",        sPathXmlFile, L"XML", sVarsList, L"UPDATE_SENHA", L"AGENTE", L"Codigo", sAgente))
			if(CConsultasHttpSender::GravaHttpSenderJob(CppSQLite3DB::getInstance(), L"SENHA",  sPathXmlFile, L"XML", sVarsList, L"", L"", L"", L""))
			{
				STLOG_WRITE(L"CSenhaDlg::OnAlterar() Inserido registro job de envio de troca de SENHA");
				//m_stDateLastJob = st;
				//m_sDateLastJob = m_sDataHora;


				if(!_AlteraSenha(sNova, m_sData))
				{
					MessageBox(L"Erro tentando alterar senha. Tente novamente.", L"Erro", MB_OK | MB_ICONEXCLAMATION);
					return;	
				}

			}
		}
	}

	EndDialog(ID_LOGOUT);

}


BOOL CSenhaDlg::_ValidaSenha(LPCTSTR szAntiga)
{
	CStringA s;
	s.Format("SELECT count(*) AS X "
		     "  FROM AGENTE		"
			 " WHERE codigo = '%S' "
			"   AND senha='%S'	",
			m_params->GetValue(L"codigo"),
			szAntiga);
	try
	{
		CppSQLite3Query q = CppSQLite3DB::getInstance()->execQuery(s);
		if(!q.eof())
		{
			if(q.getIntField("X") > 0)
			{
				STLOG_WRITE("CSenhaDlg::_ValidaSenha(): Existe usuário");
				return TRUE;
			}
		}
	}
	catch(CppSQLite3Exception e)
	{
		MessageBox(L"Erro acessando o banco de dados", L"Mensagem", MB_ICONERROR|MB_OK);

		STLOG_WRITE(L"CSenhaDlg::_ValidaSenha(): %S", e.errorMessage());
		
	}

	return FALSE;
}

BOOL CSenhaDlg::_AlteraSenha(LPCTSTR szNova, LPCTSTR szDataAlteracao)
{
	CStringA s;
	s.Format("UPDATE AGENTE "
		    "SET senha='%S', data_ultima_troca='%S', data_limite_troca = 0" 
			" WHERE codigo = '%S' ", szNova, szDataAlteracao, m_params->GetValue(L"codigo"));
	try
	{
		CppSQLite3DB::getInstance()->execQuery(s);
		return TRUE;
	}
	catch(CppSQLite3Exception e)
	{
		MessageBox(L"Erro acessando o banco de dados", L"Mensagem", MB_ICONERROR|MB_OK);

		STLOG_WRITE(L"CSenhaDlg::_AlteraSenha(): %S", e.errorMessage());
		
	}

	return FALSE;
}
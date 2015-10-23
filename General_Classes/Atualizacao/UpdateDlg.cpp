#include "stdafx.h"
#include "Atualizacao.h"
#include "UpdateDlg.h"
#include "ModParam.h"
#include "CStr.h"
#include "Utils.h"
#include "TableBase.h"
#include "TableStruct.h"
#include "ProcessSystem.h"
#include "Splash.h"
#include "ContratoTable.h"

#define ID_TIMER_FILL_LIST	 1
#define ID_TIMER_PROCESS	 2
#define ID_TIMER_OLD_FILE    3
#define ID_TIMER_SERVER_FILE 4

CMsgWindow* CUpdateDlg::m_wnd;

IMPLEMENT_DYNAMIC(CUpdateDlg, CDialogEx)

/**
\brief Construtor da classe
\details
	Funções executadas neste módulo:
	- Início de variáveis globais da classe

\param void
*/
CUpdateDlg::CUpdateDlg(LPCTSTR szPath, CWnd* pParent /*=NULL*/)
	: CDialogEx(CUpdateDlg::IDD, pParent)
{
	m_manageDB = new CManageDB();
	m_sPath = szPath;
	m_bCreateDB = FALSE;
	m_params = NULL;
}

/**
\brief Destrutor da classe
\param void
*/
CUpdateDlg::~CUpdateDlg()
{
}

/**
\brief Configura as trocas e validações dos campos desta janela (União de controles e variáveis)
\param CDataExchange* pDx: Ponteiro para a classe que faz essa troca
\return void
*/
void CUpdateDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_list);
	DDX_Control(pDX, IDC_STATIC_ST, m_status);
}


BEGIN_MESSAGE_MAP(CUpdateDlg, CDialogEx)
	ON_WM_TIMER()
END_MESSAGE_MAP()


/**
\brief Início desta janela.
\details	Funções executadas neste módulo:
	- Configurar a janela para se adequar o padrão do PMA:
		- Configuração da barra de comando;
		- Criar o banner da parte superior;
		- Configurar tela cheia;
	- Configurar as colunas da lista que serão mostrados os arquivos a serem baixados e inseridos;
	- Iniciar a passagem de parâmetros a serem utilizados neste módulo;
	- Indicar configurações de proxy;
	- Indicar arquivo de estrutura de dados;
	- Criar o timer para a próxima execução

\param void
\return TRUE se a execução ocorrer com sucesso
*/
BOOL CUpdateDlg::OnInitDialog()
{
	m_wnd = CMsgWindow::getInstance();
	m_wnd->Create(this);
	m_wnd->Show(L"Processando atualizações...");

	CDialogEx::OnInitDialog();

#ifdef _WIN32_WCE
	HideSIP();

	if (!m_dlgCommandBar.Create(this))
	{
		STLOG_WRITE("CUpdateDlg::OnInitDialog() : Failed to create CommandBar");
		return FALSE;      // fail to create
	}

	_CreateBanner(IDB_HEADER, FALSE, 28, L"Atualização de dados", 5);
	m_banner.ShowWindow(SW_SHOW);


	//Prepara lista em que vai aparecer os arquivos e o total
	m_list.InsertColumn( 0, L"Nome Arquivo", LVCFMT_LEFT, DRA::SCALEX(115) );
	m_list.InsertColumn( 1, L"Total", LVCFMT_LEFT, DRA::SCALEX(90) );

	_FullScreen();
#else
	m_list.InsertColumn( 0, L"Nome Arquivo", LVCFMT_LEFT, 115 );
	m_list.InsertColumn( 1, L"Total", LVCFMT_LEFT, 90 );
#endif

	ASSERT(NULL != m_params);

	m_proxyInfo.bDiscagem = m_params->GetValue(_T("DISCAGEM")).CompareNoCase(L"TRUE") == 0;
	m_proxyInfo.bProxy	  = m_params->GetValue(_T("PROXY")).CompareNoCase(L"TRUE") == 0;
	m_proxyInfo.sServer	  = m_params->GetValue(_T("SERVER"));
	m_proxyInfo.nPort	  = _wtol(m_params->GetValue(_T("PORT")));
	m_proxyInfo.sUser	  = m_params->GetValue(_T("USER"));
	m_proxyInfo.sPass	  = m_params->GetValue(_T("PASS"));

	STLOG_WRITE(L"m_proxyInfo.bProxy %ld", m_proxyInfo.bProxy);
	STLOG_WRITE(L"m_proxyInfo.sServer %s", m_proxyInfo.sServer);
	STLOG_WRITE(L"m_proxyInfo.nPort %ld", m_proxyInfo.nPort);
	STLOG_WRITE(L"m_proxyInfo.sUser %s", m_proxyInfo.sUser);

	SetStatus(L"Procurando por atualizações...");

	//Procura pelo arquivo que contém a estrutura das tabelas
	m_sStructPath = CUtil::GetAppPath();
	if(m_sStructPath.GetAt(m_sStructPath.GetLength()-1) != '\\')
		m_sStructPath += L"\\";
	m_sStructPath += L"EstruturaBD.txt";

	STLOG_WRITE("CUpdateDlg::OnInitDialog() : arquivo de estrutura: %S", m_sStructPath);

	if(!CUtil::FileExists(m_sStructPath))
	{
		STLOG_WRITE("CUpdateDlg::OnInitDialog() : Erro arquivo %S nao encontrado", m_sStructPath);
		EndDialog(IDCANCEL);
		return FALSE;
	}

	////m_httpFile.SetProxy(&m_proxyInfo);
	////m_httpFile.SetStructFile(m_sStructPath);

	//_CreateDatabase();

	//Sleep(1000);
	//OnTimer(ID_TIMER_SERVER_FILE);
	
	SetTimer(ID_TIMER_SERVER_FILE, 1000, NULL);

	return TRUE;
}

/**
/brief
	Método encarregado de preencher a lista de atualizações a serem feitas, este método escolhe
	quais arquivos irão aparecer nesta lista.
/details
	Faz a leitura do arquivo de atualizações na web e compara a data e hora da lista 
	de arquivos com a tabela de atualizações, se ambas estiverem diferentes a tabela é
	marcada para atualização
/param void
/return void
*/
BOOL CUpdateDlg::_PreencheLista(LPCTSTR szSystem)
{
	CString strSys = szSystem;
	CProcessSystem sys;

	sys.LoadTable(szSystem, &m_tabMap, &m_indexMap);

	try
	{
		if(!CppSQLite3DB::getInstance()->isOpen())
		{
			CppSQLite3DB::getInstance()->open(CStr(m_sPath));
			//CppSQLite3DB::getInstance()->setBusyHandler();
		}
	}
	catch(CppSQLite3Exception e)
	{
		CString msg;
		msg.LoadString(IDS_ERRO_CRIANDO_DB);

		MessageBox(msg, L"Mensagem", MB_ICONINFORMATION|MB_OK);
		STLOG_WRITE("%s(%d): ERRO ABRINDO BANCO DE DADOS %s", __FUNCTION__, __LINE__, e.errorMessage());
		return FALSE;
	}

	m_manageDB->CreateDatabase(&m_tabMap, &m_indexMap);

	//Insert contrato info
	CContratoTable contrato;
	CContratoInfo info;

	info.sCodAutuador = m_params->GetValue(L"cod_autuador");
	info.sContrato = m_params->GetValue(L"contrato");

	if(!info.sCodAutuador.Trim().IsEmpty())
	{
		contrato.Init();
		contrato.SetValues(&info);
		if(!contrato.Insert(CppSQLite3DB::getInstance()))
		{
			STLOG_WRITE("Erro inserindo dados do contrato no database.");
		}
	}

	_FIELDS_ meu_registro;
	CString arquivo, data ,hora;
	int idx = 0;
	TB_ATUALIZACAO *dadoArquivo = NULL;
	CGetFile *m_httpFile;
	m_httpFile = new CGetFile();

	m_httpFile->SetProxy(&m_proxyInfo);
	m_httpFile->SetStructFile(m_sStructPath);

	m_list.ClearBars();
	m_list.DeleteAllItems();
	m_list.UpdateWindow();
	m_httpFile->SetNomeTabela(L"atualizacao");

	CString sContrato = m_params->GetValue(L"contrato");
	CString sURL;
	CString sVarPma = L"transmite_" + strSys;
	
	sURL = m_params->GetValue(sVarPma);
	if(sURL.IsEmpty())
	{
		STLOG_WRITE("CUpdateDlg::_PreencheLista(): url 'transmite_' esta vazia.");
		return FALSE;
	}

	if(! m_httpFile->GetFileFromServer(sURL, sContrato))
	{
		return FALSE;
	}
	int count = 0;

	while(!m_httpFile->IsEOF())
	{
		m_httpFile->GetRecord(&meu_registro);

		meu_registro.Lookup(L"arquivo",arquivo);
		meu_registro.Lookup(L"data",data);
		meu_registro.Lookup(L"hora",hora);

		if(!arquivo.IsEmpty())
		{
			dadoArquivo = new TB_ATUALIZACAO;
			dadoArquivo->nome = arquivo.Trim();
			dadoArquivo->data = data;
			dadoArquivo->hora = hora;

			m_listaArquivos.SetAt(arquivo, dadoArquivo);
		}

		if(!CppSQLite3DB::getInstance()->isOpen())
		{
			idx = m_list.InsertItem(m_list.GetItemCount(), arquivo);
			m_list.CreateProgress(idx, 1);
		}
		else
		{
			//fazer a consulta no banco de dados
			//verificar se a data do record "data" for maior que a data do banco "data_banco"
			//se for, adicona na lista
			CString sQuery;
			sQuery.Format(L"SELECT arquivo FROM atualizacao WHERE "
						  L"arquivo = '%s' AND data = '%s' AND hora = '%s'",
						  arquivo, 
						  data,
						  hora);
			
			try
			{
				ASSERT(CppSQLite3DB::getInstance());
				CppSQLite3Query q = CppSQLite3DB::getInstance()->execQuery(CStr(sQuery));
				if(q.eof())
				{
					idx = m_list.InsertItem(m_list.GetItemCount(), arquivo);
					m_list.CreateProgress(idx, 1);
				}

				q.finalize();
			}
			catch(CppSQLite3Exception e)
			{
				STLOG_WRITE("CUpdateDlg::_PreencheLista(): %s", e.errorMessage());
				STLOG_WRITE(sQuery);
				STLOG_WRITE("----------------------------------------------------------");
				return FALSE;
			}
		}

		m_httpFile->MoveNext();
	}

	delete(m_httpFile);
	m_list.UpdateWindow();
	return TRUE;
}

/**
\brief Método executado ao destruir esta janela.
\details	Funções executadas neste método:
	- Fechar o banco de dados;
	- Destruir o conteúdo da lista de arquivos;

\param void
\return TRUE se a execução ocorrer do sucesso.
*/
BOOL CUpdateDlg::DestroyWindow()
{
	//CppSQLite3DB::getInstance()->close();

	POSITION p = m_listaArquivos.GetStartPosition();
	while(p)
	{
		TB_ATUALIZACAO *pData;
		CString sKey;
		m_listaArquivos.GetNextAssoc(p, sKey, pData);
		delete pData;
	}

	m_listaArquivos.RemoveAll();

	return CDialogEx::DestroyWindow();
}

/**
\brief Executa os processos de leitura de arquivos e inserção no banco
\details	
	Funções executadas neste método:
	- Ler cada arquivo que necessita de atualização da lista;
	- Indicar o nome do arquivo de estrutura de dados;
	- Apagar os registros das tabelas a serem atualizadas;
	- Indicar quais dados de arquivo(tabela) deve ser baixada da internet;
	- Criar a barra de progresso para cada ítem da lista;
	- Inserir os valores no banco de dados;
	- Iniciar a atualização das séries

	Erros que causam a saída deste método:
	- Erro ao indicar o nome do arquivo de estrutura;
	- Erro ao indicar um arquivo a ser baixado da internet;
	- Erro ao apagar dados da tabela;
	- Erro ao inserir dados na tabela;
	- Erro ao enviar pedido de atualização de séries.

\param LPCTSTR szSystem: Nome do sistema que serão criadas as tabelas
\return void.
*/
void CUpdateDlg::_Process(LPCTSTR szSystem)
{
	SetStatus(L"Processando as atualizações...");

	CString strSys = szSystem;
	_FIELDS_ meu_registro;
	CString nomeTabela , insertCommand, sApagaTabela;
	TB_ATUALIZACAO *dadoArquivo = NULL;
	CGetFile *fileAtualiza;

	int errors = 0;

	HCURSOR hCurs = GetCursor();
	SetCursor(LoadCursor(NULL, IDC_WAIT));

	CString sContrato = m_params->GetValue(L"contrato");
	CString sVarPma = L"transmite_" + strSys;
	CString sURL = m_params->GetValue(sVarPma);
	if(sURL.IsEmpty())
	{
		STLOG_WRITE("CUpdateDlg::_Process(): url 'transmite' esta vazia.");
	}

	//::MessageBox( NULL, (CString) this->getNomeTabela(), L"TESTE", MB_OK );
	
	for( int i = 0; i < m_list.GetItemCount() ; i++)
	{
		nomeTabela = m_list.GetItemText(i,0);
		nomeTabela.Trim();

		fileAtualiza = new CGetFile();
		fileAtualiza->SetStructFile(m_sStructPath);
		if(!fileAtualiza->SetNomeTabela(nomeTabela))
		{
			errors++;
			m_list.SetProgress(i, 0);
			STLOG_WRITE(L"CUpdateDlg::_Process(): Error setando lista %s", nomeTabela);
			STLOG_WRITE("----------------------------------------------------------");
			continue;
		}		

		if(! fileAtualiza->GetFileFromServer(sURL, sContrato))
		{
			errors++;
			m_list.SetProgress(i, 0);
			STLOG_WRITE(L"CUpdateDlg::_Process(): Error Download %s", nomeTabela);
			STLOG_WRITE("----------------------------------------------------------");
			continue;
		}

		CTableBase::BeginTransaction(CppSQLite3DB::getInstance());

		//TODO: If migração não apaga
		//sApagaTabela.Format( L"DELETE FROM %s", nomeTabela );
		sApagaTabela.Format( L"DROP TABLE %s", nomeTabela );

		try
		{
			CppSQLite3DB::getInstance()->execQuery( CStr(sApagaTabela) );
		}
		catch(CppSQLite3Exception e)
		{
			errors++;
			STLOG_WRITE("CUpdateDlg::_Process(): %s", e.errorMessage());
			STLOG_WRITE(sApagaTabela);
			STLOG_WRITE("----------------------------------------------------------");
			continue;
		}

		m_manageDB->CreateOneTable(nomeTabela);

		BOOL failed = FALSE;
		int count;

		for(count=0; !fileAtualiza->IsEOF(); count++)
		{
			if ((count%2000) == 0)
			{
				Sleep(2000);
			}

			fileAtualiza->GetRecord(&meu_registro);
			insertCommand = fileAtualiza->sqlInsertCommand;

			try
			{
				CppSQLite3DB::getInstance()->execQuery( CStr(insertCommand) );
				m_list.SetProgress(i, (int)(100*(count+1)/fileAtualiza->GetTotalRegs()));
			}
			catch(CppSQLite3Exception e)
			{
				STLOG_WRITE(L"CUpdateDlg::_Process(1): %S", e.errorMessage());
				STLOG_WRITE(insertCommand);
				STLOG_WRITE("----------------------------------------------------------");

				errors++;
				failed = TRUE;
				CTableBase::RollbackTransaction(CppSQLite3DB::getInstance());
				m_list.SetProgress(i, 0);

				break;
			}

			fileAtualiza->MoveNext();
		}

		if(count == 0 || errors == 0)
			m_list.SetProgress(i, 100);

		m_listaArquivos.Lookup(nomeTabela, dadoArquivo);

		//TODO: If migracao não atualiza
		if(!failed)
		{
			CStr sQuery;
			sQuery.Format("INSERT INTO atualizacao VALUES ('%S', '%S', '%S')",
						  dadoArquivo->nome.Trim(), 
						  dadoArquivo->data,
						  dadoArquivo->hora );

			try
			{
				CppSQLite3DB::getInstance()->execQuery( sQuery );
				CTableBase::CommitTransaction(CppSQLite3DB::getInstance());
			}
			catch(CppSQLite3Exception e)
			{
				errors++;
				m_list.SetProgress(i, 0);
				CTableBase::RollbackTransaction(CppSQLite3DB::getInstance());
				STLOG_WRITE(L"CUpdateDlg::_Process(2): %S", e.errorMessage());
				STLOG_WRITE(insertCommand);
				STLOG_WRITE("----------------------------------------------------------");
			}
		}
		delete(fileAtualiza);
	}

	if(errors > 0)
	{
		errors = 0;
		SetCursor(hCurs);
		CString sErr;
		sErr.LoadStringW(IDS_ERRO_ATUALIZACAO);
		MessageBox(sErr, L"Mensagem", MB_ICONINFORMATION|MB_OK);

		// Aplicacao nao continua...
		return;
	}

	m_wnd->Show(L"Processando atualizações...");

	SetCursor(hCurs);

	m_wnd->Hide();

	SetStatus(L"Atualização encerrada com sucesso");
	Sleep(1000);

	return;
}

//BOOL CUpdateDlg::_CreateDatabase()
//{
//	//Abre ou cria o banco de dados
//	try
//	{
//		CppSQLite3DB::getInstance()->open(CStr(m_sPath));
//	}
//	catch(CppSQLite3Exception e)
//	{
//		MessageBox(L"Erro criando arquivo do database", L"Mensagem", MB_ICONINFORMATION|MB_OK);
//		STLOG_WRITE("%s(%d): ERRO ABRINDO BANCO DE DADOS %s", __FUNCTION__, __LINE__, e.errorMessage());
//		EndDialog(IDOK);
//		return FALSE;
//	}
//
//	//Faz a criação das tabelas
//	if(!m_manageDB->CreateDatabase())
//	{
//		MessageBox(L"Erro criando database", L"Mensagem", MB_ICONINFORMATION|MB_OK);
//		STLOG_WRITE("%s(%d): Failed to create database", __FUNCTION__, __LINE__);
//		EndDialog(IDOK);
//		return FALSE;
//	}
//
//	return TRUE;
//
//	STLOG_WRITE("%s(%d): create database OK", __FUNCTION__, __LINE__);
//}

/**
\brief Método que é executado quando a janela recebe um aviso de estouro de timer.
\details	
	Funções executadas por este método:
	- Encerrar o timer;
	- Abrir/Criar o banco de dados;
	- Criar as tabelas;
	- Verificar a existência de dados da versão antiga para iniciar a migração;
	- Preencher a lista de atualizações;
	- Processar as atualizações.

	Erros que causam a saída deste método:
	- Erro na criação do banco de dados;
	- Erro na criação das tabelas;
	- Se o aplicativo for off-line ou não estiver conectado.

\param UINT_PTR nIDEvent: Flag que o id do timer.
\return void
*/
void CUpdateDlg::OnTimer(UINT_PTR nIDEvent)
{	
	ShowWindow(SW_SHOW);
	BOOL bRet = TRUE;

	CProcessSystem process(CppSQLite3DB::getInstance(), this, m_params->GetValue(L"codigo"));

	BOOL bIsOnline = FALSE;

	if(nIDEvent == ID_TIMER_SERVER_FILE)
	{
		KillTimer(ID_TIMER_SERVER_FILE);

		CString sContrato = m_params->GetValue(L"contrato");
		CString sURL = m_params->GetValue(L"util");

		if(sURL.IsEmpty())
		{
			STLOG_WRITE("CUpdateDlg::OnTimer(): url 'util' esta vazia.");
		}

		CString sResponse;


		if(CUtil::OnLine() &&
			!CUtil::IsValidTalao(&m_proxyInfo, sURL, sContrato, &sResponse))
		{
			if(sResponse.Find(L"Bloqueado")!=-1)
			{
				CSplash* splash;
				STLOG_WRITE(L"Block: Erro de acesso %s.", sResponse);
				splash = new CSplash();
				if(splash->Create( NULL, m_params->GetValue(L"blocked"), 240, 320 ))
					splash->Show();

				// CPU A 100 %...
				while(1) {}
			}
		}


		HCURSOR hCurs = GetCursor();
		SetCursor(LoadCursor(NULL, IDC_WAIT));
		
		UpdateWindow();
		SetCursor(hCurs);
		m_wnd->Hide();


		CString a = m_params->GetValue(L"atz_online");

		//Se for offline ou não estiver on line sai da atualização
		if(!CUtil::OnLine() && m_params->GetValue(L"atz_online").CompareNoCase(L"TRUE") == 0)
		{
			CString msg;
			msg.LoadString(IDS_ERRO_CONEXAO);

			MessageBox(msg, L"Mensagem", MB_ICONINFORMATION|MB_OK);
			STLOG_WRITE("CUpdateDlg::OnInitDialog() : IsOnline() falhou");
			bRet = FALSE;
			goto final;
		}
		else
		{

			//Atualiza a hora do equipamento com o servidor
			///CString sTime_URL = m_params->GetValue(L"hora");
			///CUtil::SetDataHoraServidor(&m_proxyInfo, sTime_URL, sContrato);
			CUtil::SetDataHoraServidor(&m_proxyInfo, sURL, sContrato);
		}

		if(bRet)
		{
#ifdef USA_TEM
			if (_PreencheLista(L"TEM"))
			{
				_Process(L"TEM");
			}
			else
			{
				CString msg;
				msg.LoadString(IDS_ERRO_RECEBIMENTO);

				MessageBox(msg, L"Mensagem", MB_ICONINFORMATION|MB_OK);
			}
#endif
#ifdef USA_GERINCID
			if (_PreencheLista(L"GERINCID"))
			{
				_Process(L"GERINCID");
			}
			else
			{
				MessageBox(L"Ocorreu um erro durante o recebimento da lista de atualizações, por favor tente executar a atualização novamente ou entre em contato com o suporte técnico.", L"Mensagem", MB_ICONINFORMATION|MB_OK);
			}
#endif
		}


//		SetTimer(ID_TIMER_PROCESS, 1000, NULL);
	}
	/*else if(nIDEvent == ID_TIMER_PROCESS)
	{
		KillTimer(ID_TIMER_PROCESS);
		_Process();
	}*/

final:
	process.ProcessSystem();
	m_wnd->Destroy();
	EndDialog(IDOK);

	CDialogEx::OnTimer(nIDEvent);
}

/**
\brief Muda a mensagem exibida no topo da janela de atualização
\param LPCTSTR szText: Texto a ser exibido
\return void
*/
void CUpdateDlg::SetStatus(LPCTSTR szText)
{
	m_status.SetWindowText(szText);
	m_status.Invalidate();
	m_status.UpdateWindow();
}


void CUpdateDlg::OnOK()
{
	// TODO: Add your specialized code here and/or call the base class

	//CDialogEx::OnOK();
}

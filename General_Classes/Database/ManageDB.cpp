#include "StdAfx.h"
#include "ManageDB.h"
#include "CppSQLite3.h"
#include "CStr.h"
#include "constants.h"
#include "utils.h"

/**
\brief Construtor da classe
\param CppSQLite3DB* pDB Ponteiro de conexão com banco de dados
\return void
*/
CManageDB::CManageDB()
{
	//m_pDB = CppSQLite3DB::getInstance();
}

/**
\brief Destrutor da classe
\param void
\return void
*/
CManageDB::~CManageDB(void)
{
}

/**
\brief Preenche lista checando a existência do mesmo
\param void
\return void
*/
void CManageDB::_SetTable(LPCTSTR szName, LPCTSTR szSQL)
{
	CString sValue;
	m_TableMap->Lookup(szName, sValue);

	if(sValue.Trim().IsEmpty())
	{
		m_TableMap->SetAt(szName, szSQL);
	}
}

/**
\brief Preenche lista checando a existência do mesmo
\param void
\return void
*/
void CManageDB::_SetIndex(LPCTSTR szName, LPCTSTR szSQL)
{
	CString sValue;
	m_IndexMap->Lookup(szName, sValue);

	if(sValue.Trim().IsEmpty())
	{
		m_IndexMap->SetAt(szName, szSQL);
	}
}

/**
\brief Preenche array de criação das tabelas e índices
\param void
\return void
*/
void CManageDB::_FillListTableIndex()
{
	CString sMod;

	//Tabelas do PMA
	//_SetTable(L"agente", L"CREATE TABLE IF NOT EXISTS agente (cd_entidade int, codigo varchar(12), nome varchar(45), post_grad varchar(30), senha varchar(6), definicao int, habilitado int, permissao varchar(100), data_ultima_troca DATE, data_limite_troca DATE)");
	_SetTable(L"agente", L"CREATE TABLE IF NOT EXISTS agente (codigo varchar(12), cd_entidade int, nome varchar(45), post_grad varchar(30), senha varchar(6), definicao int, habilitado int, permissao varchar(100), data_ultima_troca DATE, data_limite_troca DATE)");
	//_SetTable(L"agente", L"CREATE TABLE IF NOT EXISTS agente (codigo varchar(12), nome varchar(45), cd_entidade int, senha varchar(6), definicao int)");
	_SetTable(L"atualizacao", L"CREATE TABLE IF NOT EXISTS atualizacao (arquivo varchar(20), data Date, hora varchar(8))");	
	_SetTable(L"proxy", L"CREATE TABLE IF NOT EXISTS proxy (contador int,used int,http varchar(120),porta int,usuario varchar(20),senha varchar(20),discagem int )");
	_SetTable(L"contrato", L"CREATE TABLE IF NOT EXISTS contrato (contador int,cod_orgao_autuador varchar(20),contrato varchar(20) )");
	_SetTable(L"httpsender_info", L"CREATE TABLE IF NOT EXISTS [httpsender_info] ([obj_name] VARCHAR(15)  NULL PRIMARY KEY, [thread_time] INTEGER  NULL, [thread_handle] VARCHAR(10)  NULL, [ult_exec] VARCHAR(20)  NULL, [ult_envio] VARCHAR(20)  NULL, [thread_id] INTEGER  NULL)");
	_SetTable(L"httpsender_log", L"CREATE TABLE IF NOT EXISTS [httpsender_log] ([id] INTEGER  NOT NULL PRIMARY KEY AUTOINCREMENT, [obj_name] VARCHAR(15) NOT NULL, [file_name] VARCHAR(255) NOT NULL, [resp_server] VARCHAR(100)  NULL, status int NOT NULL, dt_log date, time_log time)");		
	_SetTable(L"httpsender_job", L"CREATE TABLE IF NOT EXISTS [httpsender_job] ([id] INTEGER  NOT NULL PRIMARY KEY AUTOINCREMENT, [obj_name] VARCHAR(15) NOT NULL, [file_web_ext] VARCHAR(20) NOT NULL, [file_path] VARCHAR(255) UNIQUE NOT NULL, [var_list] VARCHAR(255) NULL, [on_tx_ok_action] VARCHAR(50) NULL, [on_tx_ok_table] VARCHAR(30) NULL, [on_tx_ok_key_name] VARCHAR(30) NULL, [on_tx_ok_key_value] VARCHAR(30) NULL, [date_job] date, [time_job] time, [status] VARCHAR(1) NOT NULL)");
	_SetTable(L"log_agente", L"CREATE TABLE IF NOT EXISTS [log_agente] ([id] INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL, [id_talao] VARCHAR(40)  NULL, [cd_agente] VARCHAR(12)  NULL, [data] DATE  NULL, [hora] VARCHAR(8)  NULL, [operacao] VARCHAR(10) NULL, [transmissao] INTEGER  NULL)");
	_SetTable(L"cad_grupo_agente", L"CREATE TABLE IF NOT EXISTS cad_grupo_agente (codigo INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, descricao VARCHAR(50), permissao varchar(100));");
	_SetTable(L"rel_grupo_agente", L"CREATE TABLE IF NOT EXISTS rel_grupo_agente (codigo INTEGER NOT NULL PRIMARY KEY, cd_grupo_agente INTEGER NOT NULL, cd_agente VARCHAR(12) NOT NULL);");
	//_SetTable(L"mensagem", L"CREATE TABLE IF NOT EXISTS [mensagem] ([codigo] INTEGER  NOT NULL PRIMARY KEY, [descricao] VARCHAR(144)  NULL, [data_hora_cad] VARCHAR(20)  NULL);");
	_SetTable(L"mensagem", L"CREATE TABLE IF NOT EXISTS [mensagem] ([codigo] INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, [descricao] VARCHAR(144)  NULL, [data_hora_cad] VARCHAR(20)  NULL, [id_talao] VARCHAR[40] NULL, [cd_agente] VARCHAR[12] NULL, [data_hora_rec] VARCHAR[20] NOT NULL, [data_hora_vis] VARCHAR[20] NULL, [status] INTEGER NOT NULL);");
	
	//_SetTable(L"msg_agente", L"CREATE TABLE IF NOT EXISTS [msg_agente] ([codigo] INTEGER  NOT NULL PRIMARY KEY, [cod_msg] INTEGER  NULL, [cd_agente] VARCHAR(12)  NULL, [data_hora_rec] VARCHAR(20)  NULL);");
	//_SetTable(L"msg_talao", L"CREATE TABLE IF NOT EXISTS [msg_talao] ([codigo] INTEGER  NOT NULL PRIMARY KEY, [cod_msg] INTEGER  NULL, [cd_equipamento] VARCHAR(16)  NULL, [data_hora_rec] VARCHAR(20)  NULL);");
  
	//Índices do PMA
	_SetIndex(L"atualizacao",L"CREATE INDEX IF NOT EXISTS IdxArquivoAtualizacao ON Atualizacao (arquivo)");
	_SetIndex(L"contrato",L"CREATE INDEX IF NOT EXISTS IdxContadorContrato ON Contrato (contador)");
}

/**
\brief Executa queries de criação de todas as tabelas e índices oriundos dos arrays m_TableMap e m_IndexMap
\exception Erro criando tabelas e índices
\return BOOL
*/
BOOL CManageDB::CreateDatabase(__TABLEMAP *tables, __INDEXMAP *indexes)
{
	m_TableMap = tables;
	m_IndexMap = indexes;
	_FillListTableIndex();

	CStr sQuery;
	try
	{
		POSITION p;
		p = m_TableMap->GetStartPosition();
		while(p!=NULL)
		{
			CString k, v;
			m_TableMap->GetNextAssoc(p, k, v);

			sQuery = v;
			CppSQLite3DB::getInstance()->execDML(sQuery);
		}

		p = m_IndexMap->GetStartPosition();
		while(p!=NULL)
		{
			CString k, v;
			m_IndexMap->GetNextAssoc(p, k, v);

			sQuery = v;
			CppSQLite3DB::getInstance()->execDML(sQuery);
		}	

		return TRUE;
	}
	catch(CppSQLite3Exception e)
	{
		STLOG_WRITE("ERRO CRIANDO TABELAS E INDICES [%s]", e.errorMessage());
		STLOG_WRITE(sQuery);
		STLOG_WRITE("---------------------------------------------------");

		return FALSE;
	}

	return FALSE;
}

/**
\brief Executa query de criação de uma tabela e índice(s) em específico
\param const CString sName Nome da tabela/índice a ser criado
\exception Erro criando tabelas e índices
\return BOOL
*/
BOOL CManageDB::CreateOneTable(CString sName)
{
	CString sTemp;
	CStr sQuery;		

	sName.MakeLower();

	try
	{
		if(m_TableMap->Lookup(sName,sTemp))
		{
			STLOG_WRITE("CManageDB::CreateOneTable Executando criação de tabela: %S query: %S", sName, sTemp);
			sQuery = sTemp;
			CppSQLite3DB::getInstance()->execDML(sQuery);		
		}

		if(m_IndexMap->Lookup(sName,sTemp))
		{
			STLOG_WRITE("CManageDB::CreateOneTable Executando criação de indice: %S query: %S", sName, sTemp);
			sQuery = sTemp;
			CppSQLite3DB::getInstance()->execDML(sQuery);
		}
		
		int i=0;
		CString sIndex;
		while(1)
		{
			sIndex.Format(L"%s%d",sName,++i);
			if(m_IndexMap->Lookup(sIndex,sTemp))
			{
				sQuery = sTemp;
				CppSQLite3DB::getInstance()->execDML(sQuery);
			}
			else
				break;
		}

		return TRUE;
	}
	catch(CppSQLite3Exception e)
	{
		STLOG_WRITE("ERRO CRIANDO TABELA [%S] E INDICE [%S]", sName, e.errorMessage());
		STLOG_WRITE(sQuery);
		STLOG_WRITE("---------------------------------------------------");

		return FALSE;
	}

	return FALSE;
}
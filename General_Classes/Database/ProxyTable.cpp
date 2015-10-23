#include "StdAfx.h"
#include "ProxyTable.h"
#include "CppSQLite3.h"
#include "Utils.h"
#include "CStr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CProxyTable, CTableBase)


/**
\brief Construtor da classe
\param void
\return void
*/
CProxyTable::CProxyTable(void)
{
}


/**
\brief Destrutor da classe
\param void
\return void
*/
CProxyTable::~CProxyTable(void)
{
	Destroy();
}


/**
\brief Configura informações do proxy
\param CProxyInfo *pInfo Ponteiro de informações do proxy
\return void
*/
void CProxyTable::SetValues(CProxyInfo *pInfo)
{
	SetValue(_T("used"),	(long)pInfo->bProxy);
	SetValue(_T("http"),	pInfo->sServer);
	SetValue(_T("porta"),	(long)pInfo->nPort);
	SetValue(_T("usuario"),	pInfo->sUser);
	SetValue(_T("senha"),	pInfo->sPass);
	SetValue(_T("discagem"),(long)pInfo->bDiscagem);
}


/**
\brief Recupera configurações da tabela PROXY e alimenta ponteiro de informações
\param CppSQLite3DB *pDB Handle de conexão à base de dados
\param CProxyInfo *pInfo Ponteiro de informações do proxy
\return BOOL
*/
BOOL CProxyTable::Load(CppSQLite3DB *pDB, CProxyInfo *pInfo)
{
	ASSERT(pDB);

	CStr s;
	s.Format("SELECT used, http, porta, usuario, senha, discagem FROM %S", m_sTableName); 
	CStr sErr;
	try
	{
		CppSQLite3Query q = pDB->execQuery(s);
		if(!q.eof())
		{
			pInfo->bProxy    = q.getIntField(0);
			pInfo->sServer   = q.getStringField(1);
			pInfo->nPort     = q.getIntField(2);
			pInfo->sUser     = q.getStringField(3);
			pInfo->sPass     = q.getStringField(4);
			pInfo->bDiscagem = q.getIntField(5);
		}
		q.finalize();

		return TRUE;
	}
	catch(CppSQLite3Exception e)
	{
		sErr = e.errorMessage();
		STLOG_WRITE("ERRO RECUPERANDO DADOS PROXY %s", e.errorMessage());
		STLOG_WRITE(s);
		STLOG_WRITE("--------------------------------------------------------");
	}

	return FALSE;
}


/**
\brief Monta query de atualização e executa-a
\details Faz a chamada ao método CTableBase::BuildUpdateQuery() para construir a query
\param CppSQLite3DB *pDB Handle de conexão com a base de dados
\exception Erro	de execução da query
\return BOOL
*/
BOOL CProxyTable::Update(CppSQLite3DB *pDB, CProxyInfo *pInfo)
{
	SetValues(pInfo);
	ASSERT(pDB != NULL);

	CString s = BuildUpdateQuery(TRUE);
	CStr s1(s);
	long count = pDB->execDML(s1);
	return count == 1;
}


/**
\brief Retorna contagem de registros da tabela proxy
\param CppSQLite3DB *pDB Handle de conexão com a base de dados
\exception Erro de execução da query
\return int 
*/
int CProxyTable::Count(CppSQLite3DB *pDB)
{
	ASSERT(pDB);

	int count = 0;

	CStr s = "";
	s.Format("SELECT count(*) FROM %S", m_sTableName); 
	CStr sErr;
	try
	{
		CppSQLite3Query q = pDB->execQuery(s);
		if(!q.eof())
		{
			count = q.getIntField(0);
		}
		q.finalize();

		return count;
	}
	catch(CppSQLite3Exception e)
	{
		sErr = e.errorMessage();
		STLOG_WRITE("ERRO RECUPERANDO DADOS PROXY %s", e.errorMessage());
		STLOG_WRITE(s);
		STLOG_WRITE("--------------------------------------------------------");
	}

	return 0;
}
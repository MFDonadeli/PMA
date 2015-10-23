#include "StdAfx.h"
#include "Table.h"
#include "CppSQLite3.h"
#include "Utils.h"
#include "CStr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CTable, CTable)


/**
\brief Construtor da classe
\param void
\return void
*/
CTable::CTable(void)
{
}

/**
\brief Destrutor da classe
\param void
\return void
*/
CTable::~CTable(void)
{
	Destroy();
}

/**
\brief Configura nome da tabela e seus campos inclusive o tipo de dados que armazenam
\details Utiliza-se do método CTableBase::InsertField() herdado da classe pai para setar os campos
\param void
\return void
*/
void CTable::Init(LPCTSTR szTable)
{
	BOOL bKey;
	CString sKeyType;
	CStr sQuery;

	m_sTableName = szTable;
	sQuery.Format("PRAGMA table_info(%S)", m_sTableName);

	CppSQLite3DB* db = CppSQLite3DB::getInstance();
	CppSQLite3Query q;

	try
	{
		q = db->execQuery(sQuery);

		//STLOG_WRITE("%s(%d): Entrando para init de %S", __FUNCTION__, __LINE__, m_sTableName);

		while(!q.eof())
		{
			bKey = q.getIntField(5);
			sKeyType = q.getStringField(2);

			//STLOG_WRITE("%s(%d): Campo: %S - Tipo: %S - Key: %d", __FUNCTION__, __LINE__, CString(q.getStringField(1)), sKeyType, bKey);

			if(sKeyType.Left(3).CompareNoCase(L"DAT")==0)
				InsertField(CString(q.getStringField(1)), CField::FIELD_TYPE_DATE, bKey);
			else if(sKeyType.Left(3).CompareNoCase(L"INT")==0)
				InsertField(CString(q.getStringField(1)), CField::FIELD_TYPE_NUMBER, bKey);
			else if(sKeyType.Left(3).CompareNoCase(L"FLO")==0)
				InsertField(CString(q.getStringField(1)), CField::FIELD_TYPE_CURRENCY, bKey);
			else if(sKeyType.Left(3).CompareNoCase(L"VAR")==0)
				InsertField(CString(q.getStringField(1)), CField::FIELD_TYPE_STRING, bKey);
			else if(sKeyType.Left(3).CompareNoCase(L"BOO")==0)
				InsertField(CString(q.getStringField(1)), CField::FIELD_TYPE_BOOLEAN, bKey);

			q.nextRow();
		}
	}
	catch(CppSQLite3Exception e)
	{
		STLOG_WRITE("%s: ERRO executando query", __FUNCTION__,
					e.errorMessage());

		STLOG_WRITE(sQuery);
	}
}


/**
\brief Monta query de inserção e executa-a
\details Faz a chamada ao método CTableBase::BuildInsertQuery() para construir a query
\param CppSQLite3DB *pDB Handle de conexão com a base de dados
\exception Erro	de execução da query
\return BOOL
*/
BOOL CTable::Insert(CppSQLite3DB *pDB)
{
	ASSERT(pDB);

	CString sQ = BuildInsertQuery();
	CStr sErr;
	try
	{
		CStr sQ1 = sQ;
		pDB->execQuery(sQ1, true);
		return TRUE;
	}
	catch(CppSQLite3Exception e)
	{
		sErr = e.errorMessage();
		STLOG_WRITE("%s(%d): ERRO INSERINDO NA TABELA %S: %s", __FUNCTION__, __LINE__, m_sTableName, e.errorMessage());
		STLOG_WRITE(sQ);
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
BOOL CTable::Update(CppSQLite3DB *pDB)
{
	ASSERT(pDB);

	CString sQ = BuildUpdateQuery(TRUE);
	if(sQ.IsEmpty())
		return FALSE;

	CStr sErr;
	try
	{
		CStr sQ1 = sQ;
		pDB->execQuery(sQ1, true);
		STLOG_WRITE("%s(%d): Atualização em %S executada com sucesso", __FUNCTION__, __LINE__, m_sTableName);
		return TRUE;
	}
	catch(CppSQLite3Exception e)
	{
		sErr = e.errorMessage();
		STLOG_WRITE("%s(%d): ERRO ATUALIZANDO TABELA %S: %s", __FUNCTION__, __LINE__, m_sTableName, e.errorMessage());
		STLOG_WRITE(sQ);
		STLOG_WRITE("--------------------------------------------------------");
	}

	return FALSE;
}

/**
\brief Monta query de remoção e executa-a
\details Faz a chamada ao método CTableBase::BuildDeleteQuery() para construir a query
\param CppSQLite3DB *pDB Handle de conexão com a base de dados
\exception Erro	de execução da query
\return BOOL
*/
BOOL CTable::Delete(CppSQLite3DB *pDB)
{
	ASSERT(pDB);

	CString sQ = BuildDeleteQuery(FALSE);
	CStr sErr;
	try
	{
		CStr sQ1 = sQ;
		pDB->execQuery(sQ1);
		return TRUE;
	}
	catch(CppSQLite3Exception e)
	{
		sErr = e.errorMessage();
		STLOG_WRITE("%s(%d): ERRO DELETANDO NA TABELA %S: %s", __FUNCTION__, __LINE__, m_sTableName, e.errorMessage());
		STLOG_WRITE(sQ);
		STLOG_WRITE("--------------------------------------------------------");
	}

	return FALSE;
}

/**
\brief Monta query de inserção ou update e executa-a
\details Faz a chamada ao método CTableBase::BuildInsertOrReplaceQuery() para construir a query
\param CppSQLite3DB *pDB Handle de conexão com a base de dados
\exception Erro	de execução da query
\return BOOL
*/
BOOL CTable::InsertOrReplace(CppSQLite3DB *pDB)
{
	ASSERT(pDB);

	CString sQ = BuildInsertOrReplaceQuery();
	CStr sErr;
	try
	{
		CStr sQ1 = sQ;
		pDB->execQuery(sQ1);
		return TRUE;
	}
	catch(CppSQLite3Exception e)
	{
		sErr = e.errorMessage();
		STLOG_WRITE("%s(%d): ERRO INSERINDO/ATUALIZANDO NA TABELA %S: %s", __FUNCTION__, __LINE__, m_sTableName, e.errorMessage());
		STLOG_WRITE(sQ);
		STLOG_WRITE("--------------------------------------------------------");
	}

	return FALSE;
}
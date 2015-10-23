#include "StdAfx.h"
#include "Rap_Table.h"
#include "CStr.h"

CRap_Table::CRap_Table(void)
{
}

CRap_Table::~CRap_Table(void)
{
}

void CRap_Table::Init()
{
	m_sTableName = _T("RAP");

	// Key
	InsertField(_T("id_web"),	CField::FIELD_TYPE_NUMBER, TRUE);

	// Other Fields
//	InsertField(_T("contador"),			CField::FIELD_TYPE_NUMBER);
	//InsertField(_T("id_web"),			CField::FIELD_TYPE_NUMBER);
	InsertField(_T("id_pocket"),		CField::FIELD_TYPE_NUMBER);
	InsertField(_T("local"),			CField::FIELD_TYPE_STRING);
	InsertField(_T("data"),				CField::FIELD_TYPE_DATE);
	InsertField(_T("hora"),				CField::FIELD_TYPE_STRING);
	InsertField(_T("placa"),			CField::FIELD_TYPE_STRING);
	InsertField(_T("cor"),				CField::FIELD_TYPE_STRING);
	InsertField(_T("atendido"),			CField::FIELD_TYPE_NUMBER); // int
	InsertField(_T("delta_t"),			CField::FIELD_TYPE_NUMBER); // int
}

BOOL CRap_Table::Insert(CppSQLite3DB *pDB)
{
	ASSERT(pDB);

	CString sQ = BuildInsertQuery();
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
		STLOG_WRITE("ERRO INSERINDO AIIP %s", e.errorMessage());
		STLOG_WRITE(sQ);
		STLOG_WRITE("--------------------------------------------------------");
	}

	return FALSE;
}

BOOL CRap_Table::Update(CppSQLite3DB *pDB)
{
	ASSERT(pDB);
	CString sQ = BuildUpdateQuery();
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
		STLOG_WRITE("ERRO ATUALIZANDO AIIP %s", e.errorMessage());
		STLOG_WRITE(sQ);
		STLOG_WRITE("--------------------------------------------------------");
	}

	return FALSE;
}

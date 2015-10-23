#include "StdAfx.h"
#include "tablebase.h"
#include "CppSQLite3.h"
#include "Utils.h"
#include "CStr.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

static TCHAR *_BOOLEAN_TRUE_VALUES[] = { _T("T"), _T("Y"), _T("1"), _T("S") };

IMPLEMENT_DYNCREATE(CTableBase, CObject)

/**
 * !!!! IMPORTANTE !!!!
 * 
 * Usaremos o '[' no nome dos campos, por ex: [FIELD] pois temos muitos
 * campos com nome reservados como por exemplo Limit.
 */

CField::CField()
{
	m_sBooleanChar[BOOLEAN_TRUE]  = _T("T");
	m_sBooleanChar[BOOLEAN_FALSE] = _T("F");

	m_type = FIELD_TYPE_STRING;
	m_bKey = FALSE;
}

CField::~CField()
{
}

CField::CField(LPCTSTR _szLabel, LPCTSTR _szValue, _fieldType _type, BOOL _isKey)
{
	m_sBooleanChar[BOOLEAN_TRUE]  = _T("T");
	m_sBooleanChar[BOOLEAN_FALSE] = _T("F");

	m_type	 = _type;
	m_sValue = _szValue;
	m_sLabel = _szLabel;
	m_bKey   = _isKey;
//	m_dValue = 0;
}

CField::CField(LPCTSTR _szLabel, _fieldType _type, BOOL _isKey)
{
	m_sBooleanChar[BOOLEAN_TRUE]  = _T("T");
	m_sBooleanChar[BOOLEAN_FALSE] = _T("F");

	m_type	 = _type;
	m_sLabel = _szLabel;
	m_bKey   = _isKey;
//	m_dValue = 0;

	m_sValue.Empty();
}

void CField::SetBooleanCharTrue(LPCTSTR szChar)
{
	m_sBooleanChar[BOOLEAN_TRUE]  = szChar;
}

void CField::SetBooleanCharFalse(LPCTSTR szChar)
{
	m_sBooleanChar[BOOLEAN_FALSE] = szChar;
}

BOOL CField::IsKey()
{
	return m_bKey;
}

BOOL CField::IsNull()
{
	return m_sValue.IsEmpty();
}

double CField::GetDoubleValue()
{
	TCHAR *end;
	return _tcstod(m_sValue, &end);
//	return m_dValue;
}

CString CField::GetStringValue()
{
	/*if(m_type == FIELD_TYPE_CURRENCY)
	{
		CString s;
		s.Format(_T("%.7f"), m_sValue);
		return s;
	}*/

	if(m_type == FIELD_TYPE_CURRENCY && m_sValue.Compare(_T("0")) == 0)
		return _T("0"); //Se igual a zero mantem zero, não nulo

	return m_sValue;
}

long CField::GetLongValue()
{
	TCHAR *end;
	return _tcstoul(m_sValue, &end, 10);
}

BOOL CField::GetDateValue(COleDateTime *pOdt)
{
	ASSERT(pOdt != NULL);

	if(pOdt->ParseDateTime(m_sValue))
		return TRUE;

	return FALSE;
}

/**
 * Funcao para adicionar o aspas no caso de data e strings
 */
CString	CField::GetQueryFormatValue()
{
	CString s;

	if(m_type == FIELD_TYPE_STRING ||
	   m_type == FIELD_TYPE_DATE   ||
	   m_type == FIELD_TYPE_BOOLCHAR )
	{
		s.Format(_T("'%s'"), m_sValue);
		return s;
	}

	m_sValue.TrimLeft();
	m_sValue.TrimRight();

	if(m_sValue.IsEmpty() && 
		( m_type == FIELD_TYPE_NUMBER || 
		  m_type == FIELD_TYPE_CURRENCY)
	   )
	{
		return _T("");
	}

	return GetStringValue();
}

void CField::SetValue(double d)
{
	//m_dValue = d;
	m_sValue.Format(_T("%.5f"), d);
}

void CField::SetValue(long   l)
{
	m_sValue.Format(_T("%ld"), l);
}

void CField::SetValue(COleDateTime o)
{
	m_sValue = o.Format(VAR_DATEVALUEONLY);//(_T("%d/%m/%Y"));
	//VERIFICAR !!!!
//CUtil::Trace(m_sValue);
	ASSERT(FALSE);
}

void CField::SetValue(LPCTSTR s)
{
	m_sValue = s;
}

void CField::SetValue(BOOL b)
{
	if(b) m_sValue = m_sBooleanChar[BOOLEAN_TRUE];
	else  m_sValue = m_sBooleanChar[BOOLEAN_FALSE];
}
#ifndef ARRAYSIZE
	#define ARRAYSIZE(X)  (sizeof(X)/sizeof(X[0]))
#endif
BOOL CField::GetBooleanValue()
{
	for(int i = 0; i < ARRAYSIZE(_BOOLEAN_TRUE_VALUES); i++)
		if(m_sValue.CompareNoCase(_BOOLEAN_TRUE_VALUES[i]) == 0)
			return TRUE;

	//if(m_sValue.CompareNoCase(m_sBooleanChar[BOOLEAN_TRUE]) == 0)
	//	return TRUE;

	return FALSE;
}

void CField::Copy(CField *pSrc)
{
	m_sBooleanChar[0] = pSrc->m_sBooleanChar[0];
	m_sBooleanChar[1] = pSrc->m_sBooleanChar[1];
	m_bKey		= pSrc->m_bKey;
	m_type		= pSrc->m_type;
	m_sValue	= pSrc->m_sValue;
	m_sLabel	= pSrc->m_sLabel;
//	m_dValue	= pSrc->m_dValue;
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
CTableBase::CTableBase()
{
	m_sFlag = "";
}

CTableBase::~CTableBase()
{
	Destroy();
}

void CTableBase::Destroy()
{
	POSITION pos = m_mapFields.GetStartPosition();
	CString sKey;
	CField *pClass;

	while(pos != NULL)
	{
		m_mapFields.GetNextAssoc( pos, sKey, pClass );
		delete pClass;
	}

	m_mapFields.RemoveAll();
}

////////////////////////////////////////////////////////////
void CTableBase::InsertField(LPCTSTR _szLabel, 
							 CField::_fieldType _type, 
							 BOOL _isKey, 
							 BOOL _addImportList)
{
	// Adicionar na lista de ordem de importacao
	if(_addImportList)
		m_fieldOrdered.AddTail(_szLabel);

	// Agora adicionar no map ...
	CField *pField = new CField(_szLabel, _type, _isKey);
	m_mapFields.SetAt(_szLabel, pField);
}

/**
 * Metodo para recuperar a lista de nome de campos
 */ 
CString CTableBase::BuildFieldList()
{
	CString s;

	POSITION pos = m_mapFields.GetStartPosition();
	CString sKey;
	CField *pClass;
	int counter = 0;
	while(pos != NULL)
	{
		m_mapFields.GetNextAssoc( pos, sKey, pClass );

		if(counter++ > 0)
			s += _T(",\r\n");

		// Se tiver espaco, vamos usar o [label label]
		//if(sKey.Find(' ') > 0)
		s += _T("[") + sKey + _T("]");
		//else
		//	s += sKey;
	}

	return s;
}

/**
 * Metodo para construir a query de selecao
 */ 
CString CTableBase::BuildSelectQuery(BOOL bWhereWithKeyOnly, BOOL bIgnoreNULL)
{
	CString s;

	s.Format(_T("SELECT %s\r\nFROM %s\r\n%s\r\n%s\r\n"), 
			 BuildFieldList(), 
			 m_sTableName, 
			 BuildWhereList(bWhereWithKeyOnly, bIgnoreNULL),
			 BuildOrderByList());

	return s;
}

/**
 * Metodo para adicionar um campo no order by clause
 */ 
void CTableBase::AddOrderByField(LPCTSTR szField)
{
	m_orderByFields.AddTail(szField);
}

/**
 * Metodo para remover todos os campos da order by clause
 */ 
void CTableBase::ResetOrderByFields()
{
	m_orderByFields.RemoveAll();
}

/**
 * Metodo para montar a lista de campos usados no clause
 * order by 
 */ 
CString CTableBase::BuildOrderByList()
{
	CString s;
	if(m_orderByFields.GetCount() > 0)
	{
		s = _T("ORDER BY ");

		POSITION p = m_orderByFields.GetHeadPosition();
		int count = 0;
		while(p != NULL)
		{
			if(count++ > 0)
				s += _T(",");

			s += m_orderByFields.GetNext(p);
		}
	}

	return s;
}

/**
 * Metodo para montar a lista de label=valor para uso no
 * where clause da query
 */ 
CString CTableBase::BuildWhereList(BOOL bOnlyKeys, BOOL bIgnoreNULL)
{
	CString s(_T("WHERE "));
	POSITION pos = m_mapFields.GetStartPosition();
	CString sKey;
	CField *pClass;
	int counter = 0;
	BOOL usefull = FALSE;

	while(pos != NULL)
	{
		m_mapFields.GetNextAssoc( pos, sKey, pClass );

		// Se o campo estiver na lista de ignore... nao usar ...
		if(m_ignoreFields.Find(sKey) != NULL)
			continue;

		// Se nao for chave ...
		if(bOnlyKeys && !pClass->IsKey())
			continue;

		if(!pClass->IsNull())
		{
			usefull = TRUE;

			CString s1 = pClass->GetQueryFormatValue();

			if(s1.CompareNoCase(_T("NULL")) == 0 || 
			   s1.CompareNoCase(_T("\"NULL\"")) == 0 )
			{
				if(!bIgnoreNULL)
				{
					if(counter++ > 0)
						s += _T("\r\n AND ");

					s += _T("[") + sKey + _T("] IS NULL");
				}
			}
			else
			{
				if(counter++ > 0)
					s += _T("\r\n AND ");

				//if(sKey.Find(' ') > 0)
				s += _T("[") + sKey + _T("]=");
				//else
				//	s += sKey + _T("=");
				
				s += s1;
			}
		}
		else
		{
			if(!bIgnoreNULL)
			{
				if(counter++ > 0)
					s += _T("\r\n AND ");

				s += _T("[") + sKey + _T("] IS NULL");
			}
		}
	}

	if(!usefull)
		return _T("");

	return s;
}

/**
 * Metodo para setar um valor tipo String no label _szLabel
 */ 
BOOL CTableBase::SetValue(LPCTSTR _szLabel, LPCTSTR _szValue)
{
	CField *pField;
	if(m_mapFields.Lookup(_szLabel, pField))
	{
		pField->SetValue(_szValue);
		return TRUE;
	}

	return FALSE;
}

/**
 * Metodo para setar um valor tipo double no label _szLabel
 */ 
BOOL CTableBase::SetValue(LPCTSTR _szLabel, double _d)
{
	CField *pField;
	if(m_mapFields.Lookup(_szLabel, pField))
	{
		pField->SetValue(_d);
		return TRUE;
	}

	return FALSE;
}

/**
 * Metodo para setar um valor tipo long no label _szLabel
 */ 
BOOL CTableBase::SetValue(LPCTSTR _szLabel, long _l)
{
	CField *pField;
	if(m_mapFields.Lookup(_szLabel, pField))
	{
		pField->SetValue(_l);
		return TRUE;
	}

	return FALSE;
}

/**
 * Metodo para setar um valor tipo data no label _szLabel
 */ 
BOOL CTableBase::SetValue(LPCTSTR _szLabel, COleDateTime o)
{
	CField *pField;
	if(m_mapFields.Lookup(_szLabel, pField))
	{
		pField->SetValue(o);
		return TRUE;
	}

	return FALSE;
}

/**
 * Metodo para zerar o valor dos campos da tabela
 */ 
void CTableBase::ResetFields(CStringList &sIgnoreList)
{
	CField *pField;
	POSITION p;
	CString sLabel;
	CString sTmp;

	p = m_mapFields.GetStartPosition();

	while(p)
	{
		m_mapFields.GetNextAssoc(p, sLabel, pField);
		if(sIgnoreList.Find(sLabel) == NULL)
		{
			pField->SetValue(L"");
		}
	}

}

void CTableBase::BuildInsertQuery1(CString &s)
{	
	
	s.Format(_T("INSERT INTO %s\r\n(%s)\r\nVALUES(%s)\r\n"), 
			 m_sTableName, 
			 BuildFieldList(), 
			 BuildValueList());
}

/**
 * Metodo para a montagem da query de insercao de registro
 */
CString CTableBase::BuildInsertQuery()
{	

	CString s;

	//O método format trabalha somente com 1024 bytes, por isso optamos pela concatenação manual
	/*s.Format(_T("INSERT INTO %s\r\n(%s)\r\nVALUES(%s)\r\n"), 
			 m_sTableName, 
			 BuildFieldList(), 
			 BuildValueList());*/
	
	s = L"INSERT INTO "+m_sTableName+L"("+ BuildFieldList() + L") VALUES ("+ BuildValueList()+L")";
	
	//STLOG_WRITE(L"CTableBase::BuildInsertQuery() = %s",s);
	return s;
}

/**
 * Metodo para a montagem da query de insercao/update de registro
 */
CString CTableBase::BuildInsertOrReplaceQuery()
{	

	CString s;

	//O método format trabalha somente com 1024 bytes, por isso optamos pela concatenação manual
	/*s.Format(_T("INSERT INTO %s\r\n(%s)\r\nVALUES(%s)\r\n"), 
			 m_sTableName, 
			 BuildFieldList(), 
			 BuildValueList());*/
	
	s = L"INSERT OR REPLACE INTO "+m_sTableName+L"("+ BuildFieldList() + L") VALUES ("+ BuildValueList()+L")";
	
	//STLOG_WRITE(L"CTableBase::BuildInsertQuery() = %s",s);
	return s;
}

/**
 * Metodo para montar a lista de valores separados por 
 * virgula, tipico nos valores de uma query de insert 
 */
CString CTableBase::BuildValueList()
{
	CString s;
	POSITION pos = m_mapFields.GetStartPosition();
	CString sKey;
	CField *pClass;
	int counter = 0;

	while(pos != NULL)
	{
		m_mapFields.GetNextAssoc( pos, sKey, pClass );	

		if(counter++ > 0)
			s += _T(",\r\n");

		if(!pClass->IsNull())
			s += pClass->GetQueryFormatValue();
		else
			s += _T("NULL");		
	}

	return s;
}
/**
 * Metodo para a montagem da query de Update
 * bAllFields - Adiciona todos os lbls=values mesmo nulos !!!!
 */
CString CTableBase::BuildUpdateQuery(BOOL bAllFields, BOOL bWhereWithKeyOnly, BOOL bNoKeyInSet)
{
	CString s;

	s += L"UPDATE "+m_sTableName+L"\r\nSET"+BuildFieldValueList(bAllFields, bNoKeyInSet)+L"\r\n"+
	     BuildWhereList(bWhereWithKeyOnly)+L"\r\n";			
	
	/*sFields.Format(_T("UPDATE %s\r\nSET %s\r\n%s\r\n"), 
			 m_sTableName, 
			 BuildFieldValueList(bAllFields, bNoKeyInSet), 
			 BuildWhereList(bWhereWithKeyOnly));*/	

	return s;
}

/**
 * Metodo para montar a lista de labels=valores para uso no
 * no update, note bAllFields, permite levar os null's na query
 */
CString CTableBase::BuildFieldValueList(BOOL bAllFields, BOOL bNoKeyInSet)
{
	CString s;
	POSITION pos = m_mapFields.GetStartPosition();
	CString sKey;
	CField *pClass;
	int counter = 0;
	BOOL usefull = FALSE;

	///Ll## STLOG_WRITE("Inicio CTableBase::BuildFieldValueList()");

	while(pos != NULL)
	{
		m_mapFields.GetNextAssoc( pos, sKey, pClass );

		// Naoadicionar as chaves no set xxxx='xxxx'
		if(pClass->IsKey() && bNoKeyInSet)
			continue;

		if(!bAllFields) // Skipar os nulos !
		{
			if(!pClass->IsNull())
			{
				if(counter++ > 0)
					s += _T(",\r\n");

				usefull = TRUE;

				//if(sKey.Find(' ') > 0)
				s += _T("[") + sKey + _T("]=");
				//else
				//	s += sKey + _T("=");
				
				s += pClass->GetQueryFormatValue();
			}
		}
		else // fazer assign mesmo dos nulos / nao preenchidos !!!
		{
			usefull = TRUE;

			if(counter++ > 0)
				s += _T(",\r\n");

			//if(sKey.Find(' ') > 0)
			s += _T("[") + sKey + _T("]=");
			//else
			//	s += sKey + _T("=");

			if(!pClass->IsNull())
				s += pClass->GetQueryFormatValue();
			else
				s += _T("NULL");
		}
	}

	//STLOG_WRITE(L"CTableBase::BuildFieldValueList: CAMPOS/VALORES: %s", s);

	return s;
}

/**
 * Metodo para montar a query de delecao de um registro
 */ 
CString CTableBase::BuildDeleteQuery(BOOL bWhereWithKeyOnly)
{
	CString s;
	s.Format(_T("DELETE FROM %s\r\n%s\r\n"), 
			 m_sTableName, 
			 BuildWhereList(bWhereWithKeyOnly, TRUE));

	return s;
}

BOOL CTableBase::ExecuteQuery(LPCTSTR szQuery)
{
	return TRUE;
}

long CTableBase::GetFieldCount()
{
	return (long) m_mapFields.GetCount();
}

/**
 * As classes derivadas podem implementar acoes antes da
 * geracao da query de insercao no processo de import
 */ 
void CTableBase::ProcessBeforeImport()
{
}

double CTableBase::GetDoubleValue(LPCTSTR _szLabel)
{
	CField *pField;
	if(m_mapFields.Lookup(_szLabel, pField))
		return pField->GetDoubleValue();

	ASSERT(FALSE); // NAo encontrou o campo !
	return 0.0;
}
BOOL CTableBase::GetBooleanValue(LPCTSTR _szLabel)
{
	CField *pField;
	if(m_mapFields.Lookup(_szLabel, pField))
		return pField->GetBooleanValue();

	ASSERT(FALSE); // NAo encontrou o campo !
	return FALSE;
}

CString	CTableBase::GetStringValue(LPCTSTR _szLabel)
{
	CField *pField;
	if(m_mapFields.Lookup(_szLabel, pField))
	{
		//ASSERT(pField->GetType() == CField::FIELD_TYPE_STRING);
		return pField->GetStringValue();
	}

	TRACE(L"CTableBase::GetStringValue Não encontrei campo: %s\r\n", _szLabel);
	ASSERT(FALSE); // Nao encontrou o campo !
	return _T("");
}

long CTableBase::GetLongValue(LPCTSTR _szLabel)
{
	CField *pField;
	if(m_mapFields.Lookup(_szLabel, pField))
	{
		ASSERT(pField->GetType() == CField::FIELD_TYPE_NUMBER);
		return pField->GetLongValue();
	}

	ASSERT(FALSE); // NAo encontrou o campo !
	return 0L;
}

BOOL CTableBase::GetDateValue(LPCTSTR _szLabel, COleDateTime *pOdt)
{
	CField *pField;
	if(m_mapFields.Lookup(_szLabel, pField))
		return pField->GetDateValue(pOdt);

	ASSERT(FALSE); // Nao encontrou o campo !
	return FALSE;
}

/**
 * Remove todos os dados da tabela sem verificar os indexes
 */
CString CTableBase::BuildTruncateQuery()
{
	CString s;
	s.Format(_T("DELETE FROM %s\r\n"), m_sTableName);

	return s;
}

/**
 * Metodo para preencher a instancia corrente com os valores
 * recebidos do pQuery. NOTE que se o pQuery possuir mais de
 * uma linha, as outras serao ignoradas - serve somente para
 * busca com chave completa, recuperacao do detalhe
 */
BOOL CTableBase::FillMe(CppSQLite3Query *pQuery)
{
	ASSERT(pQuery != NULL);
	int fields = pQuery->numFields();

    if(pQuery->eof())
		return FALSE;

	// Setar os valores ...
	for(int fld = 0; fld < fields; fld++)
	{
		CString s1(pQuery->fieldName(fld));

		s1.Remove('[');
		s1.Remove(']');

		if(pQuery->fieldDataType(fld) == SQLITE_FLOAT)
		{
			double d = pQuery->getFloatField(fld);
			VERIFY(SetValue(s1, d));
		}
		else
		{
			CString s2(pQuery->fieldValue(fld));
			VERIFY(SetValue(s1, s2));
		}
	}

	return TRUE;
}

/**
 * Metodo para preencher uma lista de dericados de tablebase
 * 
 */
long CTableBase::Fill(CppSQLite3Query *pQuery, TABLELIST *pList, CRuntimeClass *pClass)
{
	ASSERT(pQuery != NULL);
	ASSERT(pList  != NULL);
	int fields = pQuery->numFields();
	long counter = 0;

    while(!pQuery->eof())
    {
		CTableBase *pTable = (CTableBase *) pClass->CreateObject();
		ASSERT(pTable != NULL);

		for(int fld = 0; fld < fields; fld++)
		{
			CString s1(pQuery->fieldName(fld));

			s1.Remove('[');
			s1.Remove(']');

			if(pQuery->fieldDataType(fld) == SQLITE_FLOAT)
			{
				double d = pQuery->getFloatField(fld);
				VERIFY(pTable->SetValue(s1, d));
			}
			else
			{
				CString s2(pQuery->fieldValue(fld));
				VERIFY(pTable->SetValue(s1, s2));
			}
		}

		pList->AddTail(pTable);
        pQuery->nextRow();

		counter++;
    }

	return counter;
}

/**
 * Metodo para limpar todos os valores dos campos da tabela
 */
void CTableBase::Reset(BOOL bExcludeKeys)
{
	POSITION pos = m_mapFields.GetStartPosition();
	CString sKey;
	CField *pField;

	while(pos != NULL)
	{
		m_mapFields.GetNextAssoc( pos, sKey, pField );
		
		if(pField->IsKey() && bExcludeKeys)
			continue;

		pField->SetValue(_T(""));
	}
}

/**
 * Metodo para destruir os itens de uma lista
 */
void CTableBase::DestroyList(TABLELIST *pList)
{
	POSITION p = pList->GetHeadPosition();
	while(p != NULL)
	{
		CTableBase *pTable = pList->GetNext(p);
		delete pTable;
	}

	pList->RemoveAll();
}

/**
 * Metodo para executar um busca pela chave primaria
 */
BOOL CTableBase::FindByPK(BOOL bWhereWithKeyOnly)
{
/*	CString s = BuildSelectQuery();
	if(s.IsEmpty())
		return FALSE;

//CUtil::Trace(s);

//	char *p = CUtil::ConvertUnicodeToAnsi(s);
	CStr s1(s);
	CppSQLite3Query q = m_pDb->execQuery(s1);
//	delete p;

	BOOL b = FillMe(&q);
	q.finalize();
	return b;
*/
	ASSERT(0);
	return FALSE;
}

/**
 * Metodo para pesquisar, retornando uma lista
 */ 
long CTableBase::_Find(CppSQLite3DB *pDB, 
					   LPCTSTR sQuery, 
					   TABLELIST *pList, 
					   CRuntimeClass *pClass)
{
	/*
	ASSERT(pDB != NULL);
	ASSERT(sQuery != NULL);

//	char *p = CUtil::ConvertUnicodeToAnsi(sQuery);
	CStr s1(sQuery);
	CppSQLite3Query q = pDB->execQuery(s1);
//	delete p;

	long l = CTableBase::Fill(&q, pList, pClass);
	q.finalize();
	return l;
	*/
	ASSERT(0);
	return -1;
}

/**
 * Metodo para inserir um novo registro
 */
long CTableBase::Insert()
{
/*
	ASSERT(m_pDb != NULL);

	CString s = BuildInsertQuery();
//	char *p = CUtil::ConvertUnicodeToAnsi(s);
	CStr s1(s);
	long count = m_pDb->execDML(s1);
//	delete p;

	return count;
*/
	ASSERT(0);
	return -1;
}

/**
 * Metodo para atualizar um registro
 */ 
long CTableBase::Update()
{
/*
	ASSERT(m_pDb != NULL);

	CString s = BuildUpdateQuery();
//	char *p = CUtil::ConvertUnicodeToAnsi(s);
	CStr s1(s);
	long count = m_pDb->execDML(s1);
//	delete p;

	return count;
*/
	ASSERT(0);
	return -1;
}

/**
 * MEtodo para deletar um registro
 */
long CTableBase::Delete()
{
/*	ASSERT(m_pDb != NULL);

	CString s = BuildDeleteQuery();
//	char *p = CUtil::ConvertUnicodeToAnsi(s);
	CStr s1(s);
	long count = m_pDb->execDML(s1);
//	delete p;

	return count;
*/
	ASSERT(0);
	return -1;

}

/**
 * Metodo para construir a query de contagem de registros
 */
CString CTableBase::BuildCountQuery(BOOL bWhereWithKeyOnly)
{
	CString s;

	s.Format(_T("SELECT COUNT(*)\r\nFROM %s\r\n%s\r\n"), 
			 m_sTableName, 
			 BuildWhereList(bWhereWithKeyOnly));

	return s;
}

/**
 * Metodo para executar a contagem de registros
 */
long CTableBase::Count(CppSQLite3DB *pDB)
{
	long count = 0;

	CString s = BuildCountQuery();
	CStr s1(s);
	CppSQLite3Query q = pDB->execQuery(s1);
    if(!q.eof())
	{
		TCHAR *endp;
		CString s2(q.fieldValue(0));
		count = _tcstol(s2, &endp, 10);
	}

	q.finalize();

	return count;
}

void CTableBase::DoReset()
{
}

CString CTableBase::GetValueList(BOOL bUseNULL, BOOL bExport)
{
	CString s = _T("");

	// Recuperar a lista de campos pela ordenacao de
	// importacao/Exportacao
	POSITION p = m_fieldOrdered.GetHeadPosition();
	CField *pField = NULL;
	int counter = 0;

	while(p != NULL)
	{
		CString sKey = m_fieldOrdered.GetNext(p);
		VERIFY(m_mapFields.Lookup(sKey, pField));

		if(counter++ > 0)
			s += _T(",");

		if(!pField->IsNull())
			s += pField->GetQueryFormatValue();
		else
		{
			// Se puder usar NULL ...
			if(bUseNULL)
				s += _T("NULL");
			else
			{
				if(bExport)
				{
					if(pField->GetType() == CField::FIELD_TYPE_CURRENCY)
						s += _T("");
					else if(pField->GetType() == CField::FIELD_TYPE_NUMBER)
						s += _T("");
					else
						s += _T("\"\"");
				}
				else
				{
					// Caso contrario, mandamos Zero para os numericos ...
					if(pField->GetType() == CField::FIELD_TYPE_CURRENCY ||
						pField->GetType() == CField::FIELD_TYPE_NUMBER)
					{
						s += _T("");
					}
					else // e vazio para os outros ...
						s += _T("");
				}
			}
		}
	}

	s += _T("\r\n");

	return s;
}

long CTableBase::List(TABLELIST *pList, BOOL bIgnoreNULL, BOOL bUseOnlyKeys)
{
/*	ASSERT(m_pDb != NULL);
	CString sQuery = BuildSelectQuery(bUseOnlyKeys, bIgnoreNULL);

//CUtil::Trace(sQuery);

//	char *p = CUtil::ConvertUnicodeToAnsi(sQuery);
	CStr s1(sQuery);
	CppSQLite3Query q = m_pDb->execQuery(s1);
//	delete p;

	long l = CTableBase::Fill(&q, pList, GetRuntimeClass());
	q.finalize();
	return l;
*/
ASSERT(0);
	return -1;
}

void CTableBase::Copy(CTableBase *pSrc)
{ 
	Destroy();

	// since v. 3.3.6 ...
	m_sTableName = pSrc->m_sTableName;

	POSITION pos = pSrc->GetFieldMap()->GetStartPosition();
	CString sKey;
	CField *pField;

	while(pos != NULL)
	{
		pSrc->GetFieldMap()->GetNextAssoc( pos, sKey, pField );
		CField *pField1 = new CField();
		pField1->Copy(pField);
		m_mapFields.SetAt(sKey, pField1);
	}
}

CString	CTableBase::GetStringValueNoNULL(LPCTSTR _szLabel)
{
	CString s = GetStringValue(_szLabel);
	if(s.CompareNoCase(_T("NULL")) == 0)
		return _T("");
	return s;
}

BOOL CTableBase::Exists(BOOL bWhereWithKeyOnly)
{
/*	ASSERT(m_pDb != NULL);

	CString s;
	s.Format(_T("SELECT 1\r\nFROM %s\r\n%s\r\n%s\r\n"), 
			 m_sTableName, 
			 BuildWhereList(bWhereWithKeyOnly, FALSE),
			 BuildOrderByList());

//	char *p = CUtil::ConvertUnicodeToAnsi(s);
	CStr s1(s);
	CppSQLite3Query q = m_pDb->execQuery(s1);
//	delete p;

	BOOL bReturn = TRUE;

    if(q.eof())
		bReturn = FALSE;
	else
	{
		CString s1(q.fieldValue(0));
		if(s1.CompareNoCase(_T("1")) != 0)
			bReturn = FALSE;
	}

	q.finalize();
	return bReturn;
*/
ASSERT(0);
	return FALSE;
}

BOOL CTableBase::BeginTransaction(CppSQLite3DB *pDB)
{
	try
	{
		pDB->execDML("begin transaction;");
	}
	catch(CppSQLite3Exception e)
	{
		STLOG_WRITE("Erro iniciando transação: %s", e.errorMessage());
		return FALSE;
	}
	return TRUE;
}

void CTableBase::RollbackTransaction(CppSQLite3DB *pDB)
{	
	try
	{
		pDB->execDML("rollback transaction;");
	}
	catch(CppSQLite3Exception e)
	{
		STLOG_WRITE("Erro voltando transação: %s", e.errorMessage());
	}
}

void CTableBase::CommitTransaction(CppSQLite3DB *pDB)
{
	try
	{
		pDB->execDML("commit transaction;");
	}
	catch(CppSQLite3Exception e)
	{
		STLOG_WRITE("Erro confirmando transação: %s", e.errorMessage());
	}
}

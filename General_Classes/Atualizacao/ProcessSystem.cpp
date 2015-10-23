#include "StdAfx.h"
#include "ProcessSystem.h"
#include "SimpleRequest.h"
#ifdef USA_TEM
	#include "Process_TEM.h"
	#include "ManageDB_TEM.h"
#endif
//#include "constants_contrato.h"
#ifdef MODULO_TEM
	#include "Consultas_TEM.h"
	#include "DataMigration.h"
	#include "AitSend.h"
#ifdef _WIN32_WCE
	#include "LogMigrationViewDlg.h"
#endif
	#include "IdentificacaoDlg.h"
#endif
#include "Utils.h"
#include "TableStruct.h"
#ifdef USA_GERINCID
#include "ManageDB_GerIncid.h"
#include "Process_Gerincid2.h"
#endif

CProcessSystem::CProcessSystem(CppSQLite3DB *pDB, CWnd* pWnd, LPCTSTR szCodigo)
{
	m_pUpdateDlg = reinterpret_cast<CUpdateDlg*>(pWnd);
	m_pDB = pDB;
	m_sCodigo = szCodigo;
}

CProcessSystem::CProcessSystem()
{
	
}

CProcessSystem::~CProcessSystem(void)
{
}

BOOL CProcessSystem::ProcessSystem()
{
#ifdef USA_TEM
	if(!m_pUpdateDlg)
	{
		STLOG_WRITE("%s(%d): Tela do UpdateDlg não configurada", __FUNCTION__, __LINE__);
		return FALSE;
	}
	
	CString m_sStructPath = CUtil::GetAppPath();
	if(m_sStructPath.GetAt(m_sStructPath.GetLength()-1) != '\\')
		m_sStructPath += L"\\";
	m_sStructPath += L"EstruturaBD.txt";

	CTableStruct *szCTableStruct = new CTableStruct(m_sStructPath);

	CList<TABLE_STRUCT,TABLE_STRUCT> lstEstrutura;
	szCTableStruct->GetTabela( L"aipte", &lstEstrutura );
	delete szCTableStruct;

	CProcess_TEM tem(CppSQLite3DB::getInstance(), m_pUpdateDlg, m_sCodigo, m_pUpdateDlg->m_params, &lstEstrutura);
	tem.ProcessSystem();
#endif

#ifdef USA_GERINCID
	CProcess_Gerincid2 process_g2(CppSQLite3DB::getInstance(), m_pUpdateDlg, m_sCodigo, m_pUpdateDlg->m_params, NULL);
	process_g2.ProcessSystem();
#endif
	return TRUE;
}

void CProcessSystem::LoadTable(LPCTSTR szSystem, __TABLEMAP *table, __INDEXMAP *index)
{
#ifdef USA_TEM
	if(CString(szSystem).CompareNoCase(L"TEM")==0)
	{

		CManageDB_TEM db;
		db.FillListTableIndex(table, index);

	}
	else
#endif
	{
#ifdef USA_GERINCID
		if(CString(szSystem).CompareNoCase(L"GERINCID")==0)
		{
			CManageDB_GerIncid dbg;
			dbg.FillListTableIndex(table, index);
		}
#endif
	}
}
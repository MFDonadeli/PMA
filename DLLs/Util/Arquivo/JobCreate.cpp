#include "StdAfx.h"
#include "JobCreate.h"
#include "Utils.h"


/**
\brief Construtor da classe
\param void
\return void
*/
CJobCreate::CJobCreate(LPCTSTR szJobFilePath)
{
	m_sJobFilePath = szJobFilePath;	
}


/**
\brief Destrutor da classe
\param void
\return void
*/
CJobCreate::~CJobCreate(void)
{
}

void CJobCreate::SetFileInfoSection(LPCTSTR sPostFileVarName, LPCTSTR sPathSendFile)
{
	m_sJobContent.AppendFormat(L"[FILE_INFO]\r\n\r\n");
	m_sJobContent.AppendFormat(L"VAR_FILE_NAME = %s\r\n", sPostFileVarName);
	m_sJobContent.AppendFormat(L"FILE_PATH = %s\r\n\r\n", sPathSendFile);
}


void CJobCreate::SetVarsInfoSection(LPCTSTR sVarsList)
{
	m_sJobContent.AppendFormat(L"[VARS_INFO]\r\n\r\n");
	m_sJobContent.AppendFormat(L"VARS_LIST = %s\r\n\r\n", sVarsList);
}


void CJobCreate::SetOnTxOkSection(LPCTSTR sFuncName, LPCTSTR sTabela, LPCTSTR sKeyName, LPCTSTR sKeyValue)
{
	m_sJobContent.AppendFormat(L"[ON_TX_OK]\r\n\r\n");
	m_sJobContent.AppendFormat(L"EXEC_FUNC_ON_TX_OK = %s\r\n", sFuncName);
	m_sJobContent.AppendFormat(L"TABELA = %s\r\n", sTabela);
	m_sJobContent.AppendFormat(L"KEY_NAME = %s\r\n", sKeyName);
	m_sJobContent.AppendFormat(L"KEY_VALUE = %s\r\n\r\n", sKeyValue);
}


/**
\brief Abre sessão principal
\param LPCTSTR sSectionName Nome da sessão
\return void
*/
void CJobCreate::OpenSection(LPCTSTR sSectionName)
{
	m_sJobContent.AppendFormat(L"[%s]\r\n\r\n", sSectionName);
}


/**
\brief Cria elemento do tipo campo = valor
\param LPCTSTR sVarName Nome da variável
\param LPCTSTR sValue Valor da variável
\return void
*/
void CJobCreate::AddVar(LPCTSTR sVarName, LPCTSTR sValue)
{	
	m_sJobContent.AppendFormat(L"%s = %s\r\n",sVarName, sValue);	
}

/**
\brief Cria elemento do tipo campo = valor
\param LPCTSTR sVarName Nome da variável
\param double sValue Valor da variável
\return void
*/
void CJobCreate::AddVar(LPCTSTR sVarName, double sValue)
{		
	m_sJobContent.AppendFormat(L"%s = %f\r\n",sVarName, sValue);		
}

/**
\brief Cria elemento do tipo campo = valor
\param LPCTSTR sVarName Nome da variável
\param LPCTSTR int Valor da variável
\return void
*/
void CJobCreate::AddVar(LPCTSTR sVarName, int sValue)
{		
	m_sJobContent.AppendFormat(L"%s = %d\r\n",sVarName, sValue);		
}


/**
\brief Cria arquivo Job físico
\return BOOL
*/
BOOL CJobCreate::Create()
{
	//Cria arquivo job de envio		
	CFile fJobFile;	
	CFileException e;

	if(fJobFile.Open(m_sJobFilePath, CFile::modeCreate | CFile::modeWrite, &e))
	{
		fJobFile.Write(CStringA (m_sJobContent), CStringA (m_sJobContent).GetLength());		
		fJobFile.Close();

		//Renomea para .job p/ a thread lê-lo...
		CUtil::MoveFileOverwrite(m_sJobFilePath, m_sJobFilePath+L".job");
		return TRUE;
	}
	else
	{
		STLOG_WRITE(L"%S(%d): Falha ao criar job: [%d]", __FUNCTION__, __LINE__, e.m_cause);
	}
		
	
	return FALSE;
}



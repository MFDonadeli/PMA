#include "StdAfx.h"
#include "VersaoApp.h"

#include "versao_smi.h"

CVersao::CVersao(void)
{
}

CVersao::~CVersao(void)
{
}


/**
\brief Retorna a versão do SMI do contrato
\param void
\return String contendo a versão do contrato
*/
CString CVersao::GetAppVersion()
{
	CString sVersao;
	CString sFullPath/* = CString(_SOLUTION_PATH_)*/;
	CString sContrato;
	

	int size = 5;

	sFullPath = sFullPath.Left(sFullPath.GetLength() - 1);
	size = sFullPath.ReverseFind(L'\\');

	sContrato = sFullPath.Mid(size+1);

#ifdef BETA_VERSION
	sVersao.Format(L"SMI%s", VERSAO_SMI/*, sContrato, VERSAO_SMI, VERSAO_CONTRATO*/);
#else
	sVersao.Format(L"SMI%s_%s-%s.%s", VERSAO_SMI, sContrato, VERSAO_SMI, VERSAO_CONTRATO);
#endif
	
	return sVersao;
}
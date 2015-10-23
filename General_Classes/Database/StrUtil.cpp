#include "stdafx.h"
#include "StrUtil.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

/*
ERROR_INSUFFICIENT_BUFFER	 122L
ERROR_INVALID_FLAGS			 1004L
ERROR_INVALID_PARAMETER		 87L
ERROR_NO_UNICODE_TRANSLATION 1113L
*/
//////////////////////////////////////////////////////////////////////////////////
BOOL CStrUtil::AnsiToUnicode(LPCSTR szInput, WCHAR **szOutput, int *nSize)
{
	if(szOutput != NULL)
		delete szOutput;

	*nSize = 0;
	// Calcula o tamanho...
	int needed = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szInput, -1, NULL, 0);
	if(needed > 0)
	{
		// Aloca...
		*szOutput = new WCHAR[needed];
		ZeroMemory(*szOutput, needed);

		// Converte...
		*nSize = MultiByteToWideChar(CP_ACP, 
									 MB_PRECOMPOSED, 
									 szInput, 
									 -1, 
									 *szOutput, 
									 needed);
		// Erro ?
		if(*nSize == 0 || *nSize != needed)
		{
			delete []szOutput;
			DWORD dwErr = GetLastError();
			return FALSE;
		}

		return TRUE;
	}

	return FALSE;
}

BOOL CStrUtil::UnicodeToAnsi(LPCWSTR szInput, char *szOutput, int size)
{
	if(WideCharToMultiByte(CP_ACP, 0, szInput, -1, szOutput, size, NULL, NULL) == 0)
		return FALSE;

	return TRUE;
}

BOOL CStrUtil::UnicodeToAnsi(LPCWSTR szInput, char **szOutput, int *nSize)
{
	if(szOutput != NULL)
		delete szOutput;

	*nSize = 0;
	// Calcula o tamanho...
	int needed = WideCharToMultiByte(CP_ACP, 0, szInput, -1, NULL, 0, NULL, NULL);
	if(needed > 0)
	{
		// Aloca...
		*szOutput = new char[needed];
		ZeroMemory(*szOutput, needed);

		// Converte...
		*nSize = WideCharToMultiByte(CP_ACP, 0, szInput, -1, *szOutput, needed, NULL, NULL);

		// Erro ?
		if(*nSize == 0 || *nSize != needed)
		{
			delete []szOutput;
			DWORD dwErr = GetLastError();
			return FALSE;
		}

		return TRUE;
	}	

	return FALSE;
}


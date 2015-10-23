// Copyright (c) 2007 Marcos Mori de Siqueira. All Rights Reserved. 
// mori@softfactory.com.br
// Classe que encapsula funcoes de acesso ao registry
#include "stdafx.h"
#include <windows.h>
#include "registry.h"
#include "utils.h"

/*------------------------------------------------------------------------------------*\
   Registry::Registry(HKEY hSessionParam, LPCTSTR sKey)

   HKEY hSessionParam   - Sessao do registry (definida pela API do Windows)
							HKEY_CLASSES_ROOT,
							HKEY_CURRENT_USER,
							HKEY_LOCAL_MACHINE,
							HKEY_USERS
   LPCTSTR sKey         - Chave de accesso ao registry

   Retorno....: Nao ha retorno.
   Descricao..: Inicializa as variáveis pertencentes a classe.
   
\*------------------------------------------------------------------------------------*/
Registry::Registry(HKEY hSessionParam, LPCTSTR sKey)
{
	_Init();

	hSession = hSessionParam;
	lstrcpy(szKey, sKey);
}

/*------------------------------------------------------------------------------------*\
   Registry::Registry(HKEY hSessionParam)

   HKEY hSessionParam   - Sessao do registry (definida pela API do Windows)
							HKEY_CLASSES_ROOT,
							HKEY_CURRENT_USER,
							HKEY_LOCAL_MACHINE,
							HKEY_USERS

   Retorno....: Nao ha retorno.
   Descricao..: Inicializa as variáveis pertencentes a classe.
   
\*------------------------------------------------------------------------------------*/
Registry::Registry(HKEY hSessionParam)
{
	_Init();

	hSession = hSessionParam;
}

Registry::~Registry()
{
	Close();
}

void Registry::_Init()
{
	hSession		= NULL;
	isOpened		= FALSE;
	dwOptions		= REG_OPTION_NON_VOLATILE;
	samDesired		= KEY_ALL_ACCESS;
	dwDisposition	= REG_CREATED_NEW_KEY;
	hKey			= NULL;

	ZeroMemory(szKey  , sizeof(szKey));
	ZeroMemory(szClass, sizeof(szClass));
	ZeroMemory(&lpSecAttributes, sizeof(lpSecAttributes));
}

/*------------------------------------------------------------------------------------*\
   BOOL Registry::Open()

   Retorno....: Boolean indicando o sucesso da operacao
   Descricao..: Abre a chave do registry, criando-a se necessário. Pode-se vericar se
                a chave precisou ser criada atraves da funcao WasCreated()
   
\*------------------------------------------------------------------------------------*/
BOOL Registry::Open()
{
	if(isOpened)
		return TRUE;

//	if(!_StartSecurity())
//		return FALSE;

    lpSecAttributes.nLength				 = sizeof (SECURITY_ATTRIBUTES);
    lpSecAttributes.lpSecurityDescriptor = NULL;
    lpSecAttributes.bInheritHandle		 = FALSE;

	nError = RegCreateKeyEx(hSession,
							szKey,
							0,
							szClass,
							dwOptions,
							samDesired,
							&lpSecAttributes,
							&hKey,
							&dwDisposition);

	if(nError != ERROR_SUCCESS)
	{
		m_sLastError.Format(L"Registry::Open: %s", CUtil::ErrorString(nError));
		return FALSE;
	}

	return (isOpened = (nError == ERROR_SUCCESS));
}

/*------------------------------------------------------------------------------------*\
   BOOL Registry::Open(HKEY hSessionParam, LPCTSTR sKey)

   HKEY hSessionParam   - Sessao do registry (definida pela API do Windows)
							HKEY_CLASSES_ROOT,
							HKEY_CURRENT_USER,
							HKEY_LOCAL_MACHINE,
							HKEY_USERS
   LPCTSTR sKey         - Chave de accesso ao registry

   Retorno....: Boolean indicando o sucesso da operacao
   Descricao..: Abre a chave do registry, criando-a se necessário. Pode-se vericar se
                a chave precisou ser criada atraves da funcao WasCreated()
   
\*------------------------------------------------------------------------------------*/
BOOL Registry::Open(HKEY hSessionParam, LPCTSTR sKey)
{
	_Init();

	hSession = hSessionParam;
	lstrcpy(szKey, sKey);

	if(isOpened)
		return TRUE;

//	if(!_StartSecurity())
//		return FALSE;

    lpSecAttributes.nLength				 = sizeof (SECURITY_ATTRIBUTES);
    lpSecAttributes.lpSecurityDescriptor = NULL;
    lpSecAttributes.bInheritHandle		 = FALSE;

	nError = RegCreateKeyEx(hSession,
							szKey,
							0,
							szClass,
							dwOptions,
							samDesired,
							&lpSecAttributes,
							&hKey,
							&dwDisposition);

	if(nError != ERROR_SUCCESS)
	{
		m_sLastError.Format(L"Registry::Open: %s", CUtil::ErrorString(nError));
		return FALSE;
	}

	return (isOpened = (nError == ERROR_SUCCESS));
}

/*------------------------------------------------------------------------------------*\
   BOOL Registry::Close()

   Retorno....: Boolean indicando o sucesso da operacao
   Descricao..: Fecha a chave do registry.
   
\*------------------------------------------------------------------------------------*/
BOOL Registry::Close()
{
	nError = RegCloseKey(hKey);
	m_sLastError = CUtil::ErrorString(nError);

	return (!(isOpened = !(nError == ERROR_SUCCESS)));
}

/*------------------------------------------------------------------------------------*\
   BOOL Registry::SetDefault(DWORD dwValue)

      DWORD dwValue  - Valor a ser atribuido

   Retorno....: Boolean indicando o sucesso da operacao
   Descricao..: Seta um valor numérico ao item default.
   
\*------------------------------------------------------------------------------------*/
BOOL Registry::SetDefault(DWORD dwValue)
{
	return SetValue(_T(""), dwValue);
}

/*------------------------------------------------------------------------------------*\
   BOOL Registry::SetDefault(LPCTSTR szValue)

      LPCTSTR szValue   - String a ser atribuida

   Retorno....: Boolean indicando o sucesso da operacao
   Descricao..: Seta um valor alfanumérico ao item default.
   
\*------------------------------------------------------------------------------------*/
BOOL Registry::SetDefault(LPCTSTR szValue)
{
	return SetValue(_T(""), szValue);
}

/*------------------------------------------------------------------------------------*\
   BOOL Registry::SetDefault(LPCTSTR sValue, DWORD dwLen)

      LPCTSTR szValue   - Serie de strings terminadas por dois nulls
      DWORD dwLen       - Tamanho do buffer szValue

   Retorno....: Boolean indicando o sucesso da operacao
   Descricao..: Atribui varias strings terminados por dois nulls ao item default
   
\*------------------------------------------------------------------------------------*/
BOOL Registry::SetDefault(LPCTSTR sValue, DWORD dwLen)
{
	return SetValue(_T(""), sValue, dwLen);
}

/*------------------------------------------------------------------------------------*\
   BOOL Registry::SetDefault(LPVOID mValue, DWORD dwLen)

      LPVOID mValue   - Ponteiro indicando o dado a ser atribuido
      DWORD dwLen     - Tamanho da regiao de memoria indicada por mValue

   Retorno....: Boolean indicando o sucesso da operacao
   Descricao..: Atribui uma regiao de memoria ao item default
   
\*------------------------------------------------------------------------------------*/
BOOL Registry::SetDefault(LPVOID mValue, DWORD dwLen)
{
	return SetValue(_T(""), mValue, dwLen);
}

/*------------------------------------------------------------------------------------*\
   BOOL Registry::SetDefault(LPBYTE mValue, DWORD dwLen)

      LPBYTE mValue  - Valor binario a ser atribuido
      DWORD dwLen    - Tamanho do dado binario

   Retorno....: Boolean indicando o sucesso da operacao
   Descricao..: Atribui um valor binario ao item default
   
\*------------------------------------------------------------------------------------*/
BOOL Registry::SetDefault(LPBYTE mValue, DWORD dwLen)
{
	return SetValue(_T(""), mValue, dwLen);
}

/*------------------------------------------------------------------------------------*\
   BOOL Registry::SetUnexpandedDefault(LPCTSTR szValue)

      LPCTSTR szValue   - String expandivel a ser atribuida

   Retorno....: Boolean indicando o sucesso da operacao
   Descricao..: Atribui uma string expandivel ao item default. Strings expandiveis
                sao aquelas que indicam uma variavel de ambiente e sao expandidas
                atraves da funcao ExpandEnvironmentStrings(). Por exemplo: %PATH%
   
\*------------------------------------------------------------------------------------*/
BOOL Registry::SetUnexpandedDefault(LPCTSTR szValue)
{
	return SetUnexpandedValue(_T(""), szValue);
}

/*------------------------------------------------------------------------------------*\
   BOOL Registry::SetValue(LPCTSTR szEntry, DWORD dwValue)

      LPCTSTR szEntry   - Item dentro da chave de registry
      DWORD dwValue     - Valor a ser atribuido

   Retorno....: Boolean indicando o sucesso da operacao
   Descricao..: Seta um valor numérico ao item especificado
   
\*------------------------------------------------------------------------------------*/
BOOL Registry::SetValue(LPCTSTR szEntry, DWORD dwValue)
{
	if (!isOpened)
		if (!Open())
			return FALSE;

	nError = RegSetValueEx(hKey, szEntry, 0, REG_DWORD, (CONST BYTE*)&dwValue, sizeof(dwValue));
	m_sLastError = CUtil::ErrorString(nError);
	return (nError == ERROR_SUCCESS);
}

/*------------------------------------------------------------------------------------*\
   BOOL Registry::SetValue(LPCTSTR szEntry, LPCTSTR szValue)

      LPCTSTR szEntry   - Item dentro da chave de registry
      LPCTSTR szValue   - String a ser atribuida

   Retorno....: Boolean indicando o sucesso da operacao
   Descricao..: Atribui uma atring ao item especificado
   
\*------------------------------------------------------------------------------------*/
BOOL Registry::SetValue(LPCTSTR szEntry, LPCTSTR szValue)
{
	if (!isOpened)
		if (!Open())
			return FALSE;

	nError = RegSetValueEx(hKey, 
						   szEntry, 
						   0, 
						   REG_SZ, 
						   (CONST BYTE*)szValue, 
						   lstrlen(szValue) * sizeof(TCHAR));

	m_sLastError = CUtil::ErrorString(nError);
	return (nError == ERROR_SUCCESS);
}

/*------------------------------------------------------------------------------------*\
   BOOL Registry::SetValue(LPCTSTR szEntry, LPCTSTR sValue, DWORD dwLen)

      LPCTSTR szEntry   - Item dentro da chave de registry
      LPCTSTR szValue   - Serie de strings terminadas por dois nulls
      DWORD dwLen       - Tamanho do buffer szValue

   Retorno....: Boolean indicando o sucesso da operacao
   Descricao..: Atribui varias strings terminados por dois nulls ao item especificado
   
\*------------------------------------------------------------------------------------*/
BOOL Registry::SetValue(LPCTSTR szEntry, LPCTSTR sValue, DWORD dwLen)
{
	if (!isOpened)
		if (!Open())
			return FALSE;

	nError = RegSetValueEx(hKey, szEntry, 0, REG_MULTI_SZ, (CONST BYTE*)sValue, dwLen);
	m_sLastError = CUtil::ErrorString(nError);
	return (nError == ERROR_SUCCESS);
}

/*------------------------------------------------------------------------------------*\
   BOOL Registry::SetValue(LPCTSTR szEntry, LPVOID mValue, DWORD dwLen)

      LPCTSTR szEntry   - Item dentro da chave de registry
      LPVOID mValue     - Ponteiro indicando o dado a ser atribuido
      DWORD dwLen       - Tamanho da regiao de memoria indicada por mValue

   Retorno....: Boolean indicando o sucesso da operacao
   Descricao..: Atribui uma regiao de memoria ao item especificado
   
\*------------------------------------------------------------------------------------*/
BOOL Registry::SetValue(LPCTSTR szEntry, LPVOID mValue, DWORD dwLen)
{
	if (!isOpened)
		if (!Open())
			return FALSE;

	nError = RegSetValueEx(hKey, szEntry, 0, REG_NONE, (CONST BYTE*)mValue, dwLen);
	m_sLastError = CUtil::ErrorString(nError);
	return (nError == ERROR_SUCCESS);
}

/*------------------------------------------------------------------------------------*\
   BOOL Registry::SetValue(LPCTSTR szEntry, LPBYTE mValue, DWORD dwLen)

      LPCTSTR szEntry   - Item dentro da chave de registry
      LPBYTE mValue     - Valor binario a ser atribuido
      DWORD dwLen       - Tamanho do dado binario

   Retorno....: Boolean indicando o sucesso da operacao
   Descricao..: Atribui um valor binario ao item especificado
   
\*------------------------------------------------------------------------------------*/
BOOL Registry::SetValue(LPCTSTR szEntry, LPBYTE mValue, DWORD dwLen)
{
	if (!isOpened)
		if (!Open())
			return FALSE;

	nError = RegSetValueEx(hKey, szEntry, 0, REG_BINARY, (CONST BYTE*)mValue, dwLen);
	m_sLastError = CUtil::ErrorString(nError);
	return (nError == ERROR_SUCCESS);
}

/*------------------------------------------------------------------------------------*\
   BOOL Registry::SetUnexpandedValue(LPCTSTR szEntry, LPCTSTR szValue)

      LPCTSTR szEntry   - Item dentro da chave de registry
      LPCTSTR szValue   - String expandivel a ser atribuida

   Retorno....: Boolean indicando o sucesso da operacao
   Descricao..: Atribui uma string expandivel ao item default. Strings expandiveis
                sao aquelas que indicam uma variavel de ambiente e sao expandidas
                atraves da funcao ExpandEnvironmentStrings(). Por exemplo: %PATH%
   
\*------------------------------------------------------------------------------------*/
BOOL Registry::SetUnexpandedValue(LPCTSTR szEntry, LPCTSTR szValue)
{
	if (!isOpened)
		if (!Open())
			return FALSE;

	nError = RegSetValueEx(hKey, szEntry, 0, REG_EXPAND_SZ, (CONST BYTE*)szValue, lstrlen(szValue)+1);
	m_sLastError = CUtil::ErrorString(nError);
	return (nError == ERROR_SUCCESS);
}

/*------------------------------------------------------------------------------------*\
   BOOL Registry::GetDefault(LPDWORD dwValue)

      LPDWORD dwValue   - Ponteiro para receber o valor lido

   Retorno....: Boolean indicando o sucesso da operacao
   Descricao..: Le o valor numerico associado ao item default
   
\*------------------------------------------------------------------------------------*/
BOOL Registry::GetDefault(LPDWORD dwValue)
{
	return GetValue(_T(""), dwValue);
}

/*------------------------------------------------------------------------------------*\
   BOOL Registry::GetDefault(LPTSTR szValue, LPDWORD dwLen)

      LPTSTR szValue   - Area para receber o valor lido
      LPDWORD dwLen    - Tamanho da area.

   Retorno....: Boolean indicando o sucesso da operacao
   Descricao..: Le a string associada ao item default
   
\*------------------------------------------------------------------------------------*/
BOOL Registry::GetDefault(LPTSTR szValue, LPDWORD dwLen)
{
	return GetValue(_T(""), szValue, dwLen);
}

/*------------------------------------------------------------------------------------*\
   BOOL Registry::GetDefault(LPVOID mValue, LPDWORD dwLen)

      LPVOID mValue   - Area para receber o valor
      LPDWORD dwLen   - Tamanho da area.

   Retorno....: Boolean indicando o sucesso da operacao
   Descricao..: Le um dado qualquer associado ao item default
   
\*------------------------------------------------------------------------------------*/
BOOL Registry::GetDefault(LPVOID mValue, LPDWORD dwLen)
{
	return GetValue(_T(""), mValue, dwLen);
}

/*------------------------------------------------------------------------------------*\
   BOOL Registry::GetDefault(LPBYTE mValue, LPDWORD dwLen)

      LPBYTE mValue   - Area para receber o valor
      LPDWORD dwLen   - Tamanho da area.

   Retorno....: Boolean indicando o sucesso da operacao
   Descricao..: Le um dado binario associado ao item default
   
\*------------------------------------------------------------------------------------*/
BOOL Registry::GetDefault(LPBYTE mValue, LPDWORD dwLen)
{
	return GetValue(_T(""), mValue, dwLen);
}

/*------------------------------------------------------------------------------------*\
   BOOL Registry::GetUnexpandedDefault(LPTSTR szValue, LPDWORD dwLen)

      LPTSTR szValue   - Area para receber o valor lido
      LPDWORD dwLen    - Tamanho da area.

   Retorno....: Boolean indicando o sucesso da operacao
   Descricao..: Leu uma string expandivel associada ao item default. Strings 
                expandiveis sao aquelas que indicam uma variavel de ambiente e sao 
                expandidas atraves da funcao ExpandEnvironmentStrings(). Ex: %PATH%
   
\*------------------------------------------------------------------------------------*/
BOOL Registry::GetUnexpandedDefault(LPTSTR szValue, LPDWORD dwLen)
{
	return GetUnexpandedValue(_T(""), szValue, dwLen);
}

/*------------------------------------------------------------------------------------*\
   BOOL Registry::GetValue(LPCTSTR szEntry, LPDWORD dwValue)

      LPCTSTR szEntry   - Item dentro da chave de registry
      LPDWORD dwValue   - Ponteiro para receber o valor lido

   Retorno....: Boolean indicando o sucesso da operacao
   Descricao..: Le o valor numerico associado ao item especificado
   
\*------------------------------------------------------------------------------------*/
BOOL Registry::GetValue(LPCTSTR szEntry, LPDWORD dwValue)
{
	DWORD dwLen = sizeof(DWORD);	// Tamanho da area de dados
	DWORD dwType;					// Tipo do dado lido

	if (!isOpened)
		if (!Open())
			return FALSE;

	nError = RegQueryValueEx(hKey, szEntry, 0, &dwType, (LPBYTE)dwValue, &dwLen);

	if (dwType != REG_DWORD && nError == ERROR_SUCCESS)
		nError = ERROR_INVALID_DATATYPE;

	m_sLastError = CUtil::ErrorString(nError);

	return (nError == ERROR_SUCCESS);
}

/*------------------------------------------------------------------------------------*\
   BOOL Registry::GetValue(LPCTSTR szEntry, LPTSTR szValue, LPDWORD dwLen)

      LPCTSTR szEntry   - Item dentro da chave de registry
      LPTSTR szValue    - Area para receber o valor lido
      LPDWORD dwLen     - Tamanho da area.

   Retorno....: Boolean indicando o sucesso da operacao
   Descricao..: Le a string associada ao item especificado
   
\*------------------------------------------------------------------------------------*/
BOOL Registry::GetValue(LPCTSTR szEntry, LPTSTR szValue, LPDWORD dwLen)
{
	if(!isOpened)
		if(!Open())
			return FALSE;

	DWORD dwType; // Tipo do dado lido
	nError = RegQueryValueEx(hKey, 
							 szEntry, 
							 0, 
							 &dwType, 
							 (LPBYTE)szValue, 
							 dwLen);

	if(nError != ERROR_SUCCESS)
	{
		m_sLastError.Format(L"Registry::GetValue: %s %x", CUtil::ErrorString(nError), hKey);
		//TRACE(m_sLastError);
	}

	if (dwType != REG_SZ && dwType != REG_MULTI_SZ && nError == ERROR_SUCCESS)
		nError = ERROR_INVALID_DATATYPE;

	return (nError == ERROR_SUCCESS);
}

/*------------------------------------------------------------------------------------*\
   BOOL Registry::GetValue(LPCTSTR szEntry, LPVOID mValue, LPDWORD dwLen)

      LPCTSTR szEntry   - Item dentro da chave de registry
      LPVOID mValue     - Area para receber o valor
      LPDWORD dwLen     - Tamanho da area.

   Retorno....: Boolean indicando o sucesso da operacao
   Descricao..: Le um dado qualquer associado ao item especificado
   
\*------------------------------------------------------------------------------------*/
BOOL Registry::GetValue(LPCTSTR szEntry, LPVOID mValue, LPDWORD dwLen)
{
	DWORD dwType;                  // Tipo do dado lido

	if (!isOpened)
		if (!Open())
			return FALSE;

	nError = RegQueryValueEx(hKey, szEntry, 0, &dwType, (LPBYTE)mValue, dwLen);

	if (dwType != REG_NONE && nError == ERROR_SUCCESS)
		nError = ERROR_INVALID_DATATYPE;

	if(nError != ERROR_SUCCESS)
	{
		CString s = CUtil::ErrorString(nError);
		//TRACE(s);
	}
	
	return (nError == ERROR_SUCCESS);
}

/*------------------------------------------------------------------------------------*\
   BOOL Registry::GetValue(LPCTSTR szEntry, LPBYTE mValue, LPDWORD dwLen)

      LPCTSTR szEntry   - Item dentro da chave de registry
      LPBYTE mValue     - Area para receber o valor
      LPDWORD dwLen     - Tamanho da area.

   Retorno....: Boolean indicando o sucesso da operacao
   Descricao..: Le um dado binario associado ao item especificado
   
\*------------------------------------------------------------------------------------*/
BOOL Registry::GetValue(LPCTSTR szEntry, LPBYTE mValue, LPDWORD dwLen)
{
	DWORD dwType;                  // Tipo do dado lido

	if (!isOpened)
		if (!Open())
			return FALSE;

	nError = RegQueryValueEx(hKey, szEntry, 0, &dwType, (LPBYTE)mValue, dwLen);

	if (dwType != REG_BINARY && nError == ERROR_SUCCESS)
		nError = ERROR_INVALID_DATATYPE;

	m_sLastError = CUtil::ErrorString(nError);

	return (nError == ERROR_SUCCESS);
}

/*------------------------------------------------------------------------------------*\
   BOOL Registry::GetUnexpandedValue(LPCTSTR szEntry, LPTSTR szValue, LPDWORD dwLen)

      LPCTSTR szEntry  - Item dentro da chave de registry
      LPTSTR szValue   - Area para receber o valor lido
      LPDWORD dwLen    - Tamanho da area.

   Retorno....: Boolean indicando o sucesso da operacao
   Descricao..: Leu uma string expandivel associada ao item especificado. Strings 
                expandiveis sao aquelas que indicam uma variavel de ambiente e sao 
                expandidas atraves da funcao ExpandEnvironmentStrings(). Ex: %PATH%
   
\*------------------------------------------------------------------------------------*/
BOOL Registry::GetUnexpandedValue(LPCTSTR szEntry, LPTSTR szValue, LPDWORD dwLen)
{
	DWORD dwType;                  // Tipo do dado lido

	if (!isOpened)
		if (!Open())
			return FALSE;

	nError = RegQueryValueEx(hKey, szEntry, 0, &dwType, (LPBYTE)szValue, dwLen);

	if (dwType != REG_EXPAND_SZ && nError == ERROR_SUCCESS)
		nError = ERROR_INVALID_DATATYPE;

	m_sLastError = CUtil::ErrorString(nError);

	return (nError == ERROR_SUCCESS);
}

/*------------------------------------------------------------------------------------*\
   REGSAM Registry::SetLevelAccess(REGSAM samAccess)

      REGSAM samAccess   - Nível de segurança (permissões)

   Retorno....: Nivel de seguranca anteriormente usado
   Descricao..: Muda as permissões de acesso a chave. Esta função só tem utilidade se 
                usada antes de qualquer função GetX(), SetX(), DeleteX(), QueryX() ou 
                GetInfo().
   
\*------------------------------------------------------------------------------------*/
REGSAM Registry::SetLevelAccess(REGSAM samAccess)
{
	REGSAM samOld;

	samOld      = samDesired;
	samDesired  = samAccess;

	return samOld;
}

/*------------------------------------------------------------------------------------*\
   BOOL Registry::WasCreated()

   Retorno....: Boolean indicando se a chave foi criada (TRUE) ou acessada (FALSE)
   Descricao..: Verifica se a chave foi criada ou acessada
   
\*------------------------------------------------------------------------------------*/
BOOL Registry::WasCreated()
{
	return dwDisposition == REG_CREATED_NEW_KEY ? TRUE : FALSE;
}

/*------------------------------------------------------------------------------------*\
   VOID Registry::SetClass(LPCTSTR szNewClass)

      LPCTSTR szNewClass - Nova classe da chave

   Retorno....: Nao ha retorno
   Descricao..: Atribui uma nova classe a chave. Esta função so tem utilidade se usada 
                antes das funcoes GetX(), SetX(), DeleteX(), QueryX() ou GetInfo()
   
\*------------------------------------------------------------------------------------*/
VOID Registry::SetClass(LPCTSTR szNewClass)
{
	lstrcpy(szClass, szNewClass);
}

/*------------------------------------------------------------------------------------*\
   BOOL Registry::DeleteDefault()

   Retorno....: Boolean indicando o sucesso da operacao
   Descricao..: Apaga o item default da chave do registry
   
\*------------------------------------------------------------------------------------*/
BOOL Registry::DeleteDefault()
{
	return DeleteValue(_T(""));
}

/*------------------------------------------------------------------------------------*\
   BOOL Registry::DeleteValue(LPCTSTR szEntry)

      LPCTSTR szEntry   - Item dentro da chave de registry

   Retorno....: Boolean indicando o sucesso da operacao
   Descricao..: Apaga o item especificado da chave do registry
   
\*------------------------------------------------------------------------------------*/
BOOL Registry::DeleteValue(LPCTSTR szEntry)
{
	if (!isOpened)
		if (!Open())
			return FALSE;

	nError = RegDeleteValue(hKey, szEntry);
	m_sLastError = CUtil::ErrorString(nError);

	return (nError == ERROR_SUCCESS);
}

/*------------------------------------------------------------------------------------*\
   BOOL Registry::DeleteSubKey(LPCTSTR szSubKey)

   Retorno....: Boolean indicando o sucesso da operacao
   Descricao..: Apaga a subchave do registry
   
\*------------------------------------------------------------------------------------*/
BOOL Registry::DeleteSubKey(LPCTSTR szSubKey)
{
	if (!isOpened)
		if (!Open())
			return FALSE;

	nError = RegDeleteKey(hKey, szSubKey);
	m_sLastError = CUtil::ErrorString(nError);
	return (nError == ERROR_SUCCESS);
}
/*------------------------------------------------------------------------------------*\
   BOOL Registry::DeleteAllSubKey(LPCTSTR szSubKey)

   Retorno....: Boolean indicando o sucesso da operacao
   Descricao..: Apaga a subchave do registry
   
\*------------------------------------------------------------------------------------*/
//BOOL Registry::DeleteAllSubKey(LPTSTR szSubKey)
//{
//	return DeleteAllSubKey(hKey, szSubKey);
//}

/*------------------------------------------------------------------------------------*\
   BOOL Registry::DeleteAllSubKey(LPCTSTR szSubKey)

   Retorno....: Boolean indicando o sucesso da operacao
   Descricao..: Apaga a subchave do registry
   
\*------------------------------------------------------------------------------------*/
BOOL Registry::DeleteAllSubKey(HKEY hStartKey, LPTSTR pKeyName )
{
  DWORD   dwRtn, dwSubKeyLength;
  LPTSTR  pSubKey = NULL;
  TCHAR   szSubKey[256]; // (256) this should be dynamic.
  HKEY    hKeyTmp;

  // Do not allow NULL or empty key name
  if ( pKeyName &&  lstrlen(pKeyName))
  {
     if( (dwRtn=RegOpenKeyEx(hStartKey,pKeyName,
        0, KEY_ENUMERATE_SUB_KEYS | DELETE, &hKeyTmp )) == ERROR_SUCCESS)
     {
        while (dwRtn == ERROR_SUCCESS )
        {
           dwSubKeyLength = 256;
           dwRtn=RegEnumKeyEx(
                          hKeyTmp,
                          0,       // always index zero
                          szSubKey,
                          &dwSubKeyLength,
                          NULL,
                          NULL,
                          NULL,
                          NULL
                        );

           if(dwRtn == ERROR_NO_MORE_ITEMS)
           {
              dwRtn = RegDeleteKey(hStartKey, pKeyName);
              break;
           }
           else if(dwRtn == ERROR_SUCCESS)
              dwRtn=DeleteAllSubKey(hKeyTmp, szSubKey);
        }
        RegCloseKey(hKeyTmp);
        // Do not save return code because error
        // has already occurred
     }
  }
  else
     dwRtn = ERROR_BADKEY;

  return dwRtn;
}

/*------------------------------------------------------------------------------------*\
   BOOL Registry::GetInfo(LPDWORD dwSubKeys, LPDWORD dwMaxSubKeyLen, LPDWORD dwMaxClassLen, 
						  LPDWORD dwEntry  , LPDWORD dwMaxEntryLen , LPDWORD dwMaxValueLen, 
                          PFILETIME pLastWrite)

      LPDWORD dwSubKeys       - Numero de sub chaves
      LPDWORD dwMaxSubKeyLen  - Tamanho do nome da maior sub chave
      LPDWORD dwMaxClassLen   - Tamanho do nome da maior classe das sub chaves
      LPDWORD dwEntry         - Numero de ítens que a chave contém
      LPDWORD dwMaxEntryLen   - Tamanho do nome do maior ítem
      LPDWORD dwMaxValueLen   - Tamanho do maior valor dos ítens
      PFILETIME pLastWrite    - Hora da última modificação na chave

   Retorno....: Boolean indicando o sucesso da operacao
   Descricao..: Obtem informacoes sobre a chave do registry
   
\*------------------------------------------------------------------------------------*/
BOOL Registry::GetInfo(LPDWORD dwSubKeys, LPDWORD dwMaxSubKeyLen, LPDWORD dwMaxClassLen,
					   LPDWORD dwEntry  , LPDWORD dwMaxEntryLen , LPDWORD dwMaxValueLen,
					   PFILETIME pLastWrite)
{
	DWORD dwClassLen;
	DWORD dwSecurity;
	DWORD MaxSubKeyLen;
	DWORD MaxClassLen;
	DWORD Entry;
	DWORD MaxEntryLen;
	DWORD MaxValueLen;
	FILETIME LastWrite;

	if (!isOpened)
		if (!Open())
			return FALSE;

	nError = RegQueryInfoKey(hKey,szClass,&dwClassLen,NULL,dwSubKeys,&MaxSubKeyLen,&MaxClassLen, 
							&Entry,&MaxEntryLen,&MaxValueLen,&dwSecurity,&LastWrite);

	if(nError == ERROR_MORE_DATA)
	{
		TCHAR* sClass = new TCHAR(dwClassLen);
		nError = RegQueryInfoKey(hKey,sClass,&dwClassLen,NULL,dwSubKeys,&MaxSubKeyLen,&MaxClassLen, 
							&Entry,&MaxEntryLen,&MaxValueLen,&dwSecurity,&LastWrite);
		delete sClass;
	}

	if (dwMaxSubKeyLen)	*dwMaxSubKeyLen = MaxSubKeyLen;
	if (dwMaxClassLen)	*dwMaxClassLen	= MaxClassLen;
	if (dwEntry)		*dwEntry		= Entry;
	if (dwMaxEntryLen)	*dwMaxEntryLen	= MaxEntryLen;
	if (dwMaxValueLen)	*dwMaxValueLen	= MaxValueLen;
	if (pLastWrite)		MoveMemory((PVOID)pLastWrite, (CONST VOID*)&LastWrite, (DWORD)sizeof(LastWrite));

	return (nError == ERROR_SUCCESS);
}

/*------------------------------------------------------------------------------------*\
   BOOL Registry::QuerySubKeys(DWORD dwSubKeyIndex, LPTSTR szSubKeyName, 
                               LPDWORD dwSubKeyLen, LPTSTR szClass, 
                               LPDWORD dwClassLen , PFILETIME pLastWrite)

      DWORD dwSubKeyIndex      - Número da sub chave a ser pesquisada
      LPTSTR szSubKeyName      - Área que conterá o nome da sub chave
      LPDWORD dwSubKeyLen      - Tamanho da área
      LPTSTR szClass           - Área que conterá o nome da classe que a sub chave pertence
      LPDWORD dwClassLen       - Tamanho da área
      PFILETIME pLastWrite     - Área que conterá a hora da última alteração da sub chave

   Retorno....: Boolean indicando o sucesso da operacao
   Descricao..: Obtem informacoes sobre subchaves do registry
   
\*------------------------------------------------------------------------------------*/
BOOL Registry::QuerySubKeys(DWORD  dwSubKeyIndex,
							LPTSTR szSubKeyName , LPDWORD dwSubKeyLen,
							LPTSTR szClass      , LPDWORD dwClassLen , PFILETIME pLastWrite)
{
	TCHAR Class[CLASSLEN_MAX];      
	DWORD ClassLen;   
	FILETIME LastWrite;

	if (!isOpened)
		if (!Open())
			return FALSE;

	nError = RegEnumKeyEx(hKey,dwSubKeyIndex,szSubKeyName,dwSubKeyLen,NULL,Class,&ClassLen,&LastWrite);
	m_sLastError = CUtil::ErrorString(nError);

	if (szClass)	lstrcpy(szClass, Class);
	if (dwClassLen)	*dwClassLen = ClassLen;
	if (pLastWrite)	MoveMemory((PVOID)pLastWrite, (CONST VOID*)&LastWrite, (DWORD)sizeof(LastWrite));

	return (nError == ERROR_SUCCESS);
}

/*------------------------------------------------------------------------------------*\
   BOOL Registry::QueryEntry(DWORD   dwEntryIndex, LPTSTR  szEntry, 
                             LPDWORD dwEntryLen  , LPDWORD dwEntryType, 
                             LPBYTE  szValue     , LPDWORD dwValueLen)

      DWORD dwEntryIndex      - Número do ítem a ser pesquisado
      LPTSTR szEntry          - Área que conterá o nome do ítem
      LPDWORD dwEntryLen      - Tamanho da área
      LPDWORD dwEntryType     - Área que conterá o tipo da entrada
      LPBYTE szValue          - Área que conterá o valor do ítem
      LPDWORD dwValueLen      - Tamanho da área

   Retorno....: Boolean indicando o sucesso da operacao
   Descricao..: Obtem informacoes sobre itens da chaves do registry
   
\*------------------------------------------------------------------------------------*/
BOOL Registry::QueryEntry(DWORD   dwEntryIndex, LPTSTR  szEntry, 
                          LPDWORD dwEntryLen  , LPDWORD dwEntryType, 
                          LPBYTE  szValue     , LPDWORD dwValueLen)
{
	if (!isOpened)
		if (!Open())
			return FALSE;

	DWORD EntryType;
	BYTE  Value[1024];
	DWORD ValueLen = sizeof(Value);

	nError = RegEnumValue(hKey,dwEntryIndex,szEntry,dwEntryLen,NULL,&EntryType,Value,&ValueLen);
	m_sLastError = CUtil::ErrorString(nError);

	if (dwEntryType)	*dwEntryType = EntryType;
	if (dwValueLen)		*dwValueLen  = ValueLen;
	if (szValue)		lstrcpy((TCHAR*)szValue, (TCHAR*)Value);

	return (nError == ERROR_SUCCESS);
}

BOOL Registry::GetString(LPCTSTR szRegKey, CString &sOut)
{
	TCHAR szBuffer[4 * 1024] = {0};
	DWORD dwLen = sizeof(szBuffer);

	if(!GetValue(szRegKey, szBuffer, &dwLen))
		return FALSE;

	if(dwLen > 1)
	{
		CString sAux(szBuffer, dwLen);
		sOut = sAux;

		return TRUE;
	}

	return TRUE;
}

CString Registry::GetLastErrorString()
{
	return m_sLastError;
}

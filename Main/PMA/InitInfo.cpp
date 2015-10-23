// Copyright (c) 2007 Marcos Mori de Siqueira. All Rights Reserved. 
// mori@softfactory.com.br

#include "StdAfx.h"
#include "InitInfo.h"
#include "Utils.h"
#include "CStr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define TAG_SPLASH		_T("splash")
#define TAG_HOTKEY		_T("hotkey")
#define TAG_DATABASE	_T("database")
#define TAG_SERVER		_T("server_")
#define TAG_LOGO_LOGIN	_T("logo_login")
#define TAG_LOGO_MENU	_T("logo_menu") 
#define TAG_ORGAO		_T("orgao_autuador")
#define TAG_URL			_T("url")
#define TAG_FILE		_T("file")
#define TAG_VEICULOS	_T("veiculos")
#define TAG_ONLINE		_T("atz_online")
#define TAG_ENABLE_GPS	_T("enable_gps")
#define TAG_ENABLE_LOGIN _T("enable_login")
#define TAG_ENABLE_PHONE _T("enable_phone")
#define TAG_ENABLE_ATUALIZACAO	_T("enable_atualizacao")
#define TAG_ENABLE_BT	_T("enable_bluetooth")
#define TAG_ENABLE_CAMERA _T("enable_camera")
#define TAG_ENABLE_GERINCID _T("enable_gerincid")
#define TAG_VERSION		_T("version")
#define TAG_SIMBOLO		_T("simbolo")
#define TAG_PRINTER		_T("printer")
#define TAG_PROXY		_T("proxy")
#define TAG_BLOCKED		_T("blocked")
#define TAG_BT_MS_MGR	_T("msbtmgr")
#define TAG_BACKUP		_T("backup")
#define TAG_HTTPSENDER	_T("httpsenderthread")
#define TAG_SERIES		_T("series_alocadas")
#define TAG_ANTIGAS		_T("series_antigas")
#define TAG_LOGIN		_T("login")
#define TAG_ESTADUAL	_T("estadual")
#define TAG_TMOUT_SITUACAO _T("timeout_situacao")
#define TAG_TMOUT_MENSAGENS _T("timeout_mensagens")

/**
\brief Construtor da classe
\details
	Funções executadas neste módulo:
	- Início de variáveis globais da classe

\param void
*/
CInitInfo::CInitInfo(void)
	: m_bOnline(FALSE)
	, m_bBackupActive(FALSE)
	, m_bEnableGPS(FALSE)
	, m_bEnableAVL(FALSE)
	, m_bEnableBT(FALSE)
	, m_bEnableCamera(FALSE)
	, m_bEnableGerincid(FALSE)
	, m_bEnablePhone(FALSE)
	, m_sTmoutSituacao(L"")
	, m_sTmoutMensagens(L"")
{
}

/**
\brief Destrutor da classe
*/
CInitInfo::~CInitInfo(void)
{
}

/**
\brief Lê o arquivo XML de configuração, armazena em um buffer e inicia o parse
\param LPCTSTR szFile: Caminho do arquivo XML de configuração
\return TRUE se a leitura ocorrer com sucesso
*/
BOOL CInitInfo::LoadXML(LPCTSTR szFile)
{
	CFile f;
	CFileException fe;
	if(!f.Open(szFile, CFile::modeRead|CFile::modeNoTruncate, &fe))
	{
		STLOG_WRITE("Erro abrindo xml de configuracao. error code: %ld", fe.m_cause);
		return FALSE;
	}

	char* szBuffer = new char[(LONG)f.GetLength()+1];
	f.Read(szBuffer, (LONG)f.GetLength());	
	szBuffer[(LONG)f.GetLength()] = 0;

	f.Close();

	Create();

	BOOL bReturn = FALSE;

	if(Parse(szBuffer))
		bReturn = TRUE;
	else
	{
		STLOG_WRITE("CInitInfo::LoadXML - Erro efetuando parse do xml de configuracao [%S]",szBuffer);
	}

	delete [] szBuffer;

	return bReturn;
}

/**
\brief Inicia a configuração para iniciar a leitura das tags XML
\details
	É iniciado após a leitura do arquivo de configuração XML

\param void
\return void
*/
void CInitInfo::OnPostCreate()
{
	// Enable all the event routines we want
	EnableStartElementHandler ();
	EnableEndElementHandler ();
	// Note: EnableElementHandler will do both start and end
	EnableCharacterDataHandler ();
}

/**
\brief Método que é executado ao iniciar a leitura da tag XML
\details
	Funções executadas neste módulo:
	- Ler e configurar todas as variáveis que serão base para a utilização do aplicativo

\param const XML_Char *pszName: Nome da tag
\param const XML_Char **papszAttrs: Atributos adicionais contidos nesta tag
\return void
*/
void CInitInfo::OnStartElement(const XML_Char *pszName, const XML_Char **papszAttrs)
{
	m_sCurrentTag = pszName;

	if(m_sCurrentTag.CompareNoCase(TAG_SPLASH) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			for(int i = 0; i < 6; i++)
			{
				if(papszAttrs[i] == NULL)
					break;

				if(_stricmp(papszAttrs[i], "path") == 0)
				{
					m_sPath.Format(L"%S", papszAttrs[++i]);

					// Se tiver um $path no texto, vamos trocar pelo app path
					// ex: \\$path\\teste\\texte.dll
					CUtil::GetPathFromVariable(m_sPath);
				}
				else if(_stricmp(papszAttrs[i], "width") == 0)
				{
					m_nWidth = atol(papszAttrs[++i]);
				}
				else if(_stricmp(papszAttrs[i], "height") == 0)
				{
					m_nHeight = atol(papszAttrs[++i]);
				}
			}
		}
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_HOTKEY) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			if(_stricmp(papszAttrs[0], "path") == 0)
			{
				CString s;
				s.Format(L"%S", papszAttrs[1]);

				// Se tiver um $path no texto, vamos trocar pelo app path
				// ex: \\$path\\teste\\texte.dll
				CUtil::GetPathFromVariable(s);

				m_hotkeyPaths.AddTail(s);
			}
		}
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_DATABASE) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			if(_stricmp(papszAttrs[0], "path") == 0)
			{
				m_sDBPath.Format(L"%S", papszAttrs[1]);

				// Se tiver um $path no texto, vamos trocar pelo app path
				// ex: \\$path\\teste\\texte.dll
				CUtil::GetPathFromVariable(m_sDBPath);
			}
		}
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_BLOCKED) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			if(_stricmp(papszAttrs[0], "path") == 0)
			{
				m_sBlockedPath.Format(L"%S", papszAttrs[1]);

				// Se tiver um $path no texto, vamos trocar pelo app path
				// ex: \\$path\\teste\\texte.dll
				CUtil::GetPathFromVariable(m_sBlockedPath);
			}
		}
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_PROXY) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			if(_stricmp(papszAttrs[0], "path") == 0)
			{
				m_sProxyPath.Format(L"%S", papszAttrs[1]);

				// Se tiver um $path no texto, vamos trocar pelo app path
				// ex: \\$path\\teste\\texte.dll
				CUtil::GetPathFromVariable(m_sProxyPath);
			}
		}
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_LOGO_LOGIN) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			if(_stricmp(papszAttrs[0], "path") == 0)
			{
				m_sLogoLogin.Format(L"%S", papszAttrs[1]);

				// Se tiver um $path no texto, vamos trocar pelo app path
				// ex: \\$path\\teste\\texte.dll
				CUtil::GetPathFromVariable(m_sLogoLogin);
			}
		}
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_LOGO_MENU) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			if(_stricmp(papszAttrs[0], "path") == 0)
			{
				m_sLogoMenu.Format(L"%S", papszAttrs[1]);

				// Se tiver um $path no texto, vamos trocar pelo app path
				// ex: \\$path\\teste\\texte.dll
				CUtil::GetPathFromVariable(m_sLogoMenu);
			}
		}
	}
	else if(m_sCurrentTag.Find(TAG_SERVER) > -1)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			for(int i = 0; i < 4; i++)
			{
				if(papszAttrs[i] == NULL)
					break;

				if(_stricmp(papszAttrs[i], "url") == 0)
				{
					m_sAuxName = m_sCurrentTag.MakeLower();
					m_sAuxTarget.Format(L"%S", papszAttrs[++i]);
				}
				else if(_stricmp(papszAttrs[i], "contrato") == 0)
				{
					m_sContrato.Format(L"%S", papszAttrs[++i]);
				}
			}
		}
	}	
	else if(m_sCurrentTag.CompareNoCase(TAG_ORGAO) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			if(_stricmp(papszAttrs[0], "code") == 0)
				m_sOrgaoAutuador.Format(L"%S", papszAttrs[1]);
		}
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_PRINTER) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			if(_stricmp(papszAttrs[0], "name") == 0)
				m_sPrinterName.Format(L"%S", papszAttrs[1]);
		}
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_VEICULOS) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			if(_stricmp(papszAttrs[0], "path") == 0)
				m_sVeicPath.Format(L"%S", papszAttrs[1]);
		}
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_VERSION) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			if(_stricmp(papszAttrs[0], "value") == 0)
				m_sVersion.Format(L"%S", papszAttrs[1]);
		}
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_ESTADUAL) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			if(_stricmp(papszAttrs[0], "value") == 0)
			{
				if(strcmp(papszAttrs[1], "true") == 0 ||
				   strcmp(papszAttrs[1], "TRUE") == 0  )
				{
					m_bEstadual = TRUE;
				}
			}
		}
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_ONLINE) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			if(_stricmp(papszAttrs[0], "value") == 0)
			{
				if(strcmp(papszAttrs[1], "true") == 0 ||
				   strcmp(papszAttrs[1], "TRUE") == 0  )
				{
					m_bOnline = TRUE;
				}
			}
		}
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_ENABLE_GPS) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			for(int i = 0; i < 10; i++)
			{
				STLOG_WRITE("%s(%d): %s", __FUNCTION__, __LINE__, papszAttrs[i]);
				
				if(_stricmp(papszAttrs[i], "value") == 0)
				{
					++i;
					if(strcmp(papszAttrs[i], "true") == 0 ||
					   strcmp(papszAttrs[i], "TRUE") == 0  )
					{
						m_bEnableGPS = TRUE;
					}
				}
				else if(_stricmp(papszAttrs[i], "avl") == 0)
				{
					++i;
					if(strcmp(papszAttrs[i], "true") == 0 ||
					   strcmp(papszAttrs[i], "TRUE") == 0  )
					{
						m_bEnableAVL = TRUE;
					}
				}				
				else if(_stricmp(papszAttrs[i], "points_distance") == 0)
				{					
					m_iBetweenPointsDistance = atol(papszAttrs[++i]);
				}
				else if(_stricmp(papszAttrs[i], "expiration_time_static") == 0)
				{
					m_iExpirationTimeWhenStatic = atol(papszAttrs[++i]);					
				}
				else if(_stricmp(papszAttrs[i], "expiration_time_moving") == 0)
				{
					m_iExpirationTimeWhenMoving = atol(papszAttrs[++i]);					
				}
			}
		}
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_ENABLE_LOGIN) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			if(_stricmp(papszAttrs[0], "value") == 0)
			{
				if(strcmp(papszAttrs[1], "true") == 0 ||
				   strcmp(papszAttrs[1], "TRUE") == 0  )
				{
					m_bEnableLogin = TRUE;
				}
			}
		}
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_ENABLE_PHONE) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			if(_stricmp(papszAttrs[0], "value") == 0)
			{
				if(strcmp(papszAttrs[1], "true") == 0 ||
				   strcmp(papszAttrs[1], "TRUE") == 0  )
				{
					m_bEnablePhone = TRUE;
				}
			}
		}
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_ENABLE_ATUALIZACAO) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			if(_stricmp(papszAttrs[0], "value") == 0)
			{
				if(strcmp(papszAttrs[1], "true") == 0 ||
				   strcmp(papszAttrs[1], "TRUE") == 0  )
				{
					m_bEnableAtualizacao = TRUE;
				}
			}
		}
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_ENABLE_BT) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			if(_stricmp(papszAttrs[0], "value") == 0)
			{
				if(strcmp(papszAttrs[1], "true") == 0 ||
				   strcmp(papszAttrs[1], "TRUE") == 0  )
				{
					m_bEnableBT = TRUE;
				}
			}
		}
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_ENABLE_CAMERA) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			for(int i = 0; i < 4; i++)
			{
				if(papszAttrs[i] == NULL)
					break;

				if(_stricmp(papszAttrs[i], "driver") == 0)
				{
					m_sCameraDriver.Format(L"%S", papszAttrs[++i]);					
				}
				else if(_stricmp(papszAttrs[i], "value") == 0)
				{
					i++;
					if(strcmp(papszAttrs[i], "true") == 0 ||
					   strcmp(papszAttrs[i], "TRUE") == 0  )
					{
						m_bEnableCamera = TRUE;
					}
				}
			}
		}
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_TMOUT_SITUACAO) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			if(_stricmp(papszAttrs[0], "value") == 0)
			{
				m_sTmoutSituacao = CString(papszAttrs[1]);
			}
		}
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_TMOUT_MENSAGENS) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			if(_stricmp(papszAttrs[0], "value") == 0)
			{
				m_sTmoutMensagens = CString(papszAttrs[1]);
			}
		}
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_ENABLE_GERINCID) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			if(_stricmp(papszAttrs[0], "value") == 0)
			{
				if(strcmp(papszAttrs[1], "true") == 0 ||
				   strcmp(papszAttrs[1], "TRUE") == 0  )
				{
					m_bEnableGerincid = TRUE;
				}
			}
		}
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_SERIES) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			if(_stricmp(papszAttrs[0], "value") == 0)
			{
				m_sQtSeries = CString(papszAttrs[1]);
			}
		}
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_ANTIGAS) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			if(_stricmp(papszAttrs[0], "value") == 0)
			{
				m_sDiasAntigas = CString(papszAttrs[1]);
			}
		}
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_URL) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			for(int i = 0; i < 4; i++)
			{
				if(papszAttrs[i] == NULL)
					break;

				if(_stricmp(papszAttrs[i], "name") == 0)
				{
					m_sAuxName.Format(L"%S", papszAttrs[++i]);
				}
				else if(_stricmp(papszAttrs[i], "target") == 0)
				{
					m_sAuxTarget.Format(L"%S", papszAttrs[++i]);
				}
			}
		}
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_FILE) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			for(int i = 0; i < 4; i++)
			{
				if(papszAttrs[i] == NULL)
					break;

				if(_stricmp(papszAttrs[i], "name") == 0)
				{
					m_sAuxName.Format(L"%S", papszAttrs[++i]);
				}
				else if(_stricmp(papszAttrs[i], "path") == 0)
				{
					m_sAuxTarget.Format(L"%S", papszAttrs[++i]);
				}
			}
		}
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_SIMBOLO) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			for(int i = 0; i < 4; i++)
			{
				if(papszAttrs[i] == NULL)
					break;

				if(_stricmp(papszAttrs[i], "path") == 0)
				{
					m_sSymbolPath.Format(L"%S", papszAttrs[++i]);

					// Se tiver um $path no texto, vamos trocar pelo app path
					// ex: \\$path\\teste\\texte.dll
					CUtil::GetPathFromVariable(m_sSymbolPath);
				}
				else if(_stricmp(papszAttrs[i], "prefix") == 0)
				{
					m_sSymbolPrefix.Format(L"%S", papszAttrs[++i]);
				}
			}
		}
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_BT_MS_MGR) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			if(_stricmp(papszAttrs[0], "path") == 0)
				m_sMSBtMgrPath.Format(L"%S", papszAttrs[1]);
		}
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_BACKUP) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			for(int i = 0; i < 4; i++)
			{
				if(papszAttrs[i] == NULL)
					break;

				if(_stricmp(papszAttrs[i], "path") == 0)
				{
					m_sBackupPath.Format(L"%S", papszAttrs[++i]);

					// Se tiver um $path no texto, vamos trocar pelo app path
					// ex: \\$path\\teste\\texte.dll
					CUtil::GetPathFromVariable(m_sBackupPath);
				}
				else if(_stricmp(papszAttrs[i], "ativo") == 0)
				{
					i++;
					if(strcmp(papszAttrs[i], "true") == 0 ||
					   strcmp(papszAttrs[i], "TRUE") == 0  )
					{
						m_bBackupActive = TRUE;
					}
				}
			}
		}
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_LOGIN) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			for(int i = 0; i < 6; i++)
			{
				if(papszAttrs[i] == NULL)
					break;

				if(_stricmp(papszAttrs[i], "path") == 0)
				{
		
				}
				else if(_stricmp(papszAttrs[i], "idletimeout") == 0)
				{
					i++;
					if(strcmp(papszAttrs[i], "true") == 0 ||
					   strcmp(papszAttrs[i], "TRUE") == 0  )
					{
						m_bEnableIdleTimeout = TRUE;
					}
				}
				else if(_stricmp(papszAttrs[i], "time") == 0)
				{
					m_nIdleTimeout = atol(papszAttrs[++i]);
				}
			}
		}
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_HTTPSENDER) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			__HttpSenderThreadItem *httpsenderthreadItem = new __HttpSenderThreadItem(); 
			CString sAux;

			for(int i = 0; i < 8; i++)
			{
				if(papszAttrs[i] == NULL)
					break;				                        

				if(_stricmp(papszAttrs[i], "name") == 0)
				{
					m_sAuxHttpSenderThreadName.Format(L"%S", papszAttrs[++i]);
				}
				else if(_stricmp(papszAttrs[i], "descr") == 0)
				{
					sAux.Format(L"%S", papszAttrs[++i]);
					httpsenderthreadItem->sDescr = 	sAux;
				}				
				else if(_stricmp(papszAttrs[i], "url2send") == 0)
				{
					sAux.Format(L"%S", papszAttrs[++i]);
					httpsenderthreadItem->sURL2Send = sAux;
				}
				else if(_stricmp(papszAttrs[i], "time") == 0)
				{
					sAux.Format(L"%S", papszAttrs[++i]);
					CStr s = sAux;

					httpsenderthreadItem->iTime = atoi(s);
				}
				/*else if(_stricmp(papaszAttrs[i], "delete_file") == 0)
				{
					i++;
					if(strcmp(papszAttrs[i], "true") == 0 ||
					   strcmp(papszAttrs[i], "TRUE") == 0  )
					{
						httpsenderthreadItem->bDeleteAfterSend = TRUE;
					}
					else
					{
						httpsenderthreadItem->bDeleteAfterSend = FALSE;
					}
				}*/

				if(i == 7)
				{
					if(!m_sAuxHttpSenderThreadName.IsEmpty())
						m_HttpSenderThreadList.SetAt(m_sAuxHttpSenderThreadName, httpsenderthreadItem);
				}
			}
		}
	}
}

/**
\brief Método que é executado ao final da leitura da tag XML
\details
	Funções executadas neste módulo:
	- Alimentar todos os mapas de variáveis

\param const XML_Char *pszName: Nome da tag
\return void
*/
void CInitInfo::OnEndElement(const XML_Char *pszName)
{
	m_sCurrentTag = pszName;
	if(m_sCurrentTag.CompareNoCase(TAG_URL) == 0)
	{
		if(!m_sAuxName.IsEmpty())
			m_urls.SetAt(m_sAuxName, m_sAuxTarget);
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_FILE) == 0)
	{
		if(!m_sAuxName.IsEmpty())
			m_files.SetAt(m_sAuxName, m_sAuxTarget);
	}
	if(m_sCurrentTag.Find(TAG_SERVER) > -1)
	{
		if(!m_sAuxName.IsEmpty())
			m_servers.SetAt(m_sAuxName, m_sAuxTarget);
	}

	m_sCurrentTag.Empty();
}

/**
\brief Método que é executado quando for encontrado algum caracter fora das limitações <>
\details
	Sem uso

\param const XML_Char *pszData: Caracter lido
\return void
*/
void CInitInfo::OnCharacterData(const XML_Char *pszData, int nLength)
{
}

/**
\brief Procura uma determinada URL do XML de configuração
\param LPCTSTR szName: Nome da URL a ser buscada
\return Endereço da URL
*/
CString CInitInfo::GetURL(LPCTSTR szName)
{
	CString s;
	m_urls.Lookup(szName, s);
	return s;
}

/**
\brief Procura um determinado SERVER do XML de configuração
\param LPCTSTR szName: Nome do SERVER a ser buscada
\return Endereço do SERVER
*/
CString CInitInfo::GetServer(LPCTSTR szName)
{
	CString s;
	m_servers.Lookup(szName, s);
	return s;
}

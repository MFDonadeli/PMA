// Copyright (c) 2007 Marcos Mori de Siqueira. All Rights Reserved. 
// mori@softfactory.com.br
//Classe que encapsula os items de menu e lista de opcoes (modulos)
#include "stdafx.h"
#include "ModuleInfo.h"
#include "Utils.h"
#include "constants.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define TAG_LOGIN		_T("login")
#define TAG_STARTUP		_T("startup")
#define TAG_FOLDER		_T("folder")
#define TAG_MODULE		_T("module")
#define TAG_SEPARATOR	_T("separator")
#define TAG_ROOT		_T("root")
#define TAG_UPDATE		_T("update")

UINT CModuleInfo::ID_START = 17300;
UINT CModuleInfo::ID_END   = CModuleInfo::ID_START + 100; // MAX 100 ID´s

///Estrutura para troca de caracteres
struct _char_change
{
	wchar_t de;
	wchar_t para;
}  __cc[] = {{0xA1, 0xE1}, {0xA9, 0xE9}, {0xAD, 0xED},
			 {0xB3, 0xF3}, {0xBA, 0xFA}, {0xA3, 0xE3},
			 {0xA4, 0xE4}, {0xAB, 0xEB}, {0xAF, 0xEF},
			 {0xB6, 0xF6}, {0xBC, 0xFC}, {0xA2, 0xE2},
			 {0xAA, 0xEA}, {0xAE, 0xEE}, {0xB4, 0xF4},
			 {0xBB, 0xFB}, {0xA0, 0xE0}, {0xA8, 0xE8},
			 {0xAC, 0xEC}, {0xB2, 0xF2}, {0xB9, 0xF9},
			 {0xA7, 0xE7}, {0xE6, 0xF5}, {0xB5, 0xF5}, 
			 {0x93, 0xD3}, {0, 0}, };

/**
\brief Construtor da classe
\details
	Funções executadas neste módulo:
	- Início de variáveis globais da classe

\param void
*/
CModuleInfo::CModuleInfo()
{
	m_bUseBackItem = FALSE;
	m_pCurrentItem = &m_root;
}

/**
\brief Destrutor da classe
*/
CModuleInfo::~CModuleInfo()
{
}

/**
\brief Inicia a configuração para iniciar a leitura das tags XML
\details
	É iniciado após a leitura do arquivo de configuração XML

\param void
\return void
*/
void CModuleInfo::OnPostCreate()
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
	- Ler e configurar todas as informações do menu

\param const XML_Char *pszName: Nome da tag
\param const XML_Char **papszAttrs: Atributos adicionais contidos nesta tag
\return void
*/
void CModuleInfo::OnStartElement(const XML_Char *pszName, const XML_Char **papszAttrs)
{
	m_sCurrentTag = pszName;

	if(m_sCurrentTag.CompareNoCase(TAG_ROOT) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			if(_stricmp(papszAttrs[0], "title") == 0)
			{
				m_root.sTitle.Format(L"%S", papszAttrs[1]);
				_FixTexts(m_root.sTitle);
			}
		}
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_UPDATE) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			if(_stricmp(papszAttrs[0], "path") == 0)
			{
				m_update.nType = __MenuItem::TYPE_LOGIN;
				m_update.sModulePath.Format(L"%S", papszAttrs[1]);

				// Se tiver um $path no texto, vamos trocar pelo app path
				// ex: \\$path\\teste\\texte.dll
				CUtil::GetPathFromVariable(m_update.sModulePath);
			}
		}
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_LOGIN) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			if(_stricmp(papszAttrs[0], "path") == 0)
			{
				m_login.nType = __MenuItem::TYPE_LOGIN;
				m_login.sModulePath.Format(L"%S", papszAttrs[1]);

				// Se tiver um $path no texto, vamos trocar pelo app path
				// ex: \\$path\\teste\\texte.dll
				CUtil::GetPathFromVariable(m_login.sModulePath);
			}
		}
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_STARTUP) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			__MenuItem *pItem = new __MenuItem();
			m_startup.m_children.AddTail(pItem);
			pItem->nType = __MenuItem::TYPE_STARTUP;
			pItem->pParent = NULL;

			for(int i = 0; i < 4; i++)
			{
				if(papszAttrs[i] == NULL)
					break;

				if(_stricmp(papszAttrs[i], "path") == 0)
				{
					pItem->sModulePath.Format(L"%S", papszAttrs[++i]);

					// Se tiver um $path no texto, vamos trocar pelo app path
					// ex: \\$path\\teste\\texte.dll
					CUtil::GetPathFromVariable(pItem->sModulePath);
				}
				else if(_stricmp(papszAttrs[i], "sequence") == 0)
				{
					pItem->nSequence = atol(papszAttrs[++i]);
				}
				else if(_stricmp(papszAttrs[i], "param") == 0)
				{
					pItem->sParameter.Format(L"%S", papszAttrs[++i]);
					pItem->sParameter.Replace(L"&quot;", L"\"");
				}
			}
		}
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_FOLDER) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			__MenuItem *pItem = new __MenuItem();
			m_pCurrentItem->m_children.AddTail(pItem);
			pItem->nType   = __MenuItem::TYPE_POPUP;
			pItem->level   = m_pCurrentItem->level + 1;
			pItem->pParent = m_pCurrentItem;
			m_pCurrentItem = pItem;

			for(int i = 0; i < 6; i++)
			{
				if(papszAttrs[i] == NULL)
					break;

				if(_stricmp(papszAttrs[i], "text") == 0)
				{					pItem->sText.Format(L"%S", papszAttrs[++i]);
					_FixTexts(pItem->sText);
				}
				else if(_stricmp(papszAttrs[i], "title") == 0)
				{
					pItem->sTitle.Format(L"%S", papszAttrs[++i]);
					_FixTexts(pItem->sTitle);
				}
	
				else if(_stricmp(papszAttrs[i], "permissao") == 0)
				{
					pItem->sPermissao = papszAttrs[++i];
				}

				//else if(_stricmp(papszAttrs[i], "icon") == 0)
				//{
				//	pItem->sModulePath = papszAttrs[++i];
				//}
			}
		}
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_MODULE) == 0)
	{
		if(papszAttrs != NULL && papszAttrs[0] != NULL)
		{
			__MenuItem *pItem = new __MenuItem();
			m_pCurrentItem->m_children.AddTail(pItem);
			pItem->pParent = m_pCurrentItem;
			pItem->nType = __MenuItem::TYPE_ITEM;
			pItem->level = m_pCurrentItem->level + 1;

			for(int i = 0; i < 12; i++)
			{
				if(papszAttrs[i] == NULL)
					break;

				if(_stricmp(papszAttrs[i], "text") == 0)
				{
					pItem->sText.Format(L"%S", papszAttrs[++i]);
					_FixTexts(pItem->sText);
				}
				else if(_stricmp(papszAttrs[i], "param") == 0)
				{
					pItem->sParameter.Format(L"%S", papszAttrs[++i]);
					CUtil::GetPathFromVariable(pItem->sParameter);
				}
				else if(_stricmp(papszAttrs[i], "id") == 0)
				{
					i++;

					// Excessoes para o IDOK e o IDCANCEL...
					if(_stricmp(papszAttrs[i], "IDOK") == 0)
						pItem->nID = IDOK;
					else if(_stricmp(papszAttrs[i], "IDCANCEL") == 0)
						pItem->nID = IDCANCEL;
					else if(_stricmp(papszAttrs[i], "IDALIGNSCREEN") == 0)
						pItem->nID = IDALIGNSCREEN;
					else
						pItem->nID = atol(papszAttrs[i]);
				}
				else if(_stricmp(papszAttrs[i], "path") == 0)
				{
					pItem->sModulePath = papszAttrs[++i];

					// Se tiver um $path no texto, vamos trocar pelo app path
					// ex: \\$path\\teste\\texte.dll
					CUtil::GetPathFromVariable(pItem->sModulePath);
				}
				else if(_stricmp(papszAttrs[i], "icon") == 0)
				{
					pItem->nIconResID = atol(papszAttrs[++i]);
				}
				else if(_stricmp(papszAttrs[i], "permissao") == 0)
				{
					pItem->sPermissao = papszAttrs[++i];
				}
			}
		}
	}
	else if(m_sCurrentTag.CompareNoCase(TAG_SEPARATOR) == 0)
	{
		__MenuItem *pItem = new __MenuItem();
		m_pCurrentItem->m_children.AddTail(pItem);
		pItem->pParent = m_pCurrentItem;
		pItem->nType = __MenuItem::TYPE_SEPARATOR;
		pItem->level = m_pCurrentItem->level + 1;
		pItem->sText = _T("<SEPARATOR>");
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
void CModuleInfo::OnEndElement(const XML_Char *pszName)
{
	m_sCurrentTag = pszName;
	if(m_sCurrentTag.CompareNoCase(TAG_FOLDER) == 0)
	{
		if(m_bUseBackItem)
		{
			if(m_pCurrentItem != &m_root)
			{
				__MenuItem *pItem = new __MenuItem();
				m_pCurrentItem->m_children.AddTail(pItem);
				pItem->pParent = m_pCurrentItem->pParent;
				pItem->nType   = __MenuItem::TYPE_BACK;
			}
		}

		m_pCurrentItem = m_pCurrentItem->pParent;
	}

	m_sCurrentTag.Empty();
}

/**
\brief Método que é executado quando for encontrado algum caracter fora das limitações <>
\details
	Sem uso

\param const XML_Char *pszName: Nome da tag
\return void
*/
void CModuleInfo::OnCharacterData(const XML_Char *pszData, int nLength)
{
}

/**
\brief Lê o arquivo XML de configuração, armazena em um buffer e inicia o parse
\param LPCTSTR szFile: Caminho do arquivo XML de configuração
\return TRUE se a leitura ocorrer com sucesso
*/
BOOL CModuleInfo::LoadXML(LPCTSTR szFile, BOOL bUseBackItem)
{
	m_bUseBackItem = bUseBackItem;

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
		STLOG_WRITE("CModuleInfo::LoadXML - Erro efetuando parse do xml de configuracao. Erro [%s]", GetErrorString());
		AfxMessageBox(CString(szBuffer));
	}

	delete [] szBuffer;

	return bReturn;
}

/**
\brief Método público que inicia a construção do menu da tela principal
\param CMenu *pMenu: Ponteiro para o menu que será construído
\return void
*/
void CModuleInfo::BuildMenu(CMenu *pMenu)
{
	POSITION p = GetRootItem()->m_children.GetHeadPosition();
	while(p)
	{
		__MenuItem *pItem = GetRootItem()->m_children.GetNext(p);
		_BuildMenu(pMenu, pItem, 0);
	}
}

/**
\brief Varre todos os ítens de menu para descoberta de erros
\param void
\return void
*/
void CModuleInfo::Debug()
{
	MessageBox(NULL, GetLoginItem()->sModulePath, L"Debug", MB_OK);	

	POSITION p = GetStartupItem()->m_children.GetHeadPosition();
	while(p)
	{
		__MenuItem *pItem = GetRootItem()->m_children.GetNext(p);
		MessageBox(NULL, pItem->sModulePath + CString(L" ") + pItem->sText, L"Debug", MB_OK);
	}

	POSITION p2 = GetRootItem()->m_children.GetHeadPosition();
	while(p2)
	{
		__MenuItem *pItem = GetRootItem()->m_children.GetNext(p2);
		if(pItem->nType == __MenuItem::TYPE_ITEM)
			MessageBox(NULL, pItem->sModulePath + CString(L" ") + pItem->sText, L"Debug", MB_OK);
		else if(pItem->nType == __MenuItem::TYPE_POPUP)
		{
			POSITION p1 = pItem->m_children.GetHeadPosition();
			while(p1)
			{
				__MenuItem *pItem1 = pItem->m_children.GetNext(p1);
				if(pItem1->nType == __MenuItem::TYPE_ITEM)
					MessageBox(NULL, pItem1->sModulePath + CString(L" ") + pItem1->sText, L"Debug", MB_OK);
			}
		}
	}
}

/**
\brief Método protegido que constrói o menu da tela principal
\param CMenu *pMenu: Ponteiro para o menu que será construído
\param __MenuItem *pItem*: Itens a serem construídos
\param int level: Nível inicial de construção do menu (Depende do nível dos ítens de pItem).
\return void
*/
void CModuleInfo::_BuildMenu(CMenu *pMenu, __MenuItem *pItem, int level)
{
	CMenu *_pMenu = pMenu;

	// Adiciona um item normal...
	if(pItem->nType == __MenuItem::TYPE_ITEM)
	{
		BOOL bAdd = TRUE;
		if(pItem->sModulePath.Find(L".exe") > 0 || 
		   pItem->sModulePath.Find(L".EXE") > 0 )
		{
			// Se for executavel, soh adicionar se existir...
			if(!CUtil::FileExists(pItem->sModulePath))
				bAdd = FALSE;
		}
		
		if(bAdd)
			pMenu->AppendMenu(MF_STRING, pItem->nID, pItem->sText);
	}

	// Adiciona um separador...
	if(pItem->nType == __MenuItem::TYPE_SEPARATOR)
		pMenu->AppendMenu(MF_SEPARATOR);

	// Adiciona um submenu...
	if(pItem->nType == __MenuItem::TYPE_POPUP ||
	   pItem->nType == __MenuItem::TYPE_ROOT )
	{
		// Cria o menu popup...
		if(pItem->nType == __MenuItem::TYPE_POPUP)
		{
			VERIFY(pItem->menu.CreatePopupMenu());
			_pMenu = &pItem->menu; // Se o novo parent...
		}

		// Varrer os submenus...
		POSITION p = pItem->m_children.GetHeadPosition();
		while(p)
		{
			__MenuItem *pItem1 = pItem->m_children.GetNext(p);
			_BuildMenu(_pMenu, pItem1, level + 1);
		}

		// Adiciona o popup ao menu corrente...
		if(pItem->nType == __MenuItem::TYPE_POPUP)
		{
			ASSERT(pItem->menu.GetSafeHmenu() != NULL);
			pMenu->AppendMenu(MF_POPUP, 
							  (UINT)pItem->menu.GetSafeHmenu(), 
							  pItem->sText);
			_pMenu = pMenu; // Reseta o parent...
		}
	}
}

/**
\brief Inicia a procura o nome de um módulo pelo ID
\param __MenuItem *pItem: Lista de itens do menu
\param UINT nID: ID a ser procurado
\return Nome do módulo
*/
CString CModuleInfo::GetModuleByID(UINT nID)
{
	return _GetModuleByID(GetRootItem(), nID);
}

/**
\brief Inicia a procura informações de um módulo pelo ID
\param __MenuItem *pItem: Lista de itens do menu
\param UINT nID: ID a ser procurado
\return Ponteiro para a classe de informações do menu
*/
__MenuItem *CModuleInfo::GetInfoByMenuID(UINT nID)
{
	return _GetInfoByMenuID(GetRootItem(), nID);
}

/**
\brief Procura informações de um módulo pelo ID
\param __MenuItem *pItem: Lista de itens do menu
\param UINT nID: ID a ser procurado
\return Ponteiro para a classe de informações do menu
*/
__MenuItem *CModuleInfo::_GetInfoByMenuID(__MenuItem *pItem, UINT nID)
{
	if(pItem->nType == __MenuItem::TYPE_ITEM)
		if(pItem->nID == nID)
			return pItem;

	if(pItem->nType == __MenuItem::TYPE_ROOT ||
	   pItem->nType == __MenuItem::TYPE_POPUP )
	{
		POSITION p = pItem->m_children.GetHeadPosition();
		while(p)
		{
			__MenuItem *pItem1 = pItem->m_children.GetNext(p);
			__MenuItem *pItem2 = _GetInfoByMenuID(pItem1, nID);
			if(pItem2 != NULL)
				return pItem2;
		}
	}

	return NULL;
}

/**
\brief Procura o nome de um módulo pelo ID
\param __MenuItem *pItem: Lista de itens do menu
\param UINT nID: ID a ser procurado
\return Nome do módulo
*/
CString CModuleInfo::_GetModuleByID(__MenuItem *pItem, UINT nID)
{
	if(pItem->nType == __MenuItem::TYPE_ITEM)
		if(pItem->nID == nID)
			return pItem->sModulePath;

	if(pItem->nType == __MenuItem::TYPE_ROOT ||
	   pItem->nType == __MenuItem::TYPE_POPUP )
	{
		POSITION p = pItem->m_children.GetHeadPosition();
		while(p)
		{
			__MenuItem *pItem1 = pItem->m_children.GetNext(p);
			CString s =_GetModuleByID(pItem1, nID);
			if(!s.IsEmpty())
				return s;
		}
	}

	return _T("");
}

/**
\brief Substitui caracteres inválidos
\param CString &s: String a ser pesquisada
\return void
*/
void CModuleInfo::_FixTexts(CString &s)
{
//STLOG_WRITE(L"TEXT %s", s);

	s.Remove(0xC3);

	// Fazer a correcao de caracteres invalidos...
	for(int i = 0; i < s.GetLength(); i++)
	{	
		for(int j = 0; j < sizeof(__cc)/sizeof(_char_change); j++)
		{
			wchar_t b1 = s.GetAt(i);
			wchar_t b2 = __cc[j].de;

			if(s.GetAt(i) == __cc[j].de)
			{
				s.SetAt(i, __cc[j].para);
				break;
			}
		}
	}

//	STLOG_WRITE(L"TEXT %s", s);
}

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/**
\brief Destrutor da classe
\details
	Funções executadas neste módulo:
	- Remoção e destruição de todos os item do menu e da lista do menu
*/
__MenuItem::~__MenuItem()
{
	POSITION p = m_children.GetHeadPosition();
	while(p)
	{
		__MenuItem *pItem = m_children.GetNext(p);
		delete pItem;
	}

	m_children.RemoveAll();

	// Se for TYPE_POPUP...
	if(menu.GetSafeHmenu() != NULL)
		menu.DestroyMenu();
}

/**
\brief Construtor da classe
\details
	Funções executadas neste módulo:
	- Início de variáveis globais da classe

\param void
*/
__MenuItem::__MenuItem() 
{ 
	pParent    = NULL; 
	nID		   = 0;
	nType	   = TYPE_ROOT;
	level	   = -1;
	nSequence  = 0;
	nIconResID = -1;
	nIconIdx   = -1;
}

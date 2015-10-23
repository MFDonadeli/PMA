#include "StdAfx.h"
#include "XmlCreate.h"
#include "Utils.h"
#include "CStr.h"
#include "FormData.h"
#include "Utils2.h"

/**
\brief Construtor da classe
\param void
\return void
*/
CXmlCreate::CXmlCreate(void)
{
	m_sXmlEncoding = L"ISO-8859-1";		
	m_bWithHeader = TRUE;
}


/**
\brief Destrutor da classe
\param void
\return void
*/
CXmlCreate::~CXmlCreate(void)
{
	DeleteAllItems();
}


/**
\brief Configura encoding do arquivo XML
\param LPCTSTR sXmlEncoding Padrão: ISO-8859-1
\return void
*/
void CXmlCreate::SetXMLEncoding(LPCTSTR sXmlEncoding)
{
	m_sXmlEncoding = sXmlEncoding;
}


/**
\brief Configura flag do responsável pelo header
\param BOOL bWithHeader Padrão: TRUE (esta classe é a responsável pelo header)
\return void
*/
void CXmlCreate::SetWithHeader(BOOL bWithHeader)
{
	m_bWithHeader = bWithHeader;
}


/**
\brief Carrega um XML já montado
\param LPCTSTR sXmlStruct XML a ser carregado
\return void
*/
void CXmlCreate::SetXMLStruct(LPCTSTR sXmlStruct)
{
	m_sXmlDocument = sXmlStruct;
}


/**
\brief Abre nós principais (sub-seções)
\param LPCTSTR sOpenRootTagName Nome da Tag
\return void
*/
void CXmlCreate::OpenRootTag(LPCTSTR sOpenRootTagName)
{
	CString temp;
	temp.Format(L"<%s>",sOpenRootTagName);
	m_arrXmlElements.Add(temp);	
}

/**
\brief Fecha nós principais (sub-seções)
\param LPCTSTR sCloseRootTagName Nome da Tag
\return void
*/
void CXmlCreate::CloseRootTag(LPCTSTR sCloseRootTagName)
{
	CString temp;	
	temp.Format(L"</%s>",sCloseRootTagName);
	m_arrXmlElements.Add(temp);		
}

/**
\brief Cria elemento do tipo <tag>valor</tag>
\param LPCTSTR sTagName Nome da Tag
\param LPCTSTR sValue Valor do elemento
\return void
*/
void CXmlCreate::AddElement(LPCTSTR sTagName, LPCTSTR sValue)
{
	CString temp;
	CString sCData;
	
	if(CString(sValue).Find(L"&") > -1 || CString(sValue).Find(L">") > -1 || CString(sValue).Find(L"<") > -1 || CString(sValue).Find(L"'") > -1)
	{
		sCData.Format(L"<![CDATA[%s]]>", sValue);
	}
	else
	{
		sCData = CString(sValue);
	}

	temp.Format(L"<%s>%s</%s>",sTagName, sCData, sTagName);

	m_arrXmlElements.Add(temp);
}

/**
\brief Cria elemento do tipo <tag>valor</tag>
\param LPCTSTR sTagName Nome da Tag
\param double sValue Valor do elemento
\return void
*/
void CXmlCreate::AddElement(LPCTSTR sTagName, double sValue)
{	
	CString temp;
	temp.Format(L"<%s>%f</%s>",sTagName, sValue, sTagName);

	m_arrXmlElements.Add(temp);
}

/**
\brief Cria elemento do tipo <tag>valor</tag>
\param LPCTSTR sTagName Nome da Tag
\param LPCTSTR int Valor do elemento
\return void
*/
void CXmlCreate::AddElement(LPCTSTR sTagName, int sValue)
{	
	CString temp;
	temp.Format(L"<%s>%d</%s>",sTagName, sValue, sTagName);

	m_arrXmlElements.Add(temp);
}

/**
\brief Monta arquivo XML, lendo o array m_arrXmlElements
\param void
\return CString m_sXmlDocument Arquivo XML
*/
CString CXmlCreate::GetXml()
{
	//m_sXmlDocument = L"";
	if(m_bWithHeader)
		m_sXmlDocument.Format(L"<?xml version=\"1.0\" encoding=\"%s\"?>", m_sXmlEncoding);

	for(int i=0; i<m_arrXmlElements.GetSize(); i++)
		m_sXmlDocument.Append(m_arrXmlElements[i]);		
	
	return m_sXmlDocument;
}

/**
\brief Cria arquivo XML físico
\param CString sPathFile Path do arquivo XML a ser criado
\param BOOL bCripto=TRUE se TRUE criptografa arquivo
\return BOOL
*/
BOOL CXmlCreate::CreateXmlFile(CString sPathFile, BOOL bCripto)
{
	CFile xmlFile;
	CFileException e;

	CString sXmlDocument = GetXml();

	char * bufXml = (char *)malloc(sXmlDocument.GetLength() + 8);

	if (bCripto)
	{
		#define CRIPTOGRAFA_XML
		int len = CriptografaBufferXml(bufXml);

		if((xmlFile.Open(sPathFile, CFile::modeCreate | CFile::modeWrite, &e)) && (len > 0))
		{

			xmlFile.Write( bufXml, len); //ANSI

			xmlFile.Close();		
			free(bufXml);
			return TRUE;
		}
		else
		{
			STLOG_WRITE(L"%S(%d): Falha ao criar xml: [%d]", __FUNCTION__, __LINE__, e.m_cause);
		}

	}
	else
	{
		if (xmlFile.Open(sPathFile, CFile::modeCreate | CFile::modeWrite, &e))
		{
			xmlFile.Write(CStringA (sXmlDocument), CStringA (sXmlDocument).GetLength()); //ANSI
			xmlFile.Close();		
			free(bufXml);
			return TRUE;

		}
		else
		{
			STLOG_WRITE(L"%S(%d): Falha ao criar xml: [%d]", __FUNCTION__, __LINE__, e.m_cause);
		}
	}
	free(bufXml);
	return FALSE;
}


/////**
////\brief Cria arquivo XML físico
////\param CString sPathFile Path do arquivo XML a ser criado
////\return BOOL
////*/
////BOOL CXmlCreate::CreateXmlFile(CString sPathFile)
////{
////	CFile xmlFile;
////	CFileException e;
////
////	CString sXmlDocument = GetXml();
////
////	char * bufXml = (char *)malloc(sXmlDocument.GetLength() + 8);
////
////#define CRIPTOGRAFA_XML
////#ifdef CRIPTOGRAFA_XML
////	int len = CriptografaBufferXml(bufXml);
////
////	if((xmlFile.Open(sPathFile, CFile::modeCreate | CFile::modeWrite, &e)) && (len > 0))
////#else
////	if (xmlFile.Open(sPathFile, CFile::modeCreate | CFile::modeWrite, &e))
////#endif
////	{
////		//xmlFile.Write(sXmlDocument, sXmlDocument.GetLength()*2); //unicode
////#ifndef CRIPTOGRAFA_XML
////		xmlFile.Write(CStringA (sXmlDocument), CStringA (sXmlDocument).GetLength()); //ANSI
////#else
////		xmlFile.Write( bufXml, len); //ANSI
////#endif
////		xmlFile.Close();		
////		free(bufXml);
////		return TRUE;
////	}
////	else
////	{
////		STLOG_WRITE(L"%S(%d): Falha ao criar xml: [%d]", __FUNCTION__, __LINE__, e.m_cause);
////	}
////	free(bufXml);
////	return FALSE;
////}

/**
\brief Criptografa dados a serem gravados no arquivo XML físico
\param -
\return BOOL
*/
int CXmlCreate::CriptografaBufferXml(char * cBuffXml)
{
	
	char password[100]= "";

	CString sXmlInp = GetXml();

	int ret = CUtils2::CriptoBuffer(CStr(sXmlInp), cBuffXml, &password[0], sXmlInp.GetLength());
	if (!ret > 0)
	{
		STLOG_WRITE(L"%S(%d): Falha ao criptografar xml", __FUNCTION__, __LINE__ );
		return -1;
	}

	return ret;
}

/**
\brief Valida estrutura do arquivo XML
\details Utiliza método Parse() do expatimpl
\return BOOL TRUE: Estrutura válida / FALSE: Estrutura inválida
*/
BOOL CXmlCreate::ValidateXml()
{
	Create();
	return Parse(CStringA (GetXml()));
}

/**
\brief Envia arquivo XML (como anexo) via post multipart
\details Utiliza classe CHttp
\param CString sPathFile Arquivo a ser enviado
\param CString sUrl URL para onde o XML será enviado
\param CString sContrato Contrato do sistema
\param CString sXmlMD5 MD5 do arquivo XML
\return BOOL TRUE: Envio OK / FALSE: Falha no envio
*/
//BOOL CXmlCreate::SendXml(CString sPathFile, CString sUrl, CString sContrato, CString sXmlMD5)
//{
//	CMapStringToString strMapVarValue;
//
//	strMapVarValue.SetAt(L"contrato", sContrato);
//	strMapVarValue.SetAt(L"MD5Xml", sXmlMD5);
//
//	CString sResp;
//	int iRespCod;
//
//	return CUtil::HttpSendFile(L"XML", sPathFile, sUrl, &strMapVarValue, iRespCod, &sResp);
//}


void CXmlCreate::DeleteAllItems()
{
	m_arrXmlElements.RemoveAll();	
	m_sXmlDocument = L"";
}

#include "StdAfx.h"
#include "TxtSearchWrapper.h"
#include "TxtSearch.h"
#include "CStr.h"
#include "Cripto.h"

/**
\brief Construtor da classe
\param void
\return void
*/
CTxtSearchWrapper::CTxtSearchWrapper(void)
{
}


/**
\brief Destrutor da classe
\param void
\return void
*/
CTxtSearchWrapper::~CTxtSearchWrapper(void)
{
}

void CTxtSearchWrapper::Setup(int* iVeic, int nCols, int nMCol)
{
	m_iVeic = iVeic;
	m_nCols = nCols;
	m_nMCol = nMCol;
}

/**
\brief Realiza conculta de placa de veículo no arquivo veiculos.egb
\details O retorno da pesquisa é uma string (&sBuffer) formada dos dados separados por pipe (|)
\param LPCTSTR szFile Path do arquivo
\param LPCTSTR szPlaca Placa a ser pesquisada
\param CString &sBuffer Retorno das informações encontradas no arquivo ref. a placa pesquisada
\param BOOL *bFound Flag que indica se placa do arquivo foi encontrada no arquivo szFile
\return BOOL
*/
BOOL CTxtSearchWrapper::Find(LPCTSTR szFile, LPCTSTR szPlaca, CString &sBuffer, BOOL *bFound)
{
	CStr sFile(szFile);
	CStr sPlaca(szPlaca);

	CTxtSearch _v(sFile);
	_v.setupColumns(m_iVeic, m_nCols, m_nMCol);

	if(!_v.open())
	{
		STLOG_WRITE("Erro abrindo arquivo veiculos.egb");
		return FALSE;
	}


	*bFound = FALSE;

	if(_v.find(sPlaca))
	{
		*bFound = TRUE;
		sBuffer = CString(_v.registro());
		sBuffer.SetAt(_v.size_registro(), '\0');
		STLOG_WRITE(L"BUFFER PESQUISADO NO ARQUIVO TEXTO: [%s]", sBuffer);
	}

	_v.close();

	return TRUE;
}


/**
\brief Realiza conculta de placa de veículo no arquivo veiculos.egb
\details O retorno da pesquisa é armazenado nos ponteiros dos campos recebidos pelo método
\param LPCTSTR szFile Path do arquivo
\param LPCTSTR szPlaca Placa a ser pesquisada
\param long *modelo Armazena modelo do veículo resultante da pesquisa
\param int *categoria Armazena categoria do veículo resultante da pesquisa
\param int *tipo Armazena tipo do veículo resultante da pesquisa
\param int *cor Armazena cor do veículo resultante da pesquisa
\param int *especie Armazena espécie do veículo resultante da pesquisa
\param int *municipio Armazena município do veículo resultante da pesquisa
\param BOOL *bFound Flag que indica se placa do arquivo foi encontrada no arquivo szFile
\return BOOL
*/
BOOL CTxtSearchWrapper::Find(LPCTSTR szFile, LPCTSTR szPlaca, CStringArray &sBuffer, BOOL *bFound)
{
	CStr sFile(szFile);
	CStr sPlaca(szPlaca);

	CTxtSearch _v(sFile);
	_v.setupColumns(m_iVeic, m_nCols, m_nMCol);


	if(!_v.open())
	{
		STLOG_WRITE("Erro abrindo arquivo veiculos.egb");
		return FALSE;
	}

	*bFound = FALSE;

	if(_v.find(sPlaca))
		*bFound = TRUE;

	if(bFound)
	{
		CString s;
		while(1)
		{
			s = _v.getColumn();
			if(s.GetAt(0) == 0) break;
			sBuffer.Add(s);
		}
	}

	_v.close();

	return TRUE;
}

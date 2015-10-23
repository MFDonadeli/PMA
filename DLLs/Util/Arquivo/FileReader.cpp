// Copyright (c) 2007 Marcos Mori de Siqueira. All Rights Reserved. 
// mori@softfactory.com.br
// Classe abstrata que que le um arquivo e a cada linha terminada 
// por cr/lf/crlf processa a mesma, para fazer importacao de dados
// Para melhorar a performance, a cada FINAL de linha encontrado, eh
// criado um item com a posicao inicial e o tamanho da linha, este
// item eh inserido em uma lista, estas lista conterah um numero
// maximo de itens. Com isso a verredura sera feita por esta lista
// a criacao de varias listas, agiliza a limpeza final.
#include "StdAfx.h"
#include "FileReader.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/**
\brief Construtor da classe
\param void
\return void
*/
CFileReader::CFileReader(void)
{
}

/**
\brief Destrutor da classe
\param void
\return void
*/
CFileReader::~CFileReader(void)
{
	Destroy();
}


/**
\brief Abre e lê arquivo linha a linha alimenzando a lista de dados m_lists
\param LPCTSTR szFile Arquivo a ser carregado
\param __fileType type Tipo do arquivo em questão
\return BOOL
*/
BOOL CFileReader::Load(LPCTSTR szFile, __fileType type)
{
#if _DEBUG
	DWORD dwStart = GetTickCount();
#endif

	if(!m_file.Open(szFile, CFile::modeRead))
	{
		STLOG_WRITE(L"Cannot open file");
		STLOG_LASTERROR;
		return FALSE;
	}

	m_file.SeekToBegin();

	LONG pos_atual = 0;
	char buffer[1024 * 4];
	LONG lastPos = 0;

	__FILIST *_list = new __FILIST;
	m_lists.AddTail(_list);

	LONG readed = 0;
	long counter = 0;
	do
	{
		memset(&buffer, 0, sizeof(buffer));
		readed = m_file.Read(buffer, sizeof(buffer));

		if(readed > 0)
		{
			char *pch = NULL;
			do
			{
				if(pch != NULL)
					pch = strstr(*buffer + pch, "\r\n");
				else
					pch = strstr(buffer, "\r\n");
					LONG pos = (LONG)(pch - buffer);

				if(pos > 0)
				{
					if(counter++ > 1000)
					{
						_list = new __FILIST;
						m_lists.AddTail(_list);
						counter = 0;
					}

					_fi_ *_fi = new _fi_;
					if(lastPos > 0)
						_fi->begin = (type == TYPE_CRLF) ? lastPos + 2 : lastPos + 1;
					else
						_fi->begin = lastPos;

					_fi->size  = (pos + pos_atual) - lastPos;
					if(lastPos > 0)
						_fi->size -= (type == TYPE_CRLF) ? 2 : 1;

					_list->AddTail(_fi);

					lastPos = pos + pos_atual;
				}

			} while(pch != NULL);

			pos_atual += readed;
		}

	} while(readed > 0);

#ifdef _DEBUG
	DWORD dwEnd = GetTickCount() - dwStart;
	//TRACE(L"T1: %ld\n", dwEnd/1000);
#endif

	return TRUE;
}


/**
\brief Calcula o tamanho da lista de dados lidos do arquivo
\param void
\return LONG Tamanho da lista 
*/
LONG CFileReader::GetCount() const
{
	LONG counter = 0;
	POSITION p = m_lists.GetHeadPosition();
	while(p)
	{
		__FILIST  *_list = m_lists.GetNext(p);
		counter += (LONG)_list->GetCount();
	}

	return counter;
}


/**
\brief Deleta listas de dados e encerra handle do arquivo lido
\param void
\return void
*/
void CFileReader::Destroy()
{
	if(m_file.m_hFile != INVALID_HANDLE_VALUE)
		m_file.Close();
	
	if(m_lists.GetCount() == 0)
		return;

#ifdef _DEBUG
	//TRACE(L"DELETANDO LISTAS...\n");
	DWORD dwStart = GetTickCount();
#endif

	POSITION p = m_lists.GetHeadPosition();
	while(p)
	{
		__FILIST  *_list = m_lists.GetNext(p);
		POSITION p1 = _list->GetHeadPosition();
		while(p1)
		{
			_fi_  *_fi = _list->GetNext(p1);
			delete _fi;
		}

		_list->RemoveAll();
		delete _list;
	}

	m_lists.RemoveAll();

#ifdef _DEBUG
	DWORD dwEnd = GetTickCount() - dwStart;
	//TRACE(L"T2: %ld\n", dwEnd/1000);
#endif
}


/**
\brief Processa lista de dados lidos do arquivo
\details Processa linha a linha através da chamada ao método ProcessFileLine
\param void
\return void
*/
BOOL CFileReader::Process()
{
#ifdef _DEBUG
	//TRACE(L"PROCESSING LIST...\n");
	DWORD dwStart = GetTickCount();
#endif

	POSITION p = m_lists.GetHeadPosition();
	while(p)
	{
//TRACE(L">>> NOVA LISTA\n");
		__FILIST  *_list = m_lists.GetNext(p);
		POSITION p1 = _list->GetHeadPosition();
		while(p1)
		{
			_fi_  *_fi = _list->GetNext(p1);
			if(_fi != NULL)
			{
				char *buffer = new char[_fi->size+1];
				memset(buffer, 0, _fi->size+1);
				if(buffer != NULL)
				{
					m_file.Seek(_fi->begin, CFile::begin);
					m_file.Read(buffer, _fi->size);
					BOOL bContinue = ProcessFileLine(buffer);
					delete [] buffer;

					if(!bContinue)
						return FALSE;
				}
				else
					return FALSE;
			}
		}
	}

#ifdef _DEBUG
	DWORD dwEnd = GetTickCount() - dwStart;
	//TRACE(L"T3: %ld\n", dwEnd/1000);
#endif

	return TRUE;
}

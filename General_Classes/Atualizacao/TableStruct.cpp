#include "stdafx.h"
#include "TableStruct.h"

/**
\brief Construtor da classe
\details
	Funções executadas neste método:
	- Ler o conteúdo do arquivo de estrutura;
	- Separar cada campo (quebra de informações da linha);
	- Inicializar um struct e incluí-lo em um vetor.

\param LPCTSTR szStructFile: Nome do arquivo de estrutura
*/
CTableStruct::CTableStruct(LPCTSTR szStructFile)
{
	string line;
	ifstream txtFields(szStructFile);	
	int pos=0;
	int pos_ini=0;

	if(txtFields.is_open())
	{
		char* token;
		char str[128];
		char sep[] = "|";
		LPCSTR tName="";
		while(! txtFields.eof() )
		{
			getline(txtFields, line);
			strcpy(str, line.c_str());
			token = strtok(str, sep);
			if(!token)
				break;
			strcpy(tTable.tnome, token);
			token = strtok(NULL, sep);
			if(!token)
			{
				STLOG_WRITE(L"Arquivo de estrutura está incorreto. Linha: %S", line.c_str());
				break;
			}
			strcpy(tTable.nome,token);
			token = strtok(NULL, sep);
			if(!token)
			{
				STLOG_WRITE(L"Arquivo de estrutura está incorreto. Linha: %S", line.c_str());
				break;
			}
			strcpy(tTable.tipo,token);
			token = strtok(NULL, sep);
			if(!token)
			{
				STLOG_WRITE(L"Arquivo de estrutura está incorreto. Linha: %S", line.c_str());
				break;
			}
			tTable.tamanho = atoi(token);

			vTable.push_back(tTable);
		}
		txtFields.close();
	}
}

/**
\brief
	Destrutor da classe
*/
CTableStruct::~CTableStruct(void)
{
	vTable.clear();
}

/**
\brief Procura todos os campos de uma determinada tabela e guarda em uma lista
\param LPCTSTR szTableName: Nome da tabela a ser buscada
\param _LISTCAMPO* fields: Ponteiro para a lista de campos da tabela buscada
\return TRUE se achar a tabela
*/
BOOL CTableStruct::GetTabela(LPCTSTR szTableName, _LISTCAMPO* fields)
{
	bool find = false;
	CString stTableName;

	for(int i=0; i< (int)vTable.size(); i++)
	{
		stTableName = CString(vTable[i].tnome);
		if(stTableName.CompareNoCase(szTableName)==0)
		{
			tTable = vTable.at(i);

			fields->AddTail(tTable);

			/*if(stTableName.CompareNoCase(L"AIIP_AD")==0)
			  STLOG_WRITE(L"CTableStruct::GetTabela CAMPO: %S", tTable.nome);*/

			find = true;
		}	
	}

	return find;
}



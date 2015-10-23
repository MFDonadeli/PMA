//
// C++ Implementation: veiculos
//
// Description:
//
//
// Author: Anderson de Castro Fonseca <cf.anderson@gmail.com>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "TxtSearch.h"
#include "cripto.h"
#include <tchar.h>


/**
\brief Construtor da classe
\param CMultiDlgBase* pParent
\return void
*/
CTxtSearch::CTxtSearch( ) : m_columnSize( 26 + 2 ) //(size of the column plus \n)
{
	memset(&m_registro, 0x00, 100);
	m_fileName = "";
}

/**
\brief Construtor da classe
\param char *file Nome do arquivo
\return void
*/
CTxtSearch::CTxtSearch( char *file ) : m_columnSize( 26 + 2 ) //(size of the column plus \n)
{
	m_fileName = file;
}

/**
\brief Elimina espaços à esquerda de uma determinada string
\param char *dest String usada como parâmetro
\return const char String resultante
*/
const char* TrimLeft( char *dest)
{
	if (!dest ) return dest; //all done
	size_t size = 0;
	// trim left
	while( size >= 0 && ( _istspace( dest[ size]) ||
		dest[ size] == 10 || dest[ size] == 13))
	{
		for ( size_t loop = 0; loop < strlen( dest ) -1; loop++ )
			dest[ loop] = dest[ loop +1];

		dest[ strlen( dest ) -1 ] = '\0';
	}
	return dest;
}

/**
\brief Elimina espaços à direita de uma determinada string
\param char *dest String usada como parâmetro
\return const char String resultante
*/
const char* TrimRight( char *dest)
{
	if (!dest ) return dest; //all done
	int size = int(strlen( dest ));

	// trim right
	size--;
	while( size >= 0 && ( _istspace( dest[ size]) ||
		dest[ size] == 10 || dest[ size] == 13))
	{
		dest[ size] = '\0';
		size--;
	}
	return dest;
}

/**
\brief Elimina espaços à esquerda e direita de uma determinada string
\param char *dest String usada como parâmetro
\return const char String resultante
*/
const char* strtrim( char *dest)
{
	TrimLeft ( dest );
	TrimRight ( dest );
	return dest;
}


/**
\brief Abre e lê aquivo de dados dos veículos, obtem tamanho do arquivo, número de linhas e valida sua estrutura
\details Faz chamada ao método isValid() para saber se arquivo é válido
\param void
\return bool
*/
bool CTxtSearch::open( )
{
	if( ! ( m_file = fopen( m_fileName, "r" ) ) ) return false;

	fseek(m_file, 0, SEEK_END);
	m_fileSize = ftell(m_file);
	fseek (m_file, 0, SEEK_SET);

	m_position = 1;
	m_totalLines = m_fileSize / m_columnSize;

	if( ! isValid( ) ) return false;

	return true;
}

void CTxtSearch::setupColumns(int* cols, int ncols, int mcol)
{
	iCols = cols;
	nCols = ncols;
	iMainCol = mcol;

	m_columnSize=0;


	for(int i=0; i<ncols; i++)
	{
		m_columnSize+=iCols[i];
	}

	m_columnSize+=2;
}

/**
\brief Fecha aquivo de dados dos veículos
\param void
\return void
*/
void CTxtSearch::close( )
{
	fclose( m_file );
}

/**
\brief Realiza pesquisa por placa do veículo
\param char *placa Placa do veículo em questão
\return bool
*/
bool CTxtSearch::find( char *placa )
{
	conteudo.clear();
	moveFirst( );
	if( ! strcmp( getColumn(iMainCol), placa ) ) return true;
	else
	{
		conteudo.clear();
		bool ret = locate( m_position, m_totalLines, placa );

		if( ! ret )
		{
			memset( m_placa, '\0', sizeof( m_placa ) );
			m_modelo = m_categoria = m_tipo = m_cor = m_especie = m_municipio = 0;
		}

		return ret;
	}
}

/**
\brief Retorna o número de linhas totais do arquivo
\param void
\return long
*/
long CTxtSearch::registerTotal( )
{
	return m_totalLines;
}

/**
\brief Movimenta o ponteiro de leitura do arquivo
\param long number Número da linha
\return bool
*/
bool CTxtSearch::goTo( long number )
{
	if( number < 1 || number > m_totalLines ) return false;

	m_position = number;
	getLine( );

	return true;
}

/**
\brief Incrementa o ponteiro de leitura do arquivo
\param void
\return void
*/
void CTxtSearch::moveNext( )
{
	goTo( ++m_position );
}

/**
\brief Decrementa o ponteiro de leitura do arquivo
\param void
\return void
*/
void CTxtSearch::movePrevious( )
{
	goTo( --m_position );
}

/**
\brief Move o ponteiro de leitura do arquivo para a primeira linha
\param void
\return void
*/
void CTxtSearch::moveFirst( )
{
	goTo( 1 );
}

/**
\brief Move o ponteiro de leitura do arquivo para a última linha
\param void
\return void
*/
void CTxtSearch::moveLast( )
{
	goTo( m_totalLines );
}

/**
\brief Informa se o ponteiro atingiu o final do arquivo
\param void
\return bool
*/
bool CTxtSearch::isEof( )
{
	return m_position == m_totalLines;
}

/**
\brief Informa se o ponteiro atingiu o começo do arquivo
\param void
\return bool
*/
bool CTxtSearch::isBof( )
{
	return m_position == 1;
}

/**
\brief Faz a validação da estrutura do arquivo
\param void
\return bool
*/
bool CTxtSearch::isValid( )
{
	return ( m_fileSize % m_columnSize ) == 0;
}

/**
\brief Retorna a placa do veículo
\param void
\return char
*/
char *CTxtSearch::placa( )
{
	return m_placa;
}

char *CTxtSearch::getColumn(int aCol)
{
	if(aCol!=-1)
		return (char*)conteudo.at(aCol).c_str();

	if(iRet == conteudo.size())
		return 0;
	else
		return (char*)conteudo.at(iRet++).c_str();
}

/**
\brief Retorna o modelo do veículo
\param void
\return long
*/
long CTxtSearch::modelo( )
{
	return m_modelo;
}

/**
\brief Retorna a categoria do veículo
\param void
\return int
*/
int CTxtSearch::categoria( )
{
	return m_categoria;
}

/**
\brief Retorna o tipo do veículo
\param void
\return int
*/
int CTxtSearch::tipo( )
{
	return m_tipo;
}

/**
\brief Retorna a cor do veículo
\param void
\return int
*/
int CTxtSearch::cor( )
{
	return m_cor;
}

/**
\brief Retorna a espécie do veículo
\param void
\return int
*/
int CTxtSearch::especie( )
{
	return m_especie;
}

/**
\brief Retorna o município do veículo
\param void
\return int
*/
int CTxtSearch::municipio( )
{
	return m_municipio;
}


/**
\brief Lê dados de uma linha, descriptografa e armazena dados nas suas respectivas variáveis
\param void
\return void
*/
void CTxtSearch::getLine( )
{
	iRet = 0;
	char* line = new char[m_columnSize+1];
	char buffer[ 10 ];
	int begin=0;
	char cont_temp[255];

	memset( m_placa, '\0', sizeof( m_placa ) );
	m_modelo = m_categoria = m_tipo = m_cor = m_especie = m_municipio = 0;

	fseek( m_file, ( m_position - 1 ) * m_columnSize, SEEK_SET );
	fgets( line, m_columnSize + 1, m_file );

	if( strlen( line ) == 0 ) return;

	char *decript = Cripto::decriptograph( line );

	strncpy(m_registro, decript, strlen(decript));

	//printf( "Line: [%s]", decript );
	//Getting the field placa--------------------------------------
	for(int i=0; i<nCols; i++)
	{
		strncpy( cont_temp, decript + begin, iCols[i] );
		begin += iCols[i];
		cont_temp[iCols[i]] = 0;
		strtrim(cont_temp);
		conteudo.push_back(cont_temp);
	}
/*
	//Getting the field modelo---------------------------------------
	memset( buffer, '\0', sizeof( buffer ) );
	strncpy( buffer, decript + BEGIN_MODELO - 1, WIDTH_MODELO );
	buffer[ WIDTH_MODELO + 1 ] = '\0';
	m_modelo = strtol( buffer, ( char ** ) &buffer, 10 );

	//Getting the field categoria-------------------------------------
	memset( buffer, '\0', sizeof( buffer ) );
	strncpy( buffer, decript + BEGIN_CATEGORIA - 1, WIDTH_CATEGORIA );
	buffer[ WIDTH_CATEGORIA + 1 ] = '\0';
	m_categoria = atoi( buffer );

	//Getting the field tipo-------------------------------------
	memset( buffer, '\0', sizeof( buffer ) );
	strncpy( buffer, decript + BEGIN_TIPO - 1, WIDTH_TIPO );
	buffer[ WIDTH_TIPO + 1 ] = '\0';
	m_tipo = atoi( buffer );

	//Getting the field cor-------------------------------------
	memset( buffer, '\0', sizeof( buffer ) );
	strncpy( buffer, decript + BEGIN_COR - 1, WIDTH_COR );
	buffer[ WIDTH_COR + 1 ] = '\0';
	m_cor = atoi( buffer );

	//Getting the field espC)cie-------------------------------------
	memset( buffer, '\0', sizeof( buffer ) );
	strncpy( buffer, decript + BEGIN_ESPECIE - 1, WIDTH_ESPECIE );
	buffer[ WIDTH_ESPECIE + 1 ] = '\0';
	m_especie = atoi( buffer );

	//Getting the field espC)cie-------------------------------------
	memset( buffer, '\0', sizeof( buffer ) );
	strncpy( buffer, decript + BEGIN_MUNICIPIO - 1, WIDTH_MUNICIPIO );
	buffer[ WIDTH_MUNICIPIO + 1 ] = '\0';
	m_municipio = atoi( buffer );*/

	delete line;

	//delete decript;
}

/**
\brief Localiza de modo recursivo, cadeia de string no arquivo
\param long pos_ini Posição inicial da pesquisa
\param long pos_fim Posição final da pesquisa
\param char *text Texto a ser pesquisado
\return bool
*/
bool CTxtSearch::locate( long pos_ini, long pos_fim, char *text )
{
	if( ( pos_fim - pos_ini ) < REST )
	{
		goTo( pos_ini );

		while( m_position <= pos_fim )
		{
			if( ! strcmp( text, getColumn(iMainCol) ) ) return true;
			conteudo.clear();
			moveNext( );
		}

		return false;
	}
	else
	{
		goTo( ( int ) ( ( pos_ini + pos_fim ) / 2 ) );

		int comp = strcmp( text, getColumn(iMainCol) );

		if( ! comp )
			return true;

		conteudo.clear();

		if( comp > 0 ) return locate( m_position, pos_fim, text );
		else return locate( pos_ini, m_position, text );
	}
}

/**
\brief Retorna o conteúdo de uma linha
\param void
\return char
*/
char *CTxtSearch::registro()
{
	return m_registro;
}


int CTxtSearch::size_registro()
{
	return m_columnSize-2;
}
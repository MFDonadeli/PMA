#include "stdafx.h"
#include "IniFile.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CIniFile::CIniFile(void)
{
	this->msts = new CMapStringToString(60);
}

CIniFile::~CIniFile(void)
{
	this->msts->RemoveAll();
	delete this->msts;
}

int CIniFile::loadFile(CString filename)
{
	CFile          file;
	CFileException fe;
	TCHAR          letra = ' ';
	CString        linha, linha2;
	CString		   campo;
	CString		   valor;	

	if(!file.Open(filename, CFile::modeRead|CFile::modeNoTruncate, &fe))
	{
		STLOG_WRITE(L"CIniFile::loadFile() Não foi possível abrir arquivo %s", filename);
		return 0;
	}
	
	while(file.Read(&letra,1))
	{
		if(letra != '\n')
		{
			if(letra != '\r')
				linha.Insert(linha.GetLength(), letra);
		}
		else
		{
			//STLOG_WRITE(L"CIniFile::loadFile Linha = %s", linha);

			if(linha.Find('[') == -1 && linha.GetLength() > 2)
			{
				campo = linha.Mid(0, linha.Find(L"="));
				valor = linha.Mid(linha.Find(L"=")+1, linha.GetLength());
				campo.Trim();
				valor.Trim();
				this->msts->SetAt(campo, valor);				
			}
			campo.Empty();
			valor.Empty();
			linha.Empty();
		}
	}
	file.Close();
	return 1;
}

CString CIniFile::getValue(CString key)
{
	CString ret;

	if(this->msts->Lookup(key, ret))
		return ret;
	else
		return L"";
}
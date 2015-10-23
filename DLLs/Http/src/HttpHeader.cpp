#include "StdAfx.h"
#include "HttpHeader.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

const wchar_t *NO_COMMA_SEP[] = { L"Date", L"Last-Modified", L"Expires", L"If-Modified-Since", L"Set-Cookie" };

CHttpHeader::CHttpHeader(void)
{
}

CHttpHeader::~CHttpHeader(void)
{
	//STLOG_WRITE("%s(%d): Ponto de apoio. Cleaning Memory...", __FUNCTION__, __LINE__);
	Clear();
}

BOOL CHttpHeader::Parse(const CStringA __text)
{
	// Convert ansi to unicode...
	return Parse(CStringW(__text));
}

BOOL CHttpHeader::Parse(const CStringW __text)
{
	int pos = 0;
	CStringW token;
	CStringList sl;

	// Quebrar o header em linhas...
	token = __text.Tokenize(L"\r\n", pos);
	while(token != "")
	{
		// Parse das linhas...
		//_ParseToken(token.Trim());
		sl.AddTail(token.Trim());

		token = __text.Tokenize(L"\r\n", pos);
	}

	POSITION p = sl.GetHeadPosition();
	while(p)
	{
		_ParseToken(sl.GetNext(p));
	}

#ifdef _DEBUG
	_Debug();
#endif

	return TRUE;
}

void CHttpHeader::_ParseToken(const CStringW __token)
{
	int pos = __token.Find(':');
	if(pos > 0)
	{
		CStringW lbl = __token.Mid(0, pos).Trim();
		CStringW val = __token.Mid(pos+1).Trim();

		//TRACE(__token.Mid(0, pos) + L"\r\n");
		//TRACE(__token.Mid(pos+1) + L"\r\n");
		
		CItemHeader *pHeader = NULL;
		if(!m_headers.Lookup(CStringW(lbl).MakeLower(), pHeader))
		{
			// Se nao existe, criar um novo...
			pHeader = new CItemHeader(lbl);
			m_headers.SetAt(CStringW(lbl).MakeLower(), pHeader);
		}

		// Nestes casos a virgula nao eh separador...
		for(int i = 0; i < ARRAYSIZE(NO_COMMA_SEP); i++)
		{
			if(lbl.CompareNoCase(NO_COMMA_SEP[i]) == 0)
			{
				// Atributo sem nome... adiciona como default...
				pHeader->m_defValues.AddTail(val.Trim());
				return;
			}
		}
		
		// Processar os outros tipos, separadores ',' e ';'
		if(val.FindOneOf(L";,") < 0)
		{
			// Um unico atributo... default...
			pHeader->m_defValues.AddTail(val.Trim());
		}
		else
		{
			// Mais de um atributo, vamos quebra-los...
			int pos = 0;
			CStringW token;
			token = val.Tokenize(L";,", pos);
			while(token != "")
			{
				//TRACE(L">>>" + token + L"\r\n");

				// Agora separar o label e valor do atributo...
				int pos1 = token.Find('=');
				if(pos1 > 0)
					pHeader->AddAttribute(token.Mid(0, pos1).Trim(), token.Mid(pos1+1).Trim());
				else
				{
					// Se nao encontrei o '=', eh um default...
					pHeader->m_defValues.AddTail(token.Trim());
				}

				token = val.Tokenize(L";,", pos);
			}
		}
	}
}

void CHttpHeader::_Debug()
{
/*
	TRACE(L" ----------------------- DEBUG --------------------------- \r\n");
	POSITION p = m_headers.GetStartPosition();
	while(p)
	{
		CStringW key;
		CItemHeader *pHeader;
		m_headers.GetNextAssoc(p, key, pHeader); 
		if(pHeader != NULL)
		{
			TRACE(L">> " + pHeader->m_key + L"\r\n");

			POSITION p2 = pHeader->m_defValues.GetHeadPosition();
			while(p2)
			{
				CStringW s = pHeader->m_defValues.GetNext(p2);
				TRACE(L"\t>> " + s + L"\r\n");
			}

			POSITION p1 = pHeader->m_attributes.GetStartPosition();
			while(p1)
			{
				CStringW key1;
				CAttribute *val;
				pHeader->m_attributes.GetNextAssoc(p1, key1, val);

				TRACE(L"\t>> " + val->m_key + L" = " + val->m_value + L"\r\n");
			}
		}
	}

	TRACE(L" ----------------------- DEBUG --------------------------- \r\n");
*/
}

BOOL CHttpHeader::GetAttribute(const CStringW &__header, const CStringW &__attribute, CStringW &__val)
{
	__val.Empty();

	// Localizar o header...
	CItemHeader *pHeader = NULL;
	if(!m_headers.Lookup(CStringW(__header).MakeLower(), pHeader))
		return FALSE;
	
	ASSERT(pHeader != NULL);

	// Se nao tem atributo, eh o default
	if(__attribute.IsEmpty())
	{
		// Monta lista de defaults separados por virgula...
		int count = 0;
		POSITION p = pHeader->m_defValues.GetHeadPosition();
		while(p)
		{
			if(count++ > 0)
				__val += ", ";

			__val += pHeader->m_defValues.GetNext(p);
		}

		return TRUE;
	}

	// Atributos nomeados, tentar localizar...
	CAttribute *value = NULL;
	if(!pHeader->m_attributes.Lookup(CStringW(__attribute).MakeLower(), value))
		return FALSE;

	ASSERT(value != NULL);

	// Recupera o valor...
	__val = value->m_value;

	return TRUE;
}

BOOL CHttpHeader::GetAttribute(const CStringA &__header, const CStringA &__attribute, CStringA &__val)
{
	// Ansi version...
	CStringW s;
	BOOL b = GetAttribute(CStringW(__header), CStringW(__attribute), s);
	if(b)
		__val = CStringA(s);

	return b;
}

BOOL CHttpHeader::AddAttribute(const CStringW &__header, const CStringW &__attribute, const CStringW &__val)
{
	CItemHeader *pHeader = NULL;
	if(!m_headers.Lookup(CStringW(__header).MakeLower(), pHeader))
	{
		pHeader = new CItemHeader(__header);
		m_headers.SetAt(CStringW(__header).MakeLower(), pHeader);
	}

	ASSERT(pHeader != NULL);

	// Default value
	if(__attribute.IsEmpty())
	{
		if(pHeader->m_defValues.Find(__val) == NULL)
			pHeader->m_defValues.AddTail(__val);

		return TRUE;
	}
	
	// Attribute
	CAttribute *value = NULL;
	if(!pHeader->m_attributes.Lookup(CStringW(__attribute).MakeLower(), value))
	{
		value = new CAttribute(__attribute, __val);
		pHeader->m_attributes.SetAt(CStringW(__attribute).MakeLower(), value);
	}
	else
	{
		value->m_value = __val;
	}

	return TRUE;
}

BOOL CHttpHeader::AddAttribute(const CStringA &__header, const CStringA &__attribute, const CStringA &__val)
{
	return AddAttribute(CStringW(__header), CStringW(__attribute), CStringW(__val));
}

BOOL CHttpHeader::GetDefaultAttribute(const CStringW &__header, CStringW &__val)
{
	return GetAttribute(__header, L"", __val);
}

BOOL CHttpHeader::GetDefaultAttribute(const CStringA &__header, CStringA &__val)
{
	CStringW s;
	BOOL b = GetDefaultAttribute(CStringW(__header), s);
	if(b)
		__val = CStringA(s);

	return b;
}

CStringA CHttpHeader::BuildHeaderA()
{
	CStringW s = BuildHeaderW();
	return CStringA(s);
}

CStringW CHttpHeader::BuildHeaderW()
{
	CStringW buffer;

	// Exemplo:
	// Set-Cookie: PHPSESSID=jhc6s20okv8jv0msrevieg9f27; path=/
	// Set-Cookie [NOME HEADER]
	//           : [SEPARADOR]
	//			   PHPSESSID=jhc6s20okv8jv0msrevieg9f27 [ATRIBUTO]
	//												   ; [SEPARADOR]
	//													 path=/ [ATRIBUTO]

	// Varrer os headers...
	POSITION p = m_headers.GetStartPosition();
	while(p)
	{
		CStringW key;
		CItemHeader *pHeader;
		m_headers.GetNextAssoc(p, key, pHeader); 
		if(pHeader != NULL)
		{
			// Nao adicionar no header...
			if(key.CompareNoCase(L"Set-Cookie") == 0)
				continue;

			// Adiciona o separador...
			buffer += pHeader->m_key + L": ";

			// Varrer os defaults...
			int count = 0;
			POSITION p2 = pHeader->m_defValues.GetHeadPosition();
			while(p2)
			{
				if(count++ > 0)
					buffer += ", ";
				
				buffer += pHeader->m_defValues.GetNext(p2);
			}

			// Adicionar os nomeados...
			POSITION p1 = pHeader->m_attributes.GetStartPosition();
			while(p1)
			{
				if(count++ > 0)
					buffer += ", ";

				CStringW key1;
				CAttribute *val;
				pHeader->m_attributes.GetNextAssoc(p1, key1, val);

				buffer += val->m_key + L"=" + val->m_value;
			}

			// Adiciona a quebra de linha...
			buffer += L"\r\n";
		}
	}

	return buffer;
}

void CHttpHeader::CopyFrom(CHttpHeader *__pSource)
{
	if(__pSource != NULL)
	{
		POSITION p = __pSource->m_headers.GetStartPosition();
		while(p)
		{
			CStringW key;
			CItemHeader *pHeader;
			__pSource->m_headers.GetNextAssoc(p, key, pHeader);
			if(pHeader != NULL)
			{
				CItemHeader *pNewHeader = new CItemHeader(pHeader->m_key);
				m_headers.SetAt(key, pNewHeader);

				POSITION p2 = pHeader->m_defValues.GetHeadPosition();
				while(p2)
				{
					CString s = pHeader->m_defValues.GetNext(p2);
					pNewHeader->m_defValues.AddTail(s);
				}

				POSITION p1 = pHeader->m_attributes.GetStartPosition();
				while(p1)
				{
					CStringW key1;
					CAttribute *pAttr;
					pHeader->m_attributes.GetNextAssoc(p1, key1, pAttr);
					if(pAttr != NULL)
					{
						pNewHeader->AddAttribute(pAttr->m_key, pAttr->m_value);
					}
				}
			}
		}

#ifdef _DEBUG
		_Debug();
#endif
	}
}

void CHttpHeader::Clear()
{
	// Efetuar a limpeza da memoria alocada...

	// Varrer os headers...
	POSITION p = m_headers.GetStartPosition();
	while(p)
	{
		CStringW key;
		CItemHeader *pHeader;
		m_headers.GetNextAssoc(p, key, pHeader);
		if(pHeader != NULL)
		{
			// Remover os atributos default...
			pHeader->m_defValues.RemoveAll();
			
			// Varrer os atributos nomeados...
			POSITION p1 = pHeader->m_attributes.GetStartPosition();
			while(p1)
			{
				CStringW key1;
				CAttribute *pAttr;
				pHeader->m_attributes.GetNextAssoc(p1, key1, pAttr);

				// Deletar...
				if(pAttr != NULL)
					delete pAttr;
			}

			// Deletar...
			pHeader->m_attributes.RemoveAll();
			delete pHeader;
		}
	}

	m_headers.RemoveAll();
}

void CHttpHeader::AddDefault(const CStringW &__header, const CStringW &__val)
{
	CItemHeader *pHeader = NULL;

	if(!m_headers.Lookup(CStringW(__header).MakeLower(), pHeader))
	{
		pHeader = new CItemHeader(__header);
		m_headers.SetAt(CStringW(__header).MakeLower(), pHeader);
	}

	ASSERT(pHeader != NULL);

	pHeader->AddDefault(__val);
}

void CHttpHeader::AddDefault(const CStringA &__header, const CStringA &__val)
{
	AddDefault(CStringW(__header), CStringW(__val));
}

void CHttpHeader::DelDefault(const CStringW &__header)
{
	CItemHeader *pHeader = NULL;
	if(!m_headers.Lookup(CStringW(__header).MakeLower(), pHeader))
	{
		pHeader = new CItemHeader(__header);
		m_headers.SetAt(CStringW(__header).MakeLower(), pHeader);
	}

	ASSERT(pHeader != NULL);

	pHeader->Reset();
}

void CHttpHeader::DelDefault(const CStringA &__header)
{
	DelDefault(CStringW(__header));
}

void CHttpHeader::Remove(const CStringA &__header)
{
	Remove(CStringW(__header));
}

void CHttpHeader::Remove(const CStringW &__header)
{
	CItemHeader *pHeader = NULL;
	if(m_headers.Lookup(CStringW(__header).MakeLower(), pHeader))
	{
		delete pHeader;
		m_headers.RemoveKey(CStringW(__header).MakeLower());
	}
}

BOOL CHttpHeader::IsDefined(const CStringA &__header)
{
	CItemHeader *pHeader = NULL;
	return m_headers.Lookup(CStringW(__header), pHeader);
}

CItemHeader *CHttpHeader::Find(const CStringA &__header)
{
	CItemHeader *pHeader = NULL;
	m_headers.Lookup(CStringW(__header), pHeader);
	return pHeader;
}

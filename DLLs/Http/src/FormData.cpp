#include "StdAfx.h"
#include "FormData.h"

#include "Utils2.h"

// Content-Type: application/x-www-form-urlencoded; boundary=---------------------------7d9781630cc8
//
//-- ---------------------------7d9781630cc8
//Content-Disposition: form-data; name="uploaded"; filename="Base64.cpp"
//Content-Type: text/plain
//
//TESTE (CONTEUDO DO ARQUIVO)
//-----------------------------7d9781630cc8
//Content-Disposition: form-data; name="submit"
//
//Submit
//-- ---------------------------7d9781630cc8--
//

#define CONTENT_TYPE	_T("multipart/form-data; boundary=%s")
#define FILE_BOUND		_T("Content-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\nContent-Type: %s\r\n")
#define FIELD_BOUND		_T("Content-Disposition: form-data; name=\"%s\"\r\n\r\n%s\r\n")

//
//Submit

CFormData::CFormData(void)
{
	m_sBoundary = _T("BOUNDARY");
}

CFormData::~CFormData(void)
{
	//STLOG_WRITE("%s(%d): Ponto de apoio. Cleaning Memory...", __FUNCTION__, __LINE__);
	Clear();
}

void CFormData::Clear()
{
	// Varrer os campos...
	POSITION p1 = m_items.GetStartPosition();
	while(p1)
	{
		CStringW key1;
		CFormItem *pAttr;
		m_items.GetNextAssoc(p1, key1, pAttr);

		// Deletar...
		if(pAttr != NULL)
			delete pAttr;
	}
	m_items.RemoveAll();

	m_buffer.RemoveAll();
}

void CFormData::AddItem(LPCTSTR __szField, LPCTSTR __szValue)
{
	CFormItem *attr = NULL;

	// Verifica se ja foi inserido...
	if(!m_items.Lookup(CStringW(__szField).MakeLower(), attr))
		attr = new CFormItem(__szField, __szValue);
	else
		attr->m_value = __szValue; // Atualiza...

	m_items.SetAt(CStringW(__szField).MakeLower(), attr);
}

void CFormData::AddFile(LPCTSTR __szField, LPCTSTR __szFilename, LPCTSTR __szMime)
{
	CFormItem *attr = NULL;

	CStringW sMime = __szMime;
	// Se o mime for null, vamos tentar determinar...
	if(__szMime == NULL)
	{
		CStringW s1 = __szFilename;
		int pos = 0;
		if((pos = s1.ReverseFind('.')) > 0)
			s1 = s1.Right(s1.GetLength()-pos-1);
		sMime = GetMime(s1);
	}

	// Verificar se ja foi incluido...
	if(!m_items.Lookup(CStringW(__szField).MakeLower(), attr))
		attr = new CFormItem(__szField, __szFilename, sMime);
	else
	{
		// Se sim, vamos atualizar o item...
		attr->m_value = __szFilename;
		attr->m_mime  = sMime;
	}

	m_items.SetAt(CStringW(__szField).MakeLower(), attr);
}

ULONG CFormData::ProcessForm()
{
	CStringA startPart = "--" + CStringA(m_sBoundary) + "\r\n";
	Append(startPart);

	BOOL bHasFields = FALSE;

	//STLOG_WRITE("%s(%d): Ponto de apoio", __FUNCTION__, __LINE__);

	// Varrer os fields primeiro
	POSITION p1 = m_items.GetStartPosition();
	while(p1)
	{
		CStringW key1;
		CFormItem *pAttr;
		m_items.GetNextAssoc(p1, key1, pAttr);
		if(!pAttr->m_bFile)
		{
			bHasFields = TRUE;
			CStringW s;
			s.Format(FIELD_BOUND, pAttr->m_name, pAttr->m_value);
			Append(CStringA(s));
		}

		if(p1 != NULL)
		{
			startPart = "--" + CStringA(m_sBoundary) + "\r\n";
			Append(startPart);
		}
	}

	//STLOG_WRITE("%s(%d): Ponto de apoio", __FUNCTION__, __LINE__);

	BOOL bFirst = TRUE;
	// Varrer os files
	p1 = m_items.GetStartPosition();
	while(p1)
	{
		CStringW key1;
		CFormItem *pAttr;
		m_items.GetNextAssoc(p1, key1, pAttr);
		if(pAttr->m_bFile)
		{
			if(bFirst && bHasFields)
			{
				startPart = "--" + CStringA(m_sBoundary) + "\r\n";
				Append(startPart);
				bFirst = FALSE;
			}

			CStringW sFileName = pAttr->m_value;
			if(sFileName.Find('\\') >= 0 || sFileName.Find('/') >= 0)
			{
				int pos = sFileName.ReverseFind('\\');
				if(pos < 0)
					pos = sFileName.ReverseFind('/');

				sFileName = sFileName.Mid(pos+1);
			}

			CStringW s;
			s.Format(FILE_BOUND, pAttr->m_name, sFileName, pAttr->m_mime);
			Append(CStringA(s));

			Append("\r\n");

			AppendFile(pAttr->m_value);

			Append("\r\n");

			if(p1 != NULL)
			{
				startPart = "--" + CStringA(m_sBoundary) + "\r\n";
				Append(startPart);
			}
		}
	}

	startPart = "--" + CStringA(m_sBoundary) + "--\r\n";
	Append(startPart);

	//STLOG_WRITE("%s(%d): Ponto de apoio", __FUNCTION__, __LINE__);
	//STLOG_WRITE("%s(%d): Buffer: %s", __FUNCTION__, __LINE__, m_buffer.GetData());

	return m_buffer.GetCount();
}

ULONG CFormData::GetFileSize(LPCTSTR __szFilename)
{
	CFile f;
	if(f.Open(__szFilename, CFile::modeRead, NULL))
	{
		ULONG v = (ULONG)f.GetLength();
		f.Close();
	}

	return 0;
}

CStringA CFormData::GetContentType()
{
	CString s;
	s.Format(CONTENT_TYPE, m_sBoundary);
	return CStringA(s);
}

CStringW CFormData::GetMime(LPCTSTR __szExt)
{
	HKEY   hkey;
	DWORD dwType, dwSize;
	wchar_t buffer[255];

	ZeroMemory(&buffer, sizeof(buffer));

	CStringW s;
	s.Format(L"\\.%s", __szExt);

	if(RegOpenKeyEx(HKEY_CLASSES_ROOT, s, 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS)
	{
		dwType = REG_SZ;
		dwSize = sizeof(buffer);
		RegQueryValueEx(hkey, TEXT("Content Type"), 0, &dwType, (LPBYTE)&buffer, &dwSize);
        
		RegCloseKey(hkey);
	}

	return CStringW(buffer);
}

void CFormData::Append(LPCSTR szBuffer)
{
	for(size_t i = 0; i < strlen(szBuffer); i++)
		m_buffer.Add(szBuffer[i]);
}

ULONG CFormData::AppendFile(LPCTSTR __szFilename)
{
	CFile f;
	if(f.Open(__szFilename, CFile::modeRead, NULL))
	{
		BYTE buff[4096];
		UINT readed = 0;
		ULONG total = 0;

		do
		{
			ZeroMemory(&buff, sizeof(buff));
			readed = f.Read(&buff, sizeof(buff));

#define CRIPTOGRAFA_XML
#ifdef CRIPTOGRAFA_XML
			if(CString(__szFilename).Find(L".xml") > -1)
			{
				if (readed > 0)
				{
					char password[100]= "";

					BYTE bTemp[4096];
					ZeroMemory(&bTemp, sizeof(bTemp));
					memcpy(&bTemp, &buff, readed);
					ZeroMemory(&buff, sizeof(buff));
					int ret = CUtils2::CriptoBuffer((char *)&bTemp, (char *)&buff, &password[0], readed, TRUE); //decrypt
					if (!ret > 0)
					{
						STLOG_WRITE(L"%S(%d): Falha ao descriptografar xml", __FUNCTION__, __LINE__ );
					}
					readed = ret;
				}
			}

#endif

			for(UINT i = 0; i < readed; i++)
				m_buffer.Add(buff[i]);
			
			total += readed;

		} while(readed != 0);

		f.Close();

		return total;
	}

	return 0;
}



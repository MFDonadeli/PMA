#include "stdafx.h"
#include "httpResponse.h"
#include "HTTPCtrl.h"
#include "HttpHeader.h"

CHttpResponse::CHttpResponse() 
	: m_statusCode(0)
{
}

CHttpResponse::~CHttpResponse()
{
}

void CHttpResponse::Init(CHTTPCtrl *__src, CHttpHeader *__headers, int __status)
{
	m_src = __src;
	m_statusCode = __status;
	// Guardar uma copia do header...
	m_headers.CopyFrom(__headers);
}

void CHttpResponse::ReadToBuffer(CArray<BYTE> &__buff)
{
	// Limpar o buffer...
	__buff.RemoveAll();
	BYTE tmp[PH_BUFFER_SIZE];
	// Ler o socket enquanto houver dados...
	DWORD cb = 0;
	do
	{
		m_src->Read(tmp, sizeof(tmp), &cb);
		for(DWORD i = 0; i < cb; i++)
			__buff.Add(tmp[i]);
    
	} while (cb > 0);
}

static HRESULT Utf8Transcoder(CArray<UTF16> &tmpBuffer, const BYTE *start, DWORD len, CStringW &dest, DWORD *numBytesLeftInSrc, CStringW &msgErr)
{
	// pior caso eh sempre UTF8 para UTF16...
	tmpBuffer.SetSize(len);

	const UTF8 *srcStart = (const UTF8 *)start;
	const UTF8 *srcEnd   = (const UTF8 *)(start + len);
	UTF16 *destStart	 = &tmpBuffer.GetData()[0];
	UTF16 *destEnd		 = destStart + tmpBuffer.GetSize();

	// Efetuar a conversao...
	ConversionResult cr = ConvertUTF8toUTF16(&srcStart, srcEnd, &destStart, destEnd, lenientConversion);

	if(sourceIllegal == cr)
	{
		msgErr = _T("The returned UTF-8 data has an invalid byte sequence for UTF-8 encoded data");
		return E_FAIL;
	}

	*numBytesLeftInSrc = (srcEnd - srcStart);

	// Agora para WCHAR...
	dest += CStringW((LPWSTR)tmpBuffer.GetData(), destStart - &tmpBuffer.GetData()[0]);

	return S_OK;
}

static HRESULT Utf16Transcoder(CArray<UTF16> &tempBuffer, const BYTE *start, DWORD len, CStringW &dest, DWORD *numBytesLeftInSrc, CStringW &msgErr)
{
	// Se o charset for UTF16, basta copiar para o destino...
	if((len % 2) == 1)
	{
		--len;
		*numBytesLeftInSrc = 1;
	}
	else
		*numBytesLeftInSrc = 0;

	return dest += (LPCWSTR) start, len/2;
}

static HRESULT CPTranscoder(CArray<UTF16> &tmpBuffer, const BYTE *start, DWORD len, CStringW &dest, DWORD *numBytesLeftInSrc, CStringW &msgErr)
{
	tmpBuffer.SetSize(len);
	
	// Conversao de charset utilizando o codepage local...
	DWORD wideLen = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)start, len, (LPWSTR)&tmpBuffer.GetData()[0], len);
	if(wideLen == 0)
	{
		DWORD dwerr = GetLastError();
		msgErr = _T("Unable to convert source bytes to string using the local code page");

		return HRESULT_FROM_WIN32(dwerr);
	}

	// Agora para WCHAR...
	dest += CStringW((LPWSTR)&tmpBuffer.GetData()[0], wideLen);
	*numBytesLeftInSrc = 0;
	
	return S_OK;
}

CStringW CHttpResponse::GetString() 
{
	//STLOG_WRITE("Entrando em GetString()");

	// verificar se no header content-type, temos o charset...
	CStringA charset;
	m_headers.GetAttribute("Content-Type", "charset", charset);

	CStringW dest;

	if(!charset.IsEmpty() && ((_stricmp(charset, "utf-8") == 0) || (_stricmp(charset, "utf8") == 0)))
	{
		// Converter UTF8...
		//STLOG_WRITE("Entrando em UTF8");
		ReadToString(dest, Utf8Transcoder);
	}
	else if(!charset.IsEmpty() && ((_stricmp(charset, "utf-16") == 0) || (_stricmp(charset, "utf16") == 0)))
	{
		//STLOG_WRITE("Entrando em UTF16");
		// Converter UTF16...
		ReadToString(dest, Utf16Transcoder);
	}
	else
	{
		//STLOG_WRITE("Entrando em CP");
		// Vamos assumir o codepage local...
		ReadToString(dest, CPTranscoder);
	}

	return dest;
}

BOOL CHttpResponse::ReadToString(CStringW &__dest, TranscodeToUtf16 __transcoder)
{
	BYTE tmp[PH_BUFFER_SIZE] ;
	DWORD cb = 0, numBytesLeftInSrc = 0, srcSize;
	CArray<UTF16> tmpBuffer;
	do
	{
		// Ler dados do socket...
		m_src->Read(tmp + numBytesLeftInSrc, sizeof(tmp) - numBytesLeftInSrc, &cb);
		if(cb > 0)
		{
			srcSize = cb + numBytesLeftInSrc;
			CStringW sErr;
			//STLOG_WRITE("Tentando obter resposta: %s", tmp);
			HRESULT hr = __transcoder(tmpBuffer, tmp, srcSize, __dest, &numBytesLeftInSrc, sErr);
			if(FAILED(hr))
			{
				Log(sErr);
				return FALSE;
			}

			//STLOG_WRITE("Tentando obter resposta2: %S", __dest);

			if(numBytesLeftInSrc > 0)
				memcpy(tmp, tmp + srcSize - numBytesLeftInSrc, numBytesLeftInSrc);
		}

	} while(cb > 0); // Enquanto tiver dados...

	return TRUE;
}

BOOL CHttpResponse::SaveAs(LPCSTR __filename)
{
	return SaveAs(CStringW(__filename));
}

BOOL CHttpResponse::SaveAs(LPCWSTR __filename)
{
	// Criar o arquivo...
	HANDLE hfile = CreateFile(__filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, CREATE_FILE_FLAGS, NULL);
	if(INVALID_HANDLE_VALUE == hfile)
	{
		Log(_T("Error creating response file"), GetLastError());
		return FALSE;
	}

	BYTE buff[PH_BUFFER_SIZE];
	DWORD cbRead =0, cbWrite = 0;
	do 
	{
		// Ler os dados do socket...
		if(!m_src->Read(buff, sizeof(buff), &cbRead))
		{
			CloseHandle(hfile);
			return FALSE;
		}

		if(cbRead > 0)
		{
			if(!WriteFile(hfile, buff, cbRead, &cbWrite, NULL))
			{
				CloseHandle(hfile);
				Log(_T("Error wrting data to response file"), GetLastError());
				return FALSE;
			}
		} 
	} while ( cbRead > 0);

	// Fechar o arquivo...
	CloseHandle(hfile);
	return TRUE;
}

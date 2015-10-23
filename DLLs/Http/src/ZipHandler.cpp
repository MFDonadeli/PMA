#include "StdAfx.h"
#include "ZipHandler.h"


CInflateHandler::CInflateHandler(CTransferDecoder * __src)
	:CTransferDecoder(__src)
	, m_initDone(FALSE) 
	, m_eof(FALSE)
{
	memset(&m_stream , 0, sizeof(m_stream));
}

CInflateHandler::~CInflateHandler()
{
	if(m_initDone)
		inflateEnd(&m_stream);

	if(m_pTd != NULL)
	{
		delete m_pTd;
		m_pTd = NULL;
	}
}

BOOL CInflateHandler::Read(BYTE *__pvx, DWORD __cb, DWORD *__pcbRead)
{
	if(!ReadInput(__cb))
		return FALSE;

	// Terminou?
	if(m_eof)
	{
		*__pcbRead = 0;
		return TRUE;
	}

	// this is complicated by the fact that its not clear in the http spec
	// if the deflate scheme should include the 2 byte zlib stream header or
	// not, there seems to be a split as to which servers generate it and
	// which don't, so we end up having to try both ways, arrrgghhhh
	int zlibErr;
	BOOL tryWithHeaderNext = FALSE;

	if(!m_initDone)
	{
		if(!PreInit())
			return FALSE;

		// make sure we have some input if the PreInit call used up the
		// whole buffer contents
		if(m_buff.GetCount() == 0)
			if(!ReadInput(__cb))
				return FALSE;
	}
	
	while(true)
	{
		// make sure the stream buffer states are correct.
		m_stream.next_in   = &m_buff.GetData()[0];
		m_stream.avail_in  = m_buff.GetCount();
		m_stream.next_out  = __pvx;
		m_stream.avail_out = __cb;

		// init the stream first time around
		if(!m_initDone)
		{
			m_stream.zalloc = (alloc_func)0;
			m_stream.zfree  = (free_func) 0;

			if(!tryWithHeaderNext)
			{
				zlibErr = inflateInit2(&m_stream, -MAX_WBITS);
				tryWithHeaderNext = TRUE;
			}
			else
			{
				zlibErr = inflateInit(&m_stream);
				tryWithHeaderNext = FALSE;
			}

			if(Z_OK != zlibErr)
			{
				Log(_T("There was an error trying to uncompress the response from the server"));
				return FALSE;
			}

			m_initDone = TRUE;
		}

		// inflate whatever we've got
		zlibErr = inflate(&m_stream, Z_NO_FLUSH);

		// err ?
		if((Z_OK != zlibErr) && (Z_STREAM_END != zlibErr))
		{
			if(!tryWithHeaderNext)
			{
				// if we're doing inflate without the zlib header (i.e. gzip and some
				// deflate responses), we can get a Z_BUF_ERROR when we try and do
				// an avail_in == 0, this is not a problem, see zlib/contrib/minizip/unzip.c
				// and http://gcc.gnu.org/ml/java-patches/2000-q4/msg00263.html
				if(Z_BUF_ERROR == zlibErr && m_stream.avail_in == 0)
				{
					BYTE dummy = 0;
					m_stream.next_in  = &dummy;
					m_stream.avail_in = 1;

					int rc = inflate(&m_stream, Z_FINISH);

					if(rc == Z_STREAM_END)
						zlibErr = rc;
				}

				if(Z_STREAM_END != zlibErr)
				{
					Log(_T("There was an error trying to uncompress the response from the server"));
					return FALSE;
				}
			}

			inflateEnd(&m_stream);
			m_initDone = FALSE;
		}
		else
		{
			break;
		}
	}

	// Remover o que foi processado...
	int x = (m_stream.next_in - m_buff.GetData());
	for(INT_PTR i = 0; i < x; ++i)
	{
		m_buff.RemoveAt(0);
	}

	//Terminou?
	if(Z_STREAM_END == zlibErr)
		m_eof = TRUE;
	
	//Atualizar o tamanho...
	*__pcbRead = __cb - m_stream.avail_out;

	return TRUE;
}
	
BOOL CInflateHandler::PreInit()
{
	return TRUE;
}

BOOL CInflateHandler::ReadInput(DWORD cb)
{
	BOOL hr = TRUE;

	// read something from the source, if its still active
	if(m_pTd != NULL)
	{
		// if the caller is using large buffers, we will too[ish]
		DWORD cbToRead = max(PH_BUFFER_SIZE/2, cb/2);
		DWORD cbSrc;
		DWORD oldSize = m_buff.GetCount();

		BYTE *p = new BYTE[cbToRead];
		hr = m_pTd->Read(p, cbToRead, &cbSrc);

		for(DWORD i = 0; i < cbSrc; i++)
			m_buff.Add(p[i]);

		delete [] p;
		
		if(!cbSrc)
		{
			delete m_pTd;
			m_pTd = NULL;
		}
	}

	return hr;
}

BOOL CInflateHandler::EnsureBufferContains(DWORD cb)
{
	while(m_buff.GetCount() < (INT_PTR)cb)
		ReadInput(cb);

	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
CUnGzipHandler::CUnGzipHandler(CTransferDecoder * __src)
	: CInflateHandler(__src)
{
}

CUnGzipHandler::~CUnGzipHandler(void)
{
}

BOOL CUnGzipHandler::Read(BYTE *__pvx, DWORD __cb, DWORD *__pcbRead)
{
	BOOL eofAtStart = m_eof;
	if(!CInflateHandler::Read(__pvx, __cb, __pcbRead))
		return FALSE;

	if(*__pcbRead)
	{
		m_crc32 = crc32(m_crc32, __pvx, *__pcbRead);
		m_totalSize += *__pcbRead;
	}

	if((!eofAtStart) && m_eof)
		return EndOfInflation();

	return TRUE;
}

BOOL CUnGzipHandler::PreInit()
{
	// walk over the gzip file header to get to the compressed data
	if(!EnsureBufferContains(gzip_MIN_HDR_SIZE))
		return FALSE;

	if(m_buff[0] != gzip_id1 || m_buff[1] != gzip_id2 || m_buff[2] != gzip_cm_deflate)
	{
		Log(_T("GZIP File header is invalid"));
		return FALSE;
	}

	BYTE flags = m_buff[3] ;
	bool fCrc 	  = (flags & gzip_FHCRC)	> 0;
	bool fExtra	  = (flags & gzip_FEXTRA)   > 0;
	bool fName	  = (flags & gzip_FNAME)	> 0;
	bool fComment = (flags & gzip_FCOMMENT) > 0;

	// move over the fixed part of the gzip header
	int offset = gzip_MIN_HDR_SIZE;

	if(fExtra)
	{
		// skip over the extra stuff
		if(!EnsureBufferContains(offset+2))
			return FALSE;

		unsigned short cb = *((unsigned short *)&m_buff[offset]);
		offset += 2;	// the length of the extra data
		offset += cb;	// the extra data

		if(!EnsureBufferContains(offset))
			return FALSE;
	}
	
	if(fName)
	{
		// skip over the name field
		while(m_buff[offset++] != 0)
			if(!EnsureBufferContains(offset))
				return FALSE;
	}

	if(fComment)
	{
		// skip over the comment
		while(m_buff[offset++] != 0)
			if(!EnsureBufferContains(offset))
				return FALSE;
	}

	if(fCrc)
	{
		// skip over the crc
		offset += 2;
		if(!EnsureBufferContains(offset))
			return FALSE;
	} 

	// remove everything we've read
	for(INT_PTR i = 0; i < offset; ++i)
		m_buff.RemoveAt(0);

	// reset the length and crc rolling values
	m_crc32     = crc32(0,NULL,0);
	m_totalSize = 0;

	return TRUE;
}

BOOL CUnGzipHandler::EndOfInflation()
{
	// Precisamos ler 8 bytes, 4 de CRC e 4 do tamanho do corpo
	if(!EnsureBufferContains(8))
		return FALSE;

	DWORD len = 0, crc = 0;

	crc |= m_buff[0] ;	
	crc |= m_buff[1] << 8;	
	crc |= m_buff[2] << 16;	
	crc |= m_buff[3] << 24;

	len |= m_buff[4] ;	
	len |= m_buff[5] << 8;	
	len |= m_buff[6] << 16;	
	len |= m_buff[7] << 24;

	CString s;
	s.Format(_T("crc %08lx expected %08lx \t\tuncompressed length %08lx expected %08lx"),
			 m_crc32, 
			 crc, 
			 m_totalSize, 
			 len);
	Log(s);

	for(INT_PTR i = 0; i < 8; ++i)
		m_buff.RemoveAt(0);

	// Verificar o CRC
	if(crc != m_crc32)
	{
		s.Format(_T("Invalid gzip crc value, expecting 0x%08lx, found 0x%08lx"), crc, m_crc32);
		Log(s);
		return FALSE;
	}

	// Verificar o tamanho
	if(len != m_totalSize)
	{
		s.Format(_T("Invalid gzip length value, expecting 0x%08lx, found 0x%08lx"), len, m_totalSize);
		Log(s);
		return FALSE;
	}

	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
CDeflateHandler::CDeflateHandler(CStreamReader * __src)
{
	m_src		= __src;
	m_initDone  = FALSE;
	m_eof	    = FALSE;
	m_srcDone   = FALSE;
	m_compLevel = -1;
}

CDeflateHandler::~CDeflateHandler()
{
	if(m_initDone) 
		deflateEnd(&m_stream);

	if(m_src != NULL)
	{
		delete m_src;
		m_src = NULL;
	}
}

BOOL CDeflateHandler::Read(LPVOID __pvx, ULONG __cb, ULONG *__pcbRead)
{
	// Ainda nao terminou, note que este metodo eh chamado recursivamente...
	if(!m_srcDone)
	{
		DWORD cbToRead = PH_BUFFER_SIZE / 2;
		DWORD cbSrc;
		DWORD oldSize = m_buff.GetCount();

		BYTE *b = new BYTE[cbToRead];
		ZeroMemory(b, sizeof(b));
		BOOL hr = m_src->Read(b, cbToRead, &cbSrc);
		
		for(DWORD i = 0; i < cbSrc; ++i)
			m_buff.Add(b[i]);

		if(cbSrc > 0)
			ProcessSource(m_buff.GetData() + oldSize, cbSrc);

		if(cbSrc == 0)
			m_srcDone = TRUE;

		delete [] b;
	}

	// Se acabou...
	if(m_eof)
	{
		*__pcbRead = 0;
		return TRUE;
	}

	// Se for a primeira vez...
	if(!m_initDone)
	{
	    m_stream.zalloc = Z_NULL;
	    m_stream.zfree  = Z_NULL;
	    m_stream.opaque = Z_NULL;

		int zlibErr = DeflateInit();
		if(Z_OK != zlibErr)
		{
			Log(_T("There was a problem trying to compress the request data"));
			return FALSE;
		}

		m_initDone = TRUE;
	}

	// Fazer o setup dos ponteiros...
	m_stream.next_in   = &m_buff.GetData()[0];
	m_stream.avail_in  = m_buff.GetCount();
	m_stream.next_out  = (BYTE *)__pvx;
	m_stream.avail_out = __cb;

	// fazer o deflate...
	int zlibErr = deflate(&m_stream, m_srcDone ? Z_FINISH : Z_NO_FLUSH);

	// err ?
	if((Z_OK != zlibErr) && (Z_STREAM_END != zlibErr))
	{
		Log(_T("There was a problem trying to compress the request data"));
		return FALSE;
	}

	// Terminou?
	if(m_srcDone && (Z_STREAM_END == zlibErr))
	{
		GenerateTrailer();
		m_eof = TRUE;
	}

	int a = m_stream.next_in - m_buff.GetData();

	// Remover o que foi processado...
	for(INT_PTR i = 0; i < a; ++i)
	{
		m_buff.RemoveAt(0);
	}

	// Atualizar a qtde lida...
	*__pcbRead = __cb - m_stream.avail_out;

	// Se ainda tiver dados, vamos recursar...
	if((!m_eof) && ((*__pcbRead == 0) || (__cb - *__pcbRead) >= 512))
	{
		DWORD pcbNested = 0 ;
		BOOL hr = Read((BYTE *)__pvx + *__pcbRead, __cb - *__pcbRead, &pcbNested);
		(*__pcbRead) += pcbNested;
		return hr;
	}

	return TRUE;
}

int	CDeflateHandler::DeflateInit()
{
	return deflateInit(&m_stream, m_compLevel);
}

void CDeflateHandler::ProcessSource(BYTE *pv, DWORD cb)
{
	// nothing todo
}

void CDeflateHandler::GenerateTrailer()
{
	// deflate has no trailer
}

void CDeflateHandler::Reset()
{
	m_buff.RemoveAll();

	if(m_initDone) 
		deflateEnd(&m_stream);

	m_initDone = FALSE;
	m_eof	   = FALSE;
	m_srcDone  = FALSE;

	if(m_src != NULL)
		m_src->Reset();
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
CGzipHandler::CGzipHandler(CStreamReader * __src)
	: CDeflateHandler(__src)
{
	Init();
}

CGzipHandler::~CGzipHandler(void)
{
}

BOOL CGzipHandler::Read(LPVOID __pvx, ULONG __cb, ULONG *__pcbRead)
{
	// Gera o header...
	if(!m_initDone)
		GenerateHeader();

	// Grava o header e/ou trailer...
	if(m_cOut > 0)
	{
		ULONG c = min(m_cOut, __cb);
		memcpy(__pvx, m_outBuff, c);
		m_cOut -= (BYTE)c;
		*__pcbRead = c;
		BYTE *pvx = (BYTE *)__pvx;
		pvx += c;
		__pvx = pvx;
	}
	else
		*__pcbRead = 0;

	ULONG inCB = 0;
	CDeflateHandler::Read(__pvx, __cb, &inCB);
	*__pcbRead += inCB;

	return TRUE;
}

void CGzipHandler::Reset()
{
	CDeflateHandler::Reset();
	Init();
}

void CGzipHandler::Init()
{
	m_cOut		= 0;
	m_crc32		= 0;
	m_totalSize = 0;
}

void CGzipHandler::GenerateHeader() 
{
	m_outBuff[0] = gzip_id1;
	m_outBuff[1] = gzip_id2;
	m_outBuff[2] = gzip_cm_deflate;
	m_outBuff[3] = 0;		// flags
	m_outBuff[4] = 0;		// mtime
	m_outBuff[5] = 0;		// mtime
	m_outBuff[6] = 0;		// mtime
	m_outBuff[7] = 0;		// mtime
	m_outBuff[8] = 0;		// xfl
	m_outBuff[9] = 255;		// os
	m_cOut		 = 0x0A;
}

void CGzipHandler::ProcessSource(BYTE *pv, DWORD cb)
{
	// Calcula o CRC do bloco...
	m_crc32 = crc32(m_crc32, pv, cb);
	// Incrementa o tamanho do buffer...
	m_totalSize += cb ;
}

int CGzipHandler::DeflateInit()
{
	static const int DEF_MEM_LEVEL = 8;
	return deflateInit2(&m_stream, m_compLevel, Z_DEFLATED, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY);
}

void CGzipHandler::GenerateTrailer()
{
	// Gravar o tamanho e o CRC do zip no final do buffer...
	m_outBuff[0] = (BYTE)(m_crc32       & 0xFF);
	m_outBuff[1] = (BYTE)(m_crc32 >> 8  & 0xFF);
	m_outBuff[2] = (BYTE)(m_crc32 >> 16 & 0xFF);
	m_outBuff[3] = (BYTE)(m_crc32 >> 24 & 0xFF);

	m_outBuff[4] = (BYTE)(m_totalSize       & 0xFF);
	m_outBuff[5] = (BYTE)(m_totalSize >> 8  & 0xFF);
	m_outBuff[6] = (BYTE)(m_totalSize >> 16 & 0xFF);
	m_outBuff[7] = (BYTE)(m_totalSize >> 24 & 0xFF);

	m_cOut = 8;
}

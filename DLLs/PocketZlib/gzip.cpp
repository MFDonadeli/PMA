/* zlib.h -- interface of the 'zlib' general purpose compression library
  version 1.1.4, March 11th, 2002

  Copyright (C) 1995-2002 Jean-loup Gailly and Mark Adler

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Jean-loup Gailly jloup@gzip.org
  Mark Adler madler@alumni.caltech.edu

*/

#include "StdAfx.h"
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
//#include <atlbase.h>
//#include <atlconv.h>
#include "src/zlib.h"  
#include "gzip.h"
//#include <io.h>
//#include <sys/types.h>
//#include <fcntl.h>
//#include <list>
//#include <utility>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

namespace zlib
{

bool CZipper::Open(EArchiveMode eArchiveMode)
{
	m_eArchiveMode = eArchiveMode;

	return true;
}


LPCTSTR CZipper::m_stsVersion=_T(ZLIB_VERSION);

CZipper::CZipper()
: m_eCompressionMode(CompressionModeDefault), 
m_eArchiveMode(ArchiveModeClosed),
m_eStrategy(StrategyDefault)
{

}

CZipper::~CZipper()
{
}

CGZip::~CGZip()
{
	if (IsOpen())
		Close();
}

bool CGZip::Open(LPCTSTR szFileName, EArchiveMode eArchiveMode)
{
	USES_CONVERSION;

	if (IsOpen())
		return false;

	if (eArchiveMode == ArchiveModeWrite)
	{
		m_gzf = gzopen(T2CA(szFileName),"wb");
		UpdateParams();
	}
	else if (eArchiveMode == ArchiveModeRead)
	{
		CFile f;
		if(!f.Open(szFileName, CFile::modeRead))
			return false;

		m_bufferSize = f.GetLength();
		f.Close();

		m_gzf = gzopen(T2CA(szFileName),"rb");
	}

	if (m_gzf != 0)
		m_eArchiveMode=eArchiveMode;
	else
		m_eArchiveMode=ArchiveModeClosed;

	return m_gzf != 0;
}
	
bool CGZip::Close()
{
	if (!IsOpen())
		return false;

	int result = gzclose(m_gzf);
	m_gzf=0;

	return result == 0;
};

bool CGZip::WriteBuffer( void* pBuffer, size_t nBytes)
{
	if (!IsOpen() || !IsWriting())
		return false;
	int written=gzwrite(m_gzf, pBuffer, nBytes);

	return written == (int)(nBytes);
};

bool CGZip::Flush( EFlushMode eFlush)
{
	if (!IsOpen() || !IsWriting())
		return false;

	return gzflush(m_gzf, eFlush)==Z_OK;
}
	
bool CGZip::WriteString( LPCTSTR str)
{
	return WriteBuffer( (void*)str, (_tcsclen(str)) * sizeof(TCHAR));
};

int CGZip::ReadBuffer( voidp* ppBuffer, size_t& nBytes)
{
	int read;
	nBytes = 0;

	if(!IsOpen() || !IsReading())
		return false;

	if(!m_bufferSize)
		return false;

	CArray<BYTE> lBuffers;
	BYTE* pBuffer = NULL;

	read = 1;
	while(read > 0)
	{
		pBuffer = new BYTE[(size_t)m_bufferSize];
		read = gzread(m_gzf, pBuffer, (size_t)m_bufferSize);
		if(read > 0)
		{
			for(int i = 0; i < read; i++)
				lBuffers.Add(pBuffer[i]);

			nBytes += read;
		}
		else
			delete [] pBuffer;
	}; 

	if (read == -1)
	{
		/*while (!lBuffers.empty())
		{
			delete[] lBuffers.front().first;
			lBuffers.pop_front();
		}*/

		lBuffers.RemoveAll();
		delete[] pBuffer;
		return false;
	}

	ASSERT(nBytes == lBuffers.GetCount());

	// allocating memory and writing buffer	
	*ppBuffer = new BYTE[nBytes];

	for(INT_PTR i = 0; i < lBuffers.GetCount(); i++)
	{
		((BYTE *)*ppBuffer)[i] = lBuffers[i];
	}

	lBuffers.RemoveAll();
	delete[] pBuffer;

	/*size_t offset=0;
	while (!lBuffers.empty())
	{
		pBuffer=lBuffers.front().first;
		read=lBuffers.front().second;
		memcpy((char*)*ppBuffer+offset, pBuffer, read);
		offset+=read;

		delete[] pBuffer;
		lBuffers.pop_front();
	}*/

	return nBytes != 0;
};

int CGZip::ReadString(LPTSTR* ppString)
{
	int read;
	size_t nBytes = 0;

	if (!IsOpen() || !IsReading())
		return false;

	if(!m_bufferSize)
		return false;

	//list< pair < char*,size_t > > lBuffers;
	CArray<BYTE> lBuffers;

	BYTE *pBuffer = NULL;

	read = 1;
	while(read > 0)
	{
		pBuffer = new BYTE[(size_t)m_bufferSize];
		read = gzread(m_gzf, pBuffer, (size_t)m_bufferSize);
		if(read > 0)
		{
			for(INT_PTR i = 0; i < read; i++)
				lBuffers.Add(pBuffer[i]);

			nBytes += read;
		}
		else
			delete [] pBuffer;
	};

	if(read == -1)
	{
		/*while(lBuffers.GetCount() > 0)
		{
			delete [] lBuffers.front().first;
			lBuffers.pop_front();
		}*/

		delete[] pBuffer;
		lBuffers.RemoveAll();
		return false;
	}

	// allocating memory and writing buffer	
	*ppString = new TCHAR[nBytes+1];
	//size_t offset = 0;
	
	ASSERT(nBytes == lBuffers.GetCount());

	for(INT_PTR i = 0; i < lBuffers.GetCount(); i++)
	{
		(*ppString)[i] = lBuffers[i];
	}

	delete[] pBuffer;
	lBuffers.RemoveAll();

	/*while(lBuffers.GetCount() > 0)
	{
		pBuffer = lBuffers.front().first;
		read = lBuffers.front().second;
		memcpy(((char*)*ppString)+offset, pBuffer, read);
		offset+=read;

		delete[] pBuffer;
		lBuffers.pop_front();
	}*/

	(*ppString)[nBytes] = '\0';

	return nBytes != 0;
}

int CGZip::ReadBufferSize( voidp pBuffer, size_t nBytes)
{
	if (!IsOpen() || !IsReading())
		return false;

	return gzread(m_gzf, pBuffer, nBytes);
}

void CGZip::UpdateParams()
{
	if (!IsOpen() || !IsWriting())
		return;

	gzsetparams( m_gzf, m_eCompressionMode, m_eStrategy);
};

bool CGZip::IsEOF() const
{
	if (!IsOpen())
		return true;

	return gzeof(m_gzf)==1;
}

};
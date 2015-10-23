#include "StdAfx.h"
#include "StreamReader.h"
#include "FormData.h"

CStringStreamReader::CStringStreamReader(void)
	: m_pos(0)
{
}

CStringStreamReader::~CStringStreamReader(void)
{
}

BOOL CStringStreamReader::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
	if(pv == NULL) return FALSE;
	if(pcbRead == NULL) return FALSE;

	*pcbRead = min(cb, m_buffer.GetLength() - m_pos);

	LPCSTR p = m_buffer.GetBuffer();
	memcpy(pv, &p[m_pos], *pcbRead);
	m_pos += *pcbRead;
	m_buffer.ReleaseBuffer();

	return (m_pos >= (ULONG)m_buffer.GetLength()) ? FALSE : TRUE;
}

void CStringStreamReader::Reset()
{
	m_pos = 0;
}

void CStringStreamReader::SetData(LPCTSTR szData)
{
	m_buffer = CStringA(szData);
}

void CStringStreamReader::SetData(LPCSTR szData)
{
	m_buffer = szData;
}

ULONG CStringStreamReader::GetSize()
{
	return m_buffer.GetLength();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

CByteStreamReader::CByteStreamReader(void)
	: m_pos(0)
{
}

CByteStreamReader::~CByteStreamReader(void)
{
}

BOOL CByteStreamReader::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
	if(pv == NULL) return FALSE;
	if(pcbRead == NULL) return FALSE;

	*pcbRead = min(cb, m_buffer.GetCount() - m_pos);

	memcpy(pv, &m_buffer.GetData()[m_pos], *pcbRead);
	m_pos += *pcbRead;

	return (m_pos >= (ULONG)m_buffer.GetCount()) ? FALSE : TRUE;
}

void CByteStreamReader::Reset()
{
	m_pos = 0;
}

ULONG CByteStreamReader::GetSize()
{
	return m_buffer.GetCount();
}

void CByteStreamReader::SetData(CArray<BYTE> &pdata)
{
	m_buffer.RemoveAll();
	for(int i = 0; i < pdata.GetCount(); ++i)
	{
		m_buffer.Add(pdata.GetAt(i));
	}
}

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
CFileStreamReader::CFileStreamReader(void)
	: m_pos(0)
{
}

CFileStreamReader::~CFileStreamReader(void)
{
	Close();
}

void CFileStreamReader::Close()
{
	if(m_file.m_hFile != NULL)
		m_file.Close();
}

BOOL CFileStreamReader::SetFileName(LPCTSTR szFilename)
{
	if(!m_file.Open(szFilename, CFile::modeRead|CFile::shareDenyNone, NULL))
	{
		Log(_T("Error openning file"));
		return FALSE;
	}
	return TRUE;
}

BOOL CFileStreamReader::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
	if(pv == NULL) return FALSE;
	if(pcbRead == NULL) return FALSE;

/*	*pcbRead = min(cb, m_buffer.GetCount() - m_pos);

	memcpy(pv, &m_buffer.GetData()[m_pos], *pcbRead);
	m_pos += *pcbRead;

	return (m_pos >= (ULONG)m_buffer.GetCount()) ? FALSE : TRUE;	*/

	*pcbRead = m_file.Read(pv, cb);

	return (*pcbRead) > 0;
}

void CFileStreamReader::Reset()
{
	m_pos = 0;
}

ULONG CFileStreamReader::GetSize()
{
	return (ULONG)m_file.GetLength();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
CFormStreamReader::CFormStreamReader(void)
	: m_pos(0)
{
}

CFormStreamReader::~CFormStreamReader(void)
{
}

void CFormStreamReader::ProcessForm(CFormData *__pForm)
{
	m_pForm = __pForm;
	m_pForm->ProcessForm();
}

BOOL CFormStreamReader::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
	if(pv == NULL) return FALSE;
	if(pcbRead == NULL) return FALSE;

	CArray<BYTE, BYTE> *pBuffer = m_pForm->GetBuffer();

	*pcbRead = min(cb, pBuffer->GetCount() - m_pos);

	memcpy(pv, &pBuffer->GetData()[m_pos], *pcbRead);
	m_pos += *pcbRead;

	return (m_pos >= (ULONG)pBuffer->GetCount()) ? FALSE : TRUE;
}

void CFormStreamReader::Reset()
{
	m_pos = 0;
}

ULONG CFormStreamReader::GetSize()
{
	return m_pForm->GetSize();
}

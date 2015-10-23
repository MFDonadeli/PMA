// Copyright (c) 2007 Marcos Mori de Siqueira. All Rights Reserved. 
// mori@softfactory.com.br
// Classe que representa os parametros para a execucao dos modulos,
// sao enviados e recebidos pelos modulos para trocar dados.
#include "stdafx.h"
#include "ModParam.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

UINT CModParam::WM_MODULE_MESSAGE = 
		RegisterWindowMessage(_T("CModParam::WM_MODULE_MESSAGE"));

UINT CModParam::WM_MODULE_READY = 
		RegisterWindowMessage(_T("CModParam::WM_MODULE_READY"));

UINT CModParam::WM_BACKUP_REQUEST = 
		RegisterWindowMessage(_T("CModParam::WM_BACKUP_REQUEST"));

UINT CModParam::WM_RETORNO_LOGIN = 
		RegisterWindowMessage(_T("CModParam::WM_RETORNO_LOGIN"));

UINT CModParam::WM_MODULE_EXECUTE =
		RegisterWindowMessage(_T("CModParam::WM_MODULE_EXECUTE"));

UINT CModParam::WM_HTTPSENDER_START =
		RegisterWindowMessage(_T("CModParam::WM_HTTPSENDER_START"));

UINT CModParam::WM_HTTPSENDER_STOP =
		RegisterWindowMessage(_T("CModParam::WM_HTTPSENDER_STOP"));

UINT CModParam::WM_CHANGE_PROXY =
		RegisterWindowMessage(_T("CModParam::WM_CHANGE_PROXY"));

UINT CModParam::WM_UPDATE_BANNER = 
		RegisterWindowMessage(_T("CModParam::WM_UPDATE_BANNER"));

UINT CModParam::WM_SINCRO_BACKUP = 
		RegisterWindowMessage(_T("CModParam::WM_SINCRO_BACKUP"));

CModParam::CModParam()
{
	m_hInst = NULL;
	m_hwnd  = NULL;
	m_buffer = NULL;
	m_bDelBuffer = TRUE;
}

CModParam::~CModParam()
{
	//STLOG_WRITE("%s(%d): Apagando: %x\r\n", __FUNCTION__, __LINE__, m_buffer);
	//Comentado para ver o que dá...
	/*
	if(m_buffer != NULL && m_bDelBuffer) 
	{
		TRACE("Ia Deletando: %x\r\n", m_buffer);
		delete [] m_buffer;
		m_buffer = NULL;
	}*/
}

void CModParam::Serialize(CArchive &ar)
{
	CObject::Serialize(ar);

	CString s;
	CString sKey;

    if(ar.IsStoring())
	{
		ar << (long) m_hInst;
		ar << (long) m_hwnd;

		ar << m_map.GetCount();

		POSITION p = m_map.GetStartPosition();
		while(p)
		{
			m_map.GetNextAssoc(p, sKey, s); 
			ar << sKey;
			ar << s;
		}
	}
    else
	{
		long value;

		ar >> value;
		m_hInst = (HINSTANCE) value;

		ar >> value;
		m_hwnd = (HWND) value;

		ar >> value;

		for(int i = 0; i < value; i++)
		{
			ar >> sKey;
			ar >> s;
			m_map.SetAt(sKey, s);
		}
	}
}

LPBYTE CModParam::GetBuffer(LONG *size, BOOL bDelBuffer)
{
	m_bDelBuffer = bDelBuffer;

	CMemFile mf;
	CArchive ar(&mf, CArchive::store);
	Serialize(ar);
	ar.Flush();

	mf.Seek(0, CFile::begin);
	
	if(m_buffer != NULL)
	{
		STLOG_WRITE("%s(%d): Apagando: %x\r\n", __FUNCTION__, __LINE__, m_buffer);
		delete [] m_buffer;
		m_buffer = NULL;
	}

	*size = 0;

	int x = (LONG)mf.GetLength();
	if(x > 0)
	{
		m_buffer = new BYTE[x];
		STLOG_WRITE("%s(%d): Criado: %x\r\n", __FUNCTION__, __LINE__, m_buffer);

		memset(m_buffer, 0, sizeof(m_buffer));

		if(!mf.Read(m_buffer, x))
		{
			delete [] m_buffer;
			m_buffer = NULL;
			return NULL;
		}

		*size = x;
	}

	return m_buffer;
}

BOOL CModParam::SetBuffer(LPBYTE buffer, int count)
{
	CMemFile mf;

	mf.Write(buffer, count);
	mf.Flush();
	mf.Seek(0, CFile::begin);

	CArchive ar(&mf, CArchive::load);
	Serialize(ar);

	return TRUE;
}

void CModParam::ReleaseBuffer()
{
	STLOG_WRITE("%s(%d): Apagando: %x\r\n", __FUNCTION__, __LINE__, m_buffer);
	delete [] m_buffer;
	m_buffer = NULL;
}

void CModParam::SetHInstance(HINSTANCE hInst)
{
	m_hInst = hInst;
}

HINSTANCE CModParam::GetHInstance()
{
	return m_hInst;
}

void CModParam::SetHwnd(CWnd *pWnd)
{
	m_hwnd = pWnd->GetSafeHwnd();
}

void CModParam::SetHwnd(HWND hWnd)
{
	m_hwnd = hWnd;
}

HWND CModParam::GetHwnd()
{
	return m_hwnd;
}

void CModParam::AddPair(LPCTSTR szKey, LPCTSTR szValue)
{
	m_map.SetAt(szKey, szValue);
}

CString CModParam::GetValue(LPCTSTR szKey)
{
	CString s;
	m_map.Lookup(szKey, s);
	return s;
}

void CModParam::HideParent()
{
	if(GetHwnd())
	{
		CWnd *pWnd = CWnd::FromHandle(GetHwnd());
		if(pWnd && pWnd->IsWindowVisible())
			pWnd->ShowWindow(SW_HIDE);
	}
}

void CModParam::ShowParent()
{
	if(GetHwnd())
	{
		CWnd *pWnd = CWnd::FromHandle(GetHwnd());
		if(pWnd)
		{
			pWnd->ShowWindow(SW_SHOW);
			pWnd->SetForegroundWindow();
		}
	}
}

BOOL CModParam::IsParentVisible()
{
	if(GetHwnd())
	{
		CWnd *pWnd = CWnd::FromHandle(GetHwnd());
		if(pWnd)
		{
			return pWnd->IsWindowVisible();
		}
	}

	return FALSE;
}

BOOL CModParam::SetValue(LPCTSTR szKey, LPCTSTR szValue)
{
	CString s;
	if(m_map.Lookup(szKey, s))
	{
		m_map.SetAt(szKey, szValue);
		return TRUE;
	}

	return FALSE;
}

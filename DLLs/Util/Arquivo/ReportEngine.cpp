#include "StdAfx.h"
#include "ReportEngine.h"
#include <fstream>
#include <string>
#include "Utils.h"

using namespace std;

CReportEngine::CReportEngine(void)
{
}

CReportEngine::~CReportEngine(void)
{
}

void CReportEngine::AddItem(LPCTSTR szLabel, LPCTSTR szValue)
{
//	TRACE(L"ADD de %s - %s\n", szLabel, szValue);
	m_items.SetAt(szLabel, szValue);
}

void CReportEngine::SetItens(__REITEMMAP *mapItens)
{
	POSITION p;
	p = mapItens->GetStartPosition();
	CString sLabel, sValue;

	while(p)
	{
		mapItens->GetNextAssoc(p, sLabel, sValue);
		AddItem(sLabel, sValue);
	}

	//memcpy(&m_items, mapItens, sizeof(__REITEMMAP));
}

BOOL CReportEngine::Open(LPCTSTR szTempl)
{
	ifstream arq(szTempl);
	string line;
	m_lines.RemoveAll();
	while(!arq.eof())
	{
		getline(arq, line);
		m_lines.AddTail(CString(line.c_str()));
	}

	return TRUE;
}

BOOL CReportEngine::ProcessFileLine(LPCSTR szBuffer)
{
	m_lines.AddTail(CString(szBuffer));
	return TRUE;
}

void CReportEngine::GetArray(CStringArray &sArr)
{ 
	for(int i=0; i<m_sProcessedArray.GetCount(); i++)
	{
		sArr.Add(m_sProcessedArray[i]);
	}
}

BOOL CReportEngine::ProcessTemplate(CString &sBuffer)
{
	sBuffer.Empty();
	m_sProcessedArray.RemoveAll();

	POSITION pos = m_items.GetStartPosition();
	CString sKey, sVal;
	while(pos)
	{
		m_items.GetNextAssoc(pos, sKey, sVal);
		OutputDebugString(sKey + L" -> " + sVal + L"\r\n");
	}

	POSITION p = m_lines.GetHeadPosition();
	while(p)
	{
		CString s = m_lines.GetNext(p);	

		OutputDebugString(L"O que há agora: " + s + L"\r\n");
		
		int pos1, pos2;
		if((pos1 = s.Find('['))>=0)
		{
			pos2 = s.Find(']');
		}
		else if((pos1 = s.Find('&'))>=0)
		{
			pos2 = s.Find('&', pos1+1);
		}
		else
		{
			sBuffer += s;
			sBuffer += L"\r\n";
			m_sProcessedArray.Add(s);
			continue;
		}

		if(pos2 < 0)
		{
			STLOG_WRITE("Template error: %s", s);
			return FALSE;
		}

		CString sLbl = s.Mid(pos1+1, pos2 - (pos1+1));
		CString sValue;
		CString sLine;

		if(m_items.Lookup(sLbl, sValue))
		{
//			TRACE(L"Substituindo %s\n", sLbl);
			if(!sValue.Trim().IsEmpty())
			{
				sLine  = s.Mid(0, pos1);
				sLine += sValue;
				sLine += s.Mid(pos2+1);

				//no template de impressão ## é igual a quebra de linha
				sLine.Replace(L"##",L"\r\n");	

				m_sProcessedArray.Add(sLine);

				sBuffer += sLine;
				sBuffer += L"\r\n";


			}
		}
		else
		{
//			TRACE(L"Substituindo %s - NAO ENCONTROU\n", sLbl);
			STLOG_WRITE("Template error: field nao encontrado %S", sLbl);
			sBuffer += s;
			m_sProcessedArray.Add(s);
			sBuffer += L"\r\n";
		}
	}

	return TRUE;
}

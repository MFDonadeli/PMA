#include "stdafx.h"
#include "EditEx.h"
#include "Utils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

IMPLEMENT_DYNAMIC(CEditEx, CEdit)

/**
\brief
	Construtor da classe
\details
	Funções executadas neste método:
	- Definir o tipo da caixa de texto
*/
CEditEx::CEditEx()
{
	m_nType = EDT_TYPE_DEFAULT;
}

/**
\brief
	Destrutor da classe
*/
CEditEx::~CEditEx()
{
}

BEGIN_MESSAGE_MAP(CEditEx, CEdit)
	ON_CONTROL_REFLECT(EN_CHANGE, &CEditEx::OnEnChange)
	ON_WM_CHAR()
END_MESSAGE_MAP()

void CEditEx::OnEnChange()
{

}

/**
\brief
	Função executada ao ser preenchido uma letra dentro da caixa de texto
\details
	Funções executadas neste método:
	- Validar a entrada para cada tipo de caixa de texto
\param
	UINT nChar: Caracter digitado
\param
	UINT nRepCnt: Contador de repetições
\param
	UINT nFlags: Flag de alteração/informações sobre o caracter digitado
\return
	Não processa a digitação caso não atenda à regra específica
*/
void CEditEx::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if(nChar != VK_BACK && nChar != VK_RETURN)
	{
		if(m_nType == EDT_TYPE_CHARONLY)
		{
			if(_istdigit(nChar))
			{
				MessageBeep(MB_ICONERROR);
				return;
			}
		}
		else if(m_nType == EDT_TYPE_NUMBERONLY)
		{
			if(!_istdigit(nChar))
			{
				MessageBeep(MB_ICONERROR);
				return;
			}
		}
		else if(m_nType == EDT_TYPE_ALPHANUM)
		{
			if(!_istalnum(nChar) || nChar > 'z')
			{
				MessageBeep(MB_ICONERROR);
				return;
			}
		}
		else if(m_nType == EDT_TYPE_FLOAT)
		{
			int count=0;

			CString s;
			GetWindowText(s);

			for(int i=0; i<s.GetLength(); i++)
			{
				int start, end;
				GetSel(start, end);
				s.Replace(s.Mid(start, end-start), L"");

				if(s.GetAt(i) == ',')
					s.SetAt(i,'.');

				if(s.GetAt(i) == '.')
					count++;
			}

			if(nChar == ',')
				nChar = '.';

			if((!_istdigit(nChar) && nChar != '.') || (nChar == '.' && count!=0))
			{
				MessageBeep(MB_ICONERROR);
				return;
			}
		}
	}

	CEdit::OnChar(nChar, nRepCnt, nFlags);
}

void CEditEx::SetFontSize(const int nSize)
{
	LOGFONT lf;

	CFont *font = GetFont();
	font->GetLogFont(&lf);

	lf.lfHeight = nSize;

	m_ft.CreateFontIndirect(&lf);

	SetFont(&m_ft);
}

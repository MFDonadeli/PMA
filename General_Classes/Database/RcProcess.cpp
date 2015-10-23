#include "StdAfx.h"
#include "RcProcess.h"
#include "CStr.h"
#include "Consultas.h"

/**
\brief Construtor da classe
\param void
\return void
*/
CRcProcess::CRcProcess(void)
{
}

/**
\brief Destrutor da classe
\param void
\return void
*/
CRcProcess::~CRcProcess(void)
{
}


/**
\brief Valida campos obrigatórios onde 'obrigatorio' = 1 da tabela RES 
\param CString &sErrMsg: Mensagem retornada pela validação
\param int iGroupBegin: Grupo inicial
\param int iGroupEnd: Grupo final
\return int Retorna o id do controle não validado ou -1 se todos os campos foram validados
*/
int CRcProcess::_ValidateObrigatorios(CString &sErrMsg, int iGroupBegin, int iGroupEnd)
{
	CStringA sQuery;
	CppSQLite3Query q;
	CString sVal;
	CTelaAtual* pTa = NULL;

	sQuery.Format("SELECT * FROM RES WHERE obrigatorio = 1 AND pagina BETWEEN %d AND %d", iGroupBegin, iGroupEnd);

	try
	{
		q = m_pDb->execQuery(sQuery);
		while(!q.eof())
		{
			sVal.Format(L"%S.%S", q.getStringField("DBRef"), q.getStringField("DBFieldRef"));
			sVal = _GetfromTable(sVal);

			#ifdef _WIN32_WCE
				//CRcGen::mapTelaAtual.Lookup(CString(q.getStringField("IDStr")), pTa);

				if(sVal.IsEmpty() /*&& pTa*/)			
				{
					sErrMsg.Format(L"Campo %S é obrigatório", q.getStringField("Nome"));
					return q.getIntField("ID");
				}
			#else
				if(sVal.IsEmpty())			
				{
					sErrMsg.Format(L"Campo %S é obrigatório", q.getStringField("Nome"));
					return q.getIntField("ID");
				}
			#endif

			q.nextRow();
		}
	}
	catch(CppSQLite3Exception e)
	{		
	}

	return -1;
}


/**
\brief Alimenta combo com os estados do Brasil
\param CEGBComboBox *pCb: Ponteiro para o combo que será alimentado
\return void
*/
void CRcProcess::_FillUF(CEGBComboBox *pCb)
{
	pCb->AddString(L"AC");
	pCb->AddString(L"AL");
	pCb->AddString(L"AM");
	pCb->AddString(L"AP");
	pCb->AddString(L"BA");
	pCb->AddString(L"CE");
	pCb->AddString(L"DF");
	pCb->AddString(L"ES");
	pCb->AddString(L"GO");
	pCb->AddString(L"MA");
	pCb->AddString(L"MG");
	pCb->AddString(L"MS");
	pCb->AddString(L"MT");
	pCb->AddString(L"PA");
	pCb->AddString(L"PB");
	pCb->AddString(L"PE");
	pCb->AddString(L"PI");
	pCb->AddString(L"PR");
	pCb->AddString(L"RJ");
	pCb->AddString(L"RN");
	pCb->AddString(L"RO");
	pCb->AddString(L"RR");
	pCb->AddString(L"RS");
	pCb->AddString(L"SC");
	pCb->AddString(L"SE");
	pCb->AddString(L"SP");
	pCb->AddString(L"TO");
}

/**
\brief Inicia tabelas de utilização dos cadastros. Alimenta m_MapTables
\details m_MapTables é o array de tabelas utilizadas no sistema
\return void
\todo Tratamento da exceção
*/
void CRcProcess::_IniciaTabelas()
{
	CStringA sQuery = "";
	CString sVal = L"";
	sQuery = "select count(distinct(DBRef)) from res where dbref != ''";

	if(m_MapTables.GetSize()>0)
		return;

	int nCount;

	CppSQLite3Query q;

	try
	{
		q = m_pDb->execQuery(sQuery);

		if(!q.eof())
		{
			nCount = q.getIntField(0);
		}

		sQuery = "select distinct(DBRef) from res where dbref != ''";

		q = m_pDb->execQuery(sQuery);
		for(int i=0; i<nCount; i++)
		{
			sVal = q.getStringField(0);
			if(sVal.CompareNoCase(L"ARV")==0)
			{
				m_pTable = new CTable();
				m_pTable->Init(sVal);

				m_MapTables.SetAt(sVal, m_pTable);
			}
			else
			{
				m_pTable = new CTable();
				m_pTable->Init(sVal);

				m_MapTables.SetAt(sVal, m_pTable);
			}
			q.nextRow();
		}
	}
	catch(CppSQLite3Exception e)
	{
		//TODO
	}
}

/**
\brief Recupera valor de um determinado campo salvo na estrutua CTable
\param const CString& sDbRef: Campo SQL no formato tabela.campo. Campo DBRef da tabela RES
\return CString Retorna valor do campo
*/
CString CRcProcess::_GetfromTable(const CString& sDbRef)
{
	CString sVal;

	if(sDbRef.CompareNoCase(L".")==0)
		return L"";

	m_MapTables.Lookup(sDbRef.Left(sDbRef.Find(L".")), m_pTable);
	sVal = sDbRef.Right(sDbRef.GetLength()-1 - sDbRef.Find(L"."));

	return m_pTable->GetStringValue(sVal);

}

/**
\brief Alimenta a estrutura CTable com o valor do campo passado
\param const CString& sDbRef: Campo SQL no formato tabela.campo. Campo DBRef da tabela RES
\param LPCTSTR szVal Valor a ser salvo
\return void
*/
void CRcProcess::_SaveinTable(const CString& sDbRef, LPCTSTR szVal)
{
	CString sIDField;

	m_MapTables.Lookup(sDbRef.Left(sDbRef.Find(L".")), m_pTable);
	sIDField = sDbRef.Right(sDbRef.GetLength()-1 - sDbRef.Find(L"."));	

	if(!sIDField.IsEmpty())
	{
		//TRACE(L"CRcProcess::_SaveinTable() %s = %s \r\n", sIDField, szVal);
		m_pTable->SetValue(sIDField, szVal);
	}
}


/**
\brief Recupera valor de um controle especifico e executa _SaveinTable() que alimenta a estrutura CTable
\param cControles* ctrl: Ponteiro de array dos controles com informações oriundas da tabela RES
\param CTelaAtual* pTa: Ponteiro de array dos controles de uma determinada tela
\return void
*/
void CRcProcess::SaveValues(cControles* ctrl, CTelaAtual* pTa)
{
	CString sVal;

	CEGBComboBox* egbcombo;
	CButton* button;
	CDateTimeCtrl* dateTimeCtrl;
	SYSTEMTIME st;
	
	switch(pTa->tipoCtrl)
	{
		case COMBO:
		case COMBO_EDITAVEL:		
			egbcombo = (CEGBComboBox*)pTa->wndControl;
			if(ctrl->sBDValue.CompareNoCase(L"ID")==0)
			{
				long lData = egbcombo->GetSelectedCode();
				if(lData != -1)
					sVal.Format(L"%ld", lData);					
			}
			else if(ctrl->sBDValue.CompareNoCase(L"Value")==0)
				pTa->wndControl->GetWindowText(sVal);				
		break;			

		case RADIO:
			button = (CButton*)pTa->wndControl;
			if(button->GetCheck() == TRUE)
				sVal = ctrl->sBDValue;
		break;			

		case CHECK:
			button = (CButton*)pTa->wndControl;
			if(button->GetCheck() == TRUE)
				sVal = L"1";
			else
				sVal = L"0";
		break;

		case DATA:
			dateTimeCtrl = (CDateTimeCtrl*)pTa->wndControl;			
			dateTimeCtrl->GetWindowText(sVal);			
			dateTimeCtrl->GetTime(&st);
			sVal.Format(L"%d-%02d-%02d", st.wYear, st.wMonth, st.wDay);			
		break;

		case HORA:
			dateTimeCtrl = (CDateTimeCtrl*)pTa->wndControl;
			if(dateTimeCtrl->GetTime(&st)!=GDT_NONE)
				pTa->wndControl->GetWindowText(sVal);									
		break;

		default:
			pTa->wndControl->GetWindowText(sVal);			
		break;
	}

	if(!sVal.IsEmpty())
		_SaveinTable(ctrl->sDBRef, sVal);
}




/**
\brief Realiza o processo de inicialização padrão dos controles segundo seu tipo
\details Recupera os valores digitados pelo usuário
\param cControles* ctrl: Ponteiro para o controle a ser iniciado
\return void
*/
void CRcProcess::_DoInitializationProcess(cControles* ctrl)
{
	CTelaAtual* pTa = NULL, *pTaTemp = NULL;
	CString sVal;
	int iItemData;

	CRcGen::mapTelaAtual.Lookup(ctrl->sID, pTa);

	if(!pTa)
		return;

	sVal = _GetfromTable(ctrl->sDBRef);
	switch(pTa->tipoCtrl)
	{
		case TEXTO:
		case TEXTO_MULTILINHA:
			if(!sVal.IsEmpty())
				pTa->wndControl->SetWindowText(sVal);	
			CEGBEdit* edit;
			edit = (CEGBEdit*)pTa->wndControl;
			if(ctrl->iSizeDb!=0)
			{
				if(ctrl->iSizeDb < 0)
				{
					edit->SetLimitText(ctrl->iSizeDb * -1);
					edit->SetType(CEditEx::EDT_TYPE_NUMBERONLY);
				}
				else
				{
					edit->SetLimitText(ctrl->iSizeDb);
					edit->SetType(CEditEx::EDT_TYPE_ALPHANUM);
				}
			}
			break;

		case RADIO:				
			if(sVal.CompareNoCase(ctrl->sBDValue)==0)
			{
				CButton* button = (CButton*)pTa->wndControl;
				button->SetCheck(TRUE);
			}
			break;

		case CHECK:
			if(sVal.CompareNoCase(L"1")==0)
			{
				CButton* button = (CButton*)pTa->wndControl;
				button->SetCheck(TRUE);
			}
			else
			{
				CButton* button = (CButton*)pTa->wndControl;
				button->SetCheck(FALSE);
			}
			break;

		case COMBO:			
			if(!sVal.IsEmpty())
			{
				iItemData = atoi(CStr(sVal));
				if(iItemData != -1)
				{
					CEGBComboBox* cmbCtrl = (CEGBComboBox*)pTa->wndControl;				

					for(int i = 0; i < cmbCtrl->GetCount(); i++)
					{
						long value = (long) cmbCtrl->GetItemData(i);
						if(value == iItemData)
						{
							cmbCtrl->SetCurSel(i);
							break;
						}
					}
				}
			}			
			break;	

		case COMBO_EDITAVEL:
			if(!sVal.IsEmpty())
			{
				CEGBComboBox* cmbCtrl = (CEGBComboBox*)pTa->wndControl;				
				cmbCtrl->Search(sVal);
			}
			break;	

		case DATA:
		case HORA:
			if(!sVal.IsEmpty())
			{				
				COleDateTime dt;
				dt.ParseDateTime(sVal, LOCALE_NOUSEROVERRIDE); //Ignora Time Zone		
				CDateTimeCtrl* dtCtrl = (CDateTimeCtrl*)pTa->wndControl;
				dtCtrl->SetTime(dt);
			}	
			break;	

		default:
			break;
	}	
}


/**
\brief Método que habilita um controle, geralmente um textbox, quando uma opção específica de um combo box é selecionada
\param LPCTSTR szCtrlMainName Nome do controle principal, no caso o combo box
\param LPCTSTR szCrtlSecName Nome do controle que sofrerá modificação, geralmente um text box
\param LPCTSTR szQuery String que a opção selecionada deve conter para disparar ação no szCrtlSecName
\param int iQuery Id que a opção selecionada deve ser para disparar ação no szCrtlSecName. Default -1
\return void
*/
void CRcProcess::OnCmbOptSelEnableCtrl(LPCTSTR szCtrlMainName, LPCTSTR szCrtlSecName, LPCTSTR szQuery, int iQuery)
{
	cControles ctrl;
	CTelaAtual *pTa, *pTa1;
	CString sVal;

	CRcGen::mapTelaAtual.Lookup(szCtrlMainName, pTa);
	if(pTa)
	{
		CRcGen::mapTelaAtual.Lookup(szCrtlSecName, pTa1);

		if(iQuery == -1)
		{
			pTa->wndControl->GetWindowText(sVal);									
		
			if(sVal.CompareNoCase(szQuery) == 0)
			{				
				pTa1->wndControl->EnableWindow(TRUE);								
			}
			else
			{
				pTa1->wndControl->EnableWindow(FALSE);
				pTa1->wndControl->SetWindowText(L"");				
			}
		}
		else
		{
			CComboBox* cmbCtrl;
			cmbCtrl = (CComboBox*)pTa->wndControl;
			int idx = cmbCtrl->GetCurSel();			 

			if(idx != CB_ERR)
			{				
				if(idx == iQuery)
				{
					pTa1->wndControl->EnableWindow(TRUE);
				}
				else
				{
					pTa1->wndControl->EnableWindow(FALSE);
					pTa1->wndControl->SetWindowText(L"");
				}
			}			
		}
	}
}

/**
\brief Método que torna um campo obrigatório se um outro campo estiver diferente de vazio
\param LPCTSTR szCtrlMainName Nome do controle principal
\param LPCTSTR szCrtlSecName Nome do controle que sofrerá a validação
\param LPCTSTR sErrMsg Mensagem da validação exibida para o usuário
\param CString szQuery String que a opção selecionada deve conter para disparar ação no szCrtlSecName. Default empty.
\return int nID do controle. Caso validação ok retorna -1. Caso erro retorna 0. 
*/
int CRcProcess::ValidateIfFilled(LPCTSTR szCtrlMainName, LPCTSTR szCrtlSecName, CString &sErrMsg, CString szQuery)
{	
	int iCtrlId, iCtrlId2;
	CString sVal, sNomeCtrl, sDBRef;
	CString sVal2,sNomeCtrl2, sDBRef2;

	if(!CConsultas::GetDBRefByIdStr(m_pDb, szCtrlMainName, iCtrlId, sNomeCtrl, sDBRef))
		return 0;

	sVal = _GetfromTable(sDBRef);

	if(!CConsultas::GetDBRefByIdStr(m_pDb, szCrtlSecName, iCtrlId2, sNomeCtrl2, sDBRef2))
		return 0;

	sVal2 = _GetfromTable(sDBRef2);

	if(szQuery.IsEmpty())
	{
		if(!sVal.IsEmpty() && sVal2.IsEmpty())
		{
			sErrMsg.Format(L"Campo %s é obrigatório", sNomeCtrl2);
			return iCtrlId2;
		}
		else
			return -1;
	}
	else
	{
		if(!szQuery.IsEmpty())
		{
			if(sVal.CompareNoCase(szQuery) == 0)
			{	
				if(sVal2.IsEmpty())
				{
					sErrMsg.Format(L"Campo %s é obrigatório", sNomeCtrl2);
					return iCtrlId2;
				}
				else
					return -1;				
			}	
		}	
	}

	return 0;


	/*cControles ctrl;
	CTelaAtual *pTa, *pTa1;
	CString sVal, sVal2;

	CRcGen::mapTelaAtual.Lookup(szCtrlMainName, pTa);
	if(!pTa)
		return 0;

	CRcGen::mapTelaAtual.Lookup(szCrtlSecName, pTa1);
	if(!pTa1)
		return 0;


	if(pTa->tipoCtrl == TEXTO && pTa1->tipoCtrl == TEXTO)
	{
		pTa->wndControl->GetWindowText(sVal);			
		pTa1->wndControl->GetWindowText(sVal2);
		
		if(!sVal.IsEmpty() && sVal2.IsEmpty())
		{
			ctrl = CRcGen::arrControles[pTa1->nIdCtrlArray];

			sErrMsg.Format(L"Campo %s é obrigatório", ctrl.sNome);
			return ctrl.nID;
		}
		else
			return -1;
	}
	else if(pTa->tipoCtrl == COMBO && pTa1->tipoCtrl == TEXTO)
	{	
		CComboBox* cmbCtrl;
		cmbCtrl = (CComboBox*)pTa->wndControl;
		int idx = cmbCtrl->GetCurSel();		

		pTa1->wndControl->GetWindowText(sVal2);

		if(iQuery == -1)
		{
			pTa->wndControl->GetWindowText(sVal);									
		
			if(sVal.CompareNoCase(szQuery) == 0)
			{	
				if(sVal2.IsEmpty())
				{
					ctrl = CRcGen::arrControles[pTa1->nIdCtrlArray];

					sErrMsg.Format(L"Campo %s é obrigatório", ctrl.sNome);
					return ctrl.nID;
				}
				else
					return -1;				
			}			
		}
		else if(idx != CB_ERR)
		{				
			if(idx == iQuery)
			{
				if(sVal2.IsEmpty())
				{
					ctrl = CRcGen::arrControles[pTa1->nIdCtrlArray];

					sErrMsg.Format(L"Campo %s é obrigatório", ctrl.sNome);
					return ctrl.nID;
				}
				else
					return -1;
			}
		}	
	}

	return 0;*/
}
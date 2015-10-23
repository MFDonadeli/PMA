#include "StdAfx.h"
#include "PMA.h"
#include "HttpSenderThread.h"
#include "utils.h"
#include "MsgWindow.h"
#include "registry.h"
#include "Table.h"
//#include "Blowfish.h"

///N�mero de tentativas v�lidas antes de baixar o tempo de envio
#define LIMITE_TX_COM_FALHA  3

CEvent			CHttpSenderThread::m_evStop;
CppSQLite3DB*	CHttpSenderThread::m_pDB;
int				CHttpSenderThread::iNumTentativaComErro = 0;
int				CHttpSenderThread::iTimeExecThread;


/**
\brief Construtor da classe
\details Fun��es executadas neste m�dulo:
\details Inicia as vari�veis globais desta classe
\param void
*/
CHttpSenderThread::CHttpSenderThread(void)
{
	HttpSenderDados = new CHttpSenderThreadDados();
	
	//Padr�o � gravar logs na tabela httpsender_log
	HttpSenderDados->fRecordLog = TRUE;
	
	//Padr�o � n�o deletar arquivos enviados
	HttpSenderDados->fDeleteFileAfterSend = FALSE;

	//Padr�o � n�o atualizar hora a cada envio
	HttpSenderDados->m_sURLRelogio = L"";

	//Ainda n�o executando...
	bRunning = false;	
	
	//InitializeCriticalSection(&m_SendLock);
	//InitializeCriticalSection(&CUtil::m_uSendLock);
}

/**
\brief Destrutor da classe
*/
CHttpSenderThread::~CHttpSenderThread(void)
{
	delete HttpSenderDados;
}

/**
\brief Cria thread que monitora um determinado tipo de objeto a ser enviado
\details Atualiza status de jobs de T (transmitindo) para N (novo) se existir.
\param LPCTSTR szThreadObjName Nome do objeto a ser monitorado (XML_GPS, FOTO etc..)
\param CppSQLite3DB *pDB Ponteiro para banco de dados
\param LPCTSTR szURLDestino URL a ser enviado arquivo
\param int iTimeExec Intervalo de execu��o da thread
\return TRUE se a thread for criada com sucesso.
*/
BOOL CHttpSenderThread::CreateHttpSenderThread(LPCTSTR szThreadObjName, CppSQLite3DB *pDB, LPCTSTR szURLDestino, int iTimeExecSegundos)
{
	//Carrega dados (do pma.xml) espec�ficos da thread...
	m_pDB = pDB;
	HttpSenderDados->sThreadName = szThreadObjName;	
	HttpSenderDados->sURLDestino = szURLDestino;
	HttpSenderDados->iTimeExecNormal = iTimeExecSegundos * 1000;
	HttpSenderDados->iTimeExecLento  = iTimeExecSegundos * 1000 * 2;

////CString sTT;
////sTT.Format(L"ThreadId: [%x] ThreadName: [%s] Time [%d] seg. CreateHttpSenderThread(): Criando Thread.", GetCurrentThreadId() ,szThreadObjName,iTimeExecSegundos);
////STLOG_WRITE(sTT);


	//Configura tempo de execu��o normal...
	///iTimeExecThread = HttpSenderDados->iTimeExecNormal;
	HttpSenderDados->iTimeExecThread = HttpSenderDados->iTimeExecNormal;

	//Pesquisa se na tabela httpsender_job, algum registro permaneceu com status=T (transmitindo) e renomeia p/ status=N (novo)
	if(!CConsultasHttpSender::UpdateJobStatus(m_pDB, szThreadObjName, L"T", L"N"))
	{
		STLOG_WRITE(L"[%s] CHttpSenderThread::CreateHttpSenderThread() Erro atualizando jobs status de T p/ N", szThreadObjName);
	}

	//Apaga os jobs j� enviados e que ainda n�o foram apagados
	_DeleteNotDeletedSentJobs();
	
	//Cria thread
	m_hThread = AfxBeginThread(_HttpSenderThreadProc, HttpSenderDados);
	
	if(m_hThread == NULL)
	{
		STLOG_WRITE(L"[%s] CHttpSenderThread::CreateHttpSenderThread() Erro criando thread do HttpSender [%s]", HttpSenderDados->sThreadName);
		return FALSE;
	}

	_SetThreadInfo(HttpSenderDados->sThreadName, HttpSenderDados->iTimeExecThread, GetCurrentThreadId(), m_hThread, CUtil::GetCurrentDateTime(L"DATA_HORA"), L"");
	
	STLOG_WRITE(L"[%s] CHttpSenderThread::CreateHttpSenderThread(): === Iniciada thread de envio httpsender ===", HttpSenderDados->sThreadName);
	
	//Thread em execu��o...
	bRunning = true;
	return TRUE;
}

/**
\brief Destr�i o monitoramento criado por CreateHttpSenderThread.
\details Se o monitoramento n�o for destru�do com sucesso somente ser� registrado no log.
\param void
\return void. 
*/
void CHttpSenderThread::DestroyHttpSenderThread()
{
	if (m_hThread)
    {
        CloseHandle(m_hThread);
		m_evStop.SetEvent();
        m_hThread = NULL;
		STLOG_WRITE(L"[%s] CHttpSenderThread::DestroyHttpSenderThread(): === Encerrada thread de envio httpsender ===", HttpSenderDados->sThreadName);
    }

	bRunning = false;	
}



/**
\brief Executa todo o processamento dos job de envio de arquivos.
\param LPVOID lpParameter Objeto HttpSenderDados referente aos dados espec�ficos da thread
\return UINT
*/
UINT CHttpSenderThread::_HttpSenderThreadProc(LPVOID lpParameter)
{
	CHttpSenderThreadDados *p_HttpSenderDados = reinterpret_cast<CHttpSenderThreadDados*>(lpParameter);	
	ZeroMemory(&p_HttpSenderDados->st, sizeof(SYSTEMTIME));
	
	//Lista de jobs ainda n�o enviados pela thread HttpSender. Filtrado por nome do objeto (sObjName)
	__CONSULTAHTTPSENDERJOBSLIST HttpSenderJobList;

	//Lista var=value para update de diversas tabelas
	CMapStringToString campoValorSetList;
	
	while(WaitForSingleObject(m_evStop, p_HttpSenderDados->iTimeExecThread) != WAIT_OBJECT_0)
    {

		//STLOG_WRITE(L"%s(%d): ->Thread Name: [%s] - Memoria Disponivel: [%d]", L"CHttpSenderThread::_HttpSenderThreadProc", __LINE__, p_HttpSenderDados->sThreadName, CUtil::GetAvailableMemory());
#ifdef _WIN32_WCE

		_SetThreadInfo(p_HttpSenderDados->sThreadName, p_HttpSenderDados->iTimeExecThread, GetCurrentThreadId(), INVALID_HANDLE_VALUE, CUtil::GetCurrentDateTime(L"DATA_HORA"), NULL);
		//Para conex�o(do modem) em caso de off-line
		CUtil::IsOnline();
		//Se off-line nao faz nada...
		if(!CUtil::IsOnline(L"", L""))
		{
			STLOG_WRITE(L"[%s] CHttpSenderThread::HttpSenderThreadProc: Talao Off-Line", p_HttpSenderDados->sThreadName);			
			continue;
		}

#endif
		if(p_HttpSenderDados->sThreadName.CompareNoCase(L"LOGIN_LOGOUT") == 0)
		{
			CString sTT;
			//sTT.Format(L"ThreadId: [%x]Usu�rio Logado: [%s] Time: [%d] Executando Verifica��o.", GetCurrentThreadId(), iTimeExecThread);
			//sTT.Format(L"ThreadId: [%x] Usu�rio Logado: [%s] Time: [%d], [%d] Executando Verifica��o.", GetCurrentThreadId(),CUtil::m_sLoggedUser, p_HttpSenderDados->iTimeExecNormal, p_HttpSenderDados->iTimeExecThread);
			//STLOG_WRITE(sTT);
			

			if ((CUtil::m_sLoggedUser.CompareNoCase( L"--NENHUM--") != 0) && (!CUtil::m_bAutenticando))
			{
				CString sTTT;
				//sTTT.Format(L"ThreadId: [%x][%s] CHttpSenderThread::_HttpSenderThreadProc(): Executando Verifica��o.", GetCurrentThreadId() ,p_HttpSenderDados->sThreadName);
				//STLOG_WRITE(sTTT);

				///Verifica se permiss�o do agente � somente GERINCID
				CString sUserPerms = CUtil::GetLoggedUserPerms();
				if (sUserPerms.Find(L"TEM")> -1)  ///n�o � somente GERINCID
				{
					if (CUtil::VerificaAgenteLogado( &sTTT, &p_HttpSenderDados->m_proxyInfo))
					{
						//mensagem 
						//STLOG_WRITE(sTTT);
						CString msg;
						msg.LoadStringW(IDS_AGENTELOGADO);
						::MessageBox(GetForegroundWindow(), msg, L"Aten��o!", MB_ICONEXCLAMATION|MB_OK);
					}
				}
				else
				{
					continue;
				}
			}
		}


		// Atualiza hora da �ltima entrada na thread...
		GetLocalTime(&p_HttpSenderDados->st);

		//Pesquisa por todos os jobs ainda n�o enviados filtrados por nome do obj e status = novo...
		if(!CConsultasHttpSender::GetJobs2Send(m_pDB, p_HttpSenderDados->sThreadName, &HttpSenderJobList))
		{
			// SQL Error...
			STLOG_WRITE(L"[%s] CHttpSenderThread::HttpSenderThreadProc: N�o foi poss�vel consultar jobs com status = N", p_HttpSenderDados->sThreadName);				
			return 0;
		}		
		
		if(HttpSenderJobList.GetCount() > 0)
		{
			///STLOG_WRITE(L"[%s] CHttpSenderThread::HttpSenderThreadProc: %d jobs novos encontrados", p_HttpSenderDados->sThreadName, HttpSenderJobList.GetCount());	

			//Se objeto estiver usando UpdateClockAfterSend, atualiza rel�gio...
			if(!p_HttpSenderDados->m_sURLRelogio.IsEmpty())
			{
				if(!CUtil::SetDataHoraServidor(&p_HttpSenderDados->m_proxyInfo, p_HttpSenderDados->m_sURLRelogio, p_HttpSenderDados->m_sContrato))
				{
					STLOG_WRITE(L"[%s] CHttpSenderThread::HttpSenderThreadProc Erro atualizando rel�gio", p_HttpSenderDados->sThreadName);
				}
				else
				{
					//STLOG_WRITE(L"[%s] CHttpSenderThread::HttpSenderThreadProc Rel�gio atualizando", p_HttpSenderDados->sThreadName);
				}
			}

			POSITION p = HttpSenderJobList.GetHeadPosition();		
			while(p)
			{			
				ConsultaJobs2Send *inc = HttpSenderJobList.GetNext(p);

				CString sVarFileName	 = CString(inc->sFileWebExt);
				CString sPathFile		 = CString(inc->sFilePath);
				CString sPostVarsList	 = CString(inc->sVarList);
				CString sOnTXOkAction	 = CString(inc->sOnTXOkAction);							

				STLOG_WRITE(L"[%s] CHttpSenderThread::HttpSenderThreadProc: Job selecionado p/ envio [%s], Tentativa: [%d], Tempo: [%d]", p_HttpSenderDados->sThreadName, sPathFile, iNumTentativaComErro, iTimeExecThread);				

				//Atualiza status do job selecionado p/ T (Transmitindo)						
				CString sId;
				sId.Format(L"%d", inc->iId);
				campoValorSetList.SetAt(L"status", L"'T'");
				if(!CConsultas::Update(m_pDB, L"httpsender_job", L"id", sId, &campoValorSetList))
				{
					STLOG_WRITE(L"[%s] CHttpSenderThread::CreateHttpSenderThread() Erro atualizando status do job [%s] a ser enviado de N p/ T", p_HttpSenderDados->sThreadName, sId);
				}
						
				//Separa lista de variavel=valor (sPostVarsList)
				int n = 3;
				CMapStringToString sList;				
				CUtil::Tokenize(sPostVarsList, sList, n); 				

				//Envia arquivo...
				CString sResp;
				int iRespCod = 0;

				//Lock do sender...
				//EnterCriticalSection(&m_SendLock);	//&CUtil::m_uSendLock
//#ifndef _SIMPLE_REQUEST
				EnterCriticalSection(&CUtil::m_uSendLock);
//#endif
				//Se envio OK...
				if(CUtil::HttpSendFile(sVarFileName, sPathFile, p_HttpSenderDados->sURLDestino, &sList, iRespCod, &sResp, &p_HttpSenderDados->m_proxyInfo, TRUE))
				{	
					CString sTime = CUtil::GetCurrentDateTime(L"DATA_HORA");
					_SetThreadInfo(p_HttpSenderDados->sThreadName, p_HttpSenderDados->iTimeExecThread, GetCurrentThreadId(), INVALID_HANDLE_VALUE, sTime, sTime);
					//Executa fun��o apos TX=OK, geralmente atualiza status de transmiss�o do registro na tabela do obj de envio				
					if(_ExecFuncOnTxOk(sOnTXOkAction, inc))
					{			
						BOOL job_deleted = FALSE;
						//Deleta job...
						if(!CConsultasHttpSender::DeleteJobSent(m_pDB, inc->iId))
						{
							STLOG_WRITE(L"[%s] CHttpSenderThread::CreateHttpSenderThread() Erro deletando job [%s] da tabela httpsender_job", p_HttpSenderDados->sThreadName, sId);
						}
						else
						{
							job_deleted = TRUE;
						}
												
						//STLOG_WRITE(L"[%s] CHttpSenderThread::HttpSenderThreadProc Enviado com sucesso arquivo [%s]", p_HttpSenderDados->sThreadName, sPathFile);										
						//(L"[%s] CHttpSenderThread::HttpSenderThreadProc Resposta do servidor: %s", p_HttpSenderDados->sThreadName, sResp);
						//Volta valores iniciais de num. de tentativas e intervalo de execu��o da thread
						iNumTentativaComErro = 0;
						///iTimeExecThread = p_HttpSenderDados->iTimeExecNormal;
						p_HttpSenderDados->iTimeExecThread = p_HttpSenderDados->iTimeExecNormal;

						//Armazaena data/hora �ltimo envio do GPS no registro do windows
						if(p_HttpSenderDados->sThreadName.CompareNoCase(L"XML_GPS") == 0)
						{
							//Data e hora atual...
							SYSTEMTIME st;
							GetLocalTime(&st);	
							CString sDataHora;	
							sDataHora.Format(L"%02d/%02d/%4d %02d:%02d:%02d", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute, st.wSecond);							

							Registry reg(HKEY_LOCAL_MACHINE, L"SOFTWARE\\GPS\\LastSend");
							reg.SetValue(L"SOFTWARE\\GPS\\LastSend", sDataHora);
							reg.Close();
						}						
						
						//Se true, grava log...
						if(p_HttpSenderDados->fRecordLog)
							CConsultas::GravaHttpSenderLog(m_pDB, p_HttpSenderDados->sThreadName, sPathFile, sResp, iRespCod, job_deleted);												

						//Se true, deleta arquivo enviado com sucesso...
						//if(p_HttpSenderDados->fDeleteFileAfterSend)
						DeleteFile(sPathFile);										
					}
					else //Erro ao atualizar status de transmissao
					{	
						//Se true, grava log (erro na atualiza��o de status de transmiss�o!)
						if(p_HttpSenderDados->fRecordLog)
							CConsultas::GravaHttpSenderLog(m_pDB, p_HttpSenderDados->sThreadName, sPathFile, L"Erro na atualiza��o de status de transmiss�o!", iRespCod, false);											
					}									
				}
				//Erro no envio...
				else
				{					
					STLOG_WRITE(L"[%s] CHttpSenderThread::HttpSenderThreadProc Falha no envio do arquivo [%s] URL: %s Resp. Serv.: [%d] %s", p_HttpSenderDados->sThreadName, sPathFile, p_HttpSenderDados->sURLDestino, iRespCod, sResp);
					///STLOG_WRITE(L"[%s] CHttpSenderThread::HttpSenderThreadProc Resposta do servidor: [%d] %s", p_HttpSenderDados->sThreadName, iRespCod, sResp);					

					//Trata c�digos de erro...
					switch(iRespCod)
					{
						/***** Erros que implicam reenvio *****/						
						case 200:
							//Volta o status do job para N
							campoValorSetList.SetAt(L"status", L"'N'");
							if(!CConsultas::Update(m_pDB, L"httpsender_job", L"id", sId, &campoValorSetList))
							{
								STLOG_WRITE(L"[%s] CHttpSenderThread::CreateHttpSenderThread() Erro ao marcar job p/ reenvio", p_HttpSenderDados->sThreadName);
							}

							//e incrementa n�m de tentativa de envio com falha
							iNumTentativaComErro++;
							//LeaveCriticalSection(&m_SendLock); //&CUtil::m_uSendLock
							LeaveCriticalSection(&CUtil::m_uSendLock);
						break;

						/***** Erros fatais *****/
						case 300:						
							//Marca o job como erro (status=E) e n�o envia mais
							campoValorSetList.SetAt(L"status", L"'E'");
							if(!CConsultas::Update(m_pDB, L"httpsender_job", L"id", sId, &campoValorSetList))
							{
								STLOG_WRITE(L"[%s] CHttpSenderThread::CreateHttpSenderThread() Erro ao marcar job com erro fatal", p_HttpSenderDados->sThreadName);
							}
							//LeaveCriticalSection(&m_SendLock);
							LeaveCriticalSection(&CUtil::m_uSendLock);
						break;

						/***** Erro inv�lido ou resposta vazia *****/
						default:
							//Qualquer outro valor de iRespCod, o job fica com status=T e s� ser� marcado como novo (N) 
							//ao entrar no sistema novamente, na cria��o da thread (CreateHttpSenderThread)
							//LeaveCriticalSection(&m_SendLock);
							LeaveCriticalSection(&CUtil::m_uSendLock);
						break;
					}

					//Aumenta tempo de execu��o da thread se n�m de tentativas com falha alcan�ar limite estabelecido
					if(iNumTentativaComErro >= LIMITE_TX_COM_FALHA)
					{
						iNumTentativaComErro = 0;
						///iTimeExecThread = p_HttpSenderDados->iTimeExecLento;
						p_HttpSenderDados->iTimeExecThread = p_HttpSenderDados->iTimeExecLento;

						//STLOG_WRITE(L"[%s] CHttpSenderThread::HttpSenderThreadProc Mudei tempo de execu��o p/ %d", p_HttpSenderDados->sThreadName, p_HttpSenderDados->iTimeExecThread);					
					}

					//Grava log de tentativa de envio
					if(sResp.IsEmpty() || sResp.GetLength() > 300)
						sResp = L"Erro ao enviar arquivo!";

					//Se true, grava log...
					if(p_HttpSenderDados->fRecordLog)
						CConsultas::GravaHttpSenderLog(m_pDB, p_HttpSenderDados->sThreadName, sPathFile, sResp, iRespCod, FALSE);									
										
					//break;
					continue;
				}			
				//LeaveCriticalSection(&m_SendLock);
//#ifndef _SIMPLE_REQUEST
				LeaveCriticalSection(&CUtil::m_uSendLock);
//#endif
			}
			
			_DestroyJobList(&HttpSenderJobList);
			//HttpSenderJobList.RemoveAll();
		}
		else //Nenhum job encontrado...
		{
			//STLOG_WRITE(L"[%s] CHttpSenderThread::HttpSenderThreadProc: Nenhum arquivo .job encontrado!", p_HttpSenderDados->sThreadName);
			continue;
		}		
	}

    return 0;
}

void CHttpSenderThread::_DeleteNotDeletedSentJobs()
{
	CStringA sQuery;
	CStringA sTable, sKey, sValue;
	
	try
	{

		sQuery.Format("SELECT obj_name, on_tx_ok_table, on_tx_ok_key_name, on_tx_ok_key_value FROM httpsender_job WHERE obj_name = '%S'", HttpSenderDados->sThreadName);

		CppSQLite3Query q = CppSQLite3DB::getInstance()->execQuery(sQuery);
		if(!q.eof())
		{
			sTable = q.getStringField(1);
			sKey = q.getStringField(2);
			sValue = q.getStringField(3);
		}
		else
		{
			return;
		}

		q.finalize();

		sQuery.Format("DELETE FROM httpsender_job WHERE id  in (SELECT httpsender_job.id FROM httpsender_job, %s WHERE httpsender_job.on_tx_ok_key_value = %s.%s and %s.transmissao = 1)",
			sTable, sTable, sKey, sTable);
		
		if (sTable.IsEmpty()) return;
		CppSQLite3DB::getInstance()->execQuery(sQuery);
	}
	catch(CppSQLite3Exception e)
	{
		STLOG_WRITE("%s(%d): Erro executando query [%s]. Motivo: %s", __FUNCTION__, __LINE__, sQuery, e.errorMessage());
	}

}

/**
\brief Destr�i a lista de jobs a serem enviados
\param __CONSULTAHTTPSENDERJOBSLIST* jobList: Lista de jobs a serem enviados
\return void
*/
void CHttpSenderThread::_DestroyJobList(__CONSULTAHTTPSENDERJOBSLIST* jobList)
{
	POSITION p = jobList->GetHeadPosition();		
	while(p)
	{			
		ConsultaJobs2Send *inc = jobList->GetNext(p);
		delete inc;
	}
	jobList->RemoveAll();
}


/**
\brief Executa fun��o ap�s envio ok de arquivo
\param CString sFuncName Nome da fun��o a ser executada
\param ConsultaJobs2Send Ponteiro da pesquisa da tabela job que est� sendo enviado
\return BOOL
*/
BOOL CHttpSenderThread::_ExecFuncOnTxOk(CString sFuncName, ConsultaJobs2Send *inc)
{
	if(sFuncName.CompareNoCase(L"UPDATE_TRANSMISSAO") == 0)
	{		
		CMapStringToString campoValorSetList;
		campoValorSetList.SetAt(L"TRANSMISSAO", L"1");
		if((CString(inc->sOnTXOkTable)).CompareNoCase(L"LOG_AGENTE") != 0)
		{		
			campoValorSetList.SetAt(L"DT_TRANS", L"NOW");			
		}
		if(!CConsultas::Update(m_pDB, 
							   CString(inc->sOnTXOkTable), 
							   CString(inc->sOnTXOkKeyName), 
						       CString(inc->sOnTXOkKeyValue), 
						       &campoValorSetList))
		{			
			STLOG_WRITE(L"CHttpSenderThread::_ExecFuncOnTxOk(): Erro atualizando status de transmiss�o! Tabela: %s", inc->sOnTXOkTable);			
			return FALSE;
		}
	}	
	return TRUE;
}

/**
\brief Configura se ser� gravado todo o log de envio
\param BOOL bValue Flag gravar log (HttpSenderDados->fRecordLog)
\return void
*/
void CHttpSenderThread::WorkWithLog(BOOL bValue)
{	
	HttpSenderDados->fRecordLog = bValue;
}

/**
\brief Configura se ap�s o envio com sucesso o arquivo ser� deletado
\param BOOL bValue Flag remove arquivo (HttpSenderDados->fDeleteFileAfterSend)
\return void
*/
void CHttpSenderThread::DeleteFileAfterSend(BOOL bValue)
{	
	HttpSenderDados->fDeleteFileAfterSend = bValue;
}

/**
\brief Preenche as informa��es do proxy para conex�o e URL de atualiza��o do rel�gio
\param CProxyInfo *p Ponteiro para a classe de informa��es do proxy
\param LPCTSTR szURL URL de atualiza��o do rel�gio
\return void
*/
void CHttpSenderThread::UpdateClockAfterSend(CProxyInfo *p, LPCTSTR szURL, LPCTSTR szContrato)
{
	HttpSenderDados->m_proxyInfo.bDiscagem = p->bDiscagem; 
	HttpSenderDados->m_proxyInfo.bProxy    = p->bProxy;
	HttpSenderDados->m_proxyInfo.nPort	   = p->nPort;
	HttpSenderDados->m_proxyInfo.sPass     = p->sPass;
	HttpSenderDados->m_proxyInfo.sServer   = p->sServer;
	HttpSenderDados->m_proxyInfo.sUser	   = p->sUser;

	if(wcslen(szURL) > 0)
	{
		HttpSenderDados->m_sURLRelogio = szURL;
		HttpSenderDados->m_sContrato = szContrato;
	}
}

/**
\brief Preenche as informa��es do proxy para conex�o
\param CProxyInfo *p Ponteiro para a classe de informa��es do proxy
\return void
*/
void CHttpSenderThread::SetProxy(CProxyInfo *p)
{
	HttpSenderDados->m_proxyInfo.bDiscagem = p->bDiscagem; 
	HttpSenderDados->m_proxyInfo.bProxy    = p->bProxy;
	HttpSenderDados->m_proxyInfo.nPort	   = p->nPort;
	HttpSenderDados->m_proxyInfo.sPass     = p->sPass;
	HttpSenderDados->m_proxyInfo.sServer   = p->sServer;
	HttpSenderDados->m_proxyInfo.sUser	   = p->sUser;
}

void CHttpSenderThread::_SetThreadInfo(CString& sThread, int& nExecTime, DWORD dwThreadId, HANDLE hThread, CString& sTimeStamp, const CString sSendTime)
{
	CTable tabJobInfo;

	tabJobInfo.Init(L"httpsender_info");

	if(sSendTime.IsEmpty())
	{
		try
		{
			CStringA sQuery;
			sQuery.Format("SELECT ult_envio FROM httpsender_info WHERE obj_name = '%S'", sThread);

			CppSQLite3Query q = CppSQLite3DB::getInstance()->execQuery(sQuery);

			if(!q.eof())
				tabJobInfo.SetValue(L"ult_envio", CString(q.getStringField(0)));
			else
				tabJobInfo.SetValue(L"ult_envio", L"");

			q.finalize();
		}
		catch(CppSQLite3Exception e)
		{
			STLOG_WRITE("%s(%d): Erro executando query. Motivo: %s", __FUNCTION__, __LINE__, e.errorMessage());
		}
	}
	else
	{
		tabJobInfo.SetValue(L"ult_envio", sSendTime);
	}

	tabJobInfo.SetValue(L"obj_name", sThread);
	tabJobInfo.SetValue(L"thread_time", (long)nExecTime);
	tabJobInfo.SetValue(L"thread_handle", (long)hThread);
	tabJobInfo.SetValue(L"ult_exec", sTimeStamp);
	tabJobInfo.SetValue(L"thread_id", (long)dwThreadId);

	
	
	tabJobInfo.InsertOrReplace(CppSQLite3DB::getInstance());
}
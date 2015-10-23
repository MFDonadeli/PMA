#include "StdAfx.h"
#include "RcGen.h"
#include "Utils.h"


CArray<cControles, cControles> CRcGen::arrControles;
CMap<CString, LPCTSTR, CTelaAtual*, CTelaAtual*> CRcGen::mapTelaAtual;
CArray<CButton*, CButton*> CRcGen::arrGroupBox;
CArray<int, int> CRcGen::arrGroupPos;

int CRcGen::m_nCtrl = 0;
int CRcGen::m_nGrupo = 1;
int CRcGen::m_nMaxY = 700;
int CRcGen::m_nSubGrupo = 0;
BOOL CRcGen::m_bByGroup = TRUE;
int CRcGen::m_nXCol = 0;
int CRcGen::m_nIdInicial = -1;
CWnd* CRcGen::m_pTrueThis = NULL;
UINT CRcGen::WM_NEW_INIT = RegisterWindowMessage(L"CRcGen::WM_NEW_INIT");
CPropertySheet* CRcGen::m_sheet;
int CRcGen::m_nGrupoInicial = 0;
BOOL CRcGen::bFlagDeletingArrControles = FALSE;
int CRcGen::m_nBegin = 0;
int CRcGen::m_nEnd = 0;


CRcGen::CRcGen(CWnd* pParent)
{
	m_pThis = pParent;
	m_pTrueThis = pParent;

	m_x = 0;
	m_y = DEFAULT_Y_INIT;
	m_bShow = TRUE;

}

CRcGen::~CRcGen(void)
{
}

CRcGen::CRcGen(void)
{
}

#ifdef _WIN32_WCE
int CRcGen::GenerateScreen(CPaintDC* pDc)
{
	DWORD id = 100;
    CRect rect;
    CTelaAtual* at;
    int y_ant=0; //Y inicial da linha
    int y_atu; //Máximo y de uma linha
    int cont_linha = 0;
    int space = 0;
    int ratioXPos = 0;
	BOOL bSumY = TRUE; //Para iniciar a formação de controles com posição fixa
	int yMax = 0; //Guarda o y máximo quando desenhar uma sequência de controles com posições fixas/agrupamento

	//Varre o array de controles preenchido pelo select
    for(int i=m_nCtrl; i<arrControles.GetSize(); i++)
    {
		//Classe para armazenar o controle criado nesta iteração
		at = new CTelaAtual();
		cControles cCtrl, cCtrl_temp;
		//Pega um controle
		cCtrl = arrControles.GetAt(i);

		if(cCtrl.iEsquerda != -1)
		{
			bSumY = FALSE;
			yMax = max(yMax, y_atu);
		}
		else
		{
			if(!bSumY)
				m_y = yMax + DEFAULT_SPACE;

			bSumY = TRUE;
		}

		//y_atu é igual ao tamanho padrão
		y_atu = DEFAULT_HEIGHT;

		//Esta condição é para verificar a mudança de subgrupo
		//Se o subgrupo atual for diferente do subgrupo do controle pego e
		//se está definido que é por grupo (SEMPRE TRUE) e
		//se o grupo atual e o grupo do controle pego são os mesmos
        if(m_nSubGrupo!=cCtrl.iSubGrupo && m_bByGroup && m_nGrupo==cCtrl.iGrupo)
        {
			//Somente incrementa o sub-grupo, no caso do pocket vamos trabalhar somente com o grupo
            m_nSubGrupo++;
        }
		//Se o grupo atual e o grupo do controle pego for diferente
        else if(m_nGrupo!=cCtrl.iGrupo && m_bByGroup)
        {

			y_ant=0; //Y inicial da linha
			cont_linha = 0;
			space = 0;
			ratioXPos = 0;
			bSumY = TRUE; //Para iniciar a formação de controles com posição fixa
			yMax = 0; //Guarda o y máximo quando desenhar uma sequência de controles com posições fixas/agrupamento
			m_bShow = FALSE;
			////Incrementa o grupo atual
   //         m_nGrupo++;
   //         //Incrementa a posição y atual
   //         m_y+=2*DEFAULT_SPACE;
			////Cria um novo separador de grupo
   //         m_button = new CButton();
			//
			//CString sNome = _GetGrupoDesc(m_nGrupo);
   //         m_button->Create(sNome, group_style, CRect(0,m_y,DEFAULT_CTRL_WIDTH,m_y+15), m_pThis, id);
   //         m_button->ShowWindow(SW_SHOW);
			////Incrementa a posição y atual
   //         m_y+=DEFAULT_Y_INIT+15;

			////Adiciona novo grupo ao array de grupos. No pocket este array de grupos, serve somente como referência da posição de tela
			////para que os botões próximo e anterior funcionem.
   //         arrGroupPos.Add(m_y);

			//arrGroupBox.Add(m_button);


        }
            
        CRect r;
        m_pThis->GetWindowRect(r);
        if(m_nXCol+DEFAULT_CTRL_WIDTH > r.right) break;

        ratioXPos = cCtrl.iLargura;
        //Se não for manter linha verifica se os próximos deverão manter para garantir o correto posicionamento
        if(cCtrl.iManterLinha == 0)
        {
            int j = i+1;
            cont_linha=0;
			//Verifica se o array de controles não está no fim
            if(j!=arrControles.GetSize())
            {
				//Verifica se os próximos controles tem que manter a linha
                cCtrl_temp = arrControles.GetAt(j++);
                
                while(cCtrl_temp.iManterLinha !=0)
                {
					//Qtde de controles que manterão a linha
                    cont_linha++;
                    if(j==arrControles.GetSize()) break;
                    cCtrl_temp = arrControles.GetAt(j++);
                }
            }

			//Se houver controles que manterão a linha
            if(cont_linha!=0)
            {
				//Incrementa em 1 para que acertar o número de controles que manterão a linha
                cont_linha++;
				//Se os controles manterão a linha e tiver com o valor 100 na largura, então eles serão divididos igualmente na linha
                if(ratioXPos==100)
                    ratioXPos/=cont_linha;

				//Define o tamanho do espaço máximo que o controle usará
                space = (DEFAULT_CTRL_WIDTH + DEFAULT_LBL_WIDTH) * (ratioXPos/100);
            }
        }
            
        //Se é para manter linha o usa o y do controle montado anteriormente e desloca o x
        if(cCtrl.iManterLinha)
        {
            m_y = y_ant;
            m_x += space;
        }
        else
        {
			//Volta o x para o começo da tela
            m_x = 0;
        }

		//Desenha o label se for necessário
        y_atu = _BuildLabel(&cCtrl, at, m_y, ratioXPos);
        m_y += y_atu;
		
		//Gera o controle
		_GenerateControls(cCtrl, rect, ratioXPos, at, id, y_atu);

        //O y do controle anterior é igual ao y atual
		y_ant = m_y;
		//O y atual será incrementado para que o próximo controle seja desenhado somente se a posição não for fixa
        //m_y = y_atu + DEFAULT_SPACE;
		if(bSumY)
			m_y = y_atu + DEFAULT_SPACE;
		//id do controle criado no momento
        m_nCtrl = i;

		//Seta propriedades do controle
        if(cCtrl.iDesabilitado) at->wndControl->EnableWindow(FALSE);
		//Coloca o controle no mapa de controles
        at->nIdCtrlArray = i;
        if(m_nIdInicial == -1)
			m_nIdInicial = cCtrl.nID;
        mapTelaAtual.SetAt(cCtrl.sID, at);

    }

    return rect.bottom + DEFAULT_HEIGHT; //DEFAULT_HEIGHT é um espaço final
}
#endif

#ifndef _WIN32_WCE
int CRcGen::GenerateScreenByX(CPaintDC* pDc)
{
	DWORD id = 100;
	CRect rect;
	CTelaAtual* at;
	int y_ant = DEFAULT_Y_INIT; //Y inicial da linha
	int y_atu = 0; //Máximo y de uma linha
	int y_max_sub = 0; //Máximo y de uma linha dentro de um sub-grupo
	int cont_linha = 0;
	int space = 0;
	int ratioXPos = 0;
	int nAgrupado = 0;
	int xMax = 0; //O máximo x ocupado pelo subgrupo atual
	BOOL bSumY = TRUE; //Para iniciar a formação de controles com posição fixa
	int yMax = 0; //Guarda o y máximo quando desenhar uma sequência de controles com posições fixas/agrupamento
	CRect rectSubGrupo; //Retângulo dentro do qual será desenhado o subgrupo
	
	ZeroMemory(&rectSubGrupo, sizeof(CRect));

	CRect rct;
	m_pThis->GetWindowRect(rct);

	for(int i=m_nCtrl; i<arrControles.GetSize(); i++)
	{
	
		at = new CTelaAtual();
		cControles cCtrl, cCtrl_temp;
		//Pega um controle da lista de controles
		cCtrl = arrControles.GetAt(i);

		//Se for um novo agrupamento ou fim do agrupamento guarda o valor do agrupamento
		if(cCtrl.iAgrupado != nAgrupado)
		{
			nAgrupado = cCtrl.iAgrupado;			
		}
		//Se não for mas agrupamento então não precisa construir no eixo y
		else if(nAgrupado==0)
		{
			
		}
		//Ainda é o mesmo agrupamento ou não há agrupamento
		else
		{
			//Se manter o mesmo agrupamento, então constrói no eixo y
			//bBreak = TRUE;
			//Se os controles não mantiverem a mesma linha então
			if(!cCtrl.iManterLinha)
			{	
				//Se não houver posição especificada de esquerda
				if(cCtrl.iEsquerda == -1)
				{
					//Soma com yMax se acabou as posições fixas
					if(!bSumY)
						m_y = yMax + DEFAULT_SPACE;
					else
						m_y = rect.bottom + DEFAULT_SPACE;
				
					//No caso de agrupamento não precisa aumentar o valor de m_nXCol, permanecerá com o mesmo x.
					m_nXCol -= DEFAULT_CTRL_WIDTH;

					bSumY = TRUE;
				}
				else
				{
					//É para somar m_y somente na primeira vez que tiver uma posição fixa no banco de dados
					//pois tem que ser adicionado a posição do último controle para não posicionar em cima dele
					if(bSumY)
						m_y = rect.bottom + DEFAULT_SPACE;

					bSumY = FALSE;

				}
			}
			y_ant = m_y;
		}

		//Se não manter a linha
		if(!cCtrl.iManterLinha)
		{
			//Se for estourar o X máximo da tela quebra a linha
			if(m_nXCol+DEFAULT_CTRL_WIDTH > m_nMaxY)
			{
				
				if(rectSubGrupo.Height() != 0)
				{
					rectSubGrupo.right = max(rect.right, rectSubGrupo.right);
					m_nXCol = rectSubGrupo.left+5;
				}
				else
				{
					m_nXCol = 0;
				}
			
				m_y = y_max_sub + 2*DEFAULT_SPACE;
				y_ant = m_y;
				//bBreak = TRUE;
			}
			//Senão não quebra
			else
			{
				//bBreak = FALSE;
			}
			m_y = y_ant; //Zera o y para a posição inicial da linha
		}
		//Se manter linha não soma X.
		else
		{
			m_nXCol -= DEFAULT_CTRL_WIDTH;
		}

		//Esta condição é para verificar a mudança de subgrupo
		//Se o subgrupo atual for diferente do subgrupo do controle pego e
		//se está definido que é por grupo (SEMPRE TRUE) e
		//se o grupo atual e o grupo do controle pego for os mesmos
		if(m_nSubGrupo!=cCtrl.iSubGrupo && m_bByGroup && m_nGrupo==cCtrl.iGrupo)
		{
			//Espaçamento padrão para a criação do novo sub-grupo
			m_nSubGrupo++;
			//m_y=y_atu;
			m_y+=5*DEFAULT_SPACE;
			
			TRACE(L"Rect do SubGrupo anterior: %d %d %d %d\r\n", rectSubGrupo.left, rectSubGrupo.top, rectSubGrupo.right, rectSubGrupo.bottom);
			
			CRect rGrp;
			if(rectSubGrupo.Height() != 0)
			{
				m_button = arrGroupBox.GetAt(arrGroupBox.GetUpperBound());
				m_button->GetWindowRect(rGrp);
				m_pThis->ScreenToClient(rGrp);

				rGrp.right = rectSubGrupo.right+5;
				rGrp.bottom = rectSubGrupo.bottom+5;

				m_button->MoveWindow(rGrp);
				TRACE(L"Last button moved to: %d %d %d %d\r\n", rGrp.top, rGrp.bottom, rGrp.left, rGrp.right);
				//m_y = rGrp.top;

			}

			if(rectSubGrupo.right + DEFAULT_CTRL_WIDTH > m_nMaxY)
			{
				m_nXCol = 0;
				m_y = y_atu + DEFAULT_SPACE;
			}
			else
			{
				if(rectSubGrupo.Height() != 0)
					m_y = rectSubGrupo.top;
			}

			rectSubGrupo.top = m_y;
			rectSubGrupo.left = m_nXCol;
			rectSubGrupo.right = m_nXCol + DEFAULT_CTRL_WIDTH;
			m_y += 5;
			m_nXCol += 5;
			

			m_button = new CButton();
			//y = y_ant; //Zera o y para a posição inicial da linha
			//y = rect.bottom + DEFAULT_SPACE;
			CString sBtnName = _GetSubGrupoDesc(m_nGrupo, m_nSubGrupo);

			m_button->Create(sBtnName, group_style, CRect(m_nXCol,m_y,DEFAULT_CTRL_WIDTH*2+m_nXCol,m_y+15), m_pThis, id);
			TRACE(L"New button create at: %d %d %d %d\r\n", m_nXCol, m_y, DEFAULT_CTRL_WIDTH*2+m_nXCol, m_y+15);
			m_button->ShowWindow(SW_SHOW);
			arrGroupBox.Add(m_button);
			m_y+=DEFAULT_Y_INIT;

			y_ant = m_y;

			rectSubGrupo.bottom = m_y;
			y_max_sub = 0;
			
			//if(xMax !=0 && xMax + DEFAULT_CTRL_WIDTH < m_nMaxY)
			//{
			//	CRect rGrp;
			//	m_button = arrGroupBox.GetAt(arrGroupBox.GetUpperBound());
			//	m_button->GetWindowRect(rGrp);
			//	m_pThis->ScreenToClient(rGrp);
			//	rGrp.right = xMax;

			//	m_button->MoveWindow(rGrp);
			//	

			//	m_y = rGrp.top;
			//	//y = y_ant;
			//	//nXCol = 0;
			//}
			//else
			//{
			//	m_nXCol = 0;
			//}

			//xMax = m_nXCol;

			//m_button = new CButton();
			////y = y_ant; //Zera o y para a posição inicial da linha
			////y = rect.bottom + DEFAULT_SPACE;
			//m_button->Create(L"Sub-Grupo", group_style, CRect(m_nXCol,m_y,DEFAULT_CTRL_WIDTH*2+m_nXCol,m_y+15), m_pThis, id);
			//TRACE(L"New button create at: %d %d %d %d\r\n", m_nXCol, m_y, DEFAULT_CTRL_WIDTH*2+m_nXCol, m_y+15);
			//m_button->ShowWindow(SW_SHOW);
			//arrGroupBox.Add(m_button);
			//m_y+=DEFAULT_Y_INIT;

			//y_ant = m_y;
		}

		

		CRect r;
		m_pThis->GetWindowRect(r);
		//if(m_nXCol+DEFAULT_CTRL_WIDTH > r.right) break;

		ratioXPos = cCtrl.iLargura;
		//Se não for manter linha verifica se os próximos deverá manter para garantir o correto posicionamento
		//if(cCtrl.iManterLinha == 0)
		//{
		//	int j = i+1;
		//	cont_linha=0;
		//	if(j!=arrControles.GetSize())
		//	{
		//		cCtrl_temp = arrControles.GetAt(j++);
		//		
		//		while(cCtrl_temp.iManterLinha !=0)
		//		{
		//			cont_linha++;
		//			if(j==arrControles.GetSize()) break;
		//			cCtrl_temp = arrControles.GetAt(j++);
		//		}
		//	}

		//	if(cont_linha!=0)
		//	{
		//		cont_linha++;
		//		if(ratioXPos==100)
		//			ratioXPos/=cont_linha;

		//		
		//		space = (DEFAULT_CTRL_WIDTH + DEFAULT_LBL_WIDTH) * (ratioXPos/100);
		//	}
		//}
		//
		//Se é para manter linha o usa o y do controle montado anteriormente e desloca o x
		
		m_y += _BuildLabel(&cCtrl, at, y_ant, ratioXPos);

		_GenerateControls(cCtrl, rect, ratioXPos, at, id, y_atu);

		if(rectSubGrupo.Height() != 0)
		{
			rectSubGrupo.right = max(rect.right, rectSubGrupo.right);
			rectSubGrupo.bottom = max(rect.bottom, rectSubGrupo.bottom);
		}
		
		y_max_sub = max(y_max_sub, rect.bottom);

		yMax = y_atu;

		m_nCtrl = i;
		//TRACE(L"y_ant=%d y=%d y_atu=%d DS=%d Break=%d\r\n", y_ant, y, y_atu, DEFAULT_SPACE, cCtrl.iManterLinha);
		if(cCtrl.iDesabilitado) at->wndControl->EnableWindow(FALSE);
		at->nIdCtrlArray = i;
		if(m_nIdInicial == -1)
			m_nIdInicial = cCtrl.nID;
		mapTelaAtual.SetAt(cCtrl.sID, at);

		if(cCtrl.iEsquerda < 0)
			m_nXCol += DEFAULT_CTRL_WIDTH;
		xMax = max(rect.right, xMax);
	}//end for

	if(rectSubGrupo.Height() != 0)
	{
		CRect rGrp;
		m_button = arrGroupBox.GetAt(arrGroupBox.GetUpperBound());
		m_button->GetWindowRect(rGrp);
		m_pThis->ScreenToClient(rGrp);

		rGrp.right = rectSubGrupo.right+5;
		rGrp.bottom = rectSubGrupo.bottom+5;

		m_button->MoveWindow(rGrp);
		TRACE(L"Last button moved to: %d %d %d %d\r\n", rGrp.top, rGrp.bottom, rGrp.left, rGrp.right);
		//m_y = rGrp.top;

	}
	
	 return rect.bottom + DEFAULT_HEIGHT; //DEFAULT_HEIGHT é um espaço final
}
#endif

void CRcGen::_GenerateControls(cControles &cCtrl, CRect &rect, const int ratioXPos, CTelaAtual* at, DWORD &id, int &y_atu)
{
	int iTopo = m_y;
	if(cCtrl.iTopo >= 0)
		iTopo += cCtrl.iTopo;


	if(cCtrl.sTipo.Compare(_T("Label"))==0)
	{
		rect = GetCtrlRect(LABEL, iTopo, ratioXPos, cCtrl.iEsquerda);

		if(cCtrl.sNome.GetLength() > 35)
			rect.bottom += ((cCtrl.sNome.GetLength()/35)+1) * DEFAULT_HEIGHT;

		//pDc->Rectangle(rect);
		//pDc->DrawText(cCtrl.sNome, rect, 0);
		m_label = new CStatic();
		m_label->Create(cCtrl.sNome, 0, rect, m_pThis, id);
		//m_label->ShowWindow(SW_SHOW);
		y_atu = max(rect.bottom + DEFAULT_SPACE, y_atu);

		at->r = rect;
		at->tipoCtrl = LABEL;
		at->wndControl = m_label;
	}
	else if(cCtrl.sTipo.Compare(_T("Botão"))==0)
	{
		if(cCtrl.iExCtrl > 0)
		{
			m_button_ex = new CButtonEx();
		}
		else
		{
			m_button = new CButton();
		}

		rect = GetCtrlRect(BOTAO, iTopo, ratioXPos, cCtrl.iEsquerda);
		y_atu = max(rect.bottom, y_atu);

		(cCtrl.iExCtrl > 0 ? m_button_ex : m_button)->Create(cCtrl.sNome, button_style, rect, m_pThis, id+cCtrl.nID); 
		at->r = rect;
		at->tipoCtrl = BOTAO;
		at->wndControl = (cCtrl.iExCtrl > 0 ? m_button_ex : m_button);
		if(cCtrl.iExCtrl > 0)
		{
			m_button_ex->SetIconEx(cCtrl.iExCtrl, CSize(16,16));
		}
		//pDc->Rectangle(rect);
		//(cCtrl.iExCtrl > 0 ? m_button_ex : m_button)->ShowWindow(SW_SHOW);
	}
	else if(cCtrl.sTipo.Compare(_T("Check"))==0)
	{
		m_button = new CButton();

		rect = GetCtrlRect(CHECK, iTopo, ratioXPos, cCtrl.iEsquerda);
		y_atu = max(rect.bottom, y_atu);

		m_button->Create(cCtrl.sNome, check_style, rect, m_pThis, id+cCtrl.nID); 
		at->r = rect;
		at->tipoCtrl = CHECK;
		at->wndControl = m_button;
		//pDc->Rectangle(rect);
		//m_button->ShowWindow(SW_SHOW);
	}
	else if(cCtrl.sTipo.Compare(_T("Radio"))==0)
	{
		m_button = new CButton();

		rect = GetCtrlRect(RADIO, iTopo, ratioXPos, cCtrl.iEsquerda);
		y_atu = max(rect.bottom, y_atu);

		m_button->Create(cCtrl.sNome, radio_style, rect, m_pThis, id+cCtrl.nID); 
		at->r = rect;
		at->tipoCtrl = RADIO;
		at->wndControl = m_button;
		//pDc->Rectangle(rect);
		//m_button->ShowWindow(SW_SHOW);
	}
	else if(cCtrl.sTipo.Compare(_T("Drop-Down Combo"))==0)
	{
		m_combo = new CEGBComboBox();

		rect = GetCtrlRect(COMBO, iTopo, ratioXPos, cCtrl.iEsquerda);

		y_atu = max(rect.bottom, y_atu);
		//rect.bottom = rect.top + 120;

		m_combo->Create(combo_drop_style, CRect(rect.left, rect.top, rect.right, rect.top + 120), m_pThis, id+cCtrl.nID);
		m_combo->SetFixedContent();
		at->r = rect;
		at->tipoCtrl = COMBO;
		at->wndControl = m_combo;
//			cCtrl.iAltura = DEFAULT_HEIGHT;
		//pDc->Rectangle(rect);
		//m_combo->ShowWindow(SW_SHOW);

	}
	else if(cCtrl.sTipo.Compare(_T("Combo Editável"))==0)
	{
		m_combo = new CEGBComboBox();
		
		rect = GetCtrlRect(COMBO_EDITAVEL, iTopo, ratioXPos, cCtrl.iEsquerda);
		y_atu = max(rect.bottom, y_atu);
		//rect.bottom = rect.top + 120;

		m_combo->Create(combo_edit_style, CRect(rect.left, rect.top, rect.right, rect.top + 120), m_pThis, id+cCtrl.nID);
		at->r = rect;
		at->tipoCtrl = COMBO_EDITAVEL;
		at->wndControl = m_combo;
//			cCtrl.iAltura = DEFAULT_HEIGHT;
		//pDc->Rectangle(rect);
		//m_combo->ShowWindow(SW_SHOW);
	}
	else if(cCtrl.sTipo.Compare(_T("Caixa de Texto"))==0)
	{
		m_edit = new CEGBEdit();
						
		rect = GetCtrlRect(TEXTO, iTopo, ratioXPos, cCtrl.iEsquerda);

		m_edit->Create(text_style, rect, m_pThis, id+cCtrl.nID);
		if(cCtrl.iReadOnly) m_edit->SetReadOnly(TRUE);

		at->r = rect;
		at->tipoCtrl = TEXTO;
		at->wndControl = m_edit;
//			cCtrl.iAltura = DEFAULT_HEIGHT;
		//m_edit->ShowWindow(SW_SHOW);
		y_atu = max(rect.bottom, y_atu);
		//pDc->Rectangle(rect);
	}
	else if(cCtrl.sTipo.Compare(_T("Texto MultiLinha"))==0)
	{
		m_edit = new CEGBEdit();

		rect = GetCtrlRect(TEXTO_MULTILINHA, iTopo, ratioXPos, cCtrl.iEsquerda);
		if(cCtrl.iLinhas == -1)
		{
			CRect r;
			m_pTrueThis->GetWindowRect(r);
#ifdef _WIN32_WCE
			rect.bottom = r.bottom-60;
			rect.right = r.right-30;
#else
			rect.bottom = r.bottom-100;
			rect.right = r.right-100;
#endif
		}
		else
		{
			rect.bottom += (cCtrl.iLinhas-1) * DEFAULT_HEIGHT;
		}

		y_atu = max(rect.bottom, y_atu);

		m_edit->Create(multiline_style, rect, m_pThis, id+cCtrl.nID);
		if(cCtrl.iReadOnly) m_edit->SetReadOnly(TRUE);

		at->r = rect;
		at->tipoCtrl = TEXTO_MULTILINHA;
		at->wndControl = m_edit;
//			cCtrl.iAltura = DEFAULT_HEIGHT;
		//pDc->Rectangle(rect);
		//m_edit->ShowWindow(SW_SHOW);
	}
	else if(cCtrl.sTipo.Compare(_T("Seleção de Data"))==0)
	{
		m_datectrl = new CDateTimeCtrl();

		rect = GetCtrlRect(DATA, iTopo, ratioXPos, cCtrl.iEsquerda);
		y_atu = max(rect.bottom, y_atu);

		m_datectrl->Create(date_style, rect, m_pThis, id+cCtrl.nID);
		at->r = rect;
		at->tipoCtrl = DATA;
		at->wndControl = m_datectrl;
//			cCtrl.iAltura = DEFAULT_HEIGHT;
		//pDc->Rectangle(rect);
		//m_datectrl->ShowWindow(SW_SHOW);
	}
	else if(cCtrl.sTipo.Compare(_T("Seleção de Hora"))==0)
	{
		m_datectrl = new CDateTimeCtrl();

		rect = GetCtrlRect(HORA, iTopo, ratioXPos, cCtrl.iEsquerda);

		m_datectrl->Create(time_style, rect, m_pThis, id+cCtrl.nID);
		at->r = rect;
		at->tipoCtrl = HORA;
		at->wndControl = m_datectrl;
//			cCtrl.iAltura = DEFAULT_HEIGHT;
		y_atu = max(rect.bottom, y_atu);
		//pDc->Rectangle(rect);
		//m_datectrl->ShowWindow(SW_SHOW);
	}
	else if(cCtrl.sTipo.Compare(_T("List Control"))==0)
	{
		m_listctrl = new CListCtrl();

		rect = GetCtrlRect(LIST_CONTROL, iTopo, ratioXPos, cCtrl.iEsquerda);
		rect.bottom += (cCtrl.iLinhas-1) * DEFAULT_HEIGHT;
		y_atu = max(rect.bottom, y_atu);

		m_listctrl->Create(listctrl_style, rect, m_pThis, id+cCtrl.nID);
		_FormatList(cCtrl.nID);
		at->r = rect;
		at->tipoCtrl = LIST_CONTROL;
		at->wndControl = m_listctrl;
//			cCtrl.iAltura = DEFAULT_HEIGHT;
		//pDc->Rectangle(rect);
		//m_listctrl->ShowWindow(SW_SHOW);
	}
	else if(cCtrl.sTipo.Compare(_T("Image"))==0)
	{
#ifdef _WIN32_WCE
		m_image = new CDibControl();
#else
		m_image = new CPictureEx();
#endif

		rect = GetCtrlRect(LABEL, iTopo, ratioXPos, cCtrl.iEsquerda);
		if(cCtrl.iLargura > 100)
			rect.right = rect.left + cCtrl.iLargura;
		if(cCtrl.iAltura > -1)
			rect.bottom = rect.top + cCtrl.iAltura;
		else
			rect.bottom += (cCtrl.iLinhas-1) * DEFAULT_HEIGHT;

		y_atu = max(rect.bottom, y_atu);

#ifdef _WIN32_WCE
		m_image->Create(rect, m_pThis, id);
#else
		m_image->Create(L"", 0, rect, m_pThis, id);
#endif
		CUtil::GetPathFromVariable(cCtrl.sBDValue);

#ifdef _WIN32_WCE
		m_image->SetStretch(TRUE);
		m_image->LoadJPG(cCtrl.sBDValue);
#else
		m_image->SetStretch(TRUE);
		m_image->Load(cCtrl.sBDValue);
		m_image->Draw();
#endif

		at->r = rect;
		at->tipoCtrl = LABEL;
		at->wndControl = m_image;

		//m_image->ShowWindow(SW_SHOW);
	}
	else if(cCtrl.sTipo.Compare(_T("Custom"))==0)
	{
		rect = GetCtrlRect(LABEL, iTopo, ratioXPos);
		if(cCtrl.iLinhas == -1)
		{
			CRect r;
			m_pTrueThis->GetWindowRect(r);
#ifdef _WIN32_WCE
			rect.bottom = r.bottom-60;
			rect.right = r.right-30;
#else
			rect.bottom = r.bottom-100;
			rect.right = r.right-100;
#endif
		}
		else
		{
			rect.bottom += (cCtrl.iLinhas-1) * DEFAULT_HEIGHT;
		}
		y_atu = max(rect.bottom, y_atu);

		at->r = rect;
		at->tipoCtrl = CUSTOM;
		at->wndControl = NULL;
	}

	if(m_bShow && at->wndControl)
		at->wndControl->ShowWindow(SW_SHOW);

	CString t;
	t.Format(L"%s(%d): Id: %d Rect: T:%d L:%d R:%d B:%d.\r\n", CString(__FUNCTION__), __LINE__, cCtrl.nID, rect.top, rect.left, rect.right, rect.bottom);
	OutputDebugString(t);
}
int CRcGen::_BuildLabel(cControles* pCtrl, CTelaAtual* pTa, int& y, int& ratioXPos)
{
	if(pCtrl->sTipo.CompareNoCase(L"Radio") == 0 ||
		pCtrl->sTipo.CompareNoCase(L"Botão") == 0 ||
		pCtrl->sTipo.CompareNoCase(L"Check") == 0 ||
		pCtrl->sTipo.CompareNoCase(L"Label") == 0)
		return 0;

	CRect rect;
	//Se tiver label
	if(!pCtrl->sNome.IsEmpty())
	{
		rect = GetCtrlRect(LABEL_CTRL, y, ratioXPos);

		rect.bottom -= 4;

		/*if(pCtrl->sNome.GetLength() > 12)
		{
			rect.top = rect.top - 5;
			rect.bottom += ((pCtrl->sNome.GetLength()/12)+1) * DEFAULT_HEIGHT;
		}*/

		m_label = new CStatic();
		m_label->Create(pCtrl->sNome, 0, rect, m_pThis, 100);
		if(m_bShow)
			m_label->ShowWindow(SW_SHOW);
		pTa->pLabel = m_label;

		return rect.bottom - rect.top;
		//y = rect.bottom - rect.top + DEFAULT_SPACE;
	}

	return 0;
}

CRect CRcGen::GetCtrlRect(eTiposControle eTipo, int nTopo, int nDivisoes, int nEsquerda)
{
	//Tem label?
	static BOOL bLabel;
	//Próximo x a ser utilizado (No caso de manter a linha).
	static int nNextX;
	CRect r;
	//Espaço utilizado pelo controle
	static int space;
	//Tamanho da linha
	int nLineSize = DEFAULT_CTRL_WIDTH;
	static int countDiv = 0;

	if(nEsquerda > -1)
	{
		CString t;
		t.Format(L"%s(%d): Rect: T:%d L:%d R:%d B:%d\r\n", CString(__FUNCTION__), __LINE__, nTopo, nEsquerda, nEsquerda+20, nTopo+DEFAULT_HEIGHT);
		OutputDebugString(t);
		return CRect(nEsquerda, nTopo, nEsquerda+20, nTopo+DEFAULT_HEIGHT);
	}

	if(countDiv>=98)
	{
		countDiv = 0;
		nNextX = 0;
	}

#ifdef _WIN32_WCE
	const int nGroupSub = 0;
#else
	const int nGroupSub = 5;
	nLineSize -= nGroupSub;
#endif

	if(nDivisoes<100)
	{
		if(space==0) nNextX = 0;
		space = nLineSize * ((float)nDivisoes/100);
	}
	else if(nDivisoes>=98)
	{
		nNextX = 0;
		space = 0;
	}

	switch(eTipo)
	{
	case TEXTO:
	case DATA:
	case HORA:
	case LIST_CONTROL:
	case TEXTO_MULTILINHA:
	case COMBO:
	case COMBO_EDITAVEL:
	case BOTAO:
	case LABEL:
	case RADIO:
	case CHECK:
		r = CTRL_FULL_LINE;
		r.top = nTopo;
		r.left = ((bLabel ? DEFAULT_LBL_WIDTH : 0) + DEFAULT_SPACE + nNextX)+m_nXCol;
		r.right = ((nDivisoes < 100 ? space + nNextX : nLineSize)+m_nXCol);
		r.bottom = nTopo + DEFAULT_HEIGHT;
		nNextX = r.right-m_nXCol;
		countDiv += nDivisoes;
		bLabel = FALSE;
		break;
	case LABEL_CTRL:
		r = CTRL_FULL_LINE;
		r.top = nTopo;
		r.left = ((bLabel ? DEFAULT_LBL_WIDTH : 0) + DEFAULT_SPACE + nNextX)+m_nXCol;
		r.right = r.left + DEFAULT_CTRL_WIDTH - 6;
		r.bottom = nTopo + DEFAULT_HEIGHT;
		bLabel = FALSE;
		break;
	}

	return r;
}

void CRcGen::_deleteArrControles(BOOL bCtrl)
{
	bFlagDeletingArrControles = TRUE;

	CTelaAtual* cTa;
	POSITION p;
	CString sKey;

	p = mapTelaAtual.GetStartPosition();
	while(p)
	{		
		mapTelaAtual.GetNextAssoc(p, sKey, cTa);		
		if(cTa->wndControl) cTa->wndControl->DestroyWindow();
		if(cTa->pLabel) cTa->pLabel->DestroyWindow();
		//TRACE(L"APAGANDO CONTROLE: %s\r\n", arrControles[cTa->nIdCtrlArray].sID);
		delete cTa->wndControl;
		delete cTa;		
	}

	for(int i=0; i<arrGroupBox.GetSize(); i++)
	{
		m_button = arrGroupBox[i];
		delete m_button;
	}

	mapTelaAtual.RemoveAll();
	if(bCtrl) arrControles.RemoveAll();
	arrGroupBox.RemoveAll();

	bFlagDeletingArrControles = FALSE;
}

void CRcGen::Next()
{
	//Se acabou o grupo/paginamento começa um outro grupo
	if(m_nCtrl == arrControles.GetSize()-1)
	{
		Init();
	}
	else
	{
		m_y=DEFAULT_Y_INIT;
		m_x=0;
		m_nXCol=0;
		m_nCtrl++;
		_deleteArrControles(FALSE);
	}
}

void CRcGen::Previous()
{
	Init(FALSE);
}

void CRcGen::Move(int nGrupo)
{
	m_y=DEFAULT_Y_INIT;
	m_x=0;
	m_nXCol=0;
	m_nCtrl++;
	m_nGrupo = nGrupo;
#ifndef _WIN32_WCE
	Init();
#endif
}


int CRcGen::Aba2Grupo(int nAba)
{
#ifdef _WIN32_WCE
	m_nGrupo = m_nGrupoInicial + nAba;
	return m_nGrupo;
#else
	return m_nGrupoInicial + nAba;
#endif
}

int CRcGen::Grupo2Aba(int nGrupo)
{
	return nGrupo - m_nGrupoInicial;
}

void CRcGen::_SetMaxY()
{
	CRect rect;
#ifdef _WIN32_WCE
	SHSipPreference(m_pThis->GetSafeHwnd(), SIP_UP);
	SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, NULL);
	SHSipPreference(m_pThis->GetSafeHwnd(), SIP_FORCEDOWN);
	m_nMaxY = 300;
#endif
	if(m_nMaxY>0)
	{
		m_pThis->GetWindowRect(&rect);
		m_pThis->ScreenToClient(rect);
		//nMaxY = rect.bottom-35; //Desconta tamanho do header do tabcontrol
		//m_nMaxY = rect.right;
	}
}

int CRcGen::_GetMaxY()
{
	return m_nMaxY; 
}

void CRcGen::_FormatList(int nIdList)
{
	int iSize=0;
	CStringA sQuery;	
	CppSQLite3Query q;
	m_listctrl->SetExtendedStyle(LVS_EX_FULLROWSELECT);	

	try
	{
		sQuery.Format("select count(*) from reslists where IDList = %d", nIdList);

		q = m_pDB->execQuery(sQuery); 
		
		if(!q.eof())
		{
			q.finalize();

			sQuery.Format("select * from reslists where idlist = %d", nIdList);
			q = m_pDB->execQuery(sQuery);

			CRect r;
			m_listctrl->GetWindowRect(r);		

			//iSize = (r.right - r.left) / q.getIntField(0);
			iSize = q.getIntField("Largura");

			for(int i=0;!q.eof();i++)
			{
				//m_listctrl->InsertColumn(i, CString(q.getStringField("NomeColuna")), LVCFMT_LEFT, DRA::SCALEX(iSize));
				m_listctrl->InsertColumn(i, CString(q.getStringField("NomeColuna")), LVCFMT_LEFT, iSize);
				q.nextRow();
			}
			q.finalize();						
		}		
	}
	catch(CppSQLite3Exception e)
	{
	}
}

void CRcGen::Init(BOOL bUp)
{
	m_ResTable.Init(L"RES");
	CStringA sQuery;

	if(arrControles.GetSize() > 0)
	{
		_deleteArrControles();
	}

	m_nIdInicial = -1;

	_SetMaxY();
	m_nCtrl=0;
	m_nXCol=0;
	
	m_pDB = CppSQLite3DB::getInstance();
	CppSQLite3Query q;

	try
	{
	#ifdef _WIN32_WCE
		sQuery.Format("select * from res where grupo = %d", m_nGrupo);
	#else
		sQuery.Format("select * from res where grupo = %d", m_nGrupo);
	#endif
		//sQuery.Format("select * from res where id BETWEEN %d AND %d and grupo = %d", nCtrl+1, nCtrl+10, nGrupo);
		q = m_pDB->execQuery(sQuery); 

		if(!q.eof())
		{
			m_nGrupo = q.getIntField("Grupo");
		}

		while(!q.eof())
		{
			cControles cCtrl;
			
			cCtrl.nID = q.getIntField("ID");
			cCtrl.sTipo = q.getStringField("Tipo"); 
			cCtrl.iLargura = q.getIntField("Largura"); 
			cCtrl.iPagina = q.getIntField("Pagina"); 
			cCtrl.sNome = q.getStringField("Nome"); 
			cCtrl.sID = q.getStringField("IDStr"); 
			cCtrl.sDBRef.Format(L"%S.%S", q.getStringField("DBRef"), q.getStringField("DBFieldRef"));
			cCtrl.iSizeDb = q.getIntField("TamanhoCampo"); 
			cCtrl.sFill = q.getStringField("FillFrom"); 
			cCtrl.iExCtrl = q.getIntField("IsEx"); 
			cCtrl.iManterLinha = q.getIntField("Quebrar"); 
			cCtrl.iDesabilitado = q.getIntField("Desabilitar"); 
			cCtrl.iReadOnly = q.getIntField("Leitura"); 
			cCtrl.iObrigatorio = q.getIntField("Obrigatorio"); 			
			cCtrl.iGrupo = q.getIntField("Grupo"); 
			cCtrl.iSubGrupo = q.getIntField("SubGrupo");
			cCtrl.iLinhas = q.getIntField("Linhas");
			cCtrl.sBDValue = q.getStringField("BDValue");
			cCtrl.iAgrupado = q.getIntField("Agrupado");
			cCtrl.iTopo = q.getIntField("Topo");
			cCtrl.iEsquerda = q.getIntField("Esquerda");
			cCtrl.iAltura = q.getIntField("Altura");

			arrControles.Add(cCtrl);

			q.nextRow();

		}

		q.finalize();

	}
	catch(CppSQLite3Exception e)
	{
		
	}

	m_y=DEFAULT_Y_INIT;
	m_x=0;
	m_nSubGrupo=0;
}

void CRcGen::SendCreateDlg(WPARAM wParam, LPARAM lParam)
{
	m_pTrueThis->PostMessage(WM_NEW_INIT, wParam, lParam);
}

int CRcGen::_GetSubGrupoSize(int nSubGrupo, int nGrupo)
{
	CppSQLite3Query q;
	CStringA sQuery;

	int nLinhas=0;
	int nCountCtrl;

	try
	{
		if(nSubGrupo==-1)
			sQuery.Format("select linhas from res where grupo = %d", nGrupo);
		else
			sQuery.Format("select linhas from res where grupo = %d and subgrupo = %d", nGrupo, nSubGrupo);
			
		q = m_pDB->execQuery(sQuery);

		for(nCountCtrl=0;!q.eof();nCountCtrl++)
		{
			nLinhas += q.getIntField(0) == 0 ? 0 : q.getIntField(0)-1;
			q.nextRow();
		}

		if(nLinhas <= 0)
			return (nCountCtrl)*DEFAULT_HEIGHT + ((nCountCtrl - 1)*DEFAULT_SPACE);

		int size = (nLinhas + nCountCtrl) * DEFAULT_HEIGHT + ((nCountCtrl - 1) * DEFAULT_SPACE);

		return size;
	}
	catch(CppSQLite3Exception e)
	{
	
	}

	return 0;
}

int CRcGen::WhereControleIsVisible(int nIdCtrl)
{
	CStringA sQuery;

#ifdef _WIN32_WCE
	sQuery.Format("Select pagina from res where ID = %d", nIdCtrl);
#else
	sQuery.Format("Select grupo from res where ID = %d", nIdCtrl);
#endif

	try
	{
		CppSQLite3Query q = m_pDB->execQuery(sQuery);

		if(!q.eof())
			return q.getIntField(0);
	}
	catch(CppSQLite3Exception e)
	{
	
	}

	return -1;
}

void CRcGen::PutFocusOnControl(int nIdCtrl)
{
	CTelaAtual *pTa;
	cControles ctrl = arrControles[nIdCtrl - m_nIdInicial];
	
	mapTelaAtual.Lookup(ctrl.sID, pTa);
	CEGBComboBox* combo;
	
	if(pTa)
	{
		switch(pTa->tipoCtrl)
		{
			case COMBO:	
			case COMBO_EDITAVEL:
				combo = (CEGBComboBox*)pTa->wndControl;
				combo->SetFocus();
			break;
			
			default:
				pTa->wndControl->SetFocus();
			break;
		}
	}
}

//Método que monta as tab do Tab Control
void CRcGen::GetGrupos(WPARAM wParam, CStringArray& arrayGrp)
{
	CStringA sQuery;

	sQuery.Format("select distinct res.grupo, resgrupo.descricao from res, resgrupo where res.grupo = resgrupo.grupo and res.pagina between %d and %d", LOWORD(wParam), HIWORD(wParam));

	try
	{
		CppSQLite3Query q = CppSQLite3DB::getInstance()->execQuery(sQuery);

		if(!q.eof())
		{
			m_nGrupo = q.getIntField(0);
			m_nGrupoInicial = m_nGrupo;
		}

		while(!q.eof())
		{
			arrayGrp.Add(CString(q.getStringField(1)));
			q.nextRow();
		}
	}
	catch(CppSQLite3Exception e)
	{
	
	}
}

CString CRcGen::_GetSubGrupoDesc(const int nGrupo, const int nSubGrupo)
{
	CStringA sQuery;

	sQuery.Format("select descricao from RESSubGrupo where grupo = %d and subgrupo = %d", nGrupo, nSubGrupo);

	try
	{
		CppSQLite3Query q = CppSQLite3DB::getInstance()->execQuery(sQuery);

		if(q.eof())
			return L"";

		return CString(q.getStringField(0));
	}
	catch(CppSQLite3Exception e)
	{
	
	}
}

CString CRcGen::_GetGrupoDesc(const int nGrupo)
{
	CStringA sQuery;

	sQuery.Format("select descricao from RESGrupo where grupo = %d", nGrupo);

	try
	{
		CppSQLite3Query q = CppSQLite3DB::getInstance()->execQuery(sQuery);

		if(q.eof())
			return L"";

		return CString(q.getStringField(0));
	}
	catch(CppSQLite3Exception e)
	{
	
	}
}

#ifndef _WIN32_WCE
/////////////////////////CTabCtrl2

BEGIN_MESSAGE_MAP(CTabCtrl2, CTabCtrl)
ON_WM_CTLCOLOR()
ON_WM_CREATE()
END_MESSAGE_MAP()

static HBRUSH hBgBrush = 0;

HBRUSH CTabCtrl2::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CTabCtrl::OnCtlColor(pDC, pWnd, nCtlColor);

	TCHAR tName[256];
	GetClassName(pWnd->GetSafeHwnd(), tName, 256);

	if(hBgBrush && (nCtlColor==6 && CString(tName).CompareNoCase(L"edit")!=0))
	{
		CRect rc;

		pDC->SetBkMode(TRANSPARENT);
		GetWindowRect(rc);
		MapWindowPoints(this, rc);
		pDC->SetBrushOrg(-rc.left, -rc.top);

		return hBgBrush;
	}

	return hbr;
}

int CTabCtrl2::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CTabCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rc;
	GetWindowRect(rc);
	CDC* dc = GetDC();

	HDC hDCMem = CreateCompatibleDC(dc->GetSafeHdc());
	HBITMAP hBmp = CreateCompatibleBitmap(dc->GetSafeHdc(), rc.right - rc.left, rc.bottom - rc.top);
	HGDIOBJ hBmpOld = SelectObject(hDCMem, hBmp);

	SendMessage(WM_PRINTCLIENT, (WPARAM)hDCMem, (LPARAM)(PRF_ERASEBKGND | PRF_CLIENT | PRF_NONCLIENT));

	hBgBrush = CreatePatternBrush(hBmp);

	SelectObject(hDCMem, hBmpOld);

	DeleteObject(hBmp);
	DeleteDC(hDCMem);
	ReleaseDC(dc);	

	return 0;
}

#endif



CString CRcGen::GetCtrlValueByName(LPCTSTR szIdCtrl)
{
	CString sVal;
	int idx;
	CTelaAtual *pTa = NULL;
	mapTelaAtual.Lookup(CString(szIdCtrl), pTa);
	//cControles ctrl = arrControles[pTa->nIdCtrlArray];

	if(!pTa)
		return L"";

	CComboBox* cmbCtrl;
	CButton* button;

	switch(pTa->tipoCtrl)
	{
		case TEXTO:	
		case TEXTO_MULTILINHA:
			pTa->wndControl->GetWindowText(sVal);
			return sVal;
		break;		

		case COMBO:	
		case COMBO_EDITAVEL:
			cmbCtrl = (CComboBox*)pTa->wndControl;					
			idx = cmbCtrl->GetCurSel();

			if(idx != CB_ERR)
			{
				sVal.Format(L"%d", cmbCtrl->GetItemData(idx));
				return sVal;
			}
			else
				return L"";									
		break;	

		case CHECK:
		case RADIO:
			//sVal = ctrl.sBDValue;
			button = (CButton*)pTa->wndControl;
			if(button->GetCheck() == TRUE)
				return L"1";			
			else
				return L"0";			
		break;
	}	

	return L"";
}

void CRcGen::ResetCtrlByName(LPCTSTR szIdCtrl)
{	
	CTelaAtual *pTa = NULL;
	mapTelaAtual.Lookup(CString(szIdCtrl), pTa);	

	if(!pTa)
		return;

	switch(pTa->tipoCtrl)
	{
		case TEXTO:	
		case TEXTO_MULTILINHA:
			pTa->wndControl->SetWindowText(L"");			
		break;

		case COMBO:	
		case COMBO_EDITAVEL:
			CComboBox* cmbCtrl = (CComboBox*)pTa->wndControl;					
			cmbCtrl->SetCurSel(-1);			
		break;				
	}
}

int CRcGen::GetIdCtrlByName(LPCTSTR szIdCtrl)
{		
	CTelaAtual *pTa = NULL;
	if(mapTelaAtual.Lookup(CString(szIdCtrl), pTa))
	{
		cControles ctrl = arrControles[pTa->nIdCtrlArray];
		return ctrl.nID;
	}
	
	return -1;
}	

void CRcGen::EnableScreen(BOOL bFlag)
{
	CTelaAtual* pTa;
	POSITION p;
	CString sKey;
	
	cControles ctrl;
	p = mapTelaAtual.GetStartPosition();

	while(p)
	{		
		mapTelaAtual.GetNextAssoc(p, sKey, pTa);		
		
		if(!pTa)
			return;

		switch(pTa->tipoCtrl)
		{
			case TEXTO:	
			case TEXTO_MULTILINHA:
			case COMBO:	
			case COMBO_EDITAVEL:
			case DATA:
			case HORA:								
				ctrl = arrControles[pTa->nIdCtrlArray];				
				
				if(!ctrl.iDesabilitado)
				{ 
					if(!bFlag)
						ResetCtrlByName(ctrl.sID);
				
					pTa->wndControl->EnableWindow(bFlag);
				}
				
			break;
		}
	}
}
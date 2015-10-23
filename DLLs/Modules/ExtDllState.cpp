#include "StdAfx.h"
#include "ExtDllState.h"
#include "Modules.h"

HINSTANCE CEXTDLLState::m_hInst;
////////////////////////////////////////////////////////////////////////////////////////////
//File ExtDllState.cpp
//////////////////////////////////////////////////////////////////////////////////////////// 
CEXTDLLState::CEXTDLLState()
{
  m_hInstOld = AfxGetResourceHandle();
  AfxSetResourceHandle(m_hInst);
}
 
CEXTDLLState::~CEXTDLLState()
{
  AfxSetResourceHandle(m_hInstOld);
}
///////////////////////////////////////////////////////////////////////////////
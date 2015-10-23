// FastTemplateCls.cpp: implementation of the FastTemplate class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "FastTemplateCls.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

FastTemplate::FastTemplate()
{

}

FastTemplate::~FastTemplate()
{

}

void FastTemplate::define(CMapStringToString* fileList)
{
	POSITION pos;
	CString key;
	CString val;
	for(pos=fileList->GetStartPosition(); pos!=NULL;)
	{
		fileList->GetNextAssoc(pos,key,val);
		this->FileList.SetAt(key,val);		
	}
}

bool FastTemplate::define_dynamic(CString Macro, CString ParentName)
{
	this->Dynamic.SetAt(Macro, ParentName);
	if(!this->parse_dynamic(Macro))
		return false;
	return true;
}

void FastTemplate::assign(CMapStringToString *tplArray)
{
	POSITION pos;
	CString key;
	CString val;
	for(pos=tplArray->GetStartPosition(); pos!=NULL;)
	{
		tplArray->GetNextAssoc(pos,key,val);
		this->ParseVars.SetAt(key,val);		
	}
}

void FastTemplate::assign(CString tplArray, CString trailer)
{
	this->ParseVars.SetAt(tplArray,trailer);	
}

bool FastTemplate::parse(CString ReturnVar, CString FileTags)
{
	CString val, ret, newParent;
	CMapStringToString ret_val;
	bool append=false;
	val = FileTags;
	this->Last=ReturnVar;
	if(val.Left(1)==".")
	{
		append = true;
		val = val.Mid(1);
	}	

	if(!this->ParseVars.Lookup(val,ret))
	{
		if(this->Dynamic.Lookup(val,ret))
		{
			this->val = this->Content[val];
			if(this->val == "") return false;

			newParent = this->Content[ret];
			newParent.Replace(this->Content[val],ReturnVar);
			newParent.Replace(L"<!-- BEGIN DYNAMIC BLOCK: " + val + L" -->",_T(""));
			newParent.Replace(L"<!-- END DYNAMIC BLOCK: " + val + L" -->",_T(""));

			this->Content[ret] = newParent;
		}
		else
		{
			if(!this->Content.Lookup(val,ret))
			{
				this->Content[val] = this->get_template(this->FileList[val]);
				if(this->Content[val]=="") return false;
				this->val = this->Content[val];
			}
		}
	}
	else
		this->val = this->Content[val];
	this->parse_template(&this->val,&this->ParseVars);
	if(append)
		this->ReturnVars += this->val;
	else
		this->ReturnVars = this->val;
	
	ret_val.SetAt(ReturnVar, this->ReturnVars);
	this->assign(&ret_val);
	
	return true;
}

void FastTemplate::parse_template(CString* tpl, CMapStringToString *tpl_array)
{
	POSITION pos;
	CString key;
	CString val;
	for(pos=tpl_array->GetStartPosition(); pos!=NULL;)
	{
		tpl_array->GetNextAssoc(pos,key,val);
		tpl->Replace(key,val);
	}
}

bool FastTemplate::parse_dynamic(CString Macro)
{
	CString ParentTag, tpl, ret;
	CString newMacro, newParent;
	int inicio, fim;

	ParentTag = this->Dynamic[Macro];
	//if(this->ParentTag=="")
	if(!this->ParseVars.Lookup(ParentTag,ret))
	{
		if(!this->Content.Lookup(ParentTag,ret))
		{
			this->Content[ParentTag] = this->get_template(this->FileList[ParentTag]);
			if(this->Content[ParentTag]=="")
				return false;
		}
        this->ParseVars[ParentTag] = this->Content[ParentTag];
		this->Loaded.SetAt(ParentTag,_T("1"));
	}
	if(this->ParseVars.Lookup(ParentTag,ret))
	{
		tpl = this->ParseVars[ParentTag];
		inicio = tpl.Find(L"<!-- BEGIN DYNAMIC BLOCK: " + Macro + L" -->");
		fim = tpl.Find(L"<!-- END DYNAMIC BLOCK: " + Macro + L" -->");
		if(inicio == -1 || fim == -1)
		{
			//MessageBox(NULL,_T("Nenhum bloco dinâmico localizado para "+Macro),_T("Erro"),MB_OK);
			return false;
		}
		newMacro = tpl.Mid(inicio,fim-inicio);
		newMacro.Replace(L"<!-- BEGIN DYNAMIC BLOCK: " + Macro + L" -->",_T(""));
		newMacro.Replace(L"<!-- END DYNAMIC BLOCK: " + Macro + L" -->",_T(""));
	}
	this->Content[Macro] = newMacro;

	return true;
}

CString FastTemplate::get_template(CString tpl)
{
	
	CFile f;
	CFileException fe;
	CString fileName, contents;
	fileName = tpl;
	
	DWORD dwRead;
	char buffer[128];	
	
	if(!f.Open(fileName,CFile::modeRead,&fe))
	{
		STLOG_WRITE("FastTemplate::get_template() Erro abrindo template tpl. Error code: %ld", fe.m_cause);
		return L"";
	}

	do
	{	    
		dwRead = f.Read(buffer, 127);
		buffer[dwRead] = '\0';		

		CString tcont(buffer);
		
		//tcont=buffer;
		//CString tcont = CString(buffer);
		//contents+=tcont;

		contents.Append(tcont);		
	}
	while(dwRead>0);
	
	f.Close();
	
	return contents;
}

CString FastTemplate::FastPrint(CString tpl)
{
	CString ret;
	if(tpl=="")
		tpl = this->Last;

	if(this->ParseVars.Lookup(tpl,ret))
	{
		//MessageBox(NULL,this->ParseVars[tpl],_T("Mensagem"),MB_OK);
		return this->ParseVars[tpl];
	}

	//
	POSITION pos;
	CString key;
	CString val;
	for(pos=this->ParseVars.GetStartPosition(); pos!=NULL;)
	{
		this->ParseVars.GetNextAssoc(pos,key,val);
		//AfxMessageBox(key + L": " + val);
	}
	//

	return L"";
}

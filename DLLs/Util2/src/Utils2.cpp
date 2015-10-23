#include "StdAfx.h"
#include "Utils2.h"
#include "Blowfish.h"

CString CUtils2::sWrd = L"(0@&G%Y#>x27$+O9@K9)=r3}";

CUtils2::CUtils2(void)
{

}

CUtils2::~CUtils2(void)
{
}

LPCTSTR CUtils2::GetPwd()
{
	return sWrd;
}


/**
\brief Criptografa ou descriptografa buffers
\details Utiliza algoritmo Blowfish
\param char * BufEnt - buffer a ser processado
\param char * BufSai - buffer processado
\param char* Senha - senha a ser utilizada
\param BOOL decrypt - default FALSE - se presente e TRUE operação é de descriptografar
\return número de bytes do buffer de saída.
*/
int CUtils2::CriptoBuffer(char * BufEnt, char * BufSai, char* Senha, int bufSize, BOOL decrypt)
{
	//char * pass = "(0@&G%Y#>x27$+O9@K9)=r3}";
	const CStringA sW(CUtils2::GetPwd());

	BlowFishEnc encryption( sW );

	int encRet;
	if (decrypt)
	{
		encRet = encryption.decryptStream(BufEnt, (DWORD)bufSize, BufSai);
		int pos = 0;
		// removing trailing zeros - encrypted file must be x8 bytes.
		while ((pos < 8) && ((BufSai + (encRet-1) - pos)[0] == 0)) pos++;
		// if found trailing zeros - decreasing the writing buffer marker (not writing them).
		if (pos) encRet -= (pos);
	}
	else
	{
		encRet = encryption.encryptStream(BufEnt, (DWORD)bufSize, BufSai);
	}
	
	return encRet;
}
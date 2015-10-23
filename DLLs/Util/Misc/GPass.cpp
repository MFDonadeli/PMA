#include "StdAfx.h"
#include "gpass.h"
#include "crc_32.h"

GPass::GPass(void)
: nRand(-1)
{
	this->base     = 16;
	this->contrato = "NONE";
	//this->date     = 
	this->produto  = "NONE";
}

GPass::~GPass(void)
{
}

void GPass::setContrato(string contrato)
{
	this->contrato = contrato;
}

void GPass::setProduto(string produto)
{
	this->produto = produto;
}

void GPass::setDate(string date)
{
	this->date = date;
}

void GPass::setBase(int base)
{
	this->base = base;
}

void GPass::setRand(int nRand)
{
	this->nRand = nRand;
}

string GPass::makePassword(int value)
{
	CRC_32        crc;
	unsigned long val32;
	string        data;
	char          buffer[100];
	int           nrand;


	data.append(this->date);
	data.append(this->contrato);
	data.append(this->produto);

	if (this->nRand < 0)
		nrand = this->getRand();
	else
		nrand = this->nRand;

	sprintf(buffer, "%04i%02i", value, nrand);

	data.append(buffer);

	val32 = crc.CalcCRC((LPVOID) data.c_str(), (unsigned int)data.length(), 0);
	data  = _ultoa(val32, buffer, this->base);

	sprintf(buffer, "%02i", nrand);
	data.append(buffer);


	return(data);
}

string GPass::getHourPassword(int hour)
{
	return(this->makePassword(hour));
}

string GPass::getDayPassword(int day)
{
	return(this->makePassword(day));
}

string GPass::getHourDoublePassword(int hd)
{
	return(this->makePassword(hd));
}

string GPass::getHourParcialPassword(int hp)
{
	return(this->makePassword(hp));
}

int GPass::getRand()
{
	int value;

	#ifdef _WIN32_WCE
		CTime ltime;
		srand(ltime.GetTime());
	#else
		srand((unsigned int)time(NULL));
	#endif

	value = rand();

	return(value % 100);
}
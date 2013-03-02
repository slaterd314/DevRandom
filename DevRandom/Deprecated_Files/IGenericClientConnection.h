#pragma once
#ifndef __IGENERICCLIENTCONNECTION_H__
#define __IGENERICCLIENTCONNECTION_H__

#include "IThreadPoolItem.h"

class IGenericClientConnection : virtual public IThreadPoolItem
{
public:
	virtual ~IGenericClientConnection(){}
	virtual HANDLE handle()=0;
	virtual void setHandle(HANDLE h)=0;
	virtual LPOVERLAPPED overlapped()=0;
	virtual class IClientConnection *createNew()=0;
	virtual PTP_IO	pio()=0;
	virtual void setPio(PTP_IO p)=0;
};


#endif // __IGENERICCLIENTCONNECTION_H__

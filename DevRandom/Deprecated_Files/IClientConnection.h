#pragma once
#ifndef __ICLIENTCONNECTION_H__
#define __ICLIENTCONNECTION_H__

#include "IThreadPoolItem.h"

class IClientConnection : virtual public IThreadPoolItem
{
public:
	virtual ~IClientConnection(){}
	virtual HANDLE handle()=0;
	virtual LPOVERLAPPED overlapped()=0;
	virtual bool beginIo()=0;
	virtual bool ListenForNewConnection()=0;
	virtual IClientConnection *createNew()=0;
	virtual PTP_IO	pio()=0;
	virtual bool OnComplete(PTP_CALLBACK_INSTANCE Instance, PVOID Overlapped, ULONG IoResult, ULONG_PTR nBytesTransfered, PTP_IO pio)=0;
};


#endif //__ICLIENTCONNECTION_H__

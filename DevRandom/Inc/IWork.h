#pragma once
#ifndef __IWORK_H__
#define __IWORK_H__

#include "IThreadPoolItem.h"

class 
__declspec(novtable)
IWork : virtual public IThreadPoolItem
{
public:
	virtual bool Execute(PTP_CALLBACK_INSTANCE Instance)=0;
	virtual ~IWork(){}
	virtual PTP_WORK handle()=0;
};



#endif // __IWORK_H__

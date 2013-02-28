#pragma once
#ifndef __IWORK_H__
#define __IWORK_H__

#include "IThreadPoolItem.h"

class 
__declspec(novtable)
IWork : virtual public IThreadPoolItem
{
public:
	typedef ::std::function<void (PTP_CALLBACK_INSTANCE , IWork *)> FuncPtr;
	virtual void Execute(PTP_CALLBACK_INSTANCE Instance)=0;
	virtual void setWork(const FuncPtr &f)=0;
	virtual ~IWork(){}
	virtual PTP_WORK handle()=0;
};

typedef ::std::shared_ptr<IWork> IWorkPtr;

#endif // __IWORK_H__

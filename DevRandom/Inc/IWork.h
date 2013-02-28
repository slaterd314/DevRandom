#pragma once
#ifndef __IWORK_H__
#define __IWORK_H__

#include "IThreadPoolItem.h"
#include <memory>
#include <functional>

class 
__declspec(novtable)
IWork : virtual public IThreadPoolItem
{
public:
	typedef ::std::function<bool (PTP_CALLBACK_INSTANCE , IWork *)> FuncPtr;
	virtual bool Execute(PTP_CALLBACK_INSTANCE Instance)=0;
	virtual void setWork(const FuncPtr &f)=0;
	virtual ~IWork(){}
	virtual PTP_WORK handle()=0;
};

typedef ::std::shared_ptr<IWork> IWorkPtr;

#endif // __IWORK_H__

#pragma once
#ifndef __IWAIT_H__
#define __IWAIT_H__

#include "IThreadPoolItem.h"

class 
__declspec(novtable)
IWait : virtual public IThreadPoolItem
{
public:
	typedef ::std::function<void (PTP_CALLBACK_INSTANCE , TP_WAIT_RESULT , IWait *)> FuncPtr;
	virtual void Execute(PTP_CALLBACK_INSTANCE , TP_WAIT_RESULT)=0;
	virtual void setWait(const FuncPtr &f)=0;
	virtual ~IWait(){}
	virtual PTP_WAIT handle()=0;
};

typedef ::std::shared_ptr<IWait> IWaitPtr;

#endif // __IWAIT_H__

#pragma once
#ifndef __ITIMER_H__
#define __ITIMER_H__

#include "IThreadPoolItem.h"

class 
__declspec(novtable)
ITimer : virtual public IThreadPoolItem
{
public:
	typedef ::std::function<void (PTP_CALLBACK_INSTANCE, ITimer *)> FuncPtr;
	virtual void Execute(PTP_CALLBACK_INSTANCE)=0;
	virtual void setTimer(const FuncPtr &f)=0;
	virtual ~ITimer(){}
	virtual PTP_TIMER handle()=0;
};

typedef ::std::shared_ptr<ITimer> ITimerPtr;

#endif // __ITIMER_H__

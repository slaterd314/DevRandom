#pragma once
#ifndef __GENERICTIMER_H__
#define __GENERICTIMER_H__


#include "IThreadPoolItemImpl.h"
#include "ITimer.h"

class GenericTimer : public IThreadPoolItemImpl<ITimer>
{
	typedef IThreadPoolItemImpl<ITimer> super;
	FuncPtr m_func;
public:
	GenericTimer();
	virtual void Execute(PTP_CALLBACK_INSTANCE Instance);
	void setTimer(const FuncPtr &func);
	virtual PTP_TIMER handle();
	virtual bool SetThreadpoolTimer(PFILETIME pftDueTime, DWORD msPeriod, DWORD msWindowLength);
	virtual bool CloseTimer();
	virtual void WaitForCallbacks(BOOL bCancelPendingCallbacks);
};

#endif // __GENERICTIMER_H__

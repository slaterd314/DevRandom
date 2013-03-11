#pragma once
#ifndef __GENERICWAIT_H__
#define __GENERICWAIT_H__

#include "IWait.h"
#include <functional>
#include "IThreadPoolItemImpl.h"

class GenericWait : public IThreadPoolItemImpl<IWait>
{
	typedef IThreadPoolItemImpl<IWait> super;
	FuncPtr m_func;
public:
	GenericWait();
	virtual void Execute(PTP_CALLBACK_INSTANCE Instance, TP_WAIT_RESULT WaitResult);
	void setWait(const FuncPtr &func);
	virtual PTP_WAIT handle();
	virtual bool SetThreadpoolWait(HANDLE h, PFILETIME pftTimeout);
	virtual bool CloseWait();
	virtual void WaitForCallbacks(BOOL bCancelPendingCallbacks);
};

#endif // __GENERICWAIT_H__
